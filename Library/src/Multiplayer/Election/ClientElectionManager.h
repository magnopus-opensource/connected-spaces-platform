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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "ClientProxy.h"

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{

class IClientSelectionCriteria;
class OnlineRealtimeEngine;
class SpaceEntity;

enum class ElectionState
{
    Idle,
    Requested,
    Electing,
};

class ClientElectionManager
{
    typedef std::function<void(bool)> ScriptLeaderReadyCallback;

    /** @cond DO_NOT_DOCUMENT */
    friend class ClientProxy;
    friend class OnlineRealtimeEngine;
    friend class ClientElectionEventHandler;
    /** @endcond */

public:
    ClientElectionManager(
        OnlineRealtimeEngine* inOnlineRealtimeEngine, csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& jsScriptRunner);
    ~ClientElectionManager();

    void OnConnect(const csp::common::List<SpaceEntity*>& avatars, const csp::common::List<SpaceEntity*>& objects);
    void OnDisconnect();

    void OnLocalClientAdd(const SpaceEntity* clientAvatar, const csp::common::List<SpaceEntity*>& avatars, NetworkEventBus& networkEventBus);

    void OnClientAdd(const SpaceEntity* clientAvatar, const csp::common::List<SpaceEntity*>& avatars, NetworkEventBus& networkEventBus);
    void OnClientRemove(const SpaceEntity* clientAvatar, const csp::common::List<SpaceEntity*>& avatars);
    void OnObjectAdd(const SpaceEntity* object, const csp::common::List<SpaceEntity*>& objects);
    void OnObjectRemove(const SpaceEntity* object, const csp::common::List<SpaceEntity*>& objects);

    void Update();

    bool IsLocalClientLeader() const;

    ClientProxy* GetLeader() const;

private:
    void BindNetworkEvents();
    void UnBindNetworkEvents();

    void OnClientElectionEvent(const csp::common::Array<csp::common::ReplicatedValue>& data);
    void OnRemoteRunScriptEvent(const csp::common::Array<csp::common::ReplicatedValue>& data);

    ClientProxy* AddClientUsingAvatar(const SpaceEntity* clientAvatar, NetworkEventBus& networkEventBus);
    void RemoveClientUsingAvatar(const SpaceEntity* clientAvatar);
    ClientProxy* FindClientUsingAvatar(const SpaceEntity* clientAvatar);

    ClientProxy* AddClientUsingId(int64_t clientId, NetworkEventBus& networkEventBus);
    void RemoveClientUsingId(int64_t clientId);
    ClientProxy* FindClientUsingId(int64_t clientId);

    bool IsConnected() const;

    void SetLeader(ClientProxy* client);
    void CheckLeaderIsValid();
    void OnLeaderRemoved();

    // void UpdateClientStates();

    // Election state handlers
    void HandleElectionStateIdle();
    void HandleElectionStateRequested();
    void HandleElectionStateElecting();

    void OnElectionComplete(int64_t leaderId);
    void OnLeaderNotification(int64_t leaderId);

    // Async functions that may take a while as they initiate network events between all clients
    void AsyncNegotiateLeader();

    void SetElectionState(ElectionState newState);

    void SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback scriptLeaderReadyCallback);

private:
    OnlineRealtimeEngine* m_onlineRealtimeEnginePtr;
    csp::common::LogSystem& m_logSystem;
    class ClientElectionEventHandler* m_eventHandler;

    ClientMap m_clients;

    ConnectionState m_theConnectionState;
    std::atomic<ElectionState> m_theElectionState;

    ClientProxy* m_localClient;

    ClientProxy* m_leader;

    ScriptLeaderReadyCallback m_leaderReadyCallback;
    csp::common::IJSScriptRunner& m_remoteScriptRunner;
};

} // namespace csp::multiplayer
