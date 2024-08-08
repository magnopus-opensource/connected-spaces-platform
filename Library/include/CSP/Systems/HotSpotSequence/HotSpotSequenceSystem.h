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
#include "CSP/Systems/HotSpotSequence/HotSpotGroup.h"
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
/// @ingroup HotSpotSequenceSystem System
/// @brief Public facing system that allows the management of groupings of hotspots in a space.

class CSP_API HotSpotSequenceSystem : public SystemBase
{
public:
	CSP_START_IGNORE
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	friend void csp::memory::Delete<HotSpotSequenceSystem>(HotSpotSequenceSystem* Ptr);
	/** @endcond */
	CSP_END_IGNORE

	HotSpotSequenceSystem(csp::systems::SequenceSystem* SequenceSystem, csp::systems::SpaceSystem* SpaceSystem);
	/// @brief Create a Hotspot group
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param HotspotIds csp::common::Array<csp::common::String> : set of Hotspot ids to add to the group
	/// @param Callback HotSpotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void
		CreateHotspotGroup(csp::common::String GroupName, csp::common::Array<csp::common::String> HotspotIds, HotSpotGroupResultCallback Callback);

	/// @brief Update a Hotspot group
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param HotspotIds csp::common::Array<csp::common::String> : set of Hotspot ids to add to the group
	/// @param Callback HotSpotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void
		UpdateHotspotGroup(csp::common::String GroupName, csp::common::Array<csp::common::String> HotspotIds, HotSpotGroupResultCallback Callback);

	/// @brief Get a Hotspot group by name
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param Callback HotSpotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetHotspotGroup(csp::common::String GroupName, HotSpotGroupResultCallback Callback);


	/// @brief Get all Hotspot groups for the current space and logged in user.
	/// @param Callback HotSpotGroupsResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetHotspotGroups(HotSpotGroupsResultCallback Callback);


	/// @brief Get a Hotspot group by name
	/// @param GroupName csp::common::String : The unique grouping name
	/// @param Callback HotSpotGroupResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void DeleteHotspotGroup(csp::common::String GroupName, HotSpotGroupResultCallback Callback);
	~HotSpotSequenceSystem();

private:
	HotSpotSequenceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	csp::systems::SequenceSystem* sequenceSystem;
	csp::systems::SpaceSystem* spaceSystem;
};
} // namespace csp::systems
