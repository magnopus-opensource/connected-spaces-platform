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

#include "Multiplayer/WebSocketClient.h"

#include <Poco/Net/WebSocket.h>
#include <atomic>
#include <mutex>
#include <signalrclient/hub_exception.h>
#include <signalrclient/signalr_client_config.h>
#include <thread>

namespace csp::multiplayer
{

class CSPWebSocketClientPOCO : public IWebSocketClient
{
public:
    CSPWebSocketClientPOCO() noexcept;
    ~CSPWebSocketClientPOCO();

    void Start(const std::string& Url, CallbackHandler Callback) override;
    void Stop(CallbackHandler Callback) override;
    void Send(const std::string& Message, CallbackHandler Callback) override;
    void Receive(ReceiveHandler Callback) override;

private:
    void ReceiveThreadFunc();
    void HandleReceiveError(const std::string& Message);

    Poco::Net::WebSocket* PocoWebSocket;

    std::thread ReceiveThread;
    std::mutex Mutex;
    std::atomic_bool ReceiveReady;
    ReceiveHandler ReceiveCallback;
    std::atomic_bool StopFlag;
};

} // namespace csp::multiplayer
