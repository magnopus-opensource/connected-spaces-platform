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


namespace csp::multiplayer
{

class SignalRConnection;

} // namespace csp::multiplayer

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


	/// @brief Callback that will be fired when a new member joins an Organization.
	/// Event will be received by member who joined Organization and the Organization Admin/Owner.
	/// @param csp::Common::String : Id of the new member.
	typedef std::function<void(csp::common::String)> MemberJoinedOrganizationCallback;

	/// @brief Sets a callback to be executed when a member joins an Organization.
	/// Only one callback may be registered, calling this function again will override whatever was previously set.
	/// @param Callback MemberJoinedOrganizationCallback : the callback to execute.
	CSP_EVENT void SetMemberJoinedOrganizationCallback(MemberJoinedOrganizationCallback Callback);

	/// @brief Invites a given email to the User's Organization.
	/// Only a User with an Admin or Owner Organization role can invite people to the organization. If the User does not have the required role their
	/// call will be rejected.
	/// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization the user should be added to. If no Id is specified,
	/// the Id of the Organization the user is currently authenticated against will be used.
	/// @param Email csp::common::String : Email to invite to the Organization.
	/// @param OrganizationRoles csp::common::Array<EOrganizationRole> : The role/s in the Organization the invited User is to have.
	/// @param EmailLinkUrl csp::common::Optional<csp::common::String> : link that will be provided in the invite email
	/// @param SignupUrl csp::common::Optional<csp::common::String> : destination link that will be provided in the invite email
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void InviteToOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
											   const csp::common::String& Email,
											   const csp::common::Array<EOrganizationRole>& OrganizationRoles,
											   const csp::common::Optional<csp::common::String>& EmailLinkUrl,
											   const csp::common::Optional<csp::common::String>& SignupUrl,
											   NullResultCallback Callback);

	/// @brief Invites all the given emails to the User's Organization.
	/// Only a User with an Admin or Owner Organization role can invite people to the organization. If the User does not have the required role their
	/// call will be rejected.
	/// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization the users should be added to. If no Id is specified,
	/// the Id of the Organization the user is currently authenticated against will be used.
	/// @param InviteUsers InviteOrganizationRoleCollection : Collection containing the EmailLinkUrl and SignupUrl as well as the emails and
	/// Organization role/s of the Users to be invited.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void BulkInviteToOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
												   const InviteOrganizationRoleCollection& InviteUsers,
												   NullResultCallback Callback);

	/// @brief Retrieves the Organisation User Role information for the User Ids that have been passed in.
	/// Only a User with an Admin or Owner Organization role can request the role information for other Organization members.
	/// A User without these roles can only request information about their own Organization role and should pass an array containing only their own
	/// User Id.
	/// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization you want to get user roles for. If no Id is
	/// specified, the Id of the Organization the user is currently authenticated against will be used.
	/// @param UserIds csp::common::Array<csp::common::String> : Array of User Ids for which the Organization User Roles will be retrieved.
	/// @param Callback OrganizationRolesResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void GetUserRolesInOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
													 const csp::common::Array<csp::common::String>& UserIds,
													 OrganizationRolesResultCallback Callback);

	/// @brief Removes a User from the Organization.
	/// Only a User with an Admin or Owner Organization role can remove other Users from the Organization. If the user does not have the required role
	/// their call will be rejected. Anyone can remove themselves from an Organization.
	/// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization you want to remove a user from. If no Id is
	/// specified, the Id of the Organization the user is currently authenticated against will be used.
	/// @param UserId csp::common::String : Unique ID of User.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void RemoveUserFromOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
													 const csp::common::String& UserId,
													 NullResultCallback Callback);


private:
	OrganizationSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT OrganizationSystem(csp::web::WebClient* InWebClient);

	MemberJoinedOrganizationCallback InternalMemberJoinedOrganizationCallback;

	csp::services::ApiBase* OrganizationApi;
};
} // namespace csp::systems
