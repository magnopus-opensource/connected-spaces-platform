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

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Light),
    "Light",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightType),
            "lightType",
            static_cast<int64_t>(LightType::Point),
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Color),
            "color",
            csp::common::Vector3 { 255, 255, 255 },
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Intensity),
            "Intensity", // Note: exposed as PascalCase for backwards compatibility (casing was wrong when this property was originally exposed)
            5000.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Range),
            "range",
            1000.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::InnerConeAngle),
            "innerConeAngle",
            0.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::OuterConeAngle),
            "outerConeAngle",
            0.78539816339f, // Pi / 4
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightCookieAssetId),
            "cookieAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightCookieAssetCollectionId),
            "cookieAssetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightCookieType),
            "lightCookieType",
            static_cast<int64_t>(LightCookieType::NoCookie),
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::ThirdPartyComponentRef),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightShadowType),
            {}, // not exposed to scripting
            static_cast<int64_t>(LightShadowType::None),
        },
        {
            static_cast<ComponentProperty::KeyType>(LightPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& LightSpaceComponent::GetSchema() { return Schema; }

LightSpaceComponent::LightSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

LightType LightSpaceComponent::GetLightType() const
{
    return static_cast<LightType>(GetIntegerProperty(static_cast<uint32_t>(LightPropertyKeys::LightType)));
}

void LightSpaceComponent::SetLightType(LightType value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightType), static_cast<int64_t>(value));
}

const csp::common::Vector3& LightSpaceComponent::GetColor() const { return GetVector3Property(static_cast<uint32_t>(LightPropertyKeys::Color)); }

void LightSpaceComponent::SetColor(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Color), value); }

float LightSpaceComponent::GetIntensity() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::Intensity)); }

void LightSpaceComponent::SetIntensity(float value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Intensity), value); }

float LightSpaceComponent::GetRange() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::Range)); }

void LightSpaceComponent::SetRange(float value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Range), value); }

float LightSpaceComponent::GetInnerConeAngle() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle)); }

void LightSpaceComponent::SetInnerConeAngle(float value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle), value); }

float LightSpaceComponent::GetOuterConeAngle() const { return GetFloatProperty(static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle)); }

void LightSpaceComponent::SetOuterConeAngle(float value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle), value); }

/* IPositionComponent */

const csp::common::Vector3& LightSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(LightPropertyKeys::Position));
}

void LightSpaceComponent::SetPosition(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Position), value); }

/* IRotationComponent */

const csp::common::Vector4& LightSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(LightPropertyKeys::Rotation));
}

void LightSpaceComponent::SetRotation(const csp::common::Vector4& value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::Rotation), value); }

/* IVisibleComponent */

bool LightSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(LightPropertyKeys::IsVisible)); }

void LightSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsVisible), value); }

bool LightSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(LightPropertyKeys::IsARVisible)); }

void LightSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsARVisible), value); }

bool LightSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(LightPropertyKeys::IsVirtualVisible)); }

void LightSpaceComponent::SetIsVirtualVisible(bool value) { SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsVirtualVisible), value); }

const csp::common::String& LightSpaceComponent::GetLightCookieAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId));
}

void LightSpaceComponent::SetLightCookieAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId), value);
}

const csp::common::String& LightSpaceComponent::GetLightCookieAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId));
}

void LightSpaceComponent::SetLightCookieAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId), value);
}

LightCookieType LightSpaceComponent::GetLightCookieType() const
{
    return static_cast<LightCookieType>(GetIntegerProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieType)));
}

void LightSpaceComponent::SetLightCookieType(LightCookieType value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieType), static_cast<int64_t>(value));
}

const csp::common::String& LightSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(LightPropertyKeys::ThirdPartyComponentRef));
}

void LightSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& inValue)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::ThirdPartyComponentRef), inValue);
}

LightShadowType LightSpaceComponent::GetLightShadowType() const
{
    return static_cast<LightShadowType>(GetIntegerProperty(static_cast<uint32_t>(LightPropertyKeys::LightShadowType)));
}

void LightSpaceComponent::SetLightShadowType(LightShadowType value)
{
    SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightShadowType), static_cast<int64_t>(value));
}

} // namespace csp::multiplayer
