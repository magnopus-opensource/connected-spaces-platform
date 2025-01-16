#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class CollisionPropertyKeys
{
    Position = 0,
    Rotation,
    Scale,
    CollisionShape,
    CollisionMode,
    CollisionAssetId,
    AssetCollectionId,
    Num
};

enum class CollisionShape
{
    Box = 0,
    Mesh,
    Capsule,
    Sphere
};

enum class CollisionMode
{
    Collision = 0,
    Trigger
};

/// @ingroup CollisionSpaceComponent
/// @brief Data representation of an CollisionSpaceComponent.
class OLY_API CollisionSpaceComponent : public ComponentBase
{
public:
    CollisionSpaceComponent(SpaceEntity* Parent);

    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetRotation() const;
    void SetRotation(const oly_common::Vector4& Value);
    const oly_common::Vector3& GetScale() const;
    void SetScale(const oly_common::Vector3& Value);

    CollisionShape GetCollisionShape() const;
    void SetCollisionShape(CollisionShape collisionShape);
    CollisionMode GetCollisionMode() const;
    void SetCollisionMode(CollisionMode collisionMode);

    const oly_common::String& GetCollisionAssetId() const;
    void SetCollisionAssetId(const oly_common::String& Value);
    const oly_common::String& GetAssetCollectionId() const;
    void SetAssetCollectionId(const oly_common::String& Value);

    const oly_common::Vector3 GetUnscaledBoundingBoxMin();
    const oly_common::Vector3 GetUnscaledBoundingBoxMax();

    const oly_common::Vector3 GetScaledBoundingBoxMin();
    const oly_common::Vector3 GetScaledBoundingBoxMax();

    static float GetDefaultSphereRadius();
    static float GetDefaultCapsuleHalfWidth();
    static float GetDefaultCapsuleHalfHeight();
};

} // namespace oly_multiplayer
#pragma once
