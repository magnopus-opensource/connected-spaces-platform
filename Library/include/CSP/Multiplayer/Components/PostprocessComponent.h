/*
 * Copyright 2026 Magnopus LLC

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

/// @file PostprocessComponent.h
/// @brief Definitions and support for postprocess components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IRotationComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IScaleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a postprocess component.
enum class PostprocessPropertyKeys
{
    Position,
    Rotation,
    Scale,
    ExposureMin,
    ExposureMax,
    IsUnbound,
    Num
};

/// @ingroup PostprocessSpaceComponent
/// @brief Defines postprocess settings which should be applied when rendering a frame to the display for users within a space. 
/// 
/// Optionally, the component can express a bound volume (using the position, rotation and scale of the component) by setting IsUnbound to false.
/// In this scenario, it is expected that client applications treat the component as a transformed unit cube, whose extends define the volume within which these postprocess settings should be applied.
/// By default, the component is unbound, meaning it applies its postprocess settings to the entire space regardless of its position, rotation and scale.
/// 
/// The component describes exposure settings using ISO 100 units. The component does not define how these values should be applied to the final
/// rendered image, but it is expected that client applications convert these values to their own exposure units and apply them as a range of minimum
/// and maximum exposure levels as part of eye adapatation during tonemapping.
class CSP_API PostprocessSpaceComponent : public ComponentBase, public IPositionComponent, public IRotationComponent, public IScaleComponent
{
public:
    /// @brief Constructs the postprocess component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    PostprocessSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    /// \addtogroup IPositionComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @}

    /// \addtogroup IRotationComponent
    /// @{
    /// @copydoc IScaleComponent::GetScale()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IScaleComponent::SetScale()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @}

    /// \addtogroup IScaleComponent
    /// @{
    /// @copydoc IScaleComponent::GetScale()
    const csp::common::Vector3& GetScale() const override;
    /// @copydoc IScaleComponent::SetScale()
    void SetScale(const csp::common::Vector3& InValue) override;
    /// @}

    /// @brief Gets the minimum exposure level of the postprocess volume. Units are expected to be ISO 100.
    /// Defines the darkest exposure level that tonemapping can reach.
    /// Higher values result in a brighter image, while lower values result in a darker image.
    /// @return the ISO 100 exposure value.
    const float GetExposureMin() const;

    /// @brief Sets the minimum exposure level of the postprocess volume. Units are expected to be ISO 100.
    /// Defines the darkest exposure level that tonemapping can reach.
    /// Higher values result in a brighter image, while lower values result in a darker image.
    /// @param InValue the ISO 100 exposure value.
    void SetExposureMin(float InValue);

    /// @brief Gets the maximum exposure level of the postprocess volume. Units are expected to be ISO 100.
    /// Defines the lightest exposure level that tonemapping can reach.
    /// Higher values result in a brighter image, while lower values result in a darker image.
    /// @return the ISO 100 exposure value.
    const float GetExposureMax() const;

    /// @brief Sets the maximum exposure level of the postprocess volume. Units are expected to be ISO 100.
    /// Defines the lightest exposure level that tonemapping can reach.
    /// Higher values result in a brighter image, while lower values result in a darker image.
    /// @param InValue the ISO 100 exposure value.
    void SetExposureMax(float InValue);

    /// @brief Gets whether the postprocess volume is unbound, meaning it applies its effects to the entire space regardless of its position or scale.
    /// By default, this is set to true.
    /// @return true if the postprocess volume is unbound, false if it is bound to its position, rotation and scale.
    bool GetIsUnbound() const;

    /// @brief Sets whether the postprocess volume is unbound, meaning it applies its effects to the entire space regardless of its position or scale.
    /// By default, this is set to true.
    /// @param InValue true to make the postprocess volume unbound, false to bind it to its position, rotation and scale.
    void SetIsUnbound(bool InValue);
};

} // namespace csp::multiplayer
