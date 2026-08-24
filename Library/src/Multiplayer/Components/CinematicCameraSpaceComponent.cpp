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

#include "CSP/Multiplayer/Components/CinematicCameraSpaceComponent.h"
#include "Multiplayer/ComponentSchema.h"

#include "Multiplayer/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h"

#include <cmath>

namespace csp::multiplayer
{

using namespace schema;

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::CinematicCamera),
    "CinematicCamera",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Position),
            "position",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::Zero() },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Rotation),
            "rotation",
            PlainValue<csp::common::Vector4> { csp::common::Vector4::Identity() },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::IsEnabled),
            "isEnabled",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::FocalLength),
            "focalLength",
            PlainValue<float> { 0.035f },
        },
        // 16:9
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::AspectRatio),
            "aspectRatio",
            PlainValue<float> { 1.778f },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::SensorSize),
            "sensorSize",
            PlainValue<csp::common::Vector2> { csp::common::Vector2 { 0.036f, 0.024f } },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::NearClip),
            "nearClip",
            PlainValue<float> { 0.1f },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::FarClip),
            "farClip",
            PlainValue<float> { 20000.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Iso),
            "iso",
            PlainValue<float> { 400.0f },
        },
        // 60 FPS
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::ShutterSpeed),
            "shutterSpeed",
            PlainValue<float> { 0.0167f },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Aperture),
            "aperture",
            PlainValue<float> { 4.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::IsViewerCamera),
            "isViewerCamera",
            PlainValue<bool> { false },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::ThirdPartyComponentRef),
            "thirdPartyComponentRef",
            PlainValue<std::string> { "" },
            /*.IsScriptable =*/false,
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::FocusDistance),
            "focusDistance",
            PlainValue<float> { 5.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::DepthOfFieldEnabled),
            "depthOfFieldEnabled",
            PlainValue<bool> { false },
        },
    },
};

const ComponentSchema& CinematicCameraSpaceComponent::GetSchema() { return Schema; }

CinematicCameraSpaceComponent::CinematicCameraSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : CinematicCameraSpaceComponent(Schema, LogSystem, Parent)
{
}

std::unique_ptr<CinematicCameraSpaceComponent> CinematicCameraSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(CinematicCameraSpaceComponent::GetSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<CinematicCameraSpaceComponent>(new CinematicCameraSpaceComponent(InSchema, LogSystem, Parent));
}

CinematicCameraSpaceComponent::CinematicCameraSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
    SetScriptInterface(new CinematicCameraSpaceComponentScriptInterface(this));
}

float CinematicCameraSpaceComponent::GetFov() const
{
    float sensorAspectRatio = GetSensorSize().X / GetSensorSize().Y;
    float aspectRatio = GetAspectRatio();
    // When the crop changes the width, we need to update the fov to match
    float sensorCropFactor = aspectRatio < sensorAspectRatio ? aspectRatio / sensorAspectRatio : 1.0f;
    // Horizontal FOV in radians
    return 2.0f * atan((GetSensorSize().X * sensorCropFactor) / (2.0f * GetFocalLength()));
}

// transforms
const csp::common::Vector3& CinematicCameraSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(CinematicCameraPropertyKeys::Position));
}

void CinematicCameraSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::Position), Value);
}

const csp::common::Vector4& CinematicCameraSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(CinematicCameraPropertyKeys::Rotation));
}

void CinematicCameraSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::Rotation), Value);
}

// Focal Length
float CinematicCameraSpaceComponent::GetFocalLength() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FocalLength));
}

void CinematicCameraSpaceComponent::SetFocalLength(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::FocalLength), Value);
}

// Aspect Ratio
float CinematicCameraSpaceComponent::GetAspectRatio() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::AspectRatio));
}

void CinematicCameraSpaceComponent::SetAspectRatio(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::AspectRatio), Value);
}

// Sensor Size
const csp::common::Vector2& CinematicCameraSpaceComponent::GetSensorSize() const
{
    return GetVector2Property(static_cast<uint32_t>(CinematicCameraPropertyKeys::SensorSize));
}

void CinematicCameraSpaceComponent::SetSensorSize(const csp::common::Vector2& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::SensorSize), Value);
}

// Near Clip
float CinematicCameraSpaceComponent::GetNearClip() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::NearClip)); }

void CinematicCameraSpaceComponent::SetNearClip(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::NearClip), Value);
}

// Far Clip
float CinematicCameraSpaceComponent::GetFarClip() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FarClip)); }

void CinematicCameraSpaceComponent::SetFarClip(float Value) { SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::FarClip), Value); }

// ISO
float CinematicCameraSpaceComponent::GetIso() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Iso)); }

void CinematicCameraSpaceComponent::SetIso(float Value) { SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::Iso), Value); }

// Shutter Speed
float CinematicCameraSpaceComponent::GetShutterSpeed() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::ShutterSpeed));
}

void CinematicCameraSpaceComponent::SetShutterSpeed(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::ShutterSpeed), Value);
}

// Aperture
float CinematicCameraSpaceComponent::GetAperture() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Aperture)); }

void CinematicCameraSpaceComponent::SetAperture(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::Aperture), Value);
}

// Focus Distance
float CinematicCameraSpaceComponent::GetFocusDistance() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FocusDistance));
}

void CinematicCameraSpaceComponent::SetFocusDistance(float Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::FocusDistance), Value);
}

// Depth of Field Enabled
bool CinematicCameraSpaceComponent::GetDepthOfFieldEnabled() const
{
    return GetBooleanProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::DepthOfFieldEnabled));
}

void CinematicCameraSpaceComponent::SetDepthOfFieldEnabled(bool Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::DepthOfFieldEnabled), Value);
}

// Is Viewer Camera
bool CinematicCameraSpaceComponent::GetIsViewerCamera() const
{
    return GetBooleanProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsViewerCamera));
}

void CinematicCameraSpaceComponent::SetIsViewerCamera(bool Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsViewerCamera), Value);
}

// Is Enabled
bool CinematicCameraSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsEnabled)); }

void CinematicCameraSpaceComponent::SetIsEnabled(bool Value)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsEnabled), Value);
}

// Third Party Component
const csp::common::String& CinematicCameraSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::ThirdPartyComponentRef));
}

void CinematicCameraSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetPropertyDirect(static_cast<uint32_t>(CinematicCameraPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
