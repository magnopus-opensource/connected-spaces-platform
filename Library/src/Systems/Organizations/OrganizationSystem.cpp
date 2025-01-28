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

#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "Memory/Memory.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Services/UserService/Api.h"
#include "Systems/ResultHelpers.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace
{

String OrganizationRoleEnumToString(const systems::EOrganizationRole Role)
{
    switch (Role)
    {
    case systems::EOrganizationRole::Member:
        return "member";
    case systems::EOrganizationRole::Administrator:
        return "admin";
    case systems::EOrganizationRole::Owner:
        return "owner";
    default:
        throw std::invalid_argument("Unimplemented role");
    }
}

} // namespace

namespace csp::systems
{

bool HasMemberRoleBeenDefined(const csp::common::Array<EOrganizationRole>& OrganizationRoles)
{
    bool IsRoleDefined = false;

    for (auto i = 0; i < OrganizationRoles.Size(); ++i)
    {
        if (OrganizationRoles[i] == systems::EOrganizationRole::Member)
        {
            IsRoleDefined = true;
            break;
        }
    }

    return IsRoleDefined;
}

std::vector<std::shared_ptr<chs::OrganizationInviteDto>> GenerateOrganizationInvites(
    const common::Array<systems::InviteOrganizationRoleInfo>& InviteUsers)
{
    std::vector<std::shared_ptr<chs::OrganizationInviteDto>> OrganizationInvites;
    OrganizationInvites.reserve(InviteUsers.Size());

    for (auto i = 0; i < InviteUsers.Size(); ++i)
    {
        auto InviteUser = InviteUsers[i];

        auto OrganizationInvite = std::make_shared<chs::OrganizationInviteDto>();
        OrganizationInvite->SetEmail(InviteUser.UserEmail);

        // All users added to an Organization must have the 'member' role. If not defined here it will be added.
        const bool HasMemberRole = HasMemberRoleBeenDefined(InviteUser.OrganizationRoles);
        const size_t NumRoles = HasMemberRole ? InviteUser.OrganizationRoles.Size() : InviteUser.OrganizationRoles.Size() + 1;

        std::vector<String> UserRoles;
        UserRoles.reserve(NumRoles);

        UserRoles.push_back(OrganizationRoleEnumToString(EOrganizationRole::Member));

        for (size_t i = 0; i < NumRoles; ++i)
        {
            if (InviteUser.OrganizationRoles[i] == EOrganizationRole::Member)
            {
                continue;
            }

            UserRoles.push_back(OrganizationRoleEnumToString(InviteUser.OrganizationRoles[i]));
        }

        OrganizationInvite->SetRoles(UserRoles);

        OrganizationInvites.push_back(OrganizationInvite);
    }

    return OrganizationInvites;
}

OrganizationSystem::OrganizationSystem()
    : SystemBase(nullptr, nullptr)
    , OrganizationApi(nullptr)
{
}

OrganizationSystem::OrganizationSystem(csp::web::WebClient* InWebClient)
    : SystemBase(InWebClient, nullptr)
{
    OrganizationApi = CSP_NEW chs::OrganizationApi(InWebClient);
}

OrganizationSystem::~OrganizationSystem() { CSP_DELETE(OrganizationApi); }

void OrganizationSystem::CreateOrganization(
    const csp::common::String& OrganizationOwnerId, const csp::common::String& OrganizationName, OrganizationResultCallback Callback)
{
    auto OrganizationInfo = std::make_shared<chs::OrganizationDto>();
    OrganizationInfo->SetName(OrganizationName);
    OrganizationInfo->SetOrganizationOwnerId(OrganizationOwnerId);

    csp::services::ResponseHandlerPtr ResponseHandler
        = OrganizationApi->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

    static_cast<chs::OrganizationApi*>(OrganizationApi)->apiV1OrganizationsPost(OrganizationInfo, ResponseHandler);
}

void OrganizationSystem::GetOrganization(const csp::common::Optional<csp::common::String>& OrganizationId, OrganizationResultCallback Callback)
{
    csp::common::String SelectedOrganizationId;

    if (OrganizationId.HasValue())
    {
        SelectedOrganizationId = *OrganizationId;
    }
    else
    {
        SelectedOrganizationId = GetCurrentOrganizationId();

        if (SelectedOrganizationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Call to GetOrganization failed. You do not belong to an Organization.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<OrganizationResult>());

            return;
        }
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = OrganizationApi->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

    static_cast<chs::OrganizationApi*>(OrganizationApi)->apiV1OrganizationsOrganizationIdGet(SelectedOrganizationId, ResponseHandler);
}

const csp::common::String& OrganizationSystem::GetCurrentOrganizationId() const
{
    // todo: The AuthDto contains an array of Organization Ids but we have no way of knowing which one the user is authenticated against.
    // Currently people can only belong to a single organization so we can hard code the index. Ticket OF-1291, outlines the work required to
    // extract this information from the Bearer token claims, where the 'current' Organization Id is also captured.
    const auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
    const auto& OrganizationIds = UserSystem->GetLoginState().OrganizationIds;

    if (OrganizationIds.Size() == 0)
    {
        CSP_LOG_ERROR_MSG("Unable to get current Orgaization Id, you do not belong to an Organization.");

        static const String EmptyId = "";
        return EmptyId;
    }

    return OrganizationIds[0];
}

void OrganizationSystem::UpdateOrganization(
    const csp::common::Optional<csp::common::String>& OrganizationId, const csp::common::String& Name, OrganizationResultCallback Callback)
{
    if (Name.IsEmpty())
    {
        CSP_LOG_WARN_MSG("A valid new Organization name must be specified.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<OrganizationResult>());

        return;
    }

    csp::common::String SelectedOrganizationId;

    if (OrganizationId.HasValue())
    {
        SelectedOrganizationId = *OrganizationId;
    }
    else
    {
        SelectedOrganizationId = GetCurrentOrganizationId();

        if (SelectedOrganizationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Call to UpdateOrganization failed. No Organization has been updated as you do not belong to one.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<OrganizationResult>());

            return;
        }
    }

    auto OrganizationInfo = std::make_shared<chs::OrganizationDto>();
    OrganizationInfo->SetName(Name);

    auto* UserSystem = SystemsManager::Get().GetUserSystem();
    const auto& UserId = UserSystem->GetLoginState().UserId;

    OrganizationInfo->SetOrganizationOwnerId(UserId);

    csp::services::ResponseHandlerPtr ResponseHandler
        = OrganizationApi->CreateHandler<OrganizationResultCallback, OrganizationResult, void, chs::OrganizationDto>(Callback, nullptr);

    static_cast<chs::OrganizationApi*>(OrganizationApi)
        ->apiV1OrganizationsOrganizationIdPut(SelectedOrganizationId, OrganizationInfo, ResponseHandler);
}

void OrganizationSystem::DeactivateOrganization(const csp::common::String& OrganizationId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler = OrganizationApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::OrganizationApi*>(OrganizationApi)->apiV1OrganizationsOrganizationIdDelete(OrganizationId, ResponseHandler);
}

void OrganizationSystem::InviteToOrganization(const csp::common::Optional<csp::common::String>& OrganizationId, const csp::common::String& Email,
    const csp::common::Array<EOrganizationRole>& OrganizationRoles, const csp::common::Optional<csp::common::String>& EmailLinkUrl,
    const csp::common::Optional<csp::common::String>& SignupUrl, NullResultCallback Callback)
{
    csp::common::String SelectedOrganizationId;

    if (OrganizationId.HasValue())
    {
        SelectedOrganizationId = *OrganizationId;
    }
    else
    {
        SelectedOrganizationId = GetCurrentOrganizationId();

        if (SelectedOrganizationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG(
                "Call to InviteToOrganization failed. The specified user has not been invited as you do not belong to an Organization.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

            return;
        }
    }

    auto OrganizationInviteInfo = std::make_shared<chs::OrganizationInviteDto>();
    OrganizationInviteInfo->SetEmail(Email);

    // All users added to an Organization must have the 'member' role. If not defined here it will be added.
    const bool HasMemberRole = HasMemberRoleBeenDefined(OrganizationRoles);
    const size_t NumRoles = HasMemberRole ? OrganizationRoles.Size() : OrganizationRoles.Size() + 1;

    std::vector<String> UserRoles;
    UserRoles.reserve(NumRoles);

    UserRoles.push_back(OrganizationRoleEnumToString(EOrganizationRole::Member));

    for (auto i = 0; i < OrganizationRoles.Size(); ++i)
    {
        if (OrganizationRoles[i] == EOrganizationRole::Member)
        {
            continue;
        }

        UserRoles.push_back(OrganizationRoleEnumToString(OrganizationRoles[i]));
    }

    OrganizationInviteInfo->SetRoles(UserRoles);

    auto EmailLinkUrlParam = EmailLinkUrl.HasValue() && !EmailLinkUrl->IsEmpty() ? (*EmailLinkUrl) : std::optional<String>(std::nullopt);
    auto SignupUrlParam = SignupUrl.HasValue() && !SignupUrl->IsEmpty() ? (*SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr ResponseHandler = OrganizationApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::OrganizationApi*>(OrganizationApi)
        ->apiV1OrganizationsOrganizationIdMembershipInvitesPost(
            SelectedOrganizationId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, OrganizationInviteInfo, ResponseHandler);
}

void OrganizationSystem::BulkInviteToOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
    const InviteOrganizationRoleCollection& InviteUsers, NullResultCallback Callback)
{
    csp::common::String SelectedOrganizationId;

    if (OrganizationId.HasValue())
    {
        SelectedOrganizationId = *OrganizationId;
    }
    else
    {
        SelectedOrganizationId = GetCurrentOrganizationId();

        if (SelectedOrganizationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Call to BulkInviteToOrganization failed. No-one has been invited as you do not belong to an Organization.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

            return;
        }
    }

    const std::vector<std::shared_ptr<chs::OrganizationInviteDto>> OrganizationInvites = GenerateOrganizationInvites(InviteUsers.InvitedUserRoles);

    const auto EmailLinkUrlParam = !InviteUsers.EmailLinkUrl.IsEmpty() ? (InviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
    const auto SignupUrlParam = !InviteUsers.SignupUrl.IsEmpty() ? (InviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr ResponseHandler = OrganizationApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::OrganizationApi*>(OrganizationApi)
        ->apiV1OrganizationsOrganizationIdMembershipInvitesBulkPost(
            SelectedOrganizationId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, OrganizationInvites, ResponseHandler);
}

void OrganizationSystem::GetUserRolesInOrganization(const csp::common::Optional<csp::common::String>& OrganizationId,
    const csp::common::Array<csp::common::String>& UserIds, OrganizationRolesResultCallback Callback)
{
    csp::common::String SelectedOrganizationId;

    if (OrganizationId.HasValue())
    {
        SelectedOrganizationId = *OrganizationId;
    }
    else
    {
        SelectedOrganizationId = GetCurrentOrganizationId();

        if (SelectedOrganizationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Call to GetUserRolesInOrganization failed. You do not belong to an Organization.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<OrganizationRolesResult>());

            return;
        }
    }

    std::vector<String> InternalUserIds;
    InternalUserIds.reserve(UserIds.Size());

    for (auto i = 0; i < UserIds.Size(); ++i)
    {
        InternalUserIds.push_back(UserIds[i]);
    }

    csp::services::ResponseHandlerPtr ResponseHandler = OrganizationApi->CreateHandler<OrganizationRolesResultCallback, OrganizationRolesResult, void,
        csp::services::DtoArray<chs::OrganizationMember>>(Callback, nullptr);

    static_cast<chs::OrganizationApi*>(OrganizationApi)
        ->apiV1OrganizationsOrganizationIdRolesGet(SelectedOrganizationId, InternalUserIds, ResponseHandler);
}

void OrganizationSystem::RemoveUserFromOrganization(
    const csp::common::Optional<csp::common::String>& OrganizationId, const csp::common::String& UserId, NullResultCallback Callback)
{
    csp::common::String SelectedOrganizationId;

    if (OrganizationId.HasValue())
    {
        SelectedOrganizationId = *OrganizationId;
    }
    else
    {
        SelectedOrganizationId = GetCurrentOrganizationId();

        if (SelectedOrganizationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Call to RemoveUserFromOrganization failed. You do not belong to an Organization.");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

            return;
        }
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = OrganizationApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

    static_cast<chs::OrganizationApi*>(OrganizationApi)
        ->apiV1OrganizationsOrganizationIdUsersUserIdDelete(SelectedOrganizationId, UserId, ResponseHandler);
}

void OrganizationSystem::SetMemberJoinedOrganizationCallback(MemberJoinedOrganizationCallback Callback)
{
    if (InternalMemberJoinedOrganizationCallback)
    {
        CSP_LOG_WARN_MSG("MemberJoinedOrganizationCallback has already been set. Previous callback overwritten.");
    }

    InternalMemberJoinedOrganizationCallback = std::move(Callback);
}

} // namespace csp::systems
