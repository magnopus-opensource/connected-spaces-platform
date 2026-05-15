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
#include "CSP/Multiplayer/Components/CinematicCameraSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, CinematicCameraTests, CinematicCameraComponentTest)
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

    // Create object to represent the Camera
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create Camera component
    auto* cinematicCamera = static_cast<CinematicCameraSpaceComponent*>(CreatedObject->AddComponent(ComponentType::CinematicCamera));

    // Ensure defaults are set
    EXPECT_FLOAT_EQ(cinematicCamera->GetFocalLength(), 0.035f);
    EXPECT_EQ(cinematicCamera->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(cinematicCamera->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_FLOAT_EQ(cinematicCamera->GetAspectRatio(), 1.778f);
    EXPECT_EQ(cinematicCamera->GetSensorSize(), csp::common::Vector2(0.036f, 0.024f));
    EXPECT_FLOAT_EQ(cinematicCamera->GetNearClip(), 0.1f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetFarClip(), 20000.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetIso(), 400.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetShutterSpeed(), 0.0167f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetAperture(), 4.0f);
    EXPECT_FALSE(cinematicCamera->GetIsViewerCamera());

    // Set the new values
    cinematicCamera->SetFocalLength(2.0f);
    cinematicCamera->SetPosition(csp::common::Vector3(3, 2, 1));
    cinematicCamera->SetRotation(csp::common::Vector4(1, 2, 3, 1));
    cinematicCamera->SetAspectRatio(1.3f);
    cinematicCamera->SetSensorSize(csp::common::Vector2(1, 2));
    cinematicCamera->SetNearClip(1.0f);
    cinematicCamera->SetFarClip(100.0f);
    cinematicCamera->SetIso(1000.0f);
    cinematicCamera->SetShutterSpeed(0.003f);
    cinematicCamera->SetAperture(10.0f);
    cinematicCamera->SetIsViewerCamera(true);

    EXPECT_FLOAT_EQ(cinematicCamera->GetFocalLength(), 2.0f);
    EXPECT_EQ(cinematicCamera->GetPosition(), csp::common::Vector3(3, 2, 1));
    EXPECT_EQ(cinematicCamera->GetRotation(), csp::common::Vector4(1, 2, 3, 1));
    EXPECT_FLOAT_EQ(cinematicCamera->GetAspectRatio(), 1.3f);
    EXPECT_EQ(cinematicCamera->GetSensorSize(), csp::common::Vector2(1, 2));
    EXPECT_FLOAT_EQ(cinematicCamera->GetNearClip(), 1.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetFarClip(), 100.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetIso(), 1000.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetShutterSpeed(), 0.003f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetAperture(), 10.0f);
    EXPECT_TRUE(cinematicCamera->GetIsViewerCamera());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, CinematicCameraTests, CinematicCameraComponentFovTest)
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

    // Create object to represent the Camera
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create Camera component
    auto* cinematicCamera = static_cast<CinematicCameraSpaceComponent*>(CreatedObject->AddComponent(ComponentType::CinematicCamera));

    // Calcuate FOV
    cinematicCamera->SetAspectRatio(16.0f / 9.0f);
    cinematicCamera->SetFocalLength(0.035f);
    cinematicCamera->SetSensorSize(csp::common::Vector2(0.036f, 0.024f));

    EXPECT_FLOAT_EQ(cinematicCamera->GetFov(), 0.95002151f); // 54.432223114614956 degrees

    cinematicCamera->SetAspectRatio(4.0f / 3.0f);
    cinematicCamera->SetFocalLength(0.024f);
    cinematicCamera->SetSensorSize(csp::common::Vector2(0.0223f, 0.0149f));

    EXPECT_FLOAT_EQ(cinematicCamera->GetFov(), 0.78484384f); // ~44 degrees

    cinematicCamera->SetAspectRatio(16.0f / 9.0f);
    cinematicCamera->SetFocalLength(0.150f);
    cinematicCamera->SetSensorSize(csp::common::Vector2(0.02703f, 0.01425f));
    EXPECT_FLOAT_EQ(cinematicCamera->GetFov(), 0.16848914f); // ~9 degrees

    cinematicCamera->SetAspectRatio(21.0f / 9.0f);
    cinematicCamera->SetFocalLength(0.018f);
    cinematicCamera->SetSensorSize(csp::common::Vector2(0.036f, 0.024f));

    EXPECT_FLOAT_EQ(cinematicCamera->GetFov(), 1.57079632f); // 90.0f degrees

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, CinematicCameraTests, CinematicCameraScriptInterfaceTest)
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

    // Create object to represent the CinematicCamera
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create CinematicCamera component
    auto* cinematicCamera = (CinematicCameraSpaceComponent*)CreatedObject->AddComponent(ComponentType::CinematicCamera);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string cinematicCameraScriptText = R"xx(
		const cinematicCamera = ThisEntity.getCinematicCameraComponents()[0];
		cinematicCamera.position = [3, 2, 1];
		cinematicCamera.rotation = [1, 2, 3, 1];
		cinematicCamera.aspectRatio = 1.3;
		cinematicCamera.sensorSize = [1,2];
		cinematicCamera.nearClip = 1;
		cinematicCamera.farClip = 100;
		cinematicCamera.iso = 1000;
		cinematicCamera.shutterSpeed = 0.003;
		cinematicCamera.aperture = 10;
		cinematicCamera.focalLength = 2;
		cinematicCamera.isViewerCamera = true;
		cinematicCamera.isEnabled = false;
	)xx";

    CreatedObject->GetScript().SetScriptSource(cinematicCameraScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(cinematicCamera->GetPosition(), csp::common::Vector3(3, 2, 1));
    EXPECT_EQ(cinematicCamera->GetRotation(), csp::common::Vector4(1, 2, 3, 1));
    EXPECT_FLOAT_EQ(cinematicCamera->GetAspectRatio(), 1.3f);
    EXPECT_EQ(cinematicCamera->GetSensorSize(), csp::common::Vector2(1, 2));
    EXPECT_FLOAT_EQ(cinematicCamera->GetNearClip(), 1.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetFarClip(), 100.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetIso(), 1000.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetShutterSpeed(), 0.003f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetAperture(), 10.0f);
    EXPECT_FLOAT_EQ(cinematicCamera->GetFocalLength(), 2.0f);
    EXPECT_TRUE(cinematicCamera->GetIsViewerCamera());
    EXPECT_FALSE(cinematicCamera->GetIsEnabled());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}