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

#include "CSP/Systems/Script/ScriptSystem.h"
#include "Debug/Logging.h"
#include "Multiplayer/Election/ClientElectionManager.h"

namespace csp::multiplayer
{

constexpr const uint64_t ALL_CLIENTS_ID = -1;

ClientProxy::ClientProxy(ClientId Id, ClientElectionManager* ElectionManager)
	: ElectionManagerPtr(ElectionManager)
	, State(ClientElectionState::Idle)
	, Id(Id)
	, Eid(0)
	, PendingElections(0)
	, CancellationToken(nullptr)
	, IsLeaderValid(true)
	, LastHeartBeatTime(std::chrono::steady_clock::now())
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

ClientId ClientProxy::GetId() const
{
	return Id;
}

void ClientProxy::StartLeaderElection(const ClientMap& Clients)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::StartLeaderElection ClientId=%d State=%d", Id, State);

	if (State != ClientElectionState::Idle)
	{
		FOUNDATION_LOG_ERROR_MSG("ClientProxy::StartLeaderElection called when election already in progress");
		return;
	}

	State			  = ClientElectionState::Electing;
	ElectionStartTime = std::chrono::system_clock::now();
	PendingElections  = 0;

	if (IsThisClientLeader(Clients))
	{
		FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "This Client (%d) is Leader", Id);

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
			FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "Calling OnElectionComplete Pending=%d", PendingElections.load());
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
		case ClientElectionMessageType::LeaderHeartbeat:
			HandleLeaderHeartbeatEvent(ClientId);
			break;
		case ClientElectionMessageType::LeaderLost:
			HandleLeaderLostEvent(ClientId);
			break;
	}
}

void ClientProxy::NotifyLeader(int64_t TargetClientId, int64_t LeaderClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose,
						  "ClientProxy::NotifyLeader Target=%d Source=%d Leader=%d",
						  TargetClientId,
						  Id,
						  LeaderClientId);
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

void ClientProxy::HandleLeaderState()
{
	// Nothing to do currently
}

void ClientProxy::UpdateLeaderHeartbeat()
{
	if (State == ClientElectionState::Leader)
	{
		auto TimeNow = std::chrono::steady_clock::now();

		// How long since we last sent a heartbeat message?
		auto Elapsed = TimeNow - LastHeartBeatTime;

		if (Elapsed > LeaderHeartbeatPeriod)
		{
			LastHeartBeatTime = TimeNow;
			SendLeaderHeartbeat();
		}
	}
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

void ClientProxy::SendLeaderHeartbeat()
{
	FOUNDATION_LOG_MSG(csp::systems::LogLevel::VeryVerbose, " *** ClientProxy::SendLeaderHeartbeat ***");

	// Broadcast 'Heartbeat' message to all clients
	SendEvent(ALL_CLIENTS_ID, static_cast<int64_t>(ClientElectionMessageType::LeaderHeartbeat), Id);
}

void ClientProxy::SendLeaderLost()
{
	// Broadcast 'Leader lost' message to all clients
	SendEvent(ALL_CLIENTS_ID, static_cast<int64_t>(ClientElectionMessageType::LeaderLost), Id);
}

void ClientProxy::SetAsLeader(bool IsLeader)
{
	if (IsLeader)
	{
		IsLeaderValid = true;
		State		  = ClientElectionState::Leader;
	}
	else
	{
		State = ClientElectionState::Idle;
	}
}


bool ClientProxy::IsLeaderStillValid() const
{
	return IsLeaderValid;
}

void ClientProxy::SendEvent(int64_t TargetClientId, int64_t EventType, int64_t ClientId)
{
	SpaceEntitySystem* EntitySystem	  = ElectionManagerPtr->GetSpaceEntitySystem();
	MultiplayerConnection* Connection = EntitySystem->GetMultiplayerConnection();

	const int64_t MessageId = Eid++;

	const csp::multiplayer::MultiplayerConnection::CallbackHandler SignalRCallback = [=](const bool& SignalRCallbackResult)
	{
		if (!SignalRCallbackResult)
		{
			FOUNDATION_LOG_ERROR_MSG("ClientProxy::SendEvent: SignalR connection: Error");
		}
	};

	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose,
						  "SendNetworkEventToClient Target=%d Source=%d Type=%d",
						  TargetClientId,
						  ClientId,
						  EventType);

	Connection->SendNetworkEventToClient(ClientElectionMessage,
										 {ReplicatedValue(EventType), ReplicatedValue(ClientId), ReplicatedValue(MessageId)},
										 TargetClientId,
										 SignalRCallback);
}

