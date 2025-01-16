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

/// @file LightSpaceComponent.h
/// @brief Definitions and support for lights.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IRotationComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the types of light supported by the light component.
enum class LightType
{
    Directional = 0,
    Point,
    Spot,
    Num
};

/// @brief Enumerates the types of light shadows supported by the light component.
enum class LightShadowType
{
    None = 0,
    Static,
    Realtime,
    Num
};

/// @brief Enumerates the types of cookie supported by the light component.
enum class LightCookieType
{
    ImageCookie = 0,
    VideoCookie,
    NoCookie
};

/// @brief Enumerates the list of properties that can be replicated for a light component.
enum class LightPropertyKeys
{
    Name_DEPRECATED = 0,
    LightType,
    Color,
    Intensity,
    Range,
    InnerConeAngle,
    OuterConeAngle,
    Position,
    Rotation,
    IsVisible,
    LightCookieAssetId,
    LightCookieAssetCollectionId,
    LightCookieType,
    IsARVisible,
    ThirdPartyComponentRef,
    LightShadowType,
    Num
};

/// @ingroup LightSpaceComponent
/// @brief Adds various types of lighting to a SpaceEntity, such as directional, point, or spotlights.
///
/// This component is essential for creating realistic lighting effects and controlling how objects are illuminated within the space.
class CSP_API LightSpaceComponent : public ComponentBase,
                                    public IPositionComponent,
                                    public IRotationComponent,
                                    public IThirdPartyComponentRef,
                                    public IVisibleComponent
{
public:
    /// @brief Constructs the light space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    LightSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the type of light of this light component.
    /// @return The type of light of this light component.
    LightType GetLightType() const;

    /// @brief Sets the type of light of this light component.
    /// @param Value The type of light of this light component.
    void SetLightType(LightType Value);

    /// @brief Gets the type of light shadow of this light component.
    /// @return The type of light shadow of this light component.
    LightShadowType GetLightShadowType() const;

    /// @brief Sets the type of light shadow of this light component.
    /// @param Value The type of light shadow of this light component.
    void SetLightShadowType(LightShadowType Value);

    /// @brief Gets the color of the light of this component.
    /// @return The color of the light of this component.
    const csp::common::Vector3& GetColor() const;

    /// @brief Sets the color of the light of this component.
    /// @param Value The color of the light of this component.
    void SetColor(const csp::common::Vector3& Value);

    /// @brief Gets the intensity of the light of this component.
    /// @return The intensity of the light of this component.
    float GetIntensity() const;

    /// @brief Sets the intensity of the light of this component.
    /// @param Value The intensity of the light of this component.
    void SetIntensity(float Value);

    /// @brief Gets the range within which the light of this component affects the surrounding 3D scene.
    /// @return The range within which the light of this component affects the surrounding 3D scene.
    float GetRange() const;

    /// @brief Sets the range within which the light of this component affects the surrounding 3D scene.
    /// @param Value The range within which the light of this component affects the surrounding 3D scene.
    void SetRange(float Value);

    /// @brief Gets the angle of the inner cone in a spotlight.
    /// @return The angle of the inner cone in a spotlight.
    float GetInnerConeAngle() const;

    /// @brief Sets the angle of the inner cone in a spotlight.
    /// @param Value The angle of the inner cone in a spotlight.
    void SetInnerConeAngle(float Value);

    /// @brief Gets the angle of the outer cone in a spotlight.
    /// @return The angle of the outer cone in a spotlight.
    float GetOuterConeAngle() const;

    /// @brief Sets the angle of the outer cone in a spotlight.
    /// @param Value The angle of the outer cone in a spotlight.
    void SetOuterConeAngle(float Value);

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

    /// @brief Gets the ID of the asset used for the light cookie of this light component.
    /// @return The ID of the asset used for the light cookie of this light component.
    const csp::common::String& GetLightCookieAssetId() const;

    /// @brief Sets the ID of the asset used for the light cookie of this light component.
    /// @param Value The ID of the asset used for the light cookie of this light component.
    void SetLightCookieAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection used for the light cookie of this light component.
    /// @return The ID of the asset collection used for the light cookie of this light component.
    const csp::common::String& GetLightCookieAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection used for the light cookie of this light component.
    /// @param Value The ID of the asset collection used for the light cookie of this light component.
    void SetLightCookieAssetCollectionId(const csp::common::String& Value);

    /// @brief Gets the type of the light cookie used by this light component.
    /// @return The type of the light cookie used by this light component.
    LightCookieType GetLightCookieType() const;

    /// @brief Sets the type of the light cookie used by this light component.
    /// @param Value The type of the light cookie used by this light component.
    void SetLightCookieType(LightCookieType Value);

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
