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

#include "CSP/Multiplayer/Components/FiducialMarkerSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/FiducialMarkerSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

FiducialMarkerSpaceComponent::FiducialMarkerSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::FiducialMarker, Parent)
{
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::Name_DEPRECATED)] = "";
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::MarkerAssetId)] = "";
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::AssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsARVisible)] = true;

    SetScriptInterface(CSP_NEW FiducialMarkerSpaceComponentScriptInterface(this));
}

const csp::common::String& FiducialMarkerSpaceComponent::GetMarkerAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::MarkerAssetId));
}

void FiducialMarkerSpaceComponent::SetMarkerAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::MarkerAssetId), Value);
}

const csp::common::String& FiducialMarkerSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::AssetCollectionId));
}

void FiducialMarkerSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::AssetCollectionId), Value);
}

const csp::common::String& FiducialMarkerSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Name_DEPRECATED));
}

void FiducialMarkerSpaceComponent::SetName(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Name_DEPRECATED), Value);
}

/* ITransformComponent */

const csp::common::Vector3& FiducialMarkerSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Position));
}

void FiducialMarkerSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Position), Value);
}

const csp::common::Vector4& FiducialMarkerSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Rotation));
}

void FiducialMarkerSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& FiducialMarkerSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Scale));
}

void FiducialMarkerSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Scale), Value);
}

SpaceTransform FiducialMarkerSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void FiducialMarkerSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

/* IVisibleComponent */

bool FiducialMarkerSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVisible)); }

void FiducialMarkerSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVisible), Value); }

bool FiducialMarkerSpaceComponent::GetIsARVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsARVisible));
}

void FiducialMarkerSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsARVisible), Value); }
} // namespace csp::multiplayer
