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

#include "CSP/Common/String.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

namespace csp::common
{
class LogSystem;
class ReplicatedValue;
}

namespace signalr
{
class value;
}

namespace csp::systems
{

class SpaceSystem;
class SystemsManager;
class SystemBase;

} // namespace csp::systems

namespace async
{
CSP_START_IGNORE
template <typename T> class event_task;
template <typename T> class task;
CSP_END_IGNORE
}

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{
class EventDeserialiser;
enum class ErrorCode;

// The callback used to register to listen to custom network events.
// NetworkEventData lifetime is tied to the callback, do not attempt to store it via reference.
typedef std::function<void(const csp::common::NetworkEventData& NetworkEventData)> NetworkEventCallback;

// Event listener specializations
// Callbacks used to register to listen to event specializations.
// The lifetime of network event data types are tied to the callback, do not attempt to store it via reference.
typedef std::function<void(const csp::common::AccessControlChangedNetworkEventData& AccessControlChangedEventData)> AccessControlChangedEventCallback;
typedef std::function<void(const csp::common::AssetDetailBlobChangedNetworkEventData& AssetDetailBlobChangedEventData)>
    AssetDetailBlobChangedEventCallback;
typedef std::function<void(const csp::common::AsyncCallCompletedEventData& AsyncCallCompletedEventData)> AsyncCallCompletedEventCallback;
typedef std::function<void(const csp::common::ConversationNetworkEventData& ConversationNetworkEventData)> ConversationEventCallback;
typedef std::function<void(const csp::common::SequenceChangedNetworkEventData& SequenceChangedEventData)> SequenceChangedEventCallback;

/*
 * @brief Details about a network event registrations to serve as a key in the event map.
 * Consists of the event name, as well as a EventReceiverId.
 * The EventReceiverId is arbitrary, and serves to identify the object that registered this callback.
 * As no registrar can register to the same event twice, equality of the EventReceiverId drives removal and replacement of callbacks when calling
 * registration functionality.
 */
class CSP_API NetworkEventRegistration
{
public:
    /// @brief Construct a NetworkEventRegistration
    /// @param EventReceiverId : The identifying name for the event receiver, used for management purposes, allowing clients to
    /// register multiple interests in single events. May be any arbitrary unique string. This is distinct from client ID. A single client/application
    /// may register multiple receivers if they choose.
    /// @param EventName : The identifying name for the event. May be any arbitrary string.
    NetworkEventRegistration(const csp::common::String& EventReceiverId, const csp::common::String& EventName)
        : EventReceiverId(EventReceiverId)
        , EventName(EventName)
    {
    }

    csp::common::String EventReceiverId;
    csp::common::String EventName;

    bool operator==(NetworkEventRegistration const& other) const { return EventReceiverId == other.EventReceiverId && EventName == other.EventName; }
    bool operator!=(NetworkEventRegistration const& other) const { return !(*this == other); }

    // For wrapper gen ... but also our interop container types expect to be able to default construct which is a bit troublesome.
    NetworkEventRegistration()
        : EventReceiverId("")
        , EventName("")
    {
    }
};
}

CSP_START_IGNORE
// Make NetworkEventRegistration hashable so it can be used in unordered containers
// If we start to do a lot of these, consider moving them to a custom_hashes.h file or something similar.
namespace std
{
template <> struct hash<csp::multiplayer::NetworkEventRegistration>
{
    size_t operator()(csp::multiplayer::NetworkEventRegistration const& key) const noexcept
    {
        // combine two string‐hashes with boost-style mix
        size_t h1 = hash<std::string> {}(key.EventReceiverId.c_str());
        size_t h2 = hash<std::string> {}(key.EventName.c_str());

        // Weak hash, but we don't really care. Look at boost hash_combine if you want to do this better.
        return h1 ^ h2;
    }
};
}
CSP_END_IGNORE

namespace csp::multiplayer
{

/// @ingroup Multiplayer
/// @brief Handles registration of interest, and dispatch of callbacks to interested parties, of events sent over the network to connected clients.
/// This object may be used to send arbitrary messages between clients, broadcasting messages to either all clients, or particular clients specified
/// by a clientID. Particular messages are generic and may be defined as any arbitrary string, and may carry payloads of csp::common::ReplicatedValue.
class CSP_API NetworkEventBus
{
public:
    typedef std::function<void(csp::multiplayer::ErrorCode)> ErrorCodeCallbackHandler;

