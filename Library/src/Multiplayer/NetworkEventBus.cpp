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

namespace
{

template <typename EventContainer, typename EventCallback>
void RegisterEventCallback(csp::common::LogSystem& LogSystem, const csp::common::String& EventReceiverId, csp::common::String EventName,
    EventContainer& RegisteredEvents, const EventCallback& Callback)
{
    if (EventName.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NetworkEventBus: Expected non-empty name for event registration. Registration denied.");

        return;
    }

    if (EventReceiverId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("NetworkEventBus: Expected non-empty EventReceiverId for event {}. Registration denied.", EventName).c_str());

        return;
    }

    if (!Callback)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("NetworkEventBus: Expected non-null callback for event {} with EventReceiverId: {}. Registration denied.", EventName, EventReceiverId).c_str());

        return;
    }

    csp::multiplayer::NetworkEventRegistration Registration(EventReceiverId, EventName);

    if (RegisteredEvents.find(Registration) != RegisteredEvents.end())
    {
        // An event with matching ReceiverId and EventName has already been registered, double registration is not allowed.
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("NetworkEventBus: Attempting to register a duplicate {} network event "
                        "with EventReceiverId: {}. Registration denied.",
                EventName, EventReceiverId)
                .c_str());

        return;
    }

    LogSystem.LogMsg(
        csp::common::LogLevel::Verbose, fmt::format("Registering {} network event. EventReceiverId: {}.", EventName, EventReceiverId).c_str());

    RegisteredEvents[Registration] = Callback;
}

template <typename EventData, typename DeserializeFunc>
bool TryDeserializeEventData(
    csp::common::LogSystem& LogSystem, const csp::common::String& EventTypeStr, DeserializeFunc&& Deserialize, EventData& OutEventData)
{
    try
    {
        OutEventData = Deserialize();
        return true;
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

    LogSystem.LogMsg(csp::common::LogLevel::Error,
        fmt::format("NetworkEventBus: Failed to deserialize event '{}'. Registered events will not be fired.", EventTypeStr).c_str());

    return false;
}

} // namespace

namespace csp::multiplayer
{

constexpr const uint64_t ALL_CLIENTS_ID = std::numeric_limits<uint64_t>::max();

NetworkEventBus::~NetworkEventBus()
{
    // The NetworkEventBus is owned by the MultiplayerConnection which is one of the last systems to be destroyed by the Systems Manager.
    // Clean up all registered listeners.
    RegisteredCustomEvents.clear();
    RegisteredAccessControlChangedEvents.clear();
    RegisteredAssetDetailBlobChangedEvents.clear();
    RegisteredAsyncCallCompletedEvents.clear();
    RegisteredConversationEvents.clear();
    RegisteredSequenceChangedEvents.clear();
}

NetworkEventBus::NetworkEventBus(MultiplayerConnection* InMultiplayerConnection, csp::common::LogSystem& LogSystem)
    : LogSystem(LogSystem)
{
    MultiplayerConnectionInst = InMultiplayerConnection;
}

void NetworkEventBus::ListenCustomNetworkEvent(
    csp::common::String EventReceiverId, csp::common::String EventName, CustomNetworkEventCallback Callback)
{
    RegisterEventCallback(LogSystem, EventReceiverId, EventName, RegisteredCustomEvents, Callback);
}

void NetworkEventBus::ListenAccessControlChangedEvent(csp::common::String EventReceiverId, AccessControlChangedEventCallback Callback)
{
    RegisterEventCallback(
        LogSystem, EventReceiverId, StringFromNetworkEvent(NetworkEvent::AccessControlChanged), RegisteredAccessControlChangedEvents, Callback);
}

void NetworkEventBus::ListenAssetDetailBlobChangedEvent(csp::common::String EventReceiverId, AssetDetailBlobChangedEventCallback Callback)
{
    RegisterEventCallback(
        LogSystem, EventReceiverId, StringFromNetworkEvent(NetworkEvent::AssetDetailBlobChanged), RegisteredAssetDetailBlobChangedEvents, Callback);
}

void NetworkEventBus::ListenAsyncCallCompletedEvent(
    csp::common::String EventReceiverId, csp::common::String OperationName, AsyncCallCompletedEventCallback Callback)
{
    RegisterEventCallback(LogSystem, EventReceiverId, OperationName, RegisteredAsyncCallCompletedEvents, Callback);
}

void NetworkEventBus::ListenConversationEvent(csp::common::String EventReceiverId, ConversationEventCallback Callback)
{
    RegisterEventCallback(LogSystem, EventReceiverId, StringFromNetworkEvent(NetworkEvent::Conversation), RegisteredConversationEvents, Callback);
}

void NetworkEventBus::ListenSequenceChangedEvent(csp::common::String EventReceiverId, SequenceChangedEventCallback Callback)
{
    RegisterEventCallback(
        LogSystem, EventReceiverId, StringFromNetworkEvent(NetworkEvent::SequenceChanged), RegisteredSequenceChangedEvents, Callback);
}

void NetworkEventBus::StopListenCustomNetworkEvent(csp::common::String EventReceiverId, csp::common::String EventName)
{
    NetworkEventRegistration Registration(EventReceiverId, EventName);

    size_t RemovedCount = RegisteredCustomEvents.erase(Registration);

    if (RemovedCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("NetworkEventBus::StopListenCustomNetworkEvent: Could not find custom network event registration with Event ReceiverId: {}, "
                        "Event: {}. Deregistration denied.",
                EventReceiverId, EventName)
                .c_str());
    }
}

