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

#include "CSP/Systems/SystemBase.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/EventSerialisation.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"
#include <limits>

namespace csp::multiplayer
{

extern ErrorCode ParseError(std::exception_ptr Exception);

constexpr const uint64_t ALL_CLIENTS_ID = std::numeric_limits<uint64_t>::max();

EventBus::~EventBus() { }

EventBus::EventBus(MultiplayerConnection* InMultiplayerConnection)
{
    MultiplayerConnectionInst = InMultiplayerConnection;
    SystemsNetworkEventMap = {};
    CallbacksNetworkEventMap = {};
}

void EventBus::ListenNetworkEvent(const csp::common::String& EventName, csp::systems::SystemBase* System)
{
    if (!System)
    {
        std::string ErrorMessage = "Error: Expected non-null system.";
        CSP_LOG_ERROR_FORMAT("%s", ErrorMessage.c_str());
        return;
    }

    if (!CallbacksNetworkEventMap.empty() && CallbacksNetworkEventMap.find(EventName) != CallbacksNetworkEventMap.end())
    {
        if (CallbacksNetworkEventMap[EventName])
        {
            CSP_LOG_ERROR_FORMAT("Error: there is already a callback registered for %s.", EventName.c_str());
            return;
        }
    }

    if (!SystemsNetworkEventMap.empty() && SystemsNetworkEventMap.find(EventName) != SystemsNetworkEventMap.end()
        && SystemsNetworkEventMap[EventName])
    {
        if (SystemsNetworkEventMap[EventName] != System)
        {
            CSP_LOG_ERROR_FORMAT("Error: there is already a system registered for %s. Deregister it first.", EventName.c_str());
        }
        else
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "This system is already registered for %s.", EventName.c_str());
        }
        return;
    }

    SystemsNetworkEventMap[EventName] = System;
}

void EventBus::ListenNetworkEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback)
{
    if (!Callback)
    {
        std::string ErrorMessage = "Error: Expected non-null callback.";
        CSP_LOG_ERROR_FORMAT("%s", ErrorMessage.c_str());
        return;
    }

    if (!SystemsNetworkEventMap.empty() && SystemsNetworkEventMap.find(EventName) != SystemsNetworkEventMap.end()
        && SystemsNetworkEventMap[EventName])
    {
        CSP_LOG_ERROR_FORMAT(
            "Error: there is already a system registered for %s. Deregister the system before registering a callback.", EventName.c_str());
        return;
    }

    if (!CallbacksNetworkEventMap.empty() && CallbacksNetworkEventMap.find(EventName) != CallbacksNetworkEventMap.end())
    {
        if (CallbacksNetworkEventMap[EventName])
        {
            // We cannot compare callbacks, so we can't know whether it is the same callback that is already set. Therefore, we always update it
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "The callback set for %s was overwritten with a new callback.", EventName.c_str());
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
    if (MultiplayerConnectionInst->Connection == nullptr)
    {
        CSP_LOG_ERROR_MSG("Error : Multiplayer connection is unavailable, EventBus cannot start listening to events.");
        return;
    }

    std::function<void(signalr::value)> LocalCallback = [this](signalr::value Result)
    {
        if (Result.is_null())
        {
            CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "Event values were empty.");
            return;
        }

        if (CallbacksNetworkEventMap.empty() && SystemsNetworkEventMap.empty())
        {
            CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "Event map was empty.");
            return;
        }

        std::vector<signalr::value> EventValues = Result.as_array()[0].as_array();
        const csp::common::String EventType(EventValues[0].as_string().c_str());

        if (CallbacksNetworkEventMap.find(EventType) == CallbacksNetworkEventMap.end()
            && SystemsNetworkEventMap.find(EventType) == SystemsNetworkEventMap.end())
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "Event %s is no longer registered to, discarding...", EventType.c_str());
            return;
        }

        if (SystemsNetworkEventMap.find(EventType) != SystemsNetworkEventMap.end() && SystemsNetworkEventMap[EventType])
        {
            SystemsNetworkEventMap[EventType]->OnEvent(EventValues);
        }
        else if (CallbacksNetworkEventMap.find(EventType) != CallbacksNetworkEventMap.end())
        {
            // For everything else, use the generic deserialiser
            EventDeserialiser Deserialiser;
            Deserialiser.Parse(EventValues);

            CallbacksNetworkEventMap[EventType](true, Deserialiser.GetEventData());
        }
    };

    MultiplayerConnectionInst->Connection->On("OnEventMessage", LocalCallback);
}

void EventBus::SendNetworkEvent(
    const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback)
{
    SendNetworkEventToClient(EventName, Args, ALL_CLIENTS_ID, Callback);
}

void EventBus::SendNetworkEventToClient(
    const csp::common::String& EventName, const csp::common::Array<ReplicatedValue>& Args, uint64_t TargetClientId, ErrorCodeCallbackHandler Callback)
{
    MultiplayerConnectionInst->NetworkEventManager->SendNetworkEvent(EventName, Args, TargetClientId, Callback);
}

} // namespace csp::multiplayer
