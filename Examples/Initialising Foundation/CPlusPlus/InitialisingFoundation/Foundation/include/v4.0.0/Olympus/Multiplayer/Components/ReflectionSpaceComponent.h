#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class ReflectionPropertyKeys
{
    Name,
    ReflectionAssetId,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    ReflectionShape,
    Num
};

enum class ReflectionShape
{
    UnitSphere = 0,
    UnitBox
};

/// @ingroup ReflectionSpaceComponent
/// @brief Data representation of an ReflectionSpaceComponent.
class OLY_API ReflectionSpaceComponent : public ComponentBase
{
public:
    ReflectionSpaceComponent(SpaceEntity* Parent);

    /// @brief Get the name of the Reflection component.
    /// @return oly_common::String specifying the component name.
    const oly_common::String& GetName() const;
    /// @brief Set the name for the Reflection component.
    /// @param Value const oly_common::String& : Name for the Reflection component.
    void SetName(const oly_common::String& Value);
    /// @brief Get the Asset Id for the Reflection texture asset.
    const oly_common::String& GetReflectionAssetId() const;
    /// @brief Set the Asset Id for the Reflection texture asset.
    /// @param Value const oly_common::String& : Id for Reflection texture asset.
    void SetReflectionAssetId(const oly_common::String& Value);
    /// @brief Get the Id of the AssetCollection the reflection texture asset is associated with.
    const oly_common::String& GetAssetCollectionId() const;
    /// @brief Set the Id of the AssetCollection the reflection texture asset is associated with.
    /// @param Value const oly_common::String& : AssetCollection Id for reflection texture asset.
    void SetAssetCollectionId(const oly_common::String& Value);
    /// @brief Position of the Reflection component.
    const oly_common::Vector3& GetPosition() const;
    /// @brief Set the position of the Reflection component.
    /// @param Value const oly_common::Vector3& : Position of the Reflection Component.
    void SetPosition(const oly_common::Vector3& Value);
    /// @brief Rotation of the Reflection component.
    /// @return Will return an Identity Quaternion (0, 0, 0, 1).
    const oly_common::Vector4& GetRotation() const;
    /// @brief Scale of the Reflection components spatial extents over which the reflection should apply.
    /// UnitBox/Sphere * Scale == Spatial Extents.
    const oly_common::Vector3& GetScale() const;
    /// @brief Set the scale of the Reflection components spatial extents.
    /// @param Value const oly_common::Vector3& : Scale extents of the Reflection Component.
    /// UnitBox/Sphere * Scale == Spatial Extents.
    void SetScale(const oly_common::Vector3& Value);
    /// @brief Get the reflection shape enum value.
    /// ReflectionShape.UnitBox: Projects a texture in a planar fashion from all six directions (like an inward facing cube).
    /// ReflectionShape.UnitSphere: Warps the texture into a spherical shape and projects it onto a surface.
    /// @return Enum specifying whether the captured reflections are box projected (UnitCube) or spherical projected (UnitSphere).
    ReflectionShape GetReflectionShape() const;
    /// @brief Set the reflection shape.
    /// ReflectionShape.UnitBox: Projects a texture in a planar fashion from all six directions (like an inward facing cube).
    /// ReflectionShape.UnitSphere: Warps the texture into a spherical shape and projects it onto a surface.
    /// @param Value ReflectionShape : Enum specifying whether the captured reflections are box (UnitCube) or spherical projected (UnitSphere).
    void SetReflectionShape(ReflectionShape Value);
};

} // namespace oly_multiplayer
