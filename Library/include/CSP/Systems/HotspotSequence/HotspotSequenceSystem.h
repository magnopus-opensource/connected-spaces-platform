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

#pragma once


#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/HotspotSequence/HotspotGroup.h"
#include "CSP/Systems/SystemBase.h"



namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{
class SequenceSystem;
class SpaceSystem;
/// @ingroup HotspotSequenceSystem System
/// @brief Public facing system that allows the management of groupings of hotspots in a space.

class CSP_API HotspotSequenceSystem : public SystemBase
{
public:
	CSP_START_IGNORE
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	friend void csp::memory::Delete<HotspotSequenceSystem>(HotspotSequenceSystem* Ptr);
	/** @endcond */
	CSP_END_IGNORE

	HotspotSequenceSystem(csp::systems::SequenceSystem* SequenceSystem, csp::systems::SpaceSystem* SpaceSystem);
	/// @brief Create a Hotspot group
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param HotspotIds csp::common::Array<csp::common::String> : set of Hotspot ids to add to the group
	/// @param Callback HotspotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void CreateHotspotGroup(const csp::common::String& GroupName,
											 const csp::common::Array<csp::common::String>& HotspotIds,
											 HotspotGroupResultCallback Callback);

	/// @brief Rename a Hotspot group
	/// @param CurrentGroupName csp::common::String : The unique grouping name
	/// @param NewGroupName csp::common::String : The unique grouping name
	/// @param Callback HotspotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void
		RenameHotspotGroup(const csp::common::String& GroupName, const csp::common::String& NewGroupName, HotspotGroupResultCallback Callback);

	/// @brief Update a Hotspot group
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param HotspotIds csp::common::Array<csp::common::String> : set of Hotspot ids to replace
	/// @param Callback HotspotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void UpdateHotspotGroup(const csp::common::String& GroupName,
											 const csp::common::Array<csp::common::String>& HotspotIds,
											 HotspotGroupResultCallback Callback);

	/// @brief Get a Hotspot group by name
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param Callback HotspotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetHotspotGroup(const csp::common::String& GroupName, HotspotGroupResultCallback Callback);


	/// @brief Get all Hotspot groups for the current space and logged in user.
	/// @param Callback HotspotGroupsResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetHotspotGroups(HotspotGroupsResultCallback Callback);


	/// @brief Delete a Hotspot group by name
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param Callback NullResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void DeleteHotspotGroup(const csp::common::String& GroupName, NullResultCallback Callback);
	~HotspotSequenceSystem();

	/// @brief This will delete any groups which only contain this item
	/// For any groups which contanin the given item and additional items, it will just update the group by removing the given item.
	/// @param ItemName csp::common::String : An item to update all sequences containing.
	/// @param Callback NullResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void RemoveItemFromGroups(const csp::common::String& ItemName, csp::systems::NullResultCallback Callback);

private:
	HotspotSequenceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	csp::systems::SequenceSystem* SequenceSystem;
	csp::systems::SpaceSystem* SpaceSystem;
};
} // namespace csp::systems
