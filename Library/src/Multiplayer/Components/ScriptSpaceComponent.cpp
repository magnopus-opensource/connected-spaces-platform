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
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource)]					  = "";
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId)]							  = static_cast<int64_t>(0);
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptScope)]						  = static_cast<int64_t>(ScriptScope::Owner);
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::ExternalResourceAssetId)]			  = "";
	Properties[static_cast<uint32_t>(ScriptComponentPropertyKeys::ExternalResourceAssetCollectionId)] = "";

	Parent->GetScript()->SetScriptSpaceComponent(this);
}

const csp::common::String& ScriptSpaceComponent::GetExternalResourceAssetId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ExternalResourceAssetId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void ScriptSpaceComponent::SetExternalResourceAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ExternalResourceAssetId), Value);
}

const csp::common::String& ScriptSpaceComponent::GetExternalResourceAssetCollectionId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ExternalResourceAssetCollectionId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void ScriptSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ExternalResourceAssetCollectionId), Value);
}

const csp::common::String& ScriptSpaceComponent::GetScriptSource() const
{
	if (IsPrototypeBacked)
	{
		CSP_LOG_MSG(systems::LogLevel::Log, "Prototype backed Script Source being used.");

		return ScriptSource;
	}

	CSP_LOG_MSG(systems::LogLevel::Log, "Replicated property backed Script Source being used.");

	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void ScriptSpaceComponent::SetScriptSource(const csp::common::String& Value)
{
	// CSP_LOG_WARN_FORMAT("ScriptSpaceComponent::SetScriptSource '%s'", Value.c_str());

	SetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::ScriptSource), Value);
	Parent->GetScript()->OnSourceChanged(Value);
}

void ScriptSpaceComponent::SetComponentScriptSource(const csp::common::String& Value)
{
	IsPrototypeBacked = true;
	ScriptSource	  = Value;
}

int64_t ScriptSpaceComponent::GetOwnerId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ScriptComponentPropertyKeys::OwnerId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return RepVal.GetInt();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
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

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
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
		// CSP_LOG_WARN_FORMAT("ScriptSpaceComponent::SetPropertyFromPatch '%s'", Value.GetString().c_str());

		Parent->GetScript()->Bind();
		Parent->GetScript()->Invoke();
	}
}

void ScriptSpaceComponent::OnRemove()
{
	Parent->GetScript()->Shutdown();
}

} // namespace csp::multiplayer
