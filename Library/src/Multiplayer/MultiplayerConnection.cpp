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
#include "CSP/Multiplayer/MultiPlayerConnection.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/CSPAsyncScheduler.h"
#include "CSP/Common/Interfaces/IAuthContext.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CallHelpers.h"
#include "Events/EventSystem.h"
#include "Multiplayer/MultiplayerConstants.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Multiplayer/SignalR/ISignalRConnection.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"

#ifdef CSP_WASM
#include "Multiplayer/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.h"
#else
#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
#endif

#include "Common/Web/Uri.h"
#include "Debug/Logging.h"

#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <thread>

using namespace std::chrono_literals;

namespace csp::multiplayer
{

static std::map<std::string, ErrorCode> ErrorCodeMap = { { "Scopes_ConcurrentUsersQuota", ErrorCode::SpaceUserLimitExceeded } };

// First - The parsed error code (always ConcurrentUsersQuota or unknown ...), Second - the full error message.
std::pair<ErrorCode, std::string> MultiplayerConnection::ParseMultiplayerErrorFromExceptionPtr(std::exception_ptr exception)
{
    try
    {
        if (exception)
        {
            rethrow_exception(exception);
        }
        else
        {
            return { ErrorCode::Unknown, "MultiplayerConnection::ParseMultiplayerErrorFromExceptionPtr, Unexpectedly no exception was thrown." };
        }
    }
    catch (const std::exception& e)
    {
        return ParseMultiplayerError(e);
    }
}

std::pair<ErrorCode, std::string> MultiplayerConnection::ParseMultiplayerError(const std::exception& exception)
{
    /* E.M: This is a strange function
     * Whilst it does make sense to parse the errors that come out of the SignalR interaction,
     * this function is concerned with only the Scopes_ConcurrentUsersQuota error,
     * (an error that may not even be fired anymore, since it's an unspecified CHS behaviour).
     * We should either make this parse meaningful, or remove it entirely imo.
     */

    const std::string errorMessage = exception.what();

    constexpr const char* errorCodeKey = "error code:";
    constexpr size_t errorCodeKeyLength = std::char_traits<char>::length(errorCodeKey);

    auto index = errorMessage.find(errorCodeKey);

    if (index == std::string::npos)
    {
        return { ErrorCode::Unknown, errorMessage };
    }

    index += errorCodeKeyLength;

    // Trim whitespace on left
    while (isspace(errorMessage[index]))
    {
        ++index;
    }

    auto startIndex = index;

    index = errorMessage.find(',', index);

    if (index == std::string::npos)
    {
        index = errorMessage.length();
    }

    auto errorCodeString = errorMessage.substr(startIndex, index - startIndex);

    if (ErrorCodeMap.count(errorCodeString))
    {
        return { ErrorCodeMap[errorCodeString], errorMessage };
    }

    return { ErrorCode::Unknown, errorMessage };
}

namespace
{

