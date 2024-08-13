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
csp::common::String GetNameFromKey(const csp::common::String& Key)
{
	const std::regex Expression("^(?:[^:]*\:){2}([^:]*)");
	std::smatch Match;
	std::string SearchKey {Key.c_str()};
	const bool Found = std::regex_search(SearchKey, Match, Expression);
	if (Found == true)
	{
		std::string RT				= Match[1];
		csp::common::String ReturnV = RT.c_str();
		return ReturnV;
	}
	else
	{
		CSP_LOG_ERROR_FORMAT("Malformed hotspot Group Key: %s\n", Key);
		return "";
	}
}

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
	auto CB = [Callback](SequenceResult Result)
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
		Group.Name	= GetNameFromKey(Data.Key);


		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};

	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	SequenceSystem->CreateSequence(Key, "GroupId", SpaceId, HotspotIds, CB);
}

void HotspotSequenceSystem::RenameHotspotGroup(const csp::common::String& GroupName,
											   const csp::common::String& NewGroupName,
											   HotspotGroupResultCallback Callback)
{


	auto CB = [Callback](SequenceResult Result)
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
		Group.Name	= GetNameFromKey(Data.Key);

		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};
	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	auto NewKey	 = CreateKey(NewGroupName, SpaceId);
	SequenceSystem->RenameSequence(Key, NewKey, CB);
}

void HotspotSequenceSystem::UpdateHotspotGroup(const csp::common::String& GroupName,
											   const csp::common::Array<csp::common::String>& HotspotIds,
											   HotspotGroupResultCallback Callback)
{
	auto CB = [Callback](SequenceResult Result)
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
		Group.Name	= GetNameFromKey(Data.Key);
		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};

	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	auto Key	 = CreateKey(GroupName, SpaceId);
	SequenceSystem->UpdateSequence(Key, "GroupId", SpaceId, HotspotIds, CB);
}

void HotspotSequenceSystem::GetHotspotGroup(const csp::common::String& GroupName, HotspotGroupResultCallback Callback)
{
	auto CB = [Callback](SequenceResult Result)
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
		Group.Name	= GetNameFromKey(Data.Key);
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
	auto CB = [Callback](SequencesResult Result)
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
			Groups[i].Name	= GetNameFromKey(Data[i].Key);
		}
		HotspotGroupsResult ReturnValue(Groups, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
	};
	auto SpaceId = SpaceSystem->GetCurrentSpace().Id;
	SequenceSystem->GetSequencesByCriteria({}, SpaceId, "GroupId", {SpaceId}, CB);
}

void HotspotSequenceSystem::DeleteHotspotGroup(const csp::common::String& GroupName, HotspotGroupResultCallback Callback)
{
	auto CB = [Callback](NullResult Result)
	{
		HotspotGroup Group;
		HotspotGroupResult ReturnValue(Group, Result.GetResultCode(), Result.GetHttpResultCode());
		// convert SequenceResult To HotspotGroupResultCallback
		Callback(ReturnValue);
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
