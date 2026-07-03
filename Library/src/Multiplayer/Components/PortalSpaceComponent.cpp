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

#include "Multiplayer/ComponentSchemaRegistry.h"

namespace csp::multiplayer
{

PortalSpaceComponent::PortalSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : PortalSpaceComponent(GetPortalSchema(), LogSystem, Parent)
{
}

std::unique_ptr<PortalSpaceComponent> PortalSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(GetPortalSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<PortalSpaceComponent>(new PortalSpaceComponent(InSchema, LogSystem, Parent));
}

PortalSpaceComponent::PortalSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
}

const csp::common::String& PortalSpaceComponent::GetSpaceId() const { return GetStringProperty(static_cast<uint32_t>(PortalPropertyKeys::SpaceId)); }

void PortalSpaceComponent::SetSpaceId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(PortalPropertyKeys::SpaceId), Value);
}

/* IPositionComponent */

const csp::common::Vector3& PortalSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(PortalPropertyKeys::Position));
}

void PortalSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(PortalPropertyKeys::Position), Value);
}

float PortalSpaceComponent::GetRadius() const { return GetFloatProperty(static_cast<uint32_t>(PortalPropertyKeys::Radius)); }

void PortalSpaceComponent::SetRadius(float Value) { SetPropertyDirect(static_cast<uint32_t>(PortalPropertyKeys::Radius), Value); }

bool PortalSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(PortalPropertyKeys::IsEnabled)); }

void PortalSpaceComponent::SetIsEnabled(bool Value) { SetPropertyDirect(static_cast<uint32_t>(PortalPropertyKeys::IsEnabled), Value); }

} // namespace csp::multiplayer