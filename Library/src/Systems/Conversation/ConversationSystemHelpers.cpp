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
constexpr const char* ASSET_COLLECTION_METADATA_KEY_EDITED = "EditedTimestamp";

constexpr const char* ASSET_COLLECTION_METADATA_KEY_MESSAGE = "Message";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION = "IsConversation";

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
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_EDITED] = MessageData.EditedTimestamp;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION] = BoolToString(MessageData.IsConversation);

    return MetadataMap;
}

multiplayer::MessageInfo GetMessageInfoFromMessageAssetCollection(const csp::systems::AssetCollection& MessageAssetCollection)
{
    multiplayer::MessageInfo MsgInfo;
    MsgInfo.MessageId = MessageAssetCollection.Id;
    MsgInfo.ConversationId = MessageAssetCollection.ParentId;
    MsgInfo.EditedTimestamp = MessageAssetCollection.UpdatedAt;
    MsgInfo.UserId = MessageAssetCollection.CreatedBy;
    MsgInfo.CreatedTimestamp = MessageAssetCollection.CreatedAt;
    MsgInfo.IsConversation = false;

    const auto Metadata = MessageAssetCollection.GetMetadataImmutable();

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION))
    {
        MsgInfo.IsConversation = StringToBool(Metadata[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION]);
    }
    else
    {
        CSP_LOG_WARN_MSG("No IsConversation MetaData found, This is likely due to the current space outdating ConversationSpaceComponent "
                         "improvements: Default metadata has automatically been created for this space as a result. ");
        MsgInfo.IsConversation = false;
    }

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_MESSAGE))
    {
        MsgInfo.Message = Metadata[ASSET_COLLECTION_METADATA_KEY_MESSAGE];
    }
    else
    {
        CSP_LOG_WARN_MSG("No Message MetaData found, This is likely due to the current space outdating ConversationSpaceComponent "
                         "improvements: Default metadata has automatically been created for this space as a result. ");
        MsgInfo.Message = "";
    }

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_EDITED))
    {
        MsgInfo.EditedTimestamp = Metadata[ASSET_COLLECTION_METADATA_KEY_EDITED];
    }
    else
    {
        CSP_LOG_WARN_MSG("No Edited MetaData found, This is likely due to the current space outdating ConversationSpaceComponent "
                         "improvements: Default metadata has automatically been created for this space as a result. ")
        MsgInfo.EditedTimestamp = "";
    }

    return MsgInfo;
}

csp::common::Array<multiplayer::ReplicatedValue> MessageInfoToReplicatedValueArray(const multiplayer::ConversationEventParams& Params)
{
    csp::common::Array<multiplayer::ReplicatedValue> Args(8);

    Args[0] = static_cast<int64_t>(Params.MessageType);
    Args[1] = Params.MessageInfo.ConversationId;
    Args[2] = Params.MessageInfo.IsConversation;
    Args[3] = Params.MessageInfo.CreatedTimestamp;
    Args[4] = Params.MessageInfo.EditedTimestamp;
    Args[5] = Params.MessageInfo.UserId;
    Args[6] = Params.MessageInfo.Message;
    Args[7] = Params.MessageInfo.MessageId;

    return Args;
}

common::Map<common::String, common::String> GenerateConversationAssetCollectionMetadata(const multiplayer::MessageInfo& ConversationData)
{
    common::Map<common::String, common::String> MetadataMap;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = ConversationData.Message;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_EDITED] = ConversationData.EditedTimestamp;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION] = BoolToString(ConversationData.IsConversation);
    return MetadataMap;
}

multiplayer::MessageInfo GetConversationInfoFromConversationAssetCollection(const csp::systems::AssetCollection& ConversationAssetCollection)
{
    multiplayer::MessageInfo ConvoInfo;
    ConvoInfo.ConversationId = ConversationAssetCollection.Id;
    ConvoInfo.EditedTimestamp = ConversationAssetCollection.UpdatedAt;
    ConvoInfo.UserId = ConversationAssetCollection.CreatedBy;
    ConvoInfo.CreatedTimestamp = ConversationAssetCollection.CreatedAt;
    ConvoInfo.IsConversation = true;

    const auto Metadata = ConversationAssetCollection.GetMetadataImmutable();

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION))
    {
        ConvoInfo.IsConversation = Metadata[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION];
    }
    else
    {
        CSP_LOG_WARN_MSG("No IsConversation MetaData found, This is likely due to the current space outdating ConversationSpaceComponent "
                         "improvements: Default metadata has automatically been created for this space as a result. ");
        ConvoInfo.IsConversation = true;
    }

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_MESSAGE))
    {
        ConvoInfo.Message = Metadata[ASSET_COLLECTION_METADATA_KEY_MESSAGE];
    }
    else
    {
        CSP_LOG_WARN_MSG("No Message MetaData found, This is likely due to the current space outdating ConversationSpaceComponent "
                         "improvements: Default metadata has automatically been created for this space as a result. ");
        ConvoInfo.Message = "";
    }

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_EDITED))
    {
        ConvoInfo.EditedTimestamp = Metadata[ASSET_COLLECTION_METADATA_KEY_EDITED];
    }
    else
    {
        CSP_LOG_WARN_MSG("No Edited MetaData found, This is likely due to the current space outdating ConversationSpaceComponent "
                         "improvements: Default metadata has automatically been created for this space as a result. ")
        ConvoInfo.EditedTimestamp = "";
    }

    return ConvoInfo;
}

} // namespace csp::multiplayer
