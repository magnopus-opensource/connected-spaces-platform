#pragma once
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

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

enum class AvatarPlayMode
{
    Default = 0,
    AR,
    VR,
    Creator,
    Num
};

enum class LocomotionModel
{
    Grounded = 0,
    FreeCamera,
    Num
};

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

class OLY_API AvatarSpaceComponent : public ComponentBase
{
public:
    AvatarSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetAvatarId() const;
    void SetAvatarId(const oly_common::String& Value);
    const oly_common::String& GetUserId() const;
    void SetUserId(const oly_common::String& Value);
    AvatarState GetState() const;
    void SetState(AvatarState Value);
    AvatarPlayMode GetAvatarPlayMode() const;
    void SetAvatarPlayMode(AvatarPlayMode Value);
    const int64_t GetAvatarMeshIndex() const;
    void SetAvatarMeshIndex(int64_t Value);
    const oly_common::String& GetAgoraUserId() const;
    void SetAgoraUserId(const oly_common::String& Value);
    const oly_common::String& GetCustomAvatarUrl() const;
    void SetCustomAvatarUrl(const oly_common::String& Value);
    const bool GetIsHandIKEnabled() const;
    void SetIsHandIKEnabled(bool Value);
    const oly_common::Vector3& GetTargetHandIKTargetLocation() const;
    void SetTargetHandIKTargetLocation(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetHandRotation() const;
    void SetHandRotation(const oly_common::Vector4& Value);
    const oly_common::Vector4& GetHeadRotation() const;
    void SetHeadRotation(const oly_common::Vector4& Value);
    const float GetWalkRunBlendPercentage() const;
    void SetWalkRunBlendPercentage(float Value);
    const float GetTorsoTwistAlpha() const;
    void SetTorsoTwistAlpha(float Value);
    const oly_common::Vector3& GetMovementDirection() const;
    void SetMovementDirection(const oly_common::Vector3& Value);
    LocomotionModel GetLocomotionModel() const;
    void SetLocomotionModel(LocomotionModel Value);
};

} // namespace oly_multiplayer
