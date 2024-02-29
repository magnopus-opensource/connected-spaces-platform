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
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Multiplayer/EventSerialisation.h"
#include "Multiplayer/MultiplayerKeyConstants.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"
#include "CallHelpers.h"

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
#include <iostream>
#include <map>
#include <thread>


using namespace std::chrono_literals;


namespace csp::multiplayer
{

static std::map<std::string, ErrorCode> ErrorCodeMap = {{"Scopes_ConcurrentUsersQuota", ErrorCode::SpaceUserLimitExceeded}};


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

	constexpr const char* ERROR_CODE_KEY   = "error code:";
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


constexpr const uint64_t ALL_ENTITIES_ID	 = -1;
constexpr const uint64_t ALL_CLIENTS_ID		 = -1;
constexpr const uint32_t KEEP_ALIVE_INTERVAL = 15;


/// @brief MultiplayerConnection
MultiplayerConnection::MultiplayerConnection()
	: Connection(nullptr)
	, WebSocketClient(nullptr)
	, NetworkEventManager(CSP_NEW NetworkEventManagerImpl(this))
	, SpaceEntitySystemPtr(CSP_NEW SpaceEntitySystem(this))
	, ClientId(0)
	, Connected(false)
	, ConversationSystemPtr(CSP_NEW ConversationSystem(this))
{
}

MultiplayerConnection::~MultiplayerConnection()
{
	if (Connection != nullptr)
	{
		if (Connected)
		{
			const auto _Connection			  = Connection;
			const auto _WebSocketClient		  = WebSocketClient;
			const auto _NetworkEventManager	  = NetworkEventManager;
			const auto _SpaceEntitySystemPtr  = SpaceEntitySystemPtr;
			const auto _ConversationSystemPtr = ConversationSystemPtr;

			DisconnectWithReason("MultiplayerConnection shutting down.",
								 [_Connection, _WebSocketClient, _NetworkEventManager, _SpaceEntitySystemPtr, _ConversationSystemPtr](ErrorCode)
								 {
									 CSP_DELETE(_Connection);
									 CSP_DELETE(_WebSocketClient);
									 CSP_DELETE(_NetworkEventManager);
									 CSP_DELETE(_SpaceEntitySystemPtr);
									 CSP_DELETE(_ConversationSystemPtr);
								 });
		}
		else
		{
			CSP_DELETE(Connection);
			CSP_DELETE(WebSocketClient);
			CSP_DELETE(NetworkEventManager);
			CSP_DELETE(SpaceEntitySystemPtr);
			CSP_DELETE(ConversationSystemPtr);
		}
	}
}

MultiplayerConnection::MultiplayerConnection(const MultiplayerConnection& InBoundConnection)
{
	Connection					   = InBoundConnection.Connection;
	WebSocketClient				   = InBoundConnection.WebSocketClient;
	NetworkEventManager			   = InBoundConnection.NetworkEventManager;
	SpaceEntitySystemPtr		   = InBoundConnection.SpaceEntitySystemPtr;
	ConversationSystemPtr		   = InBoundConnection.ConversationSystemPtr;
	ClientId					   = InBoundConnection.ClientId;
	DisconnectionCallback		   = InBoundConnection.DisconnectionCallback;
	ConnectionCallback			   = InBoundConnection.ConnectionCallback;
	NetworkInterruptionCallback	   = InBoundConnection.NetworkInterruptionCallback;
	AssetDetailBlobChangedCallback = InBoundConnection.AssetDetailBlobChangedCallback;
	ConversationSystemCallback	   = InBoundConnection.ConversationSystemCallback;
	Connected					   = (InBoundConnection.Connected) ? true : false;
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

	Connection = CSP_NEW csp::multiplayer::SignalRConnection(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI.c_str(),
															 KEEP_ALIVE_INTERVAL,
															 std::make_shared<csp::multiplayer::CSPWebsocketClient>());
	NetworkEventManager->SetConnection(Connection);
	SpaceEntitySystemPtr->SetConnection(Connection);
	ConversationSystemPtr->SetConnection(Connection);

	StartEventMessageListening();

	// Initialise
	Start(
		[this, Callback](std::exception_ptr Except)
		{
			if (Except != nullptr)
			{
				auto Error = ParseError(Except);

				DisconnectWithReason("Error invoking 'Start' on server.",
									 [](ErrorCode)
									 {
									 });

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
						});
				});
		});
}

void MultiplayerConnection::InitialiseConnection(csp::common::String InSpaceId, ErrorCodeCallbackHandler Callback)
{
	if (Connection == nullptr || !Connected)
	{
		INVOKE_IF_NOT_NULL(Callback, ErrorCode::NotConnected);

		return;
	}

    SetScopes(InSpaceId,
			[this, Callback](ErrorCode Error)
			{
				if (Error != ErrorCode::None)
				{
					INVOKE_IF_NOT_NULL(Callback, Error);

					return;
				}

				StartListening(Callback);
			});

	// TODO: Support getting errors from RetrieveAllEntities
	SpaceEntitySystemPtr->RetrieveAllEntities();

	INVOKE_IF_NOT_NULL(Callback, ErrorCode::None);
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
	Cleanup();

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

void MultiplayerConnection::SendNetworkEvent(const csp::common::String& EventName,
											 const csp::common::Array<ReplicatedValue>& Args,
											 ErrorCodeCallbackHandler Callback)
{
	SendNetworkEventToClient(EventName, Args, ALL_CLIENTS_ID, Callback);
}

void MultiplayerConnection::SendNetworkEventToClient(const csp::common::String& EventName,
													 const csp::common::Array<ReplicatedValue>& Args,
													 uint64_t TargetClientId,
													 ErrorCodeCallbackHandler Callback)
{
	NetworkEventManager->SendNetworkEvent(EventName, Args, TargetClientId, Callback);
}

void MultiplayerConnection::SetDisconnectionCallback(DisconnectionCallbackHandler Callback)
{
	DisconnectionCallback = Callback;
}

void MultiplayerConnection::SetConnectionCallback(ConnectionCallbackHandler Callback)
{
	ConnectionCallback = Callback;
}

CSP_EVENT void MultiplayerConnection::SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler Callback)
{
	NetworkInterruptionCallback = Callback;
}

CSP_EVENT void MultiplayerConnection::SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler Callback)
{
	AssetDetailBlobChangedCallback = Callback;
}

CSP_EVENT void MultiplayerConnection::SetConversationSystemCallback(ConversationSystemCallbackHandler Callback)
{
	ConversationSystemCallback = Callback;
}

void MultiplayerConnection::SetUserPermissionsChangedCallback(UserPermissionsChangedCallbackHandler Callback)
{
	UserPermissionsChangedCallback = Callback;
}

void MultiplayerConnection::ListenNetworkEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback)
{
	if (Connection == nullptr || !Connected)
	{
		return;
	}

	NetworkEventMap[EventName].push_back(Callback);
}

void MultiplayerConnection::StopListenNetworkEvent(const csp::common::String& EventName)
{
	NetworkEventMap.erase(EventName);
}

// Begin listening to EventMessages from CHS. Must be called before Connection->Start.
void MultiplayerConnection::StartEventMessageListening()
{
	if (Connection == nullptr)
	{
		return;
	}

	std::function<void(signalr::value)> LocalCallback = [this](signalr::value Result)
	{
		if (Result.is_null())
		{
			return;
		}

		std::vector<signalr::value> EventValues = Result.as_array()[0].as_array();
		const csp::common::String EventType(EventValues[0].as_string().c_str());

		if (EventType == "AssetDetailBlobChanged")
		{
			if (!AssetDetailBlobChangedCallback)
			{
				return;
			}

			AssetChangedEventDeserialiser Deserialiser;
			Deserialiser.Parse(EventValues);
			AssetDetailBlobChangedCallback(Deserialiser.GetEventParams());
		}
		else if (EventType == "ConversationSystem")
		{
			if (!ConversationSystemCallback)
			{
				return;
			}

			ConversationEventDeserialiser Deserialiser;
			Deserialiser.Parse(EventValues);
			ConversationSystemCallback(Deserialiser.GetEventParams());
		}
		else if (EventType == "AccessControlChanged")
		{
			if (!UserPermissionsChangedCallback)
			{
				return;
			}

			UserPermissionsChangedEventDeserialiser Deserialiser;
			Deserialiser.Parse(EventValues);
			UserPermissionsChangedCallback(Deserialiser.GetEventParams());
		}
		else
		{
			// For everything else, use the generic deserialiser
			EventDeserialiser Deserialiser;
			Deserialiser.Parse(EventValues);

			for (const auto& Callback : NetworkEventMap[EventType])
			{
				Callback(true, Deserialiser.GetEventData());
			}
		}
	};

	Connection->On("OnEventMessage", LocalCallback);
}

void MultiplayerConnection::Cleanup()
{
	if (SpaceEntitySystemPtr == nullptr)
	{
		return;
	}

	SpaceEntitySystemPtr->LockEntityUpdate();

	const auto NumEntities = SpaceEntitySystemPtr->GetNumEntities();

	for (size_t i = 0; i < NumEntities; ++i)
	{
		SpaceEntity* Entity = SpaceEntitySystemPtr->GetEntityByIndex(i);

		// We automatically invoke SignalR deletion for all transient entities that were owned by this local client
		// as these are only ever valid for a single connected session
		if (Entity->GetIsTransient() && Entity->GetOwnerId() == GetClientId())
		{
			SpaceEntitySystemPtr->DestroyEntity(Entity,
												[](auto Ok)
												{
												});
		}
		// Otherwise we clear up all all locally represented entities
		else
		{
			SpaceEntitySystemPtr->LocalDestroyEntity(Entity);
		}
	}

	// Flush all pending adds/removes/updates
	SpaceEntitySystemPtr->ProcessPendingEntityOperations();

	SpaceEntitySystemPtr->UnlockEntityUpdate();
}

void MultiplayerConnection::InternalDeleteEntity(uint64_t EntityId, ErrorCodeCallbackHandler Callback) const
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

void MultiplayerConnection::DeleteOwnedEntities(ErrorCodeCallbackHandler Callback)
{
	InternalDeleteEntity(ALL_ENTITIES_ID, Callback);
}

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

	std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [this, Callback](signalr::value Result, std::exception_ptr Except)
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


void MultiplayerConnection::StartListening(ErrorCodeCallbackHandler Callback)
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

	std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [this, Callback](signalr::value Result, std::exception_ptr Except)
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

uint64_t MultiplayerConnection::GetClientId() const
{
	return ClientId;
}

SpaceEntitySystem* MultiplayerConnection::GetSpaceEntitySystem() const
{
	return SpaceEntitySystemPtr;
}

ConversationSystem* MultiplayerConnection::GetConversationSystem() const
{
	return ConversationSystemPtr;
}

ConnectionState MultiplayerConnection::GetConnectionState() const
{
	return static_cast<ConnectionState>(Connection->GetConnectionState());
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

	const std::vector InvokeArguments = {signalr::value(InAllowSelfMessaging)};
	Connection->Invoke("SetAllowSelfMessaging", InvokeArguments, LocalCallback);
}

bool MultiplayerConnection::GetAllowSelfMessagingFlag() const
{
	return AllowSelfMessaging;
}

} // namespace csp::multiplayer
