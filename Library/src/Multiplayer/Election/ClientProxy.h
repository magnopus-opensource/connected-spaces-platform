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

#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/ReplicatedValue.h"

#include <atomic>
#include <chrono>
#include <list>
#include <map>

namespace csp::common
{

class CancellationToken;
class LogSystem;

}

namespace csp::multiplayer
{

class ClientElectionManager;
class NetworkEventBus;
class SpaceEntity;

enum class ClientElectionState
{
    Idle,
    Electing
};

constexpr const char* ClientElectionMessage = "ClientElectionMessage";
constexpr const char* RemoteRunScriptMessage = "RemoteRunScriptMessage";

// Default time to wait for a response from an election message
constexpr const std::chrono::system_clock::duration DefaultElectionTimeOut = std::chrono::milliseconds(2000);

enum class ClientElectionMessageType
{
    Election = 0,
    ElectionResponse,
    ElectionLeader,
    ElectionNotifyLeader,

    NumElectionMessages
};

using ClientScore = int64_t;
using ClientId = int64_t;
using EventId = int64_t;

struct ElectionEvent
{
    std::atomic<EventId> Id;
    std::atomic<ClientId> TargetClient;
    std::atomic<ClientElectionMessageType> Type;
};

using ClientList = std::list<class ClientProxy*>;
using ClientMap = std::map<ClientId, class ClientProxy*>;

class ClientProxy
{
public:
    ClientProxy(ClientId id, ClientElectionManager* electionManager, csp::common::LogSystem& logSystem,
        csp::multiplayer::NetworkEventBus& networkEventBus, csp::common::IJSScriptRunner& scriptRunner);

    void UpdateState();

    [[nodiscard]] ClientId GetId() const;

    void StartLeaderElection(const ClientMap& clients);

    void HandleEvent(int64_t eventType, int64_t clientId);

    void NotifyLeader(int64_t targetClientId, int64_t leaderClientId);

    void RunScript(int64_t contextId, const csp::common::String& scriptText);

private:
    void HandleIdleState();
    void HandleElectingState();

    void SendElectionEvent(int64_t targetClientId);
    void SendElectionResponseEvent(int64_t targetClientId);
    void SendElectionLeaderEvent(int64_t targetClientId);

    void SendEvent(int64_t targetClientId, int64_t eventType, int64_t clientId);

    void SendRemoteRunScriptEvent(int64_t targetClientId, int64_t contextId, const csp::common::String& scriptText);

    void HandleElectionEvent(int64_t clientId);
    void HandleElectionResponseEvent(int64_t clientId);
    void HandleElectionLeaderEvent(int64_t clientId);
    void HandleElectionNotifyLeaderEvent(int64_t clientId);

    bool IsThisClientLeader(const ClientMap& clients) const;

    ClientElectionManager* m_electionManagerPtr;

    ClientElectionState m_state;
    ClientId m_id;
    ClientId m_highestResponseId;
    EventId m_eid;

    std::atomic_int m_pendingElections;

    std::chrono::system_clock::time_point m_electionStartTime;

    csp::common::LogSystem& m_logSystem;
    csp::common::IJSScriptRunner& m_scriptRunner;
    csp::multiplayer::NetworkEventBus& m_networkEventBus;
};

} // namespace csp::multiplayer
