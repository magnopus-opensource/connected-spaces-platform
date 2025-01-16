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
/// @file StaticModelSpaceComponent.h
/// @brief Definitions and support for static models.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IExternalResourceComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IShadowCasterComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a static model component.
enum class StaticModelPropertyKeys
{
    Name = 0,
    ExternalResourceAssetId,
    ExternalResourceAssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsVisible,
    IsARVisible,
    ThirdPartyComponentRef,
    IsShadowCaster,
    Num
};

/// @ingroup StaticModelSpaceComponent
/// @brief Data representation of an StaticModelSpaceComponent.
class CSP_API StaticModelSpaceComponent : public ComponentBase,
                                          public ITransformComponent,
                                          public IVisibleComponent,
                                          public IExternalResourceComponent,
                                          public IThirdPartyComponentRef,
                                          public IShadowCasterComponent
{
public:
    /// @brief Constructs the static model space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    StaticModelSpaceComponent(SpaceEntity* Parent);

    /* clang-format off */
	[[deprecated("Due to the introduction of LODs it doesn't make sense to set a specific asset anymore")]]
    const csp::common::String& GetExternalResourceAssetId() const override;

	[[deprecated("Due to the introduction of LODs it doesn't make sense to set a specific asset anymore")]]
    void SetExternalResourceAssetId(const csp::common::String& Value) override;
    /* clang-format on */

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's static asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetExternalResourceAssetCollectionId() const override;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's static asset, both the Asset ID and the Asset Collection ID are required.
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

    /// \addtogroup IThirdPartyComponentRef
    /// @{
    /// @copydoc IThirdPartyComponentRef::GetThirdPartyComponentRef()
    const csp::common::String& GetThirdPartyComponentRef() const override;
    /// @copydoc IThirdPartyComponentRef::SetThirdPartyComponentRef()
    void SetThirdPartyComponentRef(const csp::common::String& InValue) override;
    /// @}

    /// \addtogroup IShadowCasterComponent
    /// @{
    /// @copydoc IShadowCasterComponent::GetIsShadowCaster()
    bool GetIsShadowCaster() const override;
    /// @copydoc IShadowCasterComponent::SetIsShadowCaster()
    void SetIsShadowCaster(bool Value) override;
    /// @}
};

} // namespace csp::multiplayer
