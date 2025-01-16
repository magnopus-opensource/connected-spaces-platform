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
#include "CSP/Systems/Users/Profile.h"

#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"
#include "Web/HttpResponse.h"

namespace chs = csp::services::generated::userservice;

namespace
{

void ProfileLiteDtoToBasicProfile(const chs::ProfileLiteDto& Dto, csp::systems::BasicProfile& Profile)
{
    Profile.UserId = Dto.GetId();
    Profile.AvatarId = Dto.GetAvatarId();

    if (Dto.HasUserName())
    {
        Profile.UserName = Dto.GetUserName();
    }

    if (Dto.HasDisplayName())
    {
        Profile.DisplayName = Dto.GetDisplayName();
    }

    if (Dto.HasPlatform())
    {
        Profile.LastPlatform = Dto.GetPlatform();
    }
}

void ProfileDtoToProfile(const chs::ProfileDto& Dto, csp::systems::Profile& Profile)
{
    Profile.UserId = Dto.GetId();

    if (Dto.HasEmail())
    {
        Profile.Email = Dto.GetEmail();
    }

    if (Dto.HasLastDeviceId())
    {
        Profile.LastDeviceId = Dto.GetLastDeviceId();
    }

    if (Dto.HasLastPlatform())
    {
        Profile.LastPlatform = Dto.GetLastPlatform();
    }

    Profile.AvatarId = Dto.GetAvatarId();

    if (Dto.HasUserName())
    {
        Profile.UserName = Dto.GetUserName();
    }

    if (Dto.HasDisplayName())
    {
        Profile.DisplayName = Dto.GetDisplayName();
    }

    // TODO: Add PersonalityType and PersonalityValuesType if Mag requests it. They are currently ignored as they seem to be Parasol-specific

    if (Dto.HasIsEmailConfirmed())
    {
        Profile.IsEmailConfirmed = Dto.GetIsEmailConfirmed();
    }

    if (Dto.HasRoles())
    {
        auto ResponseRoles = Dto.GetRoles();
        Profile.Roles = csp::common::Array<csp::common::String>(ResponseRoles.size());

        for (int i = 0; i < ResponseRoles.size(); ++i)
        {
            Profile.Roles[i] = ResponseRoles[i];
        }
    }

    if (Dto.HasCreatedBy())
    {
        Profile.CreatedBy = Dto.GetCreatedBy();
    }

    if (Dto.HasCreatedAt())
    {
        Profile.CreatedAt = Dto.GetCreatedAt();
    }

    if (Dto.HasUpdatedBy())
    {
        Profile.UpdatedBy = Dto.GetUpdatedBy();
    }

    if (Dto.HasUpdatedAt())
    {
        Profile.UpdatedAt = Dto.GetUpdatedAt();
    }
}

} // namespace

namespace csp::systems
{

Profile::Profile()
    : IsEmailConfirmed(false)
{
}

void ProfileResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto ProfileResponse = static_cast<chs::ProfileDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ProfileResponse->FromJson(Response->GetPayload().GetContent());

        ProfileDtoToProfile(*ProfileResponse, Profile);
    }
}

Profile& ProfileResult::GetProfile() { return Profile; }

const Profile& ProfileResult::GetProfile() const { return Profile; }

void BasicProfilesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* ProfileDataResponse = static_cast<csp::services::DtoArray<chs::ProfileLiteDto>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ProfileDataResponse->FromJson(Response->GetPayload().GetContent());

        const std::vector<chs::ProfileLiteDto>& ProfileArray = ProfileDataResponse->GetArray();
        Profiles = csp::common::Array<csp::systems::BasicProfile>(ProfileArray.size());

        for (size_t i = 0; i < ProfileArray.size(); ++i)
        {
            ProfileLiteDtoToBasicProfile(ProfileArray[i], Profiles[i]);
        }
    }
}

csp::common::Array<BasicProfile>& BasicProfilesResult::GetProfiles() { return Profiles; }

const csp::common::Array<BasicProfile>& BasicProfilesResult::GetProfiles() const { return Profiles; }
} // namespace csp::systems
