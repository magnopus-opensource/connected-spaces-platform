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
#include "SignalRConnection.h"

#include "Debug/Logging.h"
#include "SignalRClient.h"

#if ENABLE_SIGNALR_LOGGING
#include <iostream>

#if SIGNALR_LOG_TO_DEBUGGER
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <debugapi.h>
#include <windows.h>
#endif
#endif

using namespace signalr;

namespace csp::multiplayer
{

#if ENABLE_SIGNALR_LOGGING
class stdout_log_writer : public log_writer
{
public:
    void write(const std::string& entry) override
    {
        std::cerr << entry << std::endl;

#if SIGNALR_LOG_TO_DEBUGGER
        if (IsDebuggerPresent())
        {
            OutputDebugString(entry.c_str());
            OutputDebugString("\n");
        }
#endif
    }
};
#endif

SignalRConnection::SignalRConnection(const std::string& BaseUri, const uint32_t KeepAliveSeconds, std::shared_ptr<websocket_client> WebsocketClient)
    : Connection(hub_connection_builder::create(BaseUri)
                     .with_http_client_factory([](const signalr_client_config&) { return std::make_shared<CSPHttpClient>(); })
                     .with_websocket_factory([WebsocketClient](const signalr_client_config&) { return WebsocketClient; })
                     .skip_negotiation(true)
                     .with_messagepack_hub_protocol()
#if ENABLE_SIGNALR_LOGGING
                     .with_logging(std::make_shared<stdout_log_writer>(), trace_level::verbose)
#else
                     .with_logging(nullptr, trace_level::error)
#endif
                     .build())
    , PendingInvocations(0)
{
    config = signalr_client_config();
    config.set_keepalive_interval(std::chrono::seconds(KeepAliveSeconds));
    config.set_http_headers({ { "X-DeviceUDID", csp::CSPFoundation::GetDeviceId().c_str() } });
    Connection.set_client_config(config);
}

SignalRConnection::~SignalRConnection() { }

void SignalRConnection::Start(std::function<void(std::exception_ptr)> Callback) { Connection.start(Callback); }

void SignalRConnection::Stop(std::function<void(std::exception_ptr)> Callback)
{
    if (PendingInvocations == 0)
    {
        PendingStopCallback = nullptr;
        Connection.stop(Callback);
    }
    else
    {
        PendingStopCallback = Callback;
    }
}

SignalRConnection::ConnectionState SignalRConnection::GetConnectionState() const { return (ConnectionState)Connection.get_connection_state(); }

std::string SignalRConnection::GetConnectionId() const { return Connection.get_connection_id(); }

void SignalRConnection::SetDisconnected(const std::function<void(std::exception_ptr)>& DisconnectedCallback)
{
    Connection.set_disconnected(DisconnectedCallback);
}

void SignalRConnection::On(const std::string& EventName, const SignalRConnection::MethodInvokedHandler& Handler)
{
    CSP_PROFILE_SCOPED();

    Connection.on(EventName, Handler);
}

void SignalRConnection::Invoke(
    const std::string& MethodName, const signalr::value& Arguments, std::function<void(const signalr::value&, std::exception_ptr)> Callback)
{
    CSP_PROFILE_SCOPED();

    std::function<void(const signalr::value&, std::exception_ptr)> InvocationCallback
        = [Callback, this](const signalr::value& Value, std::exception_ptr ExceptionPtr)
    {
        Callback(Value, ExceptionPtr);

        PendingInvocations--;
        if (PendingStopCallback && PendingInvocations == 0)
        {
            Connection.stop(
                [this, ExceptionPtr](auto Exception)
                {
                    // making a copy of the PendingStopCallback in case the SignalRConnection object gets deleted inside the callback itself
                    auto _PendingStopCallback = PendingStopCallback;
                    PendingStopCallback = nullptr;

                    _PendingStopCallback(ExceptionPtr);
                });
        }
    };

    PendingInvocations++;
    Connection.invoke(MethodName, Arguments, InvocationCallback);
}

void SignalRConnection::Send(const std::string& MethodName, const signalr::value& Arguments, std::function<void(std::exception_ptr)> Callback)
{
    CSP_PROFILE_SCOPED();

    Connection.send(MethodName, Arguments, Callback);
}

} // namespace csp::multiplayer
