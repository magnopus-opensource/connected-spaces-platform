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

#include "CSP/Systems/HotSpotSequence/HotSpotGroup.h"

#include "Services/AggregationService/Api.h"

namespace csp::systems
{
const HotSpotGroup& csp::systems::HotSpotGroupResult::GetHotSpotGroup() const
{
	return Group;
}

void csp::systems::HotSpotGroupResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);
}


const csp::common::Array<HotSpotGroup>& HotSpotGroupsResult::GetHotSpotGroups() const
{
	return Groups;
}
void HotSpotGroupsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);
}
} // namespace csp::systems
