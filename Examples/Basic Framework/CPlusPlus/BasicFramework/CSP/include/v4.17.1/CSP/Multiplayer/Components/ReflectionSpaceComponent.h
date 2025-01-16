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
/// @file ReflectionSpaceComponent.h
/// @brief Definitions and support for reflection components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a reflection component.
enum class ReflectionPropertyKeys
{
    Name,
    ReflectionAssetId,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    ReflectionShape,
    ThirdPartyComponentRef,
    Num
};

/// @brief Enumerates the supported shapes for a reflection component.
enum class ReflectionShape
{
    UnitSphere = 0,
    UnitBox
};

/// @ingroup ReflectionSpaceComponent
/// @brief Data representation of an ReflectionSpaceComponent.
class CSP_API ReflectionSpaceComponent : public ComponentBase, public IThirdPartyComponentRef
{
public:
    /// @brief Constructs the reflection component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    ReflectionSpaceComponent(SpaceEntity* Parent);

    /// @brief Get the name of the Reflection component.
    /// @return csp::common::String specifying the component name.
    const csp::common::String& GetName() const;

    /// @brief Set the name for the Reflection component.
    /// @param Value const csp::common::String& : Name for the Reflection component.
    void SetName(const csp::common::String& Value);

    /// @brief Get the Asset Id for the Reflection texture asset.
    const csp::common::String& GetReflectionAssetId() const;

    /// @brief Set the Asset Id for the Reflection texture asset.
    /// @param Value const csp::common::String& : Id for Reflection texture asset.
    void SetReflectionAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's reflection asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's reflection asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetAssetCollectionId(const csp::common::String& Value);

    /// @brief Position of the Reflection component.
    const csp::common::Vector3& GetPosition() const;

    /// @brief Set the position of the Reflection component.
    /// @param Value const csp::common::Vector3& : Position of the Reflection Component.
    void SetPosition(const csp::common::Vector3& Value);

    /// @brief Rotation of the Reflection component.
    /// @return Will return an Identity Quaternion (0, 0, 0, 1).
    const csp::common::Vector4& GetRotation() const;

    /// @brief Scale of the Reflection components spatial extents over which the reflection should apply.
    /// UnitBox/Sphere * Scale == Spatial Extents.
    const csp::common::Vector3& GetScale() const;

    /// @brief Set the scale of the Reflection components spatial extents.
    /// @param Value const csp::common::Vector3& : Scale extents of the Reflection Component.
    /// UnitBox/Sphere * Scale == Spatial Extents.
    void SetScale(const csp::common::Vector3& Value);

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

    /// \addtogroup IThirdPartyComponentRef
    /// @{
    /// @copydoc IThirdPartyComponentRef::GetThirdPartyComponentRef()
    const csp::common::String& GetThirdPartyComponentRef() const override;
    /// @copydoc IThirdPartyComponentRef::SetThirdPartyComponentRef()
    void SetThirdPartyComponentRef(const csp::common::String& InValue) override;
    /// @}
};

} // namespace csp::multiplayer
