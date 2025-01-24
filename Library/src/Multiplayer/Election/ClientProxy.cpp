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
#include "Multiplayer/Election/ClientProxy.h"

#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "Debug/Logging.h"
#include "Multiplayer/Election/ClientElectionManager.h"

namespace csp::multiplayer
{

ClientProxy::ClientProxy(ClientId Id, ClientElectionManager* ElectionManager)
    : ElectionManagerPtr(ElectionManager)
    , State(ClientElectionState::Idle)
    , Id(Id)
    , HighestResponseId(0)
    , Eid(0)
    , PendingElections(0)
{
}

void ClientProxy::UpdateState()
{
    switch (State)
    {
    case ClientElectionState::Idle:
        HandleIdleState();
        break;

    case ClientElectionState::Electing:
        HandleElectingState();
        break;

    default:
        break;
    }
}

ClientId ClientProxy::GetId() const { return Id; }

void ClientProxy::StartLeaderElection(const ClientMap& Clients)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::StartLeaderElection ClientId=%d State=%d", Id, State);

    if (State != ClientElectionState::Idle)
    {
        CSP_LOG_ERROR_MSG("ClientProxy::StartLeaderElection called when election already in progress");
        return;
    }

    State = ClientElectionState::Electing;
    ElectionStartTime = std::chrono::system_clock::now();
    PendingElections = 0;
    HighestResponseId = Id;

    if (IsThisClientLeader(Clients))
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "This Client (%d) is Leader", Id);

        for (auto& Client : Clients)
        {
            const ClientId NodeId = Client.first;

            if (NodeId != Id)
            {
                SendElectionLeaderEvent(NodeId);
            }
        }

        State = ClientElectionState::Idle;

        if (ElectionManagerPtr)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "Calling OnElectionComplete Pending=%d", PendingElections.load());
            ElectionManagerPtr->OnElectionComplete(Id);
        }
    }
    else
    {
        for (auto& Client : Clients)
        {
            const ClientId NodeId = Client.first;

            if ((NodeId != Id) && (NodeId > Id))
            {
                SendElectionEvent(NodeId);
            }
        }
    }
}

bool ClientProxy::IsThisClientLeader(const ClientMap& Clients) const
{
    for (auto& Client : Clients)
    {
        const ClientId NodeId = Client.first;

        if ((NodeId != Id) && (NodeId > Id))
        {
            return false;
        }
    }

    return true;
}

void ClientProxy::HandleEvent(int64_t EventType, int64_t ClientId)
{
    ClientElectionMessageType MessageType = static_cast<ClientElectionMessageType>(EventType);

    switch (MessageType)
    {
    case ClientElectionMessageType::Election:
        HandleElectionEvent(ClientId);
        break;
    case ClientElectionMessageType::ElectionResponse:
        HandleElectionResponseEvent(ClientId);
        break;
    case ClientElectionMessageType::ElectionLeader:
        HandleElectionLeaderEvent(ClientId);
        break;
    case ClientElectionMessageType::ElectionNotifyLeader:
        HandleElectionNotifyLeaderEvent(ClientId);
        break;
    case ClientElectionMessageType::NumElectionMessages:
        // Do nothing
        break;
    }
}

void ClientProxy::NotifyLeader(int64_t TargetClientId, int64_t LeaderClientId)
{
    CSP_LOG_FORMAT(
        csp::systems::LogLevel::VeryVerbose, "ClientProxy::NotifyLeader Target=%d Source=%d Leader=%d", TargetClientId, Id, LeaderClientId);
    SendEvent(TargetClientId, static_cast<int64_t>(ClientElectionMessageType::ElectionNotifyLeader), LeaderClientId);
}

void ClientProxy::RunScript(int64_t ContextId, const csp::common::String& ScriptText)
{
    if (ContextId != Id)
    {
        const int64_t LeaderClientId = ElectionManagerPtr->GetLeader()->GetId();
        SendRemoteRunScriptEvent(LeaderClientId, ContextId, ScriptText);
    }
    else
    {
        csp::systems::ScriptSystem* TheScriptSystem = csp::systems::SystemsManager::Get().GetScriptSystem();
        TheScriptSystem->RunScript(ContextId, ScriptText);
    }
}

void ClientProxy::HandleIdleState()
{
    // Nothing to do currently
}

void ClientProxy::HandleElectingState()
{
    // Nothing to do currently
}

