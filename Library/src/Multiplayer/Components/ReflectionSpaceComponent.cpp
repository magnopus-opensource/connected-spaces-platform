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
#include "Multiplayer/ComponentSchema.h"

#include "Multiplayer/Script/ComponentBinding/ReflectionSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Reflection),
    "Reflection",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::Name_DEPRECATED),
            "name_DEPRECATED",
            PlainValue<std::string> { "" },
            /*.IsScriptable =*/false,
        },
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::ReflectionAssetId),
            "reflectionAssetId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::AssetCollectionId),
            "assetCollectionId",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::Position),
            "position",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::Zero() },
        },
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::Scale),
            "scale",
            PlainValue<csp::common::Vector3> { csp::common::Vector3::One() },
        },
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::ReflectionShape),
            "reflectionShape",
            EnumeratedValue<int64_t> {
                static_cast<int64_t>(ReflectionShape::UnitBox),
                {
                    SchemaOption<int64_t> { "UnitSphere", static_cast<int64_t>(ReflectionShape::UnitSphere) },
                    SchemaOption<int64_t> { "UnitBox", static_cast<int64_t>(ReflectionShape::UnitBox) },
                },
            },
        },
        {
            static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::ThirdPartyComponentRef),
            "thirdPartyComponentRef",
            PlainValue<std::string> { "" },
            /*.IsScriptable =*/false,
        },
    },
    /*.IsScriptable =*/false, // not exposed to scripting historically, so honouring that for now
};

const ComponentSchema& ReflectionSpaceComponent::GetSchema() { return Schema; }

ReflectionSpaceComponent::ReflectionSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ReflectionSpaceComponent(Schema, LogSystem, Parent)
{
}

std::unique_ptr<ReflectionSpaceComponent> ReflectionSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(ReflectionSpaceComponent::GetSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<ReflectionSpaceComponent>(new ReflectionSpaceComponent(InSchema, LogSystem, Parent));
}

ReflectionSpaceComponent::ReflectionSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
    SetScriptInterface(new ReflectionSpaceComponentScriptInterface(this));
}

const csp::common::String& ReflectionSpaceComponent::GetReflectionAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId));
}

void ReflectionSpaceComponent::SetReflectionAssetId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId), Value);
}

const csp::common::String& ReflectionSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId));
}

void ReflectionSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId), Value);
}

const csp::common::String& ReflectionSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Name_DEPRECATED));
}

void ReflectionSpaceComponent::SetName(const csp::common::String& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::Name_DEPRECATED), Value);
}

/* IPositionComponent */

const csp::common::Vector3& ReflectionSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ReflectionPropertyKeys::Position));
}

void ReflectionSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::Position), Value);
}

/* IScaleComponent */

const csp::common::Vector3& ReflectionSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(ReflectionPropertyKeys::Scale));
}

void ReflectionSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::Scale), Value);
}

ReflectionShape ReflectionSpaceComponent::GetReflectionShape() const
{
    return static_cast<ReflectionShape>(GetIntegerProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape)));
}

void ReflectionSpaceComponent::SetReflectionShape(ReflectionShape Value)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape), static_cast<int64_t>(Value));
}

const csp::common::String& ReflectionSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef));
}

void ReflectionSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetPropertyDirect(static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
