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

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Multiplayer/EventBus.h"
#include "Multiplayer/Election/ClientElectionManager.h"

#include <fmt/format.h>

namespace csp::multiplayer
{

ClientProxy::ClientProxy(ClientId Id, ClientElectionManager* ElectionManager, csp::common::LogSystem& LogSystem, csp::multiplayer::EventBus& EventBus,
    csp::common::IJSScriptRunner& ScriptRunner)
    : ElectionManagerPtr(ElectionManager)
    , State(ClientElectionState::Idle)
    , Id(Id)
    , HighestResponseId(0)
    , Eid(0)
    , PendingElections(0)
    , LogSystem(LogSystem)
    , ScriptRunner(ScriptRunner)
    , EventBus(EventBus)
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
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientProxy::StartLeaderElection ClientId={0} State={1}", Id, static_cast<int>(State)).c_str());

    if (State != ClientElectionState::Idle)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::StartLeaderElection called when election already in progress");
        return;
    }

    State = ClientElectionState::Electing;
    ElectionStartTime = std::chrono::system_clock::now();
    PendingElections = 0;
    HighestResponseId = Id;

    if (IsThisClientLeader(Clients))
    {
        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("This Client ({}) is Leader", Id).c_str());

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
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("Calling OnElectionComplete Pending={}", Id).c_str());
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
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientProxy::NotifyLeader Target={0} Source={1} Leader={2}", TargetClientId, Id, LeaderClientId).c_str());
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
        ScriptRunner.RunScript(ContextId, ScriptText);
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
    const int64_t MessageId = Eid++;

    const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [&LogSystem = this->LogSystem](ErrorCode Error)
    {
        if (Error != ErrorCode::None)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("SendNetworkEventToClient Target={0} Source={1} Type={2}", TargetClientId, ClientId, EventType).c_str());

    EventBus.SendNetworkEventToClient(ClientElectionMessage, { ReplicatedValue(EventType), ReplicatedValue(ClientId), ReplicatedValue(MessageId) },
        TargetClientId, SignalRCallback);
}

void ClientProxy::SendRemoteRunScriptEvent(int64_t TargetClientId, int64_t ContextId, const csp::common::String& ScriptText)
{

    const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [&LogSystem = this->LogSystem](ErrorCode Error)
    {
        if (Error != ErrorCode::None)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("SendRemoteRunScriptEvent Target={0} ContextId={1} Script='{2}'", TargetClientId, ContextId, ScriptText).c_str());

    EventBus.SendNetworkEventToClient(
        RemoteRunScriptMessage, { ReplicatedValue(ContextId), ReplicatedValue(ScriptText) }, TargetClientId, SignalRCallback);
}

void ClientProxy::HandleElectionEvent(int64_t ClientId)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("ClientProxy::HandleElectionEvent ClientId={}", ClientId).c_str());

    // We only need to send a response if we are being elected by a lower id
    if (ClientId < Id)
    {
        SendElectionResponseEvent(ClientId);
    }
}

void ClientProxy::HandleElectionResponseEvent(int64_t ClientId)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientProxy::HandleElectionResponseEvent ClientId={0} Pending={1}", ClientId, PendingElections.load()).c_str());

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
            LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent All expected reponses received");

            // We should have received a valid leader event by now so check this is as expected
            if (ElectionManagerPtr)
            {
                if (ElectionManagerPtr->Leader)
                {
                    if (ElectionManagerPtr->Leader->GetId() == HighestResponseId)
                    {
                        LogSystem.LogMsg(
                            csp::common::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent Highest response matches elected leader");
                    }
                    else
                    {
                        LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
                            "ClientProxy::HandleElectionResponseEvent Highest response Id does not match elected leader");
                    }
                }
                else
                {
                    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent Expected a valid leader by now!");
                }
            }
        }
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ClientProxy::HandleElectionResponseEvent - Response from lower Id ({}/{})", ClientId, Id).c_str());
    }

    // @Todo :- Handle response timeout (but this shouldn't happen with SignalR/TCP reliable connection)
}

void ClientProxy::HandleElectionLeaderEvent(int64_t LeaderId)
{
    LogSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("ClientProxy::HandleElectionLeaderEvent LeaderId={}", LeaderId).c_str());

    State = ClientElectionState::Idle;

    if (ElectionManagerPtr)
    {
        // Election complete so set leader client
        ElectionManagerPtr->OnElectionComplete(LeaderId);
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
    }
}

void ClientProxy::HandleElectionNotifyLeaderEvent(int64_t ClientId)
{
    LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("ClientProxy::HandleElectionNotifyLeaderEvent ClientId={}", ClientId).c_str());

    if (ElectionManagerPtr)
    {
        ElectionManagerPtr->OnLeaderNotification(ClientId);
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
    }
}

} // namespace csp::multiplayer
