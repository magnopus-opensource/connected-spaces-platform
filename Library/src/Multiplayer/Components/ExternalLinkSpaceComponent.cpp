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

#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ExternalLinkSpaceComponentScriptInterface.h"

#include <Debug/Logging.h>

namespace csp::multiplayer
{

ExternalLinkSpaceComponent::ExternalLinkSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::ExternalLink, Parent)
{
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::Name_DEPRECATED)] = "";
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::LinkUrl)] = "";
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::Scale)] = csp::common::Vector3 { 1, 1, 1 };
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::DisplayText)] = "";
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::IsEnabled)] = true;
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(ExternalLinkPropertyKeys::IsARVisible)] = true;

    SetScriptInterface(CSP_NEW ExternalLinkSpaceComponentScriptInterface(this));
}

const csp::common::String& ExternalLinkSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Name_DEPRECATED));
}

void ExternalLinkSpaceComponent::SetName(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Name_DEPRECATED), Value);
}

const csp::common::String& ExternalLinkSpaceComponent::GetLinkUrl() const
{
    return GetStringProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::LinkUrl));
}

void ExternalLinkSpaceComponent::SetLinkUrl(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::LinkUrl), Value);
}

/* ITransformComponent */

const csp::common::Vector3& ExternalLinkSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ExternalLinkPropertyKeys::Position));
}

void ExternalLinkSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Position), Value);
}

const csp::common::Vector4& ExternalLinkSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ExternalLinkPropertyKeys::Rotation));
}

void ExternalLinkSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& ExternalLinkSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(ExternalLinkPropertyKeys::Scale));
}

void ExternalLinkSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::Scale), Value);
}

SpaceTransform ExternalLinkSpaceComponent::GetTransform() const
{
    SpaceTransform Transform;
    Transform.Position = GetPosition();
    Transform.Rotation = GetRotation();
    Transform.Scale = GetScale();

    return Transform;
}

void ExternalLinkSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

const csp::common::String& ExternalLinkSpaceComponent::GetDisplayText() const
{
    return GetStringProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::DisplayText));
}

void ExternalLinkSpaceComponent::SetDisplayText(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::DisplayText), Value);
}

/* IClickableComponent */

bool ExternalLinkSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsEnabled)); }

void ExternalLinkSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsEnabled), Value); }

bool ExternalLinkSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVisible)); }

void ExternalLinkSpaceComponent::SetIsVisible(bool InValue) { SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsVisible), InValue); }

bool ExternalLinkSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsARVisible)); }

void ExternalLinkSpaceComponent::SetIsARVisible(bool InValue) { SetProperty(static_cast<uint32_t>(ExternalLinkPropertyKeys::IsARVisible), InValue); }

} // namespace csp::multiplayer
