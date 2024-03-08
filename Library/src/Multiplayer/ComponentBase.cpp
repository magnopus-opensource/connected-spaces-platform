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
#include "CSP/Multiplayer/ComponentBase.h"

#include "CSP/Common/List.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"

namespace csp::multiplayer
{

static const ReplicatedValue InvalidValue = ReplicatedValue();

ComponentBase::ComponentBase() : Id(0), Type(ComponentType::Invalid), Parent(nullptr), ScriptInterface(nullptr)
{
}

ComponentBase::ComponentBase(ComponentType Type, SpaceEntity* Parent) : Id(0), Type(Type), Parent(Parent), ScriptInterface(nullptr)
{
}

ComponentBase::~ComponentBase()
{
	if (ScriptInterface)
	{
		CSP_DELETE(ScriptInterface);
	}
}

uint16_t ComponentBase::GetId()
{
	return Id;
}

ComponentType ComponentBase::GetComponentType()
{
	return Type;
}

const csp::common::Map<uint32_t, ReplicatedValue>* ComponentBase::GetProperties()
{
	return &Properties;
}

const ReplicatedValue& ComponentBase::GetProperty(uint32_t Key) const
{
	if (Properties.HasKey(Key))
	{
		return Properties[Key];
	}

	CSP_LOG_ERROR_FORMAT("No Property with this key: %d", Key);

	return InvalidValue;
}

void ComponentBase::SetProperty(uint32_t Key, const ReplicatedValue& Value)
{
	if (Properties.HasKey(Key) && Value.GetReplicatedValueType() != Properties[Key].GetReplicatedValueType())
	{
		CSP_LOG_ERROR_FORMAT("ValueType is unexpected. Expected: %d Received: %d",
							 static_cast<uint32_t>(Properties[Key].GetReplicatedValueType()),
							 static_cast<uint32_t>(Value.GetReplicatedValueType()));
	}

	// If the entity is not owned by us, and not a transferable entity, it is not allowed to modify the entity.
	if (!GetParent()->IsModifiable())
	{
		CSP_LOG_ERROR_FORMAT("Error: Update attempted on a non-owned entity that is marked as non-transferable. Skipping update. Entity name: %s",
							 GetParent()->GetName().c_str());
		return;
	}

	/*DirtyProperties.Remove(Key);

	if (Properties[Key] != Value)
	{
		DirtyProperties[Key] = Value;

		Parent->AddDirtyComponent(this);
	}*/

	if (!Properties.HasKey(Key) || Properties[Key] != Value)
	{
		Properties[Key] = Value;

		Parent->AddDirtyComponent(this);
		Parent->OnPropertyChanged(this, Key);
	}
}

void ComponentBase::RemoveProperty(uint32_t Key)
{
	// DirtyProperties.Remove(Key);
	Properties.Remove(Key);

	Parent->AddDirtyComponent(this);
}

void ComponentBase::SetProperties(const csp::common::Map<uint32_t, ReplicatedValue>& Value)
{
	Properties = Value;
}

void ComponentBase::SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value)
{
	Properties[Key] = Value;
}

void ComponentBase::OnRemove()
{
}

SpaceEntity* ComponentBase::GetParent()
{
	return Parent;
}

void ComponentBase::SetScriptInterface(ComponentScriptInterface* InScriptInterface)
{
	ScriptInterface = InScriptInterface;
}


ComponentScriptInterface* ComponentBase::GetScriptInterface()
{
	return ScriptInterface;
}

void ComponentBase::SubscribeToPropertyChange(uint32_t PropertyKey, csp::common::String Message)
{
	GetParent()->GetScript()->SubscribeToPropertyChange(GetId(), PropertyKey, Message);
}

void ComponentBase::RegisterActionHandler(const csp::common::String& InAction, EntityActionHandler ActionHandler)
{
	if (!ActionMap.HasKey(InAction.c_str()))
	{
		ActionMap[InAction.c_str()] = ActionHandler;
	}
	else
	{
		// Already registered
		CSP_LOG_ERROR_FORMAT("Action %s already registered\n", InAction.c_str());
	}
}

void ComponentBase::UnregisterActionHandler(const csp::common::String& InAction)
{
	if (ActionMap.HasKey(InAction.c_str()))
	{
		ActionMap.Remove(InAction.c_str());
	}
	else
	{
		CSP_LOG_ERROR_FORMAT("Action %s not found\n", InAction.c_str());
	}
}

void ComponentBase::InvokeAction(const csp::common::String& InAction, const csp::common::String& InActionParams)
{
	if (ActionMap.HasKey(InAction.c_str()))
	{
		EntityActionHandler ActionHandler = ActionMap[InAction.c_str()];
		ActionHandler(this, InAction, InActionParams);
	}
}

} // namespace csp::multiplayer
