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

#include "Systems/Spaces/SpaceSystemHelpers.h"

#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Json/JsonParseHelper.h"

using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace csp::systems
{

namespace SpaceSystemHelpers
{

    String GetSpaceMetadataAssetCollectionName(const String& spaceId) { return String(SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX) + spaceId; }

    String GetSpaceIdFromMetadataAssetCollectionName(const String& metadataAssetCollectionName)
    {
        std::string returnValue = metadataAssetCollectionName.c_str();
        returnValue.erase(0, strlen(SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX));
        return String(returnValue.c_str());
    }

    Map<String, String> ConvertSpaceMetadataToAssetCollectionMetadata(const String& metadata)
    {
        Map<String, String> metadataMap;
        metadataMap[SPACE_METADATA_KEY] = metadata;

        return metadataMap;
    }

    String GetSpaceThumbnailAssetCollectionName(const String& spaceId) { return String(SPACE_THUMBNAIL_ASSET_COLLECTION_NAME_PREFIX) + spaceId; }

    String GetUniqueSpaceThumbnailAssetName(const String& spaceId) { return String(SPACE_THUMBNAIL_ASSET_NAME_PREFIX) + spaceId; }

    String GetUniqueAvatarThumbnailAssetName(const String& extension) { return String(AVATAR_THUMBNAIL_ASSET_NAME_PREFIX) + extension; }

    String GetAssetFileExtension(const String& mimeType)
    {
        if (mimeType == "image/png")
            return ".png";
        else if (mimeType == "image/jpeg")
            return ".jpeg";
        else if (mimeType == "image/gif")
            return ".gif";
        else if (mimeType == "image/apng")
            return ".apng";
        else if (mimeType == "image/avif")
            return ".avif";
        else if (mimeType == "image/svg+xml")
            return ".svg";
        else if (mimeType == "image/webp")
            return ".webp";
        else
        {
            CSP_LOG_MSG(LogLevel::Error, "Mimetype File Extension Not Supported");
            return ".buffer";
        }
    }

    void ConvertJsonMetadataToMapMetadata(const String& jsonMetadata, Map<String, String>& outMapMetadata)
    {
        rapidjson::Document json;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(json, jsonMetadata, "ConvertJsonMetadataToMapMetadata");
        if (!ok || !json.IsObject())
        {
            if (ok)
            {
                CSP_LOG_MSG(LogLevel::Verbose, "Space JSON metadata is not an object! Returning default metadata values...");
            }

            outMapMetadata["site"] = "Void";
            outMapMetadata["multiplayerVersion"]
                = "3"; // 2 represents double-msg-packed serialiser spaces, 3 represents the change to dictionary packing

            return;
        }

        for (const auto& member : json.GetObject())
        {
            if (member.value.IsString())
            {
                outMapMetadata[member.name.GetString()] = member.value.GetString();
            }
            else if (member.value.IsInt())
            {
                outMapMetadata[member.name.GetString()] = std::to_string(member.value.GetInt()).c_str();
            }
            else if (member.value.IsNull())
            {
                outMapMetadata[member.name.GetString()] = "";
            }
            else
            {
                CSP_LOG_FORMAT(LogLevel::Error, "Unsupported JSON type in space metadata! (Key = %s, Value Type = %d)", member.name.GetString(),
                    member.value.GetType());
            }
        }
    }

    std::shared_ptr<chs::GroupDto> DefaultGroupInfo()
    {
        auto defaultGroupInfo = std::make_shared<chs::GroupDto>();

        defaultGroupInfo->SetGroupType("Space");
        defaultGroupInfo->SetAutoModerator(false);
        const auto* userSystem = SystemsManager::Get().GetUserSystem();
        defaultGroupInfo->SetGroupOwnerId(userSystem->GetLoginState().UserId);

        return defaultGroupInfo;
    }

    bool IdCheck(const common::String& userId, const common::Array<common::String>& ids)
    {
        bool idFound = false;

        for (size_t i = 0; i < ids.Size(); ++i)
        {
            if (ids[i] == userId)
            {
                idFound = true;
                break;
            }
        }

        return idFound;
    }

    Map<String, String> LegacyAssetConversion(const systems::AssetCollection& assetCollection)
    {
        Map<String, String> spacesMetadata;

        const auto& metadata = assetCollection.GetMetadataImmutable();

        auto spaceId = systems::SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(assetCollection.Name);

        // Convert old JSON metadata to key-value metadata
        if (metadata.HasKey(systems::SpaceSystemHelpers::SPACE_METADATA_KEY) && !metadata.HasKey("site"))
        {
            CSP_LOG_FORMAT(common::LogLevel::Verbose, "Converting old space metadata (Space ID: %s)", spaceId.c_str());

            const auto& json = metadata[systems::SpaceSystemHelpers::SPACE_METADATA_KEY];
            Map<String, String> newMetadata;
            ConvertJsonMetadataToMapMetadata(json, newMetadata);

            spacesMetadata = newMetadata;
        }
        else
        {
            spacesMetadata = metadata;
        }

        return spacesMetadata;
    }

    std::vector<std::shared_ptr<chs::GroupInviteDto>> GenerateGroupInvites(const common::Array<systems::InviteUserRoleInfo> inviteUsers)
    {
        std::vector<std::shared_ptr<chs::GroupInviteDto>> groupInvites;
        groupInvites.reserve(inviteUsers.Size());

        for (size_t i = 0; i < inviteUsers.Size(); ++i)
        {
            auto inviteUser = inviteUsers[i];

            auto groupInvite = std::make_shared<chs::GroupInviteDto>();
            groupInvite->SetEmail(inviteUser.UserEmail);
            groupInvite->SetAsModerator(inviteUser.UserRole == systems::SpaceUserRole::Moderator);

            groupInvites.push_back(groupInvite);
        }

        return groupInvites;
    }

} // namespace SpaceSystemHelpers

} // namespace csp::systems
