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
    auto registrations = AllRegistrations();
    for (const NetworkEventRegistration& registration : registrations)
    {
        StopListenNetworkEvent(registration);
    }
}

NetworkEventBus::NetworkEventBus(MultiplayerConnection* inMultiplayerConnection, csp::common::LogSystem& logSystem)
    : m_logSystem(logSystem)
{
    m_multiplayerConnectionInst = inMultiplayerConnection;
}

void NetworkEventBus::ListenNetworkEvent(NetworkEventRegistration registration, NetworkEventCallback callback)
{
    if (!callback)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "Error: Expected non-null callback.");
        return;
    }

    if (m_registeredEvents.find(registration) != m_registeredEvents.end())
    {
        // We have found an event registered for this event receiver and this event type, double registration is disallowed.
        m_logSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("Attempting to register a duplicate network event receiver with EventReceiverId: {}, Event: {}. Registration denied.",
                registration.EventReceiverId, registration.EventName)
                .c_str());
        return;
    }

    m_logSystem.LogMsg(csp::common::LogLevel::Verbose,
        fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", registration.EventReceiverId, registration.EventName).c_str());
    m_registeredEvents[registration] = callback;
}

void NetworkEventBus::StopListenNetworkEvent(NetworkEventRegistration registration)
{
    if (m_registeredEvents.find(registration) == m_registeredEvents.end())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("Could not find network event registration with EventReceiverId: {}, Event: {}. Deregistration denied.",
                registration.EventReceiverId, registration.EventName)
                .c_str());
        return;
    }

    m_registeredEvents.erase(registration);
}

void NetworkEventBus::StopListenAllNetworkEvents(const csp::common::String& eventReceiverId)
{
    std::vector<NetworkEventRegistration> registrationsToRemove {};

    for (std::pair<NetworkEventRegistration, NetworkEventCallback> regAndCallback : m_registeredEvents)
    {
        if (regAndCallback.first.EventReceiverId == eventReceiverId)
        {
            registrationsToRemove.push_back(regAndCallback.first);
        }
    }

    for (const NetworkEventRegistration& regToRemove : registrationsToRemove)
    {
        StopListenNetworkEvent(regToRemove);
    }

    // Just be helpful in case the user was expecting to remove something
    if (registrationsToRemove.size() == 0)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("Could not find any network event registration with EventReceiverId: {}. No events were deregistered.", eventReceiverId)
                .c_str());
    }
}

csp::common::Array<NetworkEventRegistration> NetworkEventBus::AllRegistrations() const
{
    csp::common::Array<NetworkEventRegistration> registrations(m_registeredEvents.size());
    std::transform(m_registeredEvents.cbegin(), m_registeredEvents.cend(), registrations.begin(),
        [](const std::pair<NetworkEventRegistration, NetworkEventCallback>& regAndCallback) { return regAndCallback.first; });
    return registrations;
}

