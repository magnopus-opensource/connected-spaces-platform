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
#include "Systems/ResultHelpers.h"


using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace
{
constexpr const String OrganizationRoleToString(systems::EOrganizationRole Role)
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
			throw std::invalid_argument("Unimplemented item");
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
	if (!UserHasPermissions(EOrganizationRole::Administrator) || !UserHasPermissions(EOrganizationRole::Owner))
	{
		// Print message.
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
	if (!UserHasPermissions(EOrganizationRole::Owner))
	{
		// Print message.
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
	if (!UserHasPermissions(EOrganizationRole::Administrator) || !UserHasPermissions(EOrganizationRole::Owner))
	{
		// Print message.
		return;
	}

	auto OrganizationInviteInfo = std::make_shared<chs::OrganizationInviteDto>();
	OrganizationInviteInfo->SetEmail(Email);

	std::vector<String> UserRoles;
	UserRoles.reserve(OrganizationRoles.Size());

	for (auto i = 0; i < OrganizationRoles.Size(); ++i)
	{
		UserRoles.push_back(OrganizationRoleToString(OrganizationRoles[i]));
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
		->apiV1GroupsGroupIdEmailInvitesPost(CurrentOrganizationId,
											 std::nullopt,
											 EmailLinkUrlParam,
											 SignupUrlParam,
											 OrganizationInviteInfo,
											 ResponseHandler);
}

void OrganizationSystem::BulkInviteToOrganization(const InviteOrganizationRoleCollection& InviteUsers, NullResultCallback Callback)
{
	if (!UserHasPermissions(EOrganizationRole::Administrator) || !UserHasPermissions(EOrganizationRole::Owner))
	{
		// Print message.
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
		->apiV1GroupsGroupIdEmailInvitesBulkPost(CurrentOrganizationId,
												 std::nullopt,
												 EmailLinkUrlParam,
												 SignupUrlParam,
												 OrganizationInvites,
												 ResponseHandler);
}

void OrganizationSystem::GetUserRolesInOrganization(const csp::common::Array<csp::common::String>& UserIds, OrganizationRolesResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= OrganizationAPI->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

	static_cast<chs::OrganizationAPI*>(OrganizationAPI)->apiV1OrganizationsOrganizationIdGet(CurrentOrganizationId, ResponseHandler);
}

void OrganizationSystem::RemoveUserFromOrganization(const csp::common::String& UserId, NullResultCallback Callback)
{
}

void OrganizationSystem::UpdateUserRolesInOrganization(const csp::common::String& UserId,
													   const csp::common::Array<EOrganizationRole>& OrganizationRoles,
													   NullResultCallback Callback)
{
}

bool OrganizationSystem::UserHasPermissions(EOrganizationRole RequiredRole)
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
			UserRoles.push_back(OrganizationRoleToString(InviteUser.OrganizationRoles[i]));
		}

		OrganizationInvite->SetRoles(UserRoles);

		OrganizationInvites.push_back(OrganizationInvite);
	}

	return OrganizationInvites;
}

} // namespace csp::systems
