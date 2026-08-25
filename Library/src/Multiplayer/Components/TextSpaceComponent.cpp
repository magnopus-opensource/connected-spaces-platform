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
#include "Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

using namespace schema;

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Text),
    "Text",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Position),
            "position",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::Zero() },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Rotation),
            "rotation",
            PlainValue<csp::common::Vector4> { csp::common::Vector4 { 0, 0, 0, 1 } },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Scale),
            "scale",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::One() },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Text),
            "text",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::TextColor),
            "textColor",
            PlainValue<csp::common::Vector3> { csp::common::Vector3(1.0f, 1.0f, 1.0f) },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::BackgroundColor),
            "backgroundColor",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::Zero() },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsBackgroundVisible),
            "isBackgroundVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Width),
            "width",
            PlainValue<float> { 1.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Height),
            "height",
            PlainValue<float> { 1.0f },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::BillboardMode),
            "billboardMode",
            EnumeratedValue<int64_t> {
                static_cast<int64_t>(BillboardMode::Off),
                {
                    SchemaOption<int64_t> { "Off", static_cast<int64_t>(BillboardMode::Off) },
                    SchemaOption<int64_t> { "Billboard", static_cast<int64_t>(BillboardMode::Billboard) },
                    SchemaOption<int64_t> { "YawLockedBillboard", static_cast<int64_t>(BillboardMode::YawLockedBillboard) },
                },
            },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsVisible),
            "isVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsARVisible),
            "isARVisible",
            PlainValue<bool> { true },
        },
        {
            static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            PlainValue<bool> { true },
        },
    },
};

const ComponentSchema& TextSpaceComponent::GetSchema() { return Schema; }

TextSpaceComponent::TextSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : TextSpaceComponent(Schema, LogSystem, Parent)
{
}

std::unique_ptr<TextSpaceComponent> TextSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(TextSpaceComponent::GetSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<TextSpaceComponent>(new TextSpaceComponent(InSchema, LogSystem, Parent));
}

TextSpaceComponent::TextSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
}

/* ITransformComponent */

const csp::common::String& TextSpaceComponent::GetText() const { return GetStringProperty(static_cast<uint32_t>(TextPropertyKeys::Text)); }

void TextSpaceComponent::SetText(const csp::common::String& Value) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::Text), Value); }

const csp::common::Vector3& TextSpaceComponent::GetTextColor() const
{
    return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::TextColor));
}

void TextSpaceComponent::SetTextColor(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::TextColor), Value);
}

const csp::common::Vector3& TextSpaceComponent::GetBackgroundColor() const
{
    return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor));
}

void TextSpaceComponent::SetBackgroundColor(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor), Value);
}

bool TextSpaceComponent::GetIsBackgroundVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible)); }

void TextSpaceComponent::SetIsBackgroundVisible(bool InValue)
{
    SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible), InValue);
}

float TextSpaceComponent::GetWidth() const { return GetFloatProperty(static_cast<uint32_t>(TextPropertyKeys::Width)); }

void TextSpaceComponent::SetWidth(float InValue) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::Width), InValue); }

float TextSpaceComponent::GetHeight() const { return GetFloatProperty(static_cast<uint32_t>(TextPropertyKeys::Height)); }

void TextSpaceComponent::SetHeight(float InValue) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::Height), InValue); }

const csp::common::Vector3& TextSpaceComponent::GetPosition() const { return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::Position)); }

void TextSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::Position), Value);
}

const csp::common::Vector4& TextSpaceComponent::GetRotation() const { return GetVector4Property(static_cast<uint32_t>(TextPropertyKeys::Rotation)); }

void TextSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& TextSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::Scale)); }

void TextSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::Scale), Value); }

SpaceTransform TextSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void TextSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

/* IVisibleComponent */

bool TextSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsVisible)); }

void TextSpaceComponent::SetIsVisible(bool Value) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::IsVisible), Value); }

bool TextSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible)); }

void TextSpaceComponent::SetIsARVisible(bool Value) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::IsARVisible), Value); }

bool TextSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsVirtualVisible)); }

void TextSpaceComponent::SetIsVirtualVisible(bool Value) { SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::IsVirtualVisible), Value); }

BillboardMode TextSpaceComponent::GetBillboardMode() const
{
    return static_cast<BillboardMode>(GetIntegerProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode)));
}

void TextSpaceComponent::SetBillboardMode(BillboardMode Value)
{
    SetPropertyDirect(static_cast<uint32_t>(TextPropertyKeys::BillboardMode), static_cast<int64_t>(Value));
}
} // namespace csp::multiplayer
