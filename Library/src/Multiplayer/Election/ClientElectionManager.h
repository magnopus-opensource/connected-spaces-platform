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

#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "ClientProxy.h"

namespace csp::multiplayer
{

class IClientSelectionCriteria;
class SpaceEntitySystem;
class SpaceEntity;

enum class ElectionState
{
    Idle,
    Requested,
    Electing,
};

class ClientElectionManager
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ClientProxy;
    friend class SpaceEntitySystem;
    friend class ClientElectionEventHandler;
    /** @endcond */

public:
    ClientElectionManager(SpaceEntitySystem* InSpaceEntitySystem);
    ~ClientElectionManager();

    void OnConnect(const SpaceEntitySystem::SpaceEntityList& Avatars, const SpaceEntitySystem::SpaceEntityList& Objects);
    void OnDisconnect();

    void OnLocalClientAdd(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars);

    void OnClientAdd(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars);
    void OnClientRemove(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars);
    void OnObjectAdd(const SpaceEntity* Object, const SpaceEntitySystem::SpaceEntityList& Objects);
    void OnObjectRemove(const SpaceEntity* Object, const SpaceEntitySystem::SpaceEntityList& Objects);

    void Update();

    SpaceEntitySystem* GetSpaceEntitySystem();

    bool IsLocalClientLeader() const;

    ClientProxy* GetLeader() const;

private:
    void BindNetworkEvents();
    void UnBindNetworkEvents();

    void OnClientElectionEvent(const csp::common::Array<ReplicatedValue>& Data);
    void OnRemoteRunScriptEvent(const csp::common::Array<ReplicatedValue>& Data);

    ClientProxy* AddClientUsingAvatar(const SpaceEntity* ClientAvatar);
    void RemoveClientUsingAvatar(const SpaceEntity* ClientAvatar);
    ClientProxy* FindClientUsingAvatar(const SpaceEntity* ClientAvatar);

    ClientProxy* AddClientUsingId(int64_t ClientId);
    void RemoveClientUsingId(int64_t ClientId);
    ClientProxy* FindClientUsingId(int64_t ClientId);

    bool IsConnected() const;

    void SetLeader(ClientProxy* Client);
    void CheckLeaderIsValid();
    void OnLeaderRemoved();

    // void UpdateClientStates();

    // Election state handlers
    void HandleElectionStateIdle();
    void HandleElectionStateRequested();
    void HandleElectionStateElecting();

    void OnElectionComplete(int64_t LeaderId);
    void OnLeaderNotification(int64_t LeaderId);

    // Async functions that may take a while as they initiate network events between all clients
    void AsyncNegotiateLeader();

    void SetElectionState(ElectionState NewState);

    void SetScriptSystemReadyCallback(csp::multiplayer::SpaceEntitySystem::CallbackHandler InScriptSystemReadyCallback);

private:
    SpaceEntitySystem* SpaceEntitySystemPtr;
    class ClientElectionEventHandler* EventHandler;

    ClientMap Clients;

    ConnectionState TheConnectionState;
    std::atomic<ElectionState> TheElectionState;

    ClientProxy* LocalClient;

    ClientProxy* Leader;

    csp::multiplayer::SpaceEntitySystem::CallbackHandler ScriptSystemReadyCallback;
};

} // namespace csp::multiplayer
