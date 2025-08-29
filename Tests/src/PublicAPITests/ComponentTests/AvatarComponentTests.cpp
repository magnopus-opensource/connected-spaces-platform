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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

/*
    Tests that AvatarSpaceComponent default properties are correctly set on construction.
    Tests properties are correctly updated via setters.
*/
CSP_PUBLIC_TEST(CSPEngine, AvatarTests, AvatarComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Define Avatar properties
    const csp::common::String& UserName = "Creator 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.11f, 2.22f, 3.33f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    const bool IsVisible = false;
    const AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "Creator1Avatar";
    const AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Creator;
    const LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);
    EXPECT_EQ(Avatar->GetPosition(), UserTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), UserTransform.Rotation);

    auto& Components = *Avatar->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    auto* Component = Components[0];
    EXPECT_EQ(Component->GetComponentType(), ComponentType::AvatarData);

    // Verify that the default AvatarSpaceComponent property values are correct
    AvatarSpaceComponent* AvatarComponent = dynamic_cast<AvatarSpaceComponent*>(Component);
    EXPECT_EQ(AvatarComponent->GetAvatarId(), UserAvatarId);
    EXPECT_EQ(AvatarComponent->GetState(), UserAvatarState);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), UserAvatarPlayMode);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), AvatarPlayMode::Creator);
    EXPECT_EQ(AvatarComponent->GetLocomotionModel(), UserAvatarLocomotionModel);
    EXPECT_EQ(AvatarComponent->GetIsVisible(), IsVisible);
    EXPECT_EQ(AvatarComponent->GetIsARVisible(), true);
    EXPECT_EQ(AvatarComponent->GetIsVRVisible(), true);
    EXPECT_EQ(AvatarComponent->GetAvatarMeshIndex(), -1);
    EXPECT_EQ(AvatarComponent->GetAgoraUserId(), "");
    EXPECT_EQ(AvatarComponent->GetCustomAvatarUrl(), "");
    EXPECT_EQ(AvatarComponent->GetIsHandIKEnabled(), false);
    EXPECT_EQ(AvatarComponent->GetTargetHandIKTargetLocation(), csp::common::Vector3::Zero());
    EXPECT_EQ(AvatarComponent->GetHandRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(AvatarComponent->GetHeadRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(AvatarComponent->GetWalkRunBlendPercentage(), 0.0f);
    EXPECT_EQ(AvatarComponent->GetTorsoTwistAlpha(), 0.0f);
    EXPECT_EQ(AvatarComponent->GetMovementDirection(), csp::common::Vector3::Zero());

    // Set new AvatarSpaceComponent property values
    constexpr const char* NewAvatarId = "TestAvatarId";
    const AvatarState NewAvatarState = AvatarState::Flying;
    const AvatarPlayMode NewAvatarPlayMode = AvatarPlayMode::VR;
    const LocomotionModel NewAvatarLocomotionModel = LocomotionModel::FreeCamera;
    const bool NewIsVisible = true;
    const bool NewIsARVisible = false;
    const bool NewIsVRVisible = false;
    const int64_t NewAvatarMeshIndex = 42;
    const csp::common::String NewAgoraUserId = "AgoraUser123";
    const csp::common::String NewCustomAvatarUrl = "https://example.com/avatar.png";
    const bool NewIsHandIKEnabled = true;
    const csp::common::Vector3 NewTargetHandIKTargetLocation = { 0.1f, 0.2f, 0.3f };
    const csp::common::Vector4 NewHandRotation = { 0.1f, 0.2f, 0.3f, 1.0f };
    const csp::common::Vector4 NewHeadRotation = { 0.4f, 0.5f, 0.6f, 1.0f };
    const float NewWalkRunBlendPercentage = 0.75f;
    const float NewTorsoTwistAlpha = 0.5f;
    const csp::common::Vector3 NewMovementDirection = { 0.0f, 1.0f, 0.0f };

    AvatarComponent->SetAvatarId(NewAvatarId);
    AvatarComponent->SetState(NewAvatarState);
    AvatarComponent->SetAvatarPlayMode(NewAvatarPlayMode);
    AvatarComponent->SetLocomotionModel(NewAvatarLocomotionModel);
    AvatarComponent->SetIsVisible(NewIsVisible);
    AvatarComponent->SetIsARVisible(NewIsARVisible);
    AvatarComponent->SetIsVRVisible(NewIsVRVisible);
    AvatarComponent->SetAvatarMeshIndex(NewAvatarMeshIndex);
    AvatarComponent->SetAgoraUserId(NewAgoraUserId);
    AvatarComponent->SetCustomAvatarUrl(NewCustomAvatarUrl);
    AvatarComponent->SetIsHandIKEnabled(NewIsHandIKEnabled);
    AvatarComponent->SetTargetHandIKTargetLocation(NewTargetHandIKTargetLocation);
    AvatarComponent->SetHandRotation(NewHandRotation);
    AvatarComponent->SetHeadRotation(NewHeadRotation);
    AvatarComponent->SetWalkRunBlendPercentage(NewWalkRunBlendPercentage);
    AvatarComponent->SetTorsoTwistAlpha(NewTorsoTwistAlpha);
    AvatarComponent->SetMovementDirection(NewMovementDirection);

    // Verify that the AvatarSpaceComponent property values are updated correctly
    EXPECT_EQ(AvatarComponent->GetAvatarId(), NewAvatarId);
    EXPECT_EQ(AvatarComponent->GetState(), NewAvatarState);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), NewAvatarPlayMode);
    EXPECT_EQ(AvatarComponent->GetLocomotionModel(), NewAvatarLocomotionModel);
    EXPECT_EQ(AvatarComponent->GetIsVisible(), NewIsVisible);
    EXPECT_EQ(AvatarComponent->GetIsARVisible(), NewIsARVisible);
    EXPECT_EQ(AvatarComponent->GetIsVRVisible(), NewIsVRVisible);
    EXPECT_EQ(AvatarComponent->GetAvatarMeshIndex(), NewAvatarMeshIndex);
    EXPECT_EQ(AvatarComponent->GetAgoraUserId(), NewAgoraUserId);
    EXPECT_EQ(AvatarComponent->GetCustomAvatarUrl(), NewCustomAvatarUrl);
    EXPECT_EQ(AvatarComponent->GetIsHandIKEnabled(), NewIsHandIKEnabled);
    EXPECT_EQ(AvatarComponent->GetTargetHandIKTargetLocation(), NewTargetHandIKTargetLocation);
    EXPECT_EQ(AvatarComponent->GetHandRotation(), NewHandRotation);
    EXPECT_EQ(AvatarComponent->GetHeadRotation(), NewHeadRotation);
    EXPECT_EQ(AvatarComponent->GetWalkRunBlendPercentage(), NewWalkRunBlendPercentage);
    EXPECT_EQ(AvatarComponent->GetTorsoTwistAlpha(), NewTorsoTwistAlpha);
    EXPECT_EQ(AvatarComponent->GetMovementDirection(), NewMovementDirection);

    // Exit Space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
    Tests that AvatarSpaceComponents can be sucessfully modified by scripts
*/
CSP_PUBLIC_TEST(CSPEngine, AvatarTests, AvatarScriptInterfaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Define Avatar properties
    const csp::common::String& UserName = "Creator 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.11f, 2.22f, 3.33f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    const bool IsVisible = false;
    const AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "Creator1Avatar";
    const AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Creator;
    const LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);
    EXPECT_EQ(Avatar->GetPosition(), UserTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), UserTransform.Rotation);

    auto& Components = *Avatar->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    auto* Component = Components[0];
    EXPECT_EQ(Component->GetComponentType(), ComponentType::AvatarData);

    // Verify that the default AvatarSpaceComponent property values are correct
    AvatarSpaceComponent* AvatarComponent = dynamic_cast<AvatarSpaceComponent*>(Component);
    EXPECT_EQ(AvatarComponent->GetAvatarId(), UserAvatarId);
    EXPECT_EQ(AvatarComponent->GetState(), UserAvatarState);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), UserAvatarPlayMode);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), AvatarPlayMode::Creator);
    EXPECT_EQ(AvatarComponent->GetLocomotionModel(), UserAvatarLocomotionModel);
    EXPECT_EQ(AvatarComponent->GetIsVisible(), IsVisible);
    EXPECT_EQ(AvatarComponent->GetIsARVisible(), true);
    EXPECT_EQ(AvatarComponent->GetIsVRVisible(), true);
    EXPECT_EQ(AvatarComponent->GetAvatarMeshIndex(), -1);
    EXPECT_EQ(AvatarComponent->GetAgoraUserId(), "");
    EXPECT_EQ(AvatarComponent->GetCustomAvatarUrl(), "");
    EXPECT_EQ(AvatarComponent->GetIsHandIKEnabled(), false);
    EXPECT_EQ(AvatarComponent->GetTargetHandIKTargetLocation(), csp::common::Vector3::Zero());
    EXPECT_EQ(AvatarComponent->GetHandRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(AvatarComponent->GetHeadRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(AvatarComponent->GetWalkRunBlendPercentage(), 0.0f);
    EXPECT_EQ(AvatarComponent->GetTorsoTwistAlpha(), 0.0f);
    EXPECT_EQ(AvatarComponent->GetMovementDirection(), csp::common::Vector3::Zero());

    Avatar->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script to set new properties
    std::string AvatarComponentScriptText = R"xx(
			var avatar = ThisEntity.getAvatarComponents()[0];
			avatar.avatarId = "TestAvatarId";
            avatar.state = 3; // Flying
            avatar.avatarPlayMode = 2; // VR
            avatar.locomotionModel = 1; // FreeCamera
            avatar.avatarMeshIndex = 42;
            avatar.agoraUserId = "AgoraUser123";
            avatar.customAvatarUrl = "https://example.com/avatar.png";
            avatar.isHandIKEnabled = true;
            avatar.targetHandIKTargetLocation = [0.1, 0.2, 0.3];
            avatar.handRotation = [0.1, 0.2, 0.3, 1.0];
            avatar.headRotation = [0.4, 0.5, 0.6, 1.0];
            avatar.walkRunBlendPercentage = 0.75;
            avatar.torsoTwistAlpha = 0.5;
            avatar.isVisible = true;
            avatar.isARVisible = false;
            avatar.isVRVisible = false;
		)xx";

    Avatar->GetScript().SetScriptSource(AvatarComponentScriptText.c_str());
    Avatar->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    // Test scripts sets new properties
    EXPECT_EQ(AvatarComponent->GetAvatarId(), "TestAvatarId");
    EXPECT_EQ(AvatarComponent->GetState(), AvatarState::Flying);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), AvatarPlayMode::VR);
    EXPECT_EQ(AvatarComponent->GetLocomotionModel(), LocomotionModel::FreeCamera);
    EXPECT_EQ(AvatarComponent->GetAvatarMeshIndex(), 42);
    EXPECT_EQ(AvatarComponent->GetAgoraUserId(), "AgoraUser123");
    EXPECT_EQ(AvatarComponent->GetCustomAvatarUrl(), "https://example.com/avatar.png");
    EXPECT_EQ(AvatarComponent->GetIsHandIKEnabled(), true);
    EXPECT_EQ(AvatarComponent->GetTargetHandIKTargetLocation(), csp::common::Vector3(0.1f, 0.2f, 0.3f));
    EXPECT_EQ(AvatarComponent->GetHandRotation(), csp::common::Vector4(0.1f, 0.2f, 0.3f, 1.0f));
    EXPECT_EQ(AvatarComponent->GetHeadRotation(), csp::common::Vector4(0.4f, 0.5f, 0.6f, 1.0f));
    EXPECT_EQ(AvatarComponent->GetWalkRunBlendPercentage(), 0.75f);
    EXPECT_EQ(AvatarComponent->GetTorsoTwistAlpha(), 0.5f);
    EXPECT_EQ(AvatarComponent->GetIsVisible(), true);
    EXPECT_EQ(AvatarComponent->GetIsARVisible(), false);
    EXPECT_EQ(AvatarComponent->GetIsVRVisible(), false);

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
