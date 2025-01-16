/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "POCOSignalRClient.h"

#include "CSP/CSPFoundation.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Web/HttpAuth.h"

#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/NetException.h>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

constexpr const size_t INITIAL_BUFFER_SIZE = 8192;
constexpr const size_t RECEIVE_BLOCK_SIZE = 4096;

namespace csp::multiplayer
{

CSPWebSocketClientPOCO::CSPWebSocketClientPOCO() noexcept
    : StopFlag(false)
    , PocoWebSocket(nullptr)
{
}

CSPWebSocketClientPOCO::~CSPWebSocketClientPOCO()
{
    // Block exit until receive and callback threads exits
    Stop(nullptr);
}

void CSPWebSocketClientPOCO::Start(const std::string& Url, CallbackHandler Callback)
{
    CSP_PROFILE_SCOPED();

    std::string endpoint = csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI.c_str();
    auto index = endpoint.find(':');
    std::string protocol = endpoint.substr(0, index);
    index += 3; // "://"
    auto endIndex = endpoint.find('/', index);
    std::string domain = endpoint.substr(index, endIndex - index);
    index = endIndex;
    std::string path = endpoint.substr(index);

    Poco::Net::HTTPClientSession* cs;

    if (protocol == "https")
    {
        cs = CSP_NEW Poco::Net::HTTPSClientSession(domain, 443);
    }
    else
    {
        cs = CSP_NEW Poco::Net::HTTPClientSession(domain, 80);
    }

    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
    Poco::Net::HTTPResponse response;

    if (csp::web::HttpAuth::GetAccessToken().c_str() != nullptr)
    {
        char Str[1024];
        snprintf(Str, 1024, "Bearer %s", csp::web::HttpAuth::GetAccessToken().c_str());
        request.set("Authorization", Str);
    }

    StopFlag = false;

    try
    {
        PocoWebSocket = CSP_NEW Poco::Net::WebSocket(*cs, request, response);
        // Receive worker thread
        ReceiveThread = std::thread([this]() { ReceiveThreadFunc(); });

        Callback(true);
    }
    catch (std::exception& e)
    {
        CSP_UNUSED(e);
        CSP_LOG_ERROR_FORMAT("Exception %s", e.what());

        Callback(false);
    }

    CSP_DELETE(cs);
}

void CSPWebSocketClientPOCO::Stop(CallbackHandler Callback)
{
    CSP_PROFILE_SCOPED();

    // Stop can be called from multiple threads
    Mutex.lock();

    if (PocoWebSocket && !StopFlag)
    {
        StopFlag = true;

        // We need to unlock here to prevent a deadlock
        // If the ReceiveThread is locked then the other thread will never finish because itd will be waiting for the ReceiveThread to join
        Mutex.unlock();

        // POCO doesn't like close being called in the middle of
        // receiveFrame, so wait for receive thread to close
        if (std::this_thread::get_id() != ReceiveThread.get_id())
        {
            ReceiveThread.join();
        }
        else
        {
            // Safe to detach here as we know no socket polling will be done if we get here, as this is being called from the callback that was passed
            // to Receive
            ReceiveThread.detach();
        }

        try
        {
            PocoWebSocket->close();
        }
        catch (const std::exception&)
        {
            CSP_LOG_ERROR_FORMAT("%s", "Error: Failed to close socket.");
        }

        CSP_DELETE(PocoWebSocket);
        PocoWebSocket = nullptr;
    }
    else
    {
        Mutex.unlock();
    }

    if (Callback)
    {
        Callback(true);
    }
}

void CSPWebSocketClientPOCO::Send(const std::string& Message, CallbackHandler Callback)
{
    CSP_PROFILE_SCOPED();

    assert(PocoWebSocket && "Web socket not created! Please call Start() before calling Send().");

    int Flags = Poco::Net::WebSocket::SendFlags::FRAME_BINARY; // Assume binary as we don't support JSON anymore
    auto Remaining = Message.size();
    auto Succeeded = true;

    try
    {
        while (Remaining > 0)
        {
            auto SentCount = PocoWebSocket->sendFrame(Message.data(), static_cast<int>(Message.size()), Flags);

            Remaining -= SentCount;

            if (SentCount <= 0)
            {
                Succeeded = false;
                break;
            }
        }
    }
    catch (const std::exception&)
    {
        CSP_LOG_ERROR_FORMAT("%s", "Error: Failed to send data to socket.");
    }

    Callback(Succeeded);
}

void CSPWebSocketClientPOCO::Receive(ReceiveHandler Callback)
{
    CSP_PROFILE_SCOPED();

    if (!StopFlag)
    {
        assert(PocoWebSocket && "Web socket not created! Please call Start() before calling Receive().");

        ReceiveCallback = Callback;
        ReceiveReady = true;
    }
    else if (Callback)
    {
        Callback("", false);
    }
}

void CSPWebSocketClientPOCO::ReceiveThreadFunc()
{
    bool HandshakeReceived = false;
    auto* Buffer = (char*)CSP_ALLOC(INITIAL_BUFFER_SIZE);
    auto CurrentBufferSize = INITIAL_BUFFER_SIZE;
    auto CurrentBufferIndex = 0;
    auto SkipWait = false;
    auto SocketPollTimeout = Poco::Timespan(1000);
    auto ShouldRead = true;

    for (;;)
    {
        CSP_PROFILE_SCOPED();

        if (StopFlag)
        {
            return;
        }

        if (!SkipWait)
        {
            while (!ReceiveReady)
            {
                std::this_thread::sleep_for(500ns);

                if (StopFlag)
                {
                    return;
                }
            }
        }

        SkipWait = false;

        if (ShouldRead)
        {
            // Resize buffer if needed
            if (CurrentBufferIndex + RECEIVE_BLOCK_SIZE > CurrentBufferSize)
            {
                auto* NewBuffer = CSP_REALLOC(Buffer, CurrentBufferSize * 2);
                Buffer = static_cast<char*>(NewBuffer);
                CurrentBufferSize = CurrentBufferSize * 2;
                CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Resizing receive buffer to %d", CurrentBufferSize);
            }

            try
            {
                while (!PocoWebSocket->poll(SocketPollTimeout, Poco::Net::WebSocket::SELECT_READ))
                {
                    std::this_thread::sleep_for(500ns);

                    if (StopFlag)
                    {
                        return;
                    }
                }
            }
            catch (const std::exception& e)
            {
                CSP_FREE(Buffer);
                HandleReceiveError(e.what());

                return;
            }

            int Flags;
            int Received = 0;

            try
            {
                Received = PocoWebSocket->receiveFrame(Buffer + CurrentBufferIndex, RECEIVE_BLOCK_SIZE, Flags);
            }
            catch (const std::exception& e)
            {
                CSP_FREE(Buffer);
                HandleReceiveError(e.what());

                return;
            }

            if (Received == 0)
            {
                CSP_FREE(Buffer);
                HandleReceiveError("Error: Socket closed by remote host.");

                return;
            }

            assert(!(Flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_TEXT) && "The JSON hub protocol is currently not supported!");

            if (Flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_CLOSE)
            {
                CSP_FREE(Buffer);
                HandleReceiveError("Error: Socket closed.");

                return;
            }

            CurrentBufferIndex += Received;
        }

        if (StopFlag)
        {
            return;
        }

        // Handshake needs to be handled differently as it is in JSON format
        if (!HandshakeReceived)
        {
            int i;

            for (i = 0; i < CurrentBufferIndex; ++i)
            {
                // JSON messages are terminated with the 0x1E character
                if (Buffer[i] == 0x1E)
                {
                    break;
                }
            }

            // Read more data if we haven't found the message terminator yet
            if (i == CurrentBufferIndex)
            {
                SkipWait = true;
                ShouldRead = true;
                continue;
            }

            std::string CallbackMessage(Buffer, i + 1);

            if (i < (CurrentBufferIndex - (i + 1)))
            {
                auto Remaining = CurrentBufferIndex - (i + 1);
                memmove(Buffer, Buffer + i + 1, Remaining);
                CurrentBufferIndex = Remaining;
                ShouldRead = false;
            }
            else
            {
                CurrentBufferIndex = 0;
                ShouldRead = true;
            }

            HandshakeReceived = true;
            ReceiveReady = false;
            ReceiveCallback(CallbackMessage, true);

            continue;
        }

        /*
         * TODO: Make sure we have enough bytes for length
         * Max number of bytes for message length is 5. Let's ensure we have at least 5 before we try to read length.
         */

        auto Length = 0;
        int i;

        for (i = 0; i < 5; ++i)
        {
            Length |= (Buffer[i] & 0x7F) << (i * 7);

            if ((Buffer[i] & 0x80) == 0)
            {
                ++i;
                break;
            }
        }

        // Continue reading if we don't have the entire message yet
        if (Length > CurrentBufferIndex - i)
        {
            SkipWait = true;
            ShouldRead = true;
            continue;
        }

        /*
         * TODO: Look into modifying SignalR to process byte arrays instead of strings, and re-use a buffer for holding the
         *  message to be processed
         */
        std::string CallbackMessage(Buffer, Length + i);
        ReceiveReady = false;

        // Move remaining data to beginning of buffer
        if (Length < CurrentBufferIndex - i)
        {
            auto Remaining = (CurrentBufferIndex - i) - Length;
            memmove(Buffer, Buffer + Length + i, Remaining);
            CurrentBufferIndex = Remaining;
            // If we have remaining data, we should try to process it the next time Receive is called without reading from the socket
            ShouldRead = false;
        }
        else
        {
            CurrentBufferIndex = 0;
            ShouldRead = true;
        }

        auto Callback = ReceiveCallback;
        Callback(CallbackMessage, true);
    }

    StopFlag = false;
}

void CSPWebSocketClientPOCO::HandleReceiveError(const std::string& Message)
{
    CSP_LOG_ERROR_MSG(Message.c_str());

    if (ReceiveCallback)
    {
        ReceiveCallback("", false);
        ReceiveCallback = nullptr;
    }
}

} // namespace csp::multiplayer
