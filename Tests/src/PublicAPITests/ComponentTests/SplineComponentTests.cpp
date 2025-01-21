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
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
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

#if RUN_ALL_UNIT_TESTS || RUN_SPLINE_TESTS || RUN_USE_SPLINE_TEST
CSP_PUBLIC_TEST(CSPEngine, SplineTests, UseSplineTest)
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

    const csp::common::String UserName = "Player 1";
    const SpaceTransform UserTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    const AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String UserAvatarId = "MyCoolAvatar";
    const AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        // Ensure we're in the first space
        EXPECT_EQ(SpaceSystem->GetCurrentSpace().Id, Space.Id);

        // Create object to represent the spline
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create spline component
        auto* SplineComponent = (SplineSpaceComponent*)CreatedObject->AddComponent(ComponentType::Spline);
        csp::common::List<csp::common::Vector3> WayPoints
            = { { 0, 0, 0 }, { 0, 1000, 0 }, { 0, 2000, 0 }, { 0, 3000, 0 }, { 0, 4000, 0 }, { 0, 5000, 0 } };

        {
            auto Result = SplineComponent->GetWaypoints();

            EXPECT_EQ(Result.Size(), 0);
        }

        {
            auto Result = SplineComponent->GetLocationAlongSpline(1);

            EXPECT_EQ(Result.X, 0);
            EXPECT_EQ(Result.Y, 0);
            EXPECT_EQ(Result.Z, 0);
        }

        {
            SplineComponent->SetWaypoints(WayPoints);

            auto Result = SplineComponent->GetWaypoints();

            EXPECT_EQ(Result.Size(), WayPoints.Size());

            // Expect final waypoint to be the same
            EXPECT_EQ(Result[0], WayPoints[0]);
        }

        {
            // Calculated cubic interpolate spline
            auto Result = SplineComponent->GetLocationAlongSpline(1);

            // Expect final waypoint to be the same
            EXPECT_EQ(Result, WayPoints[WayPoints.Size() - 1]);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPLINE_TESTS || RUN_SPLINE_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SplineTests, SplineScriptInterfaceTest)
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

    // Create object to represent the spline
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create spline component
    auto* SplineComponent = (SplineSpaceComponent*)CreatedObject->AddComponent(ComponentType::Spline);
    csp::common::List<csp::common::Vector3> WayPoints
        = { { 0, 0, 0 }, { 0, 1000, 0 }, { 0, 2000, 0 }, { 0, 3000, 0 }, { 0, 4000, 0 }, { 0, 5000, 0 } };

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script
    const std::string SplineScriptText = R"xx(
	
		var spline = ThisEntity.getSplineComponents()[0];
		
		var waypoints = [[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
		spline.setWaypoints(waypoints);
		var positionResult = spline.getLocationAlongSpline(1);
		
    )xx";

    CreatedObject->GetScript()->SetScriptSource(SplineScriptText.c_str());
    CreatedObject->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(SplineComponent->GetWaypoints().Size(), WayPoints.Size());

    // expect final waypoint to be the same
    EXPECT_EQ(SplineComponent->GetWaypoints()[0], WayPoints[0]);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace