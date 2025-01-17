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

/// @file ExternalLinkSpaceComponent.h
/// @brief Definitions and support for external links.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief The type of actions that can be performed by an external link component.
enum class ExternalLinkActions
{
    LinkClicked,
    Num
};

/// @brief The list of properties defined within an external link component.
enum class ExternalLinkPropertyKeys
{
    Name_DEPRECATED = 0,
    LinkUrl,
    Position,
    Rotation,
    Scale,
    DisplayText,
    IsEnabled,
    IsVisible,
    IsARVisible,
    Num
};

/// @ingroup ExternalLinkSpaceComponent
/// @brief Used to handle external URLs that can be opened from within a space.
class CSP_API ExternalLinkSpaceComponent : public ComponentBase, public IEnableableComponent, public ITransformComponent, public IVisibleComponent
{
public:
    /// @brief Creates an external link component that can be added to an existing space entity.
    /// @param Parent - The space entity to which this new component will belong to.
    ExternalLinkSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the name of this external link component.
    [[deprecated("Deprecated in favour of ComponentBase::GetComponentName()")]] const csp::common::String& GetName() const;
    /// @brief Sets the name for this external link component.
    /// @param Value - The new name to assign to this external link componenent.
    [[deprecated("Deprecated in favour of ComponentBase::SetComponentName()")]] void SetName(const csp::common::String& Value);

    /// @brief Gets the URL address to which this external link component redirects the user on trigger.
    const csp::common::String& GetLinkUrl() const;
    /// @brief Sets the URL address to which this external link component redirects the user to on trigger.
    /// @param Value - The URL to be assigned to this external link component.
    void SetLinkUrl(const csp::common::String& Value);

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

    /// @brief Gets the text that will be displayed by the component as hyperlink to the URL it redirects to.
    const csp::common::String& GetDisplayText() const;
    /// @brief Sets the text that will be displayed by the component as hyperlink to the URL it redirects to.
    /// @param Value - The new text to be displayed as hyperlink.
    void SetDisplayText(const csp::common::String& Value);

    /// \addtogroup IEnableableComponent
    /// @{
    /// @copydoc IEnableableComponent::GetIsEnabled()
    bool GetIsEnabled() const override;
    /// @copydoc IEnableableComponent::SetIsEnabled()
    void SetIsEnabled(bool InValue) override;
    /// @}

    /// \addtogroup IVisibleComponent
    /// @{
    /// @copydoc IVisibleComponent::GetIsVisible()
    virtual bool GetIsVisible() const override;
    /// @copydoc IVisibleComponent::SetIsVisible()
    virtual void SetIsVisible(bool InValue) override;
    /// @copydoc IVisibleComponent::GetIsARVisible()
    virtual bool GetIsARVisible() const override;
    /// @copydoc IVisibleComponent::SetIsARVisible()
    virtual void SetIsARVisible(bool InValue) override;
    /// @}
};

} // namespace csp::multiplayer
