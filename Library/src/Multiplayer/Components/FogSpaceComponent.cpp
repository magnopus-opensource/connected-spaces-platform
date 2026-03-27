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

#include "Multiplayer/Component/Schema.h"
#include "Multiplayer/Script/ComponentBinding/FogSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

const auto Schema = ComponentBase::ComponentSchema {
    ComponentType::Fog,
    std::vector<ComponentBase::ComponentSchema::Property> {
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::FogMode),
            static_cast<int64_t>(FogMode::Exponential),
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::Position),
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::Rotation),
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::Scale),
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::StartDistance),
            0.f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::EndDistance),
            0.f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::Color),
            csp::common::Vector3 { 0.8f, 0.9f, 1.0f },
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::Density),
            0.4f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::HeightFalloff),
            0.2f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::MaxOpacity),
            1.f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::IsVolumetric),
            false,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::IsVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::IsARVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::ThirdPartyComponentRef),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(FogPropertyKeys::IsVirtualVisible),
            true,
        },
    },
};

FogSpaceComponent::FogSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new FogSpaceComponentScriptInterface(this));
}

FogMode FogSpaceComponent::GetFogMode() const { return static_cast<FogMode>(GetIntegerProperty(static_cast<uint32_t>(FogPropertyKeys::FogMode))); }

void FogSpaceComponent::SetFogMode(FogMode Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::FogMode), static_cast<int64_t>(Value)); }

/* ITransformComponent */

const csp::common::Vector3& FogSpaceComponent::GetPosition() const { return GetVector3Property(static_cast<uint32_t>(FogPropertyKeys::Position)); }

void FogSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Position), Value); }

const csp::common::Vector4& FogSpaceComponent::GetRotation() const { return GetVector4Property(static_cast<uint32_t>(FogPropertyKeys::Rotation)); }

void FogSpaceComponent::SetRotation(const csp::common::Vector4& Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Rotation), Value); }

const csp::common::Vector3& FogSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(FogPropertyKeys::Scale)); }

void FogSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Scale), Value); }

SpaceTransform FogSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void FogSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

float FogSpaceComponent::GetStartDistance() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::StartDistance)); }

void FogSpaceComponent::SetStartDistance(float Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::StartDistance), Value); }

float FogSpaceComponent::GetEndDistance() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::EndDistance)); }

void FogSpaceComponent::SetEndDistance(float Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::EndDistance), Value); }

const csp::common::Vector3& FogSpaceComponent::GetColor() const { return GetVector3Property(static_cast<uint32_t>(FogPropertyKeys::Color)); }

void FogSpaceComponent::SetColor(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Color), Value); }

float FogSpaceComponent::GetDensity() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::Density)); }

void FogSpaceComponent::SetDensity(float Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::Density), Value); }

float FogSpaceComponent::GetHeightFalloff() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::HeightFalloff)); }

void FogSpaceComponent::SetHeightFalloff(float Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::HeightFalloff), Value); }

float FogSpaceComponent::GetMaxOpacity() const { return GetFloatProperty(static_cast<uint32_t>(FogPropertyKeys::MaxOpacity)); }

void FogSpaceComponent::SetMaxOpacity(float Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::MaxOpacity), Value); }

bool FogSpaceComponent::GetIsVolumetric() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsVolumetric)); }

void FogSpaceComponent::SetIsVolumetric(bool Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsVolumetric), Value); }

/* IVisibleComponent */

bool FogSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsVisible)); }

void FogSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsVisible), Value); }

bool FogSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsARVisible)); }

void FogSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsARVisible), Value); }

bool FogSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FogPropertyKeys::IsVirtualVisible)); }

void FogSpaceComponent::SetIsVirtualVisible(bool Value) { SetProperty(static_cast<uint32_t>(FogPropertyKeys::IsVirtualVisible), Value); }

const csp::common::String& FogSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef));
}

void FogSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
