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
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
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

/*
    Tests that AvatarSpaceComponent default properties are correctly set on construction.
    Tests properties are correctly updated via setters.
*/
CSP_PUBLIC_TEST(CSPEngine, AvatarTests, AvatarComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Define Avatar properties
    const csp::common::String& userName = "Creator 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.11f, 2.22f, 3.33f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    const bool isVisible = false;
    const AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "Creator1Avatar";
    const AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Creator;
    const LocomotionModel userAvatarLocomotionModel = LocomotionModel::FreeCamera;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, userAvatarLocomotionModel);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), userName);
    EXPECT_EQ(Avatar->GetPosition(), userTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), userTransform.Rotation);

    auto& components = *Avatar->GetComponents();
    EXPECT_EQ(components.Size(), 1);

    auto* component = components[0];
    EXPECT_EQ(component->GetComponentType(), ComponentType::AvatarData);

    // Verify that the default AvatarSpaceComponent property values are correct
    AvatarSpaceComponent* avatarComponent = dynamic_cast<AvatarSpaceComponent*>(component);
    EXPECT_EQ(avatarComponent->GetAvatarId(), userAvatarId);
    EXPECT_EQ(avatarComponent->GetState(), userAvatarState);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), userAvatarPlayMode);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), AvatarPlayMode::Creator);
    EXPECT_EQ(avatarComponent->GetLocomotionModel(), userAvatarLocomotionModel);
    EXPECT_EQ(avatarComponent->GetIsVisible(), isVisible);
    EXPECT_EQ(avatarComponent->GetIsARVisible(), true);
    EXPECT_EQ(avatarComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(avatarComponent->GetAgoraUserId(), "");
    EXPECT_EQ(avatarComponent->GetAvatarUrl(), "");
    EXPECT_EQ(avatarComponent->GetIsHandIKEnabled(), false);
    EXPECT_EQ(avatarComponent->GetTargetHandIKTargetLocation(), csp::common::Vector3::Zero());
    EXPECT_EQ(avatarComponent->GetHandRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(avatarComponent->GetHeadRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(avatarComponent->GetWalkRunBlendPercentage(), 0.0f);
    EXPECT_EQ(avatarComponent->GetTorsoTwistAlpha(), 0.0f);
    EXPECT_EQ(avatarComponent->GetMovementDirection(), csp::common::Vector3::Zero());

    // Set new AvatarSpaceComponent property values
    constexpr const char* newAvatarId = "TestAvatarId";
    const AvatarState newAvatarState = AvatarState::Flying;
    const AvatarPlayMode newAvatarPlayMode = AvatarPlayMode::VR;
    const LocomotionModel newAvatarLocomotionModel = LocomotionModel::Grounded;
    const bool newIsVisible = true;
    const bool newIsArVisible = false;
    const bool newIsVirtualVisible = false;
    const csp::common::String newAgoraUserId = "AgoraUser123";
    const csp::common::String newAvatarUrl = "https://models.readyplayer.me/64ff48b0b9f61ba631e47537.glb";
    const bool newIsHandIkEnabled = true;
    const csp::common::Vector3 newTargetHandIkTargetLocation = { 0.1f, 0.2f, 0.3f };
    const csp::common::Vector4 newHandRotation = { 0.1f, 0.2f, 0.3f, 1.0f };
    const csp::common::Vector4 newHeadRotation = { 0.4f, 0.5f, 0.6f, 1.0f };
    const float newWalkRunBlendPercentage = 0.75f;
    const float newTorsoTwistAlpha = 0.5f;
    const csp::common::Vector3 newMovementDirection = { 0.0f, 1.0f, 0.0f };

    avatarComponent->SetAvatarId(newAvatarId);
    avatarComponent->SetState(newAvatarState);
    avatarComponent->SetAvatarPlayMode(newAvatarPlayMode);
    avatarComponent->SetLocomotionModel(newAvatarLocomotionModel);
    avatarComponent->SetIsVisible(newIsVisible);
    avatarComponent->SetIsARVisible(newIsArVisible);
    avatarComponent->SetIsVirtualVisible(newIsVirtualVisible);
    avatarComponent->SetAgoraUserId(newAgoraUserId);
    avatarComponent->SetAvatarUrl(newAvatarUrl);
    avatarComponent->SetIsHandIKEnabled(newIsHandIkEnabled);
    avatarComponent->SetTargetHandIKTargetLocation(newTargetHandIkTargetLocation);
    avatarComponent->SetHandRotation(newHandRotation);
    avatarComponent->SetHeadRotation(newHeadRotation);
    avatarComponent->SetWalkRunBlendPercentage(newWalkRunBlendPercentage);
    avatarComponent->SetTorsoTwistAlpha(newTorsoTwistAlpha);
    avatarComponent->SetMovementDirection(newMovementDirection);

    // Verify that the AvatarSpaceComponent property values are updated correctly
    EXPECT_EQ(avatarComponent->GetAvatarId(), newAvatarId);
    EXPECT_EQ(avatarComponent->GetState(), newAvatarState);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), newAvatarPlayMode);
    EXPECT_EQ(avatarComponent->GetLocomotionModel(), newAvatarLocomotionModel);
    EXPECT_EQ(avatarComponent->GetIsVisible(), newIsVisible);
    EXPECT_EQ(avatarComponent->GetIsARVisible(), newIsArVisible);
    EXPECT_EQ(avatarComponent->GetIsVirtualVisible(), newIsVirtualVisible);
    EXPECT_EQ(avatarComponent->GetAgoraUserId(), newAgoraUserId);
    EXPECT_EQ(avatarComponent->GetAvatarUrl(), newAvatarUrl);
    EXPECT_EQ(avatarComponent->GetIsHandIKEnabled(), newIsHandIkEnabled);
    EXPECT_EQ(avatarComponent->GetTargetHandIKTargetLocation(), newTargetHandIkTargetLocation);
    EXPECT_EQ(avatarComponent->GetHandRotation(), newHandRotation);
    EXPECT_EQ(avatarComponent->GetHeadRotation(), newHeadRotation);
    EXPECT_EQ(avatarComponent->GetWalkRunBlendPercentage(), newWalkRunBlendPercentage);
    EXPECT_EQ(avatarComponent->GetTorsoTwistAlpha(), newTorsoTwistAlpha);
    EXPECT_EQ(avatarComponent->GetMovementDirection(), newMovementDirection);

    // Exit Space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
    Tests that AvatarSpaceComponents can be sucessfully modified by scripts
*/
CSP_PUBLIC_TEST(CSPEngine, AvatarTests, AvatarScriptInterfaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Define Avatar properties
    const csp::common::String& userName = "Creator 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.11f, 2.22f, 3.33f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    const bool isVisible = false;
    const AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "Creator1Avatar";
    const AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Creator;
    const LocomotionModel userAvatarLocomotionModel = LocomotionModel::Grounded;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, userAvatarLocomotionModel);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), userName);
    EXPECT_EQ(Avatar->GetPosition(), userTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), userTransform.Rotation);

    auto& components = *Avatar->GetComponents();
    EXPECT_EQ(components.Size(), 1);

    auto* component = components[0];
    EXPECT_EQ(component->GetComponentType(), ComponentType::AvatarData);

    // Verify that the default AvatarSpaceComponent property values are correct
    AvatarSpaceComponent* avatarComponent = dynamic_cast<AvatarSpaceComponent*>(component);
    EXPECT_EQ(avatarComponent->GetAvatarId(), userAvatarId);
    EXPECT_EQ(avatarComponent->GetState(), userAvatarState);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), userAvatarPlayMode);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), AvatarPlayMode::Creator);
    EXPECT_EQ(avatarComponent->GetLocomotionModel(), userAvatarLocomotionModel);
    EXPECT_EQ(avatarComponent->GetIsVisible(), isVisible);
    EXPECT_EQ(avatarComponent->GetIsARVisible(), true);
    EXPECT_EQ(avatarComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(avatarComponent->GetAgoraUserId(), "");
    EXPECT_EQ(avatarComponent->GetAvatarUrl(), "");
    EXPECT_EQ(avatarComponent->GetIsHandIKEnabled(), false);
    EXPECT_EQ(avatarComponent->GetTargetHandIKTargetLocation(), csp::common::Vector3::Zero());
    EXPECT_EQ(avatarComponent->GetHandRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(avatarComponent->GetHeadRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(avatarComponent->GetWalkRunBlendPercentage(), 0.0f);
    EXPECT_EQ(avatarComponent->GetTorsoTwistAlpha(), 0.0f);
    EXPECT_EQ(avatarComponent->GetMovementDirection(), csp::common::Vector3::Zero());

    Avatar->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script to set new properties
    std::string avatarComponentScriptText = R"xx(
			var avatar = ThisEntity.getAvatarComponents()[0];
			avatar.avatarId = "TestAvatarId";
            avatar.state = 3; // Flying
            avatar.avatarPlayMode = 2; // VR
            avatar.locomotionModel = 1; // FreeCamera
            avatar.avatarMeshIndex = 42;
            avatar.agoraUserId = "AgoraUser123";
            avatar.avatarUrl = "https://models.readyplayer.me/64ff48b0b9f61ba631e47537.glb";
            avatar.isHandIKEnabled = true;
            avatar.targetHandIKTargetLocation = [0.1, 0.2, 0.3];
            avatar.handRotation = [0.1, 0.2, 0.3, 1.0];
            avatar.headRotation = [0.4, 0.5, 0.6, 1.0];
            avatar.walkRunBlendPercentage = 0.75;
            avatar.torsoTwistAlpha = 0.5;
            avatar.isVisible = true;
            avatar.isARVisible = false;
            avatar.isVirtualVisible = false;
            avatar.movementDirection = [1.0, 0.0, 0.0];
		)xx";

    Avatar->GetScript().SetScriptSource(avatarComponentScriptText.c_str());
    Avatar->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Test scripts sets new properties
    EXPECT_EQ(avatarComponent->GetAvatarId(), "TestAvatarId");
    EXPECT_EQ(avatarComponent->GetState(), AvatarState::Flying);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), AvatarPlayMode::VR);
    EXPECT_EQ(avatarComponent->GetLocomotionModel(), LocomotionModel::FreeCamera);
    EXPECT_EQ(avatarComponent->GetAgoraUserId(), "AgoraUser123");
    EXPECT_EQ(avatarComponent->GetAvatarUrl(), "https://models.readyplayer.me/64ff48b0b9f61ba631e47537.glb");
    EXPECT_EQ(avatarComponent->GetIsHandIKEnabled(), true);
    EXPECT_EQ(avatarComponent->GetTargetHandIKTargetLocation(), csp::common::Vector3(0.1f, 0.2f, 0.3f));
    EXPECT_EQ(avatarComponent->GetHandRotation(), csp::common::Vector4(0.1f, 0.2f, 0.3f, 1.0f));
    EXPECT_EQ(avatarComponent->GetHeadRotation(), csp::common::Vector4(0.4f, 0.5f, 0.6f, 1.0f));
    EXPECT_EQ(avatarComponent->GetWalkRunBlendPercentage(), 0.75f);
    EXPECT_EQ(avatarComponent->GetTorsoTwistAlpha(), 0.5f);
    EXPECT_EQ(avatarComponent->GetIsVisible(), true);
    EXPECT_EQ(avatarComponent->GetIsARVisible(), false);
    EXPECT_EQ(avatarComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(avatarComponent->GetMovementDirection(), csp::common::Vector3(1.0f, 0.0f, 0.0f));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
