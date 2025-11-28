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

#include "LeaderElectionTick.h"

#include <CSP/CSPFoundation.h>
#include <CSP/Multiplayer/OnlineRealtimeEngine.h>
#include <CSP/Systems/Multiplayer/MultiplayerSystem.h>
#include <CSP/Systems/Spaces/SpaceSystem.h>
#include <CSP/Systems/SystemsManager.h>
#include <CSP/Systems/Users/UserSystem.h>

#include <future>
#include <iostream>

namespace LeaderElectionTick
{

void RunTest(csp::multiplayer::OnlineRealtimeEngine& RealtimeEngine)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& UserSystem = *SystemsManager.GetUserSystem();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();
    auto& MultiplayerSystem = *SystemsManager.GetMultiplayerSystem();

    csp::common::String SpaceId = SpaceSystem.GetCurrentSpace().Id;
    csp::common::String UserId = UserSystem.GetLoginState().UserId;

    std::cout << "Client listening: " << UserId << std::endl;

    // First get the scope id.
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
        std::cout << "Fail: Invalid scope size" << std::endl;
        throw std::runtime_error("Invalid number of scopes");
    }

    csp::common::String ScopeId = Result.GetScopes()[0].Id;

    RealtimeEngine.SetOnElectedScopeLeaderCallback(
        [UserId, &MultiplayerSystem, &SpaceSystem, ScopeId, &RealtimeEngine](const csp::common::String&, const csp::common::String& LeaderUserId)
        {
            std::cout << "SetOnElectedScopeLeaderCallback called for client: " << UserId << std::endl;

            if (LeaderUserId == UserId)
            {
                std::cout << "New leader: " << LeaderUserId << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds { 5 });

                // Call the heartbeat
                csp::CSPFoundation::Tick();

                // Wait a few seconds before removing ownership.
                std::this_thread::sleep_for(std::chrono::seconds { 5 });

                // We are the second client to get leadership, so just exit the space and the main client will wait for leadership to go back to
                // them.
                std::cout << "Should be called for second client, exiting space: " << UserId << std::endl;

                SpaceSystem.ExitSpace(
                    [](const csp::systems::NullResult&) {

                    });
            }
        });
}
} // namespace LeaderElection