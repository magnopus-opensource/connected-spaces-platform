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

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/TextSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

TextSpaceComponent::TextSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Text, Parent)
{
    Properties[static_cast<uint32_t>(TextPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(TextPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(TextPropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(TextPropertyKeys::Text)] = "";
    Properties[static_cast<uint32_t>(TextPropertyKeys::TextColor)] = csp::common::Vector3(1.0f, 1.0f, 1.0f);
    Properties[static_cast<uint32_t>(TextPropertyKeys::BackgroundColor)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible)] = true;
    Properties[static_cast<uint32_t>(TextPropertyKeys::Width)] = 1.0f;
    Properties[static_cast<uint32_t>(TextPropertyKeys::Height)] = 1.0f;
    Properties[static_cast<uint32_t>(TextPropertyKeys::BillboardMode)] = static_cast<int64_t>(BillboardMode::Off);
    Properties[static_cast<uint32_t>(TextPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(TextPropertyKeys::IsARVisible)] = true;

    SetScriptInterface(CSP_NEW TextSpaceComponentScriptInterface(this));
}

/* ITransformComponent */

const csp::common::String& TextSpaceComponent::GetText() const { return GetStringProperty(static_cast<uint32_t>(TextPropertyKeys::Text)); }

void TextSpaceComponent::SetText(const csp::common::String& Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Text), Value); }

const csp::common::Vector3& TextSpaceComponent::GetTextColor() const
{
    return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::TextColor));
}

void TextSpaceComponent::SetTextColor(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::TextColor), Value); }

const csp::common::Vector3& TextSpaceComponent::GetBackgroundColor() const
{
    return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor));
}

void TextSpaceComponent::SetBackgroundColor(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor), Value);
}

bool TextSpaceComponent::GetIsBackgroundVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible)); }

void TextSpaceComponent::SetIsBackgroundVisible(bool InValue) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible), InValue); }

float TextSpaceComponent::GetWidth() const { return GetFloatProperty(static_cast<uint32_t>(TextPropertyKeys::Width)); }

void TextSpaceComponent::SetWidth(float InValue) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Width), InValue); }

float TextSpaceComponent::GetHeight() const { return GetFloatProperty(static_cast<uint32_t>(TextPropertyKeys::Height)); }

void TextSpaceComponent::SetHeight(float InValue) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Height), InValue); }

const csp::common::Vector3& TextSpaceComponent::GetPosition() const { return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::Position)); }

void TextSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Position), Value); }

const csp::common::Vector4& TextSpaceComponent::GetRotation() const { return GetVector4Property(static_cast<uint32_t>(TextPropertyKeys::Rotation)); }

void TextSpaceComponent::SetRotation(const csp::common::Vector4& Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Rotation), Value); }

const csp::common::Vector3& TextSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(TextPropertyKeys::Scale)); }

void TextSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::Scale), Value); }

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

void TextSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsVisible), Value); }

bool TextSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible)); }

void TextSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible), Value); }

BillboardMode TextSpaceComponent::GetBillboardMode() const
{
    return static_cast<BillboardMode>(GetIntegerProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode)));
}

void TextSpaceComponent::SetBillboardMode(BillboardMode Value)
{
    SetProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode), static_cast<int64_t>(Value));
}
} // namespace csp::multiplayer
