/*
 * Copyright 2025 Magnopus LLC

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

#include "../../Library/src/Multiplayer/SignalR/ISignalRConnection.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "signalrclient/signalr_value.h"
#include <gmock/gmock.h>

class SignalRConnectionMock : public csp::multiplayer::ISignalRConnection
{
public:
    MOCK_METHOD(void, Start, (std::function<void(std::exception_ptr)>), (override));
    MOCK_METHOD(void, Stop, (std::function<void(std::exception_ptr)>), (override));
    MOCK_METHOD(csp::multiplayer::ISignalRConnection::ConnectionState, GetConnectionState, (), (const, override));
    MOCK_METHOD(std::string, GetConnectionId, (), (const, override));
    MOCK_METHOD(void, SetDisconnected, (const std::function<void(std::exception_ptr)>&), (override));
    MOCK_METHOD(bool, On, (const std::string&, const MethodInvokedHandler&, csp::common::LogSystem&), (override));
    MOCK_METHOD((async::task<std::tuple<signalr::value, std::exception_ptr>>), Invoke,
        (const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)>), (override));
    MOCK_METHOD(void, Send, (const std::string&, const signalr::value&, std::function<void(std::exception_ptr)>), (override));
    MOCK_METHOD((const std::map<std::string, std::string>&), HTTPHeaders, (), (const, override));
};
