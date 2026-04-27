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

#include "CSP/Multiplayer/ComponentSchema.h"
#include "Multiplayer/Script/ComponentBinding/PortalSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Portal),
    "Portal",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsVisible),
            {}, // not exposed to scripting
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsActive),
            {}, // not exposed to scripting
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::SpaceId),
            "spaceId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsARVisible),
            {}, // not exposed to scripting
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsEnabled),
            "isEnabled",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::Radius),
            "radius",
            1.5f,
        },
    },
};

const ComponentSchema& PortalSpaceComponent::GetSchema() { return Schema; }

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