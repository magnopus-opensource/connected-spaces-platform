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
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/Assets/Asset.h"

#include <atomic>
#include <functional>
#include <map>
#include <vector>


namespace csp::systems
{

class SpaceSystem;
class SystemsManager;
class UserSystem;

} // namespace csp::systems

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory


/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{

class ReplicatedValue;
class SpaceEntitySystem;
class ConversationSystem;
class ClientElectionManager;
class SignalRConnection;
class IWebSocketClient;


/// @brief Enum used to specify the current state of the multiplayer connection.
enum class ConnectionState
{
	Connecting,
	Connected,
	Disconnecting,
	Disconnected
};


/// @brief Enum used to indicate the failure state of a multiplayer request.
enum class ErrorCode
{
	None,
	Unknown,
	NotConnected,
	AlreadyConnected,
	SpaceUserLimitExceeded
};


/// @ingroup Multiplayer
/// @brief Handling of all multiplayer connection functionality, such as connect, disconnect, entity replication and network events.
class CSP_API MultiplayerConnection
{
public:
	/** @cond DO_NOT_DOCUMENT */
	friend class ConversationSystem;
	friend class csp::systems::SpaceSystem;
	friend class csp::systems::SystemsManager;
	friend class csp::systems::UserSystem;
	friend class SpaceEntityEventHandler;
	friend class SpaceEntitySystem;
	friend class ClientElectionManager;
	friend class ClientElectionEventHandler;
	friend void csp::memory::Delete<MultiplayerConnection>(MultiplayerConnection* Ptr);
	/** @endcond */

	// Simple callback that receives an error code.
	typedef std::function<void(ErrorCode)> ErrorCodeCallbackHandler;

	// The callback used to register to listen to network events.
	typedef std::function<void(bool, const csp::common::Array<ReplicatedValue>&)> ParameterisedCallbackHandler;

	// The callback for disconnection, contains a string with a reason for disconnection.
	typedef std::function<void(const csp::common::String&)> DisconnectionCallbackHandler;

	// The callback for connection, contains a string with a status of connection.
	typedef std::function<void(const csp::common::String&)> ConnectionCallbackHandler;

	// The callback for network interruption, contains a string showing failure.
	typedef std::function<void(const csp::common::String&)> NetworkInterruptionCallbackHandler;

	// The callback to leave scopes, contains an array of strings with the scopes, and a string with the reason for leaving.
	typedef std::function<void(const csp::common::Array<csp::common::String>&, const csp::common::String&)> LeavingScopesCallbackHandler;

	// The callback for receiving asset detail changes, contains an AssetDetailBlobParams with the details.
	typedef std::function<void(const AssetDetailBlobParams&)> AssetDetailBlobChangedCallbackHandler;

	// Callback to receive ConversationSystem Data when a message is sent.
	typedef std::function<void(const ConversationSystemParams&)> ConversationSystemCallbackHandler;

	// Callback to receive access permission changes Data when a message is sent.
	typedef std::function<void(const UserPermissionsParams&)> UserPermissionsChangedCallbackHandler;

	// Callback to receive sequence changes, contains a SequenceChangedParams with the details.
	typedef std::function<void(const SequenceChangedParams&)> SequenceChangedCallbackHandler;

	// Callback to receive hotspot sequence changes, contains a SequenceHotspotChangedParams with the details.
	typedef std::function<void(const SequenceHotspotChangedParams&)> HotspotSequenceChangedCallbackHandler;

	/// @brief Sends a network event by EventName to all currently connected clients.
	/// @param EventName csp::common::String : The identifying name for the event.
	/// @param Args csp::common::Array<ReplicatedValue> : An array of arguments (ReplicatedValue) to be passed as part of the event payload.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	CSP_ASYNC_RESULT void
		SendNetworkEvent(const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback);

	/// @brief Sends a network event by EventName, to TargetClientId.
	/// @param EventName csp::common::String : The identifying name for the event.
	/// @param Args csp::common::Array<ReplicatedValue> : An array of arguments (ReplicatedValue) to be passed as part of the event payload.
	/// @param TargetClientId uint64_t : The client ID to send the event to.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	CSP_ASYNC_RESULT void SendNetworkEventToClient(const csp::common::String& EventName,
												   const csp::common::Array<ReplicatedValue>& Args,
												   uint64_t TargetClientId,
												   ErrorCodeCallbackHandler Callback);

	/// @brief Sets a callback for a disconnection event.
	/// @param Callback DisconnectionCallbackHandler : The callback for disconnection, contains a string with a reason for disconnection.
	CSP_EVENT void SetDisconnectionCallback(DisconnectionCallbackHandler Callback);

	/// @brief Sets a callback for a connection event
	/// @param Callback ConnectionCallbackHandler : The callback for connection, contains a string with a status of connection (Success, Failure etc).
	CSP_EVENT void SetConnectionCallback(ConnectionCallbackHandler Callback);

	/// @brief Sets a callback for a network interruption event.
	/// Connection isn't recoverable after this point and Disconnect should be called.
	/// @param Callback NetworkInterruptionCallbackHandler : The callback for network interruption, contains a string showing failure.
	CSP_EVENT void SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler Callback);

	/// @brief Sets a callback for a leave scopes event.
	/// @param Callback LeavingScopesCallbackHandler : The callback for leaving scopes, contains an array of strings with the scopes, and a string
	/// with the reason for leaving.
	CSP_EVENT void SetLeaveScopesCallback(LeavingScopesCallbackHandler Callback);

	/// @brief Sets a callback for an asset changed event.
	/// @param Callback AssetDetailBlobChangedCallbackHandler: Callback to receive data for the asset that has been changed.
	CSP_EVENT void SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler Callback);

	/// @brief Sets a callback for a conversation new message event.
	/// @param Callback ConversationMessageCallbackHandler: Callback to receive ConversationSystem Data when a message is sent.
	/// Callback will have to reset the callback passed to the system to avoid "dangling objects" after use.
	CSP_EVENT void SetConversationSystemCallback(ConversationSystemCallbackHandler Callback);

	/// @brief Sets a callback for an access control changed event.
	/// @param Callback UserPermissionsChangedCallbackHandler: Callback to receive data for the user permissions that has been changed.
	CSP_EVENT void SetUserPermissionsChangedCallback(UserPermissionsChangedCallbackHandler Callback);

	/// @brief Sets a callback for a sequence changed event.
	/// @param Callback SequenceChangedCallbackHandler: Callback to receive data for the sequence that has been changed.
	CSP_EVENT void SetSequenceChangedCallback(SequenceChangedCallbackHandler Callback);

	/// @brief Sets a callback to be fired when a hotspot sequence is changed.
	/// @param Callback HotspotSequenceChangedCallbackHandler: Callback to receive data for the hotspot sequence that has been changed.
	CSP_EVENT void SetHotspotSequenceChangedCallback(HotspotSequenceChangedCallbackHandler Callback);

	/// @brief Registers a callback to listen for the named event.
	/// @param EventName csp::common::String : The identifying name for the event to listen for.
	/// @param Callback ParameterisedCallbackHandler : A callback to register for the event which contains the parameter payload data.
	void ListenNetworkEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback);

	/// @brief Stops the multiplayer connection from listening for a particular network event.
	/// @param EventName csp::common::String : The identifying name for the event to stop listening for.
	void StopListenNetworkEvent(const csp::common::String& EventName);

	/// @brief Requests the ClientID.
	/// @return uint64_t the ClientID for this connection.
	uint64_t GetClientId() const;

	/// @brief Gets a pointer to the conversation system.
	/// @return A pointer to the conversation system.
	ConversationSystem* GetConversationSystem() const;

	/// @brief Gets the current connection state.
	/// @return A ConnectionState enum value.
	ConnectionState GetConnectionState() const;

	/// @brief Sets the Self Messaging flag for this client.
	/// This allows a client to declare that it wishes to recieve every patch and object message it sends.
	/// @warning Don't use this function if you aren't sure of the consequences, it's very unlikely that a client would want to use this!
	/// @param AllowSelfMessaging bool : True to allow and false to disallow.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	CSP_ASYNC_RESULT void SetAllowSelfMessagingFlag(const bool AllowSelfMessaging, ErrorCodeCallbackHandler Callback);

	/// @brief Gets the bool representing if we're using self-messaging or not.
	/// @return True if self messaging is allowed, false otherwise.
	bool GetAllowSelfMessagingFlag() const;

private:
	MultiplayerConnection();
	~MultiplayerConnection();

	MultiplayerConnection(const MultiplayerConnection& InBoundConnection);

	typedef std::function<void(std::exception_ptr)> ExceptionCallbackHandler;

	/// @brief Start the connection and register to start receiving updates from the server.
	/// Connect should be called after LogIn and before EnterSpace.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	void Connect(ErrorCodeCallbackHandler Callback);

	/// @brief End the multiplayer connection.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	void Disconnect(ErrorCodeCallbackHandler Callback);

	void Start(ExceptionCallbackHandler Callback) const;
	void Stop(ExceptionCallbackHandler Callback) const;

	void StartListening(ErrorCodeCallbackHandler Callback);
	void StopListening(ErrorCodeCallbackHandler Callback);

	void InternalDeleteEntity(uint64_t EntityId, ErrorCodeCallbackHandler Callback) const;
	void DeleteOwnedEntities(ErrorCodeCallbackHandler Callback);

	/// @brief Subscribes the connected user to the specified space's scope.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	void SetScopes(csp::common::String InSpaceId, ErrorCodeCallbackHandler Callback);

	void LeaveScopes(ErrorCodeCallbackHandler Callback);

	/// @brief Clears the connected user's subscription to their current set of scopes.
	/// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
	void ResetScopes(ErrorCodeCallbackHandler Callback);

	void RequestClientId(ErrorCodeCallbackHandler Callback);

	void DisconnectWithReason(const csp::common::String& Reason, ErrorCodeCallbackHandler Callback);

	void StartEventMessageListening();

	class csp::multiplayer::SignalRConnection* Connection;
	class csp::multiplayer::IWebSocketClient* WebSocketClient;
	class NetworkEventManagerImpl* NetworkEventManager;
	ConversationSystem* ConversationSystemPtr;

	uint64_t ClientId;

	DisconnectionCallbackHandler DisconnectionCallback;
	ConnectionCallbackHandler ConnectionCallback;
	NetworkInterruptionCallbackHandler NetworkInterruptionCallback;
	LeavingScopesCallbackHandler LeavingScopesCallback;
	AssetDetailBlobChangedCallbackHandler AssetDetailBlobChangedCallback;
	ConversationSystemCallbackHandler ConversationSystemCallback;
	UserPermissionsChangedCallbackHandler UserPermissionsChangedCallback;

	SequenceChangedCallbackHandler SequenceChangedCallback;
	HotspotSequenceChangedCallbackHandler HotspotSequenceChangedCallback;

	// TODO: Replace these with pointers! We can't use STL containers as class fields due to the fact that the class size will
	//   change depending on which runtime is used.
	typedef std::vector<ParameterisedCallbackHandler> Callbacks;
	std::map<csp::common::String, Callbacks> NetworkEventMap;

	std::atomic_bool Connected;
	uint32_t KeepAliveSeconds = 120;

	bool AllowSelfMessaging = false;
};

} // namespace csp::multiplayer
