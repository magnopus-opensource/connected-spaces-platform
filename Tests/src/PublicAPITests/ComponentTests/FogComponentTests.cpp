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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, FogTests, FogComponentTest)
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

    // Create object to represent the fog
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create fog component
    auto* FogComponent = static_cast<FogSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Fog));

    // Ensure defaults are set
    EXPECT_EQ(FogComponent->GetFogMode(), FogMode::Linear);
    EXPECT_EQ(FogComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(FogComponent->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_EQ(FogComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_FLOAT_EQ(FogComponent->GetStartDistance(), 0.f);
    EXPECT_FLOAT_EQ(FogComponent->GetEndDistance(), 0.f);
    EXPECT_EQ(FogComponent->GetColor(), csp::common::Vector3({ 0.8f, 0.9f, 1.0f }));
    EXPECT_FLOAT_EQ(FogComponent->GetDensity(), 0.2f);
    EXPECT_FLOAT_EQ(FogComponent->GetHeightFalloff(), 0.2f);
    EXPECT_FLOAT_EQ(FogComponent->GetMaxOpacity(), 1.f);
    EXPECT_FALSE(FogComponent->GetIsVolumetric());

    // Set new values
    FogComponent->SetFogMode(FogMode::Exponential);
    FogComponent->SetPosition(csp::common::Vector3::One());
    FogComponent->SetRotation(csp::common::Vector4(0, 0, 0, 1));
    FogComponent->SetScale(csp::common::Vector3(2, 2, 2));
    FogComponent->SetStartDistance(1.1f);
    FogComponent->SetEndDistance(2.2f);
    FogComponent->SetColor(csp::common::Vector3::One());
    FogComponent->SetDensity(3.3f);
    FogComponent->SetHeightFalloff(4.4f);
    FogComponent->SetMaxOpacity(5.5f);
    FogComponent->SetIsVolumetric(true);

    // Ensure values are set correctly
    EXPECT_EQ(FogComponent->GetFogMode(), FogMode::Exponential);
    EXPECT_EQ(FogComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(FogComponent->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_EQ(FogComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_FLOAT_EQ(FogComponent->GetStartDistance(), 1.1f);
    EXPECT_FLOAT_EQ(FogComponent->GetEndDistance(), 2.2f);
    EXPECT_EQ(FogComponent->GetColor(), csp::common::Vector3::One());
    EXPECT_FLOAT_EQ(FogComponent->GetDensity(), 3.3f);
    EXPECT_FLOAT_EQ(FogComponent->GetHeightFalloff(), 4.4f);
    EXPECT_FLOAT_EQ(FogComponent->GetMaxOpacity(), 5.5f);
    EXPECT_TRUE(FogComponent->GetIsVolumetric());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, FogTests, FogScriptInterfaceTest)
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

    // Create object to represent the fog
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create fog component
    auto* FogComponent = (FogSpaceComponent*)CreatedObject->AddComponent(ComponentType::Fog);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string FogScriptText = R"xx(
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

    CreatedObject->GetScript().SetScriptSource(FogScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(FogComponent->GetFogMode(), FogMode::Exponential);
    EXPECT_EQ(FogComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(FogComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(FogComponent->GetRotation(), csp::common::Vector4(1, 1, 1, 2));
    EXPECT_FLOAT_EQ(FogComponent->GetStartDistance(), 1.1f);
    EXPECT_FLOAT_EQ(FogComponent->GetEndDistance(), 2.2f);
    EXPECT_EQ(FogComponent->GetColor(), csp::common::Vector3::One());
    EXPECT_FLOAT_EQ(FogComponent->GetDensity(), 3.3f);
    EXPECT_FLOAT_EQ(FogComponent->GetHeightFalloff(), 4.4f);
    EXPECT_FLOAT_EQ(FogComponent->GetMaxOpacity(), 5.5f);
    EXPECT_TRUE(FogComponent->GetIsVolumetric());
    EXPECT_EQ(FogComponent->GetIsVisible(), false);
    EXPECT_EQ(FogComponent->GetIsARVisible(), false);
    EXPECT_EQ(FogComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}