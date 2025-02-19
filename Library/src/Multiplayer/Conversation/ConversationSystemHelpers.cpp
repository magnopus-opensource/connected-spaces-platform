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
#include "CSP/Systems/Assets/AssetCollection.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"

constexpr const char* CONVERSATION_CONTAINER_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_CONVERSATION_CONTAINER";
constexpr const char* MESSAGE_ASSET_COLLECTION_NAME_PREFIX = "ASSET_COLLECTION_MESSAGE";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_EDITED = "EditedTimestamp";

constexpr const char* ASSET_COLLECTION_METADATA_KEY_MESSAGE = "Message";
constexpr const char* ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION = "IsConversation";

namespace csp::multiplayer
{

String ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(const String& SpaceId, const String& CreatorUserId)
{
    auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return StringFormat("%s_%s", CONVERSATION_CONTAINER_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
}

String ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(const String& SpaceId, const String& CreatorUserId)
{
    auto Suffix = GetUniqueAssetCollectionSuffix(SpaceId, CreatorUserId);
    return StringFormat("%s_%s", MESSAGE_ASSET_COLLECTION_NAME_PREFIX, Suffix.c_str());
}

Map<String, String> ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(const MessageInfo& MessageData)
{
    Map<String, String> MetadataMap;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = MessageData.Message;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_EDITED] = MessageData.EditedTimestamp;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION] = BoolToString(MessageData.IsConversation);

    return MetadataMap;
}

MessageInfo ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(const csp::systems::AssetCollection& MessageAssetCollection)
{
    MessageInfo MsgInfo;
    MsgInfo.MessageId = MessageAssetCollection.Id;
    MsgInfo.ConversationId = MessageAssetCollection.ParentId;
    MsgInfo.EditedTimestamp = MessageAssetCollection.UpdatedAt;
    MsgInfo.UserId = MessageAssetCollection.CreatedBy;
    MsgInfo.CreatedTimestamp = MessageAssetCollection.CreatedAt;
    MsgInfo.IsConversation = false;

    const auto Metadata = MessageAssetCollection.GetMetadataImmutable();

    if (Metadata.HasKey(ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION))
    {
        MsgInfo.IsConversation = Metadata[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION];
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

Map<String, String> ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(const MessageInfo& ConversationData)
{
    Map<String, String> MetadataMap;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_MESSAGE] = ConversationData.Message;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_EDITED] = ConversationData.EditedTimestamp;
    MetadataMap[ASSET_COLLECTION_METADATA_KEY_ISCONVERSATION] = BoolToString(ConversationData.IsConversation);
    return MetadataMap;
}

MessageInfo ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(
    const csp::systems::AssetCollection& ConversationAssetCollection)
{
    MessageInfo ConvoInfo;
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

bool ConversationSystemHelpers::StringToBool(const String& value) { return (value == "true") ? true : false; }

String ConversationSystemHelpers::BoolToString(const bool value) { return value ? "true" : "false"; }

String ConversationSystemHelpers::Vector3ToString(const Vector3& value)
{
    return StringFormat("%s,%s,%s", std::to_string(value.X).c_str(), std::to_string(value.Y).c_str(), std::to_string(value.Z).c_str());
}

Vector3 ConversationSystemHelpers::StringToVector3(const String& value)
{
    List<String> StringList = value.Split(',');
    return Vector3(std::stof(StringList[0].c_str()), std::stof(StringList[1].c_str()), std::stof(StringList[2].c_str()));
}

String ConversationSystemHelpers::Vector4ToString(const Vector4& value)
{
    return StringFormat("%s,%s,%s,%s", std::to_string(value.X).c_str(), std::to_string(value.Y).c_str(), std::to_string(value.Z).c_str(),
        std::to_string(value.W).c_str());
}

Vector4 ConversationSystemHelpers::StringToVector4(const String& value)
{
    List<String> StringList = value.Split(',');
    return Vector4(
        std::stof(StringList[0].c_str()), std::stof(StringList[1].c_str()), std::stof(StringList[2].c_str()), std::stof(StringList[3].c_str()));
}

SpaceTransform ConversationSystemHelpers::StringToSpaceTransform(const String& value)
{
    List<String> StringList = value.Split('|');
    return SpaceTransform(StringToVector3(StringList[0]), StringToVector4(StringList[1]), StringToVector3(StringList[2]));
}

String ConversationSystemHelpers::SpaceTransformToString(const SpaceTransform& value)
{
    return StringFormat(
        "%s|%s|%s", Vector3ToString(value.Position).c_str(), Vector4ToString(value.Rotation).c_str(), Vector3ToString(value.Scale).c_str());
}

String ConversationSystemHelpers::GetUniqueAssetCollectionSuffix(const String& SpaceId, const String& CreatorUserId)
{
    const auto NowTimepoint = std::chrono::system_clock::now();
    const auto MillisecondsSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(NowTimepoint.time_since_epoch()).count();

    // TODO: consider appending a random value too if the same user creates two messages in the same millisecond
    return StringFormat("%s_%s_%llu", SpaceId.c_str(), CreatorUserId.c_str(), MillisecondsSinceEpoch);
}

} // namespace csp::multiplayer
