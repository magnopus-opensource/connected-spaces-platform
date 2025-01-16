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
/// @ingroup HotspotSequence System
/// @brief A basic class abstraction for a Hot Spot Group, including name, and items within the group.
class CSP_API HotspotGroup
{
public:
    csp::common::String Name;
    csp::common::Array<csp::common::String> Items;
};

/// @ingroup HotspotSequence System
/// @brief Data class used to contain information when attempting to get a Hot Spot Group.
class CSP_API HotspotGroupResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const HotspotGroup& GetHotspotGroup() const;
    CSP_NO_EXPORT HotspotGroupResult(const HotspotGroup& Group, csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode)
        , Group(Group) {};
    CSP_NO_EXPORT HotspotGroupResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    HotspotGroupResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    HotspotGroup Group;
};

/// @ingroup HotspotSequence System
/// @brief Data class used to contain information when attempting to get a Hot Spot Group.
class CSP_API HotspotGroupsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const csp::common::Array<HotspotGroup>& GetHotspotGroups() const;
    CSP_NO_EXPORT HotspotGroupsResult(const csp::common::Array<HotspotGroup>& Groups, csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode)
        , Groups(Groups) {};
    CSP_NO_EXPORT HotspotGroupsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    HotspotGroupsResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<HotspotGroup> Groups;
};

/// @brief Callback containing array of Hotspots.
/// @param Result HotspotGroupResult : result class
typedef std::function<void(const csp::systems::HotspotGroupResult& Result)> HotspotGroupResultCallback;

/// @brief Callback containing array of HotspotGroups.
/// @param Result HotspotGroupsResult : result class
typedef std::function<void(const csp::systems::HotspotGroupsResult& Result)> HotspotGroupsResultCallback;
} // namespace csp::systems
