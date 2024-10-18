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
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "NetworkEventManagerImpl.h"

namespace csp::multiplayer
{

extern ErrorCode ParseError(std::exception_ptr Exception);

constexpr const uint64_t ALL_CLIENTS_ID = -1;

EventBus::~EventBus()
{
	CSP_DELETE(MultiplayerConnectionInst);
}

EventBus::EventBus(const EventBus& InBoundConnection)
{
	MultiplayerConnectionInst = InBoundConnection.MultiplayerConnectionInst;
	EventMap				  = InBoundConnection.EventMap;
}

EventBus::EventBus(MultiplayerConnection* InMultiplayerConnection)
{
	MultiplayerConnectionInst = InMultiplayerConnection;
	EventMap				  = {};
}

void EventBus::ListenEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback, csp::systems::SystemBase* System)
{
	if (MultiplayerConnectionInst->Connection == nullptr || !MultiplayerConnectionInst->Connected)
	{
		return;
	}

	if (!Callback && !System)
		return;

	if (Callback)
	{
		EventMap[EventName].first.push_back(Callback);
	}
	if (System)
	{
		EventMap[EventName].second = System;
	}
}

void EventBus::ListenEvent(const csp::common::String& EventName, csp::systems::SystemBase* System)
{
	ListenEvent(EventName, nullptr, System);
}

void EventBus::ListenEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback)
{
	ListenEvent(EventName, Callback, nullptr);
}

void EventBus::StopListenEvent(const csp::common::String& EventName)
{
	if (!EventMap.empty() && EventMap.find(EventName) != EventMap.end())
	{
		EventMap.erase(EventName);
	}
}

void EventBus::StartEventMessageListening()
{
	std::function<void(signalr::value)> LocalCallback = [this](signalr::value Result)
	{
		if (Result.is_null())
		{
			return;
		}

		if (EventMap.empty())
		{
			return;
		}

		std::vector<signalr::value> EventValues = Result.as_array()[0].as_array();
		const csp::common::String EventType(EventValues[0].as_string().c_str());

		if (EventMap.find(EventType) == EventMap.end())
		{
			return;
		}

		if (EventMap[EventType].second)
		{
			EventMap[EventType].second->Deserialise(EventValues);
		}
		else
		{
			// For everything else, use the generic deserialiser
			EventDeserialiser Deserialiser;
			Deserialiser.Parse(EventValues);

			for (const auto& Callback : EventMap[EventType].first)
			{
				Callback(true, Deserialiser.GetEventData());
			}
		}
	};

	MultiplayerConnectionInst->Connection->On("OnEventMessage", LocalCallback);
}

} // namespace csp::multiplayer
