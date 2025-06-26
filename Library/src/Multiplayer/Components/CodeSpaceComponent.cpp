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
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "Multiplayer/Script/ComponentBinding/CodeSpaceComponentScriptInterface.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"
#include "CSP/Multiplayer/LocalScript/LocalScriptSystem.h"

namespace csp::multiplayer
{

CodeSpaceComponent::CodeSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Code, Parent)
{
    Properties[static_cast<uint32_t>(CodeComponentPropertyKeys::ScriptAssetPath)] = "";
Properties[static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes)]
        = csp::common::Map<csp::common::String, csp::multiplayer::ReplicatedValue>();
    SetScriptInterface(new CodeSpaceComponentScriptInterface(this));
}

const csp::common::String& CodeSpaceComponent::GetScriptAssetPath() const
{
    return GetStringProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::ScriptAssetPath));
}

void CodeSpaceComponent::SetScriptAssetPath(const csp::common::String Value)
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
 
csp::multiplayer::CodeAttribute* CodeSpaceComponent::GetAttribute(const csp::common::String& Key) const
{
    auto ReplicatedAttributes = GetStringMapProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    if (ReplicatedAttributes.HasKey(Key))
    {
        const csp::common::String& serializedAttribute = ReplicatedAttributes[Key].GetString();
        // Create a heap-allocated CodeAttribute that will persist after this function returns
        CodeAttribute attribute = csp::multiplayer::CodeAttribute::Deserialize(serializedAttribute);
        return new CodeAttribute(attribute); // Create a new heap-allocated copy
    }
    return nullptr; // Return nullptr if not found
}

bool CodeSpaceComponent::HasAttribute(const csp::common::String& Key) const
{
    auto ReplicatedAttributes = GetStringMapProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    return ReplicatedAttributes.HasKey(Key);
}

csp::common::List<csp::common::String> CodeSpaceComponent::GetAttributeKeys() const
{
    common::Map<csp::common::String, csp::multiplayer::ReplicatedValue> ReplicatedAttributes = GetStringMapProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    std::unique_ptr<csp::common::Array<csp::common::String>> Keys(const_cast<csp::common::Array<csp::common::String>*>(ReplicatedAttributes.Keys()));

    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "CodeSpaceComponent::GetAttributeKeys: Found %zu keys", Keys->Size());
    csp::common::List<csp::common::String> Result;
    for (size_t i = 0; i < Keys->Size(); ++i)
    {
        Result.Append((*Keys)[i]);
    }
    
    return Result;
}

void CodeSpaceComponent::SetAttribute(const csp::common::String& Key, const csp::multiplayer::CodeAttribute& Attribute)
{
    // Remove the unused variable warnings and implement the function
    common::Map<common::String, multiplayer::ReplicatedValue> ReplicatedAttributes
        = GetStringMapProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    csp::common::String serializedAttribute = Attribute.Serialize();
    multiplayer::ReplicatedValue ReplicatedValue;
    ReplicatedValue.SetString(serializedAttribute);
    ReplicatedAttributes[Key] = ReplicatedValue;
    SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), ReplicatedAttributes);

    csp::systems::LocalScriptSystem* localScriptSystem = csp::systems::SystemsManager::Get().GetLocalScriptSystem();
    localScriptSystem->UpdateAttributeForEntity(this->GetParent()->GetId(), Key, Attribute);

    this->GetParent()->MarkForUpdate();
}

void CodeSpaceComponent::RemoveAttribute(const csp::common::String& ModelPath)
{
    auto ReplicatedAttributes = GetStringMapProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    ReplicatedAttributes.Remove(ModelPath);

    SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), ReplicatedAttributes);
    this->GetParent()->MarkForUpdate();
}

void CodeSpaceComponent::ClearAttributes()
{
    common::Map<common::String, multiplayer::ReplicatedValue> ReplicatedAttributes
        = GetStringMapProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes));
    ReplicatedAttributes.Clear();

    SetProperty(static_cast<uint32_t>(CodeComponentPropertyKeys::Attributes), ReplicatedAttributes);
    this->GetParent()->MarkForUpdate();
}

} // namespace csp::multiplayer
