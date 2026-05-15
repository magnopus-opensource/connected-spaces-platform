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

#include "Common/Web/HttpResponse.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"

namespace chs = csp::services::generated::userservice;

namespace
{

void ProfileLiteDtoToBasicProfile(const chs::ProfileLiteDto& dto, csp::systems::BasicProfile& profile)
{
    profile.UserId = dto.GetId();
    profile.AvatarId = dto.GetAvatarId();

    if (dto.HasDisplayName())
    {
        profile.DisplayName = dto.GetDisplayName();
    }
}

void ProfileDtoToProfile(const chs::ProfileDto& dto, csp::systems::Profile& profile)
{
    profile.UserId = dto.GetId();

    if (dto.HasEmail())
    {
        profile.Email = dto.GetEmail();
    }

    if (dto.HasLastDeviceId())
    {
        profile.LastDeviceId = dto.GetLastDeviceId();
    }

    profile.AvatarId = dto.GetAvatarId();

    if (dto.HasDisplayName())
    {
        profile.DisplayName = dto.GetDisplayName();
    }

    // TODO: Add PersonalityType and PersonalityValuesType if Mag requests it. They are currently ignored as they seem to be Parasol-specific

    if (dto.HasIsEmailConfirmed())
    {
        profile.IsEmailConfirmed = dto.GetIsEmailConfirmed();
    }

    if (dto.HasRoles())
    {
        auto responseRoles = dto.GetRoles();
        profile.Roles = csp::common::Array<csp::common::String>(responseRoles.size());

        for (size_t i = 0; i < responseRoles.size(); ++i)
        {
            profile.Roles[i] = responseRoles[i];
        }
    }

    if (dto.HasCreatedBy())
    {
        profile.CreatedBy = dto.GetCreatedBy();
    }

    if (dto.HasCreatedAt())
    {
        profile.CreatedAt = dto.GetCreatedAt();
    }

    if (dto.HasUpdatedBy())
    {
        profile.UpdatedBy = dto.GetUpdatedBy();
    }

    if (dto.HasUpdatedAt())
    {
        profile.UpdatedAt = dto.GetUpdatedAt();
    }
}

} // namespace

namespace csp::systems
{

bool BasicProfile::operator==(const BasicProfile& other) const
{
    return UserId == other.UserId && DisplayName == other.DisplayName && AvatarId == other.AvatarId;
}

bool BasicProfile::operator!=(const BasicProfile& other) const { return !(*this == other); }

Profile::Profile()
    : IsEmailConfirmed(false)
{
}

bool Profile::operator==(const Profile& other) const
{
    return BasicProfile::operator==(other) && Email == other.Email && IsEmailConfirmed == other.IsEmailConfirmed && Roles == other.Roles
        && LastDeviceId == other.LastDeviceId && CreatedBy == other.CreatedBy && CreatedAt == other.CreatedAt && UpdatedBy == other.UpdatedBy
        && UpdatedAt == other.UpdatedAt;
}

bool Profile::operator!=(const Profile& other) const { return !(*this == other); }

void ProfileResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto profileResponse = static_cast<chs::ProfileDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        profileResponse->FromJson(response->GetPayload().GetContent());

        ProfileDtoToProfile(*profileResponse, m_profile);
    }
}

Profile& ProfileResult::GetProfile() { return m_profile; }

const Profile& ProfileResult::GetProfile() const { return m_profile; }

void BasicProfilesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* profileDataResponse = static_cast<csp::services::DtoArray<chs::ProfileLiteDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        profileDataResponse->FromJson(response->GetPayload().GetContent());

        const std::vector<chs::ProfileLiteDto>& profileArray = profileDataResponse->GetArray();
        m_profiles = csp::common::Array<csp::systems::BasicProfile>(profileArray.size());

        for (size_t i = 0; i < profileArray.size(); ++i)
        {
            ProfileLiteDtoToBasicProfile(profileArray[i], m_profiles[i]);
        }
    }
}

csp::common::Array<BasicProfile>& BasicProfilesResult::GetProfiles() { return m_profiles; }

const csp::common::Array<BasicProfile>& BasicProfilesResult::GetProfiles() const { return m_profiles; }
} // namespace csp::systems
