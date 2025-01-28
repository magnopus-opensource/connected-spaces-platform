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

/// @file AvatarSpaceComponent.h
/// @brief Definitions and support for avatar components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"

namespace csp::multiplayer
{

/// @brief Enumerates the supported states for an avatar.
///        These are used to establish the related animation that the avatar will use on its state machine.
enum class AvatarState
{
    Idle = 0,
    Walking,
    Running,
    Flying,
    Jumping,
    Falling,
    Num
};

/// @brief Enumerates the supported play mode for the avatar.
enum class AvatarPlayMode
{
    /// Viewer mode, with desktop or mobile viewer
    Default = 0,
    /// Intended for use with augmented reality viewers (e.g. mobile AR)
    AR,
    /// Intended for use with virtual reality viewers (e.g. Quest 2)
    VR,
    /// Intended for use with creator privileges (e.g. designers and editors customizing a space)
    Creator,
    Num
};

/// @brief Enumerates the supported locomotion models available for the avatar movements.
enum class LocomotionModel
{
    Grounded = 0,
    FreeCamera,
    Num
};

/// @brief Enumerates the list of properties that can be replicated for an avatar component.
enum class AvatarComponentPropertyKeys
{
    AvatarId = 0,
    UserId,
    State,
    AvatarMeshIndex,
    AgoraUserId,
    CustomAvatarUrl,
    IsHandIKEnabled,
    TargetHandIKTargetLocation,
    HandRotation,
    HeadRotation,
    WalkRunBlendPercentage,
    TorsoTwistAlpha,
    AvatarPlayMode,
    MovementDirection,
    LocomotionModel,
    Num
};

// @ingroup AvatarSpaceComponent
/// @brief Data representation of an AvatarSpaceComponent.
class CSP_API AvatarSpaceComponent : public ComponentBase
{
public:
    /// @brief Constructs the avatar space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    AvatarSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the ID for the avatar of this avatar component.
    /// @note Used for selecting a specific avatar depending on the user's preferences.
    /// @return The ID of the currently active avatar of this avatar component.
    const csp::common::String& GetAvatarId() const;

    /// @brief Sets the ID for the avatar of this avatar component.
    /// @note Used for selecting a specific avatar depending on the user's preferences.
    /// @param Value The ID of the currently active avatar of this avatar component.
    void SetAvatarId(const csp::common::String& Value);

    /// @brief Gets the ID of the user that controls this avatar.
    /// @return The ID of the user controlling this avatar.
    const csp::common::String& GetUserId() const;

    /// @brief Sets the ID of the user that controls this avatar.
    /// @param Value The ID of the user controlling this avatar.
    void SetUserId(const csp::common::String& Value);

    /// @brief Gets the state of the current avatar.
    /// @return The state of the current avatar.
    AvatarState GetState() const;

    /// @brief Sets the state of the current avatar.
    /// @param Value The state of the current avatar.
    void SetState(AvatarState Value);

    /// @brief Gets the play mode used by this avatar.
    /// @return The play mode used by this avatar.
    AvatarPlayMode GetAvatarPlayMode() const;

    /// @brief Sets the play mode used by this avatar.
    /// @param Value The play mode used by this avatar.
    void SetAvatarPlayMode(AvatarPlayMode Value);

    /// @brief Gets the ID of the mesh of the avatar of this component.
    /// @note  Used to establish which mesh this avatar should use among a collection of predefined meshes.
    /// @return The ID of the mesh of the avatar of this component.
    const int64_t GetAvatarMeshIndex() const;

    /// @brief Sets the ID of the mesh of the avatar of this component.
    /// @note Used to establish which mesh this avatar should use among a collection of predefined meshes.
    /// @param Value The ID of the mesh of the avatar of this component.
    void SetAvatarMeshIndex(int64_t Value);

    /// @brief Gets the ID of the Agora user bounded to this avatar.
    /// @note When using voice chat, an Agora user is associated with a specific avatar component, so that
    ///       it is possible to associate the person speaking via the Agora voice chat through the relative avatar.
    /// @return The ID of the Agora user associated with this avatar component.
    const csp::common::String& GetAgoraUserId() const;

    /// @brief Sets the ID of the Agora user bounded to this avatar.
    /// @note When using voice chat, an Agora user is associated with a specific avatar component, so that
    ///       it is possible to associate the person speaking via the Agora voice chat through the relative avatar.
    /// @param Value The ID of the Agora user associated with this avatar component.
    void SetAgoraUserId(const csp::common::String& Value);

    /// @brief Gets the URL of a custom mesh for this avatar.
    /// @note This is intended for use with external avatar managers, such as ReadyPlayerMe.
    /// @return The URL of the custom mesh this avatar component uses for its avatar.
    const csp::common::String& GetCustomAvatarUrl() const;

    /// @brief Sets the URL of a custom mesh for this avatar.
    /// @note This is intended for use with external avatar managers, such as ReadyPlayerMe.
    /// @param Value The URL of the custom mesh this avatar component uses for its avatar.
    void SetCustomAvatarUrl(const csp::common::String& Value);

    /// @brief Checks if the Hands Inverse Kinematics (IK) are enabled for this avatar.
    /// @note Intended for use in VR or with virtual hands controllers.
    /// @return True if the avatar uses IK, false otherwise.
    const bool GetIsHandIKEnabled() const;

    /// @brief Sets if the Hands Inverse Kinematics (IK) are enabled for this avatar.
    /// @note Intended for use in VR or with virtual hands controllers.
    /// @param Value True if the avatar uses IK, false otherwise.
    void SetIsHandIKEnabled(bool Value);

    /// @brief Gets the location of the target used for the hands IK.
    /// @note Used in combination with hand IK if enabled.
    /// @return The tartget location to use for the hands IK.
    const csp::common::Vector3& GetTargetHandIKTargetLocation() const;

    /// @brief Sets the location of the target used for the hands IK.
    /// @note Used in combination with hand IK if enabled.
    /// @param Value The tartget location to use for the hands IK.
    void SetTargetHandIKTargetLocation(const csp::common::Vector3& Value);

    /// @brief Gets the rotation of the avatar hand.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    /// @return The rotation of the avatar hand.
    const csp::common::Vector4& GetHandRotation() const;

    /// @brief Sets the rotation of the avatar hand.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    /// @param Value The tartget location to use for the hands IK.
    void SetHandRotation(const csp::common::Vector4& Value);

    /// @brief Gets the rotation of the avatar head.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    /// @return The rotation of the avatar head.
    const csp::common::Vector4& GetHeadRotation() const;

    /// @brief Sets the rotation of the avatar head.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    /// @param Value The rotation of the avatar head.
    void SetHeadRotation(const csp::common::Vector4& Value);

    /// @brief Gets blending between walk and run states expressed in percentage.
    /// @note Used to calculate a smooth transition between an avatar walking and an avatar running.
    ///       When 0 the avatar is fully walking, when 100 the avatar is fully running.
    /// @return The percentage of the blending between walk and run states.
    const float GetWalkRunBlendPercentage() const;

    /// @brief Sets blending between walk and run states expressed in percentage.
    /// @note Used to calculate a smooth transition between an avatar walking and an avatar running.
    ///       When 0 the avatar is fully walking, when 100 the avatar is fully running.
    /// @param Value The percentage of the blending between walk and run states.
    void SetWalkRunBlendPercentage(float Value);

    /// @brief Gets the angle to use to twist the avatar's torso.
    /// @note To calculate the twist of the avatar torso, it is convention to evaluate the
    ///       difference between the avatar's hip forward direction and the avatar's head forward direction.
    ///       The greater the difference, the further the torso should be twisted.
    ///       Positive value if the torso is turning right, negative if avatar is turning left.
    /// @return The angle to use to twist the avatar's torso.
    const float GetTorsoTwistAlpha() const;

    /// @brief Sets the angle to use to twist the avatar's torso.
    /// @note To calculate the twist of the avatar torso, it is convention to evaluate the
    ///       difference between the avatar's hip forward direction and the avatar's head forward direction.
    ///       The greater the difference, the further the torso should be twisted.
    ///       Positive value if the torso is turning right, negative if avatar is turning left.
    /// @param Value The angle to use to twist the avatar's torso.
    void SetTorsoTwistAlpha(float Value);

    /// @brief Gets a vector that represents the movement direction of the avatar.
    /// @return The vector representing the movement direction of the avatar.
    const csp::common::Vector3& GetMovementDirection() const;

    /// @brief Sets a vector that represents the movement direction of the avatar.
    /// @param Value The vector representing the movement direction of the avatar.
    void SetMovementDirection(const csp::common::Vector3& Value);

    /// @brief Specifies which locomotion model this avatar component is using.
    /// @return The locomotion model used by this avatar component.
    LocomotionModel GetLocomotionModel() const;

    /// @brief Sets which locomotion model this avatar component is using.
    /// @param Value The locomotion model used by this avatar component.
    void SetLocomotionModel(LocomotionModel Value);
};

} // namespace csp::multiplayer
