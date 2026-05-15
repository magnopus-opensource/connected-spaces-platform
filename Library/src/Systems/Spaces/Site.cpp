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

void PointOfInterestDtoToSiteInfo(const chs::PointOfInterestDto& dto, csp::systems::Site& site)
{
    site.Id = dto.GetId();
    site.SpaceId = dto.GetGroupId();

    const auto& localisedTitle = dto.GetTitle();
    for (auto& currentTitle : localisedTitle)
    {
        if (currentTitle->GetLanguageCode() == ENGLISH_LANGUAGE_CODE)
        {
            site.Name = currentTitle->GetValue();
        }
    }

    if (dto.HasLocation())
    {
        const auto& location = dto.GetLocation();
        site.Location.Longitude = location->GetLongitude();
        site.Location.Latitude = location->GetLatitude();
    }

    if (dto.HasPrototypeTransform())
    {
        const auto& transform = dto.GetPrototypeTransform();
        if (transform->HasRotation())
        {
            const auto& rotation = transform->GetRotation();
            site.Rotation.X = rotation->GetX();
            site.Rotation.Y = rotation->GetY();
            site.Rotation.Z = rotation->GetZ();
            site.Rotation.W = rotation->GetW();
        }
    }
}

} // namespace

namespace csp::systems
{

bool Site::operator==(const Site& other) const
{
    return Id == other.Id && Name == other.Name && SpaceId == other.SpaceId && Location == other.Location && Rotation == other.Rotation;
}

bool Site::operator!=(const Site& other) const { return !(*this == other); }

Site& SiteResult::GetSite() { return m_site; }

const Site& SiteResult::GetSite() const { return m_site; }

void SiteResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* poiResponse = static_cast<chs::PointOfInterestDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        poiResponse->FromJson(response->GetPayload().GetContent());

        PointOfInterestDtoToSiteInfo(*poiResponse, m_site);
    }
}

csp::common::Array<csp::systems::Site>& SitesCollectionResult::GetSites() { return m_sites; }

const csp::common::Array<csp::systems::Site>& SitesCollectionResult::GetSites() const { return m_sites; }

void SitesCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* poiCollectionResponse = static_cast<csp::services::DtoArray<chs::PointOfInterestDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        poiCollectionResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from response in our Sites array
        std::vector<chs::PointOfInterestDto>& poiArray = poiCollectionResponse->GetArray();
        m_sites = csp::common::Array<csp::systems::Site>(poiArray.size());

        for (size_t idx = 0; idx < poiArray.size(); ++idx)
        {
            PointOfInterestDtoToSiteInfo(poiArray[idx], m_sites[idx]);
        }
    }
}

} // namespace csp::systems
