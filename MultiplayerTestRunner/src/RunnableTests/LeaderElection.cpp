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

    bool ElectedScopeLeaderCallbackCalled = false;
    std::shared_ptr<bool> First { new bool { true } };

    RealtimeEngine.GetLeaderElection().SetOnElectedScopeLeaderCallback(
        [&ElectedScopeLeaderCallbackCalled, UserId, &MultiplayerSystem, &SpaceSystem, ScopeId, &RealtimeEngine, First](
            const csp::common::String&, const csp::common::String& LeaderUserId)
        {
            std::cout << "SetOnElectedScopeLeaderCallback called for client: " << UserId << std::endl;

            if (LeaderUserId == UserId)
            {
                // std::cout << "New leader: " << LeaderUserId << std::endl;
                ElectedScopeLeaderCallbackCalled = true;

                // std::this_thread::sleep_for(std::chrono::seconds { 3 });

                // RealtimeEngine.GetLeaderElection().TryHeartbeat(ScopeId);

                std::this_thread::sleep_for(std::chrono::seconds { 5 });

                std::cout << "First = " << *First << std::endl;

                if (*First)
                {

                    std::cout << "Performing leader election again, excluding the first user and itself: " << UserId << std::endl;
                    MultiplayerSystem.PerformLeaderElectionInScope(
                        ScopeId, { { SpaceSystem.GetCurrentSpace().OwnerId, UserId } }, [](const csp::systems::NullResult&) {});

                    std::this_thread::sleep_for(std::chrono::seconds { 1 });
                }
                else
                {
                    std::cout << "Should be called for second client, exeting space: " << UserId << std::endl;
                }

                SpaceSystem.ExitSpace(
                    [](const csp::systems::NullResult&) {

                    });
            }
            else
            {
                // std::cout << "Not leader: " << std::endl;
                //  FirstClient = false;
            }

            *First = false;
        });
}
} // namespace LeaderElection