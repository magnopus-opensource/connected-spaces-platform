/*
 * Copyright 2026 Magnopus LLC

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

#include "../AssetSystemTestHelpers.h"
#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/PostprocessComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <filesystem>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, PostprocessTests, PostprocessComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Entity creation
    const csp::common::String EntityName = "Postprocess Entity";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Entity] = AWAIT(RealtimeEngine.get(), CreateEntity, EntityName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Process component creation
    auto* PostprocessComponent = (PostprocessSpaceComponent*)Entity->AddComponent(ComponentType::Postprocess);

    Entity->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& Components = *Entity->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    // Validate default properties
    EXPECT_EQ(PostprocessComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(PostprocessComponent->GetRotation(), csp::common::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_EQ(PostprocessComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(PostprocessComponent->GetExposure(), 100.0f);
    EXPECT_EQ(PostprocessComponent->GetIsUnbound(), true);

    // Validate application of properties - set position, rotation, scale, exposure and unbound, then check values are replicated back correctly.
    const csp::common::Vector3 NewPosition(100.0f, 200.0f, 300.0f);
    const csp::common::Vector4 NewRotation(0.0f, 0.707f, 0.0f, 0.707f);
    const csp::common::Vector3 NewScale(2.0f, 2.0f, 2.0f);
    const float NewExposure = 50.0f;
    const bool NewIsUnbound = false;

    PostprocessComponent->SetPosition(NewPosition);
    PostprocessComponent->SetRotation(NewRotation);
    PostprocessComponent->SetScale(NewScale);
    PostprocessComponent->SetExposure(NewExposure);
    PostprocessComponent->SetIsUnbound(NewIsUnbound);

    auto PostprocessComponentKey = PostprocessComponent->GetId();
    auto* FoundComponent = (PostprocessSpaceComponent*)Entity->GetComponent(PostprocessComponentKey);

    EXPECT_EQ(PostprocessComponent, FoundComponent);

    EXPECT_EQ(FoundComponent->GetPosition(), NewPosition);
    EXPECT_EQ(FoundComponent->GetRotation(), NewRotation);
    EXPECT_EQ(FoundComponent->GetScale(), NewScale);
    EXPECT_EQ(FoundComponent->GetExposure(), NewExposure);
    EXPECT_EQ(FoundComponent->GetIsUnbound(), NewIsUnbound);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PostprocessTests, PostprocessSpaceComponentScriptInterfaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    // Entity creation
    csp::common::String ObjectName = "Postprocess Entity";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [Entity] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Component creation
    auto* PostprocessComponent = (PostprocessSpaceComponent*)Entity->AddComponent(ComponentType::Postprocess);
    auto* ScriptComponent = (ScriptSpaceComponent*)Entity->AddComponent(ComponentType::ScriptData);
    Entity->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Script mutating properties
    std::string ScriptString = R"xx(
	
		var postprocess = ThisEntity.getPostprocessComponents()[0];
		postprocess.position = [100.0,200.0,300.0];
        postprocess.rotation = [0.0, 0.707, 0.0, 0.707];        
        postprocess.scale = [2.0, 2.0, 2.0];
		postprocess.exposure = 50.0;
		postprocess.isUnbound = false;
    )xx";

    // Invoking the script
    ScriptComponent->SetScriptSource(ScriptString.c_str());
    Entity->GetScript().Invoke();
    const bool ScriptHasErrors = Entity->GetScript().HasError();
    EXPECT_FALSE(ScriptHasErrors);
    RealtimeEngine->ProcessPendingEntityOperations();

    // Validating value application
    const csp::common::Vector3 NewPosition(100.0f, 200.0f, 300.0f);
    const csp::common::Vector4 NewRotation(0.0f, 0.707f, 0.0f, 0.707f);
    const csp::common::Vector3 NewScale(2.0f, 2.0f, 2.0f);
    const float NewExposure = 50.0f;
    const bool NewIsUnbound = false;

    EXPECT_EQ(PostprocessComponent->GetPosition(), NewPosition);
    EXPECT_EQ(PostprocessComponent->GetRotation(), NewRotation);
    EXPECT_EQ(PostprocessComponent->GetScale(), NewScale);
    EXPECT_EQ(PostprocessComponent->GetExposure(), NewExposure);
    EXPECT_EQ(PostprocessComponent->GetIsUnbound(), NewIsUnbound);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}