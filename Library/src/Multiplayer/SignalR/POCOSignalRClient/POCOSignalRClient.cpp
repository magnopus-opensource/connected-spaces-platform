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
#ifndef CSP_WASM

#include "POCOSignalRClient.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>
#include <chrono>
#include <fmt/format.h>
#include <stdexcept>
#include <thread>

// For the profiling stuff ... needs broken, or just removed.
#include "Debug/Logging.h"

using namespace std::chrono_literals;

constexpr const size_t INITIAL_BUFFER_SIZE = 8192;
constexpr const size_t RECEIVE_BLOCK_SIZE = 4096;

namespace csp::multiplayer
{

CSPWebSocketClientPOCO::CSPWebSocketClientPOCO(
    const std::string& MultiplayerUri, const std::string& AccessToken, const std::string& DeviceId, csp::common::LogSystem& LogSystem) noexcept
    : PocoWebSocket(nullptr)
    , StopFlag(false)
    , MultiplayerUri { MultiplayerUri }
    , AccessToken { AccessToken }
    , DeviceId { DeviceId }
    , LogSystem(LogSystem)
{
}

CSPWebSocketClientPOCO::~CSPWebSocketClientPOCO()
{
    // Block exit until receive and callback threads exits
    Stop(nullptr);
}

CSPWebSocketClientPOCO::ParsedURIInfo CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(const std::string& MultiplayerServiceUriEndpoint)
{
    Poco::URI EndpointURI { MultiplayerServiceUriEndpoint.c_str() };

    // It's common folk may provide a "localhost:port/path/path" sort of string, with omits the protocol, just make sure one's there.
    if ((EndpointURI.getScheme() != "https") && (EndpointURI.getScheme() != "http"))
    {
        throw std::runtime_error("CSPWebSocketclientPOCO::ParsedURIInfo, Expected `https` or `http` scheme, found neither.");
    }

    CSPWebSocketClientPOCO::ParsedURIInfo Out;
    Out.Endpoint = EndpointURI.toString();
    Out.Protocol = EndpointURI.getScheme();
    Out.Domain = EndpointURI.getHost();
    Out.Path = EndpointURI.getPath();
    Out.Port = EndpointURI.getPort();

    return Out;
}

void CSPWebSocketClientPOCO::Start(const std::string& /*Url*/, CallbackHandler Callback)
{
    CSP_PROFILE_SCOPED();

    try
    {
        CSPWebSocketClientPOCO::ParsedURIInfo ParsedEndpoint = ParseMultiplayerServiceUriEndPoint(MultiplayerUri);

        auto domain = ParsedEndpoint.Domain;
        auto protocol = ParsedEndpoint.Protocol;
        auto path = ParsedEndpoint.Path;
        auto endpoint = ParsedEndpoint.Endpoint;
        auto port = ParsedEndpoint.Port;

        std::unique_ptr<Poco::Net::HTTPClientSession> cs;

        if (protocol == "https")
        {
            cs = std::make_unique<Poco::Net::HTTPSClientSession>(domain, port);
        }
        else
        {
            cs = std::make_unique<Poco::Net::HTTPClientSession>(domain, port);
        }

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
        Poco::Net::HTTPResponse response;

        char Str[1024];
        snprintf(Str, 1024, "Bearer %s", AccessToken.c_str());
        request.set("Authorization", Str);

        // Ensures another thread does not try to use the PocoWebSocket pointer (for Send or Stop) while this thread is creating it.
        {
            std::scoped_lock<std::mutex> Lock(PocoWebSocketMutex);
            PocoWebSocket = new Poco::Net::WebSocket(*cs, request, response);
            StopFlag = false;
        }

        // Receive worker thread
        ReceiveThread = std::thread([this]() { ReceiveThreadFunc(); });

        Callback(true);
    }
    catch (std::exception& e)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, e.what());

        Callback(false);
    }
}

void CSPWebSocketClientPOCO::Stop(CallbackHandler Callback)
{
    CSP_PROFILE_SCOPED();

    // Guards PocoWebSocket and the StopFlag against concurrent Start/Send/Stop.
    std::unique_lock<std::mutex> Lock(PocoWebSocketMutex);

    if (!PocoWebSocket || StopFlag)
    {
        Lock.unlock();
        if (Callback)
        {
            Callback(true);
        }

        return;
    }

    StopFlag = true;

    // We need to unlock here to prevent a deadlock.
    // If the ReceiveThread is locked then the other thread will never finish because it will be waiting for the ReceiveThread to join.
    Lock.unlock();

    // POCO doesn't like close being called in the middle of receiveFrame, so we wait for receive thread to close.
    // This gets called from websocket_transport::receive_loop() if an exception is thrown.
    // ScopeLeadershipManager::SendHeartbeatIfElectedScopeLeader and entity creation both use the main thread via CSPFoundation::Tick() which can
    // result in this being called on the main thread.
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

    // Ensures that another thread is not trying to call Send() while this thread is destroying the PocoWebSocket.
    Lock.lock();

    try
    {
        PocoWebSocket->close();
    }
    catch (const Poco::Net::InvalidSocketException& InvalidSocketEx)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("Failed to close socket with InvalidSocketException. Code: {}, Message: \"{}\", DisplayText: \"{}\"", InvalidSocketEx.code(),
                InvalidSocketEx.message(), InvalidSocketEx.displayText())
                .c_str());
    }
    catch (const Poco::Exception& PocoEx)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("Failed to close socket with exception: {}. Code: {}, Message: \"{}\", DisplayText: \"{}\"", PocoEx.className(),
                PocoEx.code(), PocoEx.message(), PocoEx.displayText())
                .c_str());
    }
    catch (const std::exception& e)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to close socket. std::exception: {}", e.what()).c_str());
    }

    delete (PocoWebSocket);
    PocoWebSocket = nullptr;

    Lock.unlock();

    if (Callback)
    {
        Callback(true);
    }
}

void CSPWebSocketClientPOCO::Send(const std::string& Message, CallbackHandler Callback)
{
    CSP_PROFILE_SCOPED();

    // Ensures that another thread is not trying to call Stop() and destroy the PocoWebSocket while this thread is calling Send().
    // It also ensures that only one one thread at a time is able to write to the socket. Previously this was causing a POCO SSLException
    // (ssl3_write_bytes: bad length) to be thrown in clients which resulted in a corrupted SSL session and a websocket disconnect.
    std::unique_lock<std::mutex> Lock(PocoWebSocketMutex);

    if (StopFlag)
    {
        Lock.unlock();

        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "Multiplayer web socket connection is being stopped, aborting Send operation.");
        if (Callback)
        {
            Callback(false);
        }
        return;
    }

    if (!PocoWebSocket)
    {
        Lock.unlock();

        LogSystem.LogMsg(csp::common::LogLevel::Error, "Web socket not created! Please call Start() before calling Send().");
        if (Callback)
        {
            Callback(false);
        }
        return;
    }

    int Flags = Poco::Net::WebSocket::SendFlags::FRAME_BINARY; // Assume binary as we don't support JSON anymore
    auto Length = static_cast<int>(Message.size());
    auto Succeeded = true;

    try
    {
        if (Length > 0)
        {
            // The websocket is configured to be blocking (SocketImpl::SocketImpl() : _blocking(true)). This means this call will block until all data
            // has been sent. If a failure occurs this method will throw an exception.
            Succeeded = PocoWebSocket->sendFrame(Message.data(), Length, Flags) == Length;
        }
    }
    catch (const Poco::Net::InvalidSocketException& InvalidSocketEx)
    {
        Succeeded = false;

        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("InvalidSocketException sending data to socket. Code: {}, Message: \"{}\", DisplayText: \"{}\"", InvalidSocketEx.code(),
                InvalidSocketEx.message(), InvalidSocketEx.displayText())
                .c_str());
    }
    catch (const Poco::Exception& PocoEx)
    {
        Succeeded = false;

        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("Exception sending data to socket: {}. Code: {}, Message: \"{}\", DisplayText: \"{}\"", PocoEx.className(), PocoEx.code(),
                PocoEx.message(), PocoEx.displayText())
                .c_str());
    }
    catch (const std::exception& e)
    {
        Succeeded = false;

        LogSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("std::exception during Send(): {}", e.what()).c_str());
    }

    Lock.unlock();

    if (Callback)
    {
        Callback(Succeeded);
    }
}

void CSPWebSocketClientPOCO::Receive(ReceiveHandler Callback)
{
    CSP_PROFILE_SCOPED();

    if (!Callback)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Receive() called with a null callback, aborting Receive operation.");
        return;
    }

    std::unique_lock<std::mutex> Lock(PocoWebSocketMutex);

    if (StopFlag)
    {
        Lock.unlock();

        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "Multiplayer web socket connection is being stopped, aborting Receive operation.");
        Callback("", false);
        return;
    }

    if (!PocoWebSocket)
    {
        Lock.unlock();

        LogSystem.LogMsg(csp::common::LogLevel::Error, "Web socket not created! Please call Start() before calling Receive().");
        Callback("", false);
        return;
    }

    Lock.unlock();

    ReceiveCallback = Callback;
    ReceiveReady = true;
}

void CSPWebSocketClientPOCO::__CauseFailure() { HandleReceiveError("__CauseFailure"); }

void CSPWebSocketClientPOCO::ReceiveThreadFunc()
{
    bool HandshakeReceived = false;
    auto* Buffer = (char*)std::malloc(INITIAL_BUFFER_SIZE);
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
                auto* NewBuffer = std::realloc(Buffer, CurrentBufferSize * 2);
                Buffer = static_cast<char*>(NewBuffer);
                CurrentBufferSize = CurrentBufferSize * 2;
                LogSystem.LogMsg(csp::common::LogLevel::Log, fmt::format("Resizing receive buffer to {}", CurrentBufferSize).c_str());
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
            catch (const Poco::Net::InvalidSocketException& InvalidSocketEx)
            {
                std::free(Buffer);
                HandleReceiveError(fmt::format("InvalidSocketException during poll(). Code: {}, Message: \"{}\", DisplayText: \"{}\"",
                    InvalidSocketEx.code(), InvalidSocketEx.message(), InvalidSocketEx.displayText()));

                return;
            }
            catch (const Poco::Exception& PocoEx)
            {
                std::free(Buffer);
                HandleReceiveError(fmt::format("Exception during poll(): {}. Code: {}, Message: \"{}\", DisplayText: \"{}\"", PocoEx.className(),
                    PocoEx.code(), PocoEx.message(), PocoEx.displayText()));

                return;
            }
            catch (const std::exception& e)
            {
                std::free(Buffer);
                HandleReceiveError(fmt::format("std::exception during poll(): {}", e.what()));

                return;
            }

            int Flags;
            int Received = 0;

            try
            {
                Received = PocoWebSocket->receiveFrame(Buffer + CurrentBufferIndex, RECEIVE_BLOCK_SIZE, Flags);
            }
            catch (const Poco::Net::InvalidSocketException& InvalidSocketEx)
            {
                std::free(Buffer);
                HandleReceiveError(fmt::format("InvalidSocketException during receiveFrame(). Code: {}, Message: \"{}\", DisplayText: \"{}\"",
                    InvalidSocketEx.code(), InvalidSocketEx.message(), InvalidSocketEx.displayText()));

                return;
            }
            catch (const Poco::Exception& PocoEx)
            {
                std::free(Buffer);
                HandleReceiveError(fmt::format("Exception during receiveFrame(): {}. Code: {}, Message: \"{}\", DisplayText: \"{}\"",
                    PocoEx.className(), PocoEx.code(), PocoEx.message(), PocoEx.displayText()));

                return;
            }
            catch (const std::exception& e)
            {
                std::free(Buffer);
                HandleReceiveError(fmt::format("std::exception during receiveFrame(): {}", e.what()));

                return;
            }

            if (Received == 0)
            {
                std::free(Buffer);
                HandleReceiveError("Error: Socket closed by remote host.");

                return;
            }

            assert(!(Flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_TEXT) && "The JSON hub protocol is currently not supported!");

            if (Flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_CLOSE)
            {
                std::free(Buffer);
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
}

void CSPWebSocketClientPOCO::HandleReceiveError(const std::string& Message)
{
    LogSystem.LogMsg(csp::common::LogLevel::Error, Message.c_str());

    if (ReceiveCallback)
    {
        ReceiveCallback("", false);
        ReceiveCallback = nullptr;
    }
}

} // namespace csp::multiplayer
#endif
