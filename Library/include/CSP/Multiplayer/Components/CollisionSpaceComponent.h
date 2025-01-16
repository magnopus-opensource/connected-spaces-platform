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
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"

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
/// @brief Add box, mesh, capsule and sphere colliders to objects in your Space.
///
/// These colliders can act as triggers, which can be used in conjunction with Scripts to drive behavior.
class CSP_API CollisionSpaceComponent : public ComponentBase, public IThirdPartyComponentRef, public ITransformComponent
{
public:
    /// @brief Constructs the collision space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    CollisionSpaceComponent(SpaceEntity* Parent);

    /// \addtogroup ITransformComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @copydoc IRotationComponent::GetRotation()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IRotationComponent::SetRotation()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @copydoc IScaleComponent::GetScale()
    const csp::common::Vector3& GetScale() const override;
    /// @copydoc IScaleComponent::SetScale()
    void SetScale(const csp::common::Vector3& InValue) override;
    /// @copydoc ITransformComponent::GetTransform()
    SpaceTransform GetTransform() const override;
    /// @copydoc ITransformComonent::SetTransform()
    void SetTransform(const SpaceTransform& InValue) override;
    /// @}

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
