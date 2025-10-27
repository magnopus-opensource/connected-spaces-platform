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
#include "CSP/Multiplayer/LeaderElection.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "Multiplayer/SignalR/ISignalRConnection.h"

#include <fmt/format.h>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

csp::multiplayer::LeaderElection::LeaderElection(MultiplayerConnection& Connection, csp::common::LogSystem& LogSystem)
    : Connection { Connection }
    , LogSystem { LogSystem }
{
}

void LeaderElection::AssumeScopeLeadership(const csp::common::String& ScopeId, AssumeScopeLeaderCallback Callback)
{
    signalr::value Params { std::vector { signalr::value { ScopeId } } };

    Connection.GetSignalRConnection()->Invoke(Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ASSUME_SCOPE_LEADERSHIP), Params,
        [Callback, this](signalr::value Value, std::exception_ptr Exception)
        {
            bool Success = Exception == nullptr;

            try
            {
                if (Exception)
                {
                    std::rethrow_exception(Exception);
                }
            }
            catch (const std::exception& Exception)
            {
                LogSystem.LogMsg(
                    csp::common::LogLevel::Error, fmt::format("LeaderElection::AssumeScopeLeadership Failed: {}", Exception.what()).c_str());
            }
            catch (...)
            {
                LogSystem.LogMsg(csp::common::LogLevel::Error, "LeaderElection::AssumeScopeLeadership Failed");
            }

            Callback(Success);
        });
}

bool LeaderElection::TryHeartbeat(const csp::common::String& ScopeId)
{
    auto CurrentTime = std::chrono::steady_clock::now();

    // Check if enough time has elapsed since the last heartbeat call.
    if (CurrentTime - LastHeartbeatTime > LeaderElectionHeartbeatInterval)
    {
        signalr::value Params { std::vector { signalr::value { ScopeId } } };

        Connection.GetSignalRConnection()->Invoke(Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT),
            Params,
            [this](signalr::value Value, std::exception_ptr Exception)
            {
                // If the response returns an exception, we will log.
                // We currently don't give any information to clients when a callback is fired, as this is called internally.
                // We may want to create another event which fires every time we get a heartbeat result.
                if (Exception)
                {
                    try
                    {
                        std::rethrow_exception(Exception);
                    }
                    catch (const std::exception& Exception)
                    {
                        LogSystem.LogMsg(
                            csp::common::LogLevel::Error, fmt::format("LeaderElection::TryHeartbeat Failed: {}", Exception.what()).c_str());
                    }
                    catch (...)
                    {
                        LogSystem.LogMsg(csp::common::LogLevel::Error, "LeaderElection::TryHeartbeat Failed");
                    }
                }
            });

        LastHeartbeatTime = CurrentTime;

        return true;
    }

    return false;
}

void LeaderElection::SetOnElectedScopeLeaderCallback(ScopeLeaderCallback Callback) { OnElectedScopeLeaderCallback = Callback; }

void LeaderElection::SetOnVacatedAsScopeLeaderCallback(ScopeLeaderCallback Callback) { OnVacatedAsScopeLeaderCallback = Callback; }

const LeaderElection::ScopeLeaderCallback& LeaderElection::GetOnElectedScopeLeaderCallback() { return OnElectedScopeLeaderCallback; }

const LeaderElection::ScopeLeaderCallback& LeaderElection::GetOnVacatedAsScopeLeaderCallback() { return OnVacatedAsScopeLeaderCallback; }

}
