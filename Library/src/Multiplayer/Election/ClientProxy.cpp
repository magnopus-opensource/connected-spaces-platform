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
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "Multiplayer/Election/ClientElectionManager.h"

#include <fmt/format.h>

namespace csp::multiplayer
{

ClientProxy::ClientProxy(ClientId id, ClientElectionManager* electionManager, csp::common::LogSystem& logSystem,
    csp::multiplayer::NetworkEventBus& networkEventBus, csp::common::IJSScriptRunner& scriptRunner)
    : m_electionManagerPtr(electionManager)
    , m_state(ClientElectionState::Idle)
    , m_id(id)
    , m_highestResponseId(0)
    , m_eid(0)
    , m_pendingElections(0)
    , m_logSystem(logSystem)
    , m_scriptRunner(scriptRunner)
    , m_networkEventBus(networkEventBus)
{
}

void ClientProxy::UpdateState()
{
    switch (m_state)
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

ClientId ClientProxy::GetId() const { return m_id; }

void ClientProxy::StartLeaderElection(const ClientMap& clients)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientProxy::StartLeaderElection ClientId={0} State={1}", m_id, static_cast<int>(m_state)).c_str());

    if (m_state != ClientElectionState::Idle)
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::StartLeaderElection called when election already in progress");
        return;
    }

    m_state = ClientElectionState::Electing;
    m_electionStartTime = std::chrono::system_clock::now();
    m_pendingElections = 0;
    m_highestResponseId = m_id;

    if (IsThisClientLeader(clients))
    {
        m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("This Client ({}) is Leader", m_id).c_str());

        for (auto& client : clients)
        {
            const ClientId nodeId = client.first;

            if (nodeId != m_id)
            {
                SendElectionLeaderEvent(nodeId);
            }
        }

        m_state = ClientElectionState::Idle;

        if (m_electionManagerPtr)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("Calling OnElectionComplete Pending={}", m_id).c_str());
            m_electionManagerPtr->OnElectionComplete(m_id);
        }
    }
    else
    {
        for (auto& client : clients)
        {
            const ClientId nodeId = client.first;

            if ((nodeId != m_id) && (nodeId > m_id))
            {
                SendElectionEvent(nodeId);
            }
        }
    }
}

bool ClientProxy::IsThisClientLeader(const ClientMap& clients) const
{
    for (auto& client : clients)
    {
        const ClientId nodeId = client.first;

        if ((nodeId != m_id) && (nodeId > m_id))
        {
            return false;
        }
    }

    return true;
}

void ClientProxy::HandleEvent(int64_t eventType, int64_t clientId)
{
    ClientElectionMessageType messageType = static_cast<ClientElectionMessageType>(eventType);

    switch (messageType)
    {
    case ClientElectionMessageType::Election:
        HandleElectionEvent(clientId);
        break;
    case ClientElectionMessageType::ElectionResponse:
        HandleElectionResponseEvent(clientId);
        break;
    case ClientElectionMessageType::ElectionLeader:
        HandleElectionLeaderEvent(clientId);
        break;
    case ClientElectionMessageType::ElectionNotifyLeader:
        HandleElectionNotifyLeaderEvent(clientId);
        break;
    case ClientElectionMessageType::NumElectionMessages:
        // Do nothing
        break;
    }
}

void ClientProxy::NotifyLeader(int64_t targetClientId, int64_t leaderClientId)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientProxy::NotifyLeader Target={0} Source={1} Leader={2}", targetClientId, m_id, leaderClientId).c_str());
}

