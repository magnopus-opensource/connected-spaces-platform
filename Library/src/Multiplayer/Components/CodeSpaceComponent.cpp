/*
 * Copyright 2026 Magnopus LLC

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

namespace csp::multiplayer
{

CodeSpaceComponent::CodeSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(ComponentType::Code, LogSystem, Parent)
{
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::ScriptAssetPath)] = "";
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::CodeScopeType)] = static_cast<int64_t>(csp::multiplayer::CodeScopeType::Owner);
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes)] = csp::common::Map<csp::common::String, csp::common::ReplicatedValue>();
}

const csp::common::String& CodeSpaceComponent::GetScriptAssetPath() const
{
    return GetStringProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::ScriptAssetPath));
}

void CodeSpaceComponent::SetScriptAssetPath(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::ScriptAssetPath), Value);
}

csp::multiplayer::CodeScopeType CodeSpaceComponent::GetCodeScopeType() const
{
    return static_cast<csp::multiplayer::CodeScopeType>(GetIntegerProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::CodeScopeType)));
}

void CodeSpaceComponent::SetCodeScopeType(csp::multiplayer::CodeScopeType Value)
{
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::CodeScopeType), static_cast<int64_t>(Value));
}

bool CodeSpaceComponent::HasAttribute(const csp::common::String& Key) const
{
    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    return Attributes.HasKey(Key);
}

bool CodeSpaceComponent::GetAttribute(const csp::common::String& Key, CodeAttribute& OutAttribute) const
{
    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    const auto AttributeIt = Attributes.Find(Key);
    if (AttributeIt == Attributes.end())
    {
        return false;
    }

    return CodeAttribute::TryFromReplicatedValue(AttributeIt->second, OutAttribute);
}

void CodeSpaceComponent::SetAttribute(const csp::common::String& Key, const CodeAttribute& Attribute)
{
    auto Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    Attributes[Key] = Attribute.ToReplicatedValue();
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes), Attributes);
}

void CodeSpaceComponent::RemoveAttribute(const csp::common::String& Key)
{
    auto Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    Attributes.Remove(Key);
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes), Attributes);
}

void CodeSpaceComponent::ClearAttributes()
{
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes),
        csp::common::Map<csp::common::String, csp::common::ReplicatedValue>());
}

csp::common::List<csp::common::String> CodeSpaceComponent::GetAttributeKeys() const
{
    csp::common::List<csp::common::String> AttributeKeys;
    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    for (const auto& AttributePair : Attributes)
    {
        AttributeKeys.Append(AttributePair.first);
    }
    return AttributeKeys;
}

} // namespace csp::multiplayer

