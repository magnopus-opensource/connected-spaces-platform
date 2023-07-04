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

#include "CSP/Multiplayer/ReplicatedValue.h"

#include <atomic>
#include <chrono>
#include <list>
#include <map>


namespace csp::common
{

class CancellationToken;

}


namespace csp::multiplayer
{

class ClientElectionManager;

class SpaceEntity;


enum class ClientElectionState
{
	Idle,
	Electing,
	Leader
};

constexpr const char* ClientElectionMessage	 = "ClientElectionMessage";
constexpr const char* RemoteRunScriptMessage = "RemoteRunScriptMessage";

// Default time to wait for a response from an election message
constexpr const std::chrono::system_clock::duration DEFAULT_ELECTION_TIMEOUT = std::chrono::milliseconds(2000);
// Period between leader heatbeats
constexpr std::chrono::milliseconds LEADER_HEARTBEAT_PERIOD = std::chrono::milliseconds(5000);
// Period to wait before sending leader lost message
constexpr std::chrono::milliseconds LEADER_HEARTBEAT_LOST_PERIOD = LEADER_HEARTBEAT_PERIOD * 3;

constexpr uint64_t ALL_CLIENTS_ID		 = -1;
constexpr double STEADY_TICKS_PER_SECOND = double(std::chrono::steady_clock::period::den) / double(std::chrono::steady_clock::period::num);

enum class ClientElectionMessageType
{
	Election = 0,
	ElectionResponse,
	ElectionLeader,
	ElectionNotifyLeader,
	LeaderHeartbeat,
	LeaderLost,

	NumElectionMessages
};


using ClientScore = int64_t;
using ClientId	  = int64_t;
using EventId	  = int64_t;


struct ElectionEvent
{
	std::atomic<EventId> Id;
	std::atomic<ClientId> TargetClient;
	std::atomic<ClientElectionMessageType> Type;
};


using ClientList = std::list<class ClientProxy*>;
using ClientMap	 = std::map<ClientId, class ClientProxy*>;


class ClientProxy
{
public:
	ClientProxy(ClientId Id, ClientElectionManager* ElectionManager);

	void UpdateState();

	[[nodiscard]] ClientId GetId() const;

	void StartLeaderElection(const ClientMap& Clients);

	void HandleEvent(int64_t EventType, int64_t ClientId);

	void NotifyLeader(int64_t TargetClientId, int64_t LeaderClientId);

	void RunScript(int64_t ContextId, const csp::common::String& ScriptText);

	void UpdateLeaderHeartbeat();
	void SendLeaderHeartbeat();
	void SendLeaderLost();
	void SetAsLeader(bool IsLeader);
	bool IsLeaderStillValid() const;

private:
	void HandleIdleState();
	void HandleElectingState();
	void HandleLeaderState();

	void SendElectionEvent(int64_t TargetClientId);
	void SendElectionResponseEvent(int64_t TargetClientId);
	void SendElectionLeaderEvent(int64_t TargetClientId);
	void SendEvent(int64_t TargetClientId, int64_t EventType, int64_t ClientId);

	void SendRemoteRunScriptEvent(int64_t TargetClientId, int64_t ContextId, const csp::common::String& ScriptText);

	void HandleElectionEvent(int64_t ClientId);
	void HandleElectionResponseEvent(int64_t ClientId);
	void HandleElectionLeaderEvent(int64_t ClientId);
	void HandleElectionNotifyLeaderEvent(int64_t ClientId);
	void HandleLeaderHeartbeatEvent(int64_t ClientId);
	void HandleLeaderLostEvent(int64_t ClientId);

	bool IsThisClientLeader(const ClientMap& Clients) const;

	ClientElectionManager* ElectionManagerPtr;

	ClientElectionState State;
	ClientId Id;
	EventId Eid;

	ElectionEvent PendingEvent;

	std::atomic_int PendingElections;

	std::chrono::system_clock::time_point ElectionStartTime;

	csp::common::CancellationToken* CancellationToken;

	std::chrono::steady_clock::time_point LastHeartBeatTime;
	bool IsLeaderValid;
};

} // namespace csp::multiplayer
