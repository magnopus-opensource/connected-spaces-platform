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
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/ComponentSchema.h"
#include "Multiplayer/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h"

#include <algorithm>
#include <functional>

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Custom),
    "Custom",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(CustomComponentPropertyKeys::ApplicationOrigin),
            "applicationOrigin",
            "",
        },
    },
};

const ComponentSchema& CustomSpaceComponent::GetSchema() { return Schema; }

CustomSpaceComponent::CustomSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent) //, NumProperties(2)
{
    SetScriptInterface(new CustomSpaceComponentScriptInterface(this));
}

const csp::common::String& CustomSpaceComponent::GetApplicationOrigin() const
{
    return GetStringProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::ApplicationOrigin));
}

void CustomSpaceComponent::SetApplicationOrigin(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::ApplicationOrigin), value);
}

uint32_t CustomSpaceComponent::GetCustomPropertySubscriptionKey(const csp::common::String& key) const
{
    // attempt to ensure no hash clashes with existing keys, but not bulletproof
    return static_cast<uint32_t>(
        static_cast<uint16_t>(std::hash<std::string> {}(key.c_str())) + static_cast<uint16_t>(CustomComponentPropertyKeys::Num));
}

bool CustomSpaceComponent::HasCustomProperty(const csp::common::String& key) const
{
    const uint32_t propertyKey = GetCustomPropertySubscriptionKey(key);

    return m_properties.HasKey(propertyKey);
}

const csp::common::ReplicatedValue& CustomSpaceComponent::GetCustomProperty(const csp::common::String& key) const
{
    const uint32_t propertyKey = GetCustomPropertySubscriptionKey(key);

    return GetProperty(propertyKey);
}

void CustomSpaceComponent::SetCustomProperty(const csp::common::String& key, const csp::common::ReplicatedValue& value)
{
    if (value.GetReplicatedValueType() != csp::common::ReplicatedValueType::InvalidType)
    {
        const uint32_t propertyKey = GetCustomPropertySubscriptionKey(key);
        if (!m_properties.HasKey(propertyKey))
        {
            AddKey(key);
        }
        SetProperty(propertyKey, value);
    }
}

void CustomSpaceComponent::RemoveCustomProperty(const csp::common::String& key)
{
    const uint32_t propertyKey = GetCustomPropertySubscriptionKey(key);

    if (m_properties.HasKey(propertyKey))
    {
        RemoveProperty(propertyKey);
        RemoveKey(key);
    }
}

csp::common::List<csp::common::String> CustomSpaceComponent::GetCustomPropertyKeys() const
{
    if (m_properties.HasKey(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)))
    {
        const auto& repVal = GetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList));

        if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::String && !repVal.GetString().IsEmpty())
        {
            return GetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)).GetString().Split(',');
        }
    }

    return csp::common::List<csp::common::String>();
}

int32_t CustomSpaceComponent::GetNumProperties() const
{
    if (m_properties.HasKey(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)))
    {
        return static_cast<uint32_t>(m_properties.Size() - 1);
    }
    else
    {
        return static_cast<uint32_t>(m_properties.Size());
    }
}

void CustomSpaceComponent::AddKey(const csp::common::String& value)
{
    if (m_properties.HasKey(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)))
    {
        const auto& repVal = GetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList));

        if (repVal.GetReplicatedValueType() != csp::common::ReplicatedValueType::String)
        {
            return;
        }

        csp::common::String returnKeys = repVal.GetString();

        if (!returnKeys.IsEmpty())
        {
            returnKeys = returnKeys + "," + value;
            SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), returnKeys);
        }
        else
        {
            SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), value);
        }
    }
    else
    {
        SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), value);
    }
}

void CustomSpaceComponent::RemoveKey(const csp::common::String& key)
{
    const csp::common::String& currentKeys = GetStringProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList));
    csp::common::List<csp::common::String> keyList = currentKeys.Split(',');
    if (keyList.Contains(key))
    {
        csp::common::String returnKeys;
        keyList.RemoveItem(key);
        if (keyList.Size() != 0)
        {
            for (size_t i = 0; i < keyList.Size(); ++i)
            {
                if (i == 0)
                {
                    returnKeys = keyList[i];
                }
                else
                {
                    returnKeys = returnKeys + "," + keyList[i];
                }
            }
        }

        SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), returnKeys);
    }
    else
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "Key Not Found.");
        }
    }
}

} // namespace csp::multiplayer
