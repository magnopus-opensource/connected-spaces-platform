/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/Common/String.h"

#include <chrono>
#include <functional>
#include <optional>
#include <unordered_map>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{
class MultiplayerConnection;

// TODO:
class ScopeLeadershipManager
{
public:
    ScopeLeadershipManager(MultiplayerConnection& Connection, csp::common::LogSystem& LogSystem);

    // Adds a scope to the ScopeLeadershipManager to track events for.
    // Scopes should be registered on space entry and when new scopes are created.
    // If a scope is accessed for a scope that hasn't been registered, an error will be logged to the log system.
    void RegisterScope(const std::string& ScopeId, const std::optional<uint64_t>& LeaderId);

    void DeregisterScope(const std::string& ScopeId);

    // Called when we receive a leader election event for a scope.
    // This will happen after a server-side election is completed.
    void OnElectedScopeLeader(const std::string& ScopeId, uint64_t ClientId);
    // Called when we receive a leader vacated event.
    // This will happen if an election is manually triggered for a scope that already has a leader,
    // or the current leader becomes unavailable (heartbeat not sent within a time, or the client disconnects".
    void OnVacatedAsScopeLeader(const std::string& ScopeId, uint64_t ClientId);

    // Loop Scopes and calls SendLeaderHeartbeat if the local client is the leader of the scope
    // and LeaderElectionHeartbeatInterval has passed since the last heartbeat.
    void Update();

    // Returns 0 if not valid
    std::optional<uint64_t> GetLeaderClientId(const std::string& ScopeId) const;
    bool IsLocalClientLeader(const std::string& ScopeId) const;

private:
    // Notifies the server that the leader of the given scope is still available.
    // If x time has passed since the last heartbeat, a re-election will happen server-side.
    void SendLeaderHeartbeat(const std::string& ScopeId);

    MultiplayerConnection& Connection;
    csp::common::LogSystem& LogSystem;

    struct ScopeLeaderData
    {
        uint64_t LeaderClientId;

        CSP_START_IGNORE
        std::chrono::steady_clock::time_point LastHeartbeatTime;
        CSP_END_IGNORE
    };

    // Used for getting leader data about each registered scope.
    // Key is the Scope id.
    // std::nullopt represents a scope which doesn't have a leader, meaning an election is currently in progress.
    std::unordered_map<std::string, std::optional<ScopeLeaderData>> Scopes;
};

CSP_START_IGNORE
constexpr const std::chrono::seconds LeaderElectionHeartbeatInterval { 3 };
CSP_END_IGNORE
}
