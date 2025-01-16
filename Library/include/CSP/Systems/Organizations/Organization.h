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
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{
// enum class TierNames;

// @ingroup Organization System
/// @brief Data representation of User Role in Organization.
enum class EOrganizationRole
{
    Member,
    Administrator,
    Owner
};

/// @ingroup Organization System
/// @brief Data representation of a User's Organization invite.
class CSP_API InviteOrganizationRoleInfo
{
public:
    InviteOrganizationRoleInfo() = default;

    csp::common::String UserEmail;
    csp::common::Array<EOrganizationRole> OrganizationRoles;
};

/// @ingroup Organization System
/// @brief Data representation of a collection of User Organization invites.
class CSP_API InviteOrganizationRoleCollection
{
public:
    InviteOrganizationRoleCollection() = default;

    csp::common::String EmailLinkUrl;
    csp::common::String SignupUrl;
    csp::common::Array<InviteOrganizationRoleInfo> InvitedUserRoles;
};

/// @ingroup Organization System
/// @brief Data representation of a User's role within an Organization.
class CSP_API OrganizationRoleInfo
{
public:
    OrganizationRoleInfo() = default;

    csp::common::String UserId;
    csp::common::Array<EOrganizationRole> OrganizationRoles;
};

/// @ingroup Organization System
/// @brief Data representation of an Organization.
class CSP_API Organization
{
public:
    Organization() = default;
    Organization(const Organization& Other) = default;

    csp::common::String Id;
    csp::common::String OwnerId;
    csp::common::String CreatedAt;
    csp::common::String CreatedBy;
    csp::common::String Name;
    csp::common::Array<OrganizationRoleInfo> Members;
    int32_t SpaceCount;
};

/// @ingroup Organization System
/// @brief Data class used to contain Organization object.
class CSP_API OrganizationResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    CSP_NO_EXPORT OrganizationResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    /// @brief Retrieves the Organization result.
    /// @return const Organization& : Organization object.
    const Organization& GetOrganization() const;

private:
    OrganizationResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Organization Organization;
};

/// @ingroup Organization System
/// @brief Data class used to contain a Users Organization Role Info object.
class CSP_API OrganizationRolesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    CSP_NO_EXPORT OrganizationRolesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    /// @brief Retrieves the Organization Role Info result.
    /// @return const common::Array<OrganizationRoleInfo>& : Array of Organization Role Info objects.
    const csp::common::Array<OrganizationRoleInfo>& GetOrganizationRoleInfo() const;

protected:
    CSP_NO_EXPORT OrganizationRolesResult(const csp::systems::ResultBase& InResult)
        : csp::systems::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

private:
    OrganizationRolesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<OrganizationRoleInfo> OrganizationRoleInfos;
};

/// @brief Callback Organization Result object.
/// @param Result OrganizationResult : result class
typedef std::function<void(const OrganizationResult& Result)> OrganizationResultCallback;

/// @brief Callback Organization Role Result object.
/// @param Result OrganizationRoleResult : result class
typedef std::function<void(const OrganizationRolesResult& Result)> OrganizationRolesResultCallback;

} // namespace csp::systems
