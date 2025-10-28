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
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "CSP/Systems/SystemsManager.h"

#include "MultiplayerTestRunnerProcess.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "TestHelpers.h"
#include "gtest/gtest.h"

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}
/*
    This test verifies that the rest and multiplayer hub api can be used to:
        - Set a scope to have managed leader election
        - A client that sends a heartbeat can become the elected leader
        - The elected leader can be retrieved
        - The OnElectedScopeLeaderCallback is fired correctly.
*/
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, GetScopeLeaderTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* MultiplayerSystem = SystemsManager.GetMultiplayerSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create OnlineRealtimeEngine
    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String ScopeId;

    // Bind to the OnElectedScopeLeaderCallback.
    bool CallbackCalled = false;
    RealtimeEngine->GetLeaderElection().SetOnElectedScopeLeaderCallback(
        [&CallbackCalled, UserId, &ScopeId](const csp::common::String& ChangedScopeId, const csp::common::String& LeaderUserId)
        {
            EXPECT_EQ(LeaderUserId, UserId);
            EXPECT_EQ(ChangedScopeId, ScopeId);
            CallbackCalled = true;
        });

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    csp::systems::Scope DefaultScope = GetScopesResult.GetScopes()[0];
    ScopeId = DefaultScope.Id;

    // Update the scope to enable managed leader election.
    DefaultScope.ManagedLeaderElection = true;
    auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, ScopeId, DefaultScope);
    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(UpdateScopeResult.GetScope().ManagedLeaderElection, true);

    // Ensure we send a heartbeat so the leader election knows we are available.
    RealtimeEngine->GetLeaderElection().TryHeartbeat(ScopeId);

    // Wait for the OnElectedScopeLeaderCallback to be called
    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    uint64_t TestTime = 0;
    uint64_t MaxTextTimeSeconds = 20;

    while (CallbackCalled == false && TestTime < MaxTextTimeSeconds)
    {
        RealtimeEngine->GetLeaderElection().TryHeartbeat(ScopeId);

        std::this_thread::sleep_for(50ms);

        Current = std::chrono::steady_clock::now();
        TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
    }
    EXPECT_TRUE(CallbackCalled);

    // Get the scope leader
    auto [GetScopeLeaderResult] = AWAIT_PRE(MultiplayerSystem, GetScopeLeader, RequestPredicate, ScopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Ensure we are the scope leader.
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ScopeId, ScopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ScopeLeaderUserId, UserId);
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ElectionInProgress, false);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

// TODO:
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, AssumeScopeLeaderTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* MultiplayerSystem = SystemsManager.GetMultiplayerSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create OnlineRealtimeEngine
    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String ScopeId;

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    csp::systems::Scope DefaultScope = GetScopesResult.GetScopes()[0];
    ScopeId = DefaultScope.Id;

    // Update the scope to enable managed leader election.
    DefaultScope.ManagedLeaderElection = true;
    auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, ScopeId, DefaultScope);
    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(UpdateScopeResult.GetScope().ManagedLeaderElection, true);

    // Bind to the OnElectedScopeLeaderCallback.

    bool OnElectedScopeLeaderCallbackCalled = false;
    RealtimeEngine->GetLeaderElection().SetOnElectedScopeLeaderCallback(
        [&OnElectedScopeLeaderCallbackCalled, UserId, &ScopeId](const csp::common::String& ChangedScopeId, const csp::common::String& LeaderUserId)
        {
            EXPECT_EQ(LeaderUserId, UserId);
            EXPECT_EQ(ChangedScopeId, ScopeId);
            OnElectedScopeLeaderCallbackCalled = true;
        });

    // Set this client to the leader.
    {
        bool CallbackCalled = false;
        RealtimeEngine->GetLeaderElection().AssumeScopeLeadership(ScopeId,
            [&CallbackCalled](bool Success)
            {
                EXPECT_TRUE(Success);
                CallbackCalled = true;
            });

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);
    }

    // Ensure this client is set as the leader.
    WaitForCallback(OnElectedScopeLeaderCallbackCalled);
    EXPECT_TRUE(OnElectedScopeLeaderCallbackCalled);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    This test uses the MultiplayerTestRunner to spawn 2 additional clients to test that the scope leader is correctly assigned.
    The test does the following steps:
    - Sets up a space with managed leader election
    - Sets the client to the leader
    - Spawns 2 more clients
    - Performs leader eleciton excluding the current user, so one of the other 2 clients gets it
    - When the first MultiplayerTestRunner client gets leadership, they well perform another leader election, excluding themselves and the main client
    - This will give leadership to the other MultiplayerTestRunner client, which will then disconnect
    - This will finally trigger another leader election, giving leadership back to the main client.
