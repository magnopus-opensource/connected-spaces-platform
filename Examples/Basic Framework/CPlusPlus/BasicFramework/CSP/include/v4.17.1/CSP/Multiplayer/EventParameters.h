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

#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Spaces/UserRoles.h"

namespace csp::multiplayer
{

/// @brief Enum specifying the type of change that occured to an asset.
enum class EAssetChangeType
{
    Created,
    Updated,
    MusubiFailed,
    Deleted,
    Invalid,
    Num
};

/// @brief Enum specifying the type of change that occured to a user's permissions whilst in a space.
enum class EPermissionChangeType
{
    Created,
    Updated,
    Removed,
    Invalid,
};

/// @brief Describes the changes an asset has undergone when the client application is connected to a space.
class CSP_API AssetDetailBlobParams
{
public:
    /// @brief The type of change this asset has undergone.
    EAssetChangeType ChangeType;

    /// @brief The unique identifer of the asset that has changed.
    csp::common::String AssetId;

    /// @brief The current version of the asset that has changed.
    csp::common::String Version;

    /// @brief The type of the asset that has changed.
    csp::systems::EAssetType AssetType;

    /// @brief The unique identifer of the asset collection the asset that has changed belongs to.
    csp::common::String AssetCollectionId;
};

/// @brief Class used to provide details of a conversation message that has been received whilst the client application is connected to a space.
class CSP_API ConversationSystemParams
{
public:
    /// @brief The type of conversation message event.
    ConversationMessageType MessageType;

    /// @brief The conversation message itself.
    csp::common::String MessageValue;
};

/// @brief Class used to provide details of a permission change that has happened to a user whilst the client application is connected to a space.
class CSP_API UserPermissionsParams
{
public:
    /// @brief The unique identifier of the space for which a user's permissions have changed.
    csp::common::String SpaceId;

    /// @brief The roles that a user has for the given space
    csp::common::Array<csp::systems::SpaceUserRole> UserRoles;

    /// @brief The type of permissions change that has occurred the user.
    EPermissionChangeType ChangeType;

    /// @brief The unique identifier of the user whose permissions have been changed.
    csp::common::String UserId;
};

} // namespace csp::multiplayer