void ClientProxy::SendElectionEvent(int64_t TargetClientId)
{
    ++PendingElections;
    SendEvent(TargetClientId, static_cast<int64_t>(ClientElectionMessageType::Election), Id);

    // @todo Set timer and handle no response
}

void ClientProxy::SendElectionResponseEvent(int64_t TargetClientId)
{
    SendEvent(TargetClientId, static_cast<int64_t>(ClientElectionMessageType::ElectionResponse), Id);
}

void ClientProxy::SendElectionLeaderEvent(int64_t TargetClientId)
{
    SendEvent(TargetClientId, static_cast<int64_t>(ClientElectionMessageType::ElectionLeader), Id);
}

void ClientProxy::SendEvent(int64_t TargetClientId, int64_t EventType, int64_t ClientId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    EventBus* EventBus = SystemsManager.GetEventBus();

    const int64_t MessageId = Eid++;

    const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [](ErrorCode Error)
    {
        if (Error != ErrorCode::None)
        {
            CSP_LOG_ERROR_MSG("ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "SendNetworkEventToClient Target=%d Source=%d Type=%d", TargetClientId, ClientId, EventType);

    EventBus->SendNetworkEventToClient(ClientElectionMessage, { ReplicatedValue(EventType), ReplicatedValue(ClientId), ReplicatedValue(MessageId) },
        TargetClientId, SignalRCallback);
}

void ClientProxy::SendRemoteRunScriptEvent(int64_t TargetClientId, int64_t ContextId, const csp::common::String& ScriptText)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    EventBus* EventBus = SystemsManager.GetEventBus();

    const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [](ErrorCode Error)
    {
        if (Error != ErrorCode::None)
        {
            CSP_LOG_ERROR_MSG("ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "SendRemoteRunScriptEvent Target=%lld ContextId=%lld Script='%s'", TargetClientId, ContextId,
        ScriptText.c_str());

    EventBus->SendNetworkEventToClient(
        RemoteRunScriptMessage, { ReplicatedValue(ContextId), ReplicatedValue(ScriptText) }, TargetClientId, SignalRCallback);
}

void ClientProxy::HandleElectionEvent(int64_t ClientId)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionEvent ClientId=%d", ClientId);

    // We only need to send a response if we are being elected by a lower id
    if (ClientId < Id)
    {
        SendElectionResponseEvent(ClientId);
    }
}

void ClientProxy::HandleElectionResponseEvent(int64_t ClientId)
{
    CSP_LOG_FORMAT(
        csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent ClientId=%d Pending=%d", ClientId, PendingElections.load());

    if (ClientId > Id)
    {
        --PendingElections;

        if (ClientId > HighestResponseId)
        {
            // Remember the highest ClientId from all the responses
            // We then use this below to check this matches the elected leader
            // as (currently) we use the highest ClientId as our election criteria
            HighestResponseId = ClientId;
        }

        // All done
        if (ElectionManagerPtr && PendingElections == 0)
        {
            CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent All expected reponses received");

            // We should have received a valid leader event by now so check this is as expected
            if (ElectionManagerPtr)
            {
                if (ElectionManagerPtr->Leader)
                {
                    if (ElectionManagerPtr->Leader->GetId() == HighestResponseId)
                    {
                        CSP_LOG_MSG(
                            csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent Highest response matches elected leader");
                    }
                    else
                    {
                        CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose,
                            "ClientProxy::HandleElectionResponseEvent Highest response Id does not match elected leader");
                    }
                }
                else
                {
                    CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent Expected a valid leader by now!");
                }
            }
        }
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("ClientProxy::HandleElectionResponseEvent - Response from lower Id (%d/%d)", ClientId, Id);
    }

    // @Todo :- Handle response timeout (but this shouldn't happen with SignalR/TCP reliable connection)
}

void ClientProxy::HandleElectionLeaderEvent(int64_t LeaderId)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionLeaderEvent LeaderId=%d", LeaderId);

    State = ClientElectionState::Idle;

    if (ElectionManagerPtr)
    {
        // Election complete so set leader client
        ElectionManagerPtr->OnElectionComplete(LeaderId);
    }
    else
    {
        CSP_LOG_ERROR_MSG("ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
    }
}

void ClientProxy::HandleElectionNotifyLeaderEvent(int64_t ClientId)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionNotifyLeaderEvent ClientId=%d", ClientId);

    if (ElectionManagerPtr)
    {
        ElectionManagerPtr->OnLeaderNotification(ClientId);
    }
    else
    {
        CSP_LOG_ERROR_MSG("ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
    }
}

} // namespace csp::multiplayer
