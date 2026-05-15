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
    const std::string& multiplayerUri, const std::string& accessToken, const std::string& deviceId, csp::common::LogSystem& logSystem) noexcept
    : m_pocoWebSocket(nullptr)
    , m_stopFlag(false)
    , m_multiplayerUri { multiplayerUri }
    , m_accessToken { accessToken }
    , m_deviceId { deviceId }
    , m_logSystem(logSystem)
{
}

CSPWebSocketClientPOCO::~CSPWebSocketClientPOCO()
{
    // Block exit until receive and callback threads exits
    Stop(nullptr);
}

CSPWebSocketClientPOCO::ParsedURIInfo CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(const std::string& multiplayerServiceUriEndpoint)
{
    Poco::URI endpointUri { multiplayerServiceUriEndpoint.c_str() };

    // It's common folk may provide a "localhost:port/path/path" sort of string, with omits the protocol, just make sure one's there.
    if ((endpointUri.getScheme() != "https") && (endpointUri.getScheme() != "http"))
    {
        throw std::runtime_error("CSPWebSocketclientPOCO::ParsedURIInfo, Expected `https` or `http` scheme, found neither.");
    }

    CSPWebSocketClientPOCO::ParsedURIInfo out;
    out.Endpoint = endpointUri.toString();
    out.Protocol = endpointUri.getScheme();
    out.Domain = endpointUri.getHost();
    out.Path = endpointUri.getPath();
    out.Port = endpointUri.getPort();

    return out;
}

void CSPWebSocketClientPOCO::Start(const std::string& /*Url*/, CallbackHandler callback)
{
    CSP_PROFILE_SCOPED();

    try
    {
        CSPWebSocketClientPOCO::ParsedURIInfo parsedEndpoint = ParseMultiplayerServiceUriEndPoint(m_multiplayerUri);

        auto domain = parsedEndpoint.Domain;
        auto protocol = parsedEndpoint.Protocol;
        auto path = parsedEndpoint.Path;
        auto endpoint = parsedEndpoint.Endpoint;
        auto port = parsedEndpoint.Port;

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

        char str[1024];
        snprintf(str, 1024, "Bearer %s", m_accessToken.c_str());
        request.set("Authorization", str);

        m_stopFlag = false;

        m_pocoWebSocket = new Poco::Net::WebSocket(*cs, request, response);
        // Receive worker thread
        m_receiveThread = std::thread([this]() { ReceiveThreadFunc(); });

        callback(true);
    }
    catch (std::exception& e)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, e.what());

        callback(false);
    }
}

void CSPWebSocketClientPOCO::Stop(CallbackHandler callback)
{
    CSP_PROFILE_SCOPED();

    // Stop can be called from multiple threads
    m_mutex.lock();

    if (m_pocoWebSocket && !m_stopFlag)
    {
        m_stopFlag = true;

        // We need to unlock here to prevent a deadlock
        // If the ReceiveThread is locked then the other thread will never finish because itd will be waiting for the ReceiveThread to join
        m_mutex.unlock();

        // POCO doesn't like close being called in the middle of
        // receiveFrame, so wait for receive thread to close
        if (std::this_thread::get_id() != m_receiveThread.get_id())
        {
            m_receiveThread.join();
        }
        else
        {
            // Safe to detach here as we know no socket polling will be done if we get here, as this is being called from the callback that was passed
            // to Receive
            m_receiveThread.detach();
        }

        try
        {
            m_pocoWebSocket->close();
        }
        catch (const std::exception&)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error, "Error: Failed to close socket.");
        }

        delete (m_pocoWebSocket);
        m_pocoWebSocket = nullptr;
    }
    else
    {
        m_mutex.unlock();
    }

    if (callback)
    {
        callback(true);
    }
}

void CSPWebSocketClientPOCO::Send(const std::string& message, CallbackHandler callback)
{
    CSP_PROFILE_SCOPED();

    assert(m_pocoWebSocket && "Web socket not created! Please call Start() before calling Send().");

    int flags = Poco::Net::WebSocket::SendFlags::FRAME_BINARY; // Assume binary as we don't support JSON anymore
    auto remaining = message.size();
    auto succeeded = true;

    try
    {
        while (remaining > 0)
        {
            auto sentCount = m_pocoWebSocket->sendFrame(message.data(), static_cast<int>(message.size()), flags);

            remaining -= sentCount;

            if (sentCount <= 0)
            {
                succeeded = false;
                break;
            }
        }
    }
    catch (const std::exception&)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "Error: Failed to send data to socket.");
    }

    callback(succeeded);
}

void CSPWebSocketClientPOCO::Receive(ReceiveHandler callback)
{
    CSP_PROFILE_SCOPED();

    if (!m_stopFlag)
    {
        assert(m_pocoWebSocket && "Web socket not created! Please call Start() before calling Receive().");

        m_receiveCallback = callback;
        m_receiveReady = true;
    }
    else if (callback)
    {
        callback("", false);
    }
}

void CSPWebSocketClientPOCO::__CauseFailure() { HandleReceiveError("__CauseFailure"); }

void CSPWebSocketClientPOCO::ReceiveThreadFunc()
{
    bool handshakeReceived = false;
    auto* buffer = (char*)std::malloc(INITIAL_BUFFER_SIZE);
    auto currentBufferSize = INITIAL_BUFFER_SIZE;
    auto currentBufferIndex = 0;
    auto skipWait = false;
    auto socketPollTimeout = Poco::Timespan(1000);
    auto shouldRead = true;

    for (;;)
    {
        CSP_PROFILE_SCOPED();

        if (m_stopFlag)
        {
            return;
        }

        if (!skipWait)
        {
            while (!m_receiveReady)
            {
                std::this_thread::sleep_for(500ns);

                if (m_stopFlag)
                {
                    return;
                }
            }
        }

        skipWait = false;

        if (shouldRead)
        {
            // Resize buffer if needed
            if (currentBufferIndex + RECEIVE_BLOCK_SIZE > currentBufferSize)
            {
                auto* newBuffer = std::realloc(buffer, currentBufferSize * 2);
                buffer = static_cast<char*>(newBuffer);
                currentBufferSize = currentBufferSize * 2;
                m_logSystem.LogMsg(csp::common::LogLevel::Log, fmt::format("Resizing receive buffer to {}", currentBufferSize).c_str());
            }

            try
            {
                while (!m_pocoWebSocket->poll(socketPollTimeout, Poco::Net::WebSocket::SELECT_READ))
                {
                    std::this_thread::sleep_for(500ns);

                    if (m_stopFlag)
                    {
                        return;
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::free(buffer);
                HandleReceiveError(e.what());

                return;
            }

            int flags;
            int received = 0;

            try
            {
                received = m_pocoWebSocket->receiveFrame(buffer + currentBufferIndex, RECEIVE_BLOCK_SIZE, flags);
            }
            catch (const std::exception& e)
            {
                std::free(buffer);
                HandleReceiveError(e.what());

                return;
            }

            if (received == 0)
            {
                std::free(buffer);
                HandleReceiveError("Error: Socket closed by remote host.");

                return;
            }

            assert(!(flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_TEXT) && "The JSON hub protocol is currently not supported!");

            if (flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_CLOSE)
            {
                std::free(buffer);
                HandleReceiveError("Error: Socket closed.");

                return;
            }

            currentBufferIndex += received;
        }

        if (m_stopFlag)
        {
            return;
        }

        // Handshake needs to be handled differently as it is in JSON format
        if (!handshakeReceived)
        {
            int i;

            for (i = 0; i < currentBufferIndex; ++i)
            {
                // JSON messages are terminated with the 0x1E character
                if (buffer[i] == 0x1E)
                {
                    break;
                }
            }

            // Read more data if we haven't found the message terminator yet
            if (i == currentBufferIndex)
            {
                skipWait = true;
                shouldRead = true;
                continue;
            }

            std::string callbackMessage(buffer, i + 1);

            if (i < (currentBufferIndex - (i + 1)))
            {
                auto remaining = currentBufferIndex - (i + 1);
                memmove(buffer, buffer + i + 1, remaining);
                currentBufferIndex = remaining;
                shouldRead = false;
            }
            else
            {
                currentBufferIndex = 0;
                shouldRead = true;
            }

            handshakeReceived = true;
            m_receiveReady = false;
            m_receiveCallback(callbackMessage, true);

            continue;
        }

        /*
         * TODO: Make sure we have enough bytes for length
         * Max number of bytes for message length is 5. Let's ensure we have at least 5 before we try to read length.
         */

        auto length = 0;
        int i;

        for (i = 0; i < 5; ++i)
        {
            length |= (buffer[i] & 0x7F) << (i * 7);

            if ((buffer[i] & 0x80) == 0)
            {
                ++i;
                break;
            }
        }

        // Continue reading if we don't have the entire message yet
        if (length > currentBufferIndex - i)
        {
            skipWait = true;
            shouldRead = true;
            continue;
        }

        /*
         * TODO: Look into modifying SignalR to process byte arrays instead of strings, and re-use a buffer for holding the
         *  message to be processed
         */
        std::string callbackMessage(buffer, length + i);
        m_receiveReady = false;

        // Move remaining data to beginning of buffer
        if (length < currentBufferIndex - i)
        {
            auto remaining = (currentBufferIndex - i) - length;
            memmove(buffer, buffer + length + i, remaining);
            currentBufferIndex = remaining;
            // If we have remaining data, we should try to process it the next time Receive is called without reading from the socket
            shouldRead = false;
        }
        else
        {
            currentBufferIndex = 0;
            shouldRead = true;
        }

        auto callback = m_receiveCallback;
        callback(callbackMessage, true);
    }
}

void CSPWebSocketClientPOCO::HandleReceiveError(const std::string& message)
{
    m_logSystem.LogMsg(csp::common::LogLevel::Error, message.c_str());

    if (m_receiveCallback)
    {
        m_receiveCallback("", false);
        m_receiveCallback = nullptr;
    }
}

} // namespace csp::multiplayer
#endif
