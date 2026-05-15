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

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::FiducialMarker),
    "FiducialMarker",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Name_DEPRECATED),
            "name",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::MarkerAssetId),
            "markerAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::AssetCollectionId),
            "assetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Scale),
            "scale",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& FiducialMarkerSpaceComponent::GetSchema() { return Schema; }

FiducialMarkerSpaceComponent::FiducialMarkerSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

const csp::common::String& FiducialMarkerSpaceComponent::GetMarkerAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::MarkerAssetId));
}

void FiducialMarkerSpaceComponent::SetMarkerAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::MarkerAssetId), value);
}

const csp::common::String& FiducialMarkerSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::AssetCollectionId));
}

void FiducialMarkerSpaceComponent::SetAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::AssetCollectionId), value);
}

const csp::common::String& FiducialMarkerSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Name_DEPRECATED));
}

void FiducialMarkerSpaceComponent::SetName(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Name_DEPRECATED), value);
}

/* ITransformComponent */

const csp::common::Vector3& FiducialMarkerSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Position));
}

void FiducialMarkerSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Position), value);
}

const csp::common::Vector4& FiducialMarkerSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Rotation));
}

void FiducialMarkerSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Rotation), value);
}

const csp::common::Vector3& FiducialMarkerSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Scale));
}

void FiducialMarkerSpaceComponent::SetScale(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::Scale), value);
}

SpaceTransform FiducialMarkerSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void FiducialMarkerSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

/* IVisibleComponent */

bool FiducialMarkerSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVisible)); }

void FiducialMarkerSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVisible), value); }

bool FiducialMarkerSpaceComponent::GetIsARVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsARVisible));
}

void FiducialMarkerSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsARVisible), value); }

bool FiducialMarkerSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVirtualVisible));
}

void FiducialMarkerSpaceComponent::SetIsVirtualVisible(bool value)
{
    SetProperty(static_cast<uint32_t>(FiducialMarkerPropertyKeys::IsVirtualVisible), value);
}

} // namespace csp::multiplayer
