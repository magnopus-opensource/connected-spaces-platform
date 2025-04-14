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
#include "ConversationSystemHelpers.h"

#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"

namespace csp::systems::ConversationSystemHelpers
{
constexpr const char* CONVERSATION_CONTAINER_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_CONVERSATION_CONTAINER";
constexpr const char* MESSAGE_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_MESSAGE";
constexpr const char* ANNOTATION_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_ANNOTATION";
constexpr const char* ANNOTATION_ASSET_NAME_PREFIX = "ASSET_ANNOTATION";
constexpr const char* ANNOTATION_THUMBNAIL_ASSET_NAME_PREFIX = "ASSET_ANNOTATION_THUMBNAIL";

constexpr const char* ANNOTATION_ASSET_FILENAME_PREFIX = "ASSET_FILE_ANNOTATION";
constexpr const char* ANNOTATION_THUMBNAIL_ASSET_FILENAME_PREFIX = "ASSET_FILE_ANNOTATION_THUMBNAIL";

// Comment keys
constexpr const char* ASSET_COLLECTION_METADATA_KEY_MESSAGE = "Message";
// Annotation keys
constexpr const char* ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID = "ThumbnailId";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID = "AnnotationId";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV = "VerticalFovId";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION = "CameraPosition";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION = "CameraRotation";

namespace
{
    // When an asset collection hasn't been edited, the UpdatedAt timestamp is the same as the CreatedAt timestamp.
    // We want this be an empty string if the conversation hasn't been modified.
    bool HasBeenEdited(const AssetCollection& AssetCollection) { return AssetCollection.CreatedAt != AssetCollection.UpdatedAt; }

    common::String Vector3ToString(const common::Vector3& value)
    {
        return common::StringFormat("%s,%s,%s", std::to_string(value.X).c_str(), std::to_string(value.Y).c_str(), std::to_string(value.Z).c_str());
    }

    common::Vector3 StringToVector3(const common::String& value)
    {
        common::List<common::String> StringList = value.Split(',');
        return common::Vector3(std::stof(StringList[0].c_str()), std::stof(StringList[1].c_str()), std::stof(StringList[2].c_str()));
    }

    common::String Vector4ToString(const common::Vector4& value)
    {
        return common::StringFormat("%s,%s,%s,%s", std::to_string(value.X).c_str(), std::to_string(value.Y).c_str(), std::to_string(value.Z).c_str(),
            std::to_string(value.W).c_str());
    }

    common::Vector4 StringToVector4(const common::String& value)
    {
        common::List<common::String> StringList = value.Split(',');
        return common::Vector4(
            std::stof(StringList[0].c_str()), std::stof(StringList[1].c_str()), std::stof(StringList[2].c_str()), std::stof(StringList[3].c_str()));
    }
}

common::String GetUniqueAssetCollectionSuffix(const common::String& SpaceId, const common::String& CreatorUserId)
{
    const auto NowTimepoint = std::chrono::system_clock::now();
    const auto MillisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(NowTimepoint.time_since_epoch()).count();

    // TODO: consider appending a random value too if the same user creates two messages in the same millisecond
    return common::StringFormat("%s_%s_%llu", SpaceId.c_str(), CreatorUserId.c_str(), MillisecondsSinceEpoch);
}

common::String GetUniqueConversationContainerAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    const auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", CONVERSATION_CONTAINER_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
}

common::String GetUniqueMessageAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    const auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", MESSAGE_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
}

common::String GetUniqueAnnotationAssetName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    const auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_ASSET_NAME_PREFIX, Suffix.c_str());
}

common::String GetUniqueAnnotationThumbnailAssetName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_THUMBNAIL_ASSET_NAME_PREFIX, Suffix.c_str());
}

common::String GetUniqueAnnotationAssetFileName(
    const common::String& SpaceId, const common::String& CreatorUserId, const csp::common::String& Extension)
{
    const auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_ASSET_FILENAME_PREFIX, Suffix.c_str()) + "." + Extension;
}

common::String GetUniqueAnnotationThumbnailFileName(
    const common::String& SpaceId, const common::String& CreatorUserId, const csp::common::String& Extension)
{
    const auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_THUMBNAIL_ASSET_FILENAME_PREFIX, Suffix.c_str()) + "." + Extension;
}

common::String GetUniqueAnnotationAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    const auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
}

common::Map<common::String, common::String> GenerateMessageAssetCollectionMetadata(const multiplayer::MessageInfo& MessageData)
{
    common::Map<common::String, common::String> MetadataMap;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = MessageData.Message;

    return MetadataMap;
}

csp::common::Array<multiplayer::ReplicatedValue> MessageInfoToReplicatedValueArray(const multiplayer::ConversationEventParams& Params)
{
    csp::common::Array<multiplayer::ReplicatedValue> Args(7);

    Args[0] = static_cast<int64_t>(Params.MessageType);
    Args[1] = Params.MessageInfo.ConversationId;
    Args[2] = Params.MessageInfo.CreatedTimestamp;
    Args[3] = Params.MessageInfo.EditedTimestamp;
    Args[4] = Params.MessageInfo.UserId;
    Args[5] = Params.MessageInfo.Message;
    Args[6] = Params.MessageInfo.MessageId;

    return Args;
}

