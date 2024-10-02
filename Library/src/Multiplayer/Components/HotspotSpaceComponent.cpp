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
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

namespace
{
void DeleteSequences(const std::vector<systems::Sequence>& Sequences, csp::systems::NullResultCallback Callback)
{
	systems::SequenceSystem* SequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

	// Remove necessary sequences
	common::Array<common::String> DeletionKeys(Sequences.size());

	for (size_t i = 0; i < Sequences.size(); ++i)
	{
		DeletionKeys[i] = Sequences[i].Key;
	}

	auto DeleteCallback = [Callback, SequenceSystem](const csp::systems::NullResult& DeleteResult)
	{
		if (DeleteResult.GetResultCode() == systems::EResultCode::InProgress)
		{
			return;
		}

		Callback(DeleteResult);
	};

	SequenceSystem->DeleteSequences(DeletionKeys, DeleteCallback);
}

void UpdateSequences(const std::vector<systems::Sequence>& Sequences,
					 const csp::common::String& ItemToRemove,
					 csp::systems::NullResultCallback Callback)
{
	systems::SequenceSystem* SequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();

	for (const auto& Sequence : Sequences)
	{
		// Remove key from items array
		common::Array<common::String> Items = Sequence.Items;
		auto ItemsList						= Items.ToList();
		ItemsList.RemoveItem(ItemToRemove);
		Items = ItemsList.ToArray();

		auto UpdateCB = [Callback](const systems::SequenceResult& Result)
		{
			if (Result.GetResultCode() == systems::EResultCode::InProgress)
			{
				return;
			}

			Callback(systems::NullResult(Result.GetResultCode(), Result.GetHttpResultCode()));
		};

		SequenceSystem->UpdateSequence(Sequence.Key, Sequence.ReferenceType, Sequence.ReferenceId, Items, Sequence.MetaData, UpdateCB);
	}
}

void ProcessAssociatedSequences(const csp::common::Array<systems::Sequence>& Sequences,
								const csp::common::String& ItemToRemove,
								csp::systems::NullResultCallback Callback)
{
	std::vector<systems::Sequence> SequencesToDelete;
	std::vector<systems::Sequence> SequencesToUpdate;
	SequencesToDelete.reserve(Sequences.Size());
	SequencesToUpdate.reserve(Sequences.Size());

	for (size_t i = 0; i < Sequences.Size(); ++i)
	{
		if (Sequences[i].Items.Size() == 1)
		{
			// This is the only item in the sequence, so delete the sequence
			SequencesToDelete.push_back(Sequences[i]);
		}
		else
		{
			// There are other items in this sequence, so only remove this item
			SequencesToUpdate.push_back(Sequences[i]);
		}
	}

	// Wait for all callbacks before returning
	int CallbackCount	 = Sequences.Size();
	int CurrentCallbacks = 0;

	auto CB = [Callback, &CallbackCount, &CurrentCallbacks](const systems::NullResult& Res)
	{
		CurrentCallbacks++;

		if (CurrentCallbacks >= CallbackCount)
		{
			Callback(Res);
		}
	};

	DeleteSequences(SequencesToDelete, CB);
	UpdateSequences(SequencesToUpdate, ItemToRemove, CB);
}
} // namespace

HotspotSpaceComponent::HotspotSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Hotspot, Parent)
{
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::Position)]		= csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::Rotation)]		= csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED)] = "";
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint)] = true;
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint)]	= false;
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsVisible)]		= true;
	Properties[static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible)]		= true;

	SetScriptInterface(CSP_NEW HotspotSpaceComponentScriptInterface(this));
}

const csp::common::String& HotspotSpaceComponent::GetName() const
{
	return GetStringProperty(static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED));
}

void HotspotSpaceComponent::SetName(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Name_DEPRECATED), Value);
}

bool HotspotSpaceComponent::GetIsTeleportPoint() const
{
	return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint));
}

void HotspotSpaceComponent::SetIsTeleportPoint(bool Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsTeleportPoint), Value);
}

bool HotspotSpaceComponent::GetIsSpawnPoint() const
{
	return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint));
}

void HotspotSpaceComponent::SetIsSpawnPoint(bool Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsSpawnPoint), Value);
}

csp::common::String HotspotSpaceComponent::GetUniqueComponentId() const
{
	csp::common::String UniqueComponentId = std::to_string(Parent->GetId()).c_str();
	UniqueComponentId += ":";
	UniqueComponentId += std::to_string(Id).c_str();

	return UniqueComponentId;
}

/* IPositionComponent */

const csp::common::Vector3& HotspotSpaceComponent::GetPosition() const
{
	return GetVector3Property(static_cast<uint32_t>(HotspotPropertyKeys::Position));
}

void HotspotSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Position), Value);
}

/* IRotationComponent */

const csp::common::Vector4& HotspotSpaceComponent::GetRotation() const
{
	return GetVector4Property(static_cast<uint32_t>(HotspotPropertyKeys::Rotation));
}

void HotspotSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::Rotation), Value);
}

/* IVisibleComponent */

bool HotspotSpaceComponent::GetIsVisible() const
{
	return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVisible));
}

void HotspotSpaceComponent::SetIsVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsVisible), Value);
}

bool HotspotSpaceComponent::GetIsARVisible() const
{
	return GetBooleanProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible));
}

void HotspotSpaceComponent::SetIsARVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(HotspotPropertyKeys::IsARVisible), Value);
}

void HotspotSpaceComponent::RemoveSequences(csp::systems::NullResultCallback Callback)
{
	systems::SequenceSystem* SequenceSystem = systems::SystemsManager::Get().GetSequenceSystem();
	systems::SpaceSystem* SpaceSystem		= systems::SystemsManager::Get().GetSpaceSystem();

	csp::common::String Id = GetUniqueComponentId();

	auto GetSequencesCallback = [this, Callback, Id](const systems::SequencesResult& SequencesResult)
	{
		if (SequencesResult.GetResultCode() == systems::EResultCode::InProgress)
		{
			return;
		}

		ProcessAssociatedSequences(SequencesResult.GetSequences(), Id, Callback);
	};

	// Find all sequences containing this name
	SequenceSystem->GetAllSequencesContainingItems({Id}, "GroupId", {SpaceSystem->GetCurrentSpace().Id}, GetSequencesCallback);
}

void HotspotSpaceComponent::OnLocalDelete()
{
	auto CB = [](const csp::systems::NullResult& Result)
	{

	};

	RemoveSequences(CB);
}

} // namespace csp::multiplayer
