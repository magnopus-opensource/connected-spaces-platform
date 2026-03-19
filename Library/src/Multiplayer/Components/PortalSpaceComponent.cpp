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

#include "Multiplayer/Component/Schema.h"
#include "Multiplayer/Script/ComponentBinding/PortalSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

const auto Schema = ComponentBase::ComponentSchema {
    ComponentType::Portal,
    std::vector<ComponentBase::ComponentSchema::Property> {
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::IsVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::IsActive),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::SpaceId),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::IsARVisible),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::IsEnabled),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::Position),
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentBase::PropertyKey>(PortalPropertyKeys::Radius),
            1.5f,
        },
    },
};

PortalSpaceComponent::PortalSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new PortalSpaceComponentScriptInterface(this));
}

const csp::common::String& PortalSpaceComponent::GetSpaceId() const { return GetStringProperty(static_cast<uint32_t>(PortalPropertyKeys::SpaceId)); }

void PortalSpaceComponent::SetSpaceId(const csp::common::String& Value) { SetProperty(static_cast<uint32_t>(PortalPropertyKeys::SpaceId), Value); }

/* IPositionComponent */

const csp::common::Vector3& PortalSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(PortalPropertyKeys::Position));
}

void PortalSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(PortalPropertyKeys::Position), Value); }

float PortalSpaceComponent::GetRadius() const { return GetFloatProperty(static_cast<uint32_t>(PortalPropertyKeys::Radius)); }

void PortalSpaceComponent::SetRadius(float Value) { SetProperty(static_cast<uint32_t>(PortalPropertyKeys::Radius), Value); }

bool PortalSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(PortalPropertyKeys::IsEnabled)); }

void PortalSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(PortalPropertyKeys::IsEnabled), Value); }

} // namespace csp::multiplayer