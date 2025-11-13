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

#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
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
        const csp::common::String Error = fmt::format(
            "ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {0} that isn't registered, for vacated leader: {1}.", TestScopeId,
            Connection.GetClientId())
                                              .c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Error)).Times(1);

        Manager.OnVacatedAsScopeLeader(TestScopeId, Connection.GetClientId());
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

        Manager.Update();
    }

    // We haven't waited long enough, so the callback shouldnt be called if we call Update again.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, Log)).Times(0);

        Manager.Update();
    }

    // Wait for the heartbeat interval before calling again. This should successfully call the callback again.
    {
        std::this_thread::sleep_for(csp::multiplayer::LeaderElectionHeartbeatInterval);

        RAIIMockLogger MockLogger {};
        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, Log)).Times(1);

        Manager.Update();
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

        Manager.Update();
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

        Manager.Update();
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

        Manager.Update();
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
        const csp::common::String Error = fmt::format(
            "ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0}, that already has the leader: {1}, for new leader: {2}.",
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

    // Test OnVacatedAsScopeLeader throws a warning when the vacated event is called for a scope thats leader doesn't match the one provided.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Error
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for the scope: {0}, that doesn't have the expected ClientId: "
                          "{1}, instead has: {2}.",
                TestScopeId, Connection.GetClientId(), TestClientId2)
                  .c_str();

        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {}.", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, Error)).Times(1);
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Log)).Times(1);

        // Call OnVacatedAsScopeLeader with the first client for a scope that hasn't got the first client as a leader.
        Manager.OnVacatedAsScopeLeader(TestScopeId, Connection.GetClientId());

        // Ensure the leader is still vacated.
        EXPECT_FALSE(Manager.GetLeaderClientId(TestScopeId).has_value());
    }

    // Test OnVacatedAsScopeLeader throws a warning when the vacated event is called for a scope that doesn't have a leader.
    {
        RAIIMockLogger MockLogger {};
        const csp::common::String Error = fmt::format(
            "ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for the scope: {0} that doesn't have a leader, for new leader: {1}",
            TestScopeId, Connection.GetClientId())
                                              .c_str();

        const csp::common::String Log
            = fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {}.", TestScopeId).c_str();

        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, Error)).Times(1);
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Log)).Times(1);

        // Call OnVacatedAsScopeLeader with the first client for a scope that hasn't got a leader.
        Manager.OnVacatedAsScopeLeader(TestScopeId, Connection.GetClientId());

        // Ensure the leader is still vacated.
        EXPECT_FALSE(Manager.GetLeaderClientId(TestScopeId).has_value());
    }
}