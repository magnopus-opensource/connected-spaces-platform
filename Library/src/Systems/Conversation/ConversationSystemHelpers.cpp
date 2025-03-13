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

constexpr const char* ASSET_COLLECTION_METADATA_KEY_MESSAGE = "Message";

namespace
{
    // When an asset collection hasn't been edited, the UpdatedAt timestamp is the same as the CreatedAt timestamp.
    // We want this be an empty string if the conversation hasn't been modified.
    bool HasBeenEdited(const AssetCollection& AssetCollection) { return AssetCollection.CreatedAt != AssetCollection.UpdatedAt; }
}

common::String GetUniqueAssetCollectionSuffix(const common::String& SpaceId, const common::String& CreatorUserId)
{
    const auto NowTimepoint = std::chrono::system_clock::now();
    const auto MillisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(NowTimepoint.time_since_epoch()).count();

    // TODO: consider appending a random value too if the same user creates two messages in the same millisecond
    return common::StringFormat("%s_%s_%llu", SpaceId.c_str(), CreatorUserId.c_str(), MillisecondsSinceEpoch);
}

bool StringToBool(const common::String& value) { return (value == "true") ? true : false; }

common::String BoolToString(bool value) { return value ? "true" : "false"; }

common::String GetUniqueConversationContainerAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", CONVERSATION_CONTAINER_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
}

common::String GetUniqueMessageAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId)
{
    auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return common::StringFormat("%s_%s", MESSAGE_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
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
