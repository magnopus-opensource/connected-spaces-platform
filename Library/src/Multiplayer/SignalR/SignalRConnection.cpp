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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/Interfaces/IAuthContext.h"
#include "SignalRClient.h"
#include <fmt/format.h>
#include <memory>
#include <signalrclient/signalr_exception.h>

// Needs broken ... it's just for profiling we could probably just remove
#include "Debug/Logging.h"

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

SignalRConnection::SignalRConnection(const std::string& baseUri, const uint32_t keepAliveSeconds, std::shared_ptr<websocket_client> websocketClient,
    csp::common::IAuthContext& authContext)
    : m_config(
          [keepAliveSeconds]
          {
              auto clientConfig = signalr_client_config();
              clientConfig.set_keepalive_interval(std::chrono::seconds(keepAliveSeconds));
              clientConfig.set_http_headers({ { "X-DeviceUDID", csp::CSPFoundation::GetDeviceId().c_str() } });
              return clientConfig;
          }())
    , m_connection(hub_connection_builder::create(baseUri)
              .with_config(m_config)
              .with_http_client_factory([&authContext](const signalr_client_config&) { return std::make_shared<CSPHttpClient>(authContext); })
              .with_websocket_factory([websocketClient](const signalr_client_config&) { return websocketClient; })
              .skip_negotiation(true)
              .with_messagepack_hub_protocol()
#if ENABLE_SIGNALR_LOGGING
              .with_logging(std::make_shared<stdout_log_writer>(), trace_level::verbose)
#else
              .with_logging(nullptr, trace_level::error)
#endif
              .build())
    , m_pendingInvocations(0)
{
}

SignalRConnection::~SignalRConnection() { }

void SignalRConnection::Start(std::function<void(std::exception_ptr)> callback) { m_connection.start(callback); }

void SignalRConnection::Stop(std::function<void(std::exception_ptr)> callback)
{
    if (m_pendingInvocations == 0)
    {
        m_pendingStopCallback = nullptr;
        m_connection.stop(callback);
    }
    else
    {
        m_pendingStopCallback = callback;
    }
}

SignalRConnection::ConnectionState SignalRConnection::GetConnectionState() const { return (ConnectionState)m_connection.get_connection_state(); }

std::string SignalRConnection::GetConnectionId() const { return m_connection.get_connection_id(); }

void SignalRConnection::SetDisconnected(const std::function<void(std::exception_ptr)>& disconnectedCallback)
{
    m_connection.set_disconnected(disconnectedCallback);
}

bool SignalRConnection::On(const std::string& eventName, const SignalRConnection::MethodInvokedHandler& handler, csp::common::LogSystem& logSystem)
{
    CSP_PROFILE_SCOPED();

    // A bit of a hack, the signalr lib can neither unbind events, nor handle duplicate events being bound
    // This checks if an event has been bound, and dosen't attempt to bind if it has.
    // By doing this, we can move some of the api to be instantiable and not on SystemsManager without having to move the connection with it, (as
    // connection rebinds every time we login as though it's a fresh object ... which it should be!) Non-ideal, but okay.
    try
    {
        m_connection.on(eventName, handler);
        return true;
    }
    catch (const signalr::signalr_exception& exception)
    {
        // If you register an event twice, you'll throw. Very hard to debug if this happens off thread, which it does with re-logging in.
        // Not really an error though, expected behaviour for our flow, just log to help in debugability
        logSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("Caught SignalR error, ignoring 'On' registration for {}. : {}", eventName, exception.what()).c_str());
        return false;
    }
}

async::task<std::tuple<signalr::value, std::exception_ptr>> SignalRConnection::Invoke(
    const std::string& methodName, const signalr::value& arguments, std::function<void(const signalr::value&, std::exception_ptr)> callback)
{
    CSP_PROFILE_SCOPED();

    auto invokeOnCompleteEvent = std::make_shared<async::event_task<std::tuple<signalr::value, std::exception_ptr>>>();
    auto invokeOnCompleteContinuation = invokeOnCompleteEvent->get_task();

    std::function<void(const signalr::value&, std::exception_ptr)> invocationCallback
        = [callback, this, completeContinuation = std::move(invokeOnCompleteEvent)](const signalr::value& value, std::exception_ptr exceptionPtr)
    {
        if (callback)
        {
            callback(value, exceptionPtr);
        }
        completeContinuation->set(std::make_tuple(value, exceptionPtr));

        m_pendingInvocations--;
        if (m_pendingStopCallback && m_pendingInvocations == 0)
        {
            m_connection.stop(
                [this, exceptionPtr](auto /*Exception*/)
                {
                    // making a copy of the PendingStopCallback in case the SignalRConnection object gets deleted inside the callback itself
                    auto pendingStopCallback = m_pendingStopCallback;
                    m_pendingStopCallback = nullptr;

                    pendingStopCallback(exceptionPtr);
                });
        }
    };

    m_pendingInvocations++;
    m_connection.invoke(methodName, arguments, invocationCallback);
    return invokeOnCompleteContinuation;
}

void SignalRConnection::Send(const std::string& methodName, const signalr::value& arguments, std::function<void(std::exception_ptr)> callback)
{
    CSP_PROFILE_SCOPED();

    m_connection.send(methodName, arguments, callback);
}

const std::map<std::string, std::string>& SignalRConnection::HTTPHeaders() const { return m_config.get_http_headers(); }

} // namespace csp::multiplayer
