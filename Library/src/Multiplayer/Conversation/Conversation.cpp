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
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Systems/Assets/Asset.h"

#include "Common/Web/HttpResponse.h"
#include "Systems/Conversation/ConversationSystemHelpers.h"

namespace csp::multiplayer
{

AnnotationData::AnnotationData()
    : VerticalFov(0.0)
    , AuthorCameraPosition()
    , AuthorCameraRotation()
{
}

AnnotationData::AnnotationData(const csp::common::String& AnnotationId, const csp::common::String& AnnotationThumbnailId, double InVerticalFov,
    const csp::common::Vector3& InAuthorCameraPosition, const csp::common::Vector4& InAuthorCameraRotation)
    : AnnotationId(AnnotationId)
    , AnnotationThumbnailId(AnnotationThumbnailId)
    , VerticalFov(InVerticalFov)
    , AuthorCameraPosition(InAuthorCameraPosition)
    , AuthorCameraRotation(InAuthorCameraRotation)
{
}

AnnotationData::AnnotationData(const AnnotationData& InAnnotationData)
    : VerticalFov(InAnnotationData.VerticalFov)
    , AuthorCameraPosition(InAnnotationData.AuthorCameraPosition)
    , AuthorCameraRotation(InAnnotationData.AuthorCameraRotation)
{
}

MessageInfo& MessageResult::GetMessageInfo() { return MsgInfo; }

const MessageInfo& MessageResult::GetMessageInfo() const { return MsgInfo; }

MessageInfo::MessageInfo()
    : ConversationId("")
    , CreatedTimestamp("")
    , EditedTimestamp("")
    , UserId("")
    , Message("")
    , MessageId("")
{
}

MessageInfo::MessageInfo(const csp::common::String& ConversationId, bool /*IsConversation*/, const csp::common::String& Message)
    : ConversationId(ConversationId)
    , Message(Message)
{
}

MessageInfo::MessageInfo(
    const csp::common::String& ConversationId, bool /*IsConversation*/, const csp::common::String& Message, const csp::common::String& MessageId)
    : ConversationId(ConversationId)
    , Message(Message)
    , MessageId(MessageId)
{
}

MessageInfo::MessageInfo(const MessageInfo& MessageData)
    : ConversationId(MessageData.ConversationId)
    , CreatedTimestamp(MessageData.CreatedTimestamp)
    , EditedTimestamp(MessageData.EditedTimestamp)
    , UserId(MessageData.UserId)
    , Message(MessageData.Message)
    , MessageId(MessageData.MessageId)
{
}

void MessageResult::FillMessageInfo(const csp::systems::AssetCollection& MessageAssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    MsgInfo = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(MessageAssetCollection);
}

csp::common::Array<MessageInfo>& MessageCollectionResult::GetMessages() { return ConversationMessages; }

const csp::common::Array<MessageInfo>& MessageCollectionResult::GetMessages() const { return ConversationMessages; }

uint64_t MessageCollectionResult::GetTotalCount() const { return ResultTotalCount; }

void MessageCollectionResult::SetTotalCount(uint64_t Value) { ResultTotalCount = Value; }

void MessageCollectionResult::FillMessageInfoCollection(const csp::common::Array<csp::systems::AssetCollection>& MessagesAssetCollections)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    ConversationMessages = csp::common::Array<MessageInfo>(MessagesAssetCollections.Size());

    MessageInfo MsgInfo;

    for (size_t idx = 0; idx < MessagesAssetCollections.Size(); ++idx)
    {
        MsgInfo = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(MessagesAssetCollections[idx]);
        ConversationMessages[idx] = MsgInfo;
    }
}

MessageInfo& ConversationResult::GetConversationInfo() { return ConvoInfo; }

const MessageInfo& ConversationResult::GetConversationInfo() const { return ConvoInfo; }

void ConversationResult::FillConversationInfo(const csp::systems::AssetCollection& ConversationAssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    ConvoInfo = systems::ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(ConversationAssetCollection);
}

uint64_t NumberOfRepliesResult::GetCount() const { return Count; }

void NumberOfRepliesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { ResultBase::OnResponse(ApiResponse); }

void AnnotationResult::ParseAnnotationAssetData(const systems::AssetCollection& AssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    Data = systems::ConversationSystemHelpers::GetAnnotationDataFromMessageAssetCollection(AssetCollection);
}

const AnnotationData& AnnotationResult::GetAnnotationData() const { return Data; }

const csp::systems::Asset& AnnotationResult::GetAnnotationAsset() const { return AnnotationAsset; }

const csp::systems::Asset& AnnotationResult::GetAnnotationThumbnailAsset() const { return AnnotationThumbnailAsset; }

void AnnotationResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { ResultBase::OnResponse(ApiResponse); }

const csp::common::Map<csp::common::String, csp::systems::Asset>& AnnotationThumbnailCollectionResult::GetAnnotationThumbnailAssetsMap() const
{
    return AnnotationThumbnailAssetsMap;
}

uint64_t AnnotationThumbnailCollectionResult::GetTotalCount() const { return AnnotationThumbnailAssetsMap.Size(); }

void AnnotationThumbnailCollectionResult::ParseAssets(const systems::AssetsResult& AssetResult)
{
    common::Array<systems::Asset> Assets = AssetResult.GetAssets();

    for (size_t i = 0; i < Assets.Size(); ++i)
    {
        AnnotationThumbnailAssetsMap[Assets[i].AssetCollectionId] = Assets[i];
    }
}
} // namespace csp::multiplayer
