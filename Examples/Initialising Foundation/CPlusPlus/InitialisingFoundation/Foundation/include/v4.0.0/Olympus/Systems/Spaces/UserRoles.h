#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Assets/AssetCollection.h"

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

class Space;

enum class SpaceUserRole
{
    Owner,
    Moderator,
    User
};

/// @ingroup Space System
/// @brief Data representation of User Roles inside a space
class OLY_API UserRoleInfo
{
public:
    UserRoleInfo() = default;
    UserRoleInfo(const UserRoleInfo& Other) = default;

    oly_common::String UserId;
    SpaceUserRole UserRole;
};

/// @ingroup Space System
/// @brief Data representation of User Roles inside a space
class OLY_API InviteUserRoleInfo
{
public:
    InviteUserRoleInfo() = default;
    InviteUserRoleInfo(const InviteUserRoleInfo& Other) = default;

    oly_common::String UserEmail;
    SpaceUserRole UserRole;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of User Roles information.
class OLY_API UserRoleCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    oly_common::Array<UserRoleInfo>& GetUsersRoles();
    const oly_common::Array<UserRoleInfo>& GetUsersRoles() const;

private:
    UserRoleCollectionResult(void*) {};
    UserRoleCollectionResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    UserRoleCollectionResult() {};

    void FillUsersRoles(const Space& Space, const oly_common::Array<oly_common::String> RequestedUserIds);

    oly_common::Array<UserRoleInfo> UserRoles;
};

typedef std::function<void(const UserRoleCollectionResult& Result)> UserRoleCollectionCallback;

} // namespace oly_systems
