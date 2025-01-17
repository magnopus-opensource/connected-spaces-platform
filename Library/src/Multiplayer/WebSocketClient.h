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

#include "CSP/CSPCommon.h"

#include <functional>
#include <string>

namespace csp::multiplayer
{

class IWebSocketClient
{
public:
    virtual ~IWebSocketClient() = default;

    using CallbackHandler = std::function<void(bool)>;
    using ReceiveHandler = std::function<void(const std::string&, bool)>;

    virtual void Start(const std::string& Url, CallbackHandler Callback) = 0;
    virtual void Stop(CallbackHandler Callback) = 0;
    virtual void Send(const std::string& Message, CallbackHandler Callback) = 0;
    virtual void Receive(ReceiveHandler Callback) = 0;
};

void SetWebSocketClient(IWebSocketClient* CSPWebSocketClientPtr);

} // namespace csp::multiplayer
