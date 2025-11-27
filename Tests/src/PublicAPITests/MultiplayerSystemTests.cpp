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

#include "RAIIMockLogger.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "gtest/gtest.h"

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

/*
    Tests that GetScopesBySpace correctly retrievs the default scope of a space.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetDefaultScopeTest)
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
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    // Ensure default scope has correct default values.
    const csp::systems::Scope& DefaultScope = GetScopesResult.GetScopes()[0];
    EXPECT_EQ(DefaultScope.ReferenceId, Space.Id);
    EXPECT_EQ(DefaultScope.ReferenceType, "GroupId");
    EXPECT_EQ(DefaultScope.PubSubType, csp::systems::PubSubModelType::Global);
    EXPECT_EQ(DefaultScope.SolveRadius, 0.0);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    Tests that GetScopesBySpace correctly returns 0 elements, and an error message when out of the space.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetDefaultScopeOutOfSpaceTest)
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
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    {
        RAIIMockLogger MockLogger {};

        const csp::common::String GetScopesBySpaceErrorMsg = "GetScopesBySpace: You must have entered the space you want to get scopes for";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, GetScopesBySpaceErrorMsg));

        // Get the default scope
        auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
        EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(GetScopesResult.GetScopes().Size(), 0);
    }

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    Tests that trying to get a scope using an invalid space id correctly returns a 400 error.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetScopeByInvalidSpaceTest)
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
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, "INVALID_SPACE_ID");
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetScopesResult.GetHttpResultCode(), 0);
    EXPECT_EQ(GetScopesResult.GetScopes().Size(), 0);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    Tests that we can update properties on a scope
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, UpdateScopeByIdTest)
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
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    const csp::systems::Scope DefaultScope = GetScopesResult.GetScopes()[0];
    const csp::common::String ScopeId = DefaultScope.Id;

    // Update scope properties
    const csp::common::String NewScopeName = ScopeId + "NewName";
    const csp::systems::PubSubModelType NewType = csp::systems::PubSubModelType::Object;
    const double NewRadius = 99.0;
    const bool NewManagedLeaderElection = true;

    csp::systems::Scope NewScope = DefaultScope;
    NewScope.Name = NewScopeName;
    NewScope.PubSubType = NewType;
    NewScope.SolveRadius = NewRadius;
    NewScope.ManagedLeaderElection = NewManagedLeaderElection;

    auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, ScopeId, NewScope);
    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Ensure properties of the new scope match the ones we set.
    EXPECT_EQ(UpdateScopeResult.GetScope().Name, NewScopeName);
    EXPECT_EQ(UpdateScopeResult.GetScope().PubSubType, NewType);
    EXPECT_EQ(UpdateScopeResult.GetScope().SolveRadius, NewRadius);
    EXPECT_EQ(UpdateScopeResult.GetScope().ManagedLeaderElection, NewManagedLeaderElection);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    Tests that trying to update a scope using an invalid scope id correctly returns a 400 error.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, UpdateInvalidScopeTest)
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
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Update the scope with an invalid id.
    const csp::systems::Scope DefaultScope = GetScopesResult.GetScopes()[0];

    auto [UpdateScopeResult] = AWAIT_PRE(MultiplayerSystem, UpdateScopeById, RequestPredicate, "INVALID_SCOPE_ID", DefaultScope);

    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdateScopeResult.GetHttpResultCode(), 400);
    EXPECT_EQ(UpdateScopeResult.GetScope().Id, "");

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

/*
    Ensures that getting the scope leader fails if the scope hasn't been set with "ManagedLeaderElection"
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetScopeLeaderNoManagedElectionTest)
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
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(MultiplayerSystem, GetScopesBySpace, RequestPredicate, Space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    csp::common::String ScopeId = GetScopesResult.GetScopes()[0].Id;

    // Getting the scope leader should fail because the scope doesn't have managed leader election enabled.
    auto [GetScopeLeaderResult] = AWAIT_PRE(MultiplayerSystem, GetScopeLeader, RequestPredicate, ScopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetScopeLeaderResult.GetHttpResultCode(), 400);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