    constexpr const uint64_t ALL_ENTITIES_ID = std::numeric_limits<uint64_t>::max();
    constexpr const uint32_t KEEP_ALIVE_INTERVAL = 15;
}

ISignalRConnection* MultiplayerConnection::MakeSignalRConnection(csp::common::IAuthContext& authContext)
{
    return new csp::multiplayer::SignalRConnection(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI().c_str(), KEEP_ALIVE_INTERVAL,
        std::make_shared<csp::multiplayer::CSPWebsocketClient>(), authContext);
}

MultiplayerConnection::MultiplayerConnection(csp::common::LogSystem& logSystem, csp::multiplayer::ISignalRConnection& connection)
    : m_connection(&connection)
    , m_webSocketClient(nullptr)
    , m_networkEventManager(new NetworkEventManagerImpl(this))
    , m_logSystem(logSystem)
    , m_clientId(0)
    , m_connected(false)
    , m_multiplayerHubMethods(MultiplayerHubMethodMap())
{
    m_eventBus = new NetworkEventBus(this, logSystem);
}

MultiplayerConnection::~MultiplayerConnection()
{
    if (m_connection != nullptr)
    {
        if (m_connected)
        {
            std::promise<ErrorCode> shutdownPromise;

            DisconnectWithReason(
                "MultiplayerConnection shutting down.", [&shutdownPromise](ErrorCode errorCode) { shutdownPromise.set_value(errorCode); });

            auto shutdownFuture = shutdownPromise.get_future();
            shutdownFuture.wait();
        }

        delete (m_connection);
        delete (m_webSocketClient);
        delete (m_networkEventManager);
        delete (m_eventBus);
    }
}

void MultiplayerConnection::SetOnlineRealtimeEngine(csp::multiplayer::OnlineRealtimeEngine* onlineRealtimeEngine)
{
    m_multiplayerRealtimeEngine = onlineRealtimeEngine;
}

csp::multiplayer::OnlineRealtimeEngine* MultiplayerConnection::GetOnlineRealtimeEngine() const { return m_multiplayerRealtimeEngine; }

void MultiplayerConnection::__CauseFailure() { m_webSocketClient->__CauseFailure(); }

MultiplayerConnection::MultiplayerConnection(const MultiplayerConnection& inBoundConnection)
    : m_logSystem(inBoundConnection.m_logSystem)
{
    m_connection = inBoundConnection.m_connection;
    m_webSocketClient = inBoundConnection.m_webSocketClient;
    m_networkEventManager = inBoundConnection.m_networkEventManager;
    m_clientId = inBoundConnection.m_clientId;
    m_disconnectionCallback = inBoundConnection.m_disconnectionCallback;
    m_connectionCallback = inBoundConnection.m_connectionCallback;
    m_networkInterruptionCallback = inBoundConnection.m_networkInterruptionCallback;
    m_eventBus = inBoundConnection.m_eventBus;
    m_connected = (inBoundConnection.m_connected) ? true : false;
}

namespace
{
    void RegisterNetworkInterruptedCallback(csp::multiplayer::ISignalRConnection* connection, csp::common::LogSystem& logSystem,
        const MultiplayerConnection::NetworkInterruptionCallbackHandler& networkInterruptionCallback)
    {
        connection->SetDisconnected(
            [&networkInterruptionCallback, &logSystem](const std::exception_ptr& except)
            {
                // We currently detect a connection interrupt if the disconnected callback contains an exception.
                if (except)
                {
                    try
                    {
                        std::rethrow_exception(except);
                    }
                    catch (const std::exception& e)
                    {
                        INVOKE_IF_NOT_NULL(networkInterruptionCallback, e.what());
                        logSystem.LogMsg(csp::common::LogLevel::Log, "Connection Interrupted.");
                    }
                }
            });
    }
}

auto MultiplayerConnection::DeleteEntities(uint64_t entityId) const
{
    return [this, entityId]()
    {
        // Shared pointer to keep alive in local callback
        auto entitiesDeletedEvent = std::make_shared<async::event_task<void>>();
        auto entitiesDeletedContinuation = entitiesDeletedEvent->get_task();
        if (!m_connected)
        {
            throw csp::common::continuations::ErrorCodeException(
                ErrorCode::NotConnected, "MultiplayerConnection::DeleteEntities, Error not connected.");
        }
        std::function<void(signalr::value, std::exception_ptr)> localCallback
            = [entitiesDeletedEvent](signalr::value /*Result*/, std::exception_ptr except)
        {
            if (except != nullptr)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(except);
                entitiesDeletedEvent->set_exception(std::make_exception_ptr(csp::common::continuations::ErrorCodeException(
                    Error, "MultiplayerConnection::DeleteEntities, Unexpected error response from SignalR \"DeleteObjects\" invocation.")));
                return;
            }

            entitiesDeletedEvent->set();
        };

        std::vector<signalr::value> paramsVec;

        if (entityId == ALL_ENTITIES_ID)
        {
            paramsVec.push_back(signalr::value_type::null);
        }
        else
        {
            std::vector<signalr::value> entityIDs;
            entityIDs.push_back(signalr::value(entityId));
            paramsVec.push_back(entityIDs);
        }

        signalr::value deleteEntityMessage = signalr::value(std::move(paramsVec));

        m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling DeleteObjects");

        m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::DELETE_OBJECTS), deleteEntityMessage, localCallback);

        return entitiesDeletedContinuation;
    };
}

/*
   Requests the connected Client ID from CHS.
 */
