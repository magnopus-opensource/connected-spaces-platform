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

#if RUN_ALL_UNIT_TESTS || RUN_GAUSSIAN_SPLAT_TESTS || RUN_GAUSSIAN_SPLAT_TEST
CSP_PUBLIC_TEST(CSPEngine, GaussianSplatTests, GaussianSplatTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    bool AssetDetailBlobChangedCallbackCalled = false;
    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    const csp::common::String ModelAssetId = "NotARealId";

    auto* GaussianSplatComponent = (GaussianSplatSpaceComponent*)Object->AddComponent(ComponentType::GaussianSplat);

    // Process component creation
    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Check component was created
    auto& Components = *Object->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    EXPECT_EQ(GaussianSplatComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(GaussianSplatComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(GaussianSplatComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(GaussianSplatComponent->GetIsVisible(), true);
    EXPECT_EQ(GaussianSplatComponent->GetIsARVisible(), true);
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
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GAUSSIAN_SPLAT_TESTS || RUN_GAUSSIAN_SPLAT_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, GaussianSplatTests, GaussianSplatScriptInterfaceTest)
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

    // Create object to represent the image
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    auto* GaussianSplatComponent = (GaussianSplatSpaceComponent*)CreatedObject->AddComponent(ComponentType::GaussianSplat);
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(GaussianSplatComponent->GetIsVisible(), true);

    // Setup script
    const std::string ScriptSource = R"xx(
	
		var splat = ThisEntity.getGaussianSplatComponents()[0];
		splat.tint = [0.0, 0.1, 0.2];
    )xx";

    ScriptComponent->SetScriptSource(ScriptSource.c_str());
    CreatedObject->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    const bool ScriptHasErrors = CreatedObject->GetScript()->HasError();
    EXPECT_FALSE(ScriptHasErrors);

    EXPECT_EQ(GaussianSplatComponent->GetTint(), csp::common::Vector3(0.0f, 0.1f, 0.2f));

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace