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
#include "ClientElectionManager.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Events/Event.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "signalrclient/signalr_value.h"
#include <fmt/format.h>

namespace csp::multiplayer
{

class ClientElectionEventHandler : public csp::events::EventListener
{
public:
    ClientElectionEventHandler(ClientElectionManager* electionManager);

    void OnEvent(const csp::events::Event& inEvent) override;

private:
    ClientElectionManager* m_electionManager;
};

ClientElectionEventHandler::ClientElectionEventHandler(ClientElectionManager* electionManager)
    : m_electionManager(electionManager)
{
}

void ClientElectionEventHandler::OnEvent(const csp::events::Event& inEvent)
{
    if (inEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && m_electionManager->IsConnected())
    {
        m_electionManager->Update();
    }
}

ClientElectionManager::ClientElectionManager(
    OnlineRealtimeEngine* inOnlineRealtimeEngine, csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& jsScriptRunner)
    : m_onlineRealtimeEnginePtr(inOnlineRealtimeEngine)
    , m_logSystem(logSystem)
    , m_eventHandler(new ClientElectionEventHandler(this))
    , m_theConnectionState(ConnectionState::Disconnected)
    , m_theElectionState(ElectionState::Idle)
    , m_localClient(nullptr)
    , m_leader(nullptr)
    , m_remoteScriptRunner(jsScriptRunner)
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler);

    logSystem.LogMsg(csp::common::LogLevel::Verbose, "ClientElectionManager Created");
}

ClientElectionManager::~ClientElectionManager()
{
    UnBindNetworkEvents();

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler);
    delete (m_eventHandler);

    for (const auto& client : m_clients)
    {
        ClientProxy* proxy = client.second;
        delete (proxy);
    }
}

void ClientElectionManager::OnConnect(const csp::common::List<SpaceEntity*>& avatars, const csp::common::List<SpaceEntity*>& /*Objects*/)
{
    m_logSystem.LogMsg(csp::common::LogLevel::Verbose, "ClientElectionManager::OnConnect called");

    BindNetworkEvents();

    if (avatars.Size() > 0)
    {
        // On connect, the first client to enter a space is set as leader

        // @note we also assume first client to enter is the last avatar in the array.
        // This seems to be consistent currently, but can we rely on this long term?
        const SpaceEntity* clientAvatar = avatars[avatars.Size() - 1];
        ClientProxy* client = FindClientUsingAvatar(clientAvatar);
        SetLeader(client);
    }

    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("Number of clients={}", avatars.Size()).c_str());
}

void ClientElectionManager::OnDisconnect()
{
    for (const auto& client : m_clients)
    {
        ClientProxy* proxy = client.second;
        delete (proxy);
    }

    UnBindNetworkEvents();
}

void ClientElectionManager::OnLocalClientAdd(
    const SpaceEntity* clientAvatar, const csp::common::List<SpaceEntity*>& avatars, NetworkEventBus& networkEventBus)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnLocalClientAdd called : ClientId={}", clientAvatar->GetOwnerId()).c_str());

    bool isFirstClient = false;

    if (avatars.Size() == 1)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "IsFirstClient=true");

        // If there is just one avatar, then it should be us,
        // So we we'll will assume the leadership role for now
        // pending negotiation if/when other clients connect
        isFirstClient = true;
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("IsFirstClient=false : Num Avatars {}", avatars.Size()).c_str());
    }

    ClientProxy* client = AddClientUsingAvatar(clientAvatar, networkEventBus);
    m_localClient = client;

    if (isFirstClient)
    {
        // We are the first (and currently only client), so just start acting as leader
        SetLeader(m_localClient);
    }
}

void ClientElectionManager::OnClientAdd(
    const SpaceEntity* clientAvatar, const csp::common::List<SpaceEntity*>& /*Avatars*/, NetworkEventBus& networkEventBus)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnLocalClientAdd called : ClientId={}", clientAvatar->GetOwnerId()).c_str());
    AddClientUsingAvatar(clientAvatar, networkEventBus);
}

void ClientElectionManager::OnClientRemove(const SpaceEntity* clientAvatar, const csp::common::List<SpaceEntity*>& /*Avatars*/)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnLocalClientAdd called : ClientId={}", clientAvatar->GetOwnerId()).c_str());
    RemoveClientUsingAvatar(clientAvatar);
}

void ClientElectionManager::OnObjectAdd(const SpaceEntity* /*Object*/, const csp::common::List<SpaceEntity*>& /*Objects*/)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientElectionManager::OnObjectAdd called");
    // @Todo - This event allows us to track individual object ownership
}

void ClientElectionManager::OnObjectRemove(const SpaceEntity* /*Object*/, const csp::common::List<SpaceEntity*>& /*Objects*/)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientElectionManager::OnObjectRemove called");
    // @Todo - This event allows us to track individual object ownership
}

ClientProxy* ClientElectionManager::AddClientUsingAvatar(const SpaceEntity* clientAvatar, NetworkEventBus& networkEventBus)
{
    if (clientAvatar == nullptr)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "Invalid entity pointer");
        return nullptr;
    }

    const int64_t clientId = static_cast<int64_t>(clientAvatar->GetOwnerId());
    return AddClientUsingId(clientId, networkEventBus);
}

void ClientElectionManager::RemoveClientUsingAvatar(const SpaceEntity* clientAvatar)
{
    if (clientAvatar == nullptr)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "Invalid entity pointer");
        return;
    }

    const int64_t clientId = static_cast<int64_t>(clientAvatar->GetOwnerId());

    RemoveClientUsingId(clientId);
}

ClientProxy* ClientElectionManager::FindClientUsingAvatar(const SpaceEntity* clientAvatar)
{
    if (clientAvatar == nullptr)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "Invalid entity pointer");
        return nullptr;
    }

    const int64_t clientId = static_cast<int64_t>(clientAvatar->GetOwnerId());
    return FindClientUsingId(clientId);
}

ClientProxy* ClientElectionManager::AddClientUsingId(int64_t clientId, NetworkEventBus& networkEventBus)
{
    m_logSystem.LogMsg(
        csp::common::LogLevel::VeryVerbose, fmt::format("ClientElectionManager::AddClientUsingAvatar called : ClientId={}", clientId).c_str());

    ClientProxy* client = nullptr;

    if (m_clients.find(clientId) == m_clients.end())
    {
        client = new ClientProxy(clientId, this, m_logSystem, networkEventBus, m_remoteScriptRunner);
        m_clients.insert(ClientMap::value_type(clientId, client));

        if ((m_localClient != nullptr) && (m_leader != nullptr))
        {
            // If a new client connects when we have a valid leader then notify them who it is
            // If it receives conflicting information then it will trigger a re-negotiation
            m_localClient->NotifyLeader(clientId, m_leader->GetId());
        }
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning, "Client already exists");
    }

    return client;
}

void ClientElectionManager::RemoveClientUsingId(int64_t clientId)
{
    m_logSystem.LogMsg(
        csp::common::LogLevel::VeryVerbose, fmt::format("ClientElectionManager::RemoveClientUsingId called : ClientId={}", clientId).c_str());

    const auto clientIt = m_clients.find(clientId);

    if (clientIt != m_clients.end())
    {
        ClientProxy* client = clientIt->second;

        if ((client == m_leader) && (client != m_localClient))
        {
            // Handle the current leader being removed
            OnLeaderRemoved();
        }
        else if (client == m_localClient)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("Local Client {} removed", clientId).c_str());
            m_localClient = nullptr;
        }

        delete (client);
        m_clients.erase(clientId);
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning, "Client not found");
    }
}

ClientProxy* ClientElectionManager::FindClientUsingId(int64_t clientId)
{
    const auto clientIt = m_clients.find(clientId);

    if (clientIt != m_clients.end())
    {
        ClientProxy* client = clientIt->second;
        return client;
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning, fmt::format("ClientElectionManager::FindClientById Client {} not found", clientId).c_str());
    }

    return nullptr;
}

void ClientElectionManager::Update()
{
    switch (m_theElectionState)
    {
    case ElectionState::Idle:
        HandleElectionStateIdle();
        break;
    case ElectionState::Requested:
        HandleElectionStateRequested();
        break;
    case ElectionState::Electing:
        HandleElectionStateElecting();
        break;
    default:
        break;
    }

    if (m_localClient)
    {
        m_localClient->UpdateState();
    }

    CheckLeaderIsValid();

    static ClientProxy* lastLeader = nullptr;

    if (m_leader != lastLeader)
    {
        // Leader has changed
        if (m_leader != nullptr)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Log, fmt::format("ClientElectionManager::Update - Leader is {}", m_leader->GetId()).c_str());
        }
        lastLeader = m_leader;
    }
}

bool ClientElectionManager::IsLocalClientLeader() const { return (m_localClient && (m_localClient == m_leader)); }

ClientProxy* ClientElectionManager::GetLeader() const { return m_leader; }

void ClientElectionManager::SetLeader(ClientProxy* client)
{
    if (client != nullptr)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("ClientElectionManager::SetLeader ClientId={}", client->GetId()).c_str());
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "ClientElectionManager::SetLeader Client is null");
    }

    m_leader = client;

    // Notify Scripts ready callback now we have a valid leader
    if (m_leaderReadyCallback)
    {
        m_leaderReadyCallback(true);
    }
}

void ClientElectionManager::CheckLeaderIsValid()
{
    // Todo :- Ping leader and check they're still there
}

void ClientElectionManager::OnLeaderRemoved()
{
    m_leader = nullptr;

    if (m_localClient != nullptr)
    {
        // The current leader has left, so we may need to find a new one
        AsyncNegotiateLeader();
    }
}

void ClientElectionManager::AsyncNegotiateLeader()
{
    if (m_clients.size() < 2)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning, "AsyncNegotiateLeader called when no other clients");
        return;
    }

    if (m_localClient == nullptr)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "AsyncNegotiateLeader called when no local client");
        return;
    }

    if (m_theElectionState != ElectionState::Idle)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "AsyncNegotiateLeader called when election already in progress");
        return;
    }

    // Request election on next update
    SetElectionState(ElectionState::Requested);
}

void ClientElectionManager::SetElectionState(ElectionState newState)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::SetElectionState From {0} to {1}", static_cast<int>(m_theElectionState.load()), static_cast<int>(newState))
            .c_str());
    m_theElectionState = newState;
}

void ClientElectionManager::SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback scriptLeaderReadyCallback)
{
    m_leaderReadyCallback = scriptLeaderReadyCallback;
}

void ClientElectionManager::HandleElectionStateIdle()
{
    // Nothing needed currently
}

void ClientElectionManager::HandleElectionStateRequested()
{
    // Start negotiating with other clients on who should be leader
    if (m_localClient != nullptr)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "HandleElectionStateRequested");
        SetElectionState(ElectionState::Electing);
        m_localClient->StartLeaderElection(m_clients);
    }
}

void ClientElectionManager::HandleElectionStateElecting()
{
    // Nothing needed currently
}

void ClientElectionManager::OnElectionComplete(int64_t leaderId)
{
    if (m_theElectionState != ElectionState::Electing)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format(
                "ClientElectionManager::OnElectionComplete called when no election in progress (State={})", static_cast<int>(m_theElectionState.load()))
                .c_str());
    }

    SetElectionState(ElectionState::Idle);

    if (m_clients.find(leaderId) != m_clients.end())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Verbose, fmt::format("OnElectionComplete: Elected Leader is {}", leaderId).c_str());
        SetLeader(m_clients[leaderId]);
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("OnElectionComplete: Unknown leader {}", leaderId).c_str());
    }
}

void ClientElectionManager::OnLeaderNotification(int64_t leaderId)
{
    if (m_leader)
    {
        if (m_leader->GetId() != leaderId)
        {
            m_logSystem.LogMsg(
                csp::common::LogLevel::Error, fmt::format("ClientElectionManager::OnLeaderNotification - Unexpected LeaderId {}", leaderId).c_str());

            // Leader Id was not what we were expecting
            // Resolve the conflict by re-negotiating
            AsyncNegotiateLeader();
        }
        else
        {
            m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
                fmt::format("ClientElectionManager::OnLeaderNotification ClientId={} is as expected", leaderId).c_str());
        }
    }
    else
    {
        ClientProxy* client = FindClientUsingId(leaderId);
        SetLeader(client);
    }
}

bool ClientElectionManager::IsConnected() const
{
    MultiplayerConnection* connection = m_onlineRealtimeEnginePtr->GetMultiplayerConnectionInstance();

    if (connection == nullptr)
    {
        return false;
    }

    return connection->IsConnected();
}

void ClientElectionManager::BindNetworkEvents()
{
    NetworkEventBus& networkEventBus = m_onlineRealtimeEnginePtr->GetMultiplayerConnectionInstance()->GetEventBus();

    networkEventBus.ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("CSPInternal::ClientElectionManager", ClientElectionMessage),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnClientElectionEvent(networkEventData.EventValues); });

    networkEventBus.ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("CSPInternal::ClientElectionManager", RemoteRunScriptMessage),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnRemoteRunScriptEvent(networkEventData.EventValues); });
}

void ClientElectionManager::UnBindNetworkEvents()
{
    NetworkEventBus& networkEventBus = m_onlineRealtimeEnginePtr->GetMultiplayerConnectionInstance()->GetEventBus();

    networkEventBus.StopListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("CSPInternal::ClientElectionManager", ClientElectionMessage));
    networkEventBus.StopListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("CSPInternal::ClientElectionManager", RemoteRunScriptMessage));
}

void ClientElectionManager::OnClientElectionEvent(const csp::common::Array<csp::common::ReplicatedValue>& data)
{
    // @Note This needs to be kept in sync with any changes to message format
    const int64_t eventType = static_cast<int64_t>(data[0].GetInt());
    const int64_t clientId = static_cast<int64_t>(data[1].GetInt());

    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnClientElectionEvent called. Event={0}, Id={1}", eventType, clientId).c_str());

    if (m_localClient != nullptr)
    {
        m_localClient->HandleEvent(eventType, clientId);
    }
}

void ClientElectionManager::OnRemoteRunScriptEvent(const csp::common::Array<csp::common::ReplicatedValue>& data)
{
    // @Note This needs to be kept in sync with any changes to message format
    const int64_t contextId = static_cast<int64_t>(data[0].GetInt());
    const csp::common::String& scriptText = data[1].GetString();

    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnRemoteRunScriptEvent called. ContextId={0}, Script={1}", contextId, scriptText.c_str()).c_str());

    if (m_localClient != nullptr)
    {
        if (IsLocalClientLeader())
        {
            m_remoteScriptRunner.RunScript(contextId, scriptText);
        }
        else
        {
            m_logSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("Client {} has received remote script event but is not the Leader", m_localClient->GetId()).c_str());
        }
    }
}

} // namespace csp::multiplayer