auto MultiplayerConnection::RequestClientId()
{
    return [this]()
    {
        // Shared pointer to keep alive in local callback
        auto clientIdRequestedEvent = std::make_shared<async::event_task<uint64_t>>();
        auto clientIdRequestedContinuation = clientIdRequestedEvent->get_task();

        if (!m_connected)
        {
            throw csp::common::continuations::ErrorCodeException(
                ErrorCode::NotConnected, "MultiplayerConnection::RequestClientId, Error not connected.");
        }

        std::function<void(signalr::value, std::exception_ptr)> localCallback
            = [clientIdRequestedEvent, this](signalr::value result, std::exception_ptr except)
        {
            if (except != nullptr)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(except);
                clientIdRequestedEvent->set_exception(std::make_exception_ptr(csp::common::continuations::ErrorCodeException(
                    Error, "MultiplayerConnection::RequestClientId, Error when starting requesting Client Id.")));
                return;
            }

            m_logSystem.LogMsg(csp::common::LogLevel::Verbose, fmt::format("ClientId={}", result.as_uinteger()).c_str());

            clientIdRequestedEvent->set(result.as_uinteger());
        };

        m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling GetClientId");

        m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::GET_CLIENT_ID), signalr::value(signalr::value_type::array), localCallback);
        return clientIdRequestedContinuation;
    };
}

async::task<std::tuple<signalr::value, std::exception_ptr>> MultiplayerConnection::StartListening()
{
    if (!m_connected)
    {
        throw csp::common::continuations::ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::StartListening, Error not connected.");
    }

    m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling StartListening");
    return m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::START_LISTENING), signalr::value(signalr::value_type::array));
}

void MultiplayerConnection::Connect(ErrorCodeCallbackHandler callback, [[maybe_unused]] const csp::common::String& multiplayerUri,
    const csp::common::String& accessToken, const csp::common::String& deviceId)
{
    if (m_connected)
    {
        INVOKE_IF_NOT_NULL(callback, ErrorCode::AlreadyConnected);

        return;
    }

// You will notice that the Emscripten webclient doesn't take the Uri into its constructor.
// This is because it uses the url passed into its Start function, which gets modified by SignalR.
// This modified version isn't compatible with our POCO implementation, so we need to directly use the MultiplayerUri.
#ifdef CSP_WASM
    WebSocketClient = new csp::multiplayer::CSPWebSocketClientEmscripten(AccessToken.c_str(), DeviceId.c_str());
#else
    m_webSocketClient = new csp::multiplayer::CSPWebSocketClientPOCO(multiplayerUri.c_str(), accessToken.c_str(), deviceId.c_str(), m_logSystem);
#endif
    csp::multiplayer::SetWebSocketClient(m_webSocketClient);

    m_networkEventManager->SetConnection(*m_connection);

    BindOnObjectMessage();
    BindOnObjectPatch();
    BindOnRequestToSendObject();
    BindOnRequestToDisconnect();
    BindOnElectedScopeLeaderCallback();
    BindOnVacatedScopeLeaderCallback();

    m_eventBus->StartEventMessageListening();

    // We register the network interruption callback as a wrapper because we want to unwrap any signalR exceptions.
    RegisterNetworkInterruptedCallback(m_connection, m_logSystem, m_networkInterruptionCallback);

    /*
     * Start() - Start the SignalR socket connection
     * Lambda - Set connected to be true if successful start()
     * RequestClientId() - Request the client Id
     * Lambda - and store it
     * StartListening() - Invoke StartListening on the SignalR socket connection
     * Lambda - Invoke success callbacks if start listening succeeded
     * InvokeIfExceptionIsInChain - Handle any errors in the Connect chain
     */

    Start()
        .then(async::inline_scheduler(), [this]() { m_connected = true; })
        .then(async::inline_scheduler(), DeleteEntities(ALL_ENTITIES_ID))
        .then(async::inline_scheduler(), RequestClientId())
        .then(async::inline_scheduler(), [this](uint64_t retrievedClientId) { m_clientId = retrievedClientId; })
        .then(async::inline_scheduler(), [this]() { return StartListening(); })
        .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>())
        .then(async::inline_scheduler(),
            [this, callback]()
            {
                // Success
                INVOKE_IF_NOT_NULL(m_connectionCallback, "Successfully connected to SignalR hub.");
                INVOKE_IF_NOT_NULL(callback, ErrorCode::None);
            })
        .then(async::inline_scheduler(),
            csp::common::continuations::InvokeIfExceptionInChain(m_logSystem,
                // Handle any errors in chain
                [callback, this]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
                {
                    auto [Error, ExceptionErrorMsg] = ParseMultiplayerError(exception);
                    DisconnectWithReason(ExceptionErrorMsg.c_str(), callback);
                }));
}

void MultiplayerConnection::Disconnect(ErrorCodeCallbackHandler callback)
{
    if (!m_connected)
    {
        INVOKE_IF_NOT_NULL(callback, ErrorCode::NotConnected);

        return;
    }

    DisconnectWithReason("Client called disconnect.", callback);
}

void MultiplayerConnection::DisconnectWithReason(const csp::common::String& reason, ErrorCodeCallbackHandler callback)
{
    const ExceptionCallbackHandler stopHandler = [this, callback, reason](const std::exception_ptr& except)
    {
        ErrorCode error = ErrorCode::None;
        if (except != nullptr)
        {
            auto [ExceptionError, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(except);
            error = ExceptionError;
        }

        m_connected = false;
        INVOKE_IF_NOT_NULL(callback, error);
        INVOKE_IF_NOT_NULL(m_disconnectionCallback, reason);
    };

    Stop(stopHandler);
}

void MultiplayerConnection::Start(const ExceptionCallbackHandler callback) const { m_connection->Start(callback); }

async::task<void> MultiplayerConnection::Start() const
{
    // Shared pointer to keep alive in local callback
    auto onCompleteEvent = std::make_shared<async::event_task<void>>();
    async::task<void> onCompleteTask = onCompleteEvent->get_task();

    m_connection->Start(
        [onCompleteEvent](std::exception_ptr exception)
        {
            if (exception)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(exception);
                onCompleteEvent->set_exception(std::make_exception_ptr(
                    csp::common::continuations::ErrorCodeException(Error, "MultiplayerConnection::Start, Error when starting SignalR connection.")));
                return;
            }

            onCompleteEvent->set();
        });

    return onCompleteTask;
}

void MultiplayerConnection::Stop(const ExceptionCallbackHandler callback) const
{
    if (!m_connected)
    {
        INVOKE_IF_NOT_NULL(
            callback, std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::NotConnected, "No Connection!")));
        return;
    }

    m_connection->Stop(callback);
}

void MultiplayerConnection::SetDisconnectionCallback(DisconnectionCallbackHandler callback) { m_disconnectionCallback = callback; }

void MultiplayerConnection::SetConnectionCallback(ConnectionCallbackHandler callback) { m_connectionCallback = callback; }

CSP_EVENT void MultiplayerConnection::SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler callback)
{
    m_networkInterruptionCallback = callback;
}

async::task<std::tuple<signalr::value, std::exception_ptr>> MultiplayerConnection::SetScopes(csp::common::String inSpaceId)
{
    if (!m_connected)
    {
        throw csp::common::continuations::ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::SetScopes, Error not connected.");
    }

    m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling SetScopes");

    std::vector<signalr::value> scopesVec;

    // Set the scope using the Space Id
    scopesVec.push_back(signalr::value(inSpaceId.c_str()));

    std::vector<signalr::value> paramsVec;
    paramsVec.push_back(scopesVec);
    signalr::value params = signalr::value(std::move(paramsVec));

    return m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::SET_SCOPES), params);
}

async::task<std::tuple<signalr::value, std::exception_ptr>> MultiplayerConnection::ResetScopes()
{
    if (!m_connected)
    {
        throw csp::common::continuations::ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::ResetScopes, Error not connected.");
    }

    std::vector<signalr::value> paramsVec;
    signalr::value params = signalr::value(std::move(paramsVec));
    return m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::RESET_SCOPES), params);
}

async::task<std::tuple<signalr::value, std::exception_ptr>> MultiplayerConnection::StopListening()
{
    if (!m_connected)
    {
        throw csp::common::continuations::ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::StopListening, Error not connected.");
    }

    m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling StopListening");

    return m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::STOP_LISTENING), signalr::value(signalr::value_type::array));
}

uint64_t MultiplayerConnection::GetClientId() const { return m_clientId; }

ConnectionState MultiplayerConnection::GetConnectionState() const { return static_cast<ConnectionState>(m_connection->GetConnectionState()); }

