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

void AnchorResolutionDtoToAnchorResolution(const chs::AnchorResolutionDto& dto, csp::systems::AnchorResolution& anchorResolution)
{
    anchorResolution.Id = dto.GetId();
    anchorResolution.AnchorId = dto.GetAnchorId();
    anchorResolution.SuccessfullyResolved = dto.GetSuccessfullyResolved();
    anchorResolution.ResolveAttempted = dto.GetResolveAttempted();
    anchorResolution.ResolveTime = dto.GetResolveTime();

    const auto& dtoTags = dto.GetTags();
    anchorResolution.Tags = csp::common::Array<csp::common::String>(dtoTags.size());

    for (size_t idx = 0; idx < dtoTags.size(); ++idx)
    {
        anchorResolution.Tags[idx] = dtoTags[idx];
    }
}

} // namespace

namespace csp::systems
{

void AnchorDtoToAnchor(const chs::AnchorDto& dto, csp::systems::Anchor& anchor)
{
    anchor.Id = dto.GetMgsId();
    anchor.CreatedBy = dto.GetCreatedBy();
    anchor.CreatedAt = dto.GetCreatedAt();

    if (dto.HasThirdPartyProviderName())
    {
        const auto name = dto.GetThirdPartyProviderName();
        if (name == "GoogleCloudAnchors")
        {
            anchor.ThirdPartyAnchorProvider = csp::systems::AnchorProvider::GoogleCloudAnchors;
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("Unknown third party anchor provider: %s", name.c_str());
        }
    }

    if (dto.HasThirdPartyAnchorId())
    {
        anchor.ThirdPartyAnchorId = dto.GetThirdPartyAnchorId();
    }

    if (dto.HasReferenceId())
    {
        anchor.SpaceId = dto.GetReferenceId();
    }

    if (dto.HasAnchoredMultiplayerObjectId())
    {
        anchor.SpaceEntityId = dto.GetAnchoredMultiplayerObjectId();
    }

    if (dto.HasAnchoredPrototypeId())
    {
        anchor.AssetCollectionId = dto.GetAnchoredPrototypeId();
    }

    if (dto.HasLocation())
    {
        const auto& location = dto.GetLocation();
        anchor.Location.Longitude = location->GetLongitude();
        anchor.Location.Latitude = location->GetLatitude();
    }

    if (dto.HasPosition())
    {
        const auto& dtoPosition = dto.GetPosition();
        anchor.Position.X = dtoPosition->GetX();
        anchor.Position.Y = dtoPosition->GetY();
        anchor.Position.Z = dtoPosition->GetZ();
    }

    if (dto.HasRotation())
    {
        const auto& dtoRotation = dto.GetRotation();
        anchor.Rotation.X = dtoRotation->GetX();
        anchor.Rotation.Y = dtoRotation->GetY();
        anchor.Rotation.Z = dtoRotation->GetZ();
        anchor.Rotation.W = dtoRotation->GetW();
    }

    if (dto.HasTags())
    {
        const auto& dtoTags = dto.GetTags();
        anchor.Tags = csp::common::Array<csp::common::String>(dtoTags.size());

        for (size_t idx = 0; idx < dtoTags.size(); ++idx)
        {
            anchor.Tags[idx] = dtoTags[idx];
        }
    }

    if (dto.HasSpatialKeyValue())
    {
        const auto& dtoSpatialKeyValue = dto.GetSpatialKeyValue();

        for (auto& pair : dtoSpatialKeyValue)
        {
            anchor.SpatialKeyValue[pair.first] = pair.second;
        }
    }
}

OlyAnchorPosition::OlyAnchorPosition()
    : X(0.0)
    , Y(0.0)
    , Z(0.0)
{
}

bool OlyAnchorPosition::operator==(const OlyAnchorPosition& other) const { return X == other.X && Y == other.Y && Z == other.Z; }

bool Anchor::operator==(const Anchor& other) const
{
    return Id == other.Id && ThirdPartyAnchorProvider == other.ThirdPartyAnchorProvider && ThirdPartyAnchorId == other.ThirdPartyAnchorId
        && CreatedBy == other.CreatedBy && CreatedAt == other.CreatedAt && SpaceId == other.SpaceId && SpaceEntityId == other.SpaceEntityId
        && AssetCollectionId == other.AssetCollectionId && Location == other.Location && Position == other.Position && Rotation == other.Rotation
        && Tags == other.Tags && SpatialKeyValue == other.SpatialKeyValue;
}

bool AnchorResolution::operator==(const AnchorResolution& other) const
{
    return Id == other.Id && AnchorId == other.AnchorId && SuccessfullyResolved == other.SuccessfullyResolved
        && ResolveAttempted == other.ResolveAttempted && ResolveTime == other.ResolveTime && Tags == other.Tags;
}

bool OlyAnchorPosition::operator!=(const OlyAnchorPosition& other) const { return !(*this == other); }
bool Anchor::operator!=(const Anchor& other) const { return !(*this == other); }
bool AnchorResolution::operator!=(const AnchorResolution& other) const { return !(*this == other); }

Anchor& AnchorResult::GetAnchor() { return m_anchor; }

const Anchor& AnchorResult::GetAnchor() const { return m_anchor; }

void AnchorResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* anchorResponse = static_cast<chs::AnchorDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        anchorResponse->FromJson(response->GetPayload().GetContent());

