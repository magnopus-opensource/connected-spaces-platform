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

#include "CSP/Systems/Spaces/Site.h"
#include "CSP/Systems/Spatial/PointOfInterestSystem.h"
#include "Common/Web/WebClient.h"
#include "Debug/Logging.h"

namespace csp::systems
{

class PointOfInterestInternalSystem : public PointOfInterestSystem
{
public:
    PointOfInterestInternalSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
        : PointOfInterestSystem(inWebClient, logSystem) {};

    void CreateSite(const Site& site, SiteResultCallback callback) { PointOfInterestSystem::CreateSite(site, callback); };

    void DeleteSite(const Site& site, NullResultCallback callback) { PointOfInterestSystem::DeleteSite(site, callback); };

    void GetSites(const csp::common::String& spaceId, SitesCollectionResultCallback callback) { PointOfInterestSystem::GetSites(spaceId, callback); };

    void AddSpaceGeoLocation(const csp::common::String& spaceId, const csp::common::Optional<GeoLocation>& location,
        const csp::common::Optional<float>& orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& geoFence,
        SpaceGeoLocationResultCallback callback)
    {
        PointOfInterestSystem::AddSpaceGeoLocation(spaceId, location, orientation, geoFence, callback);
    }

    void UpdateSpaceGeoLocation(const csp::common::String& spaceId, const csp::common::String& spaceGeoLocationId,
        const csp::common::Optional<GeoLocation>& location, const csp::common::Optional<float>& orientation,
        const csp::common::Optional<csp::common::Array<GeoLocation>>& geoFence, SpaceGeoLocationResultCallback callback)
    {
        PointOfInterestSystem::UpdateSpaceGeoLocation(spaceId, spaceGeoLocationId, location, orientation, geoFence, callback);
    }

    void GetSpaceGeoLocation(const csp::common::String& spaceId, SpaceGeoLocationResultCallback callback)
    {
        PointOfInterestSystem::GetSpaceGeoLocation(spaceId, callback);
    }

    void DeleteSpaceGeoLocation(const csp::common::String& spaceGeoLocationId, NullResultCallback callback)
    {
        PointOfInterestSystem::DeleteSpaceGeoLocation(spaceGeoLocationId, callback);
    }
};

} // namespace csp::systems
