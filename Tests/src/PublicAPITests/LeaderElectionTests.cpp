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

#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Mocks/SignalRConnectionMock.h"
#include "Multiplayer/Election/ScopeLeadershipManager.h"
#include "MultiplayerTestRunnerProcess.h"
#include "RAIIMockLogger.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "TestHelpers.h"
#include "gtest/gtest.h"
#include <fmt/format.h>

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

/*
    This tests that scope registratioon and deregistration work correctly
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, RegisterScopeTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr const char* TestScopeId1 = "TestScopeId1";
    static constexpr const char* TestScopeId2 = "TestScopeId2";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    // Ensure the RegisterScope log is called with the leader.
    {
        RAIIMockLogger MockLogger {};

        // Ensure the RegisterScope log is called.
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with leader: {1}.", TestScopeId1, Connection.GetClientId())
                  .c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Log)).Times(1);

        Manager.RegisterScope(TestScopeId1, Connection.GetClientId());

        // Ensure the leader has been set.
        EXPECT_TRUE(Manager.GetLeaderClientId(TestScopeId1).has_value() && *Manager.GetLeaderClientId(TestScopeId1) == Connection.GetClientId());
    }

    // Ensure the RegisterScope log is called without a leader.
    {
        RAIIMockLogger MockLogger {};

        // Ensure the RegisterScope log is called with no leader.
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with no leader.", TestScopeId2).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Log)).Times(1);

        Manager.RegisterScope(TestScopeId2, std::nullopt);

        // Ensure the leader is invalid.
        EXPECT_FALSE(Manager.GetLeaderClientId(TestScopeId2).has_value());
    }
}

/*
    This tests that the IsLocalClientLeaderTest returns true when a scopes leader matches the local client,
    and false when it doesn't.
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, IsLocalClientLeaderTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr uint64_t TestClientId2 = 2;
    static constexpr const char* TestScopeId1 = "TestScopeId1";
    static constexpr const char* TestScopeId2 = "TestScopeId2";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    Manager.RegisterScope(TestScopeId1, Connection.GetClientId());
    Manager.RegisterScope(TestScopeId2, TestClientId2);

    EXPECT_TRUE(Manager.IsLocalClientLeader(TestScopeId1));
    EXPECT_FALSE(Manager.IsLocalClientLeader(TestScopeId2));
    EXPECT_FALSE(Manager.IsLocalClientLeader("INVALID_SCOPE_ID"));
}

/*
    This tests that errors are correctly generated when a scope hasn't been registered to the ScopeLeadershipManager
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, UnregisteredScopeTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr const char* TestScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    // Test OnElectedScopeLeader fails with the expected error.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Error
            = fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0} that isn't registered, for new "
                          "leader: {1}.",
                TestScopeId, Connection.GetClientId())
                  .c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Error)).Times(1);

        Manager.OnElectedScopeLeader(TestScopeId, Connection.GetClientId());
    }

    // Test OnElectedScopeLeader fails with the expected error.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Error
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {0} that isn't registered.", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Error)).Times(1);

        Manager.OnVacatedAsScopeLeader(TestScopeId);
    }

    // Test GetLeaderClientId fails with the expected error.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Error
            = fmt::format("ScopeLeadershipManager::GetLeaderClientId Event called for the scope: {} that isn't registered.", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Error)).Times(1);

        Manager.GetLeaderClientId(TestScopeId);
    }
}

/*
    This tests that:
    - The heartbeat is called when a leader first gets elected.
    - It then tests it isnt called when called before the LeaderElectionHeartbeatInterval time
    - It then tests it is called again after waiting for LeaderElectionHeartbeatInterval time
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, UpdateTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr const char* TestScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    Manager.RegisterScope(TestScopeId, Connection.GetClientId());

    // Mock the heartbeat function to return a basic result.
    EXPECT_CALL(*SignalRMock,
        Invoke(Connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
            ::testing::_))
        .WillRepeatedly(
            [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> Callback)
                -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            {
                std::vector<signalr::value> Params;
                signalr::value Value(Params);

                Callback(Params, nullptr);
                return async::make_task(std::make_tuple(Value, std::exception_ptr(nullptr)));
            });

    // Ensure the callback is called when we update.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, Log)).Times(1);

        Manager.SendHeartbeatIfElectedScopeLeader();
    }

    // We haven't waited long enough, so the callback shouldnt be called if we call Update again.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, Log)).Times(0);

        Manager.SendHeartbeatIfElectedScopeLeader();
    }

    // Wait for the heartbeat interval before calling again. This should successfully call the callback again.
    {
        std::this_thread::sleep_for(csp::multiplayer::LeaderElectionHeartbeatInterval);

        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, Log)).Times(1);

        Manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    This test ensures that the heartbeat isnt called for scopes that the local client isn't a leader of.
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, NonLeaderUpdateTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr uint64_t TestClientId2 = 2;
    static constexpr const char* TestScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    Manager.RegisterScope(TestScopeId, TestClientId2);

    // Mock the heartbeat function to return a basic result.
    EXPECT_CALL(*SignalRMock,
        Invoke(Connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
            ::testing::_))
        .WillRepeatedly(
            [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> Callback)
                -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            {
                std::vector<signalr::value> Params;
                signalr::value Value(Params);

                Callback(Params, nullptr);
                return async::make_task(std::make_tuple(Value, std::exception_ptr(nullptr)));
            });

    // Ensure the callback is called when we update.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, Log)).Times(0);

        Manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    Tests that the correct logs are called when an std::exception is received from the scope leader heartbeat endpoint
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, HeartbeatStdExceptionTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr const char* TestScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    Manager.RegisterScope(TestScopeId, Connection.GetClientId());

    // Mock the heartbeat function to return a std::exception.
    {
        EXPECT_CALL(*SignalRMock,
            Invoke(Connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
                ::testing::_))
            .WillOnce(
                [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> Callback)
                    -> async::task<std::tuple<signalr::value, std::exception_ptr>>
                {
                    std::vector<signalr::value> Params;
                    signalr::value Value(Params);

                    auto Exception = std::make_exception_ptr(std::runtime_error("Test Exception"));

                    Callback(Params, Exception);
                    return async::make_task(std::make_tuple(Value, Exception));
                });

        RAIIMockLogger MockLogger {};
        const csp::common::String Log = fmt::format(
            "ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {0} with error: {1}", TestScopeId, "Test Exception")
                                            .c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Log)).Times(1);

        Manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    Tests that the correct logs are called when a non std::exception is received from the scope leader heartbeat endpoint
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, HeartbeatNonStdExceptionTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr const char* TestScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    Manager.RegisterScope(TestScopeId, Connection.GetClientId());

    // Mock the heartbeat function to return a non st::exception
    {
        EXPECT_CALL(*SignalRMock,
            Invoke(Connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
                ::testing::_))
            .WillOnce(
                [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> Callback)
                    -> async::task<std::tuple<signalr::value, std::exception_ptr>>
                {
                    std::vector<signalr::value> Params;
                    signalr::value Value(Params);

                    // Store a random type in the exception ptr to test catch(...)
                    auto Exception = std::make_exception_ptr(std::string {});

                    Callback(Params, Exception);
                    return async::make_task(std::make_tuple(Value, Exception));
                });

        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {} with an unknown error.", TestScopeId)
                  .c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Log)).Times(1);

        Manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    This tests that errors are correctly generated when an election event is called with unexpected data.
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, InvalidEventTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& Connection = *SystemsManager.GetMultiplayerConnection();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    static constexpr const uint64_t TestClientId2 = 2;

    static constexpr const char* TestScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager Manager { Connection, LogSystem };

    // Register 2 test scopes.
    Manager.RegisterScope(TestScopeId, Connection.GetClientId());

    // Test OnElectedScopeLeader throws a warning and log when the election event is called for a scope that already has a leader.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Error = fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0}, that already has "
                                                      "the leader: {1}, for new leader: {2}. Overwriting old value.",
            TestScopeId, Connection.GetClientId(), TestClientId2)
                                              .c_str();

        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::OnElectedScopeLeader New leader: {0}, for scope: {1}.", TestClientId2, TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, Error)).Times(1);
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Log)).Times(1);

        // Call OnElectedScopeLeader with the second client as leader without vacating first.
        Manager.OnElectedScopeLeader(TestScopeId, TestClientId2);

        // Ensure the leader is still updated to the new value.
        EXPECT_TRUE(Manager.GetLeaderClientId(TestScopeId).has_value() && *Manager.GetLeaderClientId(TestScopeId) == TestClientId2);
    }

    // Test OnVacatedAsScopeLeader throws a warning when the vacated event is called for a scope that doesn't have a leader.
    {
        // First vacate the scope leader.
        Manager.OnVacatedAsScopeLeader(TestScopeId);

        RAIIMockLogger MockLogger {};
        const csp::common::String Error
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for the scope: {0} that doesn't have a leader.", TestScopeId)
                  .c_str();

        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {}.", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, Error)).Times(1);
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Log)).Times(1);

        // Call OnVacatedAsScopeLeader again with the first client for a scope that hasn't got a leader.
        Manager.OnVacatedAsScopeLeader(TestScopeId);

        // Ensure the leader is still vacated.
        EXPECT_FALSE(Manager.GetLeaderClientId(TestScopeId).has_value());
    }
}

/*
    This test verifies that the rest and multiplayer hub api can be used to:
        - Set a scope to have managed leader election
        - Re-enter the space
        - The elected leader can be retrieved
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
    RealtimeEngine->SetOnElectedScopeLeaderCallback(
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

    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Enter space
    auto [ReEnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    // Get the scope leader
    auto [GetScopeLeaderResult] = AWAIT_PRE(MultiplayerSystem, GetScopeLeader, RequestPredicate, ScopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Ensure we are the scope leader.
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ScopeId, ScopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ScopeLeaderUserId, UserId);
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ElectionInProgress, false);

    // Clean up
    auto [ExitResult2] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
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
    csp::systems::Profile NewTestUser = CreateTestUser();

    csp::common::String UserId;
    LogIn(UserSystem, UserId, NewTestUser.Email, GeneratedTestAccountPassword);

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
    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

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

    // We need to ensure the scope has ManagedLeaderElection enabled.
    {
        DefaultScope.ManagedLeaderElection = true;
        auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, ScopeId, DefaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Reenter space to stop leader election
    {
        auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Enter space
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
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

    csp::common::String ElectedUserId;

    // Next, we want to remove ourselves as leader and give it to one of the test runners.
    {
        // First listen to the vacated callback to ensure we are removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, UserId, ScopeId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                std::cout << "OnVacatedAsScopeLeaderCallback 1\n" << std::endl;
                // std::cout << ThisScopeId << "ScopeId" << std::endl;
                EXPECT_EQ(UserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to one of the other clients.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, TestRunnerUser1, TestRunnerUser2, &ElectedUserId](
                const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
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
            csp::CSPFoundation::Tick();
        }

        while (ReadyForAssertionsFuture2.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            csp::CSPFoundation::Tick();
        }

        // Restart leader election, exluding this user.
        auto [LeaderElectionResult] = AWAIT_PRE(MultiplayerSystem, __PerformLeaderElectionInScope, RequestPredicate, ScopeId, { { UserId } });
        EXPECT_EQ(LeaderElectionResult.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_TRUE(VacatedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);
    }

    // The client that becomes the leader will then exit the space, which will cause another election which will exclude this client.
    // We need to ensure that the leadership election is triggered and the remaining client is elected.
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, ElectedUserId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                printf((std::string { "SetOnVacatedAsScopeLeaderCallback 2" } + VacatedUserId.c_str() + "\n").c_str());

                EXPECT_EQ(ElectedUserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to the remaining client.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, TestRunnerUser1, TestRunnerUser2, &ElectedUserId](
                const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                printf((std::string { "SetOnElectedScopeLeaderCallback 2" } + LeaderUserId.c_str() + "\n").c_str());
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

        WaitForCallback(VacatedCallbackCalled);
        EXPECT_TRUE(VacatedCallbackCalled);

        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);

        RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(nullptr);
        RealtimeEngine->SetOnElectedScopeLeaderCallback(nullptr);
    }

    // The final client will now disconnect and give leadership back to this client
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, ElectedUserId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                EXPECT_EQ(ElectedUserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to this client.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, UserId](const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                EXPECT_EQ(UserId, LeaderUserId);
                ElectedCallbackCalled = true;
            });

        WaitForCallback(VacatedCallbackCalled);
        EXPECT_TRUE(VacatedCallbackCalled);

        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);
    }

    RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(nullptr);
    RealtimeEngine->SetOnElectedScopeLeaderCallback(nullptr);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    This test uses the MultiplayerTestRunner to spawn 1 additional clientsto test that the tick event is sent to scripts for the elected client
    The test does the following steps:
    - Sets up a space with managed leader election
    - Sets the client to the leader
    - Create a script which increases an entities model z position by 1 on tick
    - Call tick to ensure the x position increases by 1
    - Spawns 1 more client
    - Performs leader eleciton excluding the current user, so one of the other 2 clients gets it
    - Calls tick when the client is vacated as leader, showing the script event isnt triggered.
    - Calls tick when the second client is elected as leader, showing the script even isnt triggered.
    - This will finally trigger another leader election, giving leadership back to the main client.
    - The main client calls tick again, ensuring the x position increases by 1
*/
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, ScopeLeadershipScriptTickTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* MultiplayerSystem = SystemsManager.GetMultiplayerSystem();

    // Log in
    csp::systems::Profile NewTestUser = CreateTestUser();

    csp::common::String UserId;
    LogIn(UserSystem, UserId, NewTestUser.Email, GeneratedTestAccountPassword);

    // Create users for test runner
    auto TestRunnerUser1 = CreateTestUser();

    csp::systems::InviteUserRoleInfo InviteUser1;
    InviteUser1.UserEmail = TestRunnerUser1.Email;
    InviteUser1.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser1 };

    // Create space
    csp::systems::Space Space;
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, InviteUsers, nullptr, nullptr, Space);

    bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

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

    // We need to ensure the scope has ManagedLeaderElection enabled.
    {
        DefaultScope.ManagedLeaderElection = true;
        auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, ScopeId, DefaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Reenter space to stop leader election
    {
        auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        RealtimeEngine.get()->SetScriptLeaderReadyCallback(ScriptSystemReadyCallback);

        // Enter space
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Wait for the script system to be ready
    {
        auto ScriptSystemIsReady = [&ScriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (ScriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    csp::multiplayer::SpaceEntity* Entity = nullptr;
    csp::multiplayer::AnimatedModelSpaceComponent* ModelComponent = nullptr;

    // Create an entity with a script that increases the models x position by 1 on each tick
    {
        const std::string ScriptText = R"xx(
		globalThis.onTick = () => {
			var model = ThisEntity.getAnimatedModelComponents()[0];
			model.position = [model.position[0] + 1, model.position.y, model.position.z];
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

        Entity = CreateTestObject(RealtimeEngine.get());
        ModelComponent
            = static_cast<csp::multiplayer::AnimatedModelSpaceComponent*>(Entity->AddComponent(csp::multiplayer::ComponentType::AnimatedModel));
        csp::multiplayer::ScriptSpaceComponent* ScriptComponent
            = static_cast<csp::multiplayer::ScriptSpaceComponent*>(Entity->AddComponent(csp::multiplayer::ComponentType::ScriptData));

        Entity->QueueUpdate();
        RealtimeEngine->QueueEntityUpdate(Entity);

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Entity->GetScript().Invoke();

        const bool ScriptHasErrors = Entity->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);
    }

    // Call tick to ensure it updates, as we are the leader.
    {
        // Ensure the a position starts at 0.
        EXPECT_EQ(ModelComponent->GetPosition().X, 0);

        // Call tick to update the script
        csp::CSPFoundation::Tick();

        // Update the entities values
        RealtimeEngine->QueueEntityUpdate(Entity);
        RealtimeEngine->ProcessPendingEntityOperations();

        // Ensure the entity has been updated
        EXPECT_EQ(ModelComponent->GetPosition().X, 1);
    }

    //  Create another client for leader election
    MultiplayerTestRunnerProcess TestRunner1
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION_TICK)
              .SetSpaceId(Space.Id.c_str())
              .SetLoginEmail(TestRunnerUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(60);

    std::future<void> ReadyForAssertionsFuture = TestRunner1.ReadyForAssertionsFuture();

    // Next, we want to remove ourselves as leader and give it to the test runner.
    {
        // First listen to the vacated callback to ensure we are removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, UserId, ScopeId](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                std::cout << "OnVacatedAsScopeLeaderCallback 1\n" << std::endl;
                EXPECT_EQ(UserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to one of the other clients.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, TestRunnerUser1](const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
                EXPECT_EQ(TestRunnerUser1.UserId, LeaderUserId);
                ElectedCallbackCalled = true;
            });

        TestRunner1.StartProcess();

        while (ReadyForAssertionsFuture.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
        }

        // Restart leader election, exluding this user.
        auto [LeaderElectionResult] = AWAIT_PRE(MultiplayerSystem, __PerformLeaderElectionInScope, RequestPredicate, ScopeId, { { UserId } });
        EXPECT_EQ(LeaderElectionResult.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(VacatedCallbackCalled);
        EXPECT_TRUE(VacatedCallbackCalled);

        // Call tick to update the script
        csp::CSPFoundation::Tick();
        // The x position should still be 1, as we were no longer the script leader.
        EXPECT_EQ(ModelComponent->GetPosition().X, 1);

        RealtimeEngine->QueueEntityUpdate(Entity);
        RealtimeEngine->ProcessPendingEntityOperations();

        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);

        // Call tick to update the script
        csp::CSPFoundation::Tick();
        // The x position should still be 1, as another client is the leader.
        EXPECT_EQ(ModelComponent->GetPosition().X, 1);
    }

    // The final client will now disconnect and give leadership back to this client
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool VacatedCallbackCalled = false;
        RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&VacatedCallbackCalled, TestRunnerUser1](const csp::common::String&, const csp::common::String& VacatedUserId)
            {
                EXPECT_EQ(TestRunnerUser1.UserId, VacatedUserId);
                VacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to this client.
        bool ElectedCallbackCalled = false;
        RealtimeEngine->SetOnElectedScopeLeaderCallback(
            [&ElectedCallbackCalled, UserId](const csp::common::String&, const csp::common::String& LeaderUserId)
            {
                EXPECT_EQ(UserId, LeaderUserId);
                ElectedCallbackCalled = true;
            });

        WaitForCallback(VacatedCallbackCalled);
        EXPECT_TRUE(VacatedCallbackCalled);

        WaitForCallback(ElectedCallbackCalled);
        EXPECT_TRUE(ElectedCallbackCalled);
    }

    // Call tick to ensure it updates, as we are the leader.
    {
        // Ensure the a position os still 1 0.
        EXPECT_EQ(ModelComponent->GetPosition().X, 1);

        // Call tick to update the script now we are leader again.
        csp::CSPFoundation::Tick();

        // Update the entities values
        RealtimeEngine->QueueEntityUpdate(Entity);
        RealtimeEngine->ProcessPendingEntityOperations();

        // Ensure the entity has been updated
        EXPECT_EQ(ModelComponent->GetPosition().X, 2);
    }

    RealtimeEngine->SetOnVacatedAsScopeLeaderCallback(nullptr);
    RealtimeEngine->SetOnElectedScopeLeaderCallback(nullptr);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
