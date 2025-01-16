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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

#if RUN_ALL_UNIT_TESTS || RUN_COLLISION_TESTS || RUN_MULTIPLAYER_COLLISION_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, CollisionTests, CollisionComponentTest)
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

    // Create object to represent the audio
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create collision component
    auto* CollisionComponent = static_cast<CollisionSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Collision));

    // Ensure defaults are set
    EXPECT_EQ(CollisionComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(CollisionComponent->GetRotation(), csp::common::Vector4(0, 0, 0, 1));
    EXPECT_EQ(CollisionComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(CollisionComponent->GetUnscaledBoundingBoxMin(), csp::common::Vector3(-0.5, -0.5, -0.5));
    EXPECT_EQ(CollisionComponent->GetUnscaledBoundingBoxMax(), csp::common::Vector3(0.5, 0.5, 0.5));
    EXPECT_EQ(CollisionComponent->GetScaledBoundingBoxMin(), csp::common::Vector3(-0.5, -0.5, -0.5));
    EXPECT_EQ(CollisionComponent->GetScaledBoundingBoxMax(), csp::common::Vector3(0.5, 0.5, 0.5));
    EXPECT_EQ(CollisionComponent->GetCollisionMode(), csp::multiplayer::CollisionMode::Collision);
    EXPECT_EQ(CollisionComponent->GetCollisionShape(), csp::multiplayer::CollisionShape::Box);
    EXPECT_EQ(CollisionComponent->GetCollisionAssetId(), "");
    EXPECT_EQ(CollisionComponent->GetAssetCollectionId(), "");

    // Set new values
    CollisionComponent->SetPosition(csp::common::Vector3::One());
    CollisionComponent->SetScale(csp::common::Vector3(2, 2, 2));
    CollisionComponent->SetCollisionMode(csp::multiplayer::CollisionMode::Trigger);
    CollisionComponent->SetCollisionShape(csp::multiplayer::CollisionShape::Mesh);
    CollisionComponent->SetCollisionAssetId("TestAssetID");
    CollisionComponent->SetAssetCollectionId("TestAssetCollectionID");

    // Ensure values are set correctly
    EXPECT_EQ(CollisionComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(CollisionComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(CollisionComponent->GetUnscaledBoundingBoxMin(), csp::common::Vector3(-0.5, -0.5, -0.5));
    EXPECT_EQ(CollisionComponent->GetUnscaledBoundingBoxMax(), csp::common::Vector3(0.5, 0.5, 0.5));
    EXPECT_EQ(CollisionComponent->GetScaledBoundingBoxMin(), csp::common::Vector3(-1, -1, -1));
    EXPECT_EQ(CollisionComponent->GetScaledBoundingBoxMax(), csp::common::Vector3(1, 1, 1));
    EXPECT_EQ(CollisionComponent->GetCollisionMode(), csp::multiplayer::CollisionMode::Trigger);
    EXPECT_EQ(CollisionComponent->GetCollisionShape(), csp::multiplayer::CollisionShape::Mesh);
    EXPECT_EQ(CollisionComponent->GetCollisionAssetId(), "TestAssetID");
    EXPECT_EQ(CollisionComponent->GetAssetCollectionId(), "TestAssetCollectionID");

    const float DefaultSphereRadius = CollisionSpaceComponent::GetDefaultSphereRadius();
    const float DefaultCapsuleHalfWidth = CollisionSpaceComponent::GetDefaultCapsuleHalfWidth();
    const float DefaultCapsuleHalfHeight = CollisionSpaceComponent::GetDefaultCapsuleHalfHeight();

    EXPECT_EQ(DefaultSphereRadius, 0.5f);
    EXPECT_EQ(DefaultCapsuleHalfWidth, 0.5f);
    EXPECT_EQ(DefaultCapsuleHalfHeight, 1.0f);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace