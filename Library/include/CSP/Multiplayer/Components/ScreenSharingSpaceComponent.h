/*
 * Copyright 2025 Magnopus LLC

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

/// @file ScreenSharingSpaceComponent.h
/// @brief Definitions and support for screen sharing components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IShadowCasterComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a screen sharing component.
enum class ScreenSharingPropertyKeys
{
    Position = 0,
    Rotation,
    Scale,
    IsVisible,
    IsARVisible,
    IsShadowCaster,
    UserId,
    DefaultImageCollectionId,
    DefaultImageAssetId,
    AttenuationRadius,
    Num
};

/// @ingroup ScreenSharingSpaceComponent
/// @brief Enables screen sharing within the space.
///
/// The screen sharing component facilitates the sharing of a user's screen within a space.
/// The component itself does not make assumptions about the mechanism by which the screen is shared.
/// It provides properties to define a default image to be displayed when no users are
/// sharing their screen, as well a UserId property to store the Id of the user currently sharing their screen.
class CSP_API ScreenSharingSpaceComponent : public ComponentBase, public IShadowCasterComponent, public ITransformComponent, public IVisibleComponent
{
public:
    /// @brief Constructs the screen sharing component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    ScreenSharingSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    /// @brief Gets the ID of the user who is currently sharing their screen to this component.
    /// @note An empty string means that no user is currently sharing their screen to this component.
    /// @return csp::common::String& : The ID of the user who is currently sharing their screen to this component.
    const csp::common::String& GetUserId() const;

    /// @brief Sets the ID of the user who is sharing their screen to this component.
    /// @param Value csp::common::String& : The ID of the user who is sharing their screen to this component. This must be set to an empty string when
    /// the user finishes sharing.
    void SetUserId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with the default image for this component.
    /// @note To retrieve this component's default image, both the DefaultImageCollectionId and the DefaultImageAssetId are required.
    /// @return csp::common::String& : The ID of the default image asset collection associated with this component.
    const csp::common::String& GetDefaultImageCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with the default image for this component.
    /// @note To retrieve this component's default image, both the DefaultImageCollectionId and the DefaultImageAssetId are required.
    /// @param Value csp::common::String& : The ID of the default image asset collection associated with this component.
    void SetDefaultImageCollectionId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset associated with the default image for this component.
    /// @note To retrieve this component's default image, both the DefaultImageCollectionId and the DefaultImageAssetId are required.
    /// @return csp::common::String& : The ID of the default image asset associated with this component.
    const csp::common::String& GetDefaultImageAssetId() const;

    /// @brief Sets the ID of the asset associated with the default image for this component.
    /// @note To retrieve this component's default image, both the DefaultImageCollectionId and the DefaultImageAssetId are required.
    /// @param Value csp::common::String& : The ID of the default image asset associated with this component.
    void SetDefaultImageAssetId(const csp::common::String& Value);

    /// @brief Gets the radius from this component origin within which the audio of this video can be heard by the user.
    /// @note Only when the user position is within this radius the audio of the video should be heard.
    /// @return The radius within which the audio of the video can be heard by the user.
    float GetAttenuationRadius() const;

    /// @brief Sets the radius from this component origin within which the audio of this video can be heard by the user.
    /// @note Only when the user position is within this radius the audio of the video should be heard.
    /// @param Value The radius within which the audio of the video can be heard by the user.
    void SetAttenuationRadius(float Value);

    // Attenuation radius

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
    bool GetIsShadowCaster() const override;
    /// @copydoc IShadowCasterComponent::SetIsShadowCaster()
    void SetIsShadowCaster(bool Value) override;
    /// @}
};

} // namespace csp::multiplayer
