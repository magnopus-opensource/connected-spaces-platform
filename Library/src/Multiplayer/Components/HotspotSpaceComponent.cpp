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

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

HotspotSpaceComponent::HotspotSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Hotspot, Parent)
{
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED)] = "";
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint)] = true;
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint)] = false;
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible)] = true;

    SetScriptInterface(CSP_NEW HotspotSpaceComponentScriptInterface(this));
}

const csp::common::String& HotspotSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED));
}

void HotspotSpaceComponent::SetName(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED), Value);
}

bool HotspotSpaceComponent::GetIsTeleportPoint() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint)); }

void HotspotSpaceComponent::SetIsTeleportPoint(bool Value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint), Value); }

bool HotspotSpaceComponent::GetIsSpawnPoint() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint)); }

void HotspotSpaceComponent::SetIsSpawnPoint(bool Value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint), Value); }

csp::common::String HotspotSpaceComponent::GetUniqueComponentId() const
{
    csp::common::String UniqueComponentId = std::to_string(Parent->GetId()).c_str();
    UniqueComponentId += ":";
    UniqueComponentId += std::to_string(Id).c_str();

    return UniqueComponentId;
}

/* IPositionComponent */

const csp::common::Vector3& HotspotSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(HotspotPropertyKeys::Position));
}

void HotspotSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Position), Value);
}

/* IRotationComponent */

const csp::common::Vector4& HotspotSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(HotspotPropertyKeys::Rotation));
}

void HotspotSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Rotation), Value);
}

/* IVisibleComponent */

bool HotspotSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVisible)); }

void HotspotSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVisible), Value); }

bool HotspotSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible)); }

void HotspotSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible), Value); }

void HotspotSpaceComponent::OnLocalDelete()
{
    auto CB = [](const csp::systems::NullResult& Result) {

    };

    systems::SystemsManager::Get().GetHotspotSequenceSystem()->RemoveItemFromGroups(GetUniqueComponentId(), CB);
}

} // namespace csp::multiplayer