void NetworkEventBus::StopListenAccessControlChangedEvent(csp::common::String EventReceiverId)
{
    NetworkEventRegistration Registration(EventReceiverId, StringFromNetworkEvent(NetworkEvent::AccessControlChanged));

    size_t RemovedCount = RegisteredAccessControlChangedEvents.erase(Registration);

    if (RemovedCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("NetworkEventBus::StopListenAccessControlChangedEvent: Could not find access control changed network event registration with "
                        "Event ReceiverId: {}. "
                        "Deregistration denied.",
                EventReceiverId)
                .c_str());
    }
}

void NetworkEventBus::StopListenAssetDetailBlobChangedEvent(csp::common::String EventReceiverId)
{
    NetworkEventRegistration Registration(EventReceiverId, StringFromNetworkEvent(NetworkEvent::AssetDetailBlobChanged));

    size_t RemovedCount = RegisteredAssetDetailBlobChangedEvents.erase(Registration);

    if (RemovedCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("NetworkEventBus::StopListenAssetDetailBlobChangedEvent: Could not find asset detail blob changed network event registration "
                        "with Event "
                        "ReceiverId: {}. "
                        "Deregistration denied.",
                EventReceiverId)
                .c_str());
    }
}

void NetworkEventBus::StopListenAsyncCallCompletedEvent(csp::common::String EventReceiverId, csp::common::String OperationName)
{
    NetworkEventRegistration Registration(EventReceiverId, OperationName);

    size_t RemovedCount = RegisteredAsyncCallCompletedEvents.erase(Registration);

    if (RemovedCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("NetworkEventBus::StopListenAsyncCallCompletedEvent: Could not find async call completed network event for registration with "
                        "Event ReceiverId: {} for Operation: {}. Deregistration denied.",
                EventReceiverId, OperationName)
                .c_str());
    }
}

void NetworkEventBus::StopListenConversationEvent(csp::common::String EventReceiverId)
{
    NetworkEventRegistration Registration(EventReceiverId, StringFromNetworkEvent(NetworkEvent::Conversation));

    size_t RemovedCount = RegisteredConversationEvents.erase(Registration);

    if (RemovedCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format(
                "NetworkEventBus::StopListenConversationEvent: Could not find conversation network event registration with Event ReceiverId: {}. "
                "Deregistration denied.",
                EventReceiverId)
                .c_str());
    }
}

void NetworkEventBus::StopListenSequenceChangedEvent(csp::common::String EventReceiverId)
{
    NetworkEventRegistration Registration(EventReceiverId, StringFromNetworkEvent(NetworkEvent::SequenceChanged));

    size_t RemovedCount = RegisteredSequenceChangedEvents.erase(Registration);

    if (RemovedCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("NetworkEventBus::StopListenSequenceChangedEvent: Could not find sequence changed network event registration with Event "
                        "ReceiverId: {}. "
                        "Deregistration denied.",
                EventReceiverId)
                .c_str());
    }
}

void NetworkEventBus::StopListenAllNetworkEvents(const csp::common::String& EventReceiverId)
{
    auto StopListenMatchingEventReceivers = [&](const auto& RegisteredEvents, auto&& StopListenAction) -> size_t
    {
        std::vector<NetworkEventRegistration> RegistrationsToRemove {};

        for (const auto& [Registration, Callback] : RegisteredEvents)
        {
            if (Registration.EventReceiverId == EventReceiverId)
            {
                RegistrationsToRemove.push_back(Registration);
            }
        }

        for (const NetworkEventRegistration& RegistrationToRemove : RegistrationsToRemove)
        {
            StopListenAction(RegistrationToRemove.EventReceiverId, RegistrationToRemove.EventName);
        }

        return RegistrationsToRemove.size();
    };

    size_t RegistrationsToRemoveCount = 0;

    RegistrationsToRemoveCount += StopListenMatchingEventReceivers(RegisteredCustomEvents,
        [&](const auto& EventReceiverId, const auto& EventName) { StopListenCustomNetworkEvent(EventReceiverId, EventName); });
    RegistrationsToRemoveCount += StopListenMatchingEventReceivers(RegisteredAccessControlChangedEvents,
        [&](const auto& EventReceiverId, const auto& /*EventName*/) { StopListenAccessControlChangedEvent(EventReceiverId); });
    RegistrationsToRemoveCount += StopListenMatchingEventReceivers(RegisteredAssetDetailBlobChangedEvents,
        [&](const auto& EventReceiverId, const auto& /*EventName*/) { StopListenAssetDetailBlobChangedEvent(EventReceiverId); });
    RegistrationsToRemoveCount += StopListenMatchingEventReceivers(RegisteredAsyncCallCompletedEvents,
        [&](const auto& EventReceiverId, const auto& OperationName) { StopListenAsyncCallCompletedEvent(EventReceiverId, OperationName); });
    RegistrationsToRemoveCount += StopListenMatchingEventReceivers(
        RegisteredConversationEvents, [&](const auto& EventReceiverId, const auto& /*EventName*/) { StopListenConversationEvent(EventReceiverId); });
    RegistrationsToRemoveCount += StopListenMatchingEventReceivers(RegisteredSequenceChangedEvents,
        [&](const auto& EventReceiverId, const auto& /*EventName*/) { StopListenSequenceChangedEvent(EventReceiverId); });

    if (RegistrationsToRemoveCount == 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("NetworkEventBus::StopListenAllNetworkEvents: Could not find any network events registered with EventReceiverId: {}. No "
                        "events were deregistered.",
                EventReceiverId)
                .c_str());
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose,
            fmt::format("NetworkEventBus::StopListenAllNetworkEvents: Removed {} network event registration/s with EventReceiverId: {}.",
                RegistrationsToRemoveCount, EventReceiverId)
                .c_str());
    }
}

csp::common::Array<NetworkEventRegistration> NetworkEventBus::AllRegistrations() const
{
    size_t TotalRegistrationsCount = RegisteredCustomEvents.size() + RegisteredAccessControlChangedEvents.size()
        + RegisteredAssetDetailBlobChangedEvents.size() + RegisteredAsyncCallCompletedEvents.size() + RegisteredConversationEvents.size()
        + RegisteredSequenceChangedEvents.size();

    csp::common::Array<NetworkEventRegistration> Registrations(TotalRegistrationsCount);

    size_t Index = 0;

    // Flatten all event containers into a single array of NetworkEventRegistration objects.
    auto CopyRegistrations = [&Index, &Registrations](const auto& EventMap)
    {
        for (const auto& [Key, Value] : EventMap)
        {
            Registrations[Index++] = Key;
        }
    };

    CopyRegistrations(RegisteredCustomEvents);
    CopyRegistrations(RegisteredAccessControlChangedEvents);
    CopyRegistrations(RegisteredAssetDetailBlobChangedEvents);
    CopyRegistrations(RegisteredAsyncCallCompletedEvents);
    CopyRegistrations(RegisteredConversationEvents);
    CopyRegistrations(RegisteredSequenceChangedEvents);

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
        // For custom events registered via ListenCustomNetworkEvent, the EventTypeStr will be the name of the event. In this case the
        // NetworkEventFromString() method call below will return NetworkEvent::GeneralPurposeEvent, and the EventTypeStr will be used to look up the
        // registered callback.
        auto EventType = NetworkEventFromString(EventTypeStr);

        bool HasMatchingRegistrations = false;

        switch (EventType)
        {
        case csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged:
        {
            if (RegisteredAssetDetailBlobChangedEvents.size() > 0)
            {
                csp::common::AssetDetailBlobChangedNetworkEventData AssetDetailBlobChangedEventData;

                if (!TryDeserializeEventData(
                        LogSystem, EventTypeStr, [&] { return csp::multiplayer::DeserializeAssetDetailBlobChangedEvent(EventValues, LogSystem); },
                        AssetDetailBlobChangedEventData))
                {
                    return;
                }

                for (auto const& [Registration, Callback] : RegisteredAssetDetailBlobChangedEvents)
                {
                    Callback(AssetDetailBlobChangedEventData);
                }

                HasMatchingRegistrations = true;
            }

            break;
        }
        case csp::multiplayer::NetworkEventBus::NetworkEvent::Conversation:
        {
            if (RegisteredConversationEvents.size() > 0)
            {
                csp::common::ConversationNetworkEventData ConversationEventData;

                if (!TryDeserializeEventData(
                        LogSystem, EventTypeStr, [&] { return csp::multiplayer::DeserializeConversationEvent(EventValues, LogSystem); },
                        ConversationEventData))
                {
                    return;
                }

                for (auto const& [Registration, Callback] : RegisteredConversationEvents)
                {
                    Callback(ConversationEventData);
                }

                HasMatchingRegistrations = true;
            }

            break;
        }
        case csp::multiplayer::NetworkEventBus::NetworkEvent::SequenceChanged:
        {
            if (RegisteredSequenceChangedEvents.size() > 0)
            {
                csp::common::SequenceChangedNetworkEventData SequenceChangedEventData;

                if (!TryDeserializeEventData(
                        LogSystem, EventTypeStr, [&] { return csp::multiplayer::DeserializeSequenceChangedEvent(EventValues, LogSystem); },
                        SequenceChangedEventData))
                {
                    return;
                }

                const csp::common::String Key = SequenceChangedEventData.Key;
                const csp::common::String SequenceType = csp::multiplayer::GetSequenceKeyIndex(Key, 0);

                if (SequenceType == "Hotspots")
                {
                    SequenceChangedEventData.SequenceType = csp::common::ESequenceType::Hotspot;

                    // If it is a hotspot, the 'key' will contain the following information [SequenceType]:[SpaceId]:[SequenceName]
                    // eg: Hotspots:abc123456:My-Hotspot-Sequence
                    csp::common::String OldHotspotSequenceName = csp::multiplayer::GetSequenceKeyIndex(SequenceChangedEventData.Key, 2);
                    // NewKey will be structured in the same way, eg: Hotspots:abc123456:My-New-Hotspot-Sequence
                    csp::common::String NewHotspotSequenceName = csp::multiplayer::GetSequenceKeyIndex(SequenceChangedEventData.NewKey, 2);

                    SequenceChangedEventData.SpaceId = csp::multiplayer::GetSequenceKeyIndex(SequenceChangedEventData.Key, 1);
                    SequenceChangedEventData.Key = OldHotspotSequenceName;
                    SequenceChangedEventData.NewKey = NewHotspotSequenceName;
                }

                for (auto const& [Registration, Callback] : RegisteredSequenceChangedEvents)
                {
                    Callback(SequenceChangedEventData);
                }

                HasMatchingRegistrations = true;
            }

            break;
        }
        case csp::multiplayer::NetworkEventBus::NetworkEvent::AccessControlChanged:
        {
            if (RegisteredAccessControlChangedEvents.size() > 0)
            {
                csp::common::AccessControlChangedNetworkEventData AccessControlChangedEventData;

                if (!TryDeserializeEventData(
                        LogSystem, EventTypeStr, [&] { return csp::multiplayer::DeserializeAccessControlChangedEvent(EventValues, LogSystem); },
                        AccessControlChangedEventData))
                {
                    return;
                }

                for (auto const& [Registration, Callback] : RegisteredAccessControlChangedEvents)
                {
                    Callback(AccessControlChangedEventData);
                }

                HasMatchingRegistrations = true;
            }

            break;
        }
        case csp::multiplayer::NetworkEventBus::NetworkEvent::AsyncCallCompleted:
        {
            if (RegisteredAsyncCallCompletedEvents.size() > 0)
            {
                csp::common::AsyncCallCompletedEventData AsyncCallCompletedEventData;

                if (!TryDeserializeEventData(
                        LogSystem, EventTypeStr, [&] { return csp::multiplayer::DeserializeAsyncCallCompletedEvent(EventValues, LogSystem); },
                        AsyncCallCompletedEventData))
                {
                    return;
                }

                for (auto const& [Registration, Callback] : RegisteredAsyncCallCompletedEvents)
                {
                    if (Registration.EventName == AsyncCallCompletedEventData.OperationName)
                    {
                        Callback(AsyncCallCompletedEventData);

                        HasMatchingRegistrations = true;
                    }
                }
            }
            break;
        }
        default:
        case csp::multiplayer::NetworkEventBus::NetworkEvent::GeneralPurposeEvent:
        {
            if (RegisteredCustomEvents.size() > 0)
            {
                csp::common::NetworkEventData GeneralPurposeEventData;

                if (!TryDeserializeEventData(
                        LogSystem, EventTypeStr, [&] { return csp::multiplayer::DeserializeGeneralPurposeEvent(EventValues, LogSystem); },
                        GeneralPurposeEventData))
                {
                    return;
                }

                for (auto const& [Registration, Callback] : RegisteredCustomEvents)
                {
                    if (Registration.EventName == EventTypeStr)
                    {
                        Callback(GeneralPurposeEventData);

                        HasMatchingRegistrations = true;
                    }
                }
            }
            break;
        }
        }

        if (!HasMatchingRegistrations)
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Verbose, fmt::format("Received event {} has no registrations, discarding...", EventTypeStr).c_str());
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
} // namespace csp::multiplayer
