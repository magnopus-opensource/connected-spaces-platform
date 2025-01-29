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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

#if RUN_ALL_UNIT_TESTS || RUN_CINEMATIC_CAMERA_TESTS || RUN_CINEMATIC_CAMERA_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, CinematicCameraTests, CinematicCameraComponentTest)
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

    // Create object to represent the Camera
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create Camera component
    auto* CinematicCamera = static_cast<CinematicCameraSpaceComponent*>(CreatedObject->AddComponent(ComponentType::CinematicCamera));

    // Ensure defaults are set
    EXPECT_FLOAT_EQ(CinematicCamera->GetFocalLength(), 0.035f);
    EXPECT_EQ(CinematicCamera->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(CinematicCamera->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_FLOAT_EQ(CinematicCamera->GetAspectRatio(), 1.778f);
    EXPECT_EQ(CinematicCamera->GetSensorSize(), csp::common::Vector2(0.036f, 0.024f));
    EXPECT_FLOAT_EQ(CinematicCamera->GetNearClip(), 0.1f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetFarClip(), 20000.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetIso(), 400.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetShutterSpeed(), 0.0167f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetAperture(), 4.0f);
    EXPECT_FALSE(CinematicCamera->GetIsViewerCamera());

    // Set the new values
    CinematicCamera->SetFocalLength(2.0f);
    CinematicCamera->SetPosition(csp::common::Vector3(3, 2, 1));
    CinematicCamera->SetRotation(csp::common::Vector4(1, 2, 3, 1));
    CinematicCamera->SetAspectRatio(1.3f);
    CinematicCamera->SetSensorSize(csp::common::Vector2(1, 2));
    CinematicCamera->SetNearClip(1.0f);
    CinematicCamera->SetFarClip(100.0f);
    CinematicCamera->SetIso(1000.0f);
    CinematicCamera->SetShutterSpeed(0.003f);
    CinematicCamera->SetAperture(10.0f);
    CinematicCamera->SetIsViewerCamera(true);

    EXPECT_FLOAT_EQ(CinematicCamera->GetFocalLength(), 2.0f);
    EXPECT_EQ(CinematicCamera->GetPosition(), csp::common::Vector3(3, 2, 1));
    EXPECT_EQ(CinematicCamera->GetRotation(), csp::common::Vector4(1, 2, 3, 1));
    EXPECT_FLOAT_EQ(CinematicCamera->GetAspectRatio(), 1.3f);
    EXPECT_EQ(CinematicCamera->GetSensorSize(), csp::common::Vector2(1, 2));
    EXPECT_FLOAT_EQ(CinematicCamera->GetNearClip(), 1.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetFarClip(), 100.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetIso(), 1000.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetShutterSpeed(), 0.003f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetAperture(), 10.0f);
    EXPECT_TRUE(CinematicCamera->GetIsViewerCamera());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CINEMATIC_CAMERA_TESTS || RUN_CINEMATIC_CAMERA_COMPONENT_FOV_TEST
CSP_PUBLIC_TEST(CSPEngine, CinematicCameraTests, CinematicCameraComponentFovTest)
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

    // Create object to represent the Camera
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create Camera component
    auto* CinematicCamera = static_cast<CinematicCameraSpaceComponent*>(CreatedObject->AddComponent(ComponentType::CinematicCamera));

    // Calcuate FOV

    CinematicCamera->SetAspectRatio(16.0f / 9.0f);
    CinematicCamera->SetFocalLength(0.035f);
    CinematicCamera->SetSensorSize(csp::common::Vector2(0.036f, 0.024f));

    EXPECT_FLOAT_EQ(CinematicCamera->GetFov(), 0.95002151f); // 54.432223114614956 degrees

    CinematicCamera->SetAspectRatio(4.0f / 3.0f);
    CinematicCamera->SetFocalLength(0.024f);
    CinematicCamera->SetSensorSize(csp::common::Vector2(0.0223f, 0.0149f));

    EXPECT_FLOAT_EQ(CinematicCamera->GetFov(), 0.78484384f); // ~44 degrees

    CinematicCamera->SetAspectRatio(16.0f / 9.0f);
    CinematicCamera->SetFocalLength(0.150f);
    CinematicCamera->SetSensorSize(csp::common::Vector2(0.02703f, 0.01425f));
    EXPECT_FLOAT_EQ(CinematicCamera->GetFov(), 0.16848914f); // ~9 degrees

    CinematicCamera->SetAspectRatio(21.0f / 9.0f);
    CinematicCamera->SetFocalLength(0.018f);
    CinematicCamera->SetSensorSize(csp::common::Vector2(0.036f, 0.024f));

    EXPECT_FLOAT_EQ(CinematicCamera->GetFov(), 1.57079632f); // 90.0f degrees

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CINEMATIC_CAMERA_TESTS || RUN_CINEMATIC_CAMERA_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, CinematicCameraTests, CinematicCameraScriptInterfaceTest)
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

    // Create object to represent the CinematicCamera
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create CinematicCamera component
    auto* CinematicCamera = (CinematicCameraSpaceComponent*)CreatedObject->AddComponent(ComponentType::CinematicCamera);

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script
    const std::string CinematicCameraScriptText = R"xx(
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
	)xx";

    CreatedObject->GetScript()->SetScriptSource(CinematicCameraScriptText.c_str());
    CreatedObject->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(CinematicCamera->GetPosition(), csp::common::Vector3(3, 2, 1));
    EXPECT_EQ(CinematicCamera->GetRotation(), csp::common::Vector4(1, 2, 3, 1));
    EXPECT_FLOAT_EQ(CinematicCamera->GetAspectRatio(), 1.3f);
    EXPECT_EQ(CinematicCamera->GetSensorSize(), csp::common::Vector2(1, 2));
    EXPECT_FLOAT_EQ(CinematicCamera->GetNearClip(), 1.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetFarClip(), 100.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetIso(), 1000.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetShutterSpeed(), 0.003f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetAperture(), 10.0f);
    EXPECT_FLOAT_EQ(CinematicCamera->GetFocalLength(), 2.0f);
    EXPECT_TRUE(CinematicCamera->GetIsViewerCamera());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace
