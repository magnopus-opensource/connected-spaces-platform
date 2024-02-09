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

#include "CSP/Systems/Organizations/Organization.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"
#include "Services/UserService/Dto.h"


namespace csp::systems
{
/// @ingroup Quota System
/// @brief Public facing system that allows interfacing with the Organization System.
/// .
class CSP_API CSP_NO_DISPOSE OrganizationSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */

public:
	~OrganizationSystem();

	/// @brief Retrieves the User's Organization by its cached Organization Id.
	/// If this request is made by a User with an Owner or Admin Organization role, the resultant Organization object will contain an array of
	/// OrganizationRoleInfo objects for each Organization member. If the request is made by a User who does not have the Owner or Admin role, the
	/// resultant Organization object will contain an array with a single OrganizationRoleInfo object which represents them.
	/// @param Callback OrganizationResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void GetOrganization(OrganizationResultCallback Callback);

	/// @brief Get the cached Id of the User's Organization.
	/// @return The cached Id of the Organization the User belongs to.
	const csp::common::String& GetOrganizationId() const;

	/// @brief Updates the name and/or the description of the User's Organization.
	/// Only a User with an Admin or Owner Organization role can update an Organization. If the user does not have the required Organization role
	/// their call will be rejected.
	/// @param Name csp::common::Optional<csp::common::String> : If a new name is provided it will be used to update the Organization name
	/// @param Description csp::common::Optional<csp::common::String> : If a new description is provided it will be used to update the Organization
	/// description description
	/// @param Callback OrganizationResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void UpdateOrganization(const csp::common::Optional<csp::common::String>& Name,
											 const csp::common::Optional<csp::common::String>& Description,
											 OrganizationResultCallback Callback);

	/// @brief Deactivates the Organization the User belongs to.
	/// This call performs a soft-delete of the Organization and will allow for Organization reactivation in the future.
	/// Only A User with an Owner Organization role can delete an Organization. If the user does not have the required role their call will be
	/// rejected.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void DeactivateOrganization(NullResultCallback Callback);

	/// @brief Invites a given email to the User's Organization.
	/// Only a User with an Admin or Owner Organization role can invite people to the organization. If the User does not have the required role their
	/// call will be rejected.
	/// @param Email csp::common::String : Email to invite to the Organization.
	/// @param OrganizationRoles csp::common::Array<EOrganizationRole> : The role/s in the Organization the invited User is to have.
	/// @param EmailLinkUrl csp::common::Optional<csp::common::String> : link that will be provided in the invite email
	/// @param SignupUrl csp::common::Optional<csp::common::String> : destination link that will be provided in the invite email
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void InviteToOrganization(const csp::common::String& Email,
											   const csp::common::Array<EOrganizationRole>& OrganizationRoles,
											   const csp::common::Optional<csp::common::String>& EmailLinkUrl,
											   const csp::common::Optional<csp::common::String>& SignupUrl,
											   NullResultCallback Callback);

	/// @brief Invites all the given emails to the User's Organization.
	/// Only a User with an Admin or Owner Organization role can invite people to the organization. If the User does not have the required role their
	/// call will be rejected.
	/// @param InviteUsers InviteOrganizationRoleCollection : Collection containing the EmailLinkUrl and SignupUrl as well as the emails and
	/// Organization role/s of the Users to be invited.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void BulkInviteToOrganization(const InviteOrganizationRoleCollection& InviteUsers, NullResultCallback Callback);

	/// @brief Retrieves the Organisation User Role information for the User Ids that have been passed in.
	/// Only a User with an Admin or Owner Organization role can request the role information for other Organization members.
	/// A User without these roles can only request information about their own Organization role and should pass an array containing only their own
	/// User Id.
	/// @param UserIds csp::common::Array<csp::common::String> : Array of User Ids for which the Organization User Roles will be retrieved.
	/// @param Callback OrganizationRolesResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void GetUserRolesInOrganization(const csp::common::Array<csp::common::String>& UserIds,
													 OrganizationRolesResultCallback Callback);

	/// @brief Removes a User from the Organization.
	/// Only a User with an Admin or Owner Organization role can remove other Users from the Organization. If the user does not have the required role
	/// their call will be rejected. Anyone can remove themselves from an Organization.
	/// @param UserId csp::common::String : Unique ID of User.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void RemoveUserFromOrganization(const csp::common::String& UserId, NullResultCallback Callback);

	/// @brief Updates the Organization Role/s for a particular User.
	/// Only a User with an Admin or Owner Organization role can update a Users role/s in the Organization. If the User does not have the required
	/// role their call will be rejected.
	/// @param UserId csp::common::String : Unique Id of the User.
	/// @param OrganizationRoles csp::common:Array<EOrganizationRole> : The new Organization Role/s for the User.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void UpdateUserRolesInOrganization(const csp::common::String& UserId,
														const csp::common::Array<EOrganizationRole>& OrganizationRoles,
														NullResultCallback Callback);


private:
	OrganizationSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT OrganizationSystem(csp::web::WebClient* InWebClient);

	bool UserHasPermissions(EOrganizationRole RequiredRole);
	std::vector<std::shared_ptr<csp::services::generated::userservice::OrganizationInviteDto>>
		GenerateOrganizationInvites(const common::Array<systems::InviteOrganizationRoleInfo> InviteUsers);

	csp::services::ApiBase* OrganizationAPI;
	Organization CurrentOrganization;
	csp::common::String CurrentOrganizationId;
	csp::common::Array<EOrganizationRole> CurrentOrganizationRoles;
};
} // namespace csp::systems
