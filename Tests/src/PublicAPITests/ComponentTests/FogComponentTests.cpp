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
#include "CSP/Multiplayer/Components/FogSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, FogTests, FogComponentTest)
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

    // Create object to represent the fog
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create fog component
    auto* fogComponent = static_cast<FogSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Fog));

    // Ensure defaults are set
    EXPECT_EQ(fogComponent->GetFogMode(), FogMode::Exponential);
    EXPECT_EQ(fogComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(fogComponent->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_EQ(fogComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_FLOAT_EQ(fogComponent->GetStartDistance(), 0.f);
    EXPECT_FLOAT_EQ(fogComponent->GetEndDistance(), 0.f);
    EXPECT_EQ(fogComponent->GetColor(), csp::common::Vector3({ 0.8f, 0.9f, 1.0f }));
    EXPECT_FLOAT_EQ(fogComponent->GetDensity(), 0.4f);
    EXPECT_FLOAT_EQ(fogComponent->GetHeightFalloff(), 0.2f);
    EXPECT_FLOAT_EQ(fogComponent->GetMaxOpacity(), 1.f);
    EXPECT_FALSE(fogComponent->GetIsVolumetric());

    // Set new values
    fogComponent->SetFogMode(FogMode::Exponential);
    fogComponent->SetPosition(csp::common::Vector3::One());
    fogComponent->SetRotation(csp::common::Vector4(0, 0, 0, 1));
    fogComponent->SetScale(csp::common::Vector3(2, 2, 2));
    fogComponent->SetStartDistance(1.1f);
    fogComponent->SetEndDistance(2.2f);
    fogComponent->SetColor(csp::common::Vector3::One());
    fogComponent->SetDensity(3.3f);
    fogComponent->SetHeightFalloff(4.4f);
    fogComponent->SetMaxOpacity(5.5f);
    fogComponent->SetIsVolumetric(true);

    // Ensure values are set correctly
    EXPECT_EQ(fogComponent->GetFogMode(), FogMode::Exponential);
    EXPECT_EQ(fogComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(fogComponent->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_EQ(fogComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_FLOAT_EQ(fogComponent->GetStartDistance(), 1.1f);
    EXPECT_FLOAT_EQ(fogComponent->GetEndDistance(), 2.2f);
    EXPECT_EQ(fogComponent->GetColor(), csp::common::Vector3::One());
    EXPECT_FLOAT_EQ(fogComponent->GetDensity(), 3.3f);
    EXPECT_FLOAT_EQ(fogComponent->GetHeightFalloff(), 4.4f);
    EXPECT_FLOAT_EQ(fogComponent->GetMaxOpacity(), 5.5f);
    EXPECT_TRUE(fogComponent->GetIsVolumetric());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, FogTests, FogScriptInterfaceTest)
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

    // Create object to represent the fog
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create fog component
    auto* fogComponent = (FogSpaceComponent*)CreatedObject->AddComponent(ComponentType::Fog);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string fogScriptText = R"xx(
		var fog = ThisEntity.getFogComponents()[0];
		fog.fogMode = 1;
		fog.position = [1, 1, 1];
        fog.scale = [2, 2, 2];
		fog.rotation = [1, 1, 1, 2];
		fog.startDistance = 1.1;
		fog.endDistance = 2.2;
		fog.color = [1, 1, 1];
		fog.density = 3.3;
		fog.heightFalloff = 4.4;
		fog.maxOpacity = 5.5;
		fog.isVolumetric = true;
        fog.isVisible = false;
        fog.isARVisible = false;
        fog.isVirtualVisible = false;
    )xx";

    CreatedObject->GetScript().SetScriptSource(fogScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(fogComponent->GetFogMode(), FogMode::Exponential);
    EXPECT_EQ(fogComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(fogComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(fogComponent->GetRotation(), csp::common::Vector4(1, 1, 1, 2));
    EXPECT_FLOAT_EQ(fogComponent->GetStartDistance(), 1.1f);
    EXPECT_FLOAT_EQ(fogComponent->GetEndDistance(), 2.2f);
    EXPECT_EQ(fogComponent->GetColor(), csp::common::Vector3::One());
    EXPECT_FLOAT_EQ(fogComponent->GetDensity(), 3.3f);
    EXPECT_FLOAT_EQ(fogComponent->GetHeightFalloff(), 4.4f);
    EXPECT_FLOAT_EQ(fogComponent->GetMaxOpacity(), 5.5f);
    EXPECT_TRUE(fogComponent->GetIsVolumetric());
    EXPECT_EQ(fogComponent->GetIsVisible(), false);
    EXPECT_EQ(fogComponent->GetIsARVisible(), false);
    EXPECT_EQ(fogComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}