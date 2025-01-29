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

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ButtonSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

ButtonSpaceComponent::ButtonSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Button, Parent)
{
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::LabelText)] = "";
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId)] = "";
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::Scale)] = csp::common::Vector3 { 1, 1, 1 };
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled)] = true;
    Properties[static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible)] = true;

    SetScriptInterface(CSP_NEW ButtonSpaceComponentScriptInterface(this));
}

const csp::common::String& ButtonSpaceComponent::GetLabelText() const
{
    return GetStringProperty(static_cast<uint32_t>(ButtonPropertyKeys::LabelText));
}

void ButtonSpaceComponent::SetLabelText(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::LabelText), Value);
}

const csp::common::String& ButtonSpaceComponent::GetIconAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId));
}

void ButtonSpaceComponent::SetIconAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId), Value);
}

const csp::common::String& ButtonSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId));
}

void ButtonSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId), Value);
}

/* ITransformComponent */

const csp::common::Vector3& ButtonSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ButtonPropertyKeys::Position));
}

void ButtonSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Position), Value); }

const csp::common::Vector4& ButtonSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ButtonPropertyKeys::Rotation));
}

void ButtonSpaceComponent::SetRotation(const csp::common::Vector4& Value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Rotation), Value); }

const csp::common::Vector3& ButtonSpaceComponent::GetScale() const { return GetVector3Property(static_cast<uint32_t>(ButtonPropertyKeys::Scale)); }

void ButtonSpaceComponent::SetScale(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Scale), Value); }

SpaceTransform ButtonSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void ButtonSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

/* IClickableComponent */

bool ButtonSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled)); }

void ButtonSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled), Value); }

/* IVisibleComponent */

bool ButtonSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVisible)); }

void ButtonSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVisible), Value); }

bool ButtonSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible)); }

void ButtonSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible), Value); }

} // namespace csp::multiplayer
