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

#include "CSP/Systems/HotSpotSequence/HotSpotSequenceSystem.h"

#include "CSP/Systems/HotSpotSequence/HotSpotGroup.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"

#include <string>

namespace csp::systems
{
HotSpotSequenceSystem::HotSpotSequenceSystem(csp::systems::SequenceSystem* SequenceSystem, csp::systems::SpaceSystem* SpaceSystem)
{
	this->sequenceSystem = SequenceSystem;
	this->spaceSystem	 = SpaceSystem;
}

void HotSpotSequenceSystem::CreateHotspotGroup(csp::common::String GroupName,
											   csp::common::Array<csp::common::String> HotspotIds,
											   HotSpotGroupResultCallback Callback)
{
	auto spaceID			= spaceSystem->GetCurrentSpace().Id;
	csp::common::String key = spaceID + ":" + GroupName;

	auto CB = [Callback, key](SequenceResult result)
	{
		auto data = result.GetSequence();
		HotSpotGroup group;
		group.Items = data.Items;
		group.Name	= data.Key;
		HotSpotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
		returnValue.setHotSpotGroup(group);
		// convert SequenceResult To HotSpotGroupResultCallback
		Callback(returnValue);
	};

	sequenceSystem->CreateSequence(key, "GroupId", spaceID, HotspotIds, CB);
}

void HotSpotSequenceSystem::UpdateHotspotGroup(csp::common::String GroupName,
											   csp::common::Array<csp::common::String> HotspotIds,
											   HotSpotGroupResultCallback Callback)
{
	auto spaceID			= spaceSystem->GetCurrentSpace().Id;
	csp::common::String key = GroupName + spaceID;

	auto CB = [Callback, key](SequenceResult result)
	{
		HotSpotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
		HotSpotGroup group;
		group.Items = result.GetSequence().Items;
		group.Name	= result.GetSequence().Key;
		returnValue.setHotSpotGroup(group);
		// convert SequenceResult To HotSpotGroupResultCallback
		Callback(returnValue);
	};

	sequenceSystem->UpdateSequence(key, "GroupId", spaceID, HotspotIds, CB);
}

void HotSpotSequenceSystem::GetHotspotGroup(csp::common::String GroupName, HotSpotGroupResultCallback Callback)
{
	auto spaceID = spaceSystem->GetCurrentSpace().Id;

	csp::common::String key = spaceID + ":" + GroupName;

	auto CB = [Callback, key](SequenceResult result)
	{
		auto data = result.GetSequence();
		HotSpotGroup group;

		group.Items = data.Items;
		group.Name	= data.Key;

		HotSpotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
		returnValue.setHotSpotGroup(group);
		// convert SequenceResult To HotSpotGroupResultCallback
		Callback(returnValue);
	};

	sequenceSystem->GetSequence(key, CB);
}

void HotSpotSequenceSystem::GetHotspotGroups(HotSpotGroupsResultCallback Callback)
{
	auto spaceID = spaceSystem->GetCurrentSpace().Id;
	auto CB		 = [Callback](SequencesResult result)
	{
		auto data = result.GetSequences();
		csp::common::Array<HotSpotGroup> groups {data.Size()};
		for (size_t i = 0; i < data.Size(); i++)
		{
			groups[i].Items = data[i].Items;
			groups[i].Name	= data[i].Key;
		}

		HotSpotGroupsResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
		returnValue.setHotSpotGroups(groups);
		// convert SequenceResult To HotSpotGroupResultCallback
		Callback(returnValue);
	};
	// funky
	sequenceSystem->GetSequencesByCriteria({}, spaceID, "GroupId", {spaceID}, CB);
}

void HotSpotSequenceSystem::DeleteHotspotGroup(csp::common::String GroupName, HotSpotGroupResultCallback Callback)
{
	auto spaceID			= spaceSystem->GetCurrentSpace().Id;
	csp::common::String key = spaceID + ":" + GroupName;

	auto CB = [Callback, key](NullResult result)
	{
		HotSpotGroupResult returnValue(result.GetResultCode(), result.GetHttpResultCode());
		// convert SequenceResult To HotSpotGroupResultCallback
		Callback(returnValue);
	};

	sequenceSystem->DeleteSequences({key}, CB);
}
HotSpotSequenceSystem::~HotSpotSequenceSystem()
{
	spaceSystem	   = nullptr;
	sequenceSystem = nullptr;
}
HotSpotSequenceSystem::HotSpotSequenceSystem()
{
	spaceSystem	   = nullptr;
	sequenceSystem = nullptr;
}
} // namespace csp::systems
