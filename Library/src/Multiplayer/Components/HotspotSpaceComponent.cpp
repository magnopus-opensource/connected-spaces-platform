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
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"

#include "CSP/Multiplayer/ComponentSchema.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Hotspot),
    "Hotspot",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::Position),
            "position",
            csp::common::Vector3::Zero(),
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::Name_DEPRECATED),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsTeleportPoint),
            "isTeleportPoint",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsSpawnPoint),
            "isSpawnPoint",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsARVisible),
            "isARVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsVirtualVisible),
            "isVirtualVisible",
            true,
        },
    },
};

const ComponentSchema& HotspotSpaceComponent::GetSchema() { return Schema; }

HotspotSpaceComponent::HotspotSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
    SetScriptInterface(new HotspotSpaceComponentScriptInterface(this));
}

const csp::common::String& HotspotSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED));
}

void HotspotSpaceComponent::SetName(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED), value);
}

bool HotspotSpaceComponent::GetIsTeleportPoint() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint)); }

void HotspotSpaceComponent::SetIsTeleportPoint(bool value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint), value); }

bool HotspotSpaceComponent::GetIsSpawnPoint() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint)); }

void HotspotSpaceComponent::SetIsSpawnPoint(bool value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint), value); }

csp::common::String HotspotSpaceComponent::GetUniqueComponentId() const
{
    csp::common::String uniqueComponentId = std::to_string(m_parent->GetId()).c_str();
    uniqueComponentId += ":";
    uniqueComponentId += std::to_string(m_id).c_str();

    return uniqueComponentId;
}

/* IPositionComponent */

const csp::common::Vector3& HotspotSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(HotspotPropertyKeys::Position));
}

void HotspotSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Position), value);
}

/* IRotationComponent */

const csp::common::Vector4& HotspotSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(HotspotPropertyKeys::Rotation));
}

void HotspotSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Rotation), value);
}

/* IVisibleComponent */

bool HotspotSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVisible)); }

void HotspotSpaceComponent::SetIsVisible(bool value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVisible), value); }

bool HotspotSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible)); }

void HotspotSpaceComponent::SetIsARVisible(bool value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible), value); }

bool HotspotSpaceComponent::GetIsVirtualVisible() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVirtualVisible)); }

void HotspotSpaceComponent::SetIsVirtualVisible(bool value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVirtualVisible), value); }

} // namespace csp::multiplayer
