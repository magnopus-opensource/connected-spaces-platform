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

#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::ScriptData),
    {}, // not exposed to scripting
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::ScriptSource),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::OwnerId),
            {}, // not exposed to scripting
            static_cast<int64_t>(0),
        },
        {
            static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::ScriptScope),
            {}, // not exposed to scripting
            static_cast<int64_t>(ScriptScope::Owner),
        },
    },
};

const ComponentSchema& ScriptSpaceComponent::GetSchema() { return Schema; }

ScriptSpaceComponent::ScriptSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
    parent->GetScript().SetScriptSpaceComponent(this);
}

const csp::common::String& ScriptSpaceComponent::GetScriptSource() const
{
    return GetStringProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource));
}

void ScriptSpaceComponent::SetScriptSource(const csp::common::String& value)
{
    // CSP_LOG_WARN_FORMAT("ScriptSpaceComponent::SetScriptSource '%s'", Value.c_str());

    SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource), value);
    m_parent->GetScript().OnSourceChanged(value);
}

int64_t ScriptSpaceComponent::GetOwnerId() const { return GetIntegerProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId)); }

void ScriptSpaceComponent::SetOwnerId(int64_t ownerId) { SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId), ownerId); }

ScriptScope ScriptSpaceComponent::GetScriptScope() const
{
    return static_cast<ScriptScope>(GetIntegerProperty((uint32_t)ScriptComponentPropertyKeys::ScriptScope));
}

void ScriptSpaceComponent::SetScriptScope(ScriptScope scope)
{
    SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptScope), static_cast<int64_t>(scope));
}

void ScriptSpaceComponent::SetPropertyFromPatch(uint32_t key, const csp::common::ReplicatedValue& value)
{
    ComponentBase::SetPropertyFromPatch(key, value);

    if (key == static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource))
    {
        // CSP_LOG_WARN_FORMAT("ScriptSpaceComponent::SetPropertyFromPatch '%s'", Value.GetString().c_str());

        m_parent->GetScript().Bind();
        m_parent->GetScript().Invoke();
    }
}

void ScriptSpaceComponent::OnRemove() { m_parent->GetScript().Shutdown(); }

} // namespace csp::multiplayer
