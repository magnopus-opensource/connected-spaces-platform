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

void GroupLiteDtoToBasicSpace(const chs_users::GroupLiteDto& Dto, csp::systems::BasicSpace& BasicSpace)
{
    BasicSpace.Id = Dto.GetId();
    BasicSpace.Name = Dto.GetName();

    if (Dto.HasDiscoverable() && Dto.GetDiscoverable())
    {
        BasicSpace.Attributes |= csp::systems::SpaceAttributes::IsDiscoverable;
    }

    if (Dto.HasRequiresInvite() && Dto.GetRequiresInvite())
    {
        BasicSpace.Attributes |= csp::systems::SpaceAttributes::RequiresInvite;
    }

    if (Dto.HasDescription())
    {
        BasicSpace.Description = Dto.GetDescription();
    }

    if (Dto.HasTags())
    {
        BasicSpace.Tags = csp::common::Convert(Dto.GetTags());
    }
}

} // namespace

namespace csp::systems
{

void GroupDtoToSpace(const chs_users::GroupDto& Dto, csp::systems::Space& Space)
{
    Space.Id = Dto.GetId();
    Space.CreatedBy = Dto.GetCreatedBy();
    Space.CreatedAt = Dto.GetCreatedAt();
    Space.OwnerId = Dto.GetGroupOwnerId();
    Space.Name = Dto.GetName();

    if (Dto.HasDiscoverable() && Dto.GetDiscoverable())
    {
        Space.Attributes |= csp::systems::SpaceAttributes::IsDiscoverable;
    }

    if (Dto.HasRequiresInvite() && Dto.GetRequiresInvite())
    {
        Space.Attributes |= csp::systems::SpaceAttributes::RequiresInvite;
    }

    if (Dto.HasDescription())
    {
        Space.Description = Dto.GetDescription();
    }

    if (Dto.HasTags())
    {
        Space.Tags = csp::common::Convert(Dto.GetTags());
    }

    if (Dto.HasUsers())
    {
        Space.UserIds = csp::common::Convert(Dto.GetUsers());
    }

    if (Dto.HasModerators())
    {
        Space.ModeratorIds = csp::common::Convert(Dto.GetModerators());
    }

    if (Dto.HasBannedUsers())
    {
        Space.BannedUserIds = csp::common::Convert(Dto.GetBannedUsers());
    }
}

bool Space::UserIsKnownToSpace(const csp::common::String UserId) const
{
    return std::any_of(UserIds.cbegin(), UserIds.cend(), [UserId](const csp::common::String& id) { return id == UserId; })
        || std::any_of(ModeratorIds.cbegin(), ModeratorIds.cend(), [UserId](const csp::common::String& id) { return id == UserId; })
        || UserId == OwnerId;
}

bool BasicSpace::operator==(const BasicSpace& Other) const
{
    return Id == Other.Id && Name == Other.Name && Description == Other.Description && Attributes == Other.Attributes && Tags == Other.Tags;
}

bool Space::operator==(const Space& Other) const
{
    return BasicSpace::operator==(Other) && CreatedBy == Other.CreatedBy && CreatedAt == Other.CreatedAt && OwnerId == Other.OwnerId
        && UserIds == Other.UserIds && ModeratorIds == Other.ModeratorIds && BannedUserIds == Other.BannedUserIds;
}

bool BasicSpace::operator!=(const BasicSpace& Other) const { return !(*this == Other); }
bool Space::operator!=(const Space& Other) const { return !(*this == Other); }

const Space& SpaceResult::GetSpace() const { return Space; }

const csp::common::String& SpaceResult::GetSpaceCode() const { return SpaceCode; }

void SpaceResult::SetSpace(const csp::systems::Space& InSpace) { Space = InSpace; }

void SpaceResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* GroupResponse = static_cast<chs_users::GroupDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        GroupResponse->FromJson(Response->GetPayload().GetContent());
        GroupDtoToSpace(*GroupResponse, Space);

        if (GroupResponse->HasGroupCode())
        {
            SpaceCode = GroupResponse->GetGroupCode();
        }
    }
}

Array<Space>& SpacesResult::GetSpaces() { return Spaces; }

const Array<Space>& SpacesResult::GetSpaces() const { return Spaces; }

void SpacesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* GroupsResponse = static_cast<csp::services::DtoArray<chs_users::GroupDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        GroupsResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from response in our Groups array
        std::vector<chs_users::GroupDto>& GroupArray = GroupsResponse->GetArray();
        Spaces = Array<csp::systems::Space>(GroupArray.size());

        for (size_t i = 0; i < GroupArray.size(); ++i)
        {
            GroupDtoToSpace(GroupArray[i], Spaces[i]);
        }
    }
}

BasicSpace& BasicSpaceResult::GetSpace() { return Space; }

const BasicSpace& BasicSpaceResult::GetSpace() const { return Space; }

void BasicSpaceResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* LiteGroupResponse = static_cast<chs_users::GroupLiteDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        LiteGroupResponse->FromJson(Response->GetPayload().GetContent());

        GroupLiteDtoToBasicSpace(*LiteGroupResponse, Space);
    }
}

Array<BasicSpace>& BasicSpacesResult::GetSpaces() { return Spaces; }

const Array<BasicSpace>& BasicSpacesResult::GetSpaces() const { return Spaces; }

uint64_t BasicSpacesResult::GetTotalCount() const { return ResultTotalCount; }

void BasicSpacesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* GroupsResponse = static_cast<csp::services::DtoArray<chs_users::GroupLiteDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        GroupsResponse->FromJson(Response->GetPayload().GetContent());
        FillResultTotalCount(Response->GetPayload().GetContent());

        // Extract data from response in our Groups array
        std::vector<chs_users::GroupLiteDto>& GroupArray = GroupsResponse->GetArray();
        Spaces = Array<csp::systems::BasicSpace>(GroupArray.size());

        for (size_t i = 0; i < GroupArray.size(); ++i)
        {
            GroupLiteDtoToBasicSpace(GroupArray[i], Spaces[i]);
        }
    }
}

void BasicSpacesResult::FillResultTotalCount(const String& JsonContent)
{
    assert(JsonContent.c_str());

    rapidjson::Document JsonDoc;
    rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(JsonDoc, JsonContent, "BasicSpacesResult::FillResultTotalCount");
    if (!ok)
    {
        return;
    }

    ResultTotalCount = 0;
    if (JsonDoc.HasMember("itemTotalCount"))
    {
        rapidjson::Value& Val = JsonDoc["itemTotalCount"];
        auto TotalCountStr = csp::web::JsonObjectToString(Val);

        uint64_t ConvertedTotalCount = 0;
        const auto result = std::from_chars(TotalCountStr.c_str(), TotalCountStr.c_str() + TotalCountStr.Length(), ConvertedTotalCount);
        if (result.ec == std::errc())
        {
            ResultTotalCount = ConvertedTotalCount;
        }
    }
}

const Map<String, String>& SpaceMetadataResult::GetMetadata() const { return Metadata; }

void SpaceMetadataResult::SetMetadata(const Map<String, String>& InMetadata) { Metadata = InMetadata; }

Array<String>& PendingInvitesResult::GetPendingInvitesEmails() { return PendingInvitesEmailAddresses; }

const Array<String>& PendingInvitesResult::GetPendingInvitesEmails() const { return PendingInvitesEmailAddresses; }

void PendingInvitesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* PendingInvitesResponse = static_cast<csp::services::DtoArray<chs_users::GroupInviteDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        PendingInvitesResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from response in our pending invites array
        std::vector<chs_users::GroupInviteDto>& PendingInvitesArray = PendingInvitesResponse->GetArray();
        PendingInvitesEmailAddresses = Array<String>(PendingInvitesArray.size());

        for (size_t idx = 0; idx < PendingInvitesArray.size(); ++idx)
        {
            PendingInvitesEmailAddresses[idx] = PendingInvitesArray[idx].GetEmail();
        }
    }
}

Array<String>& AcceptedInvitesResult::GetAcceptedInvitesUserIds() { return AcceptedInvitesUserIds; }

const Array<String>& AcceptedInvitesResult::GetAcceptedInvitesUserIds() const { return AcceptedInvitesUserIds; }

void AcceptedInvitesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* AcceptedInvitesResponse = static_cast<csp::services::DtoArray<chs_users::GroupInviteDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AcceptedInvitesResponse->FromJson(Response->GetPayload().GetContent());

        // Extract data from response in our accepted invites array
        std::vector<chs_users::GroupInviteDto>& AcceptedInvitesArray = AcceptedInvitesResponse->GetArray();
        AcceptedInvitesUserIds = Array<String>(AcceptedInvitesArray.size());

        for (size_t idx = 0; idx < AcceptedInvitesArray.size(); ++idx)
        {
            if (AcceptedInvitesArray[idx].HasId())
            {
                AcceptedInvitesUserIds[idx] = AcceptedInvitesArray[idx].GetId();
            }
        }
    }
}

const Map<String, Map<String, String>>& SpacesMetadataResult::GetMetadata() const { return Metadata; }

const Map<String, Array<String>>& SpacesMetadataResult::GetTags() const { return Tags; }

void SpacesMetadataResult::SetMetadata(const Map<String, Map<String, String>>& InMetadata) { Metadata = InMetadata; }

void SpacesMetadataResult::SetTags(const Map<String, Array<String>>& InTags) { Tags = InTags; }

bool SpaceGeoLocation::operator==(const SpaceGeoLocation& Other) const
{
    return SpaceId == Other.SpaceId && Location == Other.Location && Orientation == Other.Orientation && GeoFence == Other.GeoFence && Id == Other.Id;
}

bool SpaceGeoLocation::operator!=(const SpaceGeoLocation& Other) const { return !(*this == Other); }

bool SpaceGeoLocationResult::HasSpaceGeoLocation() const { return HasGeoLocation; }

const SpaceGeoLocation& SpaceGeoLocationResult::GetSpaceGeoLocation() const { return GeoLocation; }

void PointOfInterestDtoToSpaceGeoLocation(chs_spatial::PointOfInterestDto& Dto, SpaceGeoLocation& GeoLocation)
{
    GeoLocation.SpaceId = Dto.GetGroupId();

    if (Dto.HasLocation())
    {
        GeoLocation.Location.Latitude = Dto.GetLocation()->GetLatitude();
        GeoLocation.Location.Longitude = Dto.GetLocation()->GetLongitude();
    }

    GeoLocation.Orientation = Dto.HasOrientation() ? Dto.GetOrientation() : 0.0f;

    if (Dto.HasGeofence())
    {
        const auto& GeoFence = Dto.GetGeofence();
        GeoLocation.GeoFence = csp::common::Array<csp::systems::GeoLocation>(GeoFence.size());

        for (size_t idx = 0; idx < GeoFence.size(); ++idx)
        {
            csp::systems::GeoLocation GeoFenceLocation;
            GeoFenceLocation.Latitude = GeoFence[idx]->GetLatitude();
            GeoFenceLocation.Longitude = GeoFence[idx]->GetLongitude();

            GeoLocation.GeoFence[idx] = GeoFenceLocation;
        }
    }
}

void SpaceGeoLocationResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* GeoLocationPOIResponse = static_cast<chs_spatial::PointOfInterestDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        GeoLocationPOIResponse->FromJson(Response->GetPayload().GetContent());

        HasGeoLocation = true;

        GeoLocation.Id = GeoLocationPOIResponse->GetId();
        PointOfInterestDtoToSpaceGeoLocation(*GeoLocationPOIResponse, GeoLocation);
    }
}

void SpaceGeoLocationCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* GeoLocationPOIsResponse = static_cast<csp::services::DtoArray<chs_spatial::PointOfInterestDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        GeoLocationPOIsResponse->FromJson(Response->GetPayload().GetContent());

        std::vector<chs_spatial::PointOfInterestDto>& POIDtos = GeoLocationPOIsResponse->GetArray();
        GeoLocations = Array<SpaceGeoLocation>(POIDtos.size());

        for (size_t idx = 0; idx < POIDtos.size(); ++idx)
        {
            SpaceGeoLocation GeoLocation;
            GeoLocation.Id = POIDtos[idx].GetId();
            PointOfInterestDtoToSpaceGeoLocation(POIDtos[idx], GeoLocation);
            GeoLocations[idx] = GeoLocation;
        }
    }
}

} // namespace csp::systems
