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
bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

/*
    Tests that GetScopesBySpace correctly retrievs the default scope of a space.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetDefaultScopeTest)
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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    // Ensure default scope has correct default values.
    const csp::systems::Scope& defaultScope = GetScopesResult.GetScopes()[0];
    EXPECT_EQ(defaultScope.ReferenceId, space.Id);
    EXPECT_EQ(defaultScope.ReferenceType, "GroupId");
    EXPECT_EQ(defaultScope.PubSubType, csp::systems::PubSubModelType::Global);
    EXPECT_EQ(defaultScope.SolveRadius, 0.0);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    Tests that GetScopesBySpace correctly returns 0 elements, and an error message when out of the space.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetDefaultScopeOutOfSpaceTest)
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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    {
        RAIIMockLogger mockLogger {};

        const csp::common::String getScopesBySpaceErrorMsg = "GetScopesBySpace: You must have entered the space you want to get scopes for";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, getScopesBySpaceErrorMsg));

        // Get the default scope
        auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
        EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(GetScopesResult.GetScopes().Size(), 0);
    }

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    Tests that trying to get a scope using an invalid space id correctly returns a 400 error.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetScopeByInvalidSpaceTest)
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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, "INVALID_SPACE_ID");
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetScopesResult.GetHttpResultCode(), 0);
    EXPECT_EQ(GetScopesResult.GetScopes().Size(), 0);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    Tests that we can update properties on a scope
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, UpdateScopeByIdTest)
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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }

    const csp::systems::Scope defaultScope = GetScopesResult.GetScopes()[0];
    const csp::common::String scopeId = defaultScope.Id;

    // Update scope properties
    const csp::common::String newScopeName = scopeId + "NewName";
    const csp::systems::PubSubModelType newType = csp::systems::PubSubModelType::Object;
    const double newRadius = 99.0;
    const bool newManagedLeaderElection = false;

    csp::systems::Scope newScope = defaultScope;
    newScope.Name = newScopeName;
    newScope.PubSubType = newType;
    newScope.SolveRadius = newRadius;
    newScope.ManagedLeaderElection = newManagedLeaderElection;

    auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, scopeId, newScope);
    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Ensure properties of the new scope match the ones we set.
    EXPECT_EQ(UpdateScopeResult.GetScope().Name, newScopeName);
    EXPECT_EQ(UpdateScopeResult.GetScope().PubSubType, newType);
    EXPECT_EQ(UpdateScopeResult.GetScope().SolveRadius, newRadius);
    EXPECT_EQ(UpdateScopeResult.GetScope().ManagedLeaderElection, newManagedLeaderElection);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    Tests that trying to update a scope using an invalid scope id correctly returns a 400 error.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, UpdateInvalidScopeTest)
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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Update the scope with an invalid id.
    const csp::systems::Scope defaultScope = GetScopesResult.GetScopes()[0];

    auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, "INVALID_SCOPE_ID", defaultScope);

    EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdateScopeResult.GetHttpResultCode(), 400);
    EXPECT_EQ(UpdateScopeResult.GetScope().Id, "");

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
    Ensures that getting the scope leader fails if the scope hasn't been set with "ManagedLeaderElection"
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerSystemTests, GetScopeLeaderNoManagedElectionTest)
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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the default scope
    auto [GetScopesResult] = AWAIT_PRE(multiplayerSystem, GetScopesBySpace, RequestPredicate, space.Id);
    EXPECT_EQ(GetScopesResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (GetScopesResult.GetScopes().Size() != 1)
    {
        FAIL();
    }
    
    csp::systems::Scope defaultScope = GetScopesResult.GetScopes()[0];
    csp::common::String scopeId = defaultScope.Id;

    // Ensure managed leader election is turned off first
    {

        defaultScope.ManagedLeaderElection = false;
        auto [UpdateScopeResult] = AWAIT_PRE(multiplayerSystem, UpdateScopeById, RequestPredicate, scopeId, defaultScope);
        EXPECT_EQ(UpdateScopeResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Getting the scope leader should fail because the scope doesn't have managed leader election enabled.
    auto [GetScopeLeaderResult] = AWAIT_PRE(multiplayerSystem, GetScopeLeader, RequestPredicate, scopeId);
    EXPECT_EQ(GetScopeLeaderResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetScopeLeaderResult.GetHttpResultCode(), 400);

    // Clean up
    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}
