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
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Text),
    "Text",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Scale),
            "scale",
            csp::common::Vector3::One(),
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Text),
            "text",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::TextColor),
            "textColor",
            csp::common::Vector3(1.0f, 1.0f, 1.0f),
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::BackgroundColor),
            "backgroundColor",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsBackgroundVisible),
            "isBackgroundVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Width),
            "width",
            1.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Height),
            "height",
            1.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::BillboardMode),
            "billboardMode",
            static_cast<int64_t>(BillboardMode::Off),
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& TextSpaceComponent::GetSchema() { return Schema; }

TextSpaceComponent::TextSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

/* ITransformComponent */

const csp::common::String& TextSpaceComponent::GetText() const { return GetStringProperty(static_cast<uint32_t>(TextPropertyKeys::Text)); }

void TextSpaceComponent::SetText(const csp::common::String& value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Text), value); }

const csp::common::Vector3& TextSpaceComponent::GetTextColor() const
{
    return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::TextColor));
}

void TextSpaceComponent::SetTextColor(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::TextColor), value); }

const csp::common::Vector3& TextSpaceComponent::GetBackgroundColor() const
{
    return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor));
}

void TextSpaceComponent::SetBackgroundColor(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor), value);
}

bool TextSpaceComponent::GetIsBackgroundVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible)); }

void TextSpaceComponent::SetIsBackgroundVisible(bool inValue) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible), inValue); }

float TextSpaceComponent::GetWidth() const { return GetFloatProperty(static_cast<uint32_t>(TextPropertyKeys::Width)); }

void TextSpaceComponent::SetWidth(float inValue) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Width), inValue); }

float TextSpaceComponent::GetHeight() const { return GetFloatProperty(static_cast<uint32_t>(TextPropertyKeys::Height)); }

void TextSpaceComponent::SetHeight(float inValue) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Height), inValue); }

const csp::common::Vector3& TextSpaceComponent::GetPosition() const { return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::Position)); }

void TextSpaceComponent::SetPosition(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Position), value); }

const csp::common::Vector4& TextSpaceComponent::GetRotation() const { return GetVector4Property(static_cast<uint32_t>(TextPropertyKeys::Rotation)); }

void TextSpaceComponent::SetRotation(const csp::common::Vector4& value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Rotation), value); }

const csp::common::Vector3& TextSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::Scale)); }

void TextSpaceComponent::SetScale(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Scale), value); }

SpaceTransform TextSpaceComponent::GetTransform() const
{
    SpaceTransform transform;
    transform.Position = GetPosition();
    transform.Rotation = GetRotation();
    transform.Scale = GetScale();

    return transform;
}

void TextSpaceComponent::SetTransform(const SpaceTransform& inValue)
{
    SetPosition(inValue.Position);
    SetRotation(inValue.Rotation);
    SetScale(inValue.Scale);
}

/* IVisibleComponent */

bool TextSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsVisible)); }

void TextSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsVisible), value); }

bool TextSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible)); }

void TextSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible), value); }

bool TextSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsVirtualVisible)); }

void TextSpaceComponent::SetIsVirtualVisible(bool value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsVirtualVisible), value); }

BillboardMode TextSpaceComponent::GetBillboardMode() const
{
    return static_cast<BillboardMode>(GetIntegerProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode)));
}

void TextSpaceComponent::SetBillboardMode(BillboardMode value)
{
    SetProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode), static_cast<int64_t>(value));
}
} // namespace csp::multiplayer
