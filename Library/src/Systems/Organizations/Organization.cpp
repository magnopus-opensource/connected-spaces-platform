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
#include "CSP/Systems/Organizations/Organization.h"

#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"

using namespace csp;
using namespace csp::common;

namespace chs_users = csp::services::generated::userservice;

namespace
{

// todo: Complete implementation once CHS have finished implementing the Organization endpoints to the User Service.
void OrganizationDtoToOrganization(const chs_users::OrganizationDto& Dto, csp::systems::Organization& Organization)
{
	// More fields will be added by CHS in the future.
	Organization.Id		 = Dto.GetId();
	Organization.Name	 = Dto.GetName();
	Organization.OwnerId = Dto.GetOrganizationOwnerId();
}

// todo: Implement once CHS have finished implementing the Organization endpoints to the User Service.
void OrganizationInviteDtoToOrganizationRoleInfo()
{
}

} // namespace

namespace csp::systems
{

const Organization& OrganizationResult::GetOrganization() const
{
	return Organization;
}

const OrganizationRoleInfo& OrganizationRolesResult::GetOrganizationRoleInfo() const
{
	return OrganizationRoleInfo;
}

void OrganizationResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* OrganizationResponse			   = static_cast<chs_users::OrganizationDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		OrganizationResponse->FromJson(Response->GetPayload().GetContent());
		OrganizationDtoToOrganization(*OrganizationResponse, Organization);
	}
}

void OrganizationRolesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	// todo: CHS will be adding another Dto for this - OrganizationInviteDto.
	auto* OrganizationResponse			   = static_cast<chs_users::OrganizationDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		OrganizationResponse->FromJson(Response->GetPayload().GetContent());
		// OrganizationDtoToOrganization(*OrganizationResponse, Organization);
	}
}

} // namespace csp::systems
