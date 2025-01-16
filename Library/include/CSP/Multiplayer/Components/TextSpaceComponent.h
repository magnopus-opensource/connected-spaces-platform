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

/// @file TextSpaceComponent.h
/// @brief Definitions and support for text components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/BillBoardModeEnum.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for an text space component.
enum class TextPropertyKeys
{
    Position,
    Rotation,
    Scale,
    Text,
    TextColor,
    BackgroundColor,
    IsBackgroundVisible,
    Width,
    Height,
    BillboardMode,
    IsVisible,
    IsARVisible,
    Num
};

/// @ingroup TextSpaceComponent
/// @brief Add a spatial representation of text to your space.
class CSP_API TextSpaceComponent : public ComponentBase, public ITransformComponent, public IVisibleComponent
{
public:
    /// @brief Constructs the text space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    TextSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the text  this text component refers to.
    /// @return The text this text component refers to.
    const csp::common::String& GetText() const;

    /// @brief Sets the text this text component refers to.
    /// @param Value The text this text component refers to.
    void SetText(const csp::common::String& Value);

    /// @brief Gets the text color.
    /// @return The text color, expected to be in RGB color space, with each value normalised between 0...1.
    const csp::common::Vector3& GetTextColor() const;

    /// @brief Sets the text color.
    /// @param Value The text color, expected to be in RGB color space, with each value normalised between 0...1.
    void SetTextColor(const csp::common::Vector3& Value);

    /// @brief Gets the background color that should be globally applied text associated with this component.
    /// @return The background color, expected to be in RGB color space, with each value normalised between 0...1.
    const csp::common::Vector3& GetBackgroundColor() const;

    /// @brief Sets the background color.
    /// @param Value The background color, expected to be in RGB color space, with each value normalised between 0...1.
    void SetBackgroundColor(const csp::common::Vector3& Value);

    /// @brief Sets the background visibility.
    /// @param Value The background visibility.
    bool GetIsBackgroundVisible() const;
    /// @brief Sets the background visibility.
    /// @param Value The background visibility.
    void SetIsBackgroundVisible(bool InValue);

    /// @brief Sets the Text Width.
    /// @param Value The Text Width.
    float GetWidth() const;
    /// @brief Sets the Text Width.
    /// @param Value The Text Width.
    void SetWidth(float InValue);

    /// @brief Sets the Text Height.
    /// @param Value The Text Height.
    float GetHeight() const;
    /// @brief Sets the Text Height.
    /// @param Value The Text Height.
    void SetHeight(float InValue);

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

    /// @brief Gets the billboard mode used by this text component.
    /// @return The billboard mode used by this text component.
    BillboardMode GetBillboardMode() const;

    /// @brief Sets the billboard mode used by this text component.
    /// @param billboardMode The billboard mode used by this text component.
    void SetBillboardMode(BillboardMode billboardMode);

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
