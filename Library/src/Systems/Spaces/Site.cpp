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
#include "CSP/Systems/Spaces/Site.h"

#include "Services/SpatialDataService/Api.h"
#include "Services/SpatialDataService/Dto.h"

namespace chs = csp::services::generated::spatialdataservice;

namespace
{

const char* ENGLISH_LANGUAGE_CODE = "EN";

void PointOfInterestDtoToSiteInfo(const chs::PointOfInterestDto& Dto, csp::systems::Site& Site)
{
    Site.Id = Dto.GetId();
    Site.SpaceId = Dto.GetGroupId();

    const auto& LocalisedTitle = Dto.GetTitle();
    for (auto& CurrentTitle : LocalisedTitle)
    {
        if (CurrentTitle->GetLanguageCode() == ENGLISH_LANGUAGE_CODE)
        {
            Site.Name = CurrentTitle->GetValue();
        }
    }

    if (Dto.HasLocation())
    {
        const auto& Location = Dto.GetLocation();
        Site.Location.Longitude = Location->GetLongitude();
        Site.Location.Latitude = Location->GetLatitude();
    }

    if (Dto.HasPrototypeTransform())
    {
        const auto& Transform = Dto.GetPrototypeTransform();
        if (Transform->HasRotation())
        {
            const auto& Rotation = Transform->GetRotation();
            Site.Rotation.X = Rotation->GetX();
            Site.Rotation.Y = Rotation->GetY();
            Site.Rotation.Z = Rotation->GetZ();
            Site.Rotation.W = Rotation->GetW();
        }
    }
}

} // namespace

namespace csp::systems
{

Site& SiteResult::GetSite() { return Site; }

const Site& SiteResult::GetSite() const { return Site; }

void SiteResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* POIResponse = static_cast<chs::PointOfInterestDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        POIResponse->FromJson(Response->GetPayload().GetContent());

        PointOfInterestDtoToSiteInfo(*POIResponse, Site);
    }
}

csp::common::Array<csp::systems::Site>& SitesCollectionResult::GetSites() { return Sites; }

const csp::common::Array<csp::systems::Site>& SitesCollectionResult::GetSites() const { return Sites; }

void SitesCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* POICollectionResponse = static_cast<csp::services::DtoArray<chs::PointOfInterestDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        POICollectionResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from response in our Sites array
        std::vector<chs::PointOfInterestDto>& POIArray = POICollectionResponse->GetArray();
        Sites = csp::common::Array<csp::systems::Site>(POIArray.size());

        for (size_t idx = 0; idx < POIArray.size(); ++idx)
        {
            PointOfInterestDtoToSiteInfo(POIArray[idx], Sites[idx]);
        }
    }
}

} // namespace csp::systems
