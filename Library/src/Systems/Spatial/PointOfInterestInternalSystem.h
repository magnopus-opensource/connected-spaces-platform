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
#include "Debug/Logging.h"
#include "Web/WebClient.h"

namespace csp::systems
{

class PointOfInterestInternalSystem : public PointOfInterestSystem
{
public:
    PointOfInterestInternalSystem(csp::web::WebClient* InWebClient)
        : PointOfInterestSystem(InWebClient) {};

    void CreateSite(const Site& Site, SiteResultCallback Callback) { PointOfInterestSystem::CreateSite(Site, Callback); };

    void DeleteSite(const Site& Site, NullResultCallback Callback) { PointOfInterestSystem::DeleteSite(Site, Callback); };

    void GetSites(const csp::common::String& SpaceId, SitesCollectionResultCallback Callback) { PointOfInterestSystem::GetSites(SpaceId, Callback); };

    void AddSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::Optional<GeoLocation>& Location,
        const csp::common::Optional<float>& Orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence,
        SpaceGeoLocationResultCallback Callback)
    {
        PointOfInterestSystem::AddSpaceGeoLocation(SpaceId, Location, Orientation, GeoFence, Callback);
    }

    void UpdateSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::String& SpaceGeoLocationId,
        const csp::common::Optional<GeoLocation>& Location, const csp::common::Optional<float>& Orientation,
        const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence, SpaceGeoLocationResultCallback Callback)
    {
        PointOfInterestSystem::UpdateSpaceGeoLocation(SpaceId, SpaceGeoLocationId, Location, Orientation, GeoFence, Callback);
    }

    void GetSpaceGeoLocation(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback)
    {
        PointOfInterestSystem::GetSpaceGeoLocation(SpaceId, Callback);
    }

    void DeleteSpaceGeoLocation(const csp::common::String& SpaceGeoLocationId, NullResultCallback Callback)
    {
        PointOfInterestSystem::DeleteSpaceGeoLocation(SpaceGeoLocationId, Callback);
    }
};

} // namespace csp::systems