bool NetworkEventBus::StartEventMessageListening()
{
    if (m_multiplayerConnectionInst == nullptr || m_multiplayerConnectionInst->GetSignalRConnection() == nullptr)
    {
        m_logSystem.LogMsg(
            csp::common::LogLevel::Error, "Error : Multiplayer connection is unavailable, NetworkEventBus cannot start listening to events.");
        return false;
    }

    std::function<void(signalr::value)> eventDispatchCallback = [this](signalr::value result)
    {
        if (result.is_null())
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Log, "NetworkEventBus: Event message received with null data.");
            return;
        }

        if (!result.is_array() || result.as_array().empty())
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Event message expected to be a non-empty array.");
            return;
        }

        if (!result.as_array()[0].is_array())
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error,
                "NetworkEventBus: Event message expected to be an array with an array at element[0] but the element was of a different type.");
            return;
        }

        std::vector<signalr::value> eventValues = result.as_array()[0].as_array();

        if (eventValues.empty() || !eventValues[0].is_string())
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Event message missing EventType string at index 0.");
            return;
        }

        const csp::common::String eventTypeStr(eventValues[0].as_string().c_str());

        // Find all registrations that match this network event type.
        std::vector<NetworkEventRegistration> matchingRegistrations;
        for (auto const& [Registration, CB] : m_registeredEvents)
        {
            if (Registration.EventName == eventTypeStr)
            {
                matchingRegistrations.push_back(Registration);
            }
        }

        // If we have no registered event matching this string, ignore it entirely.
        if (matchingRegistrations.size() == 0)
        {
            m_logSystem.LogMsg(
                csp::common::LogLevel::Verbose, fmt::format("Received event {} has no registrations, discarding...", eventTypeStr).c_str());
            return;
        }

        // Deserialize the signalR packets using the appropriate deserialiser.
        // This only does anything for internal events, external events will always use the base EventDeserializer.
        // After this, we'll have ReplicatedValues, which serves as our common exchange type.
        // NOTE: This is not ideal, we'd rather have systems interpret this data directly. However, that would mean breaking the signalr dependency
        // in the deserialisation, which is very possible, just a bit time consuming, so we'll do it later.
        // This will be nullptr if the deserialisation fails or throws.
        std::unique_ptr<csp::common::NetworkEventData> deserialisedEventData;

        try
        {
            deserialisedEventData = DeserialiseForEventType(NetworkEventFromString(eventTypeStr), eventValues);
        }
        catch (const signalr::signalr_exception& e)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("NetworkEventBus: SignalR type mismatch encountered in Event {}: {}", eventTypeStr.c_str(), e.what()).c_str());
        }
        catch (const csp::common::ReplicatedValueException& e)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("NetworkEventBus: ReplicatedValue type mismatch: {}", e.what()).c_str());
        }
        catch (...)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Unknown error encountered during event deserialisation.");
        }
        
        if (deserialisedEventData == nullptr)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("NetworkEventBus: Failed to deserialize event '{}'. Registered events will not be fired.", eventTypeStr).c_str());
            return;
        }

        // Dispatch the events
        for (const NetworkEventRegistration& registration : matchingRegistrations)
        {
            // Pass NetworkEventData object to user ownership
            // This may be a subtype, the registrar will know what type they are expecting. External users should
            // only ever register general purpose events, and thus should only ever get an NetworkEventData, so no need to cast.
            // The user shouldn't expect the scope of this variable to live beyond the callback
            m_registeredEvents[registration](*deserialisedEventData);
        }
    };

    m_multiplayerConnectionInst->GetSignalRConnection()->On("OnEventMessage", eventDispatchCallback, m_logSystem);
    return true;
}

void NetworkEventBus::SendNetworkEvent(
    const csp::common::String& eventName, const csp::common::Array<csp::common::ReplicatedValue>& args, ErrorCodeCallbackHandler callback)
{
    SendNetworkEventToClient(eventName, args, ALL_CLIENTS_ID, callback);
}

async::task<std::optional<csp::multiplayer::ErrorCode>> NetworkEventBus::SendNetworkEvent(
    const csp::common::String& eventName, const csp::common::Array<csp::common::ReplicatedValue>& args)
{
    auto onCompleteEvent = std::make_shared<async::event_task<std::optional<csp::multiplayer::ErrorCode>>>();
    async::task<std::optional<csp::multiplayer::ErrorCode>> onCompleteTask = onCompleteEvent->get_task();

    SendNetworkEventToClient(eventName, args, ALL_CLIENTS_ID,
        [onCompleteEvent](ErrorCode code)
        {
            if (code != ErrorCode::None)
            {
                onCompleteEvent->set(code);
            }
            else
            {
                onCompleteEvent->set(std::nullopt);
            }
        });

    return onCompleteTask;
}

void NetworkEventBus::SendNetworkEventToClient(const csp::common::String& eventName, const csp::common::Array<csp::common::ReplicatedValue>& args,
    uint64_t targetClientId, ErrorCodeCallbackHandler callback)
{
    m_multiplayerConnectionInst->GetNetworkEventManager()->SendNetworkEvent(eventName, args, targetClientId, callback);
}

csp::common::String NetworkEventBus::StringFromNetworkEvent(NetworkEvent event)
{
    auto it = CustomDeserializationEventMap.find(event);
    if (it != CustomDeserializationEventMap.end())
        return it->second;
    throw std::invalid_argument(
        fmt::format("StringFromInternalNetworkEvent: unknown enum value {}", static_cast<std::underlying_type_t<NetworkEvent>>(event)));
}

NetworkEventBus::NetworkEvent NetworkEventBus::NetworkEventFromString(const csp::common::String& eventString)
{
    for (const auto& [Key, Val] : CustomDeserializationEventMap)
    {
        if (Val == eventString)
            return Key;
    }

    // If we don't recognise the event, it must be a general purpose event
    return NetworkEvent::GeneralPurposeEvent;
}

std::unique_ptr<csp::common::NetworkEventData> NetworkEventBus::DeserialiseForEventType(
    NetworkEvent eventType, const std::vector<signalr::value>& eventValues)
{
    using namespace csp::common;

    switch (eventType)
    {
    case NetworkEvent::AssetDetailBlobChanged:
        return std::make_unique<AssetDetailBlobChangedNetworkEventData>(
            csp::multiplayer::DeserializeAssetDetailBlobChangedEvent(eventValues, m_logSystem));
    case NetworkEvent::Conversation:
        return std::make_unique<ConversationNetworkEventData>(csp::multiplayer::DeserializeConversationEvent(eventValues, m_logSystem));
    case NetworkEvent::SequenceChanged:
    {
        std::unique_ptr<SequenceChangedNetworkEventData> sequenceEventData
            = std::make_unique<SequenceChangedNetworkEventData>(csp::multiplayer::DeserializeSequenceChangedEvent(eventValues, m_logSystem));

        const csp::common::String key = sequenceEventData->Key;
        const csp::common::String sequenceType = csp::multiplayer::GetSequenceKeyIndex(key, 0);

        if (sequenceType == "Hotspots")
        {
            sequenceEventData->SequenceType = ESequenceType::Hotspot;

            // If it is a hotspot, the 'key' will contain the following information [SequenceType]:[SpaceId]:[SequenceName]
            // eg: Hotspots:abc123456:My-Hotspot-Sequence
            String oldHotspotSequenceName = csp::multiplayer::GetSequenceKeyIndex(sequenceEventData->Key, 2);
            // NewKey will be structured in the same way, eg: Hotspots:abc123456:My-New-Hotspot-Sequence
            String newHotspotSequenceName = csp::multiplayer::GetSequenceKeyIndex(sequenceEventData->NewKey, 2);

            sequenceEventData->SpaceId = csp::multiplayer::GetSequenceKeyIndex(sequenceEventData->Key, 1);
            sequenceEventData->Key = oldHotspotSequenceName;
            sequenceEventData->NewKey = newHotspotSequenceName;
        }

        return sequenceEventData;
    }
    case NetworkEvent::AccessControlChanged:
        return std::make_unique<AccessControlChangedNetworkEventData>(
            csp::multiplayer::DeserializeAccessControlChangedEvent(eventValues, m_logSystem));
    case NetworkEvent::GeneralPurposeEvent:
        return std::make_unique<NetworkEventData>(csp::multiplayer::DeserializeGeneralPurposeEvent(eventValues, m_logSystem));
    case NetworkEvent::AsyncCallCompleted:
        return std::make_unique<AsyncCallCompletedEventData>(csp::multiplayer::DeserializeAsyncCallCompletedEvent(eventValues, m_logSystem));
    default:
        throw std::invalid_argument(
            fmt::format("DeserialiseForEventType: unknown enum value {}", static_cast<std::underlying_type_t<NetworkEvent>>(eventType)));
    }
}
} // namespace csp::multiplayer
