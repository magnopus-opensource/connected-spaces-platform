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

#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
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
    bool HasBeenEdited(const AssetCollection& assetCollection) { return assetCollection.CreatedAt != assetCollection.UpdatedAt; }

    common::String Vector3ToString(const common::Vector3& value)
    {
        return common::StringFormat("%s,%s,%s", std::to_string(value.X).c_str(), std::to_string(value.Y).c_str(), std::to_string(value.Z).c_str());
    }

    // Should only be used if string was created using Vector3ToString, or we can guarantee the string is in the format "X,Y,Z".
    common::Vector3 StringToVector3(const common::String& value)
    {
        common::List<common::String> stringList = value.Split(',');
        return common::Vector3(std::stof(stringList[0].c_str()), std::stof(stringList[1].c_str()), std::stof(stringList[2].c_str()));
    }

    common::String Vector4ToString(const common::Vector4& value)
    {
        return common::StringFormat("%s,%s,%s,%s", std::to_string(value.X).c_str(), std::to_string(value.Y).c_str(), std::to_string(value.Z).c_str(),
            std::to_string(value.W).c_str());
    }

    // Should only be used if string was created using Vector4ToString, or we can guarantee the string is in the format "X,Y,Z,W".
    common::Vector4 StringToVector4(const common::String& value)
    {
        common::List<common::String> stringList = value.Split(',');
        return common::Vector4(
            std::stof(stringList[0].c_str()), std::stof(stringList[1].c_str()), std::stof(stringList[2].c_str()), std::stof(stringList[3].c_str()));
    }
}

common::String GetUniqueAssetCollectionSuffix(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto nowTimepoint = std::chrono::system_clock::now();
    const auto millisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(nowTimepoint.time_since_epoch()).count();

    // TODO: consider appending a random value too if the same user creates two messages in the same millisecond
    return common::StringFormat("%s_%s_%llu", spaceId.c_str(), creatorUserId.c_str(), millisecondsSinceEpoch);
}

common::String GetUniqueConversationContainerAssetCollectionName(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", CONVERSATION_CONTAINER_ASSET_COLLECTION_NAME_PREFIX, suffix.c_str());
}

common::String GetUniqueMessageAssetCollectionName(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", MESSAGE_ASSET_COLLECTION_NAME_PREFIX, suffix.c_str());
}

common::String GetUniqueAnnotationAssetName(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_ASSET_NAME_PREFIX, suffix.c_str());
}

common::String GetUniqueAnnotationThumbnailAssetName(const common::String& spaceId, const common::String& creatorUserId)
{
    auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_THUMBNAIL_ASSET_NAME_PREFIX, suffix.c_str());
}

common::String GetUniqueAnnotationAssetFileName(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_ASSET_FILENAME_PREFIX, suffix.c_str());
}

common::String GetUniqueAnnotationThumbnailFileName(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_THUMBNAIL_ASSET_FILENAME_PREFIX, suffix.c_str());
}

common::String GetUniqueAnnotationAssetCollectionName(const common::String& spaceId, const common::String& creatorUserId)
{
    const auto suffix = GetUniqueAssetCollectionSuffix(spaceId, creatorUserId);
    return common::StringFormat("%s_%s", ANNOTATION_ASSET_COLLECTION_NAME_PREFIX, suffix.c_str());
}

common::Map<common::String, common::String> GenerateMessageAssetCollectionMetadata(const multiplayer::MessageInfo& messageData)
{
    common::Map<common::String, common::String> metadataMap;
    metadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = messageData.Message;

    return metadataMap;
}

csp::common::Array<common::ReplicatedValue> MessageInfoToReplicatedValueArray(
    multiplayer::ConversationEventType conversationEventType, const multiplayer::MessageInfo& messageInfo)
{
    csp::common::Array<common::ReplicatedValue> args(7);

    args[0] = static_cast<int64_t>(conversationEventType);
    args[1] = messageInfo.ConversationId;
    args[2] = messageInfo.CreatedTimestamp;
    args[3] = messageInfo.EditedTimestamp;
    args[4] = messageInfo.UserId;
    args[5] = messageInfo.Message;
    args[6] = messageInfo.MessageId;

    return args;
}

common::Map<common::String, common::String> GenerateConversationAssetCollectionMetadata(const multiplayer::MessageInfo& conversationData)
{
    common::Map<common::String, common::String> metadataMap;
    metadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = conversationData.Message;

    return metadataMap;
}

