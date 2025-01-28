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
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Spaces/Site.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spatial/PointOfInterest.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

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

class Space;
class AssetCollection;
class PointOfInterestInternalSystem;

/// @ingroup Point of Interest System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' concept of a Point of Interest.
/// Offers methods for creating and deleting POIs.
class CSP_API PointOfInterestSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class PointOfInterestInternalSystem;
    friend void csp::memory::Delete<PointOfInterestSystem>(PointOfInterestSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */

    /// @brief Creates a new Point of Interest.
    /// @param Title csp::common::String : title for the new POI
    /// @param Description csp::common::String : description for the new POI
    /// @param Name csp::common::String : name for the new POI
    /// @param Tags csp::common::Array<csp::common::String> : optional array of tags to be added to the new POI
    /// @param Type EPointOfInterestType : type of the new POI
    /// @param Owner csp::common::String : owner of the new POI
    /// @param Location csp::systems::GeoLocation : latitude and longitude coordinates of the new POI
    /// @param AssetCollection csp::systems::AssetCollection : the AssetCollection to assign this POI to
    /// @param Callback CreatePOICallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreatePOI(const csp::common::String& Title, const csp::common::String& Description, const csp::common::String& Name,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, EPointOfInterestType Type, const csp::common::String& Owner,
        const csp::systems::GeoLocation& Location, const AssetCollection& AssetCollection, POIResultCallback Callback);

    /// @brief Deletes a given Point of Interest
    /// @param POI PointOfInterest : POI to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeletePOI(const PointOfInterest& POI, NullResultCallback Callback);

    /// @brief Retrieves an array with all the Points of Interest that are located inside the circular area defined by the parameters..
    /// @param OriginLocation csp::systems::GeoLocation : The latitude and longitude coordinates of origin of the search location.
    /// @param AreaRadius double : The Radius of the circular area to search around the provided origin.
    /// @param Type csp::common::Optional<EPointOfInterestType> : The type of POI to search for. If none is specified, all types will be included in
    /// the returned set.
    /// @param Callback POICollectionResultCallback : callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetPOIsInArea(const csp::systems::GeoLocation& OriginLocation, const double AreaRadius,
        const csp::common::Optional<EPointOfInterestType>& Type, POICollectionResultCallback Callback);
    ///@}

protected:
    PointOfInterestSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT PointOfInterestSystem(csp::web::WebClient* InWebClient);

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
    void GetSites(const csp::common::String& SpaceId, SitesCollectionResultCallback Callback);

    void AddSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::Optional<GeoLocation>& Location,
        const csp::common::Optional<float>& Orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence,
        SpaceGeoLocationResultCallback Callback);

    void UpdateSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::String& SpaceGeoLocationId,
        const csp::common::Optional<GeoLocation>& Location, const csp::common::Optional<float>& Orientation,
        const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence, SpaceGeoLocationResultCallback Callback);

    void GetSpaceGeoLocation(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback);

    void DeleteSpaceGeoLocation(const csp::common::String& SpaceId, NullResultCallback Callback);

private:
    ~PointOfInterestSystem();

    void DeletePOIInternal(const csp::common::String POIId, NullResultCallback Callback);

    csp::services::ApiBase* POIApiPtr;
};

} // namespace csp::systems
