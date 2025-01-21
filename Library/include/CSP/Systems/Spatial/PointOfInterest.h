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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

enum class EPointOfInterestType
{
    DEFAULT,
    SPACE
};

/// @ingroup Point Of Interest System
/// @brief Data representation of a Point Of Interest
class CSP_API PointOfInterest
{
public:
    PointOfInterest();

    /** @name Data Values
     *   A Point of Interest contains some basic information that define it
     *
     *   @{ */
    csp::common::String Id;
    csp::common::String CreatedBy;
    csp::common::String CreatedAt;
    csp::common::Map<csp::common::String, csp::common::String> Title;
    csp::common::Map<csp::common::String, csp::common::String> Description;
    csp::common::String Name;
    EPointOfInterestType Type;
    csp::common::Array<csp::common::String> Tags;
    csp::common::String Owner;
    csp::systems::GeoLocation Location;
    csp::common::String AssetCollectionId;
    csp::common::String SpaceId;
    /** @} */
};

/// @ingroup Point Of Interest System
/// @brief Data class used to contain information after creating or retrieving a POI.
class CSP_API POIResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    PointOfInterest& GetPointOfInterest();
    const PointOfInterest& GetPointOfInterest() const;

private:
    POIResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    PointOfInterest POI;
};

/// @ingroup Point Of Interest System
/// @brief Data class used to contain information when attempting to get an array of POIs.
class CSP_API POICollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the POIs array being stored.
    /// @return csp::common::Array<PointOfInterest> : reference to POIs array
    csp::common::Array<PointOfInterest>& GetPOIs();

    /// @brief Retrieves the POIs array being stored.
    /// @return csp::common::Array<PointOfInterest> : reference to POIs array
    const csp::common::Array<PointOfInterest>& GetPOIs() const;

private:
    POICollectionResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<PointOfInterest> POIs;
};

/// @brief Callback containing a Point Of Interest and enum result used when creating or retrieving a POI.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const POIResult& Result)> POIResultCallback;

/// @brief Callback containing an array of Points Of Interest and enum result used when retrieving a POI collection.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const POICollectionResult& Result)> POICollectionResultCallback;
} // namespace csp::systems
