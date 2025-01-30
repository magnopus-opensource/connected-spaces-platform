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
#pragma once

#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @brief A basic class abstraction for a user profile, including User Id and name, a display name, avatar information
/// and the users latest platform used.
class CSP_API BasicProfile
{
public:
    csp::common::String UserId;
    csp::common::String UserName;
    csp::common::String DisplayName;
    csp::common::String AvatarId;
    csp::common::String LastPlatform;
};

/// @brief Data structure for a full user profile, which incudes user email, roles, and data for creation and update history.
class CSP_API Profile : public BasicProfile
{
public:
    Profile();

    csp::common::String Email;
    bool IsEmailConfirmed;
    csp::common::Array<csp::common::String> Roles;
    csp::common::String LastDeviceId;
    csp::common::String CreatedBy;
    csp::common::String CreatedAt;
    csp::common::String UpdatedBy;
    csp::common::String UpdatedAt;
};

/// @brief Result structure for a Profile request
class CSP_API ProfileResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    Profile& GetProfile();
    const Profile& GetProfile() const;

private:
    ProfileResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Profile Profile;
};

/// @brief Result structure for a BasicProfile request
class CSP_API BasicProfilesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    csp::common::Array<BasicProfile>& GetProfiles();
    const csp::common::Array<BasicProfile>& GetProfiles() const;

private:
    BasicProfilesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<BasicProfile> Profiles;
};

typedef std::function<void(const ProfileResult& Result)> ProfileResultCallback;
typedef std::function<void(const BasicProfilesResult& Result)> BasicProfilesResultCallback;

} // namespace csp::systems
