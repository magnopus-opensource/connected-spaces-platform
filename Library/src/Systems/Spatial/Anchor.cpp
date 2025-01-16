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
#include "CSP/Systems/Spatial/Anchor.h"

#include "Services/SpatialDataService/Api.h"
#include "Services/SpatialDataService/Dto.h"

namespace chs = csp::services::generated::spatialdataservice;

namespace
{

void AnchorDtoToAnchor(const chs::AnchorDto& Dto, csp::systems::Anchor& Anchor)
{
    Anchor.Id = Dto.GetMgsId();
    Anchor.CreatedBy = Dto.GetCreatedBy();
    Anchor.CreatedAt = Dto.GetCreatedAt();

    if (Dto.HasThirdPartyProviderName())
    {
        const auto Name = Dto.GetThirdPartyProviderName();
        if (Name == "GoogleCloudAnchors")
        {
            Anchor.ThirdPartyAnchorProvider = csp::systems::AnchorProvider::GoogleCloudAnchors;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("Unknown third party anchor provider: %s", Name.c_str());
        }
    }

    if (Dto.HasThirdPartyAnchorId())
    {
        Anchor.ThirdPartyAnchorId = Dto.GetThirdPartyAnchorId();
    }

    if (Dto.HasReferenceId())
    {
        Anchor.SpaceId = Dto.GetReferenceId();
    }

    if (Dto.HasAnchoredMultiplayerObjectId())
    {
        Anchor.SpaceEntityId = Dto.GetAnchoredMultiplayerObjectId();
    }

    if (Dto.HasAnchoredPrototypeId())
    {
        Anchor.AssetCollectionId = Dto.GetAnchoredPrototypeId();
    }

    if (Dto.HasLocation())
    {
        const auto& Location = Dto.GetLocation();
        Anchor.Location.Longitude = Location->GetLongitude();
        Anchor.Location.Latitude = Location->GetLatitude();
    }

    if (Dto.HasPosition())
    {
        const auto& DtoPosition = Dto.GetPosition();
        Anchor.Position.X = DtoPosition->GetX();
        Anchor.Position.Y = DtoPosition->GetY();
        Anchor.Position.Z = DtoPosition->GetZ();
    }

    if (Dto.HasRotation())
    {
        const auto& DtoRotation = Dto.GetRotation();
        Anchor.Rotation.X = DtoRotation->GetX();
        Anchor.Rotation.Y = DtoRotation->GetY();
        Anchor.Rotation.Z = DtoRotation->GetZ();
        Anchor.Rotation.W = DtoRotation->GetW();
    }

    if (Dto.HasTags())
    {
        const auto& DtoTags = Dto.GetTags();
        Anchor.Tags = csp::common::Array<csp::common::String>(DtoTags.size());

        for (size_t idx = 0; idx < DtoTags.size(); ++idx)
        {
            Anchor.Tags[idx] = DtoTags[idx];
        }
    }

    if (Dto.HasSpatialKeyValue())
    {
        const auto& DtoSpatialKeyValue = Dto.GetSpatialKeyValue();

        for (auto& Pair : DtoSpatialKeyValue)
        {
            Anchor.SpatialKeyValue[Pair.first] = Pair.second;
        }
    }
}

void AnchorResolutionDtoToAnchorResolution(const chs::AnchorResolutionDto& Dto, csp::systems::AnchorResolution& AnchorResolution)
{
    AnchorResolution.Id = Dto.GetId();
    AnchorResolution.AnchorId = Dto.GetAnchorId();
    AnchorResolution.SuccessfullyResolved = Dto.GetSuccessfullyResolved();
    AnchorResolution.ResolveAttempted = Dto.GetResolveAttempted();
    AnchorResolution.ResolveTime = Dto.GetResolveTime();

    const auto& DtoTags = Dto.GetTags();
    AnchorResolution.Tags = csp::common::Array<csp::common::String>(DtoTags.size());

    for (size_t idx = 0; idx < DtoTags.size(); ++idx)
    {
        AnchorResolution.Tags[idx] = DtoTags[idx];
    }
}

} // namespace

namespace csp::systems
{

OlyAnchorPosition::OlyAnchorPosition()
    : X(0.0)
    , Y(0.0)
    , Z(0.0)
{
}

Anchor& AnchorResult::GetAnchor() { return Anchor; }

const Anchor& AnchorResult::GetAnchor() const { return Anchor; }

void AnchorResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AnchorResponse = static_cast<chs::AnchorDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        AnchorResponse->FromJson(Response->GetPayload().GetContent());

        AnchorDtoToAnchor(*AnchorResponse, Anchor);
    }
}

csp::common::Array<Anchor>& AnchorCollectionResult::GetAnchors() { return Anchors; }

const csp::common::Array<Anchor>& AnchorCollectionResult::GetAnchors() const { return Anchors; }

void AnchorCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AnchorCollectionResponse = static_cast<csp::services::DtoArray<chs::AnchorDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AnchorCollectionResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from the response into our Anchors array
        std::vector<chs::AnchorDto>& AnchorsArray = AnchorCollectionResponse->GetArray();
        Anchors = csp::common::Array<csp::systems::Anchor>(AnchorsArray.size());

        for (size_t idx = 0; idx < AnchorsArray.size(); ++idx)
        {
            AnchorDtoToAnchor(AnchorsArray[idx], Anchors[idx]);
        }
    }
}

AnchorResolution& AnchorResolutionResult::GetAnchorResolution() { return AnchorResolution; }

const AnchorResolution& AnchorResolutionResult::GetAnchorResolution() const { return AnchorResolution; }

void AnchorResolutionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AnchorResolutionResponse = static_cast<chs::AnchorResolutionDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        AnchorResolutionResponse->FromJson(Response->GetPayload().GetContent());
        AnchorResolutionDtoToAnchorResolution(*AnchorResolutionResponse, AnchorResolution);
    }
}

csp::common::Array<AnchorResolution>& AnchorResolutionCollectionResult::GetAnchorResolutions() { return AnchorResolutions; }

const csp::common::Array<AnchorResolution>& AnchorResolutionCollectionResult::GetAnchorResolutions() const { return AnchorResolutions; }

void AnchorResolutionCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AnchorResolutionCollectionResponse = static_cast<csp::services::DtoArray<chs::AnchorResolutionDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AnchorResolutionCollectionResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from the response into our Anchors array
        std::vector<chs::AnchorResolutionDto>& AnchorsResolutionArray = AnchorResolutionCollectionResponse->GetArray();
        AnchorResolutions = csp::common::Array<csp::systems::AnchorResolution>(AnchorsResolutionArray.size());

        for (size_t idx = 0; idx < AnchorsResolutionArray.size(); ++idx)
        {
            AnchorResolutionDtoToAnchorResolution(AnchorsResolutionArray[idx], AnchorResolutions[idx]);
        }
    }
}
} // namespace csp::systems
