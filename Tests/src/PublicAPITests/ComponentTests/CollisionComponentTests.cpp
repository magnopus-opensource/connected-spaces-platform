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
#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
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

CSP_PUBLIC_TEST(CSPEngine, CollisionTests, CollisionComponentTest)
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

    // Create parent entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create collision component
    auto* collisionComponent = static_cast<CollisionSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Collision));

    // Ensure defaults are set
    EXPECT_EQ(collisionComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(collisionComponent->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_EQ(collisionComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(collisionComponent->GetUnscaledBoundingBoxMin(), csp::common::Vector3(-0.5, -0.5, -0.5));
    EXPECT_EQ(collisionComponent->GetUnscaledBoundingBoxMax(), csp::common::Vector3(0.5, 0.5, 0.5));
    EXPECT_EQ(collisionComponent->GetScaledBoundingBoxMin(), csp::common::Vector3(-0.5, -0.5, -0.5));
    EXPECT_EQ(collisionComponent->GetScaledBoundingBoxMax(), csp::common::Vector3(0.5, 0.5, 0.5));
    EXPECT_EQ(collisionComponent->GetCollisionMode(), csp::multiplayer::CollisionMode::Collision);
    EXPECT_EQ(collisionComponent->GetCollisionShape(), csp::multiplayer::CollisionShape::Box);
    EXPECT_EQ(collisionComponent->GetCollisionAssetId(), "");
    EXPECT_EQ(collisionComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(collisionComponent->GetIsEnabled(), true);

    // Set new values
    collisionComponent->SetPosition(csp::common::Vector3::One());
    collisionComponent->SetScale(csp::common::Vector3(2, 2, 2));
    collisionComponent->SetCollisionMode(csp::multiplayer::CollisionMode::Trigger);
    collisionComponent->SetCollisionShape(csp::multiplayer::CollisionShape::Mesh);
    collisionComponent->SetCollisionAssetId("TestAssetID");
    collisionComponent->SetAssetCollectionId("TestAssetCollectionID");
    collisionComponent->SetIsEnabled(false);

    // Ensure values are set correctly
    EXPECT_EQ(collisionComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(collisionComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(collisionComponent->GetUnscaledBoundingBoxMin(), csp::common::Vector3(-0.5, -0.5, -0.5));
    EXPECT_EQ(collisionComponent->GetUnscaledBoundingBoxMax(), csp::common::Vector3(0.5, 0.5, 0.5));
    EXPECT_EQ(collisionComponent->GetScaledBoundingBoxMin(), csp::common::Vector3(-1, -1, -1));
    EXPECT_EQ(collisionComponent->GetScaledBoundingBoxMax(), csp::common::Vector3(1, 1, 1));
    EXPECT_EQ(collisionComponent->GetCollisionMode(), csp::multiplayer::CollisionMode::Trigger);
    EXPECT_EQ(collisionComponent->GetCollisionShape(), csp::multiplayer::CollisionShape::Mesh);
    EXPECT_EQ(collisionComponent->GetCollisionAssetId(), "TestAssetID");
    EXPECT_EQ(collisionComponent->GetAssetCollectionId(), "TestAssetCollectionID");
    EXPECT_EQ(collisionComponent->GetIsEnabled(), false);

    const float defaultSphereRadius = CollisionSpaceComponent::GetDefaultSphereRadius();
    const float defaultCapsuleHalfWidth = CollisionSpaceComponent::GetDefaultCapsuleHalfWidth();
    const float defaultCapsuleHalfHeight = CollisionSpaceComponent::GetDefaultCapsuleHalfHeight();

    EXPECT_EQ(defaultSphereRadius, 0.5f);
    EXPECT_EQ(defaultCapsuleHalfWidth, 0.5f);
    EXPECT_EQ(defaultCapsuleHalfHeight, 1.0f);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}