#pragma once

#include "Olympus/Common/Optional.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/Spaces/Site.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/Spatial/PointOfInterest.h"
#include "Olympus/Systems/SystemBase.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

class Space;
class AssetCollection;

/// @ingroup Point of Interest System
/// @brief Public facing system that allows interfacing with CHS's concept of a Point of Interest.
/// Offers methods for creating and deleting POIs.
class OLY_API OLY_NO_DISPOSE PointOfInterestSystem : public SystemBase
{

public:
    ~PointOfInterestSystem();

    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */

    /// @brief Creates a new Point of Interest.
    /// @param Title oly_common::String : title for the new POI
    /// @param Description oly_common::String : description for the new POI
    /// @param Name oly_common::String : name for the new POI
    /// @param Tags oly_common::Array<oly_common::String> : optional array of tags to be added to the new POI
    /// @param Type EPointOfInterestType : type of the new POI
    /// @param Owner oly_common::String : owner of the new POI
    /// @param Location oly_systems::GeoLocation : latitude and longitude coordinates of the new POI
    /// @param AssetCollection oly_systems::AssetCollection : the AssetCollection to assign this POI to
    /// @param Callback CreatePOICallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreatePOI(const oly_common::String& Title, const oly_common::String& Description, const oly_common::String& Name,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& Tags, EPointOfInterestType Type, const oly_common::String& Owner,
        const oly_systems::GeoLocation& Location, const AssetCollection& AssetCollection, POIResultCallback Callback);

    /// @brief Deletes a given Point of Interest
    /// @param POI PointOfInterest : POI to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeletePOI(const PointOfInterest& POI, NullResultCallback Callback);

    /// @brief Retrieves an array with all the Points of Interest that are located inside the circular area defined by the parameters
    /// @param OriginLocation oly_systems::GeoLocation : latitude and longitude coordinates of the circular area origin
    /// @param AreaRadius double : radius of the circular area origin
    /// @param Callback POICollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetPOIsInArea(
        const oly_systems::GeoLocation& OriginLocation, const double AreaRadius, POICollectionResultCallback Callback);
    ///@}

protected:
    PointOfInterestSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    OLY_NO_EXPORT PointOfInterestSystem(oly_web::WebClient* InWebClient);

    /// @brief Creates a new Point of Interest for storing the Space Site information. This functionality should only be accessed through the Space
    /// System.
    /// @param Site Site : Site information to be stored
    /// @param Callback SiteResultCallback : callback when asynchronous task finishes
    void CreateSite(const Site& Site, SiteResultCallback Callback);

    /// @brief Removes the Point of Interest that was storing the Space Site information. This functionality should only be accessed through the Space
    /// System.
    /// @param Site Site : Site information to be removed
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    void DeleteSite(const Site& Site, NullResultCallback Callback);

    /// @brief Retrieves the Sites information associated with a Space. This functionality should only be accessed through the Space system
    /// @param Space Space : Space for which the associated POIs will be queried
    /// @param Callback SitesCollectionResultCallback : callback when asynchronous task finishes
    void GetSites(const oly_common::String& SpaceId, SitesCollectionResultCallback Callback);

    void AddSpaceGeoLocation(const oly_common::String& SpaceId, const oly_common::Optional<GeoLocation>& Location,
        const oly_common::Optional<float>& Orientation, const oly_common::Optional<oly_common::Array<GeoLocation>>& GeoFence,
        SpaceGeoLocationResultCallback Callback);

    void UpdateSpaceGeoLocation(const oly_common::String& SpaceId, const oly_common::String& SpaceGeoLocationId,
        const oly_common::Optional<GeoLocation>& Location, const oly_common::Optional<float>& Orientation,
        const oly_common::Optional<oly_common::Array<GeoLocation>>& GeoFence, SpaceGeoLocationResultCallback Callback);

    void GetSpaceGeoLocation(const oly_common::String& SpaceId, SpaceGeoLocationResultCallback Callback);

    void DeleteSpaceGeoLocation(const oly_common::String& SpaceId, NullResultCallback Callback);

private:
    void DeletePOIInternal(const oly_common::String POIId, NullResultCallback Callback);

    oly_services::ApiBase* POIApiPtr;
};

} // namespace oly_systems
