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
#include "CSP/Multiplayer/Components/ExternalLinkSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::ExternalLink),
    "ExternalLink",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Name_DEPRECATED),
            "name",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::LinkUrl),
            "linkUrl",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Scale),
            "scale",
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::DisplayText),
            "displayText",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsEnabled),
            "isEnabled",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& ExternalLinkSpaceComponent::GetSchema() { return Schema; }

ExternalLinkSpaceComponent::ExternalLinkSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

const csp::common::String& ExternalLinkSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Name_DEPRECATED));
}

void ExternalLinkSpaceComponent::SetName(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Name_DEPRECATED), value);
}

const csp::common::String& ExternalLinkSpaceComponent::GetLinkUrl() const
{
    return GetStringProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::LinkUrl));
}

void ExternalLinkSpaceComponent::SetLinkUrl(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::LinkUrl), value);
}

/* ITransformComponent */

const csp::common::Vector3& ExternalLinkSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ExternalLinkPropertyKeys::Position));
}

void ExternalLinkSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Position), value);
}

const csp::common::Vector4& ExternalLinkSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ExternalLinkPropertyKeys::Rotation));
}

void ExternalLinkSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Rotation), value);
}

const csp::common::Vector3& ExternalLinkSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(ExternalLinkPropertyKeys::Scale));
}

void ExternalLinkSpaceComponent::SetScale(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Scale), value);
}

SpaceTransform ExternalLinkSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void ExternalLinkSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

const csp::common::String& ExternalLinkSpaceComponent::GetDisplayText() const
{
    return GetStringProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::DisplayText));
}

void ExternalLinkSpaceComponent::SetDisplayText(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::DisplayText), value);
}

/* IClickableComponent */

bool ExternalLinkSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsEnabled)); }

void ExternalLinkSpaceComponent::SetIsEnabled(bool value) { SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsEnabled), value); }

bool ExternalLinkSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVisible)); }

void ExternalLinkSpaceComponent::SetIsVisible(bool inValue) { SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVisible), inValue); }

bool ExternalLinkSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsARVisible)); }

void ExternalLinkSpaceComponent::SetIsARVisible(bool inValue) { SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsARVisible), inValue); }

bool ExternalLinkSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVirtualVisible));
}

void ExternalLinkSpaceComponent::SetIsVirtualVisible(bool inValue)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVirtualVisible), inValue);
}

} // namespace csp::multiplayer
