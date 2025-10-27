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

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{
class MultiplayerConnection;

/// @brief Object that contains functionality for handling leader election within a space.
/// An instance of this exists on the OnlineRealtimeEngine.
class CSP_API LeaderElection
{
public:
    // @brief Internal constructor - This should not be called, as an instance can be accessed from OnlineRealtimeEngine.
    LeaderElection(MultiplayerConnection& Connection, csp::common::LogSystem& LogSystem);

    typedef std::function<void(bool)> AssumeScopeLeaderCallback;

    /// @brief Sets the scope leader for the given scope to this client.
    /// This shouldn't need to be called outside of testing, as leader election happens automatically.
    /// @param ScopeId const csp::common::String& : The scope id to change the leader of.
    /// @param Callback std::function<void(const csp::common::String&)> : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void AssumeScopeLeadership(const csp::common::String& ScopeId, AssumeScopeLeaderCallback Callback);

    /// @brief Send a heartbeat to the leader election system.
    /// This is called internally by CSPFoundation::Tick.
    /// @param ScopeId const csp::common::String& : The scope id to send a heartbeat for.
    bool TryHeartbeat(const csp::common::String& ScopeId);

    typedef std::function<void(const csp::common::String& ScopeId, const csp::common::String& UserId)> ScopeLeaderCallback;

    /// @brief Binds the provided callback to receive events when a new scope leader has been elected.
    /// @param Callback ScopeLeaderCallback : Fired when a new scope leader is elected.
    CSP_EVENT void SetOnElectedScopeLeaderCallback(ScopeLeaderCallback Callback);
    /// @brief Binds the provided callback to receive events when a scope leader has been vacated.
    /// @param Callback ScopeLeaderCallback : Fired when a scope leader is vacated.
    CSP_EVENT void SetOnVacatedAsScopeLeaderCallback(ScopeLeaderCallback Callback);

    CSP_NO_EXPORT const ScopeLeaderCallback& GetOnElectedScopeLeaderCallback();
    CSP_NO_EXPORT const ScopeLeaderCallback& GetOnVacatedAsScopeLeaderCallback();

private:
    MultiplayerConnection& Connection;
    csp::common::LogSystem& LogSystem;

    CSP_START_IGNORE
    std::chrono::steady_clock::time_point LastHeartbeatTime;
    CSP_END_IGNORE

    ScopeLeaderCallback OnElectedScopeLeaderCallback;
    ScopeLeaderCallback OnVacatedAsScopeLeaderCallback;
};
CSP_START_IGNORE
constexpr const std::chrono::seconds LeaderElectionHeartbeatInterval { 3 };
CSP_END_IGNORE
}