void ClientProxy::RunScript(int64_t contextId, const csp::common::String& scriptText)
{
    if (contextId != m_id)
    {
        const int64_t leaderClientId = m_electionManagerPtr->GetLeader()->GetId();
        SendRemoteRunScriptEvent(leaderClientId, contextId, scriptText);
    }
    else
    {
        m_scriptRunner.RunScript(contextId, scriptText);
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

void ClientProxy::SendElectionEvent(int64_t targetClientId)
{
    ++m_pendingElections;
    SendEvent(targetClientId, static_cast<int64_t>(ClientElectionMessageType::Election), m_id);

    // @todo Set timer and handle no response
}

void ClientProxy::SendElectionResponseEvent(int64_t targetClientId)
{
    SendEvent(targetClientId, static_cast<int64_t>(ClientElectionMessageType::ElectionResponse), m_id);
}

void ClientProxy::SendElectionLeaderEvent(int64_t targetClientId)
{
    SendEvent(targetClientId, static_cast<int64_t>(ClientElectionMessageType::ElectionLeader), m_id);
}

void ClientProxy::SendEvent(int64_t targetClientId, int64_t eventType, int64_t clientId)
{
    const int64_t messageId = m_eid++;

    const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [&logSystem = this->m_logSystem](ErrorCode error)
    {
        if (error != ErrorCode::None)
        {
            logSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("SendNetworkEventToClient Target={0} Source={1} Type={2}", targetClientId, clientId, eventType).c_str());

    m_networkEventBus.SendNetworkEventToClient(ClientElectionMessage,
        { csp::common::ReplicatedValue(eventType), csp::common::ReplicatedValue(clientId), csp::common::ReplicatedValue(messageId) }, targetClientId,
        signalRCallback);
}

void ClientProxy::SendRemoteRunScriptEvent(int64_t targetClientId, int64_t contextId, const csp::common::String& scriptText)
{

    const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [&logSystem = this->m_logSystem](ErrorCode error)
    {
        if (error != ErrorCode::None)
        {
            logSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("SendRemoteRunScriptEvent Target={0} ContextId={1} Script='{2}'", targetClientId, contextId, scriptText).c_str());

    m_networkEventBus.SendNetworkEventToClient(RemoteRunScriptMessage,
        { csp::common::ReplicatedValue(contextId), csp::common::ReplicatedValue(scriptText) }, targetClientId, signalRCallback);
}

void ClientProxy::HandleElectionEvent(int64_t clientId)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("ClientProxy::HandleElectionEvent ClientId={}", clientId).c_str());

    // We only need to send a response if we are being elected by a lower id
    if (clientId < m_id)
    {
        SendElectionResponseEvent(clientId);
    }
}

void ClientProxy::HandleElectionResponseEvent(int64_t clientId)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientProxy::HandleElectionResponseEvent ClientId={0} Pending={1}", clientId, m_pendingElections.load()).c_str());

    if (clientId > m_id)
    {
        --m_pendingElections;

        if (clientId > m_highestResponseId)
        {
            // Remember the highest ClientId from all the responses
            // We then use this below to check this matches the elected leader
            // as (currently) we use the highest ClientId as our election criteria
            m_highestResponseId = clientId;
        }

        // All done
        if (m_electionManagerPtr && m_pendingElections == 0)
        {
            m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent All expected reponses received");

            // We should have received a valid leader event by now so check this is as expected
            if (m_electionManagerPtr)
            {
                if (m_electionManagerPtr->m_leader)
                {
                    if (m_electionManagerPtr->m_leader->GetId() == m_highestResponseId)
                    {
                        m_logSystem.LogMsg(
                            csp::common::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent Highest response matches elected leader");
                    }
                    else
                    {
                        m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
                            "ClientProxy::HandleElectionResponseEvent Highest response Id does not match elected leader");
                    }
                }
                else
                {
                    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent Expected a valid leader by now!");
                }
            }
        }
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ClientProxy::HandleElectionResponseEvent - Response from lower Id ({}/{})", clientId, m_id).c_str());
    }

    // @Todo :- Handle response timeout (but this shouldn't happen with SignalR/TCP reliable connection)
}

void ClientProxy::HandleElectionLeaderEvent(int64_t leaderId)
{
    m_logSystem.LogMsg(csp::common::LogLevel::Error, fmt::format("ClientProxy::HandleElectionLeaderEvent LeaderId={}", leaderId).c_str());

    m_state = ClientElectionState::Idle;

    if (m_electionManagerPtr)
    {
        // Election complete so set leader client
        m_electionManagerPtr->OnElectionComplete(leaderId);
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
    }
}

void ClientProxy::HandleElectionNotifyLeaderEvent(int64_t clientId)
{
    m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("ClientProxy::HandleElectionNotifyLeaderEvent ClientId={}", clientId).c_str());

    if (m_electionManagerPtr)
    {
        m_electionManagerPtr->OnLeaderNotification(clientId);
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error, "ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
    }
}

} // namespace csp::multiplayer
