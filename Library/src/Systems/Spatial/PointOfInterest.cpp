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
#include "CSP/Systems/Spatial/PointOfInterest.h"
#include "Systems/Spatial/PointOfInterestHelpers.h"

#include "Services/SpatialDataService/Api.h"
#include "Services/SpatialDataService/Dto.h"

namespace chs = csp::services::generated::spatialdataservice;

namespace
{

void PointOfInterestDtoToPointOfInterest(const chs::PointOfInterestDto& dto, csp::systems::PointOfInterest& poi)
{
    if (dto.HasId())
    {
        poi.Id = dto.GetId();
    }

    if (dto.HasCreatedBy())
    {
        poi.CreatedBy = dto.GetCreatedBy();
    }

    if (dto.HasCreatedAt())
    {
        poi.CreatedAt = dto.GetCreatedAt();
    }

    if (dto.HasTitle())
    {
        const auto& localisedTitle = dto.GetTitle();

        for (auto& currentTitle : localisedTitle)
        {
            poi.Title[currentTitle->GetLanguageCode()] = currentTitle->GetValue();
        }
    }

    if (dto.HasDescription())
    {
        const auto& localisedDescription = dto.GetDescription();

        for (auto& currentDescription : localisedDescription)
        {
            poi.Description[currentDescription->GetLanguageCode()] = currentDescription->GetValue();
        }
    }

    if (dto.HasName())
    {
        poi.Name = dto.GetName();
    }

    if (dto.HasType())
    {
        poi.Type = csp::systems::PointOfInterestHelpers::StringToType(dto.GetType());
    }

    if (dto.HasTags())
    {
        const auto& tags = dto.GetTags();
        poi.Tags = csp::common::Array<csp::common::String>(tags.size());

        for (size_t idx = 0; idx < tags.size(); ++idx)
        {
            poi.Tags[idx] = tags[idx];
        }
    }

    if (dto.HasOwner())
    {
        poi.Owner = dto.GetOwner();
    }

    if (dto.HasLocation())
    {
        const auto& location = dto.GetLocation();
        poi.Location.Longitude = location->GetLongitude();
        poi.Location.Latitude = location->GetLatitude();
    }

    if (dto.HasPrototypeName())
    {
        // TODO: Find out why we're using name instead of Id here
        poi.AssetCollectionId = dto.GetPrototypeName();
    }

    if (dto.HasGroupId())
    {
        poi.SpaceId = dto.GetGroupId();
    }
}

} // namespace

namespace csp::systems
{

PointOfInterest::PointOfInterest()
    : Type(EPointOfInterestType::DEFAULT)
{
}

bool PointOfInterest::operator==(const PointOfInterest& other) const
{
    return Id == other.Id && CreatedBy == other.CreatedBy && CreatedAt == other.CreatedAt && Title == other.Title && Description == other.Description
        && Name == other.Name && Type == other.Type && Tags == other.Tags && Owner == other.Owner && Location == other.Location
        && AssetCollectionId == other.AssetCollectionId && SpaceId == other.SpaceId;
}

bool PointOfInterest::operator!=(const PointOfInterest& other) const { return !(*this == other); }

PointOfInterest& POIResult::GetPointOfInterest() { return m_poi; }

const PointOfInterest& POIResult::GetPointOfInterest() const { return m_poi; }

void POIResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* poiResponse = static_cast<chs::PointOfInterestDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        poiResponse->FromJson(response->GetPayload().GetContent());

        PointOfInterestDtoToPointOfInterest(*poiResponse, m_poi);
    }
}

csp::common::Array<csp::systems::PointOfInterest>& POICollectionResult::GetPOIs() { return m_poIs; }

const csp::common::Array<csp::systems::PointOfInterest>& POICollectionResult::GetPOIs() const { return m_poIs; }

void POICollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* poiCollectionResponse = static_cast<csp::services::DtoArray<chs::PointOfInterestDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        poiCollectionResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from response in our POIs array
        std::vector<chs::PointOfInterestDto>& poiArray = poiCollectionResponse->GetArray();
        m_poIs = csp::common::Array<csp::systems::PointOfInterest>(poiArray.size());

        for (size_t idx = 0; idx < poiArray.size(); ++idx)
        {
            PointOfInterestDtoToPointOfInterest(poiArray[idx], m_poIs[idx]);
        }
    }
}

} // namespace csp::systems