*/
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, ScopeLeadershipTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* MultiplayerSystem = SystemsManager.GetMultiplayerSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create users for test runner
    auto TestRunnerUser1 = CreateTestUser();
    auto TestRunnerUser2 = CreateTestUser();

    csp::systems::InviteUserRoleInfo InviteUser1;
    InviteUser1.UserEmail = TestRunnerUser1.Email;
    InviteUser1.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfo InviteUser2;
    InviteUser2.UserEmail = TestRunnerUser2.Email;
    InviteUser2.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser1, InviteUser2 };

    // Create space
    csp::systems::Space Space;
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, InviteUsers, nullptr, nullptr, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // First, we need to get the default scope
    csp::systems::Scope DefaultScope;
    csp::common::String ScopeId;

    {
        auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
        EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

        if (GetScopesResult.GetScopes().Size() != 1)
        {
            FAIL();
        }

        DefaultScope = GetScopesResult.GetScopes()[0];
        ScopeId = DefaultScope.Id;
    }

    // Exit space first, update the scope and then reenter.

    // We need to ensure the scope has ManagedLeaderElection enabled.
    {
        DefaultScope.ManagedLeaderElection = true;
        auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, ScopeId, DefaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get leadership
    {
        bool ElectedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, UserId](const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                EXPECT_EQ(LeaderUserId, UserId);
                ElectedCallbackCalled = true;
            });

        RealtimeEngine->GetLeaderElection().AssumeScopeLeadership(ScopeId, [](bool) {});
        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);
    }

    //  Create 2 additional clients for leader election.
    MultiplayerTestRunnerProcess TestRunner1 = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION)
                                                   .SetSpaceId(Space.Id.c_str())
                                                   .SetLoginEmail(TestRunnerUser1.Email.c_str())
                                                   .SetPassword(GeneratedTestAccountPassword)
                                                   .SetEndpoint(EndpointBaseURI())
                                                   .SetTimeoutInSeconds(60);

    MultiplayerTestRunnerProcess TestRunner2 = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION)
                                                   .SetSpaceId(Space.Id.c_str())
                                                   .SetLoginEmail(TestRunnerUser2.Email.c_str())
                                                   .SetPassword(GeneratedTestAccountPassword)
                                                   .SetEndpoint(EndpointBaseURI())
                                                   .SetTimeoutInSeconds(60);

    std::cout << "main user id: " << UserId << "----------------" << std::endl;

    csp::common::String ElectedUserId;

    // Next, we want to remove ourselves as leader and give it to one of the test runners.
    {
        // First listen to the vacated callback to ensure we are removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, UserId, ScopeId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                std::cout << "OnVacatedAsScopeLeaderCallback--------------------------\n" << std::endl;
                // std::cout << ThisScopeId << "ScopeId" << std::endl;
                EXPECT_EQ(UserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to one of the other clients.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, TestRunnerUser1, TestRunnerUser2, &ElectedUserId](
                const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback--------------------------\n" << std::endl;
                EXPECT_TRUE((TestRunnerUser1.UserId == LeaderUserId) || (TestRunnerUser2.UserId == LeaderUserId));
                ElectedUserId = LeaderUserId;
                ElectedCallbackCalled = true;
            });

        TestRunner1.StartProcess();
        TestRunner2.StartProcess();

        std::future<void> ReadyForAssertionsFuture1 = TestRunner1.ReadyForAssertionsFuture();
        std::future<void> ReadyForAssertionsFuture2 = TestRunner2.ReadyForAssertionsFuture();

        while (ReadyForAssertionsFuture1.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            RealtimeEngine->GetLeaderElection().TryHeartbeat(ScopeId);
        }

        while (ReadyForAssertionsFuture2.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            RealtimeEngine->GetLeaderElection().TryHeartbeat(ScopeId);
        }

        printf("Starting First Test\n");

        std::this_thread::sleep_for(1000ms);

        // Restart leader election, exluding this user.
        auto [LeaderElectionResult] = AWAIT_PRE(MultiplayerSystem, PerformLeaderElectionInScope, RequestPredicate, ScopeId, { { UserId } });
        EXPECT_EQ(LeaderElectionResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto Start = std::chrono::steady_clock::now();
        auto Current = std::chrono::steady_clock::now();
        uint64_t TestTime = 0;
        uint64_t MaxTestTimeSeconds = 30;

        // Wait for this client to be vacated as leader and a new client to be elected.
        while (((VacatedCallbackCalled && ElectedCallbackCalled) == false) && TestTime < MaxTestTimeSeconds)
        {
            std::this_thread::sleep_for(50ms);

            if (VacatedCallbackCalled == false)
            {
                // RealtimeEngine->GetLeaderElection().TryHeartbeat(ScopeId);
            }

            Current = std::chrono::steady_clock::now();
            TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        EXPECT_TRUE(VacatedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);
    }

    printf("Finished First Test\n");

    // The client that becomes the leader will then exit the space, which will cause another election which will exclude this client.
    // We need to ensure that the leadership election is triggered and the remaining client is elected.
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, ElectedUserId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                EXPECT_EQ(ElectedUserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to the remaining client.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, TestRunnerUser1, TestRunnerUser2, &ElectedUserId](
                const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                // If the previous leader was User1, the new leader should be User2
                if (ElectedUserId == TestRunnerUser1.UserId)
                {
                    EXPECT_EQ(LeaderUserId, TestRunnerUser2.UserId);
                }
                else
                {
                    EXPECT_EQ(LeaderUserId, TestRunnerUser1.UserId);
                }

                ElectedUserId = LeaderUserId;
                ElectedCallbackCalled = true;
            });

        printf("Starting Second Test\n");

        WaitForCallback(VacatedCallbackCalled);
        EXPECT_TRUE(VacatedCallbackCalled);

        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);

        printf("Finishing Second Test\n");
    }

    // The final client will now disconnect and give leadership back to this client
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, ElectedUserId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                EXPECT_EQ(ElectedUserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to this client.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->GetLeaderElection().SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, UserId](const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                EXPECT_EQ(UserId, LeaderUserId);
                ElectedCallbackCalled = true;
            });

        printf("Starting Third Test\n");

        WaitForCallback(VacatedCallbackCalled);
        EXPECT_TRUE(VacatedCallbackCalled);

        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);

        printf("Finishing Third Test\n");
    }

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}