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
#include "CSP/Multiplayer/Components/LocalScriptSpaceComponent.h"

#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

namespace csp::multiplayer
{

LocalScriptSpaceComponent::LocalScriptSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::ScriptData, Parent)
{
    Properties[static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptAssetId)] = "";
    Properties[static_cast<uint32_t>(LocalScriptComponentPropertyKeys::OwnerId)] = static_cast<int64_t>(0);
    // Properties[static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptScope)] = static_cast<int64_t>(ScriptScope::Owner);

    // Parent->GetScript()->SetLocalScriptSpaceComponent(this);
}

const csp::common::String& LocalScriptSpaceComponent::GetScriptAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptAssetId));
}

void LocalScriptSpaceComponent::SetScriptAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptAssetId), Value);
}

// const csp::common::String& LocalScriptSpaceComponent::GetScriptSource() const
// {
//     return GetStringProperty(static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptSource));
// }

// void LocalScriptSpaceComponent::SetScriptSource(const csp::common::String& Value)
// {
//     // CSP_LOG_WARN_FORMAT("LocalScriptSpaceComponent::SetScriptSource '%s'", Value.c_str());

//     SetProperty(static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptSource), Value);
//     Parent->GetScript()->OnSourceChanged(Value);
// }

int64_t LocalScriptSpaceComponent::GetOwnerId() const { return GetIntegerProperty(static_cast<uint32_t>(LocalScriptComponentPropertyKeys::OwnerId)); }

void LocalScriptSpaceComponent::SetOwnerId(int64_t OwnerId) { SetProperty(static_cast<uint32_t>(LocalScriptComponentPropertyKeys::OwnerId), OwnerId); }

void LocalScriptSpaceComponent::SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value)
{
    ComponentBase::SetPropertyFromPatch(Key, Value);

    if (Key == static_cast<uint32_t>(LocalScriptComponentPropertyKeys::ScriptAssetId))
    {
        // CSP_LOG_WARN_FORMAT("LocalScriptSpaceComponent::SetPropertyFromPatch '%s'", Value.GetString().c_str());

        Parent->GetScript()->Bind();
        Parent->GetScript()->Invoke();
    }
}

void LocalScriptSpaceComponent::OnRemove() { Parent->GetScript()->Shutdown(); }

} // namespace csp::multiplayer
