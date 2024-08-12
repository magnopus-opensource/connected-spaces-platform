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

#include "CSP/Common/Array.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::systems
{
/// @ingroup HotSpotSequence System
/// @brief A basic class abstraction for a Hot Spot Group, including name, and items within the group.
class CSP_API HotSpotGroup
{
public:
	csp::common::String Name;
	csp::common::Array<csp::common::String> Items;
};


/// @ingroup HotSpotSequence System
/// @brief Data class used to contain information when attempting to get a Hot Spot Group.
class CSP_API HotSpotGroupResult : public csp::systems::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	const HotSpotGroup& GetHotSpotGroup() const;
	CSP_NO_EXPORT HotSpotGroupResult(HotSpotGroup& group, csp::systems::EResultCode ResCode, uint16_t HttpResCode)
		: csp::systems::ResultBase(ResCode, HttpResCode), Group(group) {};
	CSP_NO_EXPORT HotSpotGroupResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode) : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
	HotSpotGroupResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	HotSpotGroup Group;
};

/// @ingroup HotSpotSequence System
/// @brief Data class used to contain information when attempting to get a Hot Spot Group.
class CSP_API HotSpotGroupsResult : public csp::systems::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	const csp::common::Array<HotSpotGroup>& GetHotSpotGroups() const;
	CSP_NO_EXPORT HotSpotGroupsResult(csp::common::Array<HotSpotGroup>& groups, csp::systems::EResultCode ResCode, uint16_t HttpResCode)
		: csp::systems::ResultBase(ResCode, HttpResCode), Groups(groups) {};
	CSP_NO_EXPORT HotSpotGroupsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode) : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
	HotSpotGroupsResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	csp::common::Array<HotSpotGroup> Groups;
};

/// @brief Callback containing array of Hotspots.
/// @param Result HotSpotGroupResult : result class
typedef std::function<void(const csp::systems::HotSpotGroupResult& Result)> HotSpotGroupResultCallback;

/// @brief Callback containing array of HotspotGroups.
/// @param Result HotSpotGroupsResult : result class
typedef std::function<void(const csp::systems::HotSpotGroupsResult& Result)> HotSpotGroupsResultCallback;
} // namespace csp::systems