common::Map<common::String, common::String> GenerateAnnotationAssetCollectionMetadata(const multiplayer::AnnotationUpdateParams& annotationData,
    const csp::common::String& annotationId, const csp::common::String& annotationThumbnailId)
{
    common::Map<common::String, common::String> metadataMap;
    metadataMap[ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID] = annotationId;
    metadataMap[ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID] = annotationThumbnailId;
    metadataMap[ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV] = std::to_string(annotationData.VerticalFov).c_str();
    metadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION] = Vector3ToString(annotationData.AuthorCameraPosition);
    metadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION] = Vector4ToString(annotationData.AuthorCameraRotation);

    return metadataMap;
}

common::Map<common::String, common::String> RemoveAnnotationMetadata(const AssetCollection& messageAssetCollection)
{
    auto metadataMap = messageAssetCollection.GetMetadataImmutable();
    metadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID);
    metadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID);
    metadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV);
    metadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION);
    metadataMap.Remove(ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION);

    return metadataMap;
}

void PopulateMessageInfoFromMetadata(const csp::common::Map<csp::common::String, csp::common::String>& metadata, multiplayer::MessageInfo& info)
{
    if (metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_MESSAGE))
    {
        info.Message = metadata[ASSET_COLLECTION_METADATA_KEY_MESSAGE];
    }
    else
    {
        CSP_LOG_WARN_MSG("No Message MetaData found");
        info.Message = "";
    }
}

multiplayer::MessageInfo GetMessageInfoFromMessageAssetCollection(const csp::systems::AssetCollection& messageAssetCollection)
{
    multiplayer::MessageInfo info;
    info.ConversationId = messageAssetCollection.ParentId;
    info.CreatedTimestamp = messageAssetCollection.CreatedAt;
    info.UserId = messageAssetCollection.CreatedBy;
    info.MessageId = messageAssetCollection.Id;

    if (HasBeenEdited(messageAssetCollection))
    {
        info.EditedTimestamp = messageAssetCollection.UpdatedAt;
    }

    PopulateMessageInfoFromMetadata(messageAssetCollection.GetMetadataImmutable(), info);

    return info;
}

bool HasAnnotationMetadata(const AssetCollection& messageAssetCollection)
{
    const auto& metadataMap = messageAssetCollection.GetMetadataImmutable();
    return metadataMap.HasKey(ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID);
}

std::unordered_map<std::string, std::string> GetAnnotationThumbnailAssetIdsFromCollectionResult(const AssetCollectionsResult& result)
{
    std::unordered_map<std::string, std::string> thumbnailAssetIds;

    for (const auto& collection : result.GetAssetCollections())
    {
        auto metadata = collection.GetMetadataImmutable();
        bool hasThumbnail = metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID);

        if (hasThumbnail)
        {
            thumbnailAssetIds[collection.Id.c_str()] = metadata[ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID].c_str();
        }
    }

    return thumbnailAssetIds;
}

multiplayer::AnnotationData GetAnnotationDataFromMessageAssetCollection(const AssetCollection& messageAssetCollection)
{
    multiplayer::AnnotationData data;
    const auto& metadataMap = messageAssetCollection.GetMetadataImmutable();

    data.AnnotationId = metadataMap[ASSET_COLLECTION_METADATA_KEY_ANNOTATION_ID];
    data.AnnotationThumbnailId = metadataMap[ASSET_COLLECTION_METADATA_KEY_THUMBNAIL_ID];
    data.VerticalFov = std::stod(metadataMap[ASSET_COLLECTION_METADATA_KEY_VERTICAL_FOV].c_str());
    data.AuthorCameraPosition = StringToVector3(metadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_POSITION]);
    data.AuthorCameraRotation = StringToVector4(metadataMap[ASSET_COLLECTION_METADATA_KEY_CAMERA_ROTATION]);

    return data;
}

multiplayer::MessageInfo GetConversationInfoFromConversationAssetCollection(const csp::systems::AssetCollection& conversationAssetCollection)
{
    multiplayer::MessageInfo info;
    info.ConversationId = conversationAssetCollection.Id;
    info.CreatedTimestamp = conversationAssetCollection.CreatedAt;
    info.UserId = conversationAssetCollection.CreatedBy;

    if (HasBeenEdited(conversationAssetCollection))
    {
        info.EditedTimestamp = conversationAssetCollection.UpdatedAt;
    }

    PopulateMessageInfoFromMetadata(conversationAssetCollection.GetMetadataImmutable(), info);

    return info;
}

void AppendMetadata(csp::common::Map<csp::common::String, csp::common::String>& metadataToUpdate,
    const csp::common::Map<csp::common::String, csp::common::String>& newMetadataValues)
{
    std::unique_ptr<common::Array<common::String>> keys(const_cast<common::Array<common::String>*>(newMetadataValues.Keys()));

    for (const auto& key : *keys)
    {
        metadataToUpdate[key] = newMetadataValues[key];
    }
}

} // namespace csp::multiplayer
