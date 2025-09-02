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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, ScreenSharingTests, ScreenSharingComponentTest)
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

    RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent Space Entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create screen sharing component
    auto* ScreenSharingComponent = static_cast<ScreenSharingSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScreenSharing));

    // Ensure defaults are set
    EXPECT_EQ(ScreenSharingComponent->GetUserId(), "");
    EXPECT_EQ(ScreenSharingComponent->GetDefaultImageCollectionId(), "");
    EXPECT_EQ(ScreenSharingComponent->GetDefaultImageAssetId(), "");
    EXPECT_EQ(ScreenSharingComponent->GetAttenuationRadius(), 10.f);

    EXPECT_EQ(ScreenSharingComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(ScreenSharingComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(ScreenSharingComponent->GetScale(), csp::common::Vector3::One());

    EXPECT_EQ(ScreenSharingComponent->GetIsVisible(), true);
    EXPECT_EQ(ScreenSharingComponent->GetIsARVisible(), true);
    EXPECT_EQ(ScreenSharingComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(ScreenSharingComponent->GetIsShadowCaster(), false);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Set new values
    csp::common::String ScreenSharingUserId = "SCREEN_SHARING_USER_ID";
    csp::common::String DefaultImageCollectionId = "TEST_COLLECTION_ID";
    csp::common::String DefaultImageAssetId = "TEST_ASSET_ID";
    float AttenuationRadius = 22.0f;

    ScreenSharingComponent->SetUserId(ScreenSharingUserId);
    ScreenSharingComponent->SetDefaultImageCollectionId(DefaultImageCollectionId);
    ScreenSharingComponent->SetDefaultImageAssetId(DefaultImageAssetId);
    ScreenSharingComponent->SetAttenuationRadius(AttenuationRadius);

    ScreenSharingComponent->SetPosition(csp::common::Vector3::One());
    ScreenSharingComponent->SetRotation(csp::common::Vector4::One());
    ScreenSharingComponent->SetScale(csp::common::Vector3::Zero());

    ScreenSharingComponent->SetIsVisible(false);
    ScreenSharingComponent->SetIsARVisible(false);
    ScreenSharingComponent->SetIsVirtualVisible(false);
    ScreenSharingComponent->SetIsShadowCaster(true);

    // Ensure values are set correctly
    EXPECT_EQ(ScreenSharingComponent->GetUserId(), ScreenSharingUserId);
    EXPECT_EQ(ScreenSharingComponent->GetDefaultImageCollectionId(), DefaultImageCollectionId);
    EXPECT_EQ(ScreenSharingComponent->GetDefaultImageAssetId(), DefaultImageAssetId);
    EXPECT_EQ(ScreenSharingComponent->GetAttenuationRadius(), AttenuationRadius);

    EXPECT_EQ(ScreenSharingComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(ScreenSharingComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(ScreenSharingComponent->GetScale(), csp::common::Vector3::Zero());

    EXPECT_EQ(ScreenSharingComponent->GetIsVisible(), false);
    EXPECT_EQ(ScreenSharingComponent->GetIsARVisible(), false);
    EXPECT_EQ(ScreenSharingComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(ScreenSharingComponent->GetIsShadowCaster(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ScreenSharingTests, ScreenSharingComponentScriptTest)
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

    RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent Space Entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create screen sharing component
    auto* ScreenSharingComponent = (ScreenSharingSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScreenSharing);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string ScreenSharingScriptText = R"xx(
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

    CreatedObject->GetScript().SetScriptSource(ScreenSharingScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    // Test new values
    EXPECT_EQ(ScreenSharingComponent->GetUserId(), "ScreenSharingUserId");
    EXPECT_EQ(ScreenSharingComponent->GetDefaultImageCollectionId(), "TestDefaultImageCollectionId");
    EXPECT_EQ(ScreenSharingComponent->GetDefaultImageAssetId(), "TestDefaultImageAssetId");
    EXPECT_EQ(ScreenSharingComponent->GetAttenuationRadius(), 22.0f);

    EXPECT_EQ(ScreenSharingComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(ScreenSharingComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(ScreenSharingComponent->GetScale(), csp::common::Vector3::Zero());

    EXPECT_EQ(ScreenSharingComponent->GetIsVisible(), false);
    EXPECT_EQ(ScreenSharingComponent->GetIsARVisible(), false);
    EXPECT_EQ(ScreenSharingComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(ScreenSharingComponent->GetIsShadowCaster(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}