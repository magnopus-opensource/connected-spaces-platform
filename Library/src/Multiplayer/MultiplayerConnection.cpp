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
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
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
#include <exception>
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
std::pair<ErrorCode, std::string> MultiplayerConnection::ParseMultiplayerErrorFromExceptionPtr(std::exception_ptr Exception)
{
    try
    {
        if (Exception)
        {
            rethrow_exception(Exception);
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

std::pair<ErrorCode, std::string> MultiplayerConnection::ParseMultiplayerError(const std::exception& Exception)
{
    /* E.M: This is a strange function
     * Whilst it does make sense to parse the errors that come out of the SignalR interaction,
     * this function is concerned with only the Scopes_ConcurrentUsersQuota error,
     * (an error that may not even be fired anymore, since it's an unspecified CHS behaviour).
     * We should either make this parse meaningful, or remove it entirely imo.
     */

    const std::string ErrorMessage = Exception.what();

    constexpr const char* ERROR_CODE_KEY = "error code:";
    constexpr size_t ERROR_CODE_KEY_LENGTH = std::char_traits<char>::length(ERROR_CODE_KEY);

    auto Index = ErrorMessage.find(ERROR_CODE_KEY);

    if (Index == std::string::npos)
    {
        return { ErrorCode::Unknown, ErrorMessage };
    }

    Index += ERROR_CODE_KEY_LENGTH;

    // Trim whitespace on left
    while (isspace(ErrorMessage[Index]))
    {
        ++Index;
    }

    auto StartIndex = Index;

    Index = ErrorMessage.find(',', Index);

    if (Index == std::string::npos)
    {
        Index = ErrorMessage.length();
    }

    auto ErrorCodeString = ErrorMessage.substr(StartIndex, Index - StartIndex);

    if (ErrorCodeMap.count(ErrorCodeString))
    {
        return { ErrorCodeMap[ErrorCodeString], ErrorMessage };
    }

    return { ErrorCode::Unknown, ErrorMessage };
}

namespace
{

    constexpr const uint64_t ALL_ENTITIES_ID = std::numeric_limits<uint64_t>::max();
    constexpr const uint32_t KEEP_ALIVE_INTERVAL = 15;

    // Exception type used for internal chaining in response to signalR exceptions
    class ErrorCodeException : public std::runtime_error
    {
    public:
        explicit ErrorCodeException(ErrorCode Code, const std::string& Message)
            : std::runtime_error(Message)
            , m_Code(Code)
        {
        }

        ErrorCode Code() const noexcept { return m_Code; }

    private:
        ErrorCode m_Code;
    };
}

ISignalRConnection* MultiplayerConnection::MakeSignalRConnection(csp::common::IAuthContext& AuthContext)
{
    return new csp::multiplayer::SignalRConnection(csp::CSPFoundation::GetEndpoints().MultiplayerService.GetURI().c_str(), KEEP_ALIVE_INTERVAL,
        std::make_shared<csp::multiplayer::CSPWebsocketClient>(), AuthContext);
}

MultiplayerConnection::MultiplayerConnection(csp::common::LogSystem& LogSystem)
    : Connection(nullptr)
    , WebSocketClient(nullptr)
    , NetworkEventManager(new NetworkEventManagerImpl(this))
    , LogSystem(LogSystem)
    , ClientId(0)
    , Connected(false)
    , MultiplayerHubMethods(MultiplayerHubMethodMap())
{
    EventBusPtr = new NetworkEventBus(this, LogSystem);
}

MultiplayerConnection::~MultiplayerConnection()
{
    if (Connection != nullptr)
    {
        if (Connected)
        {
            std::promise<ErrorCode> shutdownPromise;

            DisconnectWithReason(
                "MultiplayerConnection shutting down.", [&shutdownPromise](ErrorCode errorCode) { shutdownPromise.set_value(errorCode); });

            auto shutdownFuture = shutdownPromise.get_future();
            shutdownFuture.wait();
        }

        delete (Connection);
        delete (WebSocketClient);
        delete (NetworkEventManager);
        delete (EventBusPtr);
    }
}

MultiplayerConnection::MultiplayerConnection(const MultiplayerConnection& InBoundConnection)
    : LogSystem(InBoundConnection.LogSystem)
{
    Connection = InBoundConnection.Connection;
    WebSocketClient = InBoundConnection.WebSocketClient;
    NetworkEventManager = InBoundConnection.NetworkEventManager;
    ClientId = InBoundConnection.ClientId;
    DisconnectionCallback = InBoundConnection.DisconnectionCallback;
    ConnectionCallback = InBoundConnection.ConnectionCallback;
    NetworkInterruptionCallback = InBoundConnection.NetworkInterruptionCallback;
    EventBusPtr = InBoundConnection.EventBusPtr;
    Connected = (InBoundConnection.Connected) ? true : false;
}

namespace
{
    void RegisterNetworkInterruptedCallback(csp::multiplayer::ISignalRConnection* Connection, csp::common::LogSystem& LogSystem,
        const MultiplayerConnection::NetworkInterruptionCallbackHandler& NetworkInterruptionCallback)
    {
        Connection->SetDisconnected(
            [&NetworkInterruptionCallback, &LogSystem](const std::exception_ptr& Except)
            {
                if (Except)
                {
                    try
                    {
                        std::rethrow_exception(Except);
                    }
                    catch (const std::exception& e)
                    {
                        INVOKE_IF_NOT_NULL(NetworkInterruptionCallback, e.what());
                    }
                }

                LogSystem.LogMsg(csp::common::LogLevel::Log, "Connection Interrupted.");
            });
    }
}

auto MultiplayerConnection::DeleteEntities(uint64_t EntityId) const
{
    return [this, EntityId]()
    {
        // Shared pointer to keep alive in local callback
        auto EntitiesDeletedEvent = std::make_shared<async::event_task<void>>();
        auto EntitiesDeletedContinuation = EntitiesDeletedEvent->get_task();
        if (Connection == nullptr || !Connected)
        {
            throw ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::DeleteEntities, Error not connected.");
        }
        std::function<void(signalr::value, std::exception_ptr)> LocalCallback
            = [EntitiesDeletedEvent](signalr::value /*Result*/, std::exception_ptr Except)
        {
            if (Except != nullptr)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
                EntitiesDeletedEvent->set_exception(std::make_exception_ptr(ErrorCodeException(
                    Error, "MultiplayerConnection::DeleteEntities, Unexpected error response from SignalR \"DeleteObjects\" invocation.")));
                return;
            }

            EntitiesDeletedEvent->set();
        };

        std::vector<signalr::value> ParamsVec;

        if (EntityId == ALL_ENTITIES_ID)
        {
            ParamsVec.push_back(signalr::value_type::null);
        }
        else
        {
            std::vector<signalr::value> EntityIDs;
            EntityIDs.push_back(signalr::value(EntityId));
            ParamsVec.push_back(EntityIDs);
        }

        signalr::value DeleteEntityMessage = signalr::value(std::move(ParamsVec));

        LogSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling DeleteObjects");

        Connection->Invoke(MultiplayerHubMethods.Get(MultiplayerHubMethod::DELETE_OBJECTS), DeleteEntityMessage, LocalCallback);

        return EntitiesDeletedContinuation;
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
        auto ClientIdRequestedEvent = std::make_shared<async::event_task<uint64_t>>();
        auto ClientIdRequestedContinuation = ClientIdRequestedEvent->get_task();

        if (Connection == nullptr || !Connected)
        {
            throw ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::RequestClientId, Error not connected.");
        }

        std::function<void(signalr::value, std::exception_ptr)> LocalCallback
            = [ClientIdRequestedEvent, this](signalr::value Result, std::exception_ptr Except)
        {
            if (Except != nullptr)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
                ClientIdRequestedEvent->set_exception(std::make_exception_ptr(
                    ErrorCodeException(Error, "MultiplayerConnection::RequestClientId, Error when starting requesting Client Id.")));
                return;
            }

            LogSystem.LogMsg(csp::common::LogLevel::Verbose, fmt::format("ClientId={}", Result.as_uinteger()).c_str());

            ClientIdRequestedEvent->set(Result.as_uinteger());
        };

        LogSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling GetClientId");

        Connection->Invoke(MultiplayerHubMethods.Get(MultiplayerHubMethod::GET_CLIENT_ID), signalr::value(signalr::value_type::array), LocalCallback);
        return ClientIdRequestedContinuation;
    };
}

std::function<async::task<void>()> MultiplayerConnection::StartListening()
{
    return [this]() -> async::task<void>
    {
        // Shared pointer to keep alive in local callback
        auto StartListeningEvent = std::make_shared<async::event_task<void>>();
        auto StartListeningContinuation = StartListeningEvent->get_task();

        if (Connection == nullptr || !Connected)
        {
            throw ErrorCodeException(ErrorCode::NotConnected, "MultiplayerConnection::StartListening, Error not connected.");
        }

        std::function<void(signalr::value, std::exception_ptr)> LocalCallback
            = [StartListeningEvent](signalr::value /*Result*/, std::exception_ptr Except)
        {
            if (Except != nullptr)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
                StartListeningEvent->set_exception(
                    std::make_exception_ptr(ErrorCodeException(Error, "MultiplayerConnection::StartListening, Error when starting listening.")));
                return;
            }

            StartListeningEvent->set();
        };

        LogSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling StartListening");
        Connection->Invoke(
            MultiplayerHubMethods.Get(MultiplayerHubMethod::START_LISTENING), signalr::value(signalr::value_type::array), LocalCallback);

        return StartListeningContinuation;
    };
}

void MultiplayerConnection::Connect(ErrorCodeCallbackHandler Callback, ISignalRConnection* SignalRConnection,
    csp::multiplayer::SpaceEntitySystem& SpaceEntitySystem, [[maybe_unused]] const csp::common::String& MultiplayerUri,
    const csp::common::String& AccessToken, const csp::common::String& DeviceId)
{
    if (Connection != nullptr)
    {
        if (Connected)
        {
            INVOKE_IF_NOT_NULL(Callback, ErrorCode::AlreadyConnected);

            return;
        }

        delete Connection;
    }

// You will notice that the Emscripten webclient doesn't take the Uri into its constructor.
// This is because it uses the url passed into its Start function, which gets modified by SignalR.
// This modified version isn't compatible with our POCO implementation, so we need to directly use the MultiplayerUri.
#ifdef CSP_WASM
    WebSocketClient = new csp::multiplayer::CSPWebSocketClientEmscripten(AccessToken.c_str(), DeviceId.c_str());
#else
    WebSocketClient = new csp::multiplayer::CSPWebSocketClientPOCO(MultiplayerUri.c_str(), AccessToken.c_str(), DeviceId.c_str(), LogSystem);
#endif
    csp::multiplayer::SetWebSocketClient(WebSocketClient);

    Connection = SignalRConnection;
    NetworkEventManager->SetConnection(Connection);
    SpaceEntitySystem.SetConnection(Connection);

    EventBusPtr->StartEventMessageListening();

    // We register the network interruption callback as a wrapper because we want to unwrap any signalR exceptions.
    RegisterNetworkInterruptedCallback(Connection, LogSystem, NetworkInterruptionCallback);

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
        .then(async::inline_scheduler(), [this]() { Connected = true; })
        .then(async::inline_scheduler(), DeleteEntities(ALL_ENTITIES_ID))
        .then(async::inline_scheduler(), RequestClientId())
        .then(async::inline_scheduler(), [this](uint64_t RetrievedClientId) { ClientId = RetrievedClientId; })
        .then(async::inline_scheduler(), StartListening())
        .then(async::inline_scheduler(),
            [this, Callback]()
            {
                // Success
                INVOKE_IF_NOT_NULL(ConnectionCallback, "Successfully connected to SignalR hub.");
                INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
            })
        .then(async::inline_scheduler(),
            csp::common::continuations::InvokeIfExceptionInChain(
                // Handle any errors in chain
                [Callback, this](const std::exception& Except)
                {
                    auto [Error, ExceptionErrorMsg] = ParseMultiplayerError(Except);
                    DisconnectWithReason(ExceptionErrorMsg.c_str(), Callback);
                },
                LogSystem));
}

void MultiplayerConnection::Disconnect(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    DisconnectWithReason("Client called disconnect.", Callback);
}

void MultiplayerConnection::DisconnectWithReason(const csp::common::String& Reason, ErrorCodeCallbackHandler Callback)
{
    const ExceptionCallbackHandler StopHandler = [this, Callback, Reason](const std::exception_ptr& Except)
    {
        ErrorCode Error = ErrorCode::None;
        if (Except != nullptr)
        {
            auto [ExceptionError, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
            Error = ExceptionError;
        }

        Connected = false;
        INVOKE_IF_NOT_NULL(Callback, Error);
        INVOKE_IF_NOT_NULL(DisconnectionCallback, Reason);
    };

    Stop(StopHandler);
}

void MultiplayerConnection::Start(const ExceptionCallbackHandler Callback) const
{
    if (Connection == nullptr)
    {
        INVOKE_IF_NOT_NULL(Callback, std::make_exception_ptr(std::runtime_error("No Connection!")));

        return;
    }

    Connection->Start(Callback);
}

async::task<void> MultiplayerConnection::Start() const
{
    if (Connection == nullptr)
    {
        csp::common::continuations::LogErrorAndCancelContinuation("MultiplayerConnection::Start, SignalR connection pointer is null.", LogSystem);
    }

    // Shared pointer to keep alive in local callback
    auto OnCompleteEvent = std::make_shared<async::event_task<void>>();
    async::task<void> OnCompleteTask = OnCompleteEvent->get_task();

    Connection->Start(
        [OnCompleteEvent](std::exception_ptr exception)
        {
            if (exception)
            {
                auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(exception);
                OnCompleteEvent->set_exception(
                    std::make_exception_ptr(ErrorCodeException(Error, "MultiplayerConnection::Start, Error when starting SignalR connection.")));
                return;
            }

            OnCompleteEvent->set();
        });

    return OnCompleteTask;
}

void MultiplayerConnection::Stop(const ExceptionCallbackHandler Callback) const
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, std::make_exception_ptr(std::runtime_error("No Connection!")));

        return;
    }

    Connection->Stop(Callback);
}

void MultiplayerConnection::SetDisconnectionCallback(DisconnectionCallbackHandler Callback) { DisconnectionCallback = Callback; }

void MultiplayerConnection::SetConnectionCallback(ConnectionCallbackHandler Callback) { ConnectionCallback = Callback; }

CSP_EVENT void MultiplayerConnection::SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler Callback)
{
    NetworkInterruptionCallback = Callback;
}

void MultiplayerConnection::SetScopes(csp::common::String InSpaceId, ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value /*Result*/, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    std::vector<signalr::value> ScopesVec;

    // Set the scope using the Space Id
    ScopesVec.push_back(signalr::value(InSpaceId.c_str()));

    std::vector<signalr::value> ParamsVec;
    ParamsVec.push_back(ScopesVec);
    signalr::value Params = signalr::value(std::move(ParamsVec));

    Connection->Invoke(MultiplayerHubMethods.Get(MultiplayerHubMethod::SET_SCOPES), Params, LocalCallback);
}

void MultiplayerConnection::ResetScopes(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value /*Result*/, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    std::vector<signalr::value> ParamsVec;
    signalr::value Params = signalr::value(std::move(ParamsVec));
    Connection->Invoke(MultiplayerHubMethods.Get(MultiplayerHubMethod::RESET_SCOPES), Params, LocalCallback);
}

void MultiplayerConnection::StopListening(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value /*Result*/, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    LogSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling StopListening");

    Connection->Invoke(MultiplayerHubMethods.Get(MultiplayerHubMethod::STOP_LISTENING), signalr::value(signalr::value_type::array), LocalCallback);
}

uint64_t MultiplayerConnection::GetClientId() const { return ClientId; }

ConnectionState MultiplayerConnection::GetConnectionState() const
{
    if (Connection != nullptr)
    {
        return static_cast<ConnectionState>(Connection->GetConnectionState());
    }
    else
    {
        return ConnectionState::Disconnected;
    }
}

CSP_ASYNC_RESULT void MultiplayerConnection::SetAllowSelfMessagingFlag(const bool InAllowSelfMessaging, ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback
        = [this, Callback, InAllowSelfMessaging](signalr::value /*Result*/, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = ParseMultiplayerErrorFromExceptionPtr(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        AllowSelfMessaging = InAllowSelfMessaging;

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    LogSystem.LogMsg(csp::common::LogLevel::Verbose, "Calling SetAllowSelfMessaging");

    const std::vector InvokeArguments = { signalr::value(InAllowSelfMessaging) };
    Connection->Invoke(MultiplayerHubMethods.Get(MultiplayerHubMethod::SET_ALLOW_SELF_MESSAGING), InvokeArguments, LocalCallback);
}

bool MultiplayerConnection::GetAllowSelfMessagingFlag() const { return AllowSelfMessaging; }

} // namespace csp::multiplayer
