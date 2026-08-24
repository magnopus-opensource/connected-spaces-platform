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
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"

#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

using namespace schema;

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::ScriptData),
    "Script", // not exposed to scripting
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::ScriptSource),
            "scriptSource",
            PlainValue<std::string> { "" },
        },
        {
            static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::OwnerId),
            "ownerId",
            PlainValue<int64_t> { static_cast<int64_t>(0) },
        },
        {
            static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::ScriptScope),
            "scriptScope",
            EnumeratedValue<int64_t> {
                static_cast<int64_t>(ScriptScope::Owner),
                {
                    SchemaOption<int64_t> { "Local", static_cast<int64_t>(ScriptScope::Local) },
                    SchemaOption<int64_t> { "Owner", static_cast<int64_t>(ScriptScope::Owner) },
                },
            },
        },
    },
    /*.IsScriptable =*/false, // not exposed to scripting historically, so honouring that for now
};

const ComponentSchema& ScriptSpaceComponent::GetSchema() { return Schema; }

ScriptSpaceComponent::ScriptSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ScriptSpaceComponent(Schema, LogSystem, Parent)
{
}

std::unique_ptr<ScriptSpaceComponent> ScriptSpaceComponent::TryMake(
    const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
{
    if (!IsCompatible(ScriptSpaceComponent::GetSchema(), InSchema))
    {
        return nullptr;
    }

    return std::unique_ptr<ScriptSpaceComponent>(new ScriptSpaceComponent(InSchema, LogSystem, Parent));
}

ScriptSpaceComponent::ScriptSpaceComponent(const ComponentSchema& InSchema, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(InSchema, LogSystem, Parent)
{
    Parent->GetScript().SetScriptSpaceComponent(this);
}

const csp::common::String& ScriptSpaceComponent::GetScriptSource() const
{
    return GetStringProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource));
}

void ScriptSpaceComponent::SetScriptSource(const csp::common::String& Value)
{
    // CSP_LOG_WARN_FORMAT("ScriptSpaceComponent::SetScriptSource '%s'", Value.c_str());

    SetPropertyDirect(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource), Value);
    Parent->GetScript().OnSourceChanged(Value);
}

int64_t ScriptSpaceComponent::GetOwnerId() const { return GetIntegerProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId)); }

void ScriptSpaceComponent::SetOwnerId(int64_t OwnerId) { SetPropertyDirect(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId), OwnerId); }

ScriptScope ScriptSpaceComponent::GetScriptScope() const
{
    return static_cast<ScriptScope>(GetIntegerProperty((uint32_t)ScriptComponentPropertyKeys::ScriptScope));
}

void ScriptSpaceComponent::SetScriptScope(ScriptScope Scope)
{
    SetPropertyDirect(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptScope), static_cast<int64_t>(Scope));
}

void ScriptSpaceComponent::SetPropertyFromPatch(uint32_t Key, const csp::common::ReplicatedValue& Value)
{
    ComponentBase::SetPropertyFromPatch(Key, Value);

    if (Key == static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource))
    {
        Parent->GetScript().OnSourceChanged(Value.GetString());
        Parent->GetScript().Invoke();
    }
}

void ScriptSpaceComponent::OnRemove() { Parent->GetScript().Shutdown(); }

} // namespace csp::multiplayer
