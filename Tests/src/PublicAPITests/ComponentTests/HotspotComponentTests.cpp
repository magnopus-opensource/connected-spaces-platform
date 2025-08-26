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
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, HotspotTests, HotspotComponentTest)
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

    // Create object to represent the hotspot
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create hotspot component
    auto* HotspotComponent = static_cast<HotspotSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Hotspot));

    // Ensure defaults are set
    EXPECT_EQ(HotspotComponent->GetPosition().X, 0.0f);
    EXPECT_EQ(HotspotComponent->GetPosition().Y, 0.0f);
    EXPECT_EQ(HotspotComponent->GetPosition().Z, 0.0f);
    EXPECT_EQ(HotspotComponent->GetComponentType(), ComponentType::Hotspot);
    EXPECT_EQ(HotspotComponent->GetIsVRVisible(), true);
    EXPECT_EQ(HotspotComponent->GetIsARVisible(), true);
    EXPECT_EQ(HotspotComponent->GetIsVisible(), true);
    EXPECT_EQ(HotspotComponent->GetRotation().W, 1);
    EXPECT_EQ(HotspotComponent->GetRotation().X, 0);
    EXPECT_EQ(HotspotComponent->GetRotation().Y, 0);
    EXPECT_EQ(HotspotComponent->GetRotation().Z, 0);
    EXPECT_EQ(HotspotComponent->GetIsTeleportPoint(), true);
    EXPECT_EQ(HotspotComponent->GetIsSpawnPoint(), false);

    csp::common::String UniqueComponentId = std::to_string(CreatedObject->GetId()).c_str();
    UniqueComponentId += ":";
    UniqueComponentId += std::to_string(HotspotComponent->GetId()).c_str();

    const csp::common::String HotspotUniqueComponentId = HotspotComponent->GetUniqueComponentId();

    EXPECT_EQ(HotspotUniqueComponentId, UniqueComponentId);

    // Test again to ensure getter works with multiple calls.
    const csp::common::String HotspotUniqueComponentId2 = HotspotComponent->GetUniqueComponentId();

    EXPECT_EQ(HotspotUniqueComponentId2, UniqueComponentId);

    // Set new values
    HotspotComponent->SetPosition(csp::common::Vector3::One());
    HotspotComponent->SetIsVRVisible(false);
    HotspotComponent->SetIsARVisible(false);
    HotspotComponent->SetIsVisible(false);
    HotspotComponent->SetPosition(csp::common::Vector3::One());
    HotspotComponent->SetRotation(csp::common::Vector4 { 1, 1, 1, 1 });
    HotspotComponent->SetIsTeleportPoint(false);
    HotspotComponent->SetIsSpawnPoint(true);

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(HotspotComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetPosition().Z, 1.0f);
    EXPECT_EQ(HotspotComponent->GetIsVRVisible(), false);
    EXPECT_EQ(HotspotComponent->GetIsARVisible(), false);
    EXPECT_EQ(HotspotComponent->GetIsVisible(), false);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(HotspotComponent->GetIsTeleportPoint(), false);
    EXPECT_EQ(HotspotComponent->GetIsSpawnPoint(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotTests, HotspotSpaceComponentScriptInterfaceTest)
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

    // Create object to represent the hotspot
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create hotspot component
    auto* HotspotComponent = (HotspotSpaceComponent*)CreatedObject->AddComponent(ComponentType::Hotspot);
    // Create script component
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);
    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string HotspotScriptText = R"xx(
	
		const assetId			= "TEST_ASSET_ID";
		const assetCollectionId = "TEST_COLLECTION_ID";

		var hotspot = ThisEntity.getHotspotComponents()[0];
		hotspot.position = [1.0,1.0,1.0];
        hotspot.rotation = [1.0, 1.0, 1.0, 1.0];
		hotspot.isVisible = false;
        hotspot.isARVisible = false;
        hotspot.isVRVisible = false;
		hotspot.isSpawnPoint = true;
		hotspot.isTeleportPoint = false;

		var id = hotspot.getUniqueComponentId();
		if (!id)
		{
			throw new Error('no Unique ID generated!');
		}
    )xx";

    ScriptComponent->SetScriptSource(HotspotScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    const bool ScriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(ScriptHasErrors);
    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(HotspotComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetPosition().Z, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(HotspotComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(HotspotComponent->GetIsVisible(), false);
    EXPECT_EQ(HotspotComponent->GetIsARVisible(), false);
    EXPECT_EQ(HotspotComponent->GetIsVRVisible(), false);
    EXPECT_EQ(HotspotComponent->GetIsSpawnPoint(), true);
    EXPECT_EQ(HotspotComponent->GetIsTeleportPoint(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}