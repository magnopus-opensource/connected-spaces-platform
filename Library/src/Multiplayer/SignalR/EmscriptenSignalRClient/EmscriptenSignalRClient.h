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

#include <emscripten/websocket.h>
#include <thread>

namespace csp::multiplayer
{

class CSPWebSocketClientEmscripten : public IWebSocketClient
{
public:
    CSPWebSocketClientEmscripten() noexcept;
    ~CSPWebSocketClientEmscripten() {};

    void Start(const std::string& Url, CallbackHandler Callback) override;
    void Stop(CallbackHandler Callback) override;
    void Send(const std::string& Message, CallbackHandler Callback) override;
    void Receive(ReceiveHandler Callback) override;

    std::thread& GetStartCallbackThread();
    void StartCallbackThreadFunc(bool CallbackResult);

    size_t ProcessReceivedMessage(uint8_t* RecvData, uint32_t NumBytes, EM_BOOL IsPlainText);

    ReceiveHandler* getReceiveCallback();

private:
    std::string GetWebSocketConnectURL(const std::string& InitialUrl);

    EMSCRIPTEN_WEBSOCKET_T Socket;

    std::thread StartCallbackThread;
    CallbackHandler StartCallback;
    ReceiveHandler ReceiveCallback;
    bool ReceivedHandshake;
};

} // namespace csp::multiplayer
