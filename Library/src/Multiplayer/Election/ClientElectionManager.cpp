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
#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Script/ScriptSystem.h"
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

ClientElectionManager::ClientElectionManager(
    SpaceEntitySystem* InSpaceEntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& JSScriptRunner)
    : SpaceEntitySystemPtr(InSpaceEntitySystem)
    , LogSystem(LogSystem)
    , EventHandler(new ClientElectionEventHandler(this))
    , TheConnectionState(ConnectionState::Disconnected)
    , TheElectionState(ElectionState::Idle)
    , LocalClient(nullptr)
    , Leader(nullptr)
    , RemoteScriptRunner(JSScriptRunner)
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().RegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);

    LogSystem.LogMsg(csp::common::LogLevel::Verbose, "ClientElectionManager Created");
}

ClientElectionManager::~ClientElectionManager()
{
    UnBindNetworkEvents();

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);
    delete (EventHandler);

    for (const auto& Client : Clients)
    {
        ClientProxy* Proxy = Client.second;
        delete (Proxy);
    }
}

void ClientElectionManager::OnConnect(const SpaceEntitySystem::SpaceEntityList& Avatars, const SpaceEntitySystem::SpaceEntityList& /*Objects*/)
{
    LogSystem.LogMsg(csp::common::LogLevel::Verbose, "ClientElectionManager::OnConnect called");

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

    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("Number of clients={}", Avatars.Size()).c_str());
}

void ClientElectionManager::OnDisconnect()
{
    for (const auto& Client : Clients)
    {
        ClientProxy* Proxy = Client.second;
        delete (Proxy);
    }

    UnBindNetworkEvents();
}

void ClientElectionManager::OnLocalClientAdd(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& Avatars)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnLocalClientAdd called : ClientId={}", ClientAvatar->GetOwnerId()).c_str());

    bool IsFirstClient = false;

    if (Avatars.Size() == 1)
    {
        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "IsFirstClient=true");

        // If there is just one avatar, then it should be us,
        // So we we'll will assume the leadership role for now
        // pending negotiation if/when other clients connect
        IsFirstClient = true;
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("IsFirstClient=false : Num Avatars {}", Avatars.Size()).c_str());
    }

    ClientProxy* Client = AddClientUsingAvatar(ClientAvatar);
    LocalClient = Client;

    if (IsFirstClient)
    {
        // We are the first (and currently only client), so just start acting as leader
        SetLeader(LocalClient);
    }
}

void ClientElectionManager::OnClientAdd(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& /*Avatars*/)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnLocalClientAdd called : ClientId={}", ClientAvatar->GetOwnerId()).c_str());
    AddClientUsingAvatar(ClientAvatar);
}

void ClientElectionManager::OnClientRemove(const SpaceEntity* ClientAvatar, const SpaceEntitySystem::SpaceEntityList& /*Avatars*/)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnLocalClientAdd called : ClientId={}", ClientAvatar->GetOwnerId()).c_str());
    RemoveClientUsingAvatar(ClientAvatar);
}

void ClientElectionManager::OnObjectAdd(const SpaceEntity* /*Object*/, const SpaceEntitySystem::SpaceEntityList& /*Objects*/)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientElectionManager::OnObjectAdd called");
    // @Todo - This event allows us to track individual object ownership
}

void ClientElectionManager::OnObjectRemove(const SpaceEntity* /*Object*/, const SpaceEntitySystem::SpaceEntityList& /*Objects*/)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientElectionManager::OnObjectRemove called");
    // @Todo - This event allows us to track individual object ownership
}

ClientProxy* ClientElectionManager::AddClientUsingAvatar(const SpaceEntity* ClientAvatar)
{
    if (ClientAvatar == nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Invalid entity pointer");
        return nullptr;
    }

    const int64_t ClientId = static_cast<int64_t>(ClientAvatar->GetOwnerId());
    return AddClientUsingId(ClientId);
}

void ClientElectionManager::RemoveClientUsingAvatar(const SpaceEntity* ClientAvatar)
{
    if (ClientAvatar == nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Invalid entity pointer");
        return;
    }

    const int64_t ClientId = static_cast<int64_t>(ClientAvatar->GetOwnerId());

    RemoveClientUsingId(ClientId);
}

ClientProxy* ClientElectionManager::FindClientUsingAvatar(const SpaceEntity* ClientAvatar)
{
    if (ClientAvatar == nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Invalid entity pointer");
        return nullptr;
    }

    const int64_t ClientId = static_cast<int64_t>(ClientAvatar->GetOwnerId());
    return FindClientUsingId(ClientId);
}

ClientProxy* ClientElectionManager::AddClientUsingId(int64_t ClientId)
{
    LogSystem.LogMsg(
        csp::common::LogLevel::VeryVerbose, fmt::format("ClientElectionManager::AddClientUsingAvatar called : ClientId={}", ClientId).c_str());

    ClientProxy* Client = nullptr;

    if (Clients.find(ClientId) == Clients.end())
    {
        Client = new ClientProxy(ClientId, this, LogSystem);
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
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "Client already exists");
    }

    return Client;
}

void ClientElectionManager::RemoveClientUsingId(int64_t ClientId)
{
    LogSystem.LogMsg(
        csp::common::LogLevel::VeryVerbose, fmt::format("ClientElectionManager::RemoveClientUsingId called : ClientId={}", ClientId).c_str());

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
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("Local Client {} removed", ClientId).c_str());
            LocalClient = nullptr;
        }

        delete (Client);
        Clients.erase(ClientId);
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "Client not found");
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
        LogSystem.LogMsg(csp::common::LogLevel::Warning, fmt::format("ClientElectionManager::FindClientById Client {} not found", ClientId).c_str());
    }

    return nullptr;
}

void ClientElectionManager::Update()
{
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
            LogSystem.LogMsg(csp::common::LogLevel::Log, fmt::format("ClientElectionManager::Update - Leader is {}", Leader->GetId()).c_str());
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
        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("ClientElectionManager::SetLeader ClientId={}", Client->GetId()).c_str());
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "ClientElectionManager::SetLeader Client is null");
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
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "AsyncNegotiateLeader called when no other clients");
        return;
    }

    if (LocalClient == nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "AsyncNegotiateLeader called when no local client");
        return;
    }

    if (TheElectionState != ElectionState::Idle)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "AsyncNegotiateLeader called when election already in progress");
        return;
    }

    // Request election on next update
    SetElectionState(ElectionState::Requested);
}

void ClientElectionManager::SetElectionState(ElectionState NewState)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::SetElectionState From {0} to {1}", static_cast<int>(TheElectionState.load()), static_cast<int>(NewState))
            .c_str());
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
        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "HandleElectionStateRequested");
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
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format(
                "ClientElectionManager::OnElectionComplete called when no election in progress (State={})", static_cast<int>(TheElectionState.load()))
                .c_str());
    }

    SetElectionState(ElectionState::Idle);

    if (Clients.find(LeaderId) != Clients.end())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, fmt::format("OnElectionComplete: Elected Leader is {}", LeaderId).c_str());
        SetLeader(Clients[LeaderId]);
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("OnElectionComplete: Unknown leader {}", LeaderId).c_str());
    }
}

void ClientElectionManager::OnLeaderNotification(int64_t LeaderId)
{
    if (Leader)
    {
        if (Leader->GetId() != LeaderId)
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Error, fmt::format("ClientElectionManager::OnLeaderNotification - Unexpected LeaderId {}", LeaderId).c_str());

            // Leader Id was not what we were expecting
            // Resolve the conflict by re-negotiating
            AsyncNegotiateLeader();
        }
        else
        {
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
                fmt::format("ClientElectionManager::OnLeaderNotification ClientId={} is as expected", LeaderId).c_str());
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
    MultiplayerConnection* Connection = SpaceEntitySystemPtr->GetMultiplayerConnectionInstance();

    if (Connection == nullptr)
    {
        return false;
    }

    return Connection->IsConnected();
}

void ClientElectionManager::BindNetworkEvents()
{
    EventBus* EventBus = SpaceEntitySystemPtr->GetMultiplayerConnectionInstance()->GetEventBusPtr();

    EventBus->ListenNetworkEvent(
        ClientElectionMessage, [this](bool /*ok*/, const csp::common::Array<ReplicatedValue>& Data) { this->OnClientElectionEvent(Data); });

    EventBus->ListenNetworkEvent(
        RemoteRunScriptMessage, [this](bool /*ok*/, const csp::common::Array<ReplicatedValue>& Data) { this->OnRemoteRunScriptEvent(Data); });
}

void ClientElectionManager::UnBindNetworkEvents()
{
    EventBus* EventBus = SpaceEntitySystemPtr->GetMultiplayerConnectionInstance()->GetEventBusPtr();

    EventBus->StopListenNetworkEvent(ClientElectionMessage);
    EventBus->StopListenNetworkEvent(RemoteRunScriptMessage);
}

void ClientElectionManager::OnClientElectionEvent(const csp::common::Array<ReplicatedValue>& Data)
{
    // @Note This needs to be kept in sync with any changes to message format
    const int64_t EventType = static_cast<int64_t>(Data[0].GetInt());
    const int64_t ClientId = static_cast<int64_t>(Data[1].GetInt());

    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnClientElectionEvent called. Event={0}, Id={1}", EventType, ClientId).c_str());

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

    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnRemoteRunScriptEvent called. ContextId={0}, Script={1}", ContextId, ScriptText.c_str()).c_str());

    if (LocalClient != nullptr)
    {
        if (IsLocalClientLeader())
        {
            RemoteScriptRunner.RunScript(ContextId, ScriptText);
        }
        else
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error,
                fmt::format("Client {} has received remote script event but is not the Leader", LocalClient->GetId()).c_str());
        }
    }
}

} // namespace csp::multiplayer
