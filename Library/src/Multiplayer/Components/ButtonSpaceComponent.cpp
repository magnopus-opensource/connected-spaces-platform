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
#include "CSP/Multiplayer/Components/ButtonSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Button),
    "Button",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::LabelText),
            "labelText",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IconAssetId),
            "iconAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::AssetCollectionId),
            "assetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::Scale),
            "scale",
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsEnabled),
            "isEnabled",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& ButtonSpaceComponent::GetSchema() { return Schema; }

ButtonSpaceComponent::ButtonSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

const csp::common::String& ButtonSpaceComponent::GetLabelText() const
{
    return GetStringProperty(static_cast<uint32_t>(ButtonPropertyKeys::LabelText));
}

void ButtonSpaceComponent::SetLabelText(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::LabelText), value);
}

const csp::common::String& ButtonSpaceComponent::GetIconAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId));
}

void ButtonSpaceComponent::SetIconAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId), value);
}

const csp::common::String& ButtonSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId));
}

void ButtonSpaceComponent::SetAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId), value);
}

/* ITransformComponent */

const csp::common::Vector3& ButtonSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ButtonPropertyKeys::Position));
}

void ButtonSpaceComponent::SetPosition(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Position), value); }

const csp::common::Vector4& ButtonSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ButtonPropertyKeys::Rotation));
}

void ButtonSpaceComponent::SetRotation(const csp::common::Vector4& value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Rotation), value); }

const csp::common::Vector3& ButtonSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(ButtonPropertyKeys::Scale)); }

void ButtonSpaceComponent::SetScale(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Scale), value); }

SpaceTransform ButtonSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void ButtonSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

/* IClickableComponent */

bool ButtonSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled)); }

void ButtonSpaceComponent::SetIsEnabled(bool value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled), value); }

/* IVisibleComponent */

bool ButtonSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVisible)); }

void ButtonSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVisible), value); }

bool ButtonSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible)); }

void ButtonSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible), value); }

bool ButtonSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVirtualVisible)); }

void ButtonSpaceComponent::SetIsVirtualVisible(bool value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVirtualVisible), value); }

} // namespace csp::multiplayer
