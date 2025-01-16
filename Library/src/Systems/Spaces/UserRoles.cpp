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
#include "Debug/Logging.h"
#include "Web/HttpResponse.h"

namespace csp::systems
{

namespace UserRolesHelpers
{

    bool GetUserRole(const Space& Space, const csp::common::String& UserId, UserRoleInfo& OutRoleInfo)
    {
        OutRoleInfo = { UserId, SpaceUserRole::User };

        if (Space.OwnerId == UserId)
        {
            OutRoleInfo.UserRole = SpaceUserRole::Owner;
            return true;
        }

        for (auto idx = 0; idx < Space.ModeratorIds.Size(); ++idx)
        {
            if (UserId == Space.ModeratorIds[idx])
            {
                OutRoleInfo.UserRole = SpaceUserRole::Moderator;
                return true;
            }
        }

        for (auto idx = 0; idx < Space.UserIds.Size(); ++idx)
        {
            if (UserId == Space.UserIds[idx])
            {
                OutRoleInfo.UserRole = SpaceUserRole::User;
                return true;
            }
        }

        CSP_LOG_ERROR_MSG("UserId is not a member of the Space");
        return false;
    }
} // namespace UserRolesHelpers

csp::common::Array<UserRoleInfo>& UserRoleCollectionResult::GetUsersRoles() { return UserRoles; }

const csp::common::Array<UserRoleInfo>& UserRoleCollectionResult::GetUsersRoles() const { return UserRoles; }

void UserRoleCollectionResult::FillUsersRoles(const Space& Space, const csp::common::Array<csp::common::String> RequestedUserIds)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    UserRoles = csp::common::Array<UserRoleInfo>(RequestedUserIds.Size());

    for (auto idx = 0; idx < RequestedUserIds.Size(); ++idx)
    {
        auto& currentUserId = RequestedUserIds[idx];
        UserRoleInfo currentRoleInfo;
        if (true == UserRolesHelpers::GetUserRole(Space, currentUserId, currentRoleInfo))
        {
            UserRoles[idx] = currentRoleInfo;
        }
    }
}

} // namespace csp::systems