CSP_ASYNC_RESULT void MultiplayerConnection::SetAllowSelfMessagingFlag(const bool inAllowSelfMessaging, ErrorCodeCallbackHandler callback)
{
    if (!m_connected)
    {
        INVOKE_IF_NOT_NULL(callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> localCallback
        = [this, callback, inAllowSelfMessaging](signalr::value /*Result*/, std::exception_ptr except)
    {
        if (except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(except);
            INVOKE_IF_NOT_NULL(callback, Error);

            return;
        }

        m_allowSelfMessaging = inAllowSelfMessaging;

        INVOKE_IF_NOT_NULL(callback, ErrorCode::None);
    };

    m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling SetAllowSelfMessaging");

    const std::vector invokeArguments = { signalr::value(inAllowSelfMessaging) };
    m_connection->Invoke(m_multiplayerHubMethods.Get(MultiplayerHubMethod::SET_ALLOW_SELF_MESSAGING), invokeArguments, localCallback);
}

bool MultiplayerConnection::GetAllowSelfMessagingFlag() const { return m_allowSelfMessaging; }

void MultiplayerConnection::BindOnObjectMessage()
{
    const std::string onObjectMessage = GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ON_OBJECT_MESSAGE);
    GetSignalRConnection()->On(
        onObjectMessage,
        [this](const signalr::value& params)
        {
            if (m_multiplayerRealtimeEngine != nullptr)
            {
                m_multiplayerRealtimeEngine->OnObjectMessage(params);
            }
            else
            {
                m_logSystem.LogMsg(
                    common::LogLevel::Verbose, "Received OnObjectMessage without an alive EntitySystem. This is expected if leaving a space.");
            }
        },
        m_logSystem);
}

void MultiplayerConnection::BindOnObjectPatch()
{
    const std::string onObjectPatch = GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ON_OBJECT_PATCH);
    GetSignalRConnection()->On(
        onObjectPatch,
        [this](const signalr::value& params)
        {
            if (m_multiplayerRealtimeEngine != nullptr)
            {
                m_multiplayerRealtimeEngine->OnObjectPatch(params);
            }
            else
            {
                m_logSystem.LogMsg(
                    common::LogLevel::Verbose, "Received OnObjectPatch without an alive EntitySystem. This is expected if leaving a space.");
            }
        },
        m_logSystem);
}

void MultiplayerConnection::BindOnRequestToSendObject()
{
    const std::string onRequestToSendObject = GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ON_REQUEST_TO_SEND_OBJECT);
    GetSignalRConnection()->On(
        onRequestToSendObject,
        [this](const signalr::value& params)
        {
            if (m_multiplayerRealtimeEngine != nullptr)
            {
                m_multiplayerRealtimeEngine->OnRequestToSendObject(params);
            }
            else
            {
                m_logSystem.LogMsg(
                    common::LogLevel::Verbose, "Received OnRequestToSendObject without an alive EntitySystem. This is expected if leaving a space.");
            }
        },
        m_logSystem);
}

void MultiplayerConnection::BindOnRequestToDisconnect()
{
    const std::string onRequestToDisconnect = GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ON_REQUEST_TO_DISCONNECT);
    GetSignalRConnection()->On(
        onRequestToDisconnect,
        [&](const signalr::value& params)
        {
            std::promise<bool> promise;
            std::future<bool> future = promise.get_future();

            const std::string reason = params.as_array()[0].as_string();
            DisconnectWithReason(reason.c_str(), [&promise](ErrorCode /*Error*/) { promise.set_value(true); });

            future.wait_for(2000ms);
        },
        m_logSystem);
}

void MultiplayerConnection::BindOnElectedScopeLeaderCallback()
{
    GetSignalRConnection()->On(
        GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ON_ELECTED_SCOPE_LEADER),
        [this](signalr::value params)
        {
            if (m_multiplayerRealtimeEngine != nullptr)
            {
                m_multiplayerRealtimeEngine->OnElectedScopeLeader(params);
            }
            else
            {
                m_logSystem.LogMsg(
                    common::LogLevel::Verbose, "Received OnElectedScopeLeader without an alive EntitySystem. This is expected if leaving a space.");
            }
        },
        m_logSystem);
}

void MultiplayerConnection::BindOnVacatedScopeLeaderCallback()
{
    GetSignalRConnection()->On(
        GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ON_VACATED_AS_SCOPE_LEADER),
        [this](signalr::value params)
        {
            if (m_multiplayerRealtimeEngine != nullptr)
            {
                m_multiplayerRealtimeEngine->OnVacatedAsScopeLeader(params);
            }
            else
            {
                m_logSystem.LogMsg(
                    common::LogLevel::Verbose, "Received OnVacatedScopeLeader without an alive EntitySystem. This is expected if leaving a space.");
            }
        },
        m_logSystem);
}

} // namespace csp::multiplayer
