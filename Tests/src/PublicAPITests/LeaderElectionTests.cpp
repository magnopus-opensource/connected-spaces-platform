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
bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

/*
    This tests that scope registration and deregistration work correctly
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, RegisterScopeTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr const char* testScopeId1 = "TestScopeId1";
    static constexpr const char* testScopeId2 = "TestScopeId2";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    // Ensure the RegisterScope log is called with the leader.
    {
        RAIIMockLogger mockLogger {};

        // Ensure the RegisterScope log is called.
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with leader: {1}.", testScopeId1, connection.GetClientId())
                  .c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, log)).Times(1);

        manager.RegisterScope(testScopeId1, connection.GetClientId());

        // Ensure the leader has been set.
        EXPECT_TRUE(manager.GetLeaderClientId(testScopeId1).has_value() && *manager.GetLeaderClientId(testScopeId1) == connection.GetClientId());
    }

    // Ensure the RegisterScope log is called without a leader.
    {
        RAIIMockLogger mockLogger {};

        // Ensure the RegisterScope log is called with no leader.
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with no leader.", testScopeId2).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, log)).Times(1);

        manager.RegisterScope(testScopeId2, std::nullopt);

        // Ensure the leader is invalid.
        EXPECT_FALSE(manager.GetLeaderClientId(testScopeId2).has_value());
    }
}

/*
    This tests that the IsLocalClientLeaderTest returns true when a scopes leader matches the local client,
    and false when it doesn't.
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, IsLocalClientLeaderTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr uint64_t testClientId2 = 2;
    static constexpr const char* testScopeId1 = "TestScopeId1";
    static constexpr const char* testScopeId2 = "TestScopeId2";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    manager.RegisterScope(testScopeId1, connection.GetClientId());
    manager.RegisterScope(testScopeId2, testClientId2);

    EXPECT_TRUE(manager.IsLocalClientLeader(testScopeId1));
    EXPECT_FALSE(manager.IsLocalClientLeader(testScopeId2));
    EXPECT_FALSE(manager.IsLocalClientLeader("INVALID_SCOPE_ID"));
}

/*
    This tests that errors are correctly generated when a scope hasn't been registered to the ScopeLeadershipManager
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, UnregisteredScopeTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr const char* testScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    // Test OnElectedScopeLeader fails with the expected error.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String error
            = fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0} that isn't registered, for new "
                          "leader: {1}.",
                testScopeId, connection.GetClientId())
                  .c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

        manager.OnElectedScopeLeader(testScopeId, connection.GetClientId());
    }

    // Test OnElectedScopeLeader fails with the expected error.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String error
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {0} that isn't registered.", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

        manager.OnVacatedAsScopeLeader(testScopeId);
    }

    // Test GetLeaderClientId fails with the expected error.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String error
            = fmt::format("ScopeLeadershipManager::GetLeaderClientId Event called for the scope: {} that isn't registered.", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

        manager.GetLeaderClientId(testScopeId);
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
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr const char* testScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    manager.RegisterScope(testScopeId, connection.GetClientId());

    // Mock the heartbeat function to return a basic result.
    EXPECT_CALL(*m_signalRMock,
        Invoke(connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
            ::testing::_))
        .WillRepeatedly(
            [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> callback)
                -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            {
                std::vector<signalr::value> params;
                signalr::value value(params);

                callback(params, nullptr);
                return async::make_task(std::make_tuple(value, std::exception_ptr(nullptr)));
            });

    // Ensure the callback is called when we update.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, log)).Times(1);

        manager.SendHeartbeatIfElectedScopeLeader();
    }

    // We haven't waited long enough, so the callback shouldnt be called if we call Update again.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, log)).Times(0);

        manager.SendHeartbeatIfElectedScopeLeader();
    }

    // Wait for the heartbeat interval before calling again. This should successfully call the callback again.
    {
        std::this_thread::sleep_for(csp::multiplayer::LeaderElectionHeartbeatInterval);

        RAIIMockLogger mockLogger {};
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, log)).Times(1);

        manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    This test ensures that the heartbeat isnt called for scopes that the local client isn't a leader of.
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, NonLeaderUpdateTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr uint64_t testClientId2 = 2;
    static constexpr const char* testScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    manager.RegisterScope(testScopeId, testClientId2);

    // Mock the heartbeat function to return a basic result.
    EXPECT_CALL(*m_signalRMock,
        Invoke(connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
            ::testing::_))
        .WillRepeatedly(
            [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> callback)
                -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            {
                std::vector<signalr::value> params;
                signalr::value value(params);

                callback(params, nullptr);
                return async::make_task(std::make_tuple(value, std::exception_ptr(nullptr)));
            });

    // Ensure the callback is called when we update.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, log)).Times(0);

        manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    Tests that the correct logs are called when an std::exception is received from the scope leader heartbeat endpoint
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, HeartbeatStdExceptionTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr const char* testScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    manager.RegisterScope(testScopeId, connection.GetClientId());

    // Mock the heartbeat function to return a std::exception.
    {
        EXPECT_CALL(*m_signalRMock,
            Invoke(connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
                ::testing::_))
            .WillOnce(
                [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> callback)
                    -> async::task<std::tuple<signalr::value, std::exception_ptr>>
                {
                    std::vector<signalr::value> params;
                    signalr::value value(params);

                    auto exception = std::make_exception_ptr(std::runtime_error("Test Exception"));

                    callback(params, exception);
                    return async::make_task(std::make_tuple(value, exception));
                });

        RAIIMockLogger mockLogger {};
        const csp::common::String log = fmt::format(
            "ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {0} with error: {1}", testScopeId, "Test Exception")
                                            .c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, log)).Times(1);

        manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    Tests that the correct logs are called when a non std::exception is received from the scope leader heartbeat endpoint
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, HeartbeatNonStdExceptionTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr const char* testScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    manager.RegisterScope(testScopeId, connection.GetClientId());

    // Mock the heartbeat function to return a non std::exception
    {
        EXPECT_CALL(*m_signalRMock,
            Invoke(connection.GetMultiplayerHubMethods().Get(csp::multiplayer::MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), ::testing::_,
                ::testing::_))
            .WillOnce(
                [](const std::string&, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> callback)
                    -> async::task<std::tuple<signalr::value, std::exception_ptr>>
                {
                    std::vector<signalr::value> params;
                    signalr::value value(params);

                    // Store a random type in the exception ptr to test catch(...)
                    auto exception = std::make_exception_ptr(std::string {});

                    callback(params, exception);
                    return async::make_task(std::make_tuple(value, exception));
                });

        RAIIMockLogger mockLogger {};
        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {} with an unknown error.", testScopeId)
                  .c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, log)).Times(1);

        manager.SendHeartbeatIfElectedScopeLeader();
    }
}

/*
    This tests that errors are correctly generated when an election event is called with unexpected data.
*/
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, LeaderElectionUnitTests, InvalidEventTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& connection = *systemsManager.GetMultiplayerConnection();
    auto& logSystem = *systemsManager.GetLogSystem();

    static constexpr const uint64_t testClientId2 = 2;

    static constexpr const char* testScopeId = "TestScopeId";

    csp::multiplayer::ScopeLeadershipManager manager { connection, logSystem };

    // Register 2 test scopes.
    manager.RegisterScope(testScopeId, connection.GetClientId());

    // Test OnElectedScopeLeader throws a warning and log when the election event is called for a scope that already has a leader.
    {
        RAIIMockLogger mockLogger {};
        const csp::common::String error = fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0}, that already has "
                                                      "the leader: {1}, for new leader: {2}. Overwriting old value.",
            testScopeId, connection.GetClientId(), testClientId2)
                                              .c_str();

        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::OnElectedScopeLeader New leader: {0}, for scope: {1}.", testClientId2, testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, error)).Times(1);
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, log)).Times(1);

        // Call OnElectedScopeLeader with the second client as leader without vacating first.
        manager.OnElectedScopeLeader(testScopeId, testClientId2);

        // Ensure the leader is still updated to the new value.
        EXPECT_TRUE(manager.GetLeaderClientId(testScopeId).has_value() && *manager.GetLeaderClientId(testScopeId) == testClientId2);
    }

    // Test OnVacatedAsScopeLeader throws a warning when the vacated event is called for a scope that doesn't have a leader.
    {
        // First vacate the scope leader.
        manager.OnVacatedAsScopeLeader(testScopeId);

        RAIIMockLogger mockLogger {};
        const csp::common::String error
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for the scope: {0} that doesn't have a leader.", testScopeId)
                  .c_str();

        const csp::common::String log
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {}.", testScopeId).c_str();

        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, error)).Times(1);
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, log)).Times(1);

        // Call OnVacatedAsScopeLeader again with the first client for a scope that hasn't got a leader.
        manager.OnVacatedAsScopeLeader(testScopeId);

        // Ensure the leader is still vacated.
        EXPECT_FALSE(manager.GetLeaderClientId(testScopeId).has_value());
    }
}

/*
    This test verifies that the rest and multiplayer hub api can be used to:
        - Set a scope to have managed leader election
        - Re-enter the space
        - The elected leader can be retrieved
*/
CSP_PUBLIC_TEST(DISABLED_CSPEngine, LeaderElectionTests, GetScopeLeaderTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* multiplayerSystem = systemsManager.GetMultiplayerSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create OnlineRealtimeEngine
    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Ensure we only have 1 scope
    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    csp::systems::Scope defaultScope = GetScopesResult.GetScopes()[0];
    const csp::common::String scopeId = defaultScope.Id;

    // Ensure the scope is global
    EXPECT_EQ(defaultScope.PubSubType, csp::systems::PubSubModelType::Global);

    // Update the scope to enable managed leader election.
    defaultScope.ManagedLeaderElection = true;
    auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, scopeId, defaultScope);
    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(UpdateScopeResult.GetScope().ManagedLeaderElection, true);

    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Wait for initial elected event
    std::promise<void> onElectedScopeLeaderPromise;
    auto onElectedScopeLeaderFuture = onElectedScopeLeaderPromise.get_future();

    realtimeEngine->SetOnElectedScopeLeaderCallback(
        [&onElectedScopeLeaderPromise, &userId](const csp::common::String&, const csp::common::String& leaderUserId)
        {
            std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
            EXPECT_EQ(userId, leaderUserId);
            onElectedScopeLeaderPromise.set_value();
        });

    // Enter space
    auto [ReEnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    ASSERT_TRUE(WaitForFuture(onElectedScopeLeaderFuture));

    // Get the scope leader
    auto [GetScopeLeaderResult] = AWAIT_PRE(multiplayerSystem, GetScopeLeader, RequestPredicate, scopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Ensure we are the scope leader.
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ScopeId, scopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ScopeLeaderUserId, userId);
    EXPECT_EQ(GetScopeLeaderResult.GetScopeLeader().ElectionInProgress, false);

    // Clean up
    auto [ExitResult2] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    This test uses the MultiplayerTestRunner to spawn 2 additional clients to test that the scope leader is correctly assigned.
    The test does the following steps:
    - Sets up a space with managed leader election
    - Sets the client to the leader
    - Spawns 2 more clients
    - Performs leader election excluding the current user, so one of the other 2 clients gets it
    - When the first MultiplayerTestRunner client gets leadership, they well perform another leader election, excluding themselves and the main client
    - This will give leadership to the other MultiplayerTestRunner client, which will then disconnect
    - This will finally trigger another leader election, giving leadership back to the main client.
*/
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, ScopeLeadershipTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* multiplayerSystem = systemsManager.GetMultiplayerSystem();

    // Log in
    csp::systems::Profile newTestUser = CreateTestUser();

    csp::common::String userId;
    LogIn(userSystem, userId, newTestUser.Email, GeneratedTestAccountPassword);

    // Create users for test runner
    auto testRunnerUser1 = CreateTestUser();
    auto testRunnerUser2 = CreateTestUser();

    csp::systems::InviteUserRoleInfo inviteUser1;
    inviteUser1.UserEmail = testRunnerUser1.Email;
    inviteUser1.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfo inviteUser2;
    inviteUser2.UserEmail = testRunnerUser2.Email;
    inviteUser2.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser1, inviteUser2 };

    // Create space
    csp::systems::Space space;
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, inviteUsers, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    {
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // First, we need to get the default scope
    csp::systems::Scope defaultScope;
    csp::common::String scopeId;

    {
        auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
        EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

        if (GetScopesResult.GetScopes().Size() != 1)
        {
            FAIL();
        }

        defaultScope = GetScopesResult.GetScopes()[0];
        scopeId = defaultScope.Id;
    }

    // We need to ensure the scope has ManagedLeaderElection enabled.
    {
        defaultScope.ManagedLeaderElection = true;
        auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, scopeId, defaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Reenter space to start leader election
    {
        auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        // Wait for initial elected event
        std::promise<void> onElectedScopeLeaderPromise;
        auto onElectedScopeLeaderFuture = onElectedScopeLeaderPromise.get_future();

        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&onElectedScopeLeaderPromise, &userId](const csp::common::String&, const csp::common::String& leaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
                EXPECT_EQ(userId, leaderUserId);
                onElectedScopeLeaderPromise.set_value();
            });

        // Enter space
        auto [ReEnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        ASSERT_TRUE(WaitForFuture(onElectedScopeLeaderFuture));
    }

    //  Create 2 additional clients for leader election.
    MultiplayerTestRunnerProcess testRunner1 = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION)
                                                   .SetSpaceId(space.Id.c_str())
                                                   .SetLoginEmail(testRunnerUser1.Email.c_str())
                                                   .SetPassword(GeneratedTestAccountPassword)
                                                   .SetEndpoint(EndpointBaseURI())
                                                   .SetTimeoutInSeconds(60);

    MultiplayerTestRunnerProcess testRunner2 = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION)
                                                   .SetSpaceId(space.Id.c_str())
                                                   .SetLoginEmail(testRunnerUser2.Email.c_str())
                                                   .SetPassword(GeneratedTestAccountPassword)
                                                   .SetEndpoint(EndpointBaseURI())
                                                   .SetTimeoutInSeconds(60);

    csp::common::String electedUserId;

    // Next, we want to remove ourselves as leader and give it to one of the test runners.
    {
        // First listen to the vacated callback to ensure we are removed as the leader.
        bool vacatedCallbackCalled = false;
        realtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&vacatedCallbackCalled, userId, scopeId](const csp::common::String&, const csp::common::String& vacatedUserId)
            {
                std::cout << "OnVacatedAsScopeLeaderCallback 1\n" << std::endl;
                // std::cout << ThisScopeId << "ScopeId" << std::endl;
                EXPECT_EQ(userId, vacatedUserId);
                vacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to one of the other clients.
        bool electedCallbackCalled = false;
        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&electedCallbackCalled, testRunnerUser1, testRunnerUser2, &electedUserId](
                const csp::common::String&, const csp::common::String& leaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
                EXPECT_TRUE((testRunnerUser1.UserId == leaderUserId) || (testRunnerUser2.UserId == leaderUserId));
                electedUserId = leaderUserId;
                electedCallbackCalled = true;
            });

        testRunner1.StartProcess();
        testRunner2.StartProcess();

        std::future<void> readyForAssertionsFuture1 = testRunner1.ReadyForAssertionsFuture();
        std::future<void> readyForAssertionsFuture2 = testRunner2.ReadyForAssertionsFuture();

        while (readyForAssertionsFuture1.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            csp::CSPFoundation::Tick();
        }

        while (readyForAssertionsFuture2.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            csp::CSPFoundation::Tick();
        }

        // Restart leader election, exluding this user.
        auto [LeaderElectionResult] = AWAIT_PRE(multiplayerSystem, __PerformLeaderElectionInScope, RequestPredicate, scopeId, { { userId } });
        EXPECT_EQ(LeaderElectionResult.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(vacatedCallbackCalled);
        EXPECT_TRUE(vacatedCallbackCalled);

        WaitForCallback(electedCallbackCalled);
        EXPECT_TRUE(electedCallbackCalled);
    }

    // The client that becomes the leader will then exit the space, which will cause another election which will exclude this client.
    // We need to ensure that the leadership election is triggered and the remaining client is elected.
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool vacatedCallbackCalled = false;
        realtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&vacatedCallbackCalled, electedUserId](const csp::common::String&, const csp::common::String& vacatedUserId)
            {
                printf("SetOnVacatedAsScopeLeaderCallback 2%s\n", vacatedUserId.c_str());

                EXPECT_EQ(electedUserId, vacatedUserId);
                vacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to the remaining client.
        bool electedCallbackCalled = false;
        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&electedCallbackCalled, testRunnerUser1, testRunnerUser2, &electedUserId](
                const csp::common::String&, const csp::common::String& leaderUserId)
            {
                printf("SetOnElectedScopeLeaderCallback 2%s\n", leaderUserId.c_str());
                // If the previous leader was User1, the new leader should be User2
                if (electedUserId == testRunnerUser1.UserId)
                {
                    EXPECT_EQ(leaderUserId, testRunnerUser2.UserId);
                }
                else
                {
                    EXPECT_EQ(leaderUserId, testRunnerUser1.UserId);
                }

                electedUserId = leaderUserId;
                electedCallbackCalled = true;
            });

        WaitForCallback(vacatedCallbackCalled);
        EXPECT_TRUE(vacatedCallbackCalled);

        WaitForCallback(electedCallbackCalled);
        EXPECT_TRUE(electedCallbackCalled);

        realtimeEngine->SetOnVacatedAsScopeLeaderCallback(nullptr);
        realtimeEngine->SetOnElectedScopeLeaderCallback(nullptr);
    }

    // The final client will now disconnect and give leadership back to this client
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool vacatedCallbackCalled = false;
        realtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&vacatedCallbackCalled, electedUserId](const csp::common::String&, const csp::common::String& vacatedUserId)
            {
                EXPECT_EQ(electedUserId, vacatedUserId);
                vacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to this client.
        bool electedCallbackCalled = false;
        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&electedCallbackCalled, userId](const csp::common::String&, const csp::common::String& leaderUserId)
            {
                EXPECT_EQ(userId, leaderUserId);
                electedCallbackCalled = true;
            });

        WaitForCallback(vacatedCallbackCalled);
        EXPECT_TRUE(vacatedCallbackCalled);

        WaitForCallback(electedCallbackCalled);
        EXPECT_TRUE(electedCallbackCalled);
    }

    realtimeEngine->SetOnVacatedAsScopeLeaderCallback(nullptr);
    realtimeEngine->SetOnElectedScopeLeaderCallback(nullptr);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    This test uses the MultiplayerTestRunner to spawn 1 additional client to test that the tick event is sent to scripts for the elected client
    The test does the following steps:
    - Sets up a space with managed leader election
    - Sets the client to the leader
    - Create a script which increases an entities model z position by 1 on tick
    - Call tick to ensure the x position increases by 1
    - Spawns 1 more client
    - Performs leader election excluding the current user, so one of the other 2 clients gets it
    - Calls tick when the client is vacated as leader, showing the script event isnt triggered.
    - Calls tick when the second client is elected as leader, showing the script event isnt triggered.
    - This will finally trigger another leader election, giving leadership back to the main client.
    - The main client calls tick again, ensuring the x position increases by 1
