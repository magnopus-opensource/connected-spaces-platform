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
#include "Multiplayer/Script/ComponentBinding/ScreenSharingSpaceComponentScriptInterface.h"

namespace
{
constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
}

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::ScreenSharing),
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Position),
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Rotation),
            csp::common::Vector4::Identity(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Scale),
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsVisible),
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsARVisible),
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsShadowCaster),
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::UserId),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::DefaultImageCollectionId),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::DefaultImageAssetId),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::AttenuationRadius),
            DefaultAttenuationRadius,
        },
        {
            static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsVirtualVisible),
            true,
        },
    },
};

ScreenSharingSpaceComponent::ScreenSharingSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new ScreenSharingSpaceComponentScriptInterface(this));
}

const csp::common::String& ScreenSharingSpaceComponent::GetUserId() const
{
    return GetStringProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::UserId));
}

void ScreenSharingSpaceComponent::SetUserId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::UserId), Value);
}

const csp::common::String& ScreenSharingSpaceComponent::GetDefaultImageCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageCollectionId));
}

void ScreenSharingSpaceComponent::SetDefaultImageCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageCollectionId), Value);
}

const csp::common::String& ScreenSharingSpaceComponent::GetDefaultImageAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageAssetId));
}

void ScreenSharingSpaceComponent::SetDefaultImageAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::DefaultImageAssetId), Value);
}

float ScreenSharingSpaceComponent::GetAttenuationRadius() const
{
    return GetFloatProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::AttenuationRadius));
}

void ScreenSharingSpaceComponent::SetAttenuationRadius(float Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::AttenuationRadius), Value);
}

/* ITransformComponent */

const csp::common::Vector3& ScreenSharingSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ScreenSharingPropertyKeys::Position));
}

void ScreenSharingSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::Position), Value);
}

const csp::common::Vector4& ScreenSharingSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ScreenSharingPropertyKeys::Rotation));
}

void ScreenSharingSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& ScreenSharingSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(ScreenSharingPropertyKeys::Scale));
}

void ScreenSharingSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::Scale), Value);
}

SpaceTransform ScreenSharingSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void ScreenSharingSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

/* IVisibleComponent */

bool ScreenSharingSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVisible)); }

void ScreenSharingSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVisible), Value); }

bool ScreenSharingSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsARVisible)); }

void ScreenSharingSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsARVisible), Value); }

bool ScreenSharingSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVirtualVisible));
}

void ScreenSharingSpaceComponent::SetIsVirtualVisible(bool Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsVirtualVisible), Value);
}

/* IShadowCaster */

bool ScreenSharingSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsShadowCaster));
}

void ScreenSharingSpaceComponent::SetIsShadowCaster(bool Value)
{
    SetProperty(static_cast<uint32_t>(ScreenSharingPropertyKeys::IsShadowCaster), Value);
}

} // namespace csp::multiplayer
