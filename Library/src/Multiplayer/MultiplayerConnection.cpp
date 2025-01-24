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
#include "CSP/Multiplayer/Conversation/ConversationSystem.h"
#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Multiplayer/EventSerialisation.h"
#include "Multiplayer/MultiplayerConstants.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"

#ifdef CSP_WASM
#include "Multiplayer/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.h"
#else
#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
#endif

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Web/Uri.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <thread>

using namespace std::chrono_literals;

namespace csp::multiplayer
{

static std::map<std::string, ErrorCode> ErrorCodeMap = { { "Scopes_ConcurrentUsersQuota", ErrorCode::SpaceUserLimitExceeded } };

ErrorCode ParseError(std::exception_ptr Exception)
{
    std::string ErrorMessage;

    try
    {
        rethrow_exception(Exception);
    }
    catch (const std::exception& e)
    {
        ErrorMessage = e.what();

        CSP_LOG_ERROR_FORMAT("%s\n", ErrorMessage.c_str());
    }

    constexpr const char* ERROR_CODE_KEY = "error code:";
    constexpr size_t ERROR_CODE_KEY_LENGTH = std::char_traits<char>::length(ERROR_CODE_KEY);

    auto Index = ErrorMessage.find(ERROR_CODE_KEY);

    if (Index == std::string::npos)
    {
        return ErrorCode::Unknown;
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
        return ErrorCodeMap[ErrorCodeString];
    }

    return ErrorCode::Unknown;
}

constexpr const uint64_t ALL_ENTITIES_ID = std::numeric_limits<uint64_t>::max();
constexpr const uint32_t KEEP_ALIVE_INTERVAL = 15;

/// @brief MultiplayerConnection
MultiplayerConnection::MultiplayerConnection()
    : Connection(nullptr)
    , WebSocketClient(nullptr)
    , NetworkEventManager(CSP_NEW NetworkEventManagerImpl(this))
    , ClientId(0)
    , Connected(false)
{
    EventBusPtr = CSP_NEW EventBus(this);
    ConversationSystemPtr = CSP_NEW ConversationSystem(this);
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

        CSP_DELETE(Connection);
        CSP_DELETE(WebSocketClient);
        CSP_DELETE(NetworkEventManager);
        CSP_DELETE(ConversationSystemPtr);
        CSP_DELETE(EventBusPtr);
    }
}

MultiplayerConnection::MultiplayerConnection(const MultiplayerConnection& InBoundConnection)
{
    Connection = InBoundConnection.Connection;
    WebSocketClient = InBoundConnection.WebSocketClient;
    NetworkEventManager = InBoundConnection.NetworkEventManager;
    ConversationSystemPtr = InBoundConnection.ConversationSystemPtr;
    ClientId = InBoundConnection.ClientId;
    DisconnectionCallback = InBoundConnection.DisconnectionCallback;
    ConnectionCallback = InBoundConnection.ConnectionCallback;
    NetworkInterruptionCallback = InBoundConnection.NetworkInterruptionCallback;
    EventBusPtr = InBoundConnection.EventBusPtr;
    Connected = (InBoundConnection.Connected) ? true : false;
}

void MultiplayerConnection::Connect(ErrorCodeCallbackHandler Callback)
{
    if (Connection != nullptr)
    {
        if (Connected)
        {
            INVOKE_IF_NOT_NULL(Callback, ErrorCode::AlreadyConnected);

            return;
        }

        CSP_DELETE(Connection);
    }

#ifdef CSP_WASM
    WebSocketClient = CSP_NEW csp::multiplayer::CSPWebSocketClientEmscripten();
#else
    WebSocketClient = CSP_NEW csp::multiplayer::CSPWebSocketClientPOCO();
#endif
    csp::multiplayer::SetWebSocketClient(WebSocketClient);

    Connection = CSP_NEW csp::multiplayer::SignalRConnection(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI.c_str(), KEEP_ALIVE_INTERVAL,
        std::make_shared<csp::multiplayer::CSPWebsocketClient>());
    NetworkEventManager->SetConnection(Connection);
    ConversationSystemPtr->SetConnection(Connection);
    csp::systems::SystemsManager::Get().GetSpaceEntitySystem()->SetConnection(Connection);

    EventBusPtr->StartEventMessageListening();

    // Initialise
    Start(
        [this, Callback](std::exception_ptr Except)
        {
            if (Except != nullptr)
            {
                auto Error = ParseError(Except);

                DisconnectWithReason("Error invoking 'Start' on server.", [](ErrorCode) {});

                INVOKE_IF_NOT_NULL(Callback, Error);

                return;
            }

            Connected = true;

            DeleteOwnedEntities(
                [this, Callback](ErrorCode Error)
                {
                    if (Error != ErrorCode::None)
                    {
                        INVOKE_IF_NOT_NULL(Callback, Error);

                        return;
                    }

                    RequestClientId(
                        [this, Callback](ErrorCode Error)
                        {
                            if (Error != ErrorCode::None)
                            {
                                INVOKE_IF_NOT_NULL(Callback, Error);

                                return;
                            }

                            Connection->SetDisconnected(
                                [this](const std::exception_ptr& Except)
                                {
                                    std::string DisconnectMessage = "Connection Closed.";

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

                                    CSP_LOG_MSG(csp::systems::LogLevel::Log, DisconnectMessage.c_str());
                                });

                            StartListening(Callback);
                        });
                });
        });
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
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        Connected = false;

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
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

void MultiplayerConnection::InternalDeleteEntity(uint64_t EntityId, ErrorCodeCallbackHandler Callback) const
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
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

    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Calling DeleteObjects");

    Connection->Invoke("DeleteObjects", DeleteEntityMessage, LocalCallback);
}

void MultiplayerConnection::DeleteOwnedEntities(ErrorCodeCallbackHandler Callback) { InternalDeleteEntity(ALL_ENTITIES_ID, Callback); }

/*
   Requests the connected Client ID from CHS.
 */
void MultiplayerConnection::RequestClientId(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [this, Callback](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        CSP_LOG_FORMAT(csp::systems::LogLevel::Verbose, "ClientId=%i", Result.as_uinteger());

        this->ClientId = Result.as_uinteger();

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Calling GetClientId");

    Connection->Invoke("GetClientId", signalr::value(signalr::value_type::array), LocalCallback);
}

void MultiplayerConnection::SetScopes(csp::common::String InSpaceId, ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
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

    Connection->Invoke("SetScopes", Params, LocalCallback);
}

void MultiplayerConnection::ResetScopes(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    std::vector<signalr::value> ParamsVec;
    signalr::value Params = signalr::value(std::move(ParamsVec));
    Connection->Invoke("ResetScopes", Params, LocalCallback);
}

void MultiplayerConnection::StartListening(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Calling StartListening");

    Connection->Invoke("StartListening", signalr::value(signalr::value_type::array), LocalCallback);
}

void MultiplayerConnection::StopListening(ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr || !Connected)
    {
        INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

        return;
    }

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Calling StopListening");

    Connection->Invoke("StopListening", signalr::value(signalr::value_type::array), LocalCallback);
}

uint64_t MultiplayerConnection::GetClientId() const { return ClientId; }

ConversationSystem* MultiplayerConnection::GetConversationSystem() const { return ConversationSystemPtr; }

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
        = [this, Callback, InAllowSelfMessaging](signalr::value Result, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto Error = ParseError(Except);
            INVOKE_IF_NOT_NULL(Callback, Error);

            return;
        }

        AllowSelfMessaging = InAllowSelfMessaging;

        INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
    };

    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "Calling SetAllowSelfMessaging");

    const std::vector InvokeArguments = { signalr::value(InAllowSelfMessaging) };
    Connection->Invoke("SetAllowSelfMessaging", InvokeArguments, LocalCallback);
}

bool MultiplayerConnection::GetAllowSelfMessagingFlag() const { return AllowSelfMessaging; }

} // namespace csp::multiplayer
