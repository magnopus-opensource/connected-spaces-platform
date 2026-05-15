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
#include "CSP/Multiplayer/Components/FogSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Fog),
    "Fog",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::FogMode),
            "fogMode",
            static_cast<int64_t>(FogMode::Exponential),
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Scale),
            "scale",
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::StartDistance),
            "startDistance",
            0.f,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::EndDistance),
            "endDistance",
            0.f,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Color),
            "color",
            csp::common::Vector3 { 0.8f, 0.9f, 1.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Density),
            "density",
            0.4f,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::HeightFalloff),
            "heightFalloff",
            0.2f,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::MaxOpacity),
            "maxOpacity",
            1.f,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsVolumetric),
            "isVolumetric",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::ThirdPartyComponentRef),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& FogSpaceComponent::GetSchema() { return Schema; }

FogSpaceComponent::FogSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

FogMode FogSpaceComponent::GetFogMode() const { return static_cast<FogMode>(GetIntegerProperty(static_cast<uint32_t>(FogPropertyKeys::FogMode))); }

void FogSpaceComponent::SetFogMode(FogMode value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::FogMode), static_cast<int64_t>(value)); }

/* ITransformComponent */

const csp::common::Vector3& FogSpaceComponent::GetPosition() const { return GetVector3Property(static_cast<uint32_t>(FogPropertyKeys::Position)); }

void FogSpaceComponent::SetPosition(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Position), value); }

const csp::common::Vector4& FogSpaceComponent::GetRotation() const { return GetVector4Property(static_cast<uint32_t>(FogPropertyKeys::Rotation)); }

void FogSpaceComponent::SetRotation(const csp::common::Vector4& value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Rotation), value); }

const csp::common::Vector3& FogSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(FogPropertyKeys::Scale)); }

void FogSpaceComponent::SetScale(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Scale), value); }

SpaceTransform FogSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void FogSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

float FogSpaceComponent::GetStartDistance() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::StartDistance)); }

void FogSpaceComponent::SetStartDistance(float value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::StartDistance), value); }

float FogSpaceComponent::GetEndDistance() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::EndDistance)); }

void FogSpaceComponent::SetEndDistance(float value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::EndDistance), value); }

const csp::common::Vector3& FogSpaceComponent::GetColor() const { return GetVector3Property(static_cast<uint32_t>(FogPropertyKeys::Color)); }

void FogSpaceComponent::SetColor(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Color), value); }

float FogSpaceComponent::GetDensity() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::Density)); }

void FogSpaceComponent::SetDensity(float value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Density), value); }

float FogSpaceComponent::GetHeightFalloff() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::HeightFalloff)); }

void FogSpaceComponent::SetHeightFalloff(float value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::HeightFalloff), value); }

float FogSpaceComponent::GetMaxOpacity() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::MaxOpacity)); }

void FogSpaceComponent::SetMaxOpacity(float value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::MaxOpacity), value); }

bool FogSpaceComponent::GetIsVolumetric() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsVolumetric)); }

void FogSpaceComponent::SetIsVolumetric(bool value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsVolumetric), value); }

/* IVisibleComponent */

bool FogSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsVisible)); }

void FogSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsVisible), value); }

bool FogSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsARVisible)); }

void FogSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsARVisible), value); }

bool FogSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsVirtualVisible)); }

void FogSpaceComponent::SetIsVirtualVisible(bool value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsVirtualVisible), value); }

const csp::common::String& FogSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef));
}

void FogSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& inValue)
{
    SetProperty(static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef), inValue);
}

} // namespace csp::multiplayer
