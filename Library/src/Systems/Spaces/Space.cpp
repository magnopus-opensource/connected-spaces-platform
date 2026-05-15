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

#include "CSP/Systems/Spaces/Space.h"

#include "CSP/Systems/Assets/AssetCollection.h"
#include "Common/Convert.h"
#include "Common/Web/Json.h"
#include "Json/JsonParseHelper.h"
#include "Services/SpatialDataService/Api.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"

#include <charconv>
#include <regex>

using namespace csp;
using namespace csp::common;

namespace chs_users = csp::services::generated::userservice;
namespace chs_spatial = csp::services::generated::spatialdataservice;

namespace
{

void GroupLiteDtoToBasicSpace(const chs_users::GroupLiteDto& dto, csp::systems::BasicSpace& basicSpace)
{
    basicSpace.Id = dto.GetId();
    basicSpace.Name = dto.GetName();

    if (dto.HasDiscoverable() && dto.GetDiscoverable())
    {
        basicSpace.Attributes |= csp::systems::SpaceAttributes::IsDiscoverable;
    }

    if (dto.HasRequiresInvite() && dto.GetRequiresInvite())
    {
        basicSpace.Attributes |= csp::systems::SpaceAttributes::RequiresInvite;
    }

    if (dto.HasDescription())
    {
        basicSpace.Description = dto.GetDescription();
    }

    if (dto.HasTags())
    {
        basicSpace.Tags = csp::common::Convert(dto.GetTags());
    }
}

} // namespace

namespace csp::systems
{

void GroupDtoToSpace(const chs_users::GroupDto& dto, csp::systems::Space& space)
{
    space.Id = dto.GetId();
    space.CreatedBy = dto.GetCreatedBy();
    space.CreatedAt = dto.GetCreatedAt();
    space.OwnerId = dto.GetGroupOwnerId();
    space.Name = dto.GetName();

    if (dto.HasDiscoverable() && dto.GetDiscoverable())
    {
        space.Attributes |= csp::systems::SpaceAttributes::IsDiscoverable;
    }

    if (dto.HasRequiresInvite() && dto.GetRequiresInvite())
    {
        space.Attributes |= csp::systems::SpaceAttributes::RequiresInvite;
    }

    if (dto.HasDescription())
    {
        space.Description = dto.GetDescription();
    }

    if (dto.HasTags())
    {
        space.Tags = csp::common::Convert(dto.GetTags());
    }

    if (dto.HasUsers())
    {
        space.UserIds = csp::common::Convert(dto.GetUsers());
    }

    if (dto.HasModerators())
    {
        space.ModeratorIds = csp::common::Convert(dto.GetModerators());
    }

    if (dto.HasBannedUsers())
    {
        space.BannedUserIds = csp::common::Convert(dto.GetBannedUsers());
    }
}

bool Space::UserIsKnownToSpace(const csp::common::String userId) const
{
    return std::any_of(UserIds.cbegin(), UserIds.cend(), [userId](const csp::common::String& id) { return id == userId; })
        || std::any_of(ModeratorIds.cbegin(), ModeratorIds.cend(), [userId](const csp::common::String& id) { return id == userId; })
        || userId == OwnerId;
}

bool BasicSpace::operator==(const BasicSpace& other) const
{
    return Id == other.Id && Name == other.Name && Description == other.Description && Attributes == other.Attributes && Tags == other.Tags;
}

bool Space::operator==(const Space& other) const
{
    return BasicSpace::operator==(other) && CreatedBy == other.CreatedBy && CreatedAt == other.CreatedAt && OwnerId == other.OwnerId
        && UserIds == other.UserIds && ModeratorIds == other.ModeratorIds && BannedUserIds == other.BannedUserIds;
}

bool BasicSpace::operator!=(const BasicSpace& other) const { return !(*this == other); }
bool Space::operator!=(const Space& other) const { return !(*this == other); }

const Space& SpaceResult::GetSpace() const { return m_space; }

const csp::common::String& SpaceResult::GetSpaceCode() const { return m_spaceCode; }

void SpaceResult::SetSpace(const csp::systems::Space& inSpace) { m_space = inSpace; }

void SpaceResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* groupResponse = static_cast<chs_users::GroupDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        groupResponse->FromJson(response->GetPayload().GetContent());
        GroupDtoToSpace(*groupResponse, m_space);

        if (groupResponse->HasGroupCode())
        {
            m_spaceCode = groupResponse->GetGroupCode();
        }
    }
}

Array<Space>& SpacesResult::GetSpaces() { return m_spaces; }

const Array<Space>& SpacesResult::GetSpaces() const { return m_spaces; }

void SpacesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* groupsResponse = static_cast<csp::services::DtoArray<chs_users::GroupDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        groupsResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from response in our Groups array
        std::vector<chs_users::GroupDto>& groupArray = groupsResponse->GetArray();
        m_spaces = Array<csp::systems::Space>(groupArray.size());

        for (size_t i = 0; i < groupArray.size(); ++i)
        {
            GroupDtoToSpace(groupArray[i], m_spaces[i]);
        }
    }
}

BasicSpace& BasicSpaceResult::GetSpace() { return m_space; }

const BasicSpace& BasicSpaceResult::GetSpace() const { return m_space; }

void BasicSpaceResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* liteGroupResponse = static_cast<chs_users::GroupLiteDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        liteGroupResponse->FromJson(response->GetPayload().GetContent());

        GroupLiteDtoToBasicSpace(*liteGroupResponse, m_space);
    }
}

Array<BasicSpace>& BasicSpacesResult::GetSpaces() { return m_spaces; }

const Array<BasicSpace>& BasicSpacesResult::GetSpaces() const { return m_spaces; }

uint64_t BasicSpacesResult::GetTotalCount() const { return m_resultTotalCount; }

void BasicSpacesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* groupsResponse = static_cast<csp::services::DtoArray<chs_users::GroupLiteDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        groupsResponse->FromJson(response->GetPayload().GetContent());
        FillResultTotalCount(response->GetPayload().GetContent());

        // Extract data from response in our Groups array
        std::vector<chs_users::GroupLiteDto>& groupArray = groupsResponse->GetArray();
        m_spaces = Array<csp::systems::BasicSpace>(groupArray.size());

        for (size_t i = 0; i < groupArray.size(); ++i)
        {
            GroupLiteDtoToBasicSpace(groupArray[i], m_spaces[i]);
        }
    }
}

void BasicSpacesResult::FillResultTotalCount(const String& jsonContent)
{
    assert(jsonContent.c_str());

    rapidjson::Document jsonDoc;
    rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(jsonDoc, jsonContent, "BasicSpacesResult::FillResultTotalCount");
    if (!ok)
    {
        return;
    }

    m_resultTotalCount = 0;
    if (jsonDoc.HasMember("itemTotalCount"))
    {
        rapidjson::Value& val = jsonDoc["itemTotalCount"];
        auto totalCountStr = csp::web::JsonObjectToString(val);

        uint64_t convertedTotalCount = 0;
        const auto result = std::from_chars(totalCountStr.c_str(), totalCountStr.c_str() + totalCountStr.Length(), convertedTotalCount);
        if (result.ec == std::errc())
        {
            m_resultTotalCount = convertedTotalCount;
        }
    }
}

const Map<String, String>& SpaceMetadataResult::GetMetadata() const { return m_metadata; }

void SpaceMetadataResult::SetMetadata(const Map<String, String>& inMetadata) { m_metadata = inMetadata; }

Array<String>& PendingInvitesResult::GetPendingInvitesEmails() { return m_pendingInvitesEmailAddresses; }

const Array<String>& PendingInvitesResult::GetPendingInvitesEmails() const { return m_pendingInvitesEmailAddresses; }

void PendingInvitesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* pendingInvitesResponse = static_cast<csp::services::DtoArray<chs_users::GroupInviteDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        pendingInvitesResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from response in our pending invites array
        std::vector<chs_users::GroupInviteDto>& pendingInvitesArray = pendingInvitesResponse->GetArray();
        m_pendingInvitesEmailAddresses = Array<String>(pendingInvitesArray.size());

        for (size_t idx = 0; idx < pendingInvitesArray.size(); ++idx)
        {
            m_pendingInvitesEmailAddresses[idx] = pendingInvitesArray[idx].GetEmail();
        }
    }
}

Array<String>& AcceptedInvitesResult::GetAcceptedInvitesUserIds() { return m_acceptedInvitesUserIds; }

const Array<String>& AcceptedInvitesResult::GetAcceptedInvitesUserIds() const { return m_acceptedInvitesUserIds; }

void AcceptedInvitesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* acceptedInvitesResponse = static_cast<csp::services::DtoArray<chs_users::GroupInviteDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        acceptedInvitesResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from response in our accepted invites array
        std::vector<chs_users::GroupInviteDto>& acceptedInvitesArray = acceptedInvitesResponse->GetArray();
        m_acceptedInvitesUserIds = Array<String>(acceptedInvitesArray.size());

        for (size_t idx = 0; idx < acceptedInvitesArray.size(); ++idx)
        {
            if (acceptedInvitesArray[idx].HasId())
            {
                m_acceptedInvitesUserIds[idx] = acceptedInvitesArray[idx].GetId();
            }
        }
    }
}

const Map<String, Map<String, String>>& SpacesMetadataResult::GetMetadata() const { return m_metadata; }

const Map<String, Array<String>>& SpacesMetadataResult::GetTags() const { return m_tags; }

void SpacesMetadataResult::SetMetadata(const Map<String, Map<String, String>>& inMetadata) { m_metadata = inMetadata; }

void SpacesMetadataResult::SetTags(const Map<String, Array<String>>& inTags) { m_tags = inTags; }

bool SpaceGeoLocation::operator==(const SpaceGeoLocation& other) const
{
    return SpaceId == other.SpaceId && Location == other.Location && Orientation == other.Orientation && GeoFence == other.GeoFence && m_id == other.m_id;
}

bool SpaceGeoLocation::operator!=(const SpaceGeoLocation& other) const { return !(*this == other); }

bool SpaceGeoLocationResult::HasSpaceGeoLocation() const { return m_hasGeoLocation; }

const SpaceGeoLocation& SpaceGeoLocationResult::GetSpaceGeoLocation() const { return m_geoLocation; }

void PointOfInterestDtoToSpaceGeoLocation(chs_spatial::PointOfInterestDto& dto, SpaceGeoLocation& geoLocation)
{
    geoLocation.SpaceId = dto.GetGroupId();

    if (dto.HasLocation())
    {
        geoLocation.Location.Latitude = dto.GetLocation()->GetLatitude();
        geoLocation.Location.Longitude = dto.GetLocation()->GetLongitude();
    }

    geoLocation.Orientation = dto.HasOrientation() ? dto.GetOrientation() : 0.0f;

    if (dto.HasGeofence())
    {
        const auto& geoFence = dto.GetGeofence();
        geoLocation.GeoFence = csp::common::Array<csp::systems::GeoLocation>(geoFence.size());

        for (size_t idx = 0; idx < geoFence.size(); ++idx)
        {
            csp::systems::GeoLocation geoFenceLocation;
            geoFenceLocation.Latitude = geoFence[idx]->GetLatitude();
            geoFenceLocation.Longitude = geoFence[idx]->GetLongitude();

            geoLocation.GeoFence[idx] = geoFenceLocation;
        }
    }
}

void SpaceGeoLocationResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* geoLocationPoiResponse = static_cast<chs_spatial::PointOfInterestDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        geoLocationPoiResponse->FromJson(response->GetPayload().GetContent());

        m_hasGeoLocation = true;

        m_geoLocation.m_id = geoLocationPoiResponse->GetId();
        PointOfInterestDtoToSpaceGeoLocation(*geoLocationPoiResponse, m_geoLocation);
    }
}

void SpaceGeoLocationCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* geoLocationPoIsResponse = static_cast<csp::services::DtoArray<chs_spatial::PointOfInterestDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        geoLocationPoIsResponse->FromJson(response->GetPayload().GetContent());

        std::vector<chs_spatial::PointOfInterestDto>& poiDtos = geoLocationPoIsResponse->GetArray();
        m_geoLocations = Array<SpaceGeoLocation>(poiDtos.size());

        for (size_t idx = 0; idx < poiDtos.size(); ++idx)
        {
            SpaceGeoLocation geoLocation;
            geoLocation.m_id = poiDtos[idx].GetId();
            PointOfInterestDtoToSpaceGeoLocation(poiDtos[idx], geoLocation);
            m_geoLocations[idx] = geoLocation;
        }
    }
}

} // namespace csp::systems
