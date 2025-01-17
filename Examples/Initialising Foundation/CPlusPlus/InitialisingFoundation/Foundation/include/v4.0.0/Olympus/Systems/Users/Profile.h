#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

class OLY_API BasicProfile
{
public:
    oly_common::String UserId;
    oly_common::String UserName;
    oly_common::String DisplayName;
    oly_common::String AvatarId;
    oly_common::String LastPlatform;
};

class OLY_API Profile : public BasicProfile
{
public:
    Profile();

    oly_common::String Email;
    bool IsEmailConfirmed;
    oly_common::Array<oly_common::String> Roles;
    oly_common::String LastDeviceId;
    oly_common::String CreatedBy;
    oly_common::String CreatedAt;
    oly_common::String UpdatedBy;
    oly_common::String UpdatedAt;
};

class OLY_API ProfileResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    Profile& GetProfile();
    const Profile& GetProfile() const;

private:
    ProfileResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    Profile Profile;
};

class OLY_API BasicProfilesResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    oly_common::Array<BasicProfile>& GetProfiles();
    const oly_common::Array<BasicProfile>& GetProfiles() const;

private:
    BasicProfilesResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<BasicProfile> Profiles;
};

typedef std::function<void(const ProfileResult& Result)> ProfileResultCallback;
typedef std::function<void(const BasicProfilesResult& Result)> BasicProfilesResultCallback;

} // namespace oly_systems
