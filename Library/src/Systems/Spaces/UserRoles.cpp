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
#include "CSP/Systems/Spaces/UserRoles.h"

#include "CSP/Systems/Spaces/Space.h"
#include "Common/Web/HttpResponse.h"
#include "Debug/Logging.h"

namespace csp::systems
{

bool UserRoleInfo::operator==(const UserRoleInfo& other) const { return UserId == other.UserId && UserRole == other.UserRole; }

bool InviteUserRoleInfo::operator==(const InviteUserRoleInfo& other) const { return UserEmail == other.UserEmail && UserRole == other.UserRole; }

bool InviteUserRoleInfoCollection::operator==(const InviteUserRoleInfoCollection& other) const
{
    return EmailLinkUrl == other.EmailLinkUrl && SignupUrl == other.SignupUrl && InviteUserRoleInfos == other.InviteUserRoleInfos;
}

bool UserRoleInfo::operator!=(const UserRoleInfo& other) const { return !(*this == other); }
bool InviteUserRoleInfo::operator!=(const InviteUserRoleInfo& other) const { return !(*this == other); }
bool InviteUserRoleInfoCollection::operator!=(const InviteUserRoleInfoCollection& other) const { return !(*this == other); }

namespace UserRolesHelpers
{

    bool GetUserRole(const Space& space, const csp::common::String& userId, UserRoleInfo& outRoleInfo)
    {
        outRoleInfo = { userId, SpaceUserRole::User };

        if (space.OwnerId == userId)
        {
            outRoleInfo.UserRole = SpaceUserRole::Owner;
            return true;
        }

        for (size_t idx = 0; idx < space.ModeratorIds.Size(); ++idx)
        {
            if (userId == space.ModeratorIds[idx])
            {
                outRoleInfo.UserRole = SpaceUserRole::Moderator;
                return true;
            }
        }

        for (size_t idx = 0; idx < space.UserIds.Size(); ++idx)
        {
            if (userId == space.UserIds[idx])
            {
                outRoleInfo.UserRole = SpaceUserRole::User;
                return true;
            }
        }

        CSP_LOG_ERROR_MSG("UserId is not a member of the Space");
        return false;
    }
} // namespace UserRolesHelpers

csp::common::Array<UserRoleInfo>& UserRoleCollectionResult::GetUsersRoles() { return m_userRoles; }

const csp::common::Array<UserRoleInfo>& UserRoleCollectionResult::GetUsersRoles() const { return m_userRoles; }

void UserRoleCollectionResult::FillUsersRoles(const Space& space, const csp::common::Array<csp::common::String> requestedUserIds)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    m_userRoles = csp::common::Array<UserRoleInfo>(requestedUserIds.Size());

    for (size_t idx = 0; idx < requestedUserIds.Size(); ++idx)
    {
        auto& currentUserId = requestedUserIds[idx];
        UserRoleInfo currentRoleInfo;
        if (true == UserRolesHelpers::GetUserRole(space, currentUserId, currentRoleInfo))
        {
            m_userRoles[idx] = currentRoleInfo;
        }
    }
}

} // namespace csp::systems
