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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, GaussianSplatTests, GaussianSplatTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    csp::common::String callbackAssetId;

    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String modelAssetId = "NotARealId";

    auto* gaussianSplatComponent = (GaussianSplatSpaceComponent*)Object->AddComponent(ComponentType::GaussianSplat);

    // Process component creation
    Object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& components = *Object->GetComponents();
    EXPECT_EQ(components.Size(), 1);

    EXPECT_EQ(gaussianSplatComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(gaussianSplatComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(gaussianSplatComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(gaussianSplatComponent->GetIsVisible(), true);
    EXPECT_EQ(gaussianSplatComponent->GetIsARVisible(), true);
    EXPECT_EQ(gaussianSplatComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(gaussianSplatComponent->GetIsShadowCaster(), true);
    EXPECT_EQ(gaussianSplatComponent->GetTint(), csp::common::Vector3::One());

    gaussianSplatComponent->SetPosition(csp::common::Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(gaussianSplatComponent->GetPosition(), csp::common::Vector3(1.0f, 2.0f, 3.0f));

    gaussianSplatComponent->SetRotation(csp::common::Vector4(0.3f, 0.2f, 0.7f, 0.4f));
    EXPECT_EQ(gaussianSplatComponent->GetRotation(), csp::common::Vector4(0.3f, 0.2f, 0.7f, 0.4f));

    gaussianSplatComponent->SetScale(csp::common::Vector3(2.0f, 4.0f, 6.0f));
    EXPECT_EQ(gaussianSplatComponent->GetScale(), csp::common::Vector3(2.0f, 4.0f, 6.0f));

    gaussianSplatComponent->SetIsVisible(false);
    EXPECT_EQ(gaussianSplatComponent->GetIsVisible(), false);

    gaussianSplatComponent->SetIsARVisible(false);
    EXPECT_EQ(gaussianSplatComponent->GetIsARVisible(), false);

    gaussianSplatComponent->SetIsVirtualVisible(false);
    EXPECT_EQ(gaussianSplatComponent->GetIsVirtualVisible(), false);

    gaussianSplatComponent->SetIsShadowCaster(false);
    EXPECT_EQ(gaussianSplatComponent->GetIsShadowCaster(), false);

    gaussianSplatComponent->SetTint(csp::common::Vector3(1.0f, 0.4f, 0.0f));
    EXPECT_EQ(gaussianSplatComponent->GetTint(), csp::common::Vector3(1.0f, 0.4f, 0.0f));

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, GaussianSplatTests, GaussianSplatScriptInterfaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    auto* gaussianSplatComponent = (GaussianSplatSpaceComponent*)CreatedObject->AddComponent(ComponentType::GaussianSplat);
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(gaussianSplatComponent->GetIsVisible(), true);

    // Setup script
    const std::string scriptSource = R"xx(
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

    scriptComponent->SetScriptSource(scriptSource.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);

    EXPECT_EQ(gaussianSplatComponent->GetExternalResourceAssetCollectionId(), "TestExternalResourceAssetCollectionId");
    EXPECT_EQ(gaussianSplatComponent->GetExternalResourceAssetId(), "TestExternalResourceAssetId");
    EXPECT_EQ(gaussianSplatComponent->GetTint(), csp::common::Vector3(0.0f, 0.1f, 0.2f));
    EXPECT_EQ(gaussianSplatComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(gaussianSplatComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(gaussianSplatComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(gaussianSplatComponent->GetIsVisible(), false);
    EXPECT_EQ(gaussianSplatComponent->GetIsARVisible(), false);
    EXPECT_EQ(gaussianSplatComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}