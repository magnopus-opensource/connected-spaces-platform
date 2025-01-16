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

/// @file GaussianSplatSpaceComponent.h
/// @brief Definitions and support for Gaussian Splats.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IExternalResourceComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IShadowCasterComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a Gaussian Splat component.
enum class GaussianSplatPropertyKeys
{
    Name_DEPRECATED = 0,
    ExternalResourceAssetId,
    ExternalResourceAssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsVisible,
    IsARVisible,
    IsShadowCaster,
    Tint,
    Num
};

/// @ingroup GaussianSplatSpaceComponent
/// @brief Add Gaussian Splats to your space.
/// Gaussian Splatting is a technique for real-time 3D reconstruction and rendering of an object or environment using images taken from multiple
/// points of view. Rather than representing the object as a mesh of triangles, which has a surface but nothing inside, it is instead represented as a
/// volume, comprising a point cloud of splats (like coloured dots), each of which has a position, colour (with alpha) and covariance (scale on 3
/// axis).
class CSP_API GaussianSplatSpaceComponent : public ComponentBase,
                                            public IExternalResourceComponent,
                                            public IShadowCasterComponent,
                                            public ITransformComponent,
                                            public IVisibleComponent

{
public:
    /// @brief Constructs the Gaussian Splat component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    GaussianSplatSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the ID of the asset associated with this component.
    /// @note To retrieve this component's gaussian splat asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset associated with this component.
    const csp::common::String& GetExternalResourceAssetId() const override;

    /// @brief Sets the ID of the asset associated with this component.
    /// @note To retrieve this component's gaussian splat asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset associated with this component.
    void SetExternalResourceAssetId(const csp::common::String& Value) override;

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's gaussian splat asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetExternalResourceAssetCollectionId() const override;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's gaussian splat asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetExternalResourceAssetCollectionId(const csp::common::String& Value) override;

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

    /// \addtogroup IShadowCasterComponent
    /// @{
    /// @copydoc IShadowCasterComponent::GetIsShadowCaster()
    [[deprecated]] bool GetIsShadowCaster() const override;
    /// @copydoc IShadowCasterComponent::SetIsShadowCaster()
    [[deprecated]] void SetIsShadowCaster(bool Value) override;
    /// @}

    /// @brief Gets the tint that should be globally applied to the Gaussian Splat associated with this component.
    /// @return The global tint value, expected to be in RGB color space, with each value normalised between 0...1.
    const csp::common::Vector3& GetTint() const;

    /// @brief Sets the tint that should be globally applied to the Gaussian Splat.
    /// @param Value The tint value, expected to be in RGB color space, with each value normalised between 0...1.
    /// Defaults to 1,1,1.
    void SetTint(const csp::common::Vector3& TintValue);
};

} // namespace csp::multiplayer
