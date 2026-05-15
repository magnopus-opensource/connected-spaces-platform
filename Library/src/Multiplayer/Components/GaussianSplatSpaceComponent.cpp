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

#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::GaussianSplat),
    "GaussianSplat",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::ExternalResourceAssetId),
            "externalResourceAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId),
            "externalResourceAssetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4::Identity(),
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Scale),
            "scale",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsShadowCaster_DEPRECATED),
            {}, // not exposed to scripting: this is a deprecated property that was never exposed to scripting, so no need to start now.
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Tint),
            "tint",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& GaussianSplatSpaceComponent::GetSchema() { return Schema; }

GaussianSplatSpaceComponent::GaussianSplatSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

/* IExternalResourceComponent */

const csp::common::String& GaussianSplatSpaceComponent::GetExternalResourceAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetId));
}

void GaussianSplatSpaceComponent::SetExternalResourceAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetId), value);
}

const csp::common::String& GaussianSplatSpaceComponent::GetExternalResourceAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId));
}

void GaussianSplatSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId), value);
}

/* ITransformComponent */

const csp::common::Vector3& GaussianSplatSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(GaussianSplatPropertyKeys::Position));
}

void GaussianSplatSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Position), value);
}

const csp::common::Vector4& GaussianSplatSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(GaussianSplatPropertyKeys::Rotation));
}

void GaussianSplatSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Rotation), value);
}

const csp::common::Vector3& GaussianSplatSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(GaussianSplatPropertyKeys::Scale));
}

void GaussianSplatSpaceComponent::SetScale(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Scale), value);
}

SpaceTransform GaussianSplatSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void GaussianSplatSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

/* IVisibleComponent */

bool GaussianSplatSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVisible)); }

void GaussianSplatSpaceComponent::SetIsVisible(bool inValue) { SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVisible), inValue); }

bool GaussianSplatSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsARVisible)); }

void GaussianSplatSpaceComponent::SetIsARVisible(bool inValue)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsARVisible), inValue);
}

bool GaussianSplatSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVirtualVisible));
}

void GaussianSplatSpaceComponent::SetIsVirtualVisible(bool inValue)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVirtualVisible), inValue);
}

bool GaussianSplatSpaceComponent::GetIsShadowCaster() const
{
    return GetBooleanProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsShadowCaster_DEPRECATED));
}

void GaussianSplatSpaceComponent::SetIsShadowCaster(bool value)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsShadowCaster_DEPRECATED), value);
}

const csp::common::Vector3& GaussianSplatSpaceComponent::GetTint() const
{
    return GetVector3Property(static_cast<uint32_t>(GaussianSplatPropertyKeys::Tint));
}

void GaussianSplatSpaceComponent::SetTint(const csp::common::Vector3& tintValue)
{
    SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Tint), tintValue);
}

} // namespace csp::multiplayer
