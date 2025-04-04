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

#include <exception>
#include <functional>
#include <map>

namespace signalr
{
class value;
}

namespace csp::multiplayer
{
// This interface written initially to allow Mocks to be created in tests for the SignalRConnection
class ISignalRConnection
{
public:
    enum class ConnectionState
    {
        Connecting,
        Connected,
        Disconnecting,
        Disconnected
    };

    using MethodInvokedHandler = std::function<void(const signalr::value&)>;

    virtual ~ISignalRConnection() = default;

    virtual void Start(std::function<void(std::exception_ptr)> Callback) = 0;
    virtual void Stop(std::function<void(std::exception_ptr)> Callback) = 0;
    virtual ISignalRConnection::ConnectionState GetConnectionState() const = 0;
    virtual std::string GetConnectionId() const = 0;
    virtual void SetDisconnected(const std::function<void(std::exception_ptr)>& DisconnectedCallback) = 0;
    virtual void On(const std::string& EventName, const MethodInvokedHandler& Handler) = 0;
    virtual void Invoke(
        const std::string& MethodName, const signalr::value& Arguments,
        std::function<void(const signalr::value&, std::exception_ptr)> Callback = [](const signalr::value&, std::exception_ptr) {})
        = 0;
    virtual void Send(
        const std::string& MethodName, const signalr::value& Arguments, std::function<void(std::exception_ptr)> Callback = [](std::exception_ptr) {})
        = 0;
    virtual const std::map<std::string, std::string>& HTTPHeaders() const = 0;
};
} // namespace csp::multiplayer
