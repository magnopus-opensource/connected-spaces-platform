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

#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/ScreenSharingSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, ScreenSharingTests, ScreenSharingComponentTest)
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

    // Create parent Space Entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create screen sharing component
    auto* screenSharingComponent = static_cast<ScreenSharingSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScreenSharing));

    // Ensure defaults are set
    EXPECT_EQ(screenSharingComponent->GetUserId(), "");
    EXPECT_EQ(screenSharingComponent->GetDefaultImageCollectionId(), "");
    EXPECT_EQ(screenSharingComponent->GetDefaultImageAssetId(), "");
    EXPECT_EQ(screenSharingComponent->GetAttenuationRadius(), 10.f);

    EXPECT_EQ(screenSharingComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(screenSharingComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(screenSharingComponent->GetScale(), csp::common::Vector3::One());

    EXPECT_EQ(screenSharingComponent->GetIsVisible(), true);
    EXPECT_EQ(screenSharingComponent->GetIsARVisible(), true);
    EXPECT_EQ(screenSharingComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(screenSharingComponent->GetIsShadowCaster(), false);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Set new values
    csp::common::String screenSharingUserId = "SCREEN_SHARING_USER_ID";
    csp::common::String defaultImageCollectionId = "TEST_COLLECTION_ID";
    csp::common::String defaultImageAssetId = "TEST_ASSET_ID";
    float attenuationRadius = 22.0f;

    screenSharingComponent->SetUserId(screenSharingUserId);
    screenSharingComponent->SetDefaultImageCollectionId(defaultImageCollectionId);
    screenSharingComponent->SetDefaultImageAssetId(defaultImageAssetId);
    screenSharingComponent->SetAttenuationRadius(attenuationRadius);

    screenSharingComponent->SetPosition(csp::common::Vector3::One());
    screenSharingComponent->SetRotation(csp::common::Vector4::One());
    screenSharingComponent->SetScale(csp::common::Vector3::Zero());

    screenSharingComponent->SetIsVisible(false);
    screenSharingComponent->SetIsARVisible(false);
    screenSharingComponent->SetIsVirtualVisible(false);
    screenSharingComponent->SetIsShadowCaster(true);

    // Ensure values are set correctly
    EXPECT_EQ(screenSharingComponent->GetUserId(), screenSharingUserId);
    EXPECT_EQ(screenSharingComponent->GetDefaultImageCollectionId(), defaultImageCollectionId);
    EXPECT_EQ(screenSharingComponent->GetDefaultImageAssetId(), defaultImageAssetId);
    EXPECT_EQ(screenSharingComponent->GetAttenuationRadius(), attenuationRadius);

    EXPECT_EQ(screenSharingComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(screenSharingComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(screenSharingComponent->GetScale(), csp::common::Vector3::Zero());

    EXPECT_EQ(screenSharingComponent->GetIsVisible(), false);
    EXPECT_EQ(screenSharingComponent->GetIsARVisible(), false);
    EXPECT_EQ(screenSharingComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(screenSharingComponent->GetIsShadowCaster(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ScreenSharingTests, ScreenSharingComponentScriptTest)
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

    // Create parent Space Entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create screen sharing component
    auto* screenSharingComponent = (ScreenSharingSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScreenSharing);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string screenSharingScriptText = R"xx(
		var component = ThisEntity.getScreenSharingComponents()[0];
		component.userId = "ScreenSharingUserId";
		component.defaultImageCollectionId = "TestDefaultImageCollectionId";
		component.defaultImageAssetId = "TestDefaultImageAssetId";
		component.attenuationRadius = 22.0;
		component.position = [1, 1, 1];
		component.rotation = [1, 1, 1, 1];
		component.scale = [0, 0, 0];
		component.isVisible = false;
		component.isARVisible = false;
        component.isVirtualVisible = false;
		component.isShadowCaster = true;
    )xx";

    CreatedObject->GetScript().SetScriptSource(screenSharingScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Test new values
    EXPECT_EQ(screenSharingComponent->GetUserId(), "ScreenSharingUserId");
    EXPECT_EQ(screenSharingComponent->GetDefaultImageCollectionId(), "TestDefaultImageCollectionId");
    EXPECT_EQ(screenSharingComponent->GetDefaultImageAssetId(), "TestDefaultImageAssetId");
    EXPECT_EQ(screenSharingComponent->GetAttenuationRadius(), 22.0f);

    EXPECT_EQ(screenSharingComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(screenSharingComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(screenSharingComponent->GetScale(), csp::common::Vector3::Zero());

    EXPECT_EQ(screenSharingComponent->GetIsVisible(), false);
    EXPECT_EQ(screenSharingComponent->GetIsARVisible(), false);
    EXPECT_EQ(screenSharingComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(screenSharingComponent->GetIsShadowCaster(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}