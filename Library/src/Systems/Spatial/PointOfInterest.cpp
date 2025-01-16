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

void PointOfInterestDtoToPointOfInterest(const chs::PointOfInterestDto& Dto, csp::systems::PointOfInterest& POI)
{
    POI.Id = Dto.GetId();
    POI.CreatedBy = Dto.GetCreatedBy();
    POI.CreatedAt = Dto.GetCreatedAt();

    const auto& LocalisedTitle = Dto.GetTitle();

    for (auto& CurrentTitle : LocalisedTitle)
    {
        POI.Title[CurrentTitle->GetLanguageCode()] = CurrentTitle->GetValue();
    }

    const auto& LocalisedDescription = Dto.GetDescription();

    for (auto& CurrentDescription : LocalisedDescription)
    {
        POI.Description[CurrentDescription->GetLanguageCode()] = CurrentDescription->GetValue();
    }

    POI.Name = Dto.GetName();

    if (Dto.HasType())
    {
        POI.Type = csp::systems::PointOfInterestHelpers::StringToType(Dto.GetType());
    }

    if (Dto.HasTags())
    {
        const auto& Tags = Dto.GetTags();
        POI.Tags = csp::common::Array<csp::common::String>(Tags.size());

        for (int idx = 0; idx < Tags.size(); ++idx)
        {
            POI.Tags[idx] = Tags[idx];
        }
    }

    if (Dto.HasOwner())
    {
        POI.Owner = Dto.GetOwner();
    }

    if (Dto.HasLocation())
    {
        const auto& Location = Dto.GetLocation();
        POI.Location.Longitude = Location->GetLongitude();
        POI.Location.Latitude = Location->GetLatitude();
    }

    if (Dto.HasPrototypeName())
    {
        // TODO: Find out why we're using name instead of Id here
        POI.AssetCollectionId = Dto.GetPrototypeName();
    }

    if (Dto.HasGroupId())
    {
        POI.SpaceId = Dto.GetGroupId();
    }
}

} // namespace

namespace csp::systems
{

PointOfInterest::PointOfInterest()
    : Type(EPointOfInterestType::DEFAULT)
{
}

PointOfInterest& POIResult::GetPointOfInterest() { return POI; }

const PointOfInterest& POIResult::GetPointOfInterest() const { return POI; }

void POIResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* POIResponse = static_cast<chs::PointOfInterestDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        POIResponse->FromJson(Response->GetPayload().GetContent());

        PointOfInterestDtoToPointOfInterest(*POIResponse, POI);
    }
}

csp::common::Array<csp::systems::PointOfInterest>& POICollectionResult::GetPOIs() { return POIs; }

const csp::common::Array<csp::systems::PointOfInterest>& POICollectionResult::GetPOIs() const { return POIs; }

void POICollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* POICollectionResponse = static_cast<csp::services::DtoArray<chs::PointOfInterestDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        POICollectionResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from response in our POIs array
        std::vector<chs::PointOfInterestDto>& POIArray = POICollectionResponse->GetArray();
        POIs = csp::common::Array<csp::systems::PointOfInterest>(POIArray.size());

        for (size_t idx = 0; idx < POIArray.size(); ++idx)
        {
            PointOfInterestDtoToPointOfInterest(POIArray[idx], POIs[idx]);
        }
    }
}

} // namespace csp::systems
