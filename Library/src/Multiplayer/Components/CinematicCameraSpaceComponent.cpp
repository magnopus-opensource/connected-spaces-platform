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

#include "Debug/Logging.h"
#include "Multiplayer/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

CinematicCameraSpaceComponent::CinematicCameraSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::CinematicCamera, Parent)
{
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::Rotation)] = csp::common::Vector4::Identity();
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::IsEnabled)] = true;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::FocalLength)] = 0.035f;
    // 16:9
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::AspectRatio)] = 1.778f;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::SensorSize)] = csp::common::Vector2 { 0.036f, 0.024f };
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::NearClip)] = 0.1f;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::FarClip)] = 20000.0f;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::Iso)] = 400.0f;
    // 60 FPS
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::ShutterSpeed)] = 0.0167f;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::Aperture)] = 4.0f;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::IsViewerCamera)] = false;
    Properties[static_cast<uint32_t>(CinematicCameraPropertyKeys::ThirdPartyComponentRef)] = "";

    SetScriptInterface(CSP_NEW CinematicCameraSpaceComponentScriptInterface(this));
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
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Position), Value);
}

const csp::common::Vector4& CinematicCameraSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(CinematicCameraPropertyKeys::Rotation));
}

void CinematicCameraSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Rotation), Value);
}

// Focal Length
float CinematicCameraSpaceComponent::GetFocalLength() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FocalLength));
}

void CinematicCameraSpaceComponent::SetFocalLength(float Value)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FocalLength), Value);
}

// Aspect Ratio
float CinematicCameraSpaceComponent::GetAspectRatio() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::AspectRatio));
}

void CinematicCameraSpaceComponent::SetAspectRatio(float Value)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::AspectRatio), Value);
}

// Sensor Size
const csp::common::Vector2& CinematicCameraSpaceComponent::GetSensorSize() const
{
    return GetVector2Property(static_cast<uint32_t>(CinematicCameraPropertyKeys::SensorSize));
}

void CinematicCameraSpaceComponent::SetSensorSize(const csp::common::Vector2& Value)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::SensorSize), Value);
}

// Near Clip
float CinematicCameraSpaceComponent::GetNearClip() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::NearClip)); }

void CinematicCameraSpaceComponent::SetNearClip(float Value) { SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::NearClip), Value); }

// Far Clip
float CinematicCameraSpaceComponent::GetFarClip() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FarClip)); }

void CinematicCameraSpaceComponent::SetFarClip(float Value) { SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::FarClip), Value); }

// ISO
float CinematicCameraSpaceComponent::GetIso() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Iso)); }

void CinematicCameraSpaceComponent::SetIso(float Value) { SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Iso), Value); }

// Shutter Speed
float CinematicCameraSpaceComponent::GetShutterSpeed() const
{
    return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::ShutterSpeed));
}

void CinematicCameraSpaceComponent::SetShutterSpeed(float Value)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::ShutterSpeed), Value);
}

// Aperture
float CinematicCameraSpaceComponent::GetAperture() const { return GetFloatProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Aperture)); }

void CinematicCameraSpaceComponent::SetAperture(float Value) { SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::Aperture), Value); }

// Is Viewer Camera
bool CinematicCameraSpaceComponent::GetIsViewerCamera() const
{
    return GetBooleanProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsViewerCamera));
}

void CinematicCameraSpaceComponent::SetIsViewerCamera(bool Value)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsViewerCamera), Value);
}

// Is Enabled
bool CinematicCameraSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsEnabled)); }

void CinematicCameraSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::IsEnabled), Value); }

// Third Party Component
const csp::common::String& CinematicCameraSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::ThirdPartyComponentRef));
}

void CinematicCameraSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(CinematicCameraPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
