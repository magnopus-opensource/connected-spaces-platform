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

#include "Web/Uri.h"

#include <atomic>
#include <functional>
#include <signalrclient/hub_connection_builder.h>

CSP_START_IGNORE
class CSPEngine_MultiplayerTests_SignalRConnectionTest_Test;
CSP_END_IGNORE

namespace csp::multiplayer
{

class SignalRConnection
{
public:
    /** @cond DO_NOT_DOCUMENT */
    friend class ::CSPEngine_MultiplayerTests_SignalRConnectionTest_Test;
    /** @endcond */

    typedef std::function<void __cdecl(const signalr::value&)> MethodInvokedHandler;

    SignalRConnection(const std::string& url, const uint32_t KeepAliveSeconds, std::shared_ptr<signalr::websocket_client> WebSocketClient);
    virtual ~SignalRConnection();

    void Start(std::function<void(std::exception_ptr)> Callback);
    void Stop(std::function<void(std::exception_ptr)> Callback);

    enum class ConnectionState
    {
        Connecting,
        Connected,
        Disconnecting,
        Disconnected
    };

    ConnectionState GetConnectionState() const;

    std::string GetConnectionId() const;

    void SetDisconnected(const std::function<void(std::exception_ptr)>& DisconnectedCallback);

    void On(const std::string& EventName, const MethodInvokedHandler& Handler);

    void Invoke(
        const std::string& MethodName, const signalr::value& Arguments = signalr::value(),
        std::function<void(const signalr::value&, std::exception_ptr)> Callback = [](const signalr::value&, std::exception_ptr) {});

    void Send(
        const std::string& MethodName, const signalr::value& Arguments = signalr::value(),
        std::function<void(std::exception_ptr)> Callback = [](std::exception_ptr) {});

private:
    signalr::hub_connection Connection;

    // We tracking pending invocations so that we can guarantee that the connection is not stopped until all
    // invocations have completed. This is important for situations such as when the player exits a space, invokes a deletion of their
    // avatar entity, and we need to ensure that message has gone through before destroying the connection.
    std::atomic_uint PendingInvocations;
    std::function<void(std::exception_ptr)> PendingStopCallback;
    signalr::signalr_client_config config;
};

} // namespace csp::multiplayer
