#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Spatial/SpatialDataTypes.h"

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

enum class EPointOfInterestType
{
    DEFAULT
};

/// @ingroup Point Of Interest System
/// @brief Data representation of a Point Of Interest
class OLY_API PointOfInterest
{
public:
    PointOfInterest();

    /** @name Data Values
     *   A Point of Interest contains some basic information that define it
     *
     *   @{ */
    oly_common::String Id;
    oly_common::String CreatedBy;
    oly_common::String CreatedAt;
    oly_common::Map<oly_common::String, oly_common::String> Title;
    oly_common::Map<oly_common::String, oly_common::String> Description;
    oly_common::String Name;
    EPointOfInterestType Type;
    oly_common::Array<oly_common::String> Tags;
    oly_common::String Owner;
    oly_systems::GeoLocation Location;
    oly_common::String AssetCollectionId;
    /** @} */
};

/// @ingroup Point Of Interest System
/// @brief Data class used to contain information after creating or retrieving a POI.
class OLY_API POIResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    PointOfInterest& GetPointOfInterest();
    const PointOfInterest& GetPointOfInterest() const;

private:
    POIResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    PointOfInterest POI;
};

/// @ingroup Point Of Interest System
/// @brief Data class used to contain information when attempting to get an array of POIs.
class OLY_API POICollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the POIs array being stored.
    /// @return oly_common::Array<PointOfInterest> : reference to POIs array
    oly_common::Array<PointOfInterest>& GetPOIs();

    /// @brief Retrieves the POIs array being stored.
    /// @return oly_common::Array<PointOfInterest> : reference to POIs array
    const oly_common::Array<PointOfInterest>& GetPOIs() const;

private:
    POICollectionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<PointOfInterest> POIs;
};

/// @brief Callback containing a Point Of Interest and enum result used when creating or retrieving a POI.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const POIResult& Result)> POIResultCallback;

/// @brief Callback containing an array of Points Of Interest and enum result used when retrieving a POI collection.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const POICollectionResult& Result)> POICollectionResultCallback;
} // namespace oly_systems
