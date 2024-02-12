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

#include "CSP/Systems/Organizations/OrganizationSystem.h"

#include "CallHelpers.h"
#include "Services/UserService/Api.h"
//#include "Services/UserService/Dto.h"
#include "Memory/Memory.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Systems/ResultHelpers.h"
#include "signalrclient/signalr_value.h"


using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace
{

constexpr const String OrganizationRoleEnumToString(systems::EOrganizationRole Role)
{
	switch (Role)
	{
		case systems::EOrganizationRole::Member:
			return "member";
		case systems::EOrganizationRole::Administrator:
			return "administrator";
		case systems::EOrganizationRole::Owner:
			return "owner";
		default:
			throw std::invalid_argument("Unimplemented role");
	}
}

constexpr const systems::EOrganizationRole OrganizationRoleStringToEnum(String Role)
{
	switch (Role.ToLower())
	{
		case "member":
			return systems::EOrganizationRole::Member;
		case "administrator":
			return systems::EOrganizationRole::Administrator;
		case "owner":
			return systems::EOrganizationRole::Owner;
		default:
			throw std::invalid_argument("Invalid role specified");
	}
}

} // namespace


namespace csp::systems
{

OrganizationSystem::OrganizationSystem() : SystemBase(), OrganizationAPI(nullptr)
{
}

OrganizationSystem::OrganizationSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	OrganizationAPI = CSP_NEW chs::OrganizationApi(InWebClient);
}

OrganizationSystem::~OrganizationSystem()
{
	CSP_DELETE(OrganizationAPI);
}