common::Map<common::String, common::String> GenerateConversationAssetCollectionMetadata(const multiplayer::MessageInfo& ConversationData)
{
    common::Map<common::String, common::String> MetadataMap;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = ConversationData.Message;

    return MetadataMap;
}

common::Map<common::String, common::String> GenerateAnnotationAssetCollectionMetadata(const multiplayer::AnnotationUpdateParams& AnnotationData,
    const csp::common::String& AnnotationId, const csp::common::String& AnnotationThumbnailId)
{
    common::Map<common::String, common::String> MetadataMap;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID] = AnnotationId;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID] = AnnotationThumbnailId;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV] = std::to_string(AnnotationData.VerticalFov).c_str();
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION] = Vector3ToString(AnnotationData.AuthorCameraPosition);
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION] = Vector4ToString(AnnotationData.AuthorCameraRotation);

    return MetadataMap;
}

common::Map<common::String, common::String> RemoveAnnotationMetadata(const AssetCollection& MessageAssetCollection)
{
    auto MetadataMap = MessageAssetCollection.GetMetadataImmutable();
    MetadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID);
    MetadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID);
    MetadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV);
    MetadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION);
    MetadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION);

    return MetadataMap;
}

void PopulateMessageInfoFromMetadata(const csp::common::Map<csp::common::String, csp::common::String>& Metadata, multiplayer::MessageInfo& Info)
{
    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_MESSAGE))
    {
        Info.Message = Metadata[ASSET_COLLECTION_METADATA_KEY_MESSAGE];
    }
    else
    {
        CSP_LOG_WARN_MSG("No Message MetaData found");
        Info.Message = "";
    }
}

multiplayer::MessageInfo GetMessageInfoFromMessageAssetCollection(const csp::systems::AssetCollection& MessageAssetCollection)
{
    multiplayer::MessageInfo Info;
    Info.ConversationId = MessageAssetCollection.ParentId;
    Info.CreatedTimestamp = MessageAssetCollection.CreatedAt;
    Info.UserId = MessageAssetCollection.CreatedBy;
    Info.MessageId = MessageAssetCollection.Id;

    if (HasBeenEdited(MessageAssetCollection))
    {
        Info.EditedTimestamp = MessageAssetCollection.UpdatedAt;
    }

    PopulateMessageInfoFromMetadata(MessageAssetCollection.GetMetadataImmutable(), Info);

    return Info;
}

bool HasAnnotationMetadata(const AssetCollection& MessageAssetCollection)
{
    const auto& MetadataMap = MessageAssetCollection.GetMetadataImmutable();
    return MetadataMap.HasKey(ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID);
}

std::map<std::string, std::string> GetAnnotationThumbnailAssetIdsFromCollectionResult(const AssetCollectionsResult& Result)
{
    std::map<std::string, std::string> ThumbnailAssetIds;

    for (const auto& Collection : Result.GetAssetCollections())
    {
        auto Metadata = Collection.GetMetadataImmutable();
        bool HasThumbnail = Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID);

        if (HasThumbnail)
        {
            ThumbnailAssetIds[Collection.Id.c_str()] = Metadata[ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID].c_str();
        }
    }

    return ThumbnailAssetIds;
}

multiplayer::AnnotationData GetAnnotationDataFromMessageAssetCollection(const AssetCollection& MessageAssetCollection)
{
    multiplayer::AnnotationData Data;
    const auto& MetadataMap = MessageAssetCollection.GetMetadataImmutable();

    Data.SetAnnotationId(MetadataMap[ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID]);
    Data.SetAnnotationThumbnailId(MetadataMap[ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID]);
    Data.SetVerticalFov(std::stoi(MetadataMap[ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV].c_str()));
    Data.SetAuthorCameraPosition(StringToVector3(MetadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION]));
    Data.SetAuthorCameraRotation(StringToVector4(MetadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION]));

    return Data;
}

multiplayer::MessageInfo GetConversationInfoFromConversationAssetCollection(const csp::systems::AssetCollection& ConversationAssetCollection)
{
    multiplayer::MessageInfo Info;
    Info.ConversationId = ConversationAssetCollection.Id;
    Info.CreatedTimestamp = ConversationAssetCollection.CreatedAt;
    Info.UserId = ConversationAssetCollection.CreatedBy;

    if (HasBeenEdited(ConversationAssetCollection))
    {
        Info.EditedTimestamp = ConversationAssetCollection.UpdatedAt;
    }

    PopulateMessageInfoFromMetadata(ConversationAssetCollection.GetMetadataImmutable(), Info);

    return Info;
}

} // namespace csp::multiplayer
