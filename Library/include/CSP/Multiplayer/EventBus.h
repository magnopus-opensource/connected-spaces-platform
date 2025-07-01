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

// The callback used to register to listen to events.
// EventData lifetime is tied to the callback, do not attempt to store it via reference.
typedef std::function<void(const EventData& EventData)> NetworkEventCallback;

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
    /// @param EventReceiverId csp::common::String : The identifying name for the event receiver, used for management purposes, allowing clients to
    /// register multiple interests in single events. May be any arbitrary unique string. This is distinct from client ID. A single client/application
    /// may register multiple receivers if they choose.
    /// @param EventName csp::common::String : The identifying name for the event. May be any arbitrary string.
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
class CSP_API EventBus
{
public:
    typedef std::function<void(csp::multiplayer::ErrorCode)> ErrorCodeCallbackHandler;

    /// @brief Sends a network event by EventName to all currently connected clients.
    /// @param EventName csp::common::String : The identifying name for the event.
    /// @param Args csp::common::Array<csp::common::ReplicatedValue> : An array of arguments (csp::common::ReplicatedValue) to be passed as part of
    /// the event payload.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_ASYNC_RESULT void SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback);
    CSP_NO_EXPORT async::task<std::optional<csp::multiplayer::ErrorCode>> SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args);

    /// @brief Sends a network event by EventName, to TargetClientId.
    /// @param EventName csp::common::String : The identifying name for the event.
    /// @param Args csp::common::Array<csp::common::ReplicatedValue> : An array of arguments (csp::common::ReplicatedValue) to be passed as part of
    /// the event payload.
    /// @param TargetClientId uint64_t : The client ID to send the event to.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_ASYNC_RESULT void SendNetworkEventToClient(const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args,
        uint64_t TargetClientId, ErrorCodeCallbackHandler Callback);

    /// @brief Register interest in a network event, such that the EventBus will call the provided callback when it arrives
    /// @param Registration NetworkEventRegistration : Registration information, containing a ReceiverID and an EventName.
    /// @param Callback NetworkEventCallback : Callback to invoke when specified event is received.
    /// @return True if the registration was successful, false otherwise, such as in the case where Registration was not-unique, or the callback was
    /// null.
    bool ListenNetworkEvent(NetworkEventRegistration Registration, NetworkEventCallback Callback);

    // Here is where a nice method overload would go that lets you pass 2 strings directly as a convenience, however it can't be done at the moment
    // due to the wrapper generator (JS), and having a distinct name seems more confusing than anything.

    /// @brief Deregister interest in a network event
    /// @param Registration NetworkEventRegistration : Registration information of already registered event, containing a ReceiverID and an EventName.
    /// @return True if the deregistration was successful, false otherwise, such as in the case where Registration was not found
    bool StopListenNetworkEvent(NetworkEventRegistration Registration);

    /// @brief Deregister interest in all network events registered to a particular EventReceiverId
    /// @param EventReceiverId const csp::common::String& : EventReceiverId to deregister.
    /// @return True if the deregistration was successful, false otherwise, such as if no events were found to deregister under the provided
    /// EventReceiverId.
    bool StopListenAllNetworkEvents(const csp::common::String& EventReceiverId);

    /// @brief Get an array of all interests currently registered to the EventBus
    /// @return csp::common::Array<csp::multiplayer::NetworkEventRegistration> A container of all currently registered registrations.
    csp::common::Array<NetworkEventRegistration> AllRegistrations() const;

    /// @brief Instructs the event bus to start listening to messages
    /// @return True is successfully started listening, false if the connection is unavailable for some reason.
    bool StartEventMessageListening();

    /// @brief EventBus constructor
    /// @param InMultiplayerConnection MultiplayerConnection* : the multiplayer connection to construct the event bus with
    CSP_NO_EXPORT EventBus(MultiplayerConnection* InMultiplayerConnection, csp::common::LogSystem& LogSystem);

    /// @brief EventBus destructor
    CSP_NO_EXPORT ~EventBus();

    CSP_START_IGNORE
    /*
     * @brief Network events CSP sends over the network to facilitate its internal functionality.
     * As the EventBus provides the ability to send any event, using a string as the identifier,
     * these eventually get serialized/deserialized to go across the network
     * This is just a bit of type-safety really. In theory we could keep the strings this makes
     * manually in sync across all the systems, but this way mistakes are harder.
     * The normal case for this is GeneralPurposeEvent. Unless you need special deserialization,
     * you should probably still use that one, even internally inside CSP.
     */
    enum class NetworkEvent
    {
        AssetDetailBlobChanged, // Unpacks to AssetDetailBlobChangedEventData
        Conversation, // Unpacks to ConversationEventData
        SequenceChanged, // Unpacks to SequenceChangedEventData or SequenceHotspotChangedEventData (Better if there was a seperate event for each.)
        AccessControlChanged, // Unpacks to AccessControlChangedEventData
        GeneralPurposeEvent // Unpacks to EventData (Base type). An external event unknown to us that may have been registered with any string value.
    };

    static NetworkEvent NetworkEventFromString(const csp::common::String& EventString);
    static csp::common::String StringFromNetworkEvent(NetworkEvent Event);
    CSP_END_IGNORE

private:
    EventBus();

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
                { NetworkEvent::AccessControlChanged, "AccessControlChanged" } };

    CSP_END_IGNORE

    // Map internal event values to the deserializers needed to unpack them
    std::unique_ptr<EventData> DeserialiseForEventType(NetworkEvent EventType, const std::vector<signalr::value>& EventValues);

    std::unordered_map<NetworkEventRegistration, NetworkEventCallback> RegisteredEvents = {};
};

} // namespace csp::multiplayer
