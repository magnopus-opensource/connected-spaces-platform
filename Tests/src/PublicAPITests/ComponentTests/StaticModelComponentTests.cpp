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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_STATIC_MODEL_TESTS || RUN_STATIC_MODEL_TEST
CSP_PUBLIC_TEST(CSPEngine, StaticModelTests, StaticModelComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create custom component
        auto* StaticModelComponent = (StaticModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::StaticModel);

        constexpr const char* TestExternalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
        constexpr const char* TestExternalResourceAssetId = "TestExternalResourceAssetId";
        constexpr const char* TestMaterialPath = "TestMaterialPath";
        constexpr const char* TestMaterialAssetId = "TestMaterialAssetId";
        const csp::common::Vector3 TestPosition(1.f, 1.f, 1.f);
        const csp::common::Vector4 TestRotation(1.f, 1.f, 1.f, 1.f);
        const csp::common::Vector3 TestScale(2.f, 2.f, 2.f);
        const SpaceTransform TestTransform(TestPosition, TestRotation, TestScale);
        constexpr const bool TestIsVisible = false;
        constexpr const bool TestIsARVisible = false;
        constexpr const char* TestThirdPartyComponentRef = "TestThirdPartyComponentRef";
        constexpr const bool TestIsShadowCaster = false;

        // Test defaults
        EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetCollectionId(), "");
        EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetId(), "");
        EXPECT_EQ(StaticModelComponent->GetMaterialOverrides().Size(), 0);
        EXPECT_EQ(StaticModelComponent->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(StaticModelComponent->GetRotation(), csp::common::Vector4::Identity());
        EXPECT_EQ(StaticModelComponent->GetScale(), csp::common::Vector3::One());
        EXPECT_EQ(StaticModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());
        EXPECT_EQ(StaticModelComponent->GetIsVisible(), true);
        EXPECT_EQ(StaticModelComponent->GetIsARVisible(), true);
        EXPECT_EQ(StaticModelComponent->GetThirdPartyComponentRef(), "");
        EXPECT_EQ(StaticModelComponent->GetIsShadowCaster(), true);

        StaticModelComponent->SetExternalResourceAssetCollectionId(TestExternalResourceAssetCollectionId);
        StaticModelComponent->SetExternalResourceAssetId(TestExternalResourceAssetId);
        StaticModelComponent->AddMaterialOverride(TestMaterialPath, TestMaterialAssetId);
        StaticModelComponent->SetPosition(TestPosition);
        StaticModelComponent->SetRotation(TestRotation);
        StaticModelComponent->SetScale(TestScale);
        StaticModelComponent->SetIsVisible(TestIsVisible);
        StaticModelComponent->SetIsARVisible(TestIsARVisible);
        StaticModelComponent->SetThirdPartyComponentRef(TestThirdPartyComponentRef);
        StaticModelComponent->SetIsShadowCaster(TestIsShadowCaster);

        // Test new values
        EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetCollectionId(), TestExternalResourceAssetCollectionId);
        EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetId(), TestExternalResourceAssetId);
        EXPECT_EQ(StaticModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(StaticModelComponent->GetMaterialOverrides().HasKey(TestMaterialPath));
        EXPECT_EQ(StaticModelComponent->GetPosition(), TestPosition);
        EXPECT_EQ(StaticModelComponent->GetRotation(), TestRotation);
        EXPECT_EQ(StaticModelComponent->GetScale(), TestScale);
        EXPECT_EQ(StaticModelComponent->GetIsVisible(), TestIsVisible);
        EXPECT_EQ(StaticModelComponent->GetIsARVisible(), TestIsARVisible);
        EXPECT_EQ(StaticModelComponent->GetThirdPartyComponentRef(), TestThirdPartyComponentRef);
        EXPECT_EQ(StaticModelComponent->GetIsShadowCaster(), TestIsShadowCaster);

        // Test transform separately, as this just sets position, rotation, scale
        StaticModelComponent->SetTransform(csp::multiplayer::SpaceTransform());

        EXPECT_EQ(StaticModelComponent->GetTransform(), csp::multiplayer::SpaceTransform());

        StaticModelComponent->SetTransform(TestTransform);

        EXPECT_EQ(StaticModelComponent->GetTransform(), TestTransform);

        // Also test we can remove a material override
        StaticModelComponent->RemoveMaterialOverride(TestMaterialPath);

        EXPECT_EQ(StaticModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_STATIC_MODEL_TESTS || RUN_STATIC_MODEL_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, StaticModelTests, StaticModelScriptInterfaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Create object to represent the fog
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create fog component
    auto* StaticModelComponent = (StaticModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::StaticModel);

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script
    const std::string StaticModelScriptText = R"xx(
		var model = ThisEntity.getStaticModelComponents()[0];
		model.externalResourceAssetCollectionId = "TestExternalResourceAssetCollectionId";
		model.externalResourceAssetId = "TestExternalResourceAssetId";
		model.position = [1, 1, 1];
		model.rotation = [1, 1, 1, 1];
		model.scale = [2, 2, 2];
		model.isVisible = false;
    )xx";

    CreatedObject->GetScript()->SetScriptSource(StaticModelScriptText.c_str());
    CreatedObject->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    // Test new values
    EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetCollectionId(), "TestExternalResourceAssetCollectionId");
    EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetId(), "TestExternalResourceAssetId");
    EXPECT_EQ(StaticModelComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(StaticModelComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(StaticModelComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(StaticModelComponent->GetIsVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_STATIC_MODEL_TESTS || RUN_STATIC_MODEL_ENTER_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, StaticModelTests, StaticModelComponentEnterSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::String ObjectName = "Object 1";

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create static model component
        auto* StaticModelComponent = (StaticModelSpaceComponent*)CreatedObject->AddComponent(ComponentType::StaticModel);
        StaticModelComponent->AddMaterialOverride("TestKey", "TestValue");

        CreatedObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    {
        // Re-enter space
        bool EntitiesCreated = false;

        auto EntitiesReadyCallback = [&EntitiesCreated](bool Success)
        {
            EntitiesCreated = true;
            EXPECT_TRUE(Success);
        };

        EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, EntitySystem);
        EXPECT_TRUE(EntitiesCreated);

        SpaceEntity* FoundEntity = EntitySystem->FindSpaceObject(ObjectName);
        EXPECT_TRUE(FoundEntity != nullptr);

        auto* StaticModelComponent = (StaticModelSpaceComponent*)FoundEntity->GetComponent(0);
        EXPECT_TRUE(StaticModelComponent != nullptr);

        EXPECT_EQ(StaticModelComponent->GetMaterialOverrides().Size(), 1);
        EXPECT_TRUE(StaticModelComponent->GetMaterialOverrides().HasKey("TestKey"));
        EXPECT_EQ(StaticModelComponent->GetMaterialOverrides()["TestKey"], "TestValue");

        // Delete material override
        StaticModelComponent->RemoveMaterialOverride("TestKey");

        FoundEntity->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Ensure component data has been written to database by chs before entering the space again
        // This is due to an enforced 2 second chs database write delay
        std::this_thread::sleep_for(7s);
    }

    {
        // Re-enter space
        bool EntitiesCreated = false;

        auto EntitiesReadyCallback = [&EntitiesCreated](bool Success)
        {
            EntitiesCreated = true;
            EXPECT_TRUE(Success);
        };

        EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, EntitySystem);
        EXPECT_TRUE(EntitiesCreated);

        SpaceEntity* FoundEntity = EntitySystem->FindSpaceObject(ObjectName);
        EXPECT_TRUE(FoundEntity != nullptr);

        auto* StaticModelComponent = (StaticModelSpaceComponent*)FoundEntity->GetComponent(0);
        EXPECT_TRUE(StaticModelComponent != nullptr);

        EXPECT_EQ(StaticModelComponent->GetMaterialOverrides().Size(), 0);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif