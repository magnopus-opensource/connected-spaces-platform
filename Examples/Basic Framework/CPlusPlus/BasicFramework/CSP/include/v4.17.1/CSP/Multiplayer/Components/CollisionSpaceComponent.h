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
/// @file CollisionSpaceComponent.h
/// @brief Definitions and support for collisions.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a collision component.
enum class CollisionPropertyKeys
{
    Position = 0,
    Rotation,
    Scale,
    CollisionShape,
    CollisionMode,
    CollisionAssetId,
    AssetCollectionId,
    ThirdPartyComponentRef,
    Num
};

/// @brief Enumerates the list of shapes supported by the collision component.
enum class CollisionShape
{
    Box = 0,
    Mesh,
    Capsule,
    Sphere
};

/// @brief Enumerates the list of collision modes supported by the collision component.
enum class CollisionMode
{
    Collision = 0,
    Trigger
};

/// @ingroup CollisionSpaceComponent
/// @brief Data representation of an CollisionSpaceComponent.
class CSP_API CollisionSpaceComponent : public ComponentBase, public IThirdPartyComponentRef
{
public:
    /// @brief Constructs the collision space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    CollisionSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the position of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification, in meters.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    /// @return The 3D position as vector (left, up, forward) in meters.
    const csp::common::Vector3& GetPosition() const;

    /// @brief Sets the position of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification, in meters.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    void SetPosition(const csp::common::Vector3& Value);

    /// @brief Gets a quaternion representing the rotation of the origin of this component, expressed in radians.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    const csp::common::Vector4& GetRotation() const;

    /// @brief Sets the rotation of the origin of this component according to the specified quaternion "Value", expressed in radians.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    /// @param Value The quaternion in radians to use as new rotation of this component.
    void SetRotation(const csp::common::Vector4& Value);

    /// @brief Gets the scale of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    /// @return The 3D scale as vector (left, up, forward).
    const csp::common::Vector3& GetScale() const;

    /// @brief Sets the scale of the origin of this component in world space to the specified "Value".
    /// @param Value The new value expressed as vector (left, up, forward).
    /// @note The coordinate system used follows the glTF 2.0 specification.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    void SetScale(const csp::common::Vector3& Value);

    /// @brief Gets the collision shape used by this collision component.
    /// @return The colllision shape used by this collision component.
    CollisionShape GetCollisionShape() const;

    /// @brief Sets the collision shape used by this collision component.
    /// @param collisionShape The colllision shape used by this collision component.
    void SetCollisionShape(CollisionShape collisionShape);

    /// @brief Gets the collision mode used by this collision component.
    /// @return The colllision mode used by this collision component.
    CollisionMode GetCollisionMode() const;

    /// @brief Sets the collision mode used by this collision component.
    /// @param collisionMode The colllision mode used by this collision component.
    void SetCollisionMode(CollisionMode collisionMode);

    /// @brief Gets the ID of the collision asset used by this collision component.
    /// @return The ID of the collision asset used by this collision component.
    const csp::common::String& GetCollisionAssetId() const;

    /// @brief Sets the ID of the collision asset used by this collision component.
    /// @param Value The ID of the collision asset used by this collision component.
    void SetCollisionAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's collision asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's collision asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetAssetCollectionId(const csp::common::String& Value);

    /// @brief Gets the minimum unscaled bounding box of this collision component.
    /// @return The minimum unscaled bounding box of this collision component.
    const csp::common::Vector3 GetUnscaledBoundingBoxMin();

    /// @brief Gets the maximum unscaled bounding box of this collision component.
    /// @return The maximum unscaled bounding box of this collision component.
    const csp::common::Vector3 GetUnscaledBoundingBoxMax();

    /// @brief Gets the minimum scaled bounding box of this collision component.
    /// @return The minimum scaled bounding box of this collision component.
    const csp::common::Vector3 GetScaledBoundingBoxMin();

    /// @brief Gets the maximum scaled bounding box of this collision component.
    /// @return The maximum scaled bounding box of this collision component.
    const csp::common::Vector3 GetScaledBoundingBoxMax();

    /// @brief Gets the default radius for a sphere collision mesh.
    /// @return The default radius for a sphere collision mesh.
    static float GetDefaultSphereRadius();

    /// @brief Gets the default half width for a capsule collision mesh.
    /// @return The default half width for a capsule collision mesh.
    static float GetDefaultCapsuleHalfWidth();

    /// @brief Gets the default half height for a capsule collision mesh.
    /// @return The default half height for a capsule collision mesh.
    static float GetDefaultCapsuleHalfHeight();

    /// \addtogroup IThirdPartyComponentRef
    /// @{
    /// @copydoc IThirdPartyComponentRef::GetThirdPartyComponentRef()
    const csp::common::String& GetThirdPartyComponentRef() const override;
    /// @copydoc IThirdPartyComponentRef::SetThirdPartyComponentRef()
    void SetThirdPartyComponentRef(const csp::common::String& InValue) override;
    /// @}
};

} // namespace csp::multiplayer
#pragma once
