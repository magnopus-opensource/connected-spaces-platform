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
/// @file FogSpaceComponent.h
/// @brief Definitions and support for fog.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a fog component.
enum class FogPropertyKeys
{
    FogMode = 0,
    Position,
    Rotation,
    Scale,
    StartDistance,
    EndDistance,
    Color,
    Density,
    HeightFalloff,
    MaxOpacity,
    IsVolumetric,
    IsVisible,
    IsARVisible,
    ThirdPartyComponentRef,
    Num
};

/// @brief Enumerates the types of fog supported by the fog component.
enum class FogMode
{
    Linear = 0,
    Exponential,
    Exponential2
};

/// @ingroup FogSpaceComponent
/// @brief Data representation of an FogSpaceComponent.
class CSP_API FogSpaceComponent : public ComponentBase, public IVisibleComponent, public IThirdPartyComponentRef
{
public:
    /// @brief Constructs the fog space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    FogSpaceComponent(SpaceEntity* Parent);

    /// @brief Retrieves the type of fog currently used by this fog component.
    /// @return The modality of fog currently used by this component.
    FogMode GetFogMode() const;

    /// @brief Sets the type of fog currently to be used by this fog component.
    /// @param Value The modality of fog to be used by this component.
    void SetFogMode(FogMode Value);

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

    /// @brief Get start sistance
    /// Note: Distance from camera that the fog will start.
    /// Note: 0 = this property has no effect.
    /// @return Current start distance
    float GetStartDistance() const;

    /// @brief Set Current start distance
    /// Note: Distance from camera that the fog will start.
    /// Note: 0 = this property has no effect.
    /// @param Value float : Current start distance
    void SetStartDistance(float Value);

    /// @brief Get Current end distance
    /// Note: objects passed this distance will not be affected by fog.
    /// Note: 0 = this property has no effect.
    /// @return Current end distance
    float GetEndDistance() const;

    /// @brief Set Current end distance
    /// Note: objects passed this distance will not be affected by fog.
    /// Note: 0 = this property has no effect.
    /// @param Value float : Current end distance
    void SetEndDistance(float Value);

    /// @brief Get fog color
    /// @return Current fog color
    const csp::common::Vector3& GetColor() const;

    /// @brief Set Current fog color
    /// @param Value float : Current fog color
    void SetColor(const csp::common::Vector3& Value);

    /// @brief Get Global density factor
    /// Note: Global density factor
    /// @return Current Global density factor
    float GetDensity() const;

    /// @brief Set Global density factor
    /// Note: Global density factor
    /// @param Value float : Global density factor
    void SetDensity(float Value);

    /// @brief Get Height density factor
    /// Note: Height density factor, controls how the density increases and height decreases. Smaller values make the visible transition larger.
    /// @return Current Height density factor
    float GetHeightFalloff() const;

    /// @brief Set Height density factor
    /// Note: Height density factor, controls how the density increases and height decreases. Smaller values make the visible transition larger.
    /// @param Value float : Height density factor
    void SetHeightFalloff(float Value);

    /// @brief Get Maximum opacity of the Fog.
    /// Maximum opacity of the Fog.
    /// Note: 1 = fog becomes fully opaque at a distance and replaces the scene colour completely.
    /// Note: 0 = fog colour will have no impact.
    /// @return Current Maximum opacity of the Fog
    float GetMaxOpacity() const;

    /// @brief Set Maximum opacity of the Fog.
    /// @param Value float : Maximum opacity of the Fog.
    void SetMaxOpacity(float Value);

    /// @brief Get Is Fog Volumetric.
    /// @param Value float : Fog Volumetric Flag
    bool GetIsVolumetric() const;

    /// @brief Set Is Fog Volumetric
    /// @param Value float : Is Fog Volumetric Flag
    void SetIsVolumetric(bool Value);

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
};

} // namespace csp::multiplayer