*/
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, ScopeLeadershipScriptTickTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* multiplayerSystem = systemsManager.GetMultiplayerSystem();

    // Log in
    csp::systems::Profile newTestUser = CreateTestUser();

    csp::common::String userId;
    LogIn(userSystem, userId, newTestUser.Email, GeneratedTestAccountPassword);

    // Create users for test runner
    auto testRunnerUser1 = CreateTestUser();

    csp::systems::InviteUserRoleInfo inviteUser1;
    inviteUser1.UserEmail = testRunnerUser1.Email;
    inviteUser1.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser1 };

    // Create space
    csp::systems::Space space;
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, inviteUsers, nullptr, nullptr, space);

    bool scriptSystemReady = false;

    auto scriptSystemReadyCallback = [&scriptSystemReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptSystemReady = true;
    };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    {
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // First, we need to get the default scope
    csp::systems::Scope defaultScope;
    csp::common::String scopeId;

    {
        auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
        EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

        if (GetScopesResult.GetScopes().Size() != 1)
        {
            FAIL();
        }

        defaultScope = GetScopesResult.GetScopes()[0];
        scopeId = defaultScope.Id;
    }

    // We need to ensure the scope has ManagedLeaderElection enabled.
    {
        defaultScope.ManagedLeaderElection = true;
        auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, scopeId, defaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Reenter space to start leader election
    {
        auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        realtimeEngine.get()->SetScriptLeaderReadyCallback(scriptSystemReadyCallback);

       // Wait for initial elected event
        std::promise<void> onElectedScopeLeaderPromise;
        auto onElectedScopeLeaderFuture = onElectedScopeLeaderPromise.get_future();

        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&onElectedScopeLeaderPromise, &userId](const csp::common::String&, const csp::common::String& leaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
                EXPECT_EQ(userId, leaderUserId);
                onElectedScopeLeaderPromise.set_value();
            });

        // Enter space
        auto [ReEnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        ASSERT_TRUE(WaitForFuture(onElectedScopeLeaderFuture));

        // Call to try and send heartbeat
        csp::CSPFoundation::Tick();
    }

    // Wait for the script system to be ready
    {
        auto scriptSystemIsReady = [&scriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (scriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    // Call to try and send heartbeat
    csp::CSPFoundation::Tick();

    csp::multiplayer::SpaceEntity* entity = nullptr;
    csp::multiplayer::AnimatedModelSpaceComponent* modelComponent = nullptr;

        //  Create another client for leader election
    MultiplayerTestRunnerProcess testRunner1
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION_TICK)
              .SetSpaceId(space.Id.c_str())
              .SetLoginEmail(testRunnerUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(60);

    std::future<void> readyForAssertionsFuture = testRunner1.ReadyForAssertionsFuture();

    testRunner1.StartProcess();

    while (readyForAssertionsFuture.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
    {
        // Call to try and send heartbeat
        csp::CSPFoundation::Tick();
    }

    // Create an entity with a script that increases the models x position by 1 on each tick
    {
        const std::string scriptText = R"xx(
		globalThis.onTick = () => {
			var model = ThisEntity.getAnimatedModelComponents()[0];
			model.position = [model.position[0] + 1, model.position.y, model.position.z];
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

        entity = CreateTestObject(realtimeEngine.get());
        modelComponent
            = static_cast<csp::multiplayer::AnimatedModelSpaceComponent*>(entity->AddComponent(csp::multiplayer::ComponentType::AnimatedModel));
        csp::multiplayer::ScriptSpaceComponent* scriptComponent
            = static_cast<csp::multiplayer::ScriptSpaceComponent*>(entity->AddComponent(csp::multiplayer::ComponentType::ScriptData));

        // Call to try and send heartbeat
        csp::CSPFoundation::Tick();

        entity->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        entity->GetScript().Invoke();

        const bool scriptHasErrors = entity->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);
    }

    // Call tick to ensure it updates, as we are the leader.
    {
        // Ensure the a position starts at 0.
        EXPECT_EQ(modelComponent->GetPosition().X, 0);

        // Call tick to update the script
        csp::CSPFoundation::Tick();

        // Update the entities values
        realtimeEngine->QueueEntityUpdate(entity);
        realtimeEngine->ProcessPendingEntityOperations();

        // Ensure the entity has been updated
        EXPECT_EQ(modelComponent->GetPosition().X, 1);
    }

    // Next, we want to remove ourselves as leader and give it to the test runner.
    {
        // First listen to the vacated callback to ensure we are removed as the leader.
        bool vacatedCallbackCalled = false;
        realtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&vacatedCallbackCalled, userId, scopeId](const csp::common::String&, const csp::common::String& vacatedUserId)
            {
                std::cout << "OnVacatedAsScopeLeaderCallback 1\n" << std::endl;
                EXPECT_EQ(userId, vacatedUserId);
                vacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to one of the other clients.
        bool electedCallbackCalled = false;
        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&electedCallbackCalled, testRunnerUser1](const csp::common::String&, const csp::common::String& leaderUserId)
            {
                std::cout << "OnElectedScopeLeaderCallback 1\n" << std::endl;
                EXPECT_EQ(testRunnerUser1.UserId, leaderUserId);
                electedCallbackCalled = true;
            });

        // Restart leader election, exluding this user.
        auto [LeaderElectionResult] = AWAIT_PRE(multiplayerSystem, __PerformLeaderElectionInScope, RequestPredicate, scopeId, { { userId } });
        EXPECT_EQ(LeaderElectionResult.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(vacatedCallbackCalled);
        EXPECT_TRUE(vacatedCallbackCalled);

        // Call tick to update the script
        csp::CSPFoundation::Tick();
        // The x position should still be 1, as we were no longer the script leader.
        EXPECT_EQ(modelComponent->GetPosition().X, 1);

        realtimeEngine->QueueEntityUpdate(entity);
        realtimeEngine->ProcessPendingEntityOperations();

        WaitForCallback(electedCallbackCalled);
        EXPECT_TRUE(electedCallbackCalled);

        // Call tick to update the script
        csp::CSPFoundation::Tick();
        // The x position should still be 1, as another client is the leader.
        EXPECT_EQ(modelComponent->GetPosition().X, 1);
    }

    // The final client will now disconnect and give leadership back to this client
    {
        // First listen to the vacated callback to ensure the disconnected client is removed as the leader.
        bool vacatedCallbackCalled = false;
        realtimeEngine->SetOnVacatedAsScopeLeaderCallback(
            [&vacatedCallbackCalled, testRunnerUser1](const csp::common::String&, const csp::common::String& vacatedUserId)
            {
                EXPECT_EQ(testRunnerUser1.UserId, vacatedUserId);
                vacatedCallbackCalled = true;
            });

        // Also listen for the elected callback to ensure it goes to this client.
        bool electedCallbackCalled = false;
        realtimeEngine->SetOnElectedScopeLeaderCallback(
            [&electedCallbackCalled, userId](const csp::common::String&, const csp::common::String& leaderUserId)
            {
                EXPECT_EQ(userId, leaderUserId);
                electedCallbackCalled = true;
            });

        WaitForCallback(vacatedCallbackCalled);
        EXPECT_TRUE(vacatedCallbackCalled);

        WaitForCallback(electedCallbackCalled);
        EXPECT_TRUE(electedCallbackCalled);
    }

    // Call tick to ensure it updates, as we are the leader.
    {
        // Ensure the a position os still 1 0.
        EXPECT_EQ(modelComponent->GetPosition().X, 1);

        // Call tick to update the script now we are leader again.
        csp::CSPFoundation::Tick();

        // Update the entities values
        realtimeEngine->QueueEntityUpdate(entity);
        realtimeEngine->ProcessPendingEntityOperations();

        // Ensure the entity has been updated
        EXPECT_EQ(modelComponent->GetPosition().X, 2);
    }

    realtimeEngine->SetOnVacatedAsScopeLeaderCallback(nullptr);
    realtimeEngine->SetOnElectedScopeLeaderCallback(nullptr);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    This test uses the MultiplayerTestRunner to spawn 1 additional client to test that events that are sent are always fowarded to the leader to run.
    The test does the following steps:
    - Sets up a space with managed leader election
    - Sets the client to the leader
    - Create a script which increases an entities model z position by 1 on event
    - Fires the event to ensure the event works for the current leader
    - Spawns 1 more client
    - The second client fires the script event, causing the remote script event path to be triggered
    - Ensures the first client updates the entity from its script
*/
CSP_PUBLIC_TEST(CSPEngine, LeaderElectionTests, ScopeLeadershipScriptEventTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* multiplayerSystem = systemsManager.GetMultiplayerSystem();

    // Log in
    csp::systems::Profile newTestUser = CreateTestUser();

    csp::common::String userId;
    LogIn(userSystem, userId, newTestUser.Email, GeneratedTestAccountPassword);

    // Create users for test runner
    auto testRunnerUser1 = CreateTestUser();

    csp::systems::InviteUserRoleInfo inviteUser1;
    inviteUser1.UserEmail = testRunnerUser1.Email;
    inviteUser1.UserRole = csp::systems::SpaceUserRole::Moderator;

    csp::systems::InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser1 };

    // Create space
    csp::systems::Space space;
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, inviteUsers, nullptr, nullptr, space);

    bool scriptSystemReady = false;

    auto scriptSystemReadyCallback = [&scriptSystemReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptSystemReady = true;
    };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    {
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // First, we need to get the default scope
    csp::systems::Scope defaultScope;
    csp::common::String scopeId;

    {
        auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
        EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

        if (GetScopesResult.GetScopes().Size() != 1)
        {
            FAIL();
        }

        defaultScope = GetScopesResult.GetScopes()[0];
        scopeId = defaultScope.Id;
    }

    // We need to ensure the scope has ManagedLeaderElection enabled.
    {
        defaultScope.ManagedLeaderElection = true;
        auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, scopeId, defaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Reenter space to start leader election
    {
        auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        realtimeEngine.get()->SetScriptLeaderReadyCallback(scriptSystemReadyCallback);

        // Enter space
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Call to try and send heartbeat
    csp::CSPFoundation::Tick();

    realtimeEngine->SetEntityPatchRateLimitEnabled(false);

    // Wait for the script system to be ready
    {
        auto scriptSystemIsReady = [&scriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (scriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    // Call to try and send heartbeat
    csp::CSPFoundation::Tick();

    csp::multiplayer::SpaceEntity* entity = nullptr;
    csp::multiplayer::AnimatedModelSpaceComponent* modelComponent = nullptr;

    // Create an entity with a script that increases the models x position by 1 on each tick
    {
        const std::string scriptText = R"xx(
		globalThis.onEvent = () => {
			var model = ThisEntity.getAnimatedModelComponents()[0];
			model.position = [model.position[0] + 1, model.position.y, model.position.z];
		}

		ThisEntity.subscribeToMessage("customEvent", "onEvent");
		  
    )xx";

        entity = CreateTestObject(realtimeEngine.get());
        modelComponent
            = static_cast<csp::multiplayer::AnimatedModelSpaceComponent*>(entity->AddComponent(csp::multiplayer::ComponentType::AnimatedModel));
        csp::multiplayer::ScriptSpaceComponent* scriptComponent
            = static_cast<csp::multiplayer::ScriptSpaceComponent*>(entity->AddComponent(csp::multiplayer::ComponentType::ScriptData));

        // Call to try and send heartbeat
        csp::CSPFoundation::Tick();

        entity->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        entity->GetScript().Invoke();

        const bool scriptHasErrors = entity->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);
    }

    // Fire an event from this client to ensure events work locally
    {
        // Ensure the a position starts at 0.
        EXPECT_EQ(modelComponent->GetPosition().X, 0);

        // Call tick to update the script
        entity->GetScript().PostMessageToScript("customEvent");

        // Events happen on a different thread so we need to wait a moment.
        std::this_thread::sleep_for(std::chrono::milliseconds { 1000 });

        // Call to try and send heartbeat
        csp::CSPFoundation::Tick();

        // Update the entities values
        realtimeEngine->QueueEntityUpdate(entity);
        realtimeEngine->ProcessPendingEntityOperations();

        // Ensure the entity has been updated
        EXPECT_EQ(modelComponent->GetPosition().X, 1);
    }

    // Ensure data has been written to database before we spawn the client.
    // This fixes an issue where the script source was sometimes empty on the spawned client.
    for (int i = 0; i < 7; i++)
    {
        csp::CSPFoundation::Tick();
        std::this_thread::sleep_for(std::chrono::seconds { 1 });
    }

    // Send script event from another client
    {
        //  Create another client for leader election
        MultiplayerTestRunnerProcess testRunner1
            = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::LEADER_ELECTION_EVENT)
                  .SetSpaceId(space.Id.c_str())
                  .SetLoginEmail(testRunnerUser1.Email.c_str())
                  .SetPassword(GeneratedTestAccountPassword)
                  .SetEndpoint(EndpointBaseURI())
                  .SetTimeoutInSeconds(60);

        std::future<void> readyForAssertionsFuture = testRunner1.ReadyForAssertionsFuture();

        bool updated = false;
        entity->SetUpdateCallback([&updated](auto&&, auto&&, auto&&) { updated = true; });

        testRunner1.StartProcess();

        // Wait for other client to run
        while (readyForAssertionsFuture.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            csp::CSPFoundation::Tick();
        }

        WaitForCallbackWithUpdate(updated, realtimeEngine.get());
        EXPECT_TRUE(updated);

        // Ensure we have updated the entity form the remote script event
        EXPECT_EQ(modelComponent->GetPosition().X, 2);
    }

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}