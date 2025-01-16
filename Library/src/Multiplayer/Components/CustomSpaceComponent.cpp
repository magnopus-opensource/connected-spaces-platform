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

#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h"

#include <Debug/Logging.h>
#include <algorithm>
#include <functional>

namespace csp::multiplayer
{

CustomSpaceComponent::CustomSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Custom, Parent) //, NumProperties(2)
{
    Properties[static_cast<uint32_t>(CustomComponentPropertyKeys::ApplicationOrigin)] = "";

    SetScriptInterface(CSP_NEW CustomSpaceComponentScriptInterface(this));
}

const csp::common::String& CustomSpaceComponent::GetApplicationOrigin() const
{
    return GetStringProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::ApplicationOrigin));
}

void CustomSpaceComponent::SetApplicationOrigin(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::ApplicationOrigin), Value);
}

uint32_t CustomSpaceComponent::GetCustomPropertySubscriptionKey(const csp::common::String& Key) const
{
    // attempt to ensure no hash clashes with existing keys, but not bulletproof
    return static_cast<uint32_t>(
        static_cast<uint16_t>(std::hash<std::string> {}(Key.c_str())) + static_cast<uint16_t>(CustomComponentPropertyKeys::Num));
}

bool CustomSpaceComponent::HasCustomProperty(const csp::common::String& Key) const
{
    const uint32_t PropertyKey = GetCustomPropertySubscriptionKey(Key);

    return Properties.HasKey(PropertyKey);
}

const ReplicatedValue& CustomSpaceComponent::GetCustomProperty(const csp::common::String& Key) const
{
    const uint32_t PropertyKey = GetCustomPropertySubscriptionKey(Key);

    return GetProperty(PropertyKey);
}

void CustomSpaceComponent::SetCustomProperty(const csp::common::String& Key, const ReplicatedValue& Value)
{
    if (Value.GetReplicatedValueType() != ReplicatedValueType::InvalidType)
    {
        const uint32_t PropertyKey = GetCustomPropertySubscriptionKey(Key);
        if (!Properties.HasKey(PropertyKey))
        {
            AddKey(Key);
        }
        SetProperty(PropertyKey, Value);
    }
}

void CustomSpaceComponent::RemoveCustomProperty(const csp::common::String& Key)
{
    const uint32_t PropertyKey = GetCustomPropertySubscriptionKey(Key);

    if (Properties.HasKey(PropertyKey))
    {
        RemoveProperty(PropertyKey);
        RemoveKey(Key);
    }
}

csp::common::List<csp::common::String> CustomSpaceComponent::GetCustomPropertyKeys() const
{
    if (Properties.HasKey(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)))
    {
        const auto& RepVal = GetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList));

        if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String && !RepVal.GetString().IsEmpty())
        {
            return GetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)).GetString().Split(',');
        }
    }

    return csp::common::List<csp::common::String>();
}

int32_t CustomSpaceComponent::GetNumProperties() const
{
    if (Properties.HasKey(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)))
    {
        return static_cast<uint32_t>(Properties.Size() - 1);
    }
    else
    {
        return static_cast<uint32_t>(Properties.Size());
    }
}

void CustomSpaceComponent::AddKey(const csp::common::String& Value)
{
    if (Properties.HasKey(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList)))
    {
        const auto& RepVal = GetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList));

        if (RepVal.GetReplicatedValueType() != ReplicatedValueType::String)
        {
            return;
        }

        csp::common::String ReturnKeys = RepVal.GetString();

        if (!ReturnKeys.IsEmpty())
        {
            ReturnKeys = ReturnKeys + "," + Value;
            SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), ReturnKeys);
        }
        else
        {
            SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), Value);
        }
    }
    else
    {
        SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), Value);
    }
}

void CustomSpaceComponent::RemoveKey(const csp::common::String& Key)
{
    const csp::common::String& CurrentKeys = GetStringProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList));
    csp::common::List<csp::common::String> KeyList = CurrentKeys.Split(',');
    if (KeyList.Contains(Key))
    {
        csp::common::String ReturnKeys;
        KeyList.RemoveItem(Key);
        if (KeyList.Size() != 0)
        {
            for (int i = 0; i < KeyList.Size(); ++i)
            {
                if (i == 0)
                {
                    ReturnKeys = KeyList[i];
                }
                else
                {
                    ReturnKeys = ReturnKeys + "," + KeyList[i];
                }
            }
        }

        SetProperty(static_cast<uint32_t>(CustomComponentPropertyKeys::CustomPropertyList), ReturnKeys);
    }
    else
    {
        CSP_LOG_ERROR_MSG("Key Not Found.");
    }
}

} // namespace csp::multiplayer