void OrganizationSystem::GetOrganization(OrganizationResultCallback Callback)
{
	if (CurrentOrganizationId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("There is currently no Organization set.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<OrganizationResult>());

		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

	// static_cast<chs::OrganizationAPI*>(OrganizationAPI)->apiV1OrganizationsOrganizationIdGet(CurrentOrganizationId, ResponseHandler);
}

const csp::common::String& OrganizationSystem::GetOrganizationId() const
{
	return CurrentOrganizationId;
}

void OrganizationSystem::UpdateOrganization(const csp::common::Optional<csp::common::String>& Name,
											const csp::common::Optional<csp::common::String>& Description,
											OrganizationResultCallback Callback)
{
	if (!HasUserPermissions(EOrganizationRole::Administrator))
	{
		CSP_LOG_ERROR_MSG("OrganizationSystem::UpdateOrganization failed: User does not have the required permissions to update the Organization.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	// CSP_PROFILE_SCOPED();

	auto OrganizationInfo = std::make_shared<chs::OrganizationDto>();

	if (Name.HasValue())
	{
		OrganizationInfo->SetName(*Name);
	}

	// if (Description.HasValue())
	//{
	//	OrganizationInfo->SetDescription(*Description);
	// }

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

	// static_cast<chs::OrganizationAPI*>(OrganizationAPI)->apiV1OrganizationsOrganizationIdPut(CurrentOrganizationId, OrganizationInfo,
	// ResponseHandler);
}

void OrganizationSystem::DeactivateOrganization(NullResultCallback Callback)
{
	if (!HasUserPermissions(EOrganizationRole::Owner))
	{
		CSP_LOG_ERROR_MSG("OrganizationSystem::DeactivateOrganization failed: Only the deactivation Owner can deactivate an Organization.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									   nullptr,
																									   csp::web::EResponseCodes::ResponseNoContent);

	// static_cast<chs::OrganizationAPI*>(OrganizationAPI)->apiV1OrganizationsOrganizationIdDelete(CurrentOrganizationId, ResponseHandler);
}

void OrganizationSystem::InviteToOrganization(const csp::common::String& Email,
											  const csp::common::Array<EOrganizationRole>& OrganizationRoles,
											  const csp::common::Optional<csp::common::String>& EmailLinkUrl,
											  const csp::common::Optional<csp::common::String>& SignupUrl,
											  NullResultCallback Callback)
{
	if (!HasUserPermissions(EOrganizationRole::Administrator) || !HasUserPermissions(EOrganizationRole::Owner))
	{
		CSP_LOG_ERROR_MSG(
			"OrganizationSystem::InviteToOrganization failed: User does not have the required permissions to invite a user to the Organization.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	auto OrganizationInviteInfo = std::make_shared<chs::OrganizationInviteDto>();
	OrganizationInviteInfo->SetEmail(Email);

	std::vector<String> UserRoles;
	UserRoles.reserve(OrganizationRoles.Size());

	for (auto i = 0; i < OrganizationRoles.Size(); ++i)
	{
		UserRoles.push_back(OrganizationRoleEnumToString(OrganizationRoles[i]));
	}

	// todo: we need to ensure that everyone has the member role.
	OrganizationInviteInfo->SetRoles(UserRoles);

	auto EmailLinkUrlParam = EmailLinkUrl.HasValue() && !EmailLinkUrl->IsEmpty() ? (*EmailLinkUrl) : std::optional<String>(std::nullopt);
	auto SignupUrlParam	   = SignupUrl.HasValue() && !SignupUrl->IsEmpty() ? (*SignupUrl) : std::optional<String>(std::nullopt);

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									   nullptr,
																									   csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::OrganizationAPI*>(OrganizationAPI)
		->apiV1OrganizationsOrganizationIdEmailInvitesPost(CurrentOrganizationId,
														   std::nullopt,
														   EmailLinkUrlParam,
														   SignupUrlParam,
														   OrganizationInviteInfo,
														   ResponseHandler);
}

void OrganizationSystem::BulkInviteToOrganization(const InviteOrganizationRoleCollection& InviteUsers, NullResultCallback Callback)
{
	if (!HasUserPermissions(EOrganizationRole::Administrator))
	{
		CSP_LOG_ERROR_MSG("OrganizationSystem::BulkInviteToOrganization failed: User does not have the required permissions to bulk invite users to "
						  "the Organization.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	std::vector<std::shared_ptr<chs::OrganizationInviteDto>> OrganizationInvites = GenerateOrganizationInvites(InviteUsers.InvitedUserRoles);

	auto EmailLinkUrlParam = !InviteUsers.EmailLinkUrl.IsEmpty() ? (InviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
	auto SignupUrlParam	   = !InviteUsers.SignupUrl.IsEmpty() ? (InviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									   nullptr,
																									   csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::OrganizationAPI*>(OrganizationAPI)
		->apiV1OrganizationsOrganizationIdEmailInvitesBulkPost(CurrentOrganizationId,
															   std::nullopt,
															   EmailLinkUrlParam,
															   SignupUrlParam,
															   OrganizationInvites,
															   ResponseHandler);
}

void OrganizationSystem::GetUserRolesInOrganization(const csp::common::Array<csp::common::String>& UserIds, OrganizationRolesResultCallback Callback)
{
	std::vector<String> InternalUserIds;
	InternalUserIds.reserve(UserIds.Size());

	for (auto i = 0; i < UserIds.Size(); ++i)
	{
		InternalUserIds.push_back(UserIds[i]);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

	static_cast<chs::OrganizationAPI*>(OrganizationAPI)->apiV1OrganizationsOrganizationIdGet(CurrentOrganizationId, InternalUserIds, ResponseHandler);
}

void OrganizationSystem::RemoveUserFromOrganization(const csp::common::String& UserId, NullResultCallback Callback)
{
	if (!HasUserPermissions(EOrganizationRole::Administrator))
	{
		CSP_LOG_ERROR_MSG("OrganizationSystem::RemoveUserFromOrganization failed: User does not have the required permissions to remove users from "
						  "the Organization.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

	static_cast<chs::OrganizationAPI*>(OrganizationAPI)
		->apiV1OrganizationsOrganizationIdUsersUserIdDelete(CurrentOrganizationId, UserId, ResponseHandler);
}

void OrganizationSystem::UpdateUserRolesInOrganization(const csp::common::String& UserId,
													   const csp::common::Array<EOrganizationRole>& OrganizationRoles,
													   NullResultCallback Callback)
{
	if (!HasUserPermissions(EOrganizationRole::Administrator))
	{
		CSP_LOG_ERROR_MSG(
			"OrganizationSystem::UpdateUserRolesInOrganization failed: User does not have the required permissions to update a User's role.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	std::vector<String> UserRoles;
	UserRoles.reserve(OrganizationRoles.Size());

	for (auto i = 0; i < OrganizationRoles.Size(); ++i)
	{
		UserRoles.push_back(OrganizationRoleEnumToString(OrganizationRoles[i]));
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

	static_cast<chs::OrganizationAPI*>(OrganizationAPI)->apiV1OrganizationsOrganizationIdUserPut(CurrentOrganizationId, UserRoles, ResponseHandler);
}

bool OrganizationSystem::HasUserPermissions(EOrganizationRole RequiredRole)
{
	bool HasRole			   = false;
	size_t OrganizationRoleNum = CurrentOrganizationRoles.Size();

	for (size_t i = 0; i < OrganizationRoleNum; ++i)
	{
		if (CurrentOrganizationRoles[i] == RequiredRole)
		{
			HasRole = true;

			break;
		}
	}

	return HasRole;
}

void OrganizationSystem::SetOrganizationUpdatedCallback(OrganizationUpdatedCallback Callback)
{
	if (InternalOrganizationUpdatedCallback)
	{
		CSP_LOG_WARN_MSG("OrganizationUpdatedCallback has already been set. Previous callback overwritten.");
	}

	InternalOrganizationUpdatedCallback = std::move(Callback);
}

void OrganizationSystem::SetOrganizationDeactivatedCallback(OrganizationDeactivatedCallback Callback)
{
	if (InternalOrganizationDeactivatedCallback)
	{
		CSP_LOG_WARN_MSG("OrganizationDeactivatedCallback has already been set. Previous callback overwritten.");
	}

	InternalOrganizationDeactivatedCallback = std::move(Callback);
}

void OrganizationSystem::SetMemberJoinedOrganizationCallback(MemberJoinedOrganizationCallback Callback)
{
	if (InternalMemberJoinedOrganizationCallback)
	{
		CSP_LOG_WARN_MSG("MemberJoinedOrganizationCallback has already been set. Previous callback overwritten.");
	}

	InternalMemberJoinedOrganizationCallback = std::move(Callback);
}

void OrganizationSystem::SetMemberLeftOrganizationCallback(MemberLeftOrganizationCallback Callback)
{
	if (InternalMemberLeftOrganizationCallback)
	{
		CSP_LOG_WARN_MSG("MemberLeftOrganizationCallback has already been set. Previous callback overwritten.");
	}

	InternalMemberLeftOrganizationCallback = std::move(Callback);
}

void OrganizationSystem::SetUserRoleChangedCallback(UserRoleChangedCallback Callback)
{
	if (InternalUserRoleChangedCallback)
	{
		CSP_LOG_WARN_MSG("UserRoleChangedCallback has already been set. Previous callback overwritten.");
	}

	InternalUserRoleChangedCallback = std::move(Callback);
}

void OrganizationSystem::BindOnOrganizationUpdated()
{
	Connection->On("OnOrganizationUpdated",
				   [this](const signalr::value& Params)
				   {
					   // Params is an array of all params sent, so grab the first
					   // CurrentOrganizationId = Params.as_array()[0].as_string().c_str();

					   if (InternalOrganizationUpdatedCallback)
					   {
						   InternalOrganizationUpdatedCallback();
					   }
					   else
					   {
						   CSP_LOG_WARN_MSG("Organization has been updated but OrganizationUpdatedCallback has not been set!");
					   }
				   });
}

void OrganizationSystem::BindOnOrganizationDeactivated()
{
	Connection->On("OnOrganizationDeactivated",
				   [this](const signalr::value& Params)
				   {
					   // Params is an array of all params sent, so grab the first
					   CurrentOrganizationId = "";

					   if (InternalOrganizationDeactivatedCallback)
					   {
						   InternalOrganizationDeactivatedCallback();
					   }
					   else
					   {
						   CSP_LOG_WARN_MSG("Organization has been deactivated but OrganizationDeactivatedCallback has not been set!");
					   }
				   });
}

void OrganizationSystem::BindOnMemberJoinedOrganization()
{
	Connection->On("OnMemberJoinedOrganization",
				   [this](const signalr::value& Params)
				   {
					   // Params is an array of all params sent, so grab the first
					   String UserId = Params.as_array()[0].as_string().c_str();

					   if (InternalMemberJoinedOrganizationCallback)
					   {
						   InternalMemberJoinedOrganizationCallback(UserId);
					   }
					   else
					   {
						   CSP_LOG_WARN_MSG("Member has joined Organization but MemberJoinedOrganizationCallback has not been set!");
					   }
				   });
}

void OrganizationSystem::BindOnMemberLeftOrganization()
{
	Connection->On("OnMemberLeftOrganization",
				   [this](const signalr::value& Params)
				   {
					   // Params is an array of all params sent, so grab the first
					   String UserId = Params.as_array()[0].as_string().c_str();

					   if (InternalMemberLeftOrganizationCallback)
					   {
						   InternalMemberLeftOrganizationCallback(UserId);
					   }
					   else
					   {
						   CSP_LOG_WARN_MSG("Member has left Organization but MemberLeftOrganizationCallback has not been set!");
					   }
				   });
}

void OrganizationSystem::BindOnUserRoleChanged()
{
	Connection->On("OnUserRoleChanged",
				   [this](const signalr::value& Params)
				   {
					   // Params is an array of all params sent, so grab the first
					   String UserId				   = Params.as_array()[0].as_string().c_str();
					   auto UserOrganizationRoleValues = Params.as_array()[1].as_array();

					   Array<EOrganizationRole> UserRoles(UserOrganizationRoleValues.size());

					   for (size_t i = 0; i < UserOrganizationRoleValues.size(); ++i)
					   {
						   const auto& UserOrganizationRoleValue = UserOrganizationRoleValues[i];
						   EOrganizationRole UserRole			 = OrganizationRoleStringToEnum(UserOrganizationRoleValue.as_string().c_str());
						   UserRoles[i]							 = UserRole;
					   }

					   if (InternalUserRoleChangedCallback)
					   {
						   InternalUserRoleChangedCallback(UserId, UserRoles);
					   }
					   else
					   {
						   CSP_LOG_WARN_MSG("Member roles in Organization have changed but OnUserRoleChangedCallback has not been set!");
					   }
				   });
}

std::vector<std::shared_ptr<chs::OrganizationInviteDto>>
	GenerateOrganizationInvites(const common::Array<systems::InviteOrganizationRoleInfo> InviteUsers)
{
	std::vector<std::shared_ptr<chs::OrganizationInviteDto>> OrganizationInvites;
	OrganizationInvites.reserve(InviteUsers.Size());

	for (auto i = 0; i < InviteUsers.Size(); ++i)
	{
		auto InviteUser = InviteUsers[i];

		auto OrganizationInvite = std::make_shared<chs::OrganizationInviteDto>();
		OrganizationInvite->SetEmail(InviteUser.UserEmail);

		size_t RolesNum = InviteUser.OrganizationRoles.Size();

		std::vector<String> UserRoles;
		UserRoles.reserve(RolesNum);

		for (size_t i = 0; i < RolesNum; ++i)
		{
			UserRoles.push_back(OrganizationRoleEnumToString(InviteUser.OrganizationRoles[i]));
		}

		OrganizationInvite->SetRoles(UserRoles);

		OrganizationInvites.push_back(OrganizationInvite);
	}

	return OrganizationInvites;
}

} // namespace csp::systems
