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

/// @file ButtonSpaceComponent.h
/// @brief Definitions and support for button components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "CSP/Multiplayer/SpaceTransform.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a button component.
enum class ButtonPropertyKeys
{
    Name_DEPRECATED = 0,
    LabelText,
    IconAssetId,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsVisible,
    IsEnabled,
    IsARVisible,
    Num
};

/// @ingroup ButtonSpaceComponent
/// @brief Add a clickable button to your space. Button click events can be responded to via scripts.
class CSP_API ButtonSpaceComponent : public ComponentBase, public IEnableableComponent, public ITransformComponent, public IVisibleComponent
{
public:
    /// @brief Constructs the button space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    ButtonSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the text of the label of this button.
    /// @return The text on the label of this button.
    const csp::common::String& GetLabelText() const;

    /// @brief Sets the text of the label of this button.
    /// @param Value The text on the label of this button.
    void SetLabelText(const csp::common::String& Value);

    /// @brief Gets the ID of the icon asset associated with the button of this component.
    /// @note This is used to show a specific icon on the button by ID.
    /// @return The ID of the icon asset associated with the button of this component.
    const csp::common::String& GetIconAssetId() const;

    /// @brief Sets the ID of the icon asset associated with the button of this component.
    /// @note This is used to show a specific icon on the button by ID.
    /// @param Value The ID of the icon asset associated with the button of this component.
    void SetIconAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's button asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's button asset, both the Asset ID and the Asset Collection ID are required.
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

    /// \addtogroup IClickableComponent
    /// @{
    /// @copydoc IClickableComponent::GetIsEnabled()
    bool GetIsEnabled() const override;
    /// @copydoc IClickableComponent::SetIsEnabled()
    void SetIsEnabled(bool InValue) override;
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
