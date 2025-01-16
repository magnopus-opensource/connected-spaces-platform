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

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/FogSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

FogSpaceComponent::FogSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Fog, Parent)
{
    Properties[static_cast<uint32_t>(FogPropertyKeys::FogMode)] = static_cast<int64_t>(FogMode::Linear);
    Properties[static_cast<uint32_t>(FogPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(FogPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(FogPropertyKeys::Scale)] = csp::common::Vector3 { 1, 1, 1 };
    Properties[static_cast<uint32_t>(FogPropertyKeys::StartDistance)] = 0.f;
    Properties[static_cast<uint32_t>(FogPropertyKeys::EndDistance)] = 0.f;
    Properties[static_cast<uint32_t>(FogPropertyKeys::Color)] = csp::common::Vector3 { 0.8f, 0.9f, 1.0f };
    Properties[static_cast<uint32_t>(FogPropertyKeys::Density)] = 0.2f;
    Properties[static_cast<uint32_t>(FogPropertyKeys::HeightFalloff)] = 0.2f;
    Properties[static_cast<uint32_t>(FogPropertyKeys::MaxOpacity)] = 1.f;
    Properties[static_cast<uint32_t>(FogPropertyKeys::IsVolumetric)] = false;
    Properties[static_cast<uint32_t>(FogPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(FogPropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef)] = "";

    SetScriptInterface(CSP_NEW FogSpaceComponentScriptInterface(this));
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

const csp::common::String& FogSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef));
}

void FogSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(FogPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
