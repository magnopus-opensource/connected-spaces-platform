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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, HotspotTests, HotspotComponentTest)
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

    // Create object to represent the hotspot
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create hotspot component
    auto* hotspotComponent = static_cast<HotspotSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Hotspot));

    // Ensure defaults are set
    EXPECT_EQ(hotspotComponent->GetPosition().X, 0.0f);
    EXPECT_EQ(hotspotComponent->GetPosition().Y, 0.0f);
    EXPECT_EQ(hotspotComponent->GetPosition().Z, 0.0f);
    EXPECT_EQ(hotspotComponent->GetComponentType(), ComponentType::Hotspot);
    EXPECT_EQ(hotspotComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(hotspotComponent->GetIsARVisible(), true);
    EXPECT_EQ(hotspotComponent->GetIsVisible(), true);
    EXPECT_EQ(hotspotComponent->GetRotation().W, 1);
    EXPECT_EQ(hotspotComponent->GetRotation().X, 0);
    EXPECT_EQ(hotspotComponent->GetRotation().Y, 0);
    EXPECT_EQ(hotspotComponent->GetRotation().Z, 0);
    EXPECT_EQ(hotspotComponent->GetIsTeleportPoint(), true);
    EXPECT_EQ(hotspotComponent->GetIsSpawnPoint(), false);

    csp::common::String uniqueComponentId = std::to_string(CreatedObject->GetId()).c_str();
    uniqueComponentId += ":";
    uniqueComponentId += std::to_string(hotspotComponent->GetId()).c_str();

    const csp::common::String hotspotUniqueComponentId = hotspotComponent->GetUniqueComponentId();

    EXPECT_EQ(hotspotUniqueComponentId, uniqueComponentId);

    // Test again to ensure getter works with multiple calls.
    const csp::common::String hotspotUniqueComponentId2 = hotspotComponent->GetUniqueComponentId();

    EXPECT_EQ(hotspotUniqueComponentId2, uniqueComponentId);

    // Set new values
    hotspotComponent->SetPosition(csp::common::Vector3::One());
    hotspotComponent->SetIsVirtualVisible(false);
    hotspotComponent->SetIsARVisible(false);
    hotspotComponent->SetIsVisible(false);
    hotspotComponent->SetPosition(csp::common::Vector3::One());
    hotspotComponent->SetRotation(csp::common::Vector4 { 1, 1, 1, 1 });
    hotspotComponent->SetIsTeleportPoint(false);
    hotspotComponent->SetIsSpawnPoint(true);

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(hotspotComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetPosition().Z, 1.0f);
    EXPECT_EQ(hotspotComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(hotspotComponent->GetIsARVisible(), false);
    EXPECT_EQ(hotspotComponent->GetIsVisible(), false);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(hotspotComponent->GetIsTeleportPoint(), false);
    EXPECT_EQ(hotspotComponent->GetIsSpawnPoint(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotTests, HotspotSpaceComponentScriptInterfaceTest)
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

    // Create object to represent the hotspot
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create hotspot component
    auto* hotspotComponent = (HotspotSpaceComponent*)CreatedObject->AddComponent(ComponentType::Hotspot);
    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);
    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string hotspotScriptText = R"xx(
	
		const assetId			= "TEST_ASSET_ID";
		const assetCollectionId = "TEST_COLLECTION_ID";

		var hotspot = ThisEntity.getHotspotComponents()[0];
		hotspot.position = [1.0,1.0,1.0];
        hotspot.rotation = [1.0, 1.0, 1.0, 1.0];
		hotspot.isVisible = false;
        hotspot.isARVisible = false;
        hotspot.isVirtualVisible = false;
		hotspot.isSpawnPoint = true;
		hotspot.isTeleportPoint = false;

		var id = hotspot.getUniqueComponentId();
		if (!id)
		{
			throw new Error('no Unique ID generated!');
		}
    )xx";

    scriptComponent->SetScriptSource(hotspotScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);
    realtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(hotspotComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetPosition().Z, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(hotspotComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(hotspotComponent->GetIsVisible(), false);
    EXPECT_EQ(hotspotComponent->GetIsARVisible(), false);
    EXPECT_EQ(hotspotComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(hotspotComponent->GetIsSpawnPoint(), true);
    EXPECT_EQ(hotspotComponent->GetIsTeleportPoint(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}