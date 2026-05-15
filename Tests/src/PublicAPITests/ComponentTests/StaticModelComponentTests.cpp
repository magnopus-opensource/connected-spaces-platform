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
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, StaticModelTests, StaticModelComponentTest)
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

        csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create custom component
        auto* staticModelComponent = (StaticModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::StaticModel);

        constexpr const char* testExternalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
        constexpr const char* testExternalResourceAssetId = "TestExternalResourceAssetId";
        constexpr const char* testMaterialPath = "TestMaterialPath";
        constexpr const char* testMaterialAssetId = "TestMaterialAssetId";
        const csp::common::Vector3 testPosition(1.f, 1.f, 1.f);
        const csp::common::Vector4 testRotation(1.f, 1.f, 1.f, 1.f);
        const csp::common::Vector3 testScale(2.f, 2.f, 2.f);
        const SpaceTransform testTransform(testPosition, testRotation, testScale);
        constexpr const bool testIsVisible = false;
        constexpr const bool testIsArVisible = false;
        constexpr const bool testIsVirtualVisible = false;
        constexpr const char* testThirdPartyComponentRef = "TestThirdPartyComponentRef";
        constexpr const bool testIsShadowCaster = false;
        constexpr const bool testShowAsHoldoutInAr = true;
        constexpr const bool testShowAsHoldoutInVirtual = true;

        // Test defaults
        EXPECT_EQ(staticModelComponent->GetExternalResourceAssetCollectionId(), "");
        EXPECT_EQ(staticModelComponent->GetExternalResourceAssetId(), "");
        EXPECT_EQ(staticModelComponent->GetMaterialOverrides().Size(), 0);
        EXPECT_EQ(staticModelComponent->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(staticModelComponent->GetRotation(), csp::common::Vector4::Identity());
        EXPECT_EQ(staticModelComponent->GetScale(), csp::common::Vector3::One());
        EXPECT_EQ(staticModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());
        EXPECT_EQ(staticModelComponent->GetIsVisible(), true);
        EXPECT_EQ(staticModelComponent->GetIsARVisible(), true);
        EXPECT_EQ(staticModelComponent->GetIsVirtualVisible(), true);
        EXPECT_EQ(staticModelComponent->GetThirdPartyComponentRef(), "");
        EXPECT_EQ(staticModelComponent->GetIsShadowCaster(), true);
        EXPECT_EQ(staticModelComponent->GetShowAsHoldoutInAR(), false);
        EXPECT_EQ(staticModelComponent->GetShowAsHoldoutInVirtual(), false);

        staticModelComponent->SetExternalResourceAssetCollectionId(testExternalResourceAssetCollectionId);
        staticModelComponent->SetExternalResourceAssetId(testExternalResourceAssetId);
        staticModelComponent->AddMaterialOverride(testMaterialPath, testMaterialAssetId);
        staticModelComponent->SetPosition(testPosition);
        staticModelComponent->SetRotation(testRotation);
        staticModelComponent->SetScale(testScale);
        staticModelComponent->SetIsVisible(testIsVisible);
        staticModelComponent->SetIsARVisible(testIsArVisible);
        staticModelComponent->SetIsVirtualVisible(testIsVirtualVisible);
        staticModelComponent->SetThirdPartyComponentRef(testThirdPartyComponentRef);
        staticModelComponent->SetIsShadowCaster(testIsShadowCaster);
        staticModelComponent->SetShowAsHoldoutInAR(testShowAsHoldoutInAr);
        staticModelComponent->SetShowAsHoldoutInVirtual(testShowAsHoldoutInVirtual);

        // Test new values
        EXPECT_EQ(staticModelComponent->GetExternalResourceAssetCollectionId(), testExternalResourceAssetCollectionId);
        EXPECT_EQ(staticModelComponent->GetExternalResourceAssetId(), testExternalResourceAssetId);
        EXPECT_EQ(staticModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(staticModelComponent->GetMaterialOverrides().HasKey(testMaterialPath));
        EXPECT_EQ(staticModelComponent->GetPosition(), testPosition);
        EXPECT_EQ(staticModelComponent->GetRotation(), testRotation);
        EXPECT_EQ(staticModelComponent->GetScale(), testScale);
        EXPECT_EQ(staticModelComponent->GetIsVisible(), testIsVisible);
        EXPECT_EQ(staticModelComponent->GetIsARVisible(), testIsArVisible);
        EXPECT_EQ(staticModelComponent->GetIsVirtualVisible(), testIsVirtualVisible);
        EXPECT_EQ(staticModelComponent->GetThirdPartyComponentRef(), testThirdPartyComponentRef);
        EXPECT_EQ(staticModelComponent->GetIsShadowCaster(), testIsShadowCaster);
        EXPECT_EQ(staticModelComponent->GetShowAsHoldoutInAR(), testShowAsHoldoutInAr);
        EXPECT_EQ(staticModelComponent->GetShowAsHoldoutInVirtual(), testShowAsHoldoutInVirtual);

        // Test transform separately, as this just sets position, rotation, scale
        staticModelComponent->SetTransform(csp::multiplayer::SpaceTransform());

        EXPECT_EQ(staticModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());

        staticModelComponent->SetTransform(testTransform);

        EXPECT_EQ(staticModelComponent->GetTransform(), testTransform);

        // Also test we can remove a material override
        staticModelComponent->RemoveMaterialOverride(testMaterialPath);

        EXPECT_EQ(staticModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, StaticModelTests, StaticModelScriptInterfaceTest)
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

    // Create static model component
    auto* staticModelComponent = (StaticModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::StaticModel);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    ASSERT_EQ(staticModelComponent->GetIsShadowCaster(), true);

    // Setup script
    const std::string staticModelScriptText = R"xx(
		var model = ThisEntity.getStaticModelComponents()[0];
		model.externalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
		model.externalResourceAssetId = "TestExternalResourceAssetId";
		model.position = [1, 1, 1];
        model.scale = [2, 2, 2];
		model.rotation = [1, 1, 1, 1];
		model.isVisible = false;
        model.isARVisible = false;
        model.isVirtualVisible = false;
        model.showAsHoldoutInAR = true;
        model.showAsHoldoutInVirtual = true;
        model.isShadowCaster = false;
    )xx";

    CreatedObject->GetScript().SetScriptSource(staticModelScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Test new values
    EXPECT_EQ(staticModelComponent->GetExternalResourceAssetCollectionId(), "TestExternalResourceAssetCollectionId");
    EXPECT_EQ(staticModelComponent->GetExternalResourceAssetId(), "TestExternalResourceAssetId");
    EXPECT_EQ(staticModelComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(staticModelComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(staticModelComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(staticModelComponent->GetIsVisible(), false);
    EXPECT_EQ(staticModelComponent->GetIsARVisible(), false);
    EXPECT_EQ(staticModelComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(staticModelComponent->GetShowAsHoldoutInAR(), true);
    EXPECT_EQ(staticModelComponent->GetShowAsHoldoutInVirtual(), true);
    EXPECT_EQ(staticModelComponent->GetIsShadowCaster(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, StaticModelTests, StaticModelComponentEnterSpaceTest)
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

        // Create static model component
        auto* staticModelComponent = (StaticModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::StaticModel);
        staticModelComponent->AddMaterialOverride("TestKey", "TestValue");

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
        EXPECT_TRUE(entitiesCreated);

        SpaceEntity* foundEntity = realtimeEngine->FindSpaceObject(objectName);
        EXPECT_TRUE(foundEntity != nullptr);

        auto* staticModelComponent = (StaticModelSpaceComponent*)foundEntity->GetComponent(0);
        EXPECT_TRUE(staticModelComponent != nullptr);

        EXPECT_EQ(staticModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(staticModelComponent->GetMaterialOverrides().HasKey("TestKey"));
        EXPECT_EQ(staticModelComponent->GetMaterialOverrides()["TestKey"], "TestValue");

        // Delete material override
        staticModelComponent->RemoveMaterialOverride("TestKey");

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
        EXPECT_TRUE(entitiesCreated);

        SpaceEntity* foundEntity = realtimeEngine->FindSpaceObject(objectName);
        EXPECT_TRUE(foundEntity != nullptr);

        auto* staticModelComponent = (StaticModelSpaceComponent*)foundEntity->GetComponent(0);
        EXPECT_TRUE(staticModelComponent != nullptr);

        EXPECT_EQ(staticModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}