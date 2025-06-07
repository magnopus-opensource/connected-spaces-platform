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
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"

#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

namespace csp::multiplayer
{

CodeSpaceComponent::CodeSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Code, Parent)
{
    Properties[static_cast<uint32_t>(CodeComponentPropertyKeys::ScriptAssetPath)] = "";

}

const csp::common::String& CodeSpaceComponent::GetScriptAssetPath() const
{
    return GetStringProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::ScriptAssetPath));
}

void CodeSpaceComponent::SetScriptAssetPath(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::ScriptAssetPath), Value);
}

CodeScopeType CodeSpaceComponent::GetCodeScopeType() const
{
    return static_cast<CodeScopeType>(GetIntegerProperty((uint32_t)CodeComponentPropertyKeys::CodeScopeType));
}

void CodeSpaceComponent::SetCodeScopeType(CodeScopeType Scope)
{
    SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::CodeScopeType), static_cast<int64_t>(Scope));
}

void CodeSpaceComponent::OnRemove() {
    // not implemented
    CSP_LOG_ERROR_MSG("CodeSpaceComponent::OnRemove not implemented");
}

uint32_t CodeSpaceComponent::GetAttributeSubscriptionKey(const csp::common::String& Key) const
{
    // attempt to ensure no hash clashes with existing keys, but not bulletproof
    return static_cast<uint32_t>(
        static_cast<uint16_t>(std::hash<std::string> {}(Key.c_str())) + static_cast<uint16_t>(CodeComponentPropertyKeys::Num));
}

bool CodeSpaceComponent::HasAttribute(const csp::common::String& Key) const
{
    const uint32_t PropertyKey = GetAttributeSubscriptionKey(Key);

    return Properties.HasKey(PropertyKey);
}

const ReplicatedValue& CodeSpaceComponent::GetAttribute(const csp::common::String& Key) const
{
    const uint32_t PropertyKey = GetAttributeSubscriptionKey(Key);

    return GetProperty(PropertyKey);
}

void CodeSpaceComponent::SetAttribute(const csp::common::String& Key, const ReplicatedValue& Value)
{
    if (Value.GetReplicatedValueType() != ReplicatedValueType::InvalidType)
    {
        const uint32_t PropertyKey = GetAttributeSubscriptionKey(Key);
        if (!Properties.HasKey(PropertyKey))
        {
            AddKey(Key);
        }
        SetProperty(PropertyKey, Value);
    }
}

void CodeSpaceComponent::RemoveAttribute(const csp::common::String& Key)
{
    const uint32_t PropertyKey = GetAttributeSubscriptionKey(Key);

    if (Properties.HasKey(PropertyKey))
    {
        RemoveProperty(PropertyKey);
        RemoveKey(Key);
    }
}

csp::common::List<csp::common::String> CodeSpaceComponent::GetAttributeKeys() const
{
    if (Properties.HasKey(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes)))
    {
        const auto& RepVal = GetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));

        if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String && !RepVal.GetString().IsEmpty())
        {
            return GetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes)).GetString().Split(',');
        }
    }

    return csp::common::List<csp::common::String>();
}

int32_t CodeSpaceComponent::GetNumProperties() const
{
    if (Properties.HasKey(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes)))
    {
        return static_cast<uint32_t>(Properties.Size() - 1);
    }
    else
    {
        return static_cast<uint32_t>(Properties.Size());
    }
}

void CodeSpaceComponent::AddKey(const csp::common::String& Value)
{
    if (Properties.HasKey(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes)))
    {
        const auto& RepVal = GetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));

        if (RepVal.GetReplicatedValueType() != ReplicatedValueType::String)
        {
            return;
        }

        csp::common::String ReturnKeys = RepVal.GetString();

        if (!ReturnKeys.IsEmpty())
        {
            ReturnKeys = ReturnKeys + "," + Value;
            SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), ReturnKeys);
        }
        else
        {
            SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), Value);
        }
    }
    else
    {
        SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), Value);
    }
}

void CodeSpaceComponent::RemoveKey(const csp::common::String& Key)
{
    const csp::common::String& CurrentKeys = GetStringProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    csp::common::List<csp::common::String> KeyList = CurrentKeys.Split(',');
    if (KeyList.Contains(Key))
    {
        csp::common::String ReturnKeys;
        KeyList.RemoveItem(Key);
        if (KeyList.Size() != 0)
        {
            for (size_t i = 0; i < KeyList.Size(); ++i)
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

        SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), ReturnKeys);
    }
    else
    {
        CSP_LOG_ERROR_MSG("Key Not Found.");
    }
}

} // namespace csp::multiplayer
