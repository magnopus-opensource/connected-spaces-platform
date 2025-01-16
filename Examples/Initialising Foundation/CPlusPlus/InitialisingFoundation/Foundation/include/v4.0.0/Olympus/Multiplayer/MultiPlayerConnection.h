#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/Conversation/Conversation.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/Assets/Asset.h"

#include <atomic>
#include <functional>
#include <map>
#include <vector>

namespace oly_systems
{
class SpaceSystem;
}
namespace oly_multiplayer
{
class ClientElectionManager;
class SignalRConnection;
class IWebSocketClient;

} // namespace oly_multiplayer

namespace oly_multiplayer
{

class ReplicatedValue;
class SpaceEntitySystem;
class ConversationSystem;

enum class EAssetChangeType
{
    Created,
    Updated,
    Deleted,
    MusubiFailed,
    Invalid,
    Num
};

class OLY_API AssetDetailBlobParams
{
public:
    EAssetChangeType ChangeType;
    oly_common::String AssetId;
    oly_common::String Version;
    oly_systems::EAssetType AssetType;
    oly_common::String AssetCollectionId;
};

class OLY_API ConversationSystemParams
{
public:
    ConversationMessageType MessageType;
    oly_common::String MessageValue;
};

enum class ConnectionState
{
    Connecting,
    Connected,
    Disconnecting,
    Disconnected
};

/**
 * @ingroup Multiplayer
 * @brief Handling of all multiplayer connection functionality, such as connect, disconnect, Entity replication and network events.
 */
class OLY_API MultiplayerConnection
{
public:
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceEntitySystem;
    friend class ConversationSystem;
    friend class SpaceEntityEventHandler;
    friend class ClientElectionManager;
    friend class ClientElectionEventHandler;
    friend class oly_systems::SpaceSystem;
    /** @endcond */
    [[deprecated("Multiplayer construction at the client layer will soon be removed. Please migrate your multiplayer connection related code to "
                 "instead use the object returned when entering a space.")]] MultiplayerConnection(oly_common::String InSpaceId);
    [[deprecated("Multiplayer deconstruction at the client layer will soon be removed. Please migrate your multiplayer connection related code to "
                 "passing the object when exting a space.")]] ~MultiplayerConnection();

    MultiplayerConnection(const MultiplayerConnection& InBoundConnection);

    typedef std::function<void(bool)> CallbackHandler;
    typedef std::function<void(bool, const oly_common::Array<ReplicatedValue>&)> ParameterisedCallbackHandler;
    typedef std::function<void(const oly_common::String&)> DisconnectionCallbackHandler;
    typedef std::function<void(const oly_common::String&)> ConnectionCallbackHandler;
    typedef std::function<void(const oly_common::String&)> NetworkInterruptionCallbackHandler;
    typedef std::function<void(const AssetDetailBlobParams&)> AssetDetailBlobChangedCallbackHandler;
    typedef std::function<void(const ConversationSystemParams&)> ConversationSystemCallbackHandler;

    /**
     * @brief Start the connection and register to start receiving updates from the server.
     * @param Callback CallbackHandler: a callback with success status.
     */
    OLY_ASYNC_RESULT void Connect(CallbackHandler Callback);

    /**
     * @brief Disconnect.
     * @param Callback CallbackHandler: a callback with success status.
     */
    OLY_ASYNC_RESULT void Disconnect(CallbackHandler Callback);

    /**
     * @brief Initialise the connection and get initial entity data from the server.
     * @param Callback CallbackHandler: a callback with success status.
     */
    OLY_ASYNC_RESULT void InitialiseConnection(CallbackHandler Callback);

    /**
     * @brief Sends a network event by EventName to all currently connected clients.
     * @param EventName oly_common::String: The identifying name for the event.
     * @param Args oly_common::Array<ReplicatedValue>: An array of arguments (ReplicatedValue) to be passed as part of the event payload.
     * @param Callback CallbackHandler: A status callback which indicates if the event successfully sent.
     */
    OLY_ASYNC_RESULT void SendNetworkEvent(
        const oly_common::String& EventName, const oly_common::Array<ReplicatedValue>& Args, CallbackHandler Callback);

    /**
     * @brief Sends a network event by EventName, to TargetClientId.
     * @param EventName oly_common::String: The identifying name for the event.
     * @param Args oly_common::Array<ReplicatedValue>: An array of arguments (ReplicatedValue) to be passed as part of the event payload.
     * @param TargetClientId uint64_t: The client ID to send the event to.
     * @param Callback CallbackHandler: A status callback which indicates if the event successfully sent.
     */
    OLY_ASYNC_RESULT void SendNetworkEventToClient(
        const oly_common::String& EventName, const oly_common::Array<ReplicatedValue>& Args, uint64_t TargetClientId, CallbackHandler Callback);

    /**
     * @brief Sets a callback for a disconnection event
     * @param Callback The callback for disconnection, contains a string with a reason for disconnection.
     */
    OLY_EVENT void SetDisconnectionCallback(DisconnectionCallbackHandler Callback);

    /**
     * @brief Sets a callback for a connection event
     * @param Callback The callback for connection, contains a string with a status of connection (Success, Failure etc).
     */
    OLY_EVENT void SetConnectionCallback(ConnectionCallbackHandler Callback);

    /**
     * @brief Sets a callback for a network interruption event.
     * Connection isn't recoverable after this point and Disconnect should be called.
     * @param Callback The callback for network interruption, contains a string showing failure.
     */
    OLY_EVENT void SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler Callback);

    /**
     * @brief Sets a callback for an asset changed event.
     * @param Callback AssetDetailBlobChangedCallbackHandler: Callback to receive data for the asset that has been changed.
     */
    OLY_EVENT void SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler Callback);

    /**
     * @brief Sets a callback for an conversation new Message event.
     * @param Callback ConversationMessageCallbackHandler: Callback to receive ConversationSystem Data when a  is sent.
     * Callback will have to reset the callback passed to the system to avoid "dangling objects" after use.
     */
    OLY_EVENT void SetConversationSystemCallback(ConversationSystemCallbackHandler Callback);

    /**
     * @brief Registers a callback to listen for the named EventName.
     * @param EventName oly_common::String: The identifying name for the event to listen for.
     * @param Callback ParameterisedCallbackHandler: A callback to register for the event which contains the parameter payload data.
     */
    void ListenNetworkEvent(const oly_common::String& EventName, ParameterisedCallbackHandler Callback);

    /**
     * @brief Stops the multiplayer connection from listening for a particular network event.
     * @param EventName oly_common::String: The name of the network event to stop listening for.
     */
    void StopListenNetworkEvent(const oly_common::String& EventName);

    /**
     * @brief Requests the ClientID.
     * @return uint64_t the ClientID for this connection.
     */
    uint64_t GetClientId() const;

    /**
     * @brief Gets a pointer to the space entity system.
     * @return A pointer to the space entity system.
     */
    SpaceEntitySystem* GetSpaceEntitySystem() const;

    /**
     * @brief Gets a pointer to the conversation system.
     * @return A pointer to the conversation system.
     */
    ConversationSystem* GetConversationSystem() const;

    /**
     * @brief Gets the current connection state.
     * @return A ConnectionState enum value.
     */
    ConnectionState GetConnectionState() const;

    /**
     * @brief Sets the Self Messaging flag for this client.
     * This allows a client to declare that it wishes to recieve every patch and object message it sends.
     * @warning Don't use this function if you aren't sure of the consequences, it's very unlikely that a client would want to use this!
     */
    OLY_ASYNC_RESULT void SetAllowSelfMessagingFlag(const bool AllowSelfMessaging, CallbackHandler Callback);

    /**
     * @brief Gets the bool representing if we're using self-messaging or not.
     */
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

    void DisconnectWithReason(const oly_common::String& Reason, CallbackHandler Callback);

    void StartEventMessageListening();

    void Cleanup();
    class oly_multiplayer::SignalRConnection* Connection;
    class oly_multiplayer::IWebSocketClient* WebSocketClient;
    class NetworkEventManagerImpl* NetworkEventManager;
    SpaceEntitySystem* SpaceEntitySystemPtr;
    ConversationSystem* ConversationSystemPtr;

    uint64_t ClientId;

    oly_common::String SpaceId;

    DisconnectionCallbackHandler DisconnectionCallback;
    ConnectionCallbackHandler ConnectionCallback;
    NetworkInterruptionCallbackHandler NetworkInterruptionCallback;
    AssetDetailBlobChangedCallbackHandler AssetDetailBlobChangedCallback;
    ConversationSystemCallbackHandler ConversationSystemCallback;

    typedef std::vector<ParameterisedCallbackHandler> Callbacks;
    std::map<oly_common::String, Callbacks> NetworkEventMap;

    std::atomic_bool Connected;
    uint32_t KeepAliveSeconds = 120;

    bool AllowSelfMessaging = false;
};

} // namespace oly_multiplayer
