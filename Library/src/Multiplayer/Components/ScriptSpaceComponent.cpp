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
#include "Debug/Logging.h"

namespace csp::multiplayer
{

ScriptSpaceComponent::ScriptSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::ScriptData, Parent)
{
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource)] = "";
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId)]		 = static_cast<int64_t>(0);
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptScope)]	 = static_cast<int64_t>(ScriptScope::Owner);

	Parent->GetScript()->SetScriptSpaceComponent(this);
}

const csp::common::String& ScriptSpaceComponent::GetScriptSource() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void ScriptSpaceComponent::SetScriptSource(const csp::common::String& Value)
{
	// FOUNDATION_LOG_WARN_FORMAT("ScriptSpaceComponent::SetScriptSource '%s'", Value.c_str());

	SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource), Value);
	Parent->GetScript()->OnSourceChanged(Value);
}

int64_t ScriptSpaceComponent::GetOwnerId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return RepVal.GetInt();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0;
}

void ScriptSpaceComponent::SetOwnerId(int64_t OwnerId)
{
	SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId), OwnerId);
}

ScriptScope ScriptSpaceComponent::GetScriptScope() const
{
	if (const auto& RepVal = GetProperty((uint32_t) ScriptComponentPropertyKeys::ScriptScope);
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<ScriptScope>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ScriptScope::Local;
}

void ScriptSpaceComponent::SetScriptScope(ScriptScope Scope)
{
	SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptScope), static_cast<int64_t>(Scope));
}

void ScriptSpaceComponent::SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value)
{
	ComponentBase::SetPropertyFromPatch(Key, Value);

	if (Key == static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource))
	{
		// FOUNDATION_LOG_WARN_FORMAT("ScriptSpaceComponent::SetPropertyFromPatch '%s'", Value.GetString().c_str());

		Parent->GetScript()->Bind();
		Parent->GetScript()->Invoke();
	}
}

void ScriptSpaceComponent::OnRemove()
{
	Parent->GetScript()->Shutdown();
}

} // namespace csp::multiplayer
