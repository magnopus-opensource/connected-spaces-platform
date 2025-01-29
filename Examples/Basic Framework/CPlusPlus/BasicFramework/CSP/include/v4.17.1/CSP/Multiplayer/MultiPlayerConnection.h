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
}

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{

class ReplicatedValue;
class SpaceEntitySystem;
class ConversationSystem;
class ClientElectionManager;
class SignalRConnection;
class IWebSocketClient;

/// @brief Enum used to specify the current state of the muiltiplayer connection.
enum class ConnectionState
{
    Connecting,
    Connected,
    Disconnecting,
    Disconnected
};

/// @ingroup Multiplayer
/// @brief Handling of all multiplayer connection functionality, such as connect, disconnect, entity replication and network events.
class CSP_API MultiplayerConnection
{
public:
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceEntitySystem;
    friend class ConversationSystem;
    friend class SpaceEntityEventHandler;
    friend class ClientElectionManager;
    friend class ClientElectionEventHandler;
    friend class csp::systems::SpaceSystem;
    /** @endcond */

    MultiplayerConnection(csp::common::String InSpaceId);
    ~MultiplayerConnection();

    MultiplayerConnection(const MultiplayerConnection& InBoundConnection);

    // Simple callback that provides a success/fail boolean.
    typedef std::function<void(bool)> CallbackHandler;

    // The callback used to register to listen to network events.
    typedef std::function<void(bool, const csp::common::Array<ReplicatedValue>&)> ParameterisedCallbackHandler;

    // The callback for disconnection, contains a string with a reason for disconnection.
    typedef std::function<void(const csp::common::String&)> DisconnectionCallbackHandler;

    // The callback for connection, contains a string with a status of connection.
    typedef std::function<void(const csp::common::String&)> ConnectionCallbackHandler;

    // The callback for network interruption, contains a string showing failure.
    typedef std::function<void(const csp::common::String&)> NetworkInterruptionCallbackHandler;

    // The callback for receiving asset detail changes, contains an AssetDetailBlobParams with the details.
    typedef std::function<void(const AssetDetailBlobParams&)> AssetDetailBlobChangedCallbackHandler;

    // Callback to receive ConversationSystem Data when a message is sent.
    typedef std::function<void(const ConversationSystemParams&)> ConversationSystemCallbackHandler;

    // Callback to receive access permission changes Data when a message is sent.
    typedef std::function<void(const UserPermissionsParams&)> UserPermissionsChangedCallbackHandler;

    /// @brief Start the connection and register to start receiving updates from the server.
    /// @param Callback CallbackHandler : a callback with success status.
    CSP_ASYNC_RESULT void Connect(CallbackHandler Callback);

    /// @brief End the multiplayer connection.
    /// @param Callback CallbackHandler: a callback with success status.
    CSP_ASYNC_RESULT void Disconnect(CallbackHandler Callback);

    /// @brief Initialise the connection and get initial entity data from the server.
    /// @param Callback CallbackHandler : a callback with success status.
    CSP_ASYNC_RESULT void InitialiseConnection(CallbackHandler Callback);

    /// @brief Sends a network event by EventName to all currently connected clients.
    /// @param EventName csp::common::String : The identifying name for the event.
    /// @param Args csp::common::Array<ReplicatedValue> : An array of arguments (ReplicatedValue) to be passed as part of the event payload.
    /// @param Callback CallbackHandler : A status callback which indicates if the event successfully sent.
    CSP_ASYNC_RESULT void SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, CallbackHandler Callback);

    /// @brief Sends a network event by EventName, to TargetClientId.
    /// @param EventName csp::common::String : The identifying name for the event.
    /// @param Args csp::common::Array<ReplicatedValue> : An array of arguments (ReplicatedValue) to be passed as part of the event payload.
    /// @param TargetClientId uint64_t : The client ID to send the event to.
    /// @param Callback CallbackHandler : A status callback which indicates if the event successfully sent.
    CSP_ASYNC_RESULT void SendNetworkEventToClient(
        const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, uint64_t TargetClientId, CallbackHandler Callback);

    /// @brief Sets a callback for a disconnection event.
    /// @param Callback DisconnectionCallbackHandler : The callback for disconnection, contains a string with a reason for disconnection.
    CSP_EVENT void SetDisconnectionCallback(DisconnectionCallbackHandler Callback);

    /// @brief Sets a callback for a connection event
    /// @param Callback ConnectionCallbackHandler : The callback for connection, contains a string with a status of connection (Success, Failure etc).
    CSP_EVENT void SetConnectionCallback(ConnectionCallbackHandler Callback);

    /// @brief Sets a callback for a network interruption event.
    ///
    /// Connection isn't recoverable after this point and Disconnect should be called.
    ///
    /// @param Callback NetworkInterruptionCallbackHandler : The callback for network interruption, contains a string showing failure.
    CSP_EVENT void SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler Callback);

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

    /// @brief Gets a pointer to the space entity system.
    /// @return A pointer to the space entity system.
    SpaceEntitySystem* GetSpaceEntitySystem() const;

    /// @brief Gets a pointer to the conversation system.
    /// @return A pointer to the conversation system.
    ConversationSystem* GetConversationSystem() const;

    /// @brief Gets the current connection state.
    /// @return A ConnectionState enum value.
    ConnectionState GetConnectionState() const;

    /// @brief Sets the Self Messaging flag for this client.
    ///
    /// This allows a client to declare that it wishes to recieve every patch and object message it sends.
    ///
    /// @warning Don't use this function if you aren't sure of the consequences, it's very unlikely that a client would want to use this!
    /// @param AllowSelfMessaging bool : True to allow and false to disallow.
    /// @param Callback CallbackHandler : Callback providing success/fail boolean.
    CSP_ASYNC_RESULT void SetAllowSelfMessagingFlag(const bool AllowSelfMessaging, CallbackHandler Callback);

    /// @brief Gets the bool representing if we're using self-messaging or not.
    /// @return True if self messaging is allowed, false otherwise.
    bool GetAllowSelfMessagingFlag() const;

private:
    typedef std::function<void(std::exception_ptr)> ExceptionCallbackHandler;

    void Start(ExceptionCallbackHandler Callback) const;
    void Stop(ExceptionCallbackHandler Callback) const;

    void StartListening(CallbackHandler Callback);
    void StopListening(CallbackHandler Callback);

    void InternalDeleteEntity(uint64_t EntityId, CallbackHandler Callback) const;
    void DeleteOwnedEntities(CallbackHandler Callback);
    void SetScopes(CallbackHandler Callback);
    void RequestClientId(CallbackHandler Callback);

    void DisconnectWithReason(const csp::common::String& Reason, CallbackHandler Callback);

    void StartEventMessageListening();

    void Cleanup();
    class csp::multiplayer::SignalRConnection* Connection;
    class csp::multiplayer::IWebSocketClient* WebSocketClient;
    class NetworkEventManagerImpl* NetworkEventManager;
    SpaceEntitySystem* SpaceEntitySystemPtr;
    ConversationSystem* ConversationSystemPtr;

    uint64_t ClientId;

    csp::common::String SpaceId;

    DisconnectionCallbackHandler DisconnectionCallback;
    ConnectionCallbackHandler ConnectionCallback;
    NetworkInterruptionCallbackHandler NetworkInterruptionCallback;
    AssetDetailBlobChangedCallbackHandler AssetDetailBlobChangedCallback;
    ConversationSystemCallbackHandler ConversationSystemCallback;
    UserPermissionsChangedCallbackHandler UserPermissionsChangedCallback;

    typedef std::vector<ParameterisedCallbackHandler> Callbacks;
    std::map<csp::common::String, Callbacks> NetworkEventMap;

    std::atomic_bool Connected;
    uint32_t KeepAliveSeconds = 120;

    bool AllowSelfMessaging = false;
};

} // namespace csp::multiplayer
