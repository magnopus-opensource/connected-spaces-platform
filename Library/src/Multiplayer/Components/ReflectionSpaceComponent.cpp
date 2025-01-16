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

#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ReflectionSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

ReflectionSpaceComponent::ReflectionSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Reflection, Parent)
{
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::Name_DEPRECATED)] = "";
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId)] = "";
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::Position)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::Scale)] = csp::common::Vector3::One();
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape)] = static_cast<int64_t>(ReflectionShape::UnitBox);
    Properties[static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef)] = "";

    SetScriptInterface(CSP_NEW ReflectionSpaceComponentScriptInterface(this));
}

const csp::common::String& ReflectionSpaceComponent::GetReflectionAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId));
}

void ReflectionSpaceComponent::SetReflectionAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId), Value);
}

const csp::common::String& ReflectionSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId));
}

void ReflectionSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId), Value);
}

const csp::common::String& ReflectionSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Name_DEPRECATED));
}

void ReflectionSpaceComponent::SetName(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Name_DEPRECATED), Value);
}

/* IPositionComponent */

const csp::common::Vector3& ReflectionSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ReflectionPropertyKeys::Position));
}

void ReflectionSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Position), Value);
}

/* IScaleComponent */

const csp::common::Vector3& ReflectionSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(ReflectionPropertyKeys::Scale));
}

void ReflectionSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Scale), Value);
}

ReflectionShape ReflectionSpaceComponent::GetReflectionShape() const
{
    return static_cast<ReflectionShape>(GetIntegerProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape)));
}

void ReflectionSpaceComponent::SetReflectionShape(ReflectionShape Value)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape), static_cast<int64_t>(Value));
}

const csp::common::String& ReflectionSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef));
}

void ReflectionSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
