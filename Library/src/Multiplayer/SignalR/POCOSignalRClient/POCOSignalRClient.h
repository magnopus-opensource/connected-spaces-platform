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
#pragma once

#ifndef CSP_WASM

#include "Multiplayer/WebSocketClient.h"

#include <Poco/Net/WebSocket.h>
#include <atomic>
#include <mutex>
#include <signalrclient/hub_exception.h>
#include <signalrclient/signalr_client_config.h>
#include <thread>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{

class CSPWebSocketClientPOCO : public IWebSocketClient
{
public:
    CSPWebSocketClientPOCO(
        const std::string& multiplayerUri, const std::string& accessToken, const std::string& deviceId, csp::common::LogSystem& logSystem) noexcept;
    ~CSPWebSocketClientPOCO();

    void Start(const std::string& url, CallbackHandler callback) override;
    void Stop(CallbackHandler callback) override;
    void Send(const std::string& message, CallbackHandler callback) override;
    void Receive(ReceiveHandler callback) override;

    void __CauseFailure() override;

    struct ParsedURIInfo
    {
        std::string Endpoint;
        std::string Protocol;
        std::string Domain;
        std::string Path;
        unsigned short Port;
    };

    static ParsedURIInfo ParseMultiplayerServiceUriEndPoint(const std::string& multiplayerServiceUriEndpoint);

private:
    void ReceiveThreadFunc();
    void HandleReceiveError(const std::string& message);

    Poco::Net::WebSocket* m_pocoWebSocket;

    std::thread m_receiveThread;
    std::mutex m_mutex;
    std::atomic_bool m_receiveReady;
    ReceiveHandler m_receiveCallback;
    std::atomic_bool m_stopFlag;

    std::string m_multiplayerUri;
    std::string m_accessToken;
    std::string m_deviceId;
    csp::common::LogSystem& m_logSystem;
};

} // namespace csp::multiplayer
#endif
