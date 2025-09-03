/*
 * Copyright 2023 Magnopus LLC

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
#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
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

CSP_PUBLIC_TEST(CSPEngine, GaussianSplatTests, GaussianSplatTest)
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

    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String ModelAssetId = "NotARealId";

    auto* GaussianSplatComponent = (GaussianSplatSpaceComponent*)Object->AddComponent(ComponentType::GaussianSplat);

    // Process component creation
    Object->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& Components = *Object->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    EXPECT_EQ(GaussianSplatComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(GaussianSplatComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(GaussianSplatComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(GaussianSplatComponent->GetIsVisible(), true);
    EXPECT_EQ(GaussianSplatComponent->GetIsARVisible(), true);
    EXPECT_EQ(GaussianSplatComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(GaussianSplatComponent->GetIsShadowCaster(), true);
    EXPECT_EQ(GaussianSplatComponent->GetTint(), csp::common::Vector3::One());

    GaussianSplatComponent->SetPosition(csp::common::Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(GaussianSplatComponent->GetPosition(), csp::common::Vector3(1.0f, 2.0f, 3.0f));

    GaussianSplatComponent->SetRotation(csp::common::Vector4(0.3f, 0.2f, 0.7f, 0.4f));
    EXPECT_EQ(GaussianSplatComponent->GetRotation(), csp::common::Vector4(0.3f, 0.2f, 0.7f, 0.4f));

    GaussianSplatComponent->SetScale(csp::common::Vector3(2.0f, 4.0f, 6.0f));
    EXPECT_EQ(GaussianSplatComponent->GetScale(), csp::common::Vector3(2.0f, 4.0f, 6.0f));

    GaussianSplatComponent->SetIsVisible(false);
    EXPECT_EQ(GaussianSplatComponent->GetIsVisible(), false);

    GaussianSplatComponent->SetIsARVisible(false);
    EXPECT_EQ(GaussianSplatComponent->GetIsARVisible(), false);

    GaussianSplatComponent->SetIsVirtualVisible(false);
    EXPECT_EQ(GaussianSplatComponent->GetIsVirtualVisible(), false);

    GaussianSplatComponent->SetIsShadowCaster(false);
    EXPECT_EQ(GaussianSplatComponent->GetIsShadowCaster(), false);

    GaussianSplatComponent->SetTint(csp::common::Vector3(1.0f, 0.4f, 0.0f));
    EXPECT_EQ(GaussianSplatComponent->GetTint(), csp::common::Vector3(1.0f, 0.4f, 0.0f));

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, GaussianSplatTests, GaussianSplatScriptInterfaceTest)
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

    // Create parent entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    auto* GaussianSplatComponent = (GaussianSplatSpaceComponent*)CreatedObject->AddComponent(ComponentType::GaussianSplat);
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(GaussianSplatComponent->GetIsVisible(), true);

    // Setup script
    const std::string ScriptSource = R"xx(
		var splat = ThisEntity.getGaussianSplatComponents()[0];
        splat.externalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
		splat.externalResourceAssetId = "TestExternalResourceAssetId";
		splat.tint = [0.0, 0.1, 0.2];
        splat.position = [1, 1, 1];
        splat.scale = [2, 2, 2];
		splat.rotation = [1, 1, 1, 1];
        splat.isVisible = false;
        splat.isARVisible = false;
        splat.isVirtualVisible = false;
    )xx";

    ScriptComponent->SetScriptSource(ScriptSource.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    const bool ScriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(ScriptHasErrors);

    EXPECT_EQ(GaussianSplatComponent->GetExternalResourceAssetCollectionId(), "TestExternalResourceAssetCollectionId");
    EXPECT_EQ(GaussianSplatComponent->GetExternalResourceAssetId(), "TestExternalResourceAssetId");
    EXPECT_EQ(GaussianSplatComponent->GetTint(), csp::common::Vector3(0.0f, 0.1f, 0.2f));
    EXPECT_EQ(GaussianSplatComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(GaussianSplatComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(GaussianSplatComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(GaussianSplatComponent->GetIsVisible(), false);
    EXPECT_EQ(GaussianSplatComponent->GetIsARVisible(), false);
    EXPECT_EQ(GaussianSplatComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}