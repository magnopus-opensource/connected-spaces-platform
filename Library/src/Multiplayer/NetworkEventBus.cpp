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
#include "CSP/Multiplayer/NetworkEventBus.h"

#include "CSP/Common/ReplicatedValueException.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Systems/SystemBase.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"

#include "CSP/Common/CSPAsyncScheduler.h"
#include "signalrclient/signalr_exception.h"
#include <algorithm>
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <optional>

namespace csp::multiplayer
{

constexpr const uint64_t ALL_CLIENTS_ID = std::numeric_limits<uint64_t>::max();

NetworkEventBus::~NetworkEventBus()
{
    // Clean up all registered listeners.
    // The NetworkEventBus is owned by the MultiplayerConnection which is one of the last systems to be destroyed by the Systems Manager.
    auto Registrations = AllRegistrations();
    for (const NetworkEventRegistration& Registration : Registrations)
    {
        StopListenNetworkEvent(Registration);
    }
}

NetworkEventBus::NetworkEventBus(MultiplayerConnection* InMultiplayerConnection, csp::common::LogSystem& LogSystem)
    : LogSystem(LogSystem)
{
    MultiplayerConnectionInst = InMultiplayerConnection;
}

void NetworkEventBus::ListenNetworkEvent(NetworkEventRegistration Registration, NetworkEventCallback Callback)
{
    if (!Callback)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Error: Expected non-null callback.");
        return;
    }

    if (RegisteredEvents.find(Registration) != RegisteredEvents.end())
    {
        // We have found an event registered for this event receiver and this event type, double registration is disallowed.
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("Attempting to register a duplicate network event receiver with EventReceiverId: {}, Event: {}. Registration denied.",
                Registration.EventReceiverId, Registration.EventName)
                .c_str());
        return;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Verbose,
        fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", Registration.EventReceiverId, Registration.EventName).c_str());
    RegisteredEvents[Registration] = Callback;
}

void NetworkEventBus::StopListenNetworkEvent(NetworkEventRegistration Registration)
{
    if (RegisteredEvents.find(Registration) == RegisteredEvents.end())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("Could not find network event registration with EventReceiverId: {}, Event: {}. Deregistration denied.",
                Registration.EventReceiverId, Registration.EventName)
                .c_str());
        return;
    }

    RegisteredEvents.erase(Registration);
}

void NetworkEventBus::StopListenAllNetworkEvents(const csp::common::String& EventReceiverId)
{
    std::vector<NetworkEventRegistration> RegistrationsToRemove {};

    for (std::pair<NetworkEventRegistration, NetworkEventCallback> RegAndCallback : RegisteredEvents)
    {
        if (RegAndCallback.first.EventReceiverId == EventReceiverId)
        {
            RegistrationsToRemove.push_back(RegAndCallback.first);
        }
    }

    for (const NetworkEventRegistration& RegToRemove : RegistrationsToRemove)
    {
        StopListenNetworkEvent(RegToRemove);
    }

    // Just be helpful in case the user was expecting to remove something
    if (RegistrationsToRemove.size() == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("Could not find any network event registration with EventReceiverId: {}. No events were deregistered.", EventReceiverId)
                .c_str());
    }
}

csp::common::Array<NetworkEventRegistration> NetworkEventBus::AllRegistrations() const
{
    csp::common::Array<NetworkEventRegistration> Registrations(RegisteredEvents.size());
    std::transform(RegisteredEvents.cbegin(), RegisteredEvents.cend(), Registrations.begin(),
        [](const std::pair<NetworkEventRegistration, NetworkEventCallback>& RegAndCallback) { return RegAndCallback.first; });
    return Registrations;
}

bool NetworkEventBus::StartEventMessageListening()
{
    if (MultiplayerConnectionInst == nullptr || MultiplayerConnectionInst->GetSignalRConnection() == nullptr)
    {
        LogSystem.LogMsg(
            csp::common::LogLevel::Error, "Error : Multiplayer connection is unavailable, NetworkEventBus cannot start listening to events.");
        return false;
    }

    std::function<void(signalr::value)> EventDispatchCallback = [this](signalr::value Result)
    {
        if (Result.is_null())
        {
            LogSystem.LogMsg(csp::common::LogLevel::Log, "NetworkEventBus: Event message received with null data.");
            return;
        }

        if (!Result.is_array() || Result.as_array().empty())
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Event message expected to be a non-empty array.");
            return;
        }

        if (!Result.as_array()[0].is_array())
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error,
                "NetworkEventBus: Event message expected to be an array with an array at element[0] but the element was of a different type.");
            return;
        }

        std::vector<signalr::value> EventValues = Result.as_array()[0].as_array();

        if (EventValues.empty() || !EventValues[0].is_string())
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Event message missing EventType string at index 0.");
            return;
        }

        const csp::common::String EventTypeStr(EventValues[0].as_string().c_str());

        // Find all registrations that match this network event type.
        std::vector<NetworkEventRegistration> MatchingRegistrations;
        for (auto const& [Registration, CB] : RegisteredEvents)
        {
            if (Registration.EventName == EventTypeStr)
            {
                MatchingRegistrations.push_back(Registration);
            }
        }

        // If we have no registered event matching this string, ignore it entirely.
        if (MatchingRegistrations.size() == 0)
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Verbose, fmt::format("Received event {} has no registrations, discarding...", EventTypeStr).c_str());
            return;
        }

        // Deserialize the signalR packets using the appropriate deserialiser.
        // This only does anything for internal events, external events will always use the base EventDeserializer.
        // After this, we'll have ReplicatedValues, which serves as our common exchange type.
        // NOTE: This is not ideal, we'd rather have systems interpret this data directly. However, that would mean breaking the signalr dependency
        // in the deserialisation, which is very possible, just a bit time consuming, so we'll do it later.
        // This will be nullptr if the deserialisation fails or throws.
        std::unique_ptr<csp::common::NetworkEventData> DeserialisedEventData;

        try
        {
            DeserialisedEventData = DeserialiseForEventType(NetworkEventFromString(EventTypeStr), EventValues);
        }
        catch (const signalr::signalr_exception& e)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("NetworkEventBus: SignalR type mismatch encountered in Event {}: {}", EventTypeStr.c_str(), e.what()).c_str());
        }
        catch (const csp::common::ReplicatedValueException& e)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("NetworkEventBus: ReplicatedValue type mismatch: {}", e.what()).c_str());
        }
        catch (...)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Unknown error encountered during event deserialisation.");
        }
        
        if (DeserialisedEventData == nullptr)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("NetworkEventBus: Failed to deserialize event '{}'. Registered events will not be fired.", EventTypeStr).c_str());
            return;
        }

        // Dispatch the events
        for (const NetworkEventRegistration& Registration : MatchingRegistrations)
        {
            // Pass NetworkEventData object to user ownership
            // This may be a subtype, the registrar will know what type they are expecting. External users should
            // only ever register general purpose events, and thus should only ever get an NetworkEventData, so no need to cast.
            // The user shouldn't expect the scope of this variable to live beyond the callback
            RegisteredEvents[Registration](*DeserialisedEventData);
        }
    };

    MultiplayerConnectionInst->GetSignalRConnection()->On("OnEventMessage", EventDispatchCallback, LogSystem);
    return true;
}

void NetworkEventBus::SendNetworkEvent(
    const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback)
{
    SendNetworkEventToClient(EventName, Args, ALL_CLIENTS_ID, Callback);
}

async::task<std::optional<csp::multiplayer::ErrorCode>> NetworkEventBus::SendNetworkEvent(
    const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args)
{
    auto OnCompleteEvent = std::make_shared<async::event_task<std::optional<csp::multiplayer::ErrorCode>>>();
    async::task<std::optional<csp::multiplayer::ErrorCode>> OnCompleteTask = OnCompleteEvent->get_task();

    SendNetworkEventToClient(EventName, Args, ALL_CLIENTS_ID,
        [OnCompleteEvent](ErrorCode Code)
        {
            if (Code != ErrorCode::None)
            {
                OnCompleteEvent->set(Code);
            }
            else
            {
                OnCompleteEvent->set(std::nullopt);
            }
        });

    return OnCompleteTask;
}

void NetworkEventBus::SendNetworkEventToClient(const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args,
    uint64_t TargetClientId, ErrorCodeCallbackHandler Callback)
{
    MultiplayerConnectionInst->GetNetworkEventManager()->SendNetworkEvent(EventName, Args, TargetClientId, Callback);
}

csp::common::String NetworkEventBus::StringFromNetworkEvent(NetworkEvent Event)
{
    auto it = CustomDeserializationEventMap.find(Event);
    if (it != CustomDeserializationEventMap.end())
        return it->second;
    throw std::invalid_argument(
        fmt::format("StringFromInternalNetworkEvent: unknown enum value {}", static_cast<std::underlying_type_t<NetworkEvent>>(Event)));
}

NetworkEventBus::NetworkEvent NetworkEventBus::NetworkEventFromString(const csp::common::String& EventString)
{
    for (const auto& [Key, Val] : CustomDeserializationEventMap)
    {
        if (Val == EventString)
            return Key;
    }

    // If we don't recognise the event, it must be a general purpose event
    return NetworkEvent::GeneralPurposeEvent;
}

std::unique_ptr<csp::common::NetworkEventData> NetworkEventBus::DeserialiseForEventType(
    NetworkEvent EventType, const std::vector<signalr::value>& EventValues)
{
    using namespace csp::common;

    switch (EventType)
    {
    case NetworkEvent::AssetDetailBlobChanged:
        return std::make_unique<AssetDetailBlobChangedNetworkEventData>(
            csp::multiplayer::DeserializeAssetDetailBlobChangedEvent(EventValues, LogSystem));
    case NetworkEvent::Conversation:
        return std::make_unique<ConversationNetworkEventData>(csp::multiplayer::DeserializeConversationEvent(EventValues, LogSystem));
    case NetworkEvent::SequenceChanged:
    {
        std::unique_ptr<SequenceChangedNetworkEventData> SequenceEventData
            = std::make_unique<SequenceChangedNetworkEventData>(csp::multiplayer::DeserializeSequenceChangedEvent(EventValues, LogSystem));

        const csp::common::String Key = SequenceEventData->Key;
        const csp::common::String SequenceType = csp::multiplayer::GetSequenceKeyIndex(Key, 0);

        if (SequenceType == "Hotspots")
        {
            SequenceEventData->SequenceType = ESequenceType::Hotspot;

            // If it is a hotspot, the 'key' will contain the following information [SequenceType]:[SpaceId]:[SequenceName]
            // eg: Hotspots:abc123456:My-Hotspot-Sequence
            String OldHotspotSequenceName = csp::multiplayer::GetSequenceKeyIndex(SequenceEventData->Key, 2);
            // NewKey will be structured in the same way, eg: Hotspots:abc123456:My-New-Hotspot-Sequence
            String NewHotspotSequenceName = csp::multiplayer::GetSequenceKeyIndex(SequenceEventData->NewKey, 2);

            SequenceEventData->SpaceId = csp::multiplayer::GetSequenceKeyIndex(SequenceEventData->Key, 1);
            SequenceEventData->Key = OldHotspotSequenceName;
            SequenceEventData->NewKey = NewHotspotSequenceName;
        }

        return SequenceEventData;
    }
    case NetworkEvent::AccessControlChanged:
        return std::make_unique<AccessControlChangedNetworkEventData>(
            csp::multiplayer::DeserializeAccessControlChangedEvent(EventValues, LogSystem));
    case NetworkEvent::GeneralPurposeEvent:
        return std::make_unique<NetworkEventData>(csp::multiplayer::DeserializeGeneralPurposeEvent(EventValues, LogSystem));
    case NetworkEvent::AsyncCallCompleted:
        return std::make_unique<AsyncCallCompletedEventData>(csp::multiplayer::DeserializeAsyncCallCompletedEvent(EventValues, LogSystem));
    default:
        throw std::invalid_argument(
            fmt::format("DeserialiseForEventType: unknown enum value {}", static_cast<std::underlying_type_t<NetworkEvent>>(EventType)));
    }
}
} // namespace csp::multiplayer
