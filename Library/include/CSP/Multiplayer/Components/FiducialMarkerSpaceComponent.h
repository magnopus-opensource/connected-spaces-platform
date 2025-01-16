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

/// @file FiducialMarkerSpaceComponent.h
/// @brief Definitions and support for fiducial marker components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a fiducial marker space component.
enum class FiducialMarkerPropertyKeys
{
    Name_DEPRECATED,
    MarkerAssetId,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsVisible,
    IsARVisible,
    Num
};

/// @ingroup FiducialMarkerSpaceComponent
/// @brief As an alternative to cloud-based anchors, fiducial markers can be used to anchor your space to a physical location.
class CSP_API FiducialMarkerSpaceComponent : public ComponentBase, public ITransformComponent, public IVisibleComponent
{
public:
    /// @brief Constructs the fiducial marker space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    FiducialMarkerSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the name of this fiducial marker space component.
    /// @return The name of this fiducial marker space component.
    [[deprecated("Deprecated in favour of ComponentBase::GetComponentName()")]] const csp::common::String& GetName() const;

    /// @brief Sets the name of this fiducial marker space component.
    /// @param Value The name of this fiducial marker space component.
    [[deprecated("Deprecated in favour of ComponentBase::SetComponentName()")]] void SetName(const csp::common::String& Value);

    /// @brief Gets the ID of the image asset this image component refers to.
    /// @return The ID of the image asset this image component refers to.
    const csp::common::String& GetMarkerAssetId() const;

    /// @brief Sets the ID of the image asset this fiducial marker component refers to.
    /// @param Value The ID of the image asset this fiducial marker component refers to.
    void SetMarkerAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's image asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's image asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetAssetCollectionId(const csp::common::String& Value);

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

    /// \addtogroup IVisibleComponent
    /// @{
    /// @copydoc IVisibleComponent::GetIsVisible()
    bool GetIsVisible() const override;
    /// @copydoc IVisibleComponent::SetIsVisible()
    void SetIsVisible(bool InValue) override;
    /// @copydoc IVisibleComponent::GetIsARVisible()
    bool GetIsARVisible() const override;
    /// @copydoc IVisibleComponent::SetIsARVisible()
    void SetIsARVisible(bool InValue) override;
    /// @}
};

} // namespace csp::multiplayer