void ClientProxy::SendRemoteRunScriptEvent(int64_t TargetClientId, int64_t ContextId, const csp::common::String& ScriptText)
{
	SpaceEntitySystem* EntitySystem	  = ElectionManagerPtr->GetSpaceEntitySystem();
	MultiplayerConnection* Connection = EntitySystem->GetMultiplayerConnection();

	const csp::multiplayer::MultiplayerConnection::CallbackHandler SignalRCallback = [=](const bool& SignalRCallbackResult)
	{
		if (!SignalRCallbackResult)
		{
			FOUNDATION_LOG_ERROR_MSG("ClientProxy::SendEvent: SignalR connection: Error");
		}
	};

	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose,
						  "SendRemoteRunScriptEvent Target=%lld ContextId=%lld Script='%s'",
						  TargetClientId,
						  ContextId,
						  ScriptText.c_str());

	Connection->SendNetworkEventToClient(RemoteRunScriptMessage,
										 {ReplicatedValue(ContextId), ReplicatedValue(ScriptText)},
										 TargetClientId,
										 SignalRCallback);
}

void ClientProxy::HandleElectionEvent(int64_t ClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionEvent ClientId=%d", ClientId);

	// We only need to send a response if we are being elected by a lower id
	if (ClientId < Id)
	{
		SendElectionResponseEvent(ClientId);
	}
}

void ClientProxy::HandleElectionResponseEvent(int64_t ClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose,
						  "ClientProxy::HandleElectionResponseEvent ClientId=%d Pending=%d",
						  ClientId,
						  PendingElections.load());

	if (ClientId > Id)
	{
		--PendingElections;

		// All done
		if (ElectionManagerPtr && PendingElections == 0)
		{
			FOUNDATION_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionResponseEvent All expected reponses received");
			State = ClientElectionState::Idle;
			ElectionManagerPtr->OnElectionComplete(ClientId);
		}
	}
	else
	{
		FOUNDATION_LOG_ERROR_FORMAT("ClientProxy::HandleElectionResponseEvent - Response from lower Id (%d/%d)", ClientId, Id);
	}

	// @Todo :- Handle response timeout (but this shouldn't happen with SignalR/TCP reliable connection)
}

void ClientProxy::HandleElectionLeaderEvent(int64_t ClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionLeaderEvent ClientId=%d", ClientId);

	State		  = ClientElectionState::Idle;
	IsLeaderValid = true;

	if (ElectionManagerPtr)
	{
		ElectionManagerPtr->OnElectionComplete(ClientId);
	}
	else
	{
		FOUNDATION_LOG_ERROR_MSG("ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
	}
}

void ClientProxy::HandleElectionNotifyLeaderEvent(int64_t ClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleElectionNotifyLeaderEvent ClientId=%d", ClientId);

	if (ElectionManagerPtr)
	{
		ElectionManagerPtr->OnLeaderNotification(ClientId);
	}
	else
	{
		FOUNDATION_LOG_ERROR_MSG("ClientProxy::HandleElectionLeaderEvent - Null election manager pointer");
	}
}

void ClientProxy::HandleLeaderHeartbeatEvent(int64_t ClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleLeaderHeartbeatEvent ClientId=%d", ClientId);

	if (ElectionManagerPtr)
	{
		ElectionManagerPtr->OnLeaderHeartbeat(ClientId);
	}
	else
	{
		FOUNDATION_LOG_ERROR_MSG("ClientProxy::HandleLeaderHeartbeatEvent - Null election manager pointer");
	}
}

void ClientProxy::HandleLeaderLostEvent(int64_t ClientId)
{
	FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "ClientProxy::HandleLeaderLost ClientId=%d", ClientId);

	if (ClientId == Id)
	{
		IsLeaderValid = false;
	}
}

} // namespace csp::multiplayer
