#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Spatial/SpatialDataTypes.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

/// @ingroup Space System
/// @brief Data representation of a Site associated with a Space.
class OLY_API Site
{
public:
    Site() {};

    /** @name Data Values
     *   A Site contains some basic information that define it
     *
     *   @{ */
    oly_common::String Id;
    oly_common::String Name;
    oly_common::String SpaceId;
    GeoLocation Location;
    OlyRotation Rotation;
    /** @} */
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to retrieve Site information.
class OLY_API SiteResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    Site& GetSite();
    const Site& GetSite() const;

private:
    SiteResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    Site Site;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of Site information.
class OLY_API SitesCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Site info array being stored.
    /// @return oly_common::Array<Site> : reference to Site info array
    oly_common::Array<Site>& GetSites();

    /// @brief Retrieves the Site info array being stored.
    /// @return oly_common::Array<Site> : reference to Site info array
    const oly_common::Array<Site>& GetSites() const;

private:
    SitesCollectionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<Site> Sites;
};

/// @brief Callback containing a Site and enum result used when adding Site information.
/// @param Result SiteResult : data class containing information on task result/progress
typedef std::function<void(const SiteResult& Result)> SiteResultCallback;
/// @brief Callback containing an array of Site info and enum result used when retrieving a Site info collection.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const SitesCollectionResult& Result)> SitesCollectionResultCallback;

} // namespace oly_systems
