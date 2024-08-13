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

#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"

#include "CSP/Systems/HotspotSequence/HotspotGroup.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Debug/Logging.h"

#include <regex>
#include <string>

namespace csp::systems
{

namespace
{
csp::common::String CreateKey(const csp::common::String& Key, const csp::common::String& SpaceId)
{
	return "Hotspots:" + SpaceId + ":" + Key;
}

} // namespace
HotspotSequenceSystem::HotspotSequenceSystem(csp::systems::SequenceSystem* SequenceSystem, csp::systems::SpaceSystem* SpaceSystem)
{
	this->SequenceSystem = SequenceSystem;
	this->SpaceSystem	 = SpaceSystem;
}

void HotspotSequenceSystem::CreateHotspotGroup(const csp::common::String& GroupName,
											   const csp::common::Array<csp::common::String>& HotspotIds,
											   HotspotGroupResultCallback Callback)
{
	auto CB = [Callback](const SequenceResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
			// convert SequenceResult To HotspotGroupResultCallback
			Callback(ReturnValue);
			return;
		}

		auto Data = Result.GetSequence();
		HotspotGroup Group;
		Group.Items = Data.Items;
		Group.Name	= Data.MetaData["Name"];


		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};

	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	csp::common::Map<csp::common::String, csp::common::String> MetaData;
	MetaData["Name"] = GroupName;
	SequenceSystem->CreateSequence(Key, "GroupId", SpaceId, HotspotIds, MetaData, CB);
}

void HotspotSequenceSystem::RenameHotspotGroup(const csp::common::String& GroupName,
											   const csp::common::String& NewGroupName,
											   HotspotGroupResultCallback Callback)
{
	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	auto NewKey	 = CreateKey(NewGroupName, SpaceId);
	auto SQ		 = this->SequenceSystem;

	auto CB = [Callback, SQ, NewKey, NewGroupName, SpaceId](const SequenceResult& Result)
	{
		if (Result.GetResultCode() != csp::systems::EResultCode::Success)
		{
			HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
			// convert SequenceResult To HotspotGroupResultCallback
			Callback(ReturnValue);
			return;
		}

		auto UpdateCB = [Callback, SQ, NewKey](const SequenceResult& Result)
		{
			if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
			{
				HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
				// convert SequenceResult To HotspotGroupResultCallback
				Callback(ReturnValue);
				return;
			}
			auto Data = Result.GetSequence();
			HotspotGroup Group;
			Group.Items = Data.Items;
			Group.Name	= Data.MetaData["Name"];
			HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
			// convert SequenceResult To HotspotGroupResultCallback
			Callback(ReturnValue);
		};
		auto MetaData	 = Result.GetSequence().MetaData;
		MetaData["Name"] = NewGroupName;
		SQ->UpdateSequence(NewKey, "GroupId", SpaceId, Result.GetSequence().Items, MetaData, UpdateCB);
	};


	SequenceSystem->RenameSequence(Key, NewKey, CB);
}

void HotspotSequenceSystem::UpdateHotspotGroup(const csp::common::String& GroupName,
											   const csp::common::Array<csp::common::String>& HotspotIds,
											   HotspotGroupResultCallback Callback)
{
	auto CB = [Callback](const SequenceResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
			// convert SequenceResult To HotspotGroupResultCallback
			Callback(ReturnValue);
			return;
		}
		auto Data = Result.GetSequence();
		HotspotGroup Group;
		Group.Items = Data.Items;
		Group.Name	= Data.MetaData["Name"];
		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};

	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	csp::common::Map<csp::common::String, csp::common::String> MetaData;
	MetaData["Name"] = GroupName;
	SequenceSystem->UpdateSequence(Key, "GroupId", SpaceId, HotspotIds, MetaData, CB);
}

void HotspotSequenceSystem::GetHotspotGroup(const csp::common::String& GroupName, HotspotGroupResultCallback Callback)
{
	auto CB = [Callback](const SequenceResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			HotspotGroupResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
			// convert SequenceResult To HotspotGroupResultCallback
			Callback(ReturnValue);
			return;
		}
		auto Data = Result.GetSequence();
		HotspotGroup Group;
		Group.Items = Data.Items;
		Group.Name	= Data.MetaData["Name"];
		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};
	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	SequenceSystem->GetSequence(Key, CB);
}

void HotspotSequenceSystem::GetHotspotGroups(HotspotGroupsResultCallback Callback)
{
	auto CB = [Callback](const SequencesResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			HotspotGroupsResult ReturnValue(Result.GetResultCode(), Result.GetHttpResultCode());
			// convert SequenceResult To HotspotGroupResultCallback
			Callback(ReturnValue);
			return;
		}
		auto Data = Result.GetSequences();

		csp::common::Array<HotspotGroup> Groups(Data.Size());

		for (size_t i = 0; i < Data.Size(); i++)
		{
			Groups[i].Items = Data[i].Items;
			Groups[i].Name	= Data[i].MetaData["Name"];
		}
		HotspotGroupsResult ReturnValue(Groups, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};
	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	csp::common::Map<csp::common::String, csp::common::String> MetaData;
	SequenceSystem->GetSequencesByCriteria({}, SpaceId, "GroupId", {SpaceId}, MetaData, CB);
}

void HotspotSequenceSystem::DeleteHotspotGroup(const csp::common::String& GroupName, NullResultCallback Callback)
{
	auto CB = [Callback](const NullResult& Result)
	{
		Callback(Result);
	};

	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	SequenceSystem->DeleteSequences({Key}, CB);
}
HotspotSequenceSystem::~HotspotSequenceSystem()
{
	SpaceSystem	   = nullptr;
	SequenceSystem = nullptr;
}
HotspotSequenceSystem::HotspotSequenceSystem()
{
	SpaceSystem	   = nullptr;
	SequenceSystem = nullptr;
}
} // namespace csp::systems
