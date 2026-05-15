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

#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
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

CSP_PUBLIC_TEST(CSPEngine, AnimatedModelTests, AnimatedModelComponentTest)
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

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        // Create parent entity
        csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create animated model component
        auto* animatedModelComponent = (AnimatedModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::AnimatedModel);

        constexpr const char* testExternalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
        constexpr const char* testExternalResourceAssetId = "TestExternalResourceAssetId";
        constexpr const char* testMaterialPath = "TestMaterialPath";
        constexpr const char* testMaterialAssetId = "TestMaterialAssetId";
        const csp::common::Vector3 testPosition(1.f, 1.f, 1.f);
        const csp::common::Vector4 testRotation(1.f, 1.f, 1.f, 1.f);
        const csp::common::Vector3 testScale(2.f, 2.f, 2.f);
        const SpaceTransform testTransform(testPosition, testRotation, testScale);
        constexpr const bool testIsLoopPlayback = true;
        constexpr const bool testIsPlaying = true;
        constexpr const int testAnimationIndex = 1;
        constexpr const bool testIsVisible = false;
        constexpr const bool testIsArVisible = false;
        constexpr const bool testIsVirtualVisible = false;
        constexpr const char* testThirdPartyComponentRef = "TestThirdPartyComponentRef";
        constexpr const bool testIsShadowCaster = false;
        constexpr const bool testShowAsHoldoutInAr = true;
        constexpr const bool testShowAsHoldoutInVirtual = true;

        // Test defaults
        EXPECT_EQ(animatedModelComponent->GetExternalResourceAssetCollectionId(), "");
        EXPECT_EQ(animatedModelComponent->GetExternalResourceAssetId(), "");
        EXPECT_EQ(animatedModelComponent->GetMaterialOverrides().Size(), 0);
        EXPECT_EQ(animatedModelComponent->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(animatedModelComponent->GetRotation(), csp::common::Vector4::Identity());
        EXPECT_EQ(animatedModelComponent->GetScale(), csp::common::Vector3::One());
        EXPECT_EQ(animatedModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());
        EXPECT_EQ(animatedModelComponent->GetIsLoopPlayback(), false);
        EXPECT_EQ(animatedModelComponent->GetIsPlaying(), false);
        EXPECT_EQ(animatedModelComponent->GetAnimationIndex(), -1);
        EXPECT_EQ(animatedModelComponent->GetIsVisible(), true);
        EXPECT_EQ(animatedModelComponent->GetIsARVisible(), true);
        EXPECT_EQ(animatedModelComponent->GetIsVirtualVisible(), true);
        EXPECT_EQ(animatedModelComponent->GetThirdPartyComponentRef(), "");
        EXPECT_EQ(animatedModelComponent->GetIsShadowCaster(), true);
        EXPECT_EQ(animatedModelComponent->GetShowAsHoldoutInAR(), false);
        EXPECT_EQ(animatedModelComponent->GetShowAsHoldoutInVirtual(), false);

        animatedModelComponent->SetExternalResourceAssetCollectionId(testExternalResourceAssetCollectionId);
        animatedModelComponent->SetExternalResourceAssetId(testExternalResourceAssetId);
        animatedModelComponent->AddMaterialOverride(testMaterialPath, testMaterialAssetId);
        animatedModelComponent->SetPosition(testPosition);
        animatedModelComponent->SetRotation(testRotation);
        animatedModelComponent->SetScale(testScale);
        animatedModelComponent->SetIsLoopPlayback(testIsLoopPlayback);
        animatedModelComponent->SetIsPlaying(testIsPlaying);
        animatedModelComponent->SetAnimationIndex(testAnimationIndex);
        animatedModelComponent->SetIsVisible(testIsVisible);
        animatedModelComponent->SetIsARVisible(testIsArVisible);
        animatedModelComponent->SetIsVirtualVisible(testIsVirtualVisible);
        animatedModelComponent->SetThirdPartyComponentRef(testThirdPartyComponentRef);
        animatedModelComponent->SetIsShadowCaster(testIsShadowCaster);
        animatedModelComponent->SetShowAsHoldoutInAR(testShowAsHoldoutInAr);
        animatedModelComponent->SetShowAsHoldoutInVirtual(testShowAsHoldoutInVirtual);

        // Test new values
        EXPECT_EQ(animatedModelComponent->GetExternalResourceAssetCollectionId(), testExternalResourceAssetCollectionId);
        EXPECT_EQ(animatedModelComponent->GetExternalResourceAssetId(), testExternalResourceAssetId);
        EXPECT_EQ(animatedModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(animatedModelComponent->GetMaterialOverrides().HasKey(testMaterialPath));
        EXPECT_EQ(animatedModelComponent->GetPosition(), testPosition);
        EXPECT_EQ(animatedModelComponent->GetRotation(), testRotation);
        EXPECT_EQ(animatedModelComponent->GetScale(), testScale);
        EXPECT_EQ(animatedModelComponent->GetIsLoopPlayback(), testIsLoopPlayback);
        EXPECT_EQ(animatedModelComponent->GetIsPlaying(), testIsPlaying);
        EXPECT_EQ(animatedModelComponent->GetAnimationIndex(), testAnimationIndex);
        EXPECT_EQ(animatedModelComponent->GetIsVisible(), testIsVisible);
        EXPECT_EQ(animatedModelComponent->GetIsARVisible(), testIsArVisible);
        EXPECT_EQ(animatedModelComponent->GetIsVirtualVisible(), testIsVirtualVisible);
        EXPECT_EQ(animatedModelComponent->GetThirdPartyComponentRef(), testThirdPartyComponentRef);
        EXPECT_EQ(animatedModelComponent->GetIsShadowCaster(), testIsShadowCaster);
        EXPECT_EQ(animatedModelComponent->GetShowAsHoldoutInAR(), testShowAsHoldoutInAr);
        EXPECT_EQ(animatedModelComponent->GetShowAsHoldoutInVirtual(), testShowAsHoldoutInVirtual);

        // Test transform separately, as this just sets position, rotation, scale
        animatedModelComponent->SetTransform(csp::multiplayer::SpaceTransform());

        EXPECT_EQ(animatedModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());

        animatedModelComponent->SetTransform(testTransform);

        EXPECT_EQ(animatedModelComponent->GetTransform(), testTransform);

        // Also test we can remove a material override
        animatedModelComponent->RemoveMaterialOverride(testMaterialPath);

        EXPECT_EQ(animatedModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnimatedModelTests, AnimatedModelScriptInterfaceTest)
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

    // Create animated model component
    auto* animatedModelComponent = (AnimatedModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::AnimatedModel);

    ASSERT_EQ(animatedModelComponent->GetIsShadowCaster(), true);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string animatedModelScriptText = R"xx(
		var model = ThisEntity.getAnimatedModelComponents()[0];
		model.externalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
		model.externalResourceAssetId = "TestExternalResourceAssetId";
		model.position = [1, 1, 1];
		model.scale = [2, 2, 2];
        model.rotation = [1, 1, 1, 1];
		model.isLoopPlayback = false;
		model.isPlaying = false;
		model.isVisible = false;
        model.isARVisible = false;
        model.isVirtualVisible = false;
        model.showAsHoldoutInAR = true;
        model.showAsHoldoutInVirtual = true;
		model.animationIndex = 1;
        model.isShadowCaster = false;
    )xx";

    CreatedObject->GetScript().SetScriptSource(animatedModelScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Test new values
    EXPECT_EQ(animatedModelComponent->GetExternalResourceAssetCollectionId(), "TestExternalResourceAssetCollectionId");
    EXPECT_EQ(animatedModelComponent->GetExternalResourceAssetId(), "TestExternalResourceAssetId");
    EXPECT_EQ(animatedModelComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(animatedModelComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(animatedModelComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(animatedModelComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(animatedModelComponent->GetIsPlaying(), false);
    EXPECT_EQ(animatedModelComponent->GetIsVisible(), false);
    EXPECT_EQ(animatedModelComponent->GetIsARVisible(), false);
    EXPECT_EQ(animatedModelComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(animatedModelComponent->GetShowAsHoldoutInAR(), true);
    EXPECT_EQ(animatedModelComponent->GetShowAsHoldoutInVirtual(), true);
    EXPECT_EQ(animatedModelComponent->GetAnimationIndex(), 1);
    EXPECT_EQ(animatedModelComponent->GetIsShadowCaster(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnimatedModelTests, AnimatedModelComponentEnterSpaceTest)
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

    csp::common::String objectName = "Object 1";

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create animated model component
        auto* animatedModelComponent = (AnimatedModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::AnimatedModel);
        animatedModelComponent->AddMaterialOverride("TestKey", "TestValue");

        CreatedObject->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    std::this_thread::sleep_for(std::chrono::seconds(7));

    {
        // Re-enter space
        bool entitiesCreated = false;

        auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());
        ASSERT_TRUE(entitiesCreated);

        SpaceEntity* foundEntity = realtimeEngine->FindSpaceObject(objectName);
        ASSERT_TRUE(foundEntity != nullptr);

        auto* animatedModelComponent = (AnimatedModelSpaceComponent*)foundEntity->GetComponent(0);
        ASSERT_TRUE(animatedModelComponent != nullptr);

        EXPECT_EQ(animatedModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(animatedModelComponent->GetMaterialOverrides().HasKey("TestKey"));
        EXPECT_EQ(animatedModelComponent->GetMaterialOverrides()["TestKey"], "TestValue");

        // Delete material override
        animatedModelComponent->RemoveMaterialOverride("TestKey");

        foundEntity->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        // Ensure component data has been written to database by chs before entering the space again
        // This is due to an enforced 2 second chs database write delay
        std::this_thread::sleep_for(7s);
    }

    {
        // Re-enter space
        bool entitiesCreated = false;

        auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());
        ASSERT_TRUE(entitiesCreated);

        SpaceEntity* foundEntity = realtimeEngine->FindSpaceObject(objectName);
        ASSERT_TRUE(foundEntity != nullptr);

        auto* animatedModelComponent = (AnimatedModelSpaceComponent*)foundEntity->GetComponent(0);
        ASSERT_TRUE(animatedModelComponent != nullptr);

        EXPECT_EQ(animatedModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}