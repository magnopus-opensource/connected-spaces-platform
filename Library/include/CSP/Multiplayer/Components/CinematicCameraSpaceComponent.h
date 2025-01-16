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

/// @file CinematicCameraSpaceComponent.h
/// @brief Definitions and support for CinematicCamera.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a CinematicCamera component.
enum class CinematicCameraPropertyKeys
{
    Position = 0,
    Rotation,
    IsEnabled,
    FocalLength,
    AspectRatio,
    SensorSize,
    NearClip,
    FarClip,
    Iso,
    ShutterSpeed,
    Aperture,
    IsViewerCamera,
    ThirdPartyComponentRef,
    Num
};

/// @ingroup CinematicCameraSpaceComponent
/// @brief Data representation of an CinematicCameraSpaceComponent.
class CSP_API CinematicCameraSpaceComponent : public ComponentBase,
                                              public IThirdPartyComponentRef,
                                              public IPositionComponent,
                                              public IRotationComponent,
                                              public IEnableableComponent
{
public:
    /// @brief Constructs the CinematicCamera space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    CinematicCameraSpaceComponent(SpaceEntity* Parent);

    /// @brief Gived the sensor size and focal length, return the horizonal fov
    /// @return FOV in radians
    float GetFov() const;

    /// \addtogroup IPositionComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @}

    /// \addtogroup IRotationComponent
    /// @{
    /// @copydoc IRotationComponent::GetRotation()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IRotationComponent::SetRotation()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @}

    /// @brief Get focal length
    /// @return Current focal length
    float GetFocalLength() const;

    /// @brief Set focalLength
    /// Note: Effects the result of GetFov()
    /// @param Value float : focalLength
    void SetFocalLength(float Value);

    /// @brief Get Current aspect ratio
    /// @return Current aspect ratio
    float GetAspectRatio() const;

    /// @brief Set Current aspect ratio
    /// @param Value float : Current aspect ratio
    void SetAspectRatio(float Value);

    /// @brief Get sensor size
    /// @return Current sensor size
    const csp::common::Vector2& GetSensorSize() const;

    /// @brief Set Current SensorSize
    /// @param Value Vector2 : Current SensorSize
    void SetSensorSize(const csp::common::Vector2& Value);

    /// @brief Get near clip
    /// Note: On platforms that don't support reversedZ, near clip should be used to control the clipping distance
    /// @return Current near clip
    float GetNearClip() const;

    /// @brief Set near clip
    /// @param Value float : near clip
    void SetNearClip(float Value);

    /// @brief Get far clip
    /// Note: On platforms that don't support reversedZ, far clip should be used to control the clipping distance
    /// @return Current far clip
    float GetFarClip() const;

    /// @brief Set far clip
    /// Note: far clip, controls how the density increases and height decreases. Smaller values make the visible transition larger.
    /// @param Value float : far clip
    void SetFarClip(float Value);

    /// @brief Get ISO sensitivity for controlling exposure.
    /// Note: reserved for future use, do not implement on clients
    /// @return Current iso
    float GetIso() const;

    /// @brief Set ISO sensitivity for controlling exposure.
    /// Note: reserved for future use, do not implement on clients
    /// @param Value float : ISO sensitivity for controlling exposure.
    void SetIso(float Value);

    /// @brief Get shutter speed.
    /// Note: reserved for future use, do not implement on clients
    /// @param Value float : shutter speed
    float GetShutterSpeed() const;

    /// @brief Set shutter speed
    /// Note: reserved for future use, do not implement on clients
    /// @param Value float : shutter speed
    void SetShutterSpeed(float Value);

    /// @brief Get aperture.
    /// Note: reserved for future use, do not implement on clients
    /// @param Value float : aperture
    float GetAperture() const;

    /// @brief Set aperture
    /// Note: reserved for future use, do not implement on clients
    /// @param Value float : aperture Flag
    void SetAperture(float Value);

    /// @brief Get IsViewerCamera.
    /// Note: reserved for future use, do not implement on clients
    /// @param Value boolean : IsViewerCamera
    bool GetIsViewerCamera() const;

    /// @brief Set IsViewerCamera
    /// Note: reserved for future use, do not implement on clients
    /// @param Value boolean : IsViewerCamera Flag
    void SetIsViewerCamera(bool Value);

    /// \addtogroup IEnableableComponent
    /// @{
    /// @copydoc IEnableableComponent::GetIsEnabled()
    bool GetIsEnabled() const override;
    /// @copydoc IEnableableComponent::SetIsEnabled()
    void SetIsEnabled(bool InValue) override;
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
