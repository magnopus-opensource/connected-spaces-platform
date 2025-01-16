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
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @ingroup Space System
/// @brief Data representation of a Site associated with a Space.
class CSP_API Site
{
public:
    Site() {};

    /** @name Data Values
     *   A Site contains some basic information that define it
     *
     *   @{ */
    csp::common::String Id;
    csp::common::String Name;
    csp::common::String SpaceId;
    GeoLocation Location;
    OlyRotation Rotation;
    /** @} */
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to retrieve Site information.
class CSP_API SiteResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Get the Site data from the Result.
    Site& GetSite();
    /// @brief Get the Site data from the Result.
    const Site& GetSite() const;

private:
    SiteResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Site Site;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of Site information.
class CSP_API SitesCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Site info array being stored.
    /// @return csp::common::Array<Site> : reference to Site info array
    csp::common::Array<Site>& GetSites();

    /// @brief Retrieves the Site info array being stored.
    /// @return csp::common::Array<Site> : reference to Site info array
    const csp::common::Array<Site>& GetSites() const;

private:
    SitesCollectionResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Site> Sites;
};

/// @brief Callback containing a Site and enum result used when adding Site information.
/// @param Result SiteResult : data class containing information on task result/progress
typedef std::function<void(const SiteResult& Result)> SiteResultCallback;
/// @brief Callback containing an array of Site info and enum result used when retrieving a Site info collection.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const SitesCollectionResult& Result)> SitesCollectionResultCallback;

} // namespace csp::systems
