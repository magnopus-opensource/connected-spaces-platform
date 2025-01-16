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
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/LightSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

LightSpaceComponent::LightSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Light, Parent)
{
    Properties[static_cast<uint32_t>(LightPropertyKeys::LightType)] = static_cast<int64_t>(LightType::Point);
    Properties[static_cast<uint32_t>(LightPropertyKeys::Color)] = csp::common::Vector3 { 255, 255, 255 };
    Properties[static_cast<uint32_t>(LightPropertyKeys::Intensity)] = 5000.0f;
    Properties[static_cast<uint32_t>(LightPropertyKeys::Range)] = 1000.0f;
    Properties[static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle)] = 0.0f;
    Properties[static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle)] = 0.78539816339f; // Pi / 4
    Properties[static_cast<uint32_t>(LightPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(LightPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(LightPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId)] = "";
    Properties[static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(LightPropertyKeys::LightCookieType)] = static_cast<int64_t>(LightCookieType::NoCookie);
    Properties[static_cast<uint32_t>(LightPropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint32_t>(LightPropertyKeys::ThirdPartyComponentRef)] = "";
    Properties[static_cast<uint32_t>(LightPropertyKeys::LightShadowType)] = static_cast<int64_t>(LightShadowType::None);

    SetScriptInterface(CSP_NEW LightSpaceComponentScriptInterface(this));
}

LightType LightSpaceComponent::GetLightType() const
{
    return static_cast<LightType>(GetIntegerProperty(static_cast<uint32_t>(LightPropertyKeys::LightType)));
}

void LightSpaceComponent::SetLightType(LightType Value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightType), static_cast<int64_t>(Value));
}

const csp::common::Vector3& LightSpaceComponent::GetColor() const { return GetVector3Property(static_cast<uint32_t>(LightPropertyKeys::Color)); }

void LightSpaceComponent::SetColor(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Color), Value); }

float LightSpaceComponent::GetIntensity() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::Intensity)); }

void LightSpaceComponent::SetIntensity(float Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Intensity), Value); }

float LightSpaceComponent::GetRange() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::Range)); }

void LightSpaceComponent::SetRange(float Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Range), Value); }

float LightSpaceComponent::GetInnerConeAngle() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle)); }

void LightSpaceComponent::SetInnerConeAngle(float Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle), Value); }

float LightSpaceComponent::GetOuterConeAngle() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle)); }

void LightSpaceComponent::SetOuterConeAngle(float Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle), Value); }

/* IPositionComponent */

const csp::common::Vector3& LightSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(LightPropertyKeys::Position));
}

void LightSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Position), Value); }

/* IRotationComponent */

const csp::common::Vector4& LightSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(LightPropertyKeys::Rotation));
}

void LightSpaceComponent::SetRotation(const csp::common::Vector4& Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Rotation), Value); }

/* IVisibleComponent */

bool LightSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(LightPropertyKeys::IsVisible)); }

void LightSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsVisible), Value); }

bool LightSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(LightPropertyKeys::IsARVisible)); }

void LightSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsARVisible), Value); }

const csp::common::String& LightSpaceComponent::GetLightCookieAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId));
}

void LightSpaceComponent::SetLightCookieAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId), Value);
}

const csp::common::String& LightSpaceComponent::GetLightCookieAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId));
}

void LightSpaceComponent::SetLightCookieAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId), Value);
}

LightCookieType LightSpaceComponent::GetLightCookieType() const
{
    return static_cast<LightCookieType>(GetIntegerProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieType)));
}

void LightSpaceComponent::SetLightCookieType(LightCookieType Value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieType), static_cast<int64_t>(Value));
}

const csp::common::String& LightSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(LightPropertyKeys::ThirdPartyComponentRef));
}

void LightSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::ThirdPartyComponentRef), InValue);
}

LightShadowType LightSpaceComponent::GetLightShadowType() const
{
    return static_cast<LightShadowType>(GetIntegerProperty(static_cast<uint32_t>(LightPropertyKeys::LightShadowType)));
}

void LightSpaceComponent::SetLightShadowType(LightShadowType Value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightShadowType), static_cast<int64_t>(Value));
}

} // namespace csp::multiplayer
