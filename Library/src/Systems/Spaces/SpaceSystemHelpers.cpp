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

    String GetSpaceMetadataAssetCollectionName(const String& SpaceId) { return String(SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX) + SpaceId; }

    String GetSpaceIdFromMetadataAssetCollectionName(const String& MetadataAssetCollectionName)
    {
        std::string ReturnValue = MetadataAssetCollectionName.c_str();
        ReturnValue.erase(0, strlen(SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX));
        return String(ReturnValue.c_str());
    }

    Map<String, String> ConvertSpaceMetadataToAssetCollectionMetadata(const String& Metadata)
    {
        Map<String, String> MetadataMap;
        MetadataMap[SPACE_METADATA_KEY] = Metadata;

        return MetadataMap;
    }

    String GetSpaceThumbnailAssetCollectionName(const String& SpaceId) { return String(SPACE_THUMBNAIL_ASSET_COLLECTION_NAME_PREFIX) + SpaceId; }

    String GetUniqueSpaceThumbnailAssetName(const String& SpaceId) { return String(SPACE_THUMBNAIL_ASSET_NAME_PREFIX) + SpaceId; }

    String GetUniqueAvatarThumbnailAssetName(const String& Extension) { return String(AVATAR_THUMBNAIL_ASSET_NAME_PREFIX) + Extension; }

    String GetAssetFileExtension(const String& MimeType)
    {
        if (MimeType == "image/png")
            return ".png";
        else if (MimeType == "image/jpeg")
            return ".jpeg";
        else if (MimeType == "image/gif")
            return ".gif";
        else if (MimeType == "image/apng")
            return ".apng";
        else if (MimeType == "image/avif")
            return ".avif";
        else if (MimeType == "image/svg+xml")
            return ".svg";
        else if (MimeType == "image/webp")
            return ".webp";
        else
        {
            CSP_LOG_MSG(LogLevel::Error, "Mimetype File Extension Not Supported");
            return ".buffer";
        }
    }

    void ConvertJsonMetadataToMapMetadata(const String& JsonMetadata, Map<String, String>& OutMapMetadata)
    {
        rapidjson::Document Json;
        rapidjson::ParseResult ok = csp::json::ParseWithErrorLogging(Json, JsonMetadata, "ConvertJsonMetadataToMapMetadata");
        if (!ok || !Json.IsObject())
        {
            if (ok)
            {
                CSP_LOG_MSG(LogLevel::Verbose, "Space JSON metadata is not an object! Returning default metadata values...");
            }

            OutMapMetadata["site"] = "Void";
            OutMapMetadata["multiplayerVersion"]
                = "3"; // 2 represents double-msg-packed serialiser spaces, 3 represents the change to dictionary packing

            return;
        }

        for (const auto& Member : Json.GetObject())
        {
            if (Member.value.IsString())
            {
                OutMapMetadata[Member.name.GetString()] = Member.value.GetString();
            }
            else if (Member.value.IsInt())
            {
                OutMapMetadata[Member.name.GetString()] = std::to_string(Member.value.GetInt()).c_str();
            }
            else if (Member.value.IsNull())
            {
                OutMapMetadata[Member.name.GetString()] = "";
            }
            else
            {
                CSP_LOG_FORMAT(LogLevel::Error, "Unsupported JSON type in space metadata! (Key = %s, Value Type = %d)", Member.name.GetString(),
                    Member.value.GetType());
            }
        }
    }

    std::shared_ptr<chs::GroupDto> DefaultGroupInfo()
    {
        auto DefaultGroupInfo = std::make_shared<chs::GroupDto>();

        DefaultGroupInfo->SetGroupType("Space");
        DefaultGroupInfo->SetAutoModerator(false);
        const auto* UserSystem = SystemsManager::Get().GetUserSystem();
        DefaultGroupInfo->SetGroupOwnerId(UserSystem->GetLoginState().UserId);

        return DefaultGroupInfo;
    }

    bool IdCheck(const common::String& UserId, const common::Array<common::String>& Ids)
    {
        bool IdFound = false;

        for (size_t i = 0; i < Ids.Size(); ++i)
        {
            if (Ids[i] == UserId)
            {
                IdFound = true;
                break;
            }
        }

        return IdFound;
    }

    Map<String, String> LegacyAssetConversion(const systems::AssetCollection& AssetCollection)
    {
        Map<String, String> SpacesMetadata;

        const auto& Metadata = AssetCollection.GetMetadataImmutable();

        auto SpaceId = systems::SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(AssetCollection.Name);

        // Convert old JSON metadata to key-value metadata
        if (Metadata.HasKey(systems::SpaceSystemHelpers::SPACE_METADATA_KEY) && !Metadata.HasKey("site"))
        {
            CSP_LOG_FORMAT(common::LogLevel::Verbose, "Converting old space metadata (Space ID: %s)", SpaceId.c_str());

            const auto& Json = Metadata[systems::SpaceSystemHelpers::SPACE_METADATA_KEY];
            Map<String, String> NewMetadata;
            ConvertJsonMetadataToMapMetadata(Json, NewMetadata);

            SpacesMetadata = NewMetadata;
        }
        else
        {
            SpacesMetadata = Metadata;
        }

        return SpacesMetadata;
    }

    std::vector<std::shared_ptr<chs::GroupInviteDto>> GenerateGroupInvites(const common::Array<systems::InviteUserRoleInfo> InviteUsers)
    {
        std::vector<std::shared_ptr<chs::GroupInviteDto>> GroupInvites;
        GroupInvites.reserve(InviteUsers.Size());

        for (size_t i = 0; i < InviteUsers.Size(); ++i)
        {
            auto InviteUser = InviteUsers[i];

            auto GroupInvite = std::make_shared<chs::GroupInviteDto>();
            GroupInvite->SetEmail(InviteUser.UserEmail);
            GroupInvite->SetAsModerator(InviteUser.UserRole == systems::SpaceUserRole::Moderator);

            GroupInvites.push_back(GroupInvite);
        }

        return GroupInvites;
    }

} // namespace SpaceSystemHelpers

} // namespace csp::systems
