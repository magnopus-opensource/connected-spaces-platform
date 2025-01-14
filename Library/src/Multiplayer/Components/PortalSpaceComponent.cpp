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
#include "CSP/Multiplayer/Components/PortalSpaceComponent.h"

#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/PortalSpaceComponentScriptInterface.h"

csp::multiplayer::PortalSpaceComponent::PortalSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Portal, Parent)
{
    Properties[static_cast<uint32_t>(PortalPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(PortalPropertyKeys::IsActive)] = true;
    Properties[static_cast<uint32_t>(PortalPropertyKeys::SpaceId)] = "";
    Properties[static_cast<uint32_t>(PortalPropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint32_t>(PortalPropertyKeys::IsEnabled)] = true;
    Properties[static_cast<uint32_t>(PortalPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(PortalPropertyKeys::Radius)] = 1.5f;

    SetScriptInterface(CSP_NEW PortalSpaceComponentScriptInterface(this));
}

const csp::common::String& csp::multiplayer::PortalSpaceComponent::GetSpaceId() const
{
    return GetStringProperty(static_cast<uint32_t>(PortalPropertyKeys::SpaceId));
}

void csp::multiplayer::PortalSpaceComponent::SetSpaceId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(PortalPropertyKeys::SpaceId), Value);
}

void csp::multiplayer::PortalSpaceComponent::GetSpaceThumbnail(csp::systems::UriResultCallback Callback) const
{
    csp::systems::SpaceSystem* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();

    SpaceSystem->GetSpaceThumbnail(GetSpaceId(), Callback);
}

/* IPositionComponent */

const csp::common::Vector3& csp::multiplayer::PortalSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(PortalPropertyKeys::Position));
}

void csp::multiplayer::PortalSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(PortalPropertyKeys::Position), Value);
}

float csp::multiplayer::PortalSpaceComponent::GetRadius() const { return GetFloatProperty(static_cast<uint32_t>(PortalPropertyKeys::Radius)); }

void csp::multiplayer::PortalSpaceComponent::SetRadius(float Value) { SetProperty(static_cast<uint32_t>(PortalPropertyKeys::Radius), Value); }

bool csp::multiplayer::PortalSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(PortalPropertyKeys::IsEnabled)); }

void csp::multiplayer::PortalSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(PortalPropertyKeys::IsEnabled), Value); }
