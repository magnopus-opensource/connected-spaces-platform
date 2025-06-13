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
#include "CSP/Multiplayer/EventBus.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Systems/SystemBase.h"
#include "Common/CallHelpers.h"
#include "Multiplayer/EventSerialisation.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"

#include "CSP/Common/CSPAsyncScheduler.h"
#include <fmt/format.h>
#include <limits>
#include <memory>
#include <optional>

namespace csp::multiplayer
{

constexpr const uint64_t ALL_CLIENTS_ID = std::numeric_limits<uint64_t>::max();

EventBus::~EventBus() { }

EventBus::EventBus(MultiplayerConnection* InMultiplayerConnection, csp::common::LogSystem& LogSystem)
    : LogSystem(LogSystem)
{
    MultiplayerConnectionInst = InMultiplayerConnection;
    SystemsNetworkEventMap = {};
    CallbacksNetworkEventMap = {};
}

void EventBus::ListenNetworkEvent(const csp::common::String& EventName, csp::systems::SystemBase* System)
{
    if (!System)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Error: Expected non-null system.");
        return;
    }

    if (!CallbacksNetworkEventMap.empty() && CallbacksNetworkEventMap.find(EventName) != CallbacksNetworkEventMap.end())
    {
        if (CallbacksNetworkEventMap[EventName])
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("Error: there is already a callback registered for {}.", EventName).c_str());
            return;
        }
    }

    if (!SystemsNetworkEventMap.empty() && SystemsNetworkEventMap.find(EventName) != SystemsNetworkEventMap.end()
        && SystemsNetworkEventMap[EventName])
    {
        if (SystemsNetworkEventMap[EventName] != System)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("Error: there is already a system registered for {}. Deregister it first.", EventName).c_str());
        }
        else
        {
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("This system is already registered for {}.", EventName).c_str());
        }
        return;
    }

    SystemsNetworkEventMap[EventName] = System;
}

void EventBus::ListenNetworkEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback)
{
    if (!Callback)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Error: Expected non-null callback.");
        return;
    }

    if (!SystemsNetworkEventMap.empty() && SystemsNetworkEventMap.find(EventName) != SystemsNetworkEventMap.end()
        && SystemsNetworkEventMap[EventName])
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("Error: there is already a system registered for {}. Deregister the system before registering a callback.", EventName)
                .c_str());
        return;
    }

    if (!CallbacksNetworkEventMap.empty() && CallbacksNetworkEventMap.find(EventName) != CallbacksNetworkEventMap.end())
    {
        if (CallbacksNetworkEventMap[EventName])
        {
            // We cannot compare callbacks, so we can't know whether it is the same callback that is already set. Therefore, we always update it
            LogSystem.LogMsg(
                csp::common::LogLevel::VeryVerbose, fmt::format("The callback set for {} was overwritten with a new callback.", EventName).c_str());
        }
    }

    CallbacksNetworkEventMap[EventName] = Callback;
}

void EventBus::StopListenNetworkEvent(const csp::common::String& EventName)
{
    // There is no need to split this into two different functions because we will always
    // have either a system or a callback, not both
    if (!SystemsNetworkEventMap.empty() && SystemsNetworkEventMap.find(EventName) != SystemsNetworkEventMap.end())
    {
        SystemsNetworkEventMap.erase(EventName);
    }
    if (!CallbacksNetworkEventMap.empty() && CallbacksNetworkEventMap.find(EventName) != CallbacksNetworkEventMap.end())
    {
        CallbacksNetworkEventMap.erase(EventName);
    }
}

void EventBus::StartEventMessageListening()
{
    if (MultiplayerConnectionInst->GetSignalRConnection() == nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Error : Multiplayer connection is unavailable, EventBus cannot start listening to events.");
        return;
    }

    std::function<void(signalr::value)> LocalCallback = [this](signalr::value Result)
    {
        if (Result.is_null())
        {
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "Event values were empty.");
            return;
        }

        if (CallbacksNetworkEventMap.empty() && SystemsNetworkEventMap.empty())
        {
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "Event map was empty.");
            return;
        }

        std::vector<signalr::value> EventValues = Result.as_array()[0].as_array();
        const csp::common::String EventType(EventValues[0].as_string().c_str());

        if (CallbacksNetworkEventMap.find(EventType) == CallbacksNetworkEventMap.end()
            && SystemsNetworkEventMap.find(EventType) == SystemsNetworkEventMap.end())
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::VeryVerbose, fmt::format("Event {} is no longer registered to, discarding...", EventType).c_str());
            return;
        }

        if (SystemsNetworkEventMap.find(EventType) != SystemsNetworkEventMap.end() && SystemsNetworkEventMap[EventType])
        {
            SystemsNetworkEventMap[EventType]->OnEvent(EventValues);
        }
        else if (CallbacksNetworkEventMap.find(EventType) != CallbacksNetworkEventMap.end())
        {
            // For everything else, use the generic deserialiser
            EventDeserialiser Deserialiser { LogSystem };
            Deserialiser.Parse(EventValues);

            CallbacksNetworkEventMap[EventType](true, Deserialiser.GetEventData());
        }
    };

    MultiplayerConnectionInst->GetSignalRConnection()->On("OnEventMessage", LocalCallback);
}

void EventBus::SendNetworkEvent(
    const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback)
{
    SendNetworkEventToClient(EventName, Args, ALL_CLIENTS_ID, Callback);
}

async::task<std::optional<csp::multiplayer::ErrorCode>> EventBus::SendNetworkEvent(
    const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args)
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

void EventBus::SendNetworkEventToClient(
    const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, uint64_t TargetClientId, ErrorCodeCallbackHandler Callback)
{
    MultiplayerConnectionInst->GetNetworkEventManager()->SendNetworkEvent(EventName, Args, TargetClientId, Callback);
}

} // namespace csp::multiplayer
