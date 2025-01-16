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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "Services/UserService/Dto.h"

namespace csp::systems
{

class InviteUserRoleInfo;
class AssetCollection;

constexpr const char* PUBLIC_SPACE_TYPE = "public";
constexpr const char* PRIVATE_SPACE_TYPE = "private";

namespace SpaceSystemHelpers
{

    constexpr const char* SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_SPACE_METADATA_";
    constexpr const char* SPACE_METADATA_KEY = "SpaceMetadata";

    constexpr const char* SPACE_THUMBNAIL_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_SPACE_THUMBNAIL_";
    constexpr const char* SPACE_THUMBNAIL_ASSET_NAME_PREFIX = "SPACE_THUMBNAIL_";

    constexpr const char* AVATAR_THUMBNAIL_ASSET_NAME_PREFIX = "AVATAR_THUMBNAIL";

    csp::common::String GetSpaceMetadataAssetCollectionName(const csp::common::String& SpaceId);
    csp::common::Map<csp::common::String, csp::common::String> ConvertSpaceMetadataToAssetCollectionMetadata(const csp::common::String& Metadata);
    csp::common::String GetSpaceIdFromMetadataAssetCollectionName(const csp::common::String& MetadataAssetCollectionName);

    csp::common::String GetSpaceThumbnailAssetCollectionName(const csp::common::String& SpaceId);
    csp::common::String GetUniqueSpaceThumbnailAssetName(const csp::common::String& SpaceId);
    csp::common::String GetUniqueAvatarThumbnailAssetName(const csp::common::String& Extension);

    csp::common::String GetAssetFileExtension(const csp::common::String& MimeType);

    bool IdCheck(const common::String& UserId, const common::Array<common::String>& Ids);

    std::shared_ptr<csp::services::generated::userservice::GroupDto> DefaultGroupInfo();

    csp::common::Map<csp::common::String, csp::common::String> LegacyAssetConversion(const systems::AssetCollection& AssetCollection);

    std::vector<std::shared_ptr<csp::services::generated::userservice::GroupInviteDto>> GenerateGroupInvites(
        const common::Array<systems::InviteUserRoleInfo> InviteUsers);

    void ConvertJsonMetadataToMapMetadata(
        const csp::common::String& JsonMetadata, csp::common::Map<csp::common::String, csp::common::String>& OutMapMetadata);

} // namespace SpaceSystemHelpers

} // namespace csp::systems
