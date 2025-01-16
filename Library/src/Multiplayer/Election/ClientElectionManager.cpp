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

#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "Debug/Logging.h"
#include "Events/Event.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Memory/Memory.h"
#include "signalrclient/signalr_value.h"

namespace csp::multiplayer
{

class ClientElectionEventHandler : public csp::events::EventListener
{
public:
    ClientElectionEventHandler(ClientElectionManager* ElectionManager);

    void OnEvent(const csp::events::Event& InEvent) override;

private:
    ClientElectionManager* ElectionManager;
};

ClientElectionEventHandler::ClientElectionEventHandler(ClientElectionManager* ElectionManager)
    : ElectionManager(ElectionManager)
{
}

void ClientElectionEventHandler::OnEvent(const csp::events::Event& InEvent)
{
    if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && ElectionManager->IsConnected())
    {
        ElectionManager->Update();
    }
    else if (InEvent.GetId() == csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID)
    {
    }
}

ClientElectionManager::ClientElectionManager(SpaceEntitySystem* InSpaceEntitySystem)
    : SpaceEntitySystemPtr(InSpaceEntitySystem)
    , EventHandler(CSP_NEW ClientElectionEventHandler(this))
    , TheConnectionState(ConnectionState::Disconnected)
    , TheElectionState(ElectionState::Idle)
    , LocalClient(nullptr)
    , Leader(nullptr)
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().RegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);

    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "ClientElectionManager Created");
}

ClientElectionManager::~ClientElectionManager()
{
    UnBindNetworkEvents();

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);
    CSP_DELETE(EventHandler);

    for (const auto& Client : Clients)
    {
        ClientProxy* Proxy = Client.second;
        CSP_DELETE(Proxy);
    }
}

void ClientElectionManager::OnConnect(const SpaceEntitySystem::SpaceEntityList& Avatars, const SpaceEntitySystem::SpaceEntityList& Objects)
{
    CSP_LOG_MSG(csp::systems::LogLevel::Verbose, "ClientElectionManager::OnConnect called");

    BindNetworkEvents();

    if (Avatars.Size() > 0)
    {
        // On connect, the first client to enter a space is set as leader

        // @note we also assume first client to enter is the last avatar in the array.
        // This seems to be consistent currently, but can we rely on this long term?
        const SpaceEntity* ClientAvatar = Avatars[Avatars.Size() - 1];
        ClientProxy* Client = FindClientUsingAvatar(ClientAvatar);
        SetLeader(Client);
    }

    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "Number of clients=%d", Avatars.Size());
}

void ClientElectionManager::OnDisconnect()
{
    for (const auto& Client : Clients)
    {
        ClientProxy* Proxy = Client.second;
        CSP_DELETE(Proxy);
    }

    UnBindNetworkEvents();
}

void ClientElectionManager::OnLocalClientAdd(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnLocalClientAdd called : ClientId=%d", ClientAvatar->GetOwnerId());

    bool IsFirstClient = false;

    if (Avatars.Size() == 1)
    {
        CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "IsFirstClient=true");

        // If there is just one avatar, then it should be us,
        // So we we'll will assume the leadership role for now
        // pending negotiation if/when other clients connect
        IsFirstClient = true;
    }
    else
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "IsFirstClient=false : Num Avatars %d", Avatars.Size());
    }

    ClientProxy* Client = AddClientUsingAvatar(ClientAvatar);
    LocalClient = Client;

    if (IsFirstClient)
    {
        // We are the first (and currently only client), so just start acting as leader
        SetLeader(LocalClient);
    }
}

void ClientElectionManager::OnClientAdd(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnClientAdd called : ClientId=%d", ClientAvatar->GetOwnerId());
    AddClientUsingAvatar(ClientAvatar);
}

void ClientElectionManager::OnClientRemove(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnClientRemove called : ClientId=%d", ClientAvatar->GetOwnerId());
    RemoveClientUsingAvatar(ClientAvatar);
}

void ClientElectionManager::OnObjectAdd(const SpaceEntity* Object, const SpaceEntitySystem::SpaceEntityList& Objects)
{
    CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnObjectAdd called");
    // @Todo - This event allows us to track individual object ownership
}

void ClientElectionManager::OnObjectRemove(const SpaceEntity* Object, const SpaceEntitySystem::SpaceEntityList& Objects)
{
    CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnObjectRemove called");
    // @Todo - This event allows us to track individual object ownership
}

ClientProxy* ClientElectionManager::AddClientUsingAvatar(const SpaceEntity* ClientAvatar)
{
    if (ClientAvatar == nullptr)
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Error, "Invalid entity pointer");
        return nullptr;
    }

    const int64_t ClientId = static_cast<int64_t>(ClientAvatar->GetOwnerId());
    return AddClientUsingId(ClientId);
}

void ClientElectionManager::RemoveClientUsingAvatar(const SpaceEntity* ClientAvatar)
{
    if (ClientAvatar == nullptr)
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Error, "Invalid entity pointer");
        return;
    }

    const int64_t ClientId = static_cast<int64_t>(ClientAvatar->GetOwnerId());

    RemoveClientUsingId(ClientId);
}

ClientProxy* ClientElectionManager::FindClientUsingAvatar(const SpaceEntity* ClientAvatar)
{
    if (ClientAvatar == nullptr)
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Error, "Invalid entity pointer");
        return nullptr;
    }

    const int64_t ClientId = static_cast<int64_t>(ClientAvatar->GetOwnerId());
    return FindClientUsingId(ClientId);
}

ClientProxy* ClientElectionManager::AddClientUsingId(int64_t ClientId)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::AddClientUsingAvatar called : ClientId=%d", ClientId);

    ClientProxy* Client = nullptr;

    if (Clients.find(ClientId) == Clients.end())
    {
        Client = CSP_NEW ClientProxy(ClientId, this);
        Clients.insert(ClientMap::value_type(ClientId, Client));

        if ((LocalClient != nullptr) && (Leader != nullptr))
        {
            // If a new client connects when we have a valid leader then notify them who it is
            // If it receives conflicting information then it will trigger a re-negotiation
            LocalClient->NotifyLeader(ClientId, Leader->GetId());
        }
    }
    else
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Warning, "Client already exists");
    }

    return Client;
}

void ClientElectionManager::RemoveClientUsingId(int64_t ClientId)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::RemoveClientUsingId called : ClientId=%d", ClientId);

    const auto ClientIt = Clients.find(ClientId);

    if (ClientIt != Clients.end())
    {
        ClientProxy* Client = ClientIt->second;

        if ((Client == Leader) && (Client != LocalClient))
        {
            // Handle the current leader being removed
            OnLeaderRemoved();
        }
        else if (Client == LocalClient)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "Local Client %d removed", ClientId);
            LocalClient = nullptr;
        }

        CSP_DELETE(Client);
        Clients.erase(ClientId);
    }
    else
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Warning, "Client not found");
    }
}

ClientProxy* ClientElectionManager::FindClientUsingId(int64_t ClientId)
{
    const auto ClientIt = Clients.find(ClientId);

    if (ClientIt != Clients.end())
    {
        ClientProxy* Client = ClientIt->second;
        return Client;
    }
    else
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Warning, "ClientElectionManager::FindClientById Client %d not found", ClientId);
    }

    return nullptr;
}

void ClientElectionManager::Update()
{
    //	CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::Update called");

    switch (TheElectionState)
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

    if (LocalClient)
    {
        LocalClient->UpdateState();
    }

    CheckLeaderIsValid();

    static ClientProxy* LastLeader = nullptr;

    if (Leader != LastLeader)
    {
        // Leader has changed
        if (Leader != nullptr)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "ClientElectionManager::Update - Leader is %d", Leader->GetId());
        }
        LastLeader = Leader;
    }
}

SpaceEntitySystem* ClientElectionManager::GetSpaceEntitySystem() { return SpaceEntitySystemPtr; }

bool ClientElectionManager::IsLocalClientLeader() const { return (LocalClient && (LocalClient == Leader)); }

ClientProxy* ClientElectionManager::GetLeader() const { return Leader; }

void ClientElectionManager::SetLeader(ClientProxy* Client)
{
    if (Client != nullptr)
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::SetLeader ClientId=%d", Client->GetId());
    }
    else
    {
        CSP_LOG_ERROR_MSG("ClientElectionManager::SetLeader Client is null");
    }

    Leader = Client;

    // Notify Scripts ready callback now we have a valid leader
    if (ScriptSystemReadyCallback)
    {
        ScriptSystemReadyCallback(true);
    }
}

void ClientElectionManager::CheckLeaderIsValid()
{
    // Todo :- Ping leader and check they're still there
}

void ClientElectionManager::OnLeaderRemoved()
{
    Leader = nullptr;

    if (LocalClient != nullptr)
    {
        // The current leader has left, so we may need to find a new one
        AsyncNegotiateLeader();
    }
}

void ClientElectionManager::AsyncNegotiateLeader()
{
    if (Clients.size() < 2)
    {
        CSP_LOG_WARN_MSG("AsyncNegotiateLeader called when no other clients");
        return;
    }

    if (LocalClient == nullptr)
    {
        CSP_LOG_ERROR_MSG("AsyncNegotiateLeader called when no local client");
        return;
    }

    if (TheElectionState != ElectionState::Idle)
    {
        CSP_LOG_ERROR_MSG("AsyncNegotiateLeader called when election already in progress");
        return;
    }

    // Request election on next update
    SetElectionState(ElectionState::Requested);
}

void ClientElectionManager::SetElectionState(ElectionState NewState)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::SetElectionState From %d to %d", TheElectionState.load(), NewState);
    TheElectionState = NewState;
}

void ClientElectionManager::SetScriptSystemReadyCallback(csp::multiplayer::SpaceEntitySystem::CallbackHandler InScriptSystemReadyCallback)
{
    ScriptSystemReadyCallback = InScriptSystemReadyCallback;
}

void ClientElectionManager::HandleElectionStateIdle()
{
    // Nothing needed currently
}

void ClientElectionManager::HandleElectionStateRequested()
{
    // Start negotiating with other clients on who should be leader
    if (LocalClient != nullptr)
    {
        CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "HandleElectionStateRequested");
        SetElectionState(ElectionState::Electing);
        LocalClient->StartLeaderElection(Clients);
    }
}

void ClientElectionManager::HandleElectionStateElecting()
{
    // Nothing needed currently
}

void ClientElectionManager::OnElectionComplete(int64_t LeaderId)
{
    if (TheElectionState != ElectionState::Electing)
    {
        CSP_LOG_WARN_FORMAT("ClientElectionManager::OnElectionComplete called when no election in progress (State=%d)", TheElectionState.load());
    }

    SetElectionState(ElectionState::Idle);

    if (Clients.find(LeaderId) != Clients.end())
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Verbose, "OnElectionComplete: Elected Leader is %d", LeaderId);
        SetLeader(Clients[LeaderId]);
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("OnElectionComplete: Unknown leader %d", LeaderId);
    }
}

void ClientElectionManager::OnLeaderNotification(int64_t LeaderId)
{
    if (Leader)
    {
        if (Leader->GetId() != LeaderId)
        {
            CSP_LOG_ERROR_FORMAT("ClientElectionManager::OnLeaderNotification - Unexpected LeaderId %d", LeaderId);

            // Leader Id was not what we were expecting
            // Resolve the conflict by re-negotiating
            AsyncNegotiateLeader();
        }
        else
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnLeaderNotification ClientId=%d is as expected", LeaderId);
        }
    }
    else
    {
        ClientProxy* Client = FindClientUsingId(LeaderId);
        SetLeader(Client);
    }
}

bool ClientElectionManager::IsConnected() const
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    MultiplayerConnection* Connection = SystemsManager.GetMultiplayerConnection();

    if (Connection == nullptr)
    {
        return false;
    }

    return Connection->Connected;
}

void ClientElectionManager::BindNetworkEvents()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    EventBus* EventBus = SystemsManager.GetEventBus();

    EventBus->ListenNetworkEvent(
        ClientElectionMessage, [this](bool ok, const csp::common::Array<ReplicatedValue>& Data) { this->OnClientElectionEvent(Data); });

    EventBus->ListenNetworkEvent(
        RemoteRunScriptMessage, [this](bool ok, const csp::common::Array<ReplicatedValue>& Data) { this->OnRemoteRunScriptEvent(Data); });
}

void ClientElectionManager::UnBindNetworkEvents()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    EventBus* EventBus = SystemsManager.GetEventBus();

    EventBus->StopListenNetworkEvent(ClientElectionMessage);
    EventBus->StopListenNetworkEvent(RemoteRunScriptMessage);
}

void ClientElectionManager::OnClientElectionEvent(const csp::common::Array<ReplicatedValue>& Data)
{
    // @Note This needs to be kept in sync with any changes to message format
    const int64_t EventType = static_cast<int64_t>(Data[0].GetInt());
    const int64_t ClientId = static_cast<int64_t>(Data[1].GetInt());

    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnClientElectionEvent called. Event=%d, Id=%d", EventType, ClientId);

    if (LocalClient != nullptr)
    {
        LocalClient->HandleEvent(EventType, ClientId);
    }
}

void ClientElectionManager::OnRemoteRunScriptEvent(const csp::common::Array<ReplicatedValue>& Data)
{
    // @Note This needs to be kept in sync with any changes to message format
    const int64_t ContextId = static_cast<int64_t>(Data[0].GetInt());
    const csp::common::String& ScriptText = Data[1].GetString();

    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientElectionManager::OnRemoteRunScriptEvent called. ContextId=%lld, Script='%s'",
        ContextId, ScriptText.c_str());

    if (LocalClient != nullptr)
    {
        if (IsLocalClientLeader())
        {
            csp::systems::ScriptSystem* TheScriptSystem = csp::systems::SystemsManager::Get().GetScriptSystem();
            TheScriptSystem->RunScript(ContextId, ScriptText);
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("Client %d has received remote script event but is not the Leader", LocalClient->GetId());
        }
    }
}

} // namespace csp::multiplayer
