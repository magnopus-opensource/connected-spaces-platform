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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

class Space;

enum class SpaceUserRole
{
    Owner,
    Moderator,
    User,
    Invalid
};

/// @ingroup Space System
/// @brief Data representation of User Roles inside a space
class CSP_API UserRoleInfo
{
public:
    UserRoleInfo() = default;
    UserRoleInfo(const UserRoleInfo& Other) = default;

    csp::common::String UserId;
    SpaceUserRole UserRole;
};

/// @ingroup Space System
/// @brief Data representation of roles for an invited user inside a space
class CSP_API InviteUserRoleInfo
{
public:
    InviteUserRoleInfo() = default;
    InviteUserRoleInfo(const InviteUserRoleInfo& Other) = default;

    csp::common::String UserEmail;
    SpaceUserRole UserRole;
};

/// @ingroup Space System
/// @brief Data representation of roles for a group of invited users, the email link and the destination link to be included in the invite emails
class CSP_API InviteUserRoleInfoCollection
{
public:
    InviteUserRoleInfoCollection() = default;
    InviteUserRoleInfoCollection(const InviteUserRoleInfoCollection& Other) = default;

    csp::common::String EmailLinkUrl;
    csp::common::String SignupUrl;
    csp::common::Array<InviteUserRoleInfo> InviteUserRoleInfos;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of User Roles information.
class CSP_API UserRoleCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    csp::common::Array<UserRoleInfo>& GetUsersRoles();
    const csp::common::Array<UserRoleInfo>& GetUsersRoles() const;

private:
    UserRoleCollectionResult(void*) {};
    UserRoleCollectionResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};
    UserRoleCollectionResult() {};

    void FillUsersRoles(const Space& Space, const csp::common::Array<csp::common::String> RequestedUserIds);

    csp::common::Array<UserRoleInfo> UserRoles;
};

typedef std::function<void(const UserRoleCollectionResult& Result)> UserRoleCollectionCallback;

} // namespace csp::systems