        AnchorDtoToAnchor(*anchorResponse, m_anchor);
    }
}

csp::common::Array<Anchor>& AnchorCollectionResult::GetAnchors() { return m_anchors; }

const csp::common::Array<Anchor>& AnchorCollectionResult::GetAnchors() const { return m_anchors; }

void AnchorCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* anchorCollectionResponse = static_cast<csp::services::DtoArray<chs::AnchorDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        anchorCollectionResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from the response into our Anchors array
        std::vector<chs::AnchorDto>& anchorsArray = anchorCollectionResponse->GetArray();
        m_anchors = csp::common::Array<csp::systems::Anchor>(anchorsArray.size());

        for (size_t idx = 0; idx < anchorsArray.size(); ++idx)
        {
            AnchorDtoToAnchor(anchorsArray[idx], m_anchors[idx]);
        }
    }
}

AnchorResolution& AnchorResolutionResult::GetAnchorResolution() { return m_anchorResolution; }

const AnchorResolution& AnchorResolutionResult::GetAnchorResolution() const { return m_anchorResolution; }

void AnchorResolutionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* anchorResolutionResponse = static_cast<chs::AnchorResolutionDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        anchorResolutionResponse->FromJson(response->GetPayload().GetContent());
        AnchorResolutionDtoToAnchorResolution(*anchorResolutionResponse, m_anchorResolution);
    }
}

csp::common::Array<AnchorResolution>& AnchorResolutionCollectionResult::GetAnchorResolutions() { return m_anchorResolutions; }

const csp::common::Array<AnchorResolution>& AnchorResolutionCollectionResult::GetAnchorResolutions() const { return m_anchorResolutions; }

void AnchorResolutionCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* anchorResolutionCollectionResponse = static_cast<csp::services::DtoArray<chs::AnchorResolutionDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        anchorResolutionCollectionResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from the response into our Anchors array
        std::vector<chs::AnchorResolutionDto>& anchorsResolutionArray = anchorResolutionCollectionResponse->GetArray();
        m_anchorResolutions = csp::common::Array<csp::systems::AnchorResolution>(anchorsResolutionArray.size());

        for (size_t idx = 0; idx < anchorsResolutionArray.size(); ++idx)
        {
            AnchorResolutionDtoToAnchorResolution(anchorsResolutionArray[idx], m_anchorResolutions[idx]);
        }
    }
}
} // namespace csp::systems
