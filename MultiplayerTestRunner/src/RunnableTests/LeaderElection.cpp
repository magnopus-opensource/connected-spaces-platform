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

#include "LeaderElection.h"

#include <CSP/Multiplayer/LeaderElection.h>
#include <CSP/Multiplayer/OnlineRealtimeEngine.h>
#include <CSP/Systems/Multiplayer/MultiplayerSystem.h>
#include <CSP/Systems/Spaces/SpaceSystem.h>
#include <CSP/Systems/SystemsManager.h>
#include <CSP/Systems/Users/UserSystem.h>

#include <future>
#include <iostream>

namespace LeaderElection
{

void RunTest(csp::multiplayer::OnlineRealtimeEngine& RealtimeEngine)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& UserSystem = *SystemsManager.GetUserSystem();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();
    auto& MultiplayerSystem = *SystemsManager.GetMultiplayerSystem();

    csp::common::String SpaceId = SpaceSystem.GetCurrentSpace().Id;
    csp::common::String UserId = UserSystem.GetLoginState().UserId;

    std::cout << "User id: " << UserId << std::endl;

    std::promise<csp::systems::ScopesResult> GetScopesBySpacePromise;
    std::future<csp::systems::ScopesResult> GetScopesBySpaceFuture = GetScopesBySpacePromise.get_future();

    MultiplayerSystem.GetScopesBySpace(SpaceId,
        [&GetScopesBySpacePromise](const csp::systems::ScopesResult& Result)
        {
            if (Result.GetResultCode() != csp::systems::EResultCode::InProgress)
            {
                GetScopesBySpacePromise.set_value(Result);
            }
        });

    csp::systems::ScopesResult Result = GetScopesBySpaceFuture.get();

    if (Result.GetScopes().Size() != 1)
    {
        std::cout << "fail" << std::endl;
        throw std::runtime_error("Invalid number of scopes");
    }

    std::cout << "1" << std::endl;

    csp::common::String ScopeId = Result.GetScopes()[0].Id;

    bool FirstClient = true;
    bool ElectedScopeLeaderCallbackCalled = false;

    RealtimeEngine.GetLeaderElection().SetOnElectedScopeLeaderCallback(
        [&ElectedScopeLeaderCallbackCalled, UserId, &FirstClient](const csp::common::String&, const csp::common::String& LeaderUserId)
        {
            if (LeaderUserId == UserId)
            {
                std::cout << "New leader: " << LeaderUserId << std::endl;
                ElectedScopeLeaderCallbackCalled = true;
            }
            else
            {
                FirstClient = false;
            }
        });

    while (ElectedScopeLeaderCallbackCalled == false)
    {
        RealtimeEngine.GetLeaderElection().TryHeartbeat(ScopeId);
        std::this_thread::sleep_for(std::chrono::milliseconds { 50 });
    }

    std::cout << "2" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds { 5 });

    /*
        This exists to ensure our test assigns the correct leader.
        The test spawns 2 additional clients, if we are the first to get leadership, we want to manually call PerformLeaderElectionInScope,
        so we can exclude the main test client, as we want it to go to the second spawned client.
        When the second client gets leadership, we dont need to call this, as we want to test that when the client stops sending heartbeats,
        it automatically performs leader election.
    */
    if (FirstClient)
    {
        // MultiplayerSystem.PerformLeaderElectionInScope(
        // ScopeId, { { SpaceSystem.GetCurrentSpace().OwnerId } }, [](const csp::systems::NullResult&) {});

        SpaceSystem.ExitSpace(
            [](const csp::systems::NullResult&)
            {
                {
                }
            });
    }
}
} // namespace LeaderElection