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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, AnimatedModelTests, AnimatedModelComponentTest)
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

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        // Create parent entity
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        // Create animated model component
        auto* AnimatedModelComponent = (AnimatedModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::AnimatedModel);

        constexpr const char* TestExternalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
        constexpr const char* TestExternalResourceAssetId = "TestExternalResourceAssetId";
        constexpr const char* TestMaterialPath = "TestMaterialPath";
        constexpr const char* TestMaterialAssetId = "TestMaterialAssetId";
        const csp::common::Vector3 TestPosition(1.f, 1.f, 1.f);
        const csp::common::Vector4 TestRotation(1.f, 1.f, 1.f, 1.f);
        const csp::common::Vector3 TestScale(2.f, 2.f, 2.f);
        const SpaceTransform TestTransform(TestPosition, TestRotation, TestScale);
        constexpr const bool TestIsLoopPlayback = true;
        constexpr const bool TestIsPlaying = true;
        constexpr const int TestAnimationIndex = 1;
        constexpr const bool TestIsVisible = false;
        constexpr const bool TestIsARVisible = false;
        constexpr const bool TestIsVRVisible = false;
        constexpr const char* TestThirdPartyComponentRef = "TestThirdPartyComponentRef";
        constexpr const bool TestIsShadowCaster = false;
        constexpr const bool TestShowAsHoldout = true;
        constexpr const bool TestShowAsHoldoutInAR = true;
        constexpr const bool TestShowAsHoldoutInVR = true;

        // Test defaults
        EXPECT_EQ(AnimatedModelComponent->GetExternalResourceAssetCollectionId(), "");
        EXPECT_EQ(AnimatedModelComponent->GetExternalResourceAssetId(), "");
        EXPECT_EQ(AnimatedModelComponent->GetMaterialOverrides().Size(), 0);
        EXPECT_EQ(AnimatedModelComponent->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(AnimatedModelComponent->GetRotation(), csp::common::Vector4::Identity());
        EXPECT_EQ(AnimatedModelComponent->GetScale(), csp::common::Vector3::One());
        EXPECT_EQ(AnimatedModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());
        EXPECT_EQ(AnimatedModelComponent->GetIsLoopPlayback(), false);
        EXPECT_EQ(AnimatedModelComponent->GetIsPlaying(), false);
        EXPECT_EQ(AnimatedModelComponent->GetAnimationIndex(), -1);
        EXPECT_EQ(AnimatedModelComponent->GetIsVisible(), true);
        EXPECT_EQ(AnimatedModelComponent->GetIsARVisible(), true);
        EXPECT_EQ(AnimatedModelComponent->GetIsVRVisible(), true);
        EXPECT_EQ(AnimatedModelComponent->GetThirdPartyComponentRef(), "");
        EXPECT_EQ(AnimatedModelComponent->GetIsShadowCaster(), true);
        EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldout(), false);
        EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldoutInAR(), false);
        EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldoutInVR(), false);

        AnimatedModelComponent->SetExternalResourceAssetCollectionId(TestExternalResourceAssetCollectionId);
        AnimatedModelComponent->SetExternalResourceAssetId(TestExternalResourceAssetId);
        AnimatedModelComponent->AddMaterialOverride(TestMaterialPath, TestMaterialAssetId);
        AnimatedModelComponent->SetPosition(TestPosition);
        AnimatedModelComponent->SetRotation(TestRotation);
        AnimatedModelComponent->SetScale(TestScale);
        AnimatedModelComponent->SetIsLoopPlayback(TestIsLoopPlayback);
        AnimatedModelComponent->SetIsPlaying(TestIsPlaying);
        AnimatedModelComponent->SetAnimationIndex(TestAnimationIndex);
        AnimatedModelComponent->SetIsVisible(TestIsVisible);
        AnimatedModelComponent->SetIsARVisible(TestIsARVisible);
        AnimatedModelComponent->SetIsVRVisible(TestIsVRVisible);
        AnimatedModelComponent->SetThirdPartyComponentRef(TestThirdPartyComponentRef);
        AnimatedModelComponent->SetIsShadowCaster(TestIsShadowCaster);
        AnimatedModelComponent->SetShowAsHoldout(TestShowAsHoldout);
        AnimatedModelComponent->SetShowAsHoldoutInAR(TestShowAsHoldoutInAR);
        AnimatedModelComponent->SetShowAsHoldoutInVR(TestShowAsHoldoutInVR);

        // Test new values
        EXPECT_EQ(AnimatedModelComponent->GetExternalResourceAssetCollectionId(), TestExternalResourceAssetCollectionId);
        EXPECT_EQ(AnimatedModelComponent->GetExternalResourceAssetId(), TestExternalResourceAssetId);
        EXPECT_EQ(AnimatedModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(AnimatedModelComponent->GetMaterialOverrides().HasKey(TestMaterialPath));
        EXPECT_EQ(AnimatedModelComponent->GetPosition(), TestPosition);
        EXPECT_EQ(AnimatedModelComponent->GetRotation(), TestRotation);
        EXPECT_EQ(AnimatedModelComponent->GetScale(), TestScale);
        EXPECT_EQ(AnimatedModelComponent->GetIsLoopPlayback(), TestIsLoopPlayback);
        EXPECT_EQ(AnimatedModelComponent->GetIsPlaying(), TestIsPlaying);
        EXPECT_EQ(AnimatedModelComponent->GetAnimationIndex(), TestAnimationIndex);
        EXPECT_EQ(AnimatedModelComponent->GetIsVisible(), TestIsVisible);
        EXPECT_EQ(AnimatedModelComponent->GetIsARVisible(), TestIsARVisible);
        EXPECT_EQ(AnimatedModelComponent->GetIsVRVisible(), TestIsVRVisible);
        EXPECT_EQ(AnimatedModelComponent->GetThirdPartyComponentRef(), TestThirdPartyComponentRef);
        EXPECT_EQ(AnimatedModelComponent->GetIsShadowCaster(), TestIsShadowCaster);
        EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldout(), TestShowAsHoldout);
        EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldoutInAR(), TestShowAsHoldoutInAR);
        EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldoutInVR(), TestShowAsHoldoutInVR);

        // Test transform separately, as this just sets position, rotation, scale
        AnimatedModelComponent->SetTransform(csp::multiplayer::SpaceTransform());

        EXPECT_EQ(AnimatedModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());

        AnimatedModelComponent->SetTransform(TestTransform);

        EXPECT_EQ(AnimatedModelComponent->GetTransform(), TestTransform);

        // Also test we can remove a material override
        AnimatedModelComponent->RemoveMaterialOverride(TestMaterialPath);

        EXPECT_EQ(AnimatedModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnimatedModelTests, AnimatedModelScriptInterfaceTest)
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

    // Create parent entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create animated model component
    auto* AnimatedModelComponent = (AnimatedModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::AnimatedModel);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string AnimatedModelScriptText = R"xx(
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
        model.isVRVisible = false;
        model.showAsHoldout = true;
        model.showAsHoldoutInAR = true;
        model.showAsHoldoutInVR = true;
        model.showAsHoldoutInXR = true;
		model.animationIndex = 1;
    )xx";

    CreatedObject->GetScript().SetScriptSource(AnimatedModelScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    // Test new values
    EXPECT_EQ(AnimatedModelComponent->GetExternalResourceAssetCollectionId(), "TestExternalResourceAssetCollectionId");
    EXPECT_EQ(AnimatedModelComponent->GetExternalResourceAssetId(), "TestExternalResourceAssetId");
    EXPECT_EQ(AnimatedModelComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(AnimatedModelComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(AnimatedModelComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(AnimatedModelComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(AnimatedModelComponent->GetIsPlaying(), false);
    EXPECT_EQ(AnimatedModelComponent->GetIsVisible(), false);
    EXPECT_EQ(AnimatedModelComponent->GetIsARVisible(), false);
    EXPECT_EQ(AnimatedModelComponent->GetIsVRVisible(), false);
    EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldout(), true);
    EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldoutInAR(), true);
    EXPECT_EQ(AnimatedModelComponent->GetShowAsHoldoutInVR(), true);
    EXPECT_EQ(AnimatedModelComponent->GetAnimationIndex(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnimatedModelTests, AnimatedModelComponentEnterSpaceTest)
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

    csp::common::String ObjectName = "Object 1";

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        // Create animated model component
        auto* AnimatedModelComponent = (AnimatedModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::AnimatedModel);
        AnimatedModelComponent->AddMaterialOverride("TestKey", "TestValue");

        CreatedObject->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    std::this_thread::sleep_for(std::chrono::seconds(7));

    {
        // Re-enter space
        bool EntitiesCreated = false;

        auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
        ASSERT_TRUE(EntitiesCreated);

        SpaceEntity* FoundEntity = RealtimeEngine->FindSpaceObject(ObjectName);
        ASSERT_TRUE(FoundEntity != nullptr);

        auto* AnimatedModelComponent = (AnimatedModelSpaceComponent*)FoundEntity->GetComponent(0);
        ASSERT_TRUE(AnimatedModelComponent != nullptr);

        EXPECT_EQ(AnimatedModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(AnimatedModelComponent->GetMaterialOverrides().HasKey("TestKey"));
        EXPECT_EQ(AnimatedModelComponent->GetMaterialOverrides()["TestKey"], "TestValue");

        // Delete material override
        AnimatedModelComponent->RemoveMaterialOverride("TestKey");

        FoundEntity->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Ensure component data has been written to database by chs before entering the space again
        // This is due to an enforced 2 second chs database write delay
        std::this_thread::sleep_for(7s);
    }

    {
        // Re-enter space
        bool EntitiesCreated = false;

        auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
        ASSERT_TRUE(EntitiesCreated);

        SpaceEntity* FoundEntity = RealtimeEngine->FindSpaceObject(ObjectName);
        ASSERT_TRUE(FoundEntity != nullptr);

        auto* AnimatedModelComponent = (AnimatedModelSpaceComponent*)FoundEntity->GetComponent(0);
        ASSERT_TRUE(AnimatedModelComponent != nullptr);

        EXPECT_EQ(AnimatedModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}