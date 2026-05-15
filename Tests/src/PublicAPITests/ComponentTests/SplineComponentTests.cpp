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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, SplineTests, UseSplineTest)
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

    const csp::common::String userName = "Player 1";
    const SpaceTransform userTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    const csp::common::String userAvatarId = "MyCoolAvatar";

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        // Ensure we're in the first space
        EXPECT_EQ(spaceSystem->GetCurrentSpace().Id, space.Id);

        // Create object to represent the spline
        csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create spline component
        auto* splineComponent = (SplineSpaceComponent*)CreatedObject->AddComponent(ComponentType::Spline);
        csp::common::List<csp::common::Vector3> wayPoints
            = { { 0, 0, 0 }, { 0, 1000, 0 }, { 0, 2000, 0 }, { 0, 3000, 0 }, { 0, 4000, 0 }, { 0, 5000, 0 } };

        {
            auto result = splineComponent->GetWaypoints();

            EXPECT_EQ(result.Size(), 0);
        }

        {
            auto result = splineComponent->GetLocationAlongSpline(1);

            EXPECT_EQ(result.X, 0);
            EXPECT_EQ(result.Y, 0);
            EXPECT_EQ(result.Z, 0);
        }

        {
            splineComponent->SetWaypoints(wayPoints);

            auto result = splineComponent->GetWaypoints();

            EXPECT_EQ(result.Size(), wayPoints.Size());

            // Expect final waypoint to be the same
            EXPECT_EQ(result[0], wayPoints[0]);
        }

        {
            // Calculated cubic interpolate spline
            auto result = splineComponent->GetLocationAlongSpline(1);

            // Expect final waypoint to be the same
            EXPECT_EQ(result, wayPoints[wayPoints.Size() - 1]);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SplineTests, SplineScriptInterfaceTest)
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

    // Create object to represent the spline
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create spline component
    auto* splineComponent = (SplineSpaceComponent*)CreatedObject->AddComponent(ComponentType::Spline);
    csp::common::List<csp::common::Vector3> wayPoints
        = { { 0, 0, 0 }, { 0, 1000, 0 }, { 0, 2000, 0 }, { 0, 3000, 0 }, { 0, 4000, 0 }, { 0, 5000, 0 } };

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string splineScriptText = R"xx(
	
		var spline = ThisEntity.getSplineComponents()[0];
		
		var waypoints = [[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
		spline.setWaypoints(waypoints);
		var positionResult = spline.getLocationAlongSpline(1);
		
    )xx";

    CreatedObject->GetScript().SetScriptSource(splineScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(splineComponent->GetWaypoints().Size(), wayPoints.Size());

    // expect final waypoint to be the same
    EXPECT_EQ(splineComponent->GetWaypoints()[0], wayPoints[0]);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}