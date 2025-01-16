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

    /// @brief Create a new Organization.
    /// Only a User with tenant admin permissions can create an Organization.
    /// If the user does not have the required permissions their call will be rejected.
    /// @param OrganizationOwnerId csp::common::Optional<csp::common::String> : Id of the Organization owner.
    /// @param OrganizationName csp::common::String : The Organization name
    /// @param Callback OrganizationResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void CreateOrganization(
        const csp::common::String& OrganizationOwnerId, const csp::common::String& OrganizationName, OrganizationResultCallback Callback);

    /// @brief Retrieves Organization info for the specified Organization.
    /// If this request is made by a User with an Owner or Admin Organization role, the resultant Organization object will contain an array of
    /// OrganizationRoleInfo objects for each Organization member. If the request is made by a User who does not have the Owner or Admin role, the
    /// resultant Organization object will contain an array with a single OrganizationRoleInfo object which represents them.
    /// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization to retrieve information on. If no Id is specified,
    /// the Id of the Organization the user is currently authenticated against will be used.
    /// @param Callback OrganizationResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetOrganization(const csp::common::Optional<csp::common::String>& OrganizationId, OrganizationResultCallback Callback);

    /// @brief Get the Id of the Organization the user is authenticated against.
    /// @return The Id of the Organization the User belongs to.
    const csp::common::String& GetCurrentOrganizationId() const;

    /// @brief Updates the name and/or the description of the specified Organization.
    /// Only a User with an Organization Owner role can update an Organization. If the user does not have the required Organization role
    /// their call will be rejected.
    /// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization to update. If no Id is specified,
    /// the Id of the Organization the user is currently authenticated against will be used.
    /// @param Name csp::common::String : The new Organization name
    /// @param Callback OrganizationResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void UpdateOrganization(
        const csp::common::Optional<csp::common::String>& OrganizationId, const csp::common::String& Name, OrganizationResultCallback Callback);

    /// @brief Deactivates the specified Organization.
    /// This call performs a soft-delete of the Organization and will allow for Organization reactivation in the future.
    /// Only A User with owner-level permissions can deactivate an Organization. If the user does not have the required role their call will be
    /// rejected.
    /// @param OrganizationId csp::common::String : Id of the Organization to deactivate.
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void DeactivateOrganization(const csp::common::String& OrganizationId, NullResultCallback Callback);

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
    CSP_ASYNC_RESULT void InviteToOrganization(const csp::common::Optional<csp::common::String>& OrganizationId, const csp::common::String& Email,
        const csp::common::Array<EOrganizationRole>& OrganizationRoles, const csp::common::Optional<csp::common::String>& EmailLinkUrl,
        const csp::common::Optional<csp::common::String>& SignupUrl, NullResultCallback Callback);

    /// @brief Invites all the given emails to the User's Organization.
    /// Only a User with an Admin or Owner Organization role can invite people to the organization. If the User does not have the required role their
    /// call will be rejected.
    /// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization the users should be added to. If no Id is specified,
    /// the Id of the Organization the user is currently authenticated against will be used.
    /// @param InviteUsers InviteOrganizationRoleCollection : Collection containing the EmailLinkUrl and SignupUrl as well as the emails and
    /// Organization role/s of the Users to be invited.
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void BulkInviteToOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
        const InviteOrganizationRoleCollection& InviteUsers, NullResultCallback Callback);

    /// @brief Retrieves the Organisation User Role information for the User Ids that have been passed in.
    /// Only a User with an Admin or Owner Organization role can request the role information for other Organization members.
    /// A User without these roles can only request information about their own Organization role and should pass an array containing only their own
    /// User Id.
    /// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization you want to get user roles for. If no Id is
    /// specified, the Id of the Organization the user is currently authenticated against will be used.
    /// @param UserIds csp::common::Array<csp::common::String> : Array of User Ids for which the Organization User Roles will be retrieved.
    /// @param Callback OrganizationRolesResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetUserRolesInOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
        const csp::common::Array<csp::common::String>& UserIds, OrganizationRolesResultCallback Callback);

    /// @brief Removes a User from the Organization.
    /// Only a User with an Admin or Owner Organization role can remove other Users from the Organization. If the user does not have the required role
    /// their call will be rejected. Anyone can remove themselves from an Organization.
    /// @param OrganizationId csp::common::Optional<csp::common::String> : Id of the Organization you want to remove a user from. If no Id is
    /// specified, the Id of the Organization the user is currently authenticated against will be used.
    /// @param UserId csp::common::String : Unique ID of User.
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void RemoveUserFromOrganization(
        const csp::common::Optional<csp::common::String>& OrganizationId, const csp::common::String& UserId, NullResultCallback Callback);

private:
    OrganizationSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT OrganizationSystem(csp::web::WebClient* InWebClient);

    MemberJoinedOrganizationCallback InternalMemberJoinedOrganizationCallback;

    csp::services::ApiBase* OrganizationApi;
};
} // namespace csp::systems