    /// @brief Sends a network event by EventName to all currently connected clients.
    /// @param EventName : The identifying name for the event.
    /// @param Args : An array of arguments (csp::common::ReplicatedValue) to be passed as part of
    /// the event payload.
    /// @param Callback : a callback with failure state.
    CSP_ASYNC_RESULT void SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback);
    CSP_NO_EXPORT async::task<std::optional<csp::multiplayer::ErrorCode>> SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args);

    /// @brief Sends a network event by EventName, to TargetClientId.
    /// @param EventName : The identifying name for the event.
    /// @param Args : An array of arguments (csp::common::ReplicatedValue) to be passed as part of
    /// the event payload.
    /// @param TargetClientId : The client ID to send the event to.
    /// @param Callback : a callback with failure state.
    CSP_ASYNC_RESULT void SendNetworkEventToClient(const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args,
        uint64_t TargetClientId, ErrorCodeCallbackHandler Callback);

    /// @brief Register interest in a custom network event, such that the NetworkEventBus will call the provided callback when it arrives.
    /// @note This is for registration of custom events. To register for an event specializations, please use one of the dedicated
    /// listen methods. Registration will fail if a callback has already been registered with the same ReceiverId and EventName.
    /// @param ReceiverId : The identifying name for the event receiver, used for management purposes. Allows clients to register multiple receivers
    /// for the same event if they choose.
    /// @param EventName : The identifying name for the event. May be any arbitrary string.
    /// @param Callback : Callback to invoke when the custom event is received. Will fail to register if the callback is null.
    void ListenCustomNetworkEvent(csp::common::String EventReceiverId, csp::common::String EventName, NetworkEventCallback Callback);

    /// @brief Register interest in a access control changed network event, such that the NetworkEventBus will call the provided callback when it
    /// arrives.
    /// @note Registration will fail if a callback has already been registered with the same ReceiverId.
    /// @param EventReceiverId : The identifying name for the event receiver, used for management purposes. Allows clients to register multiple
    /// receivers for the same event if they choose.
    /// @param Callback : Callback to invoke when a access control changed event is received. Will fail to register if the callback is null.
    void ListenAccessControlChangedEvent(csp::common::String EventReceiverId, AccessControlChangedEventCallback Callback);

    /// @brief Register interest in a asset detail blob changed network event, such that the NetworkEventBus will call the provided
    /// callback when it arrives.
    /// @note Registration will fail if a callback has already been registered with the same ReceiverId.
    /// @param EventReceiverId : The identifying name for the event receiver, used for management purposes. Allows clients to register multiple
    /// receivers for the same event if they choose.
    /// @param Callback : Callback to invoke when a asset detail blob changed event is received. Will fail to register if the callback is null.
    void ListenAssetDetailBlobChangedEvent(csp::common::String EventReceiverId, AssetDetailBlobChangedEventCallback Callback);

    /// @brief Register interest in a async call completed network event for a specific operation, such that the NetworkEventBus will call the provided
    /// callback when it arrives.
    /// @note Registration will fail if a callback has already been registered with the same ReceiverId and OperationName.
    /// @param EventReceiverId : The identifying name for the event receiver, used for management purposes. Allows clients to register multiple
    /// receivers for the same event if they choose.
    /// @param OperationName : The name of the async call completed operation, eg: 'DuplicateSpaceAsync' or 'CreateSnapshotAsync'.
    /// @param Callback : Callback to invoke when a async call completed event is received with a specific operation name. Will fail to register if
    /// the callback is null.
    void ListenAsyncCallCompletedEvent(
        csp::common::String EventReceiverId, csp::common::String OperationName, AsyncCallCompletedEventCallback Callback);

    /// @brief Register interest in a conversation network event, such that the NetworkEventBus will call the provided callback when it arrives.
    /// @note Registration will fail if a callback has already been registered with the same ReceiverId.
    /// @param EventReceiverId : The identifying name for the event receiver, used for management purposes. Allows clients to register multiple
    /// receivers for the same event if they choose.
    /// @param Callback : Callback to invoke when a conversation event is received. Will fail to register if the callback is null.
    void ListenConversationEvent(csp::common::String EventReceiverId, ConversationEventCallback Callback);

    /// @brief Register interest in a sequence changed network event, such that the NetworkEventBus will call the provided callback when it arrives.
    /// @note Registration will fail if a callback has already been registered with the same ReceiverId.
    /// @param EventReceiverId : The identifying name for the event receiver, used for management purposes. Allows clients to register multiple
    /// receivers for the same event if they choose.
    /// @param Callback : Callback to invoke when a sequence changed event is received. Will fail to register if the callback is null.
    void ListenSequenceChangedEvent(csp::common::String EventReceiverId, SequenceChangedEventCallback Callback);

    /// @brief Deregister interest in a custom network event
    /// @param ReceiverId : The identifying name for the event receiver.
    /// @param EventName : The identifying name for the event.
    void StopListenCustomNetworkEvent(csp::common::String EventReceiverId, csp::common::String EventName);

    /// @brief Deregister interest in a access control changed network event
    /// @param ReceiverId : The identifying name for the event receiver.
    void StopListenAccessControlChangedEvent(csp::common::String EventReceiverId);

    /// @brief Deregister interest in a asset detail blob changed network event
    /// @param ReceiverId : The identifying name for the event receiver.
    void StopListenAssetDetailBlobChangedEvent(csp::common::String EventReceiverId);

    /// @brief Deregister interest in a async call completed network event
    /// @param ReceiverId : The identifying name for the event receiver.
    void StopListenAsyncCallCompletedEvent(csp::common::String EventReceiverId, csp::common::String OperationName);

    /// @brief Deregister interest in a conversation network event
    /// @param ReceiverId : The identifying name for the event receiver.
    void StopListenConversationEvent(csp::common::String EventReceiverId);

    /// @brief Deregister interest in a sequence changed network event
    /// @param ReceiverId : The identifying name for the event receiver.
    void StopListenSequenceChangedEvent(csp::common::String EventReceiverId);

    /// @brief Deregister interest in all network events registered to a particular EventReceiverId
    /// @param EventReceiverId : EventReceiverId to deregister.
    void StopListenAllNetworkEvents(const csp::common::String& EventReceiverId);

    /// @brief Get an array of all network event registrations currently registered with the NetworkEventBus
    /// @return An array of all current network event registrations.
    csp::common::Array<NetworkEventRegistration> AllRegistrations() const;

    /// @brief Instructs the event bus to start listening to messages
    /// @return True is successfully started listening, false if the connection is unavailable for some reason.
    bool StartEventMessageListening();

    /// @brief NetworkEventBus constructor
    /// @param InMultiplayerConnection : the multiplayer connection to construct the event bus with
    /// @param LogSystem : Logger such that this system can print status and debug output
    CSP_NO_EXPORT NetworkEventBus(MultiplayerConnection* InMultiplayerConnection, csp::common::LogSystem& LogSystem);

    /// @brief NetworkEventBus destructor
    CSP_NO_EXPORT ~NetworkEventBus();

    CSP_START_IGNORE
    /*
     * @brief Network events CSP sends over the network to facilitate its internal functionality.
     * As the NetworkEventBus provides the ability to send any event, using a string as the identifier,
     * these eventually get serialized/deserialized to go across the network
     * This is just a bit of type-safety really. In theory we could keep the strings this makes
     * manually in sync across all the systems, but this way mistakes are harder.
     * The normal case for this is GeneralPurposeEvent. Unless you need special deserialization,
     * you should probably still use that one, even internally inside CSP.
     */
    enum class NetworkEvent
    {
        AssetDetailBlobChanged, // Unpacks to AssetDetailBlobChangedNetworkEventData
        Conversation, // Unpacks to ConversationNetworkEventData
        SequenceChanged, // Unpacks to SequenceChangedNetworkEventData or SequenceHotspotChangedEventData (Better if there was a seperate event for
                         // each.)
        AccessControlChanged, // Unpacks to AccessControlChangedNetworkEventData
        GeneralPurposeEvent, // Unpacks to NetworkEventData (Base type). An external event unknown to us that may have been registered with any string
                             // value.
        AsyncCallCompleted // Unpacks to AsyncCallCompletedEventData
    };

    static NetworkEvent NetworkEventFromString(const csp::common::String& EventString);
    static csp::common::String StringFromNetworkEvent(NetworkEvent Event);
    CSP_END_IGNORE

private:
    NetworkEventBus();

    class MultiplayerConnection* MultiplayerConnectionInst;
    csp::common::LogSystem& LogSystem;

    // Map type-safe enum values to strings that can go across the network
    // The specific spelling of these events is important, they are part of the backend event contract.
    // These are events that require custom deserialization, and are the special case. General purpose events with generic deserialization are the
    // normal case, (ie, any event name that doesn't exist in this map).
    CSP_START_IGNORE // Damn you wrapper generator! Causes a hang otherwise

        // Use c++17 inline variables to allow static access to this header defined map ... neat!
        static inline const std::unordered_map<NetworkEvent, csp::common::String>
            CustomDeserializationEventMap { { NetworkEvent::AssetDetailBlobChanged, "AssetDetailBlobChanged" },
                { NetworkEvent::Conversation, "Conversation" }, { NetworkEvent::SequenceChanged, "SequenceChanged" },
                { NetworkEvent::AccessControlChanged, "AccessControlChanged" }, { NetworkEvent::AsyncCallCompleted, "AsyncCallCompleted" } };

    CSP_END_IGNORE

    // General purpose event registration.
    std::unordered_map<NetworkEventRegistration, NetworkEventCallback> RegisteredEvents = {};
    // Registrations for event listener specializations.
    std::unordered_map<NetworkEventRegistration, AccessControlChangedEventCallback> RegisteredAccessControlChangedEvents = {};
    std::unordered_map<NetworkEventRegistration, AssetDetailBlobChangedEventCallback> RegisteredAssetDetailBlobChangedEvents = {};
    std::unordered_map<NetworkEventRegistration, AsyncCallCompletedEventCallback> RegisteredAsyncCallCompletedEvents = {};
    std::unordered_map<NetworkEventRegistration, ConversationEventCallback> RegisteredConversationEvents = {};
    std::unordered_map<NetworkEventRegistration, SequenceChangedEventCallback> RegisteredSequenceChangedEvents = {};
};

} // namespace csp::multiplayer
