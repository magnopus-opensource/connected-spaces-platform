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

#include "Common/Convert.h"
#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"

using namespace csp;
using namespace csp::common;

namespace chs_users = csp::services::generated::userservice;

namespace
{

systems::EOrganizationRole OrganizationRoleStringToEnum(String Role)
{
    if (Role == "member")
    {
        return systems::EOrganizationRole::Member;
    }
    else if (Role == "admin")
    {
        return systems::EOrganizationRole::Administrator;
    }
    else if (Role == "owner")
    {
        return systems::EOrganizationRole::Owner;
    }
    else
    {
        throw std::invalid_argument("Unimplemented role");
    }
}

// todo: Complete implementation once CHS have finished implementing the Organization endpoints to the User Service.
void OrganizationDtoToOrganization(const chs_users::OrganizationDto& Dto, csp::systems::Organization& Organization)
{
    Organization.Id = Dto.GetId();
    Organization.CreatedAt = Dto.GetCreatedAt();
    Organization.OwnerId = Dto.GetOrganizationOwnerId();
    Organization.Name = Dto.GetName();

    auto& OrgMembers = Dto.GetMembers();
    Organization.Members = csp::common::Array<systems::OrganizationRoleInfo>(OrgMembers.size());

    for (int i = 0; i < OrgMembers.size(); ++i)
    {
        Organization.Members[i].UserId = OrgMembers[i]->GetUserId();
        auto& OrgMemberRoles = OrgMembers[i]->GetRoles();
        Organization.Members[i].OrganizationRoles = csp::common::Array<systems::EOrganizationRole>(OrgMemberRoles.size());

        for (int j = 0; j < OrgMemberRoles.size(); ++j)
        {
            Organization.Members[i].OrganizationRoles[j] = OrganizationRoleStringToEnum(OrgMemberRoles[j]);
        }
    }
}

void OrganizationRoleDtoToOrganizationRole(const chs_users::OrganizationMember& Dto, csp::systems::OrganizationRoleInfo& OrganizationRoleInfo)
{
    OrganizationRoleInfo.UserId = Dto.GetUserId();
    auto UserOrgRoles = csp::common::Convert(Dto.GetRoles());
    OrganizationRoleInfo.OrganizationRoles = csp::common::Array<systems::EOrganizationRole>(UserOrgRoles.Size());

    for (int i = 0; i < UserOrgRoles.Size(); ++i)
    {
        OrganizationRoleInfo.OrganizationRoles[i] = OrganizationRoleStringToEnum(UserOrgRoles[i]);
    }
}

} // namespace

namespace csp::systems
{

const Organization& OrganizationResult::GetOrganization() const { return Organization; }

const csp::common::Array<OrganizationRoleInfo>& OrganizationRolesResult::GetOrganizationRoleInfo() const { return OrganizationRoleInfos; }

void OrganizationResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* OrganizationResponse = static_cast<chs_users::OrganizationDto*>(ApiResponse->GetDto());
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

    auto* OrganizationResponse = static_cast<csp::services::DtoArray<chs_users::OrganizationMember>*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        OrganizationResponse->FromJson(Response->GetPayload().GetContent());
        auto& RoleInfoArray = OrganizationResponse->GetArray();
        OrganizationRoleInfos = csp::common::Array<OrganizationRoleInfo>(RoleInfoArray.size());

        for (int i = 0; i < RoleInfoArray.size(); ++i)
        {
            OrganizationRoleDtoToOrganizationRole(RoleInfoArray[i], OrganizationRoleInfos[i]);
        }
    }
}

} // namespace csp::systems
