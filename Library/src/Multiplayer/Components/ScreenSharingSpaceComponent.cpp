/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Multiplayer/Components/ScreenSharingSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"
#include "CSP/Multiplayer/SpaceEntity.h"

namespace
{
constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
}

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::ScreenSharing),
    "ScreenSharing",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4::Identity(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Scale),
            "scale",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsShadowCaster),
            "isShadowCaster",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::UserId),
            "userId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::DefaultImageCollectionId),
            "defaultImageCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::DefaultImageAssetId),
            "defaultImageAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::AttenuationRadius),
            "attenuationRadius",
            DefaultAttenuationRadius,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& ScreenSharingSpaceComponent::GetSchema() { return Schema; }

ScreenSharingSpaceComponent::ScreenSharingSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

const csp::common::String& ScreenSharingSpaceComponent::GetUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::UserId));
}

void ScreenSharingSpaceComponent::SetUserId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::UserId), value);
}

const csp::common::String& ScreenSharingSpaceComponent::GetDefaultImageCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageCollectionId));
}

void ScreenSharingSpaceComponent::SetDefaultImageCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageCollectionId), value);
}

const csp::common::String& ScreenSharingSpaceComponent::GetDefaultImageAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageAssetId));
}

void ScreenSharingSpaceComponent::SetDefaultImageAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageAssetId), value);
}

float ScreenSharingSpaceComponent::GetAttenuationRadius() const
{
    return GetFloatProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::AttenuationRadius));
}

void ScreenSharingSpaceComponent::SetAttenuationRadius(float value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::AttenuationRadius), value);
}

/* ITransformComponent */

const csp::common::Vector3& ScreenSharingSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ScreenSharingPropertyKeys::Position));
}

void ScreenSharingSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::Position), value);
}

const csp::common::Vector4& ScreenSharingSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ScreenSharingPropertyKeys::Rotation));
}

void ScreenSharingSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::Rotation), value);
}

const csp::common::Vector3& ScreenSharingSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(ScreenSharingPropertyKeys::Scale));
}

void ScreenSharingSpaceComponent::SetScale(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::Scale), value);
}

SpaceTransform ScreenSharingSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void ScreenSharingSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

/* IVisibleComponent */

bool ScreenSharingSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVisible)); }

void ScreenSharingSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVisible), value); }

bool ScreenSharingSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsARVisible)); }

void ScreenSharingSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsARVisible), value); }

bool ScreenSharingSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVirtualVisible));
}

void ScreenSharingSpaceComponent::SetIsVirtualVisible(bool value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVirtualVisible), value);
}

/* IShadowCaster */

bool ScreenSharingSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsShadowCaster));
}

void ScreenSharingSpaceComponent::SetIsShadowCaster(bool value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsShadowCaster), value);
}

} // namespace csp::multiplayer
