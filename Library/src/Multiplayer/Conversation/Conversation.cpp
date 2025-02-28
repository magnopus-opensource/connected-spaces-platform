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

#include "Systems/Conversation/ConversationSystemHelpers.h"
#include "Web/HttpResponse.h"

namespace csp::multiplayer
{

AnnotationData::AnnotationData()
    : AnnotationThumbnailId("")
    , AnnotationId("")
    , VerticalFov(0)
    , AuthorCameraPosition()
    , AuthorCameraRotation()
{
}

AnnotationData::AnnotationData(const csp::common::String& InAnnotationThumbnailId, const csp::common::String& InAnnotationId,
    const uint16_t InVerticalFov, const csp::common::Vector3& InAuthorCameraPosition, const csp::common::Vector4& InAuthorCameraRotation)
    : AnnotationThumbnailId(InAnnotationThumbnailId)
    , AnnotationId(InAnnotationId)
    , VerticalFov(InVerticalFov)
    , AuthorCameraPosition(InAuthorCameraPosition)
    , AuthorCameraRotation(InAuthorCameraRotation)
{
}

AnnotationData::AnnotationData(const AnnotationData& InAnnotationData)
    : AnnotationThumbnailId(InAnnotationData.AnnotationThumbnailId)
    , AnnotationId(InAnnotationData.AnnotationId)
    , VerticalFov(InAnnotationData.VerticalFov)
    , AuthorCameraPosition(InAnnotationData.AuthorCameraPosition)
    , AuthorCameraRotation(InAnnotationData.AuthorCameraRotation)
{
}

csp::common::String AnnotationData::GetAnnotationThumbnailId() { return AnnotationThumbnailId; }

csp::common::String AnnotationData::GetAnnotationId() { return AnnotationId; }

uint16_t AnnotationData::GetVerticalFov() { return VerticalFov; }

csp::common::Vector3 AnnotationData::GetAuthorCameraPosition() { return AuthorCameraPosition; }

csp::common::Vector4 AnnotationData::GetAuthorCameraRotation() { return AuthorCameraRotation; }

void AnnotationData::SetAnnotationThumbnailId(const csp::common::String& InAnnotationThumbnailId) { AnnotationThumbnailId = InAnnotationThumbnailId; }

void AnnotationData::SetAnnotationId(const csp::common::String& InAnnotationId) { AnnotationId = InAnnotationId; }

void AnnotationData::SetVerticalFov(const uint16_t InVerticalFov) { VerticalFov = InVerticalFov; }

void AnnotationData::SetAuthorCameraPosition(const csp::common::Vector3& InAuthorCameraPosition) { AuthorCameraPosition = InAuthorCameraPosition; }

void AnnotationData::SetAuthorCameraRotation(const csp::common::Vector4& InAuthorCameraRotation) { AuthorCameraRotation = InAuthorCameraRotation; }

MessageInfo& MessageResult::GetMessageInfo() { return MsgInfo; }

const MessageInfo& MessageResult::GetMessageInfo() const { return MsgInfo; }

MessageInfo::MessageInfo()
    : ConversationId("")
    , IsConversation(false)
    , CreatedTimestamp("")
    , EditedTimestamp("")
    , UserId("")
    , Message("")
    , MessageId("")
{
}

MessageInfo::MessageInfo(const csp::common::String& ConversationId, bool IsConversation, const csp::common::String& CreatedTimestamp,
    const csp::common::String& EditedTimestamp, const csp::common::String& UserId, const csp::common::String& Message,
    const csp::common::String& MessageId)
    : ConversationId(ConversationId)
    , IsConversation(IsConversation)
    , CreatedTimestamp(CreatedTimestamp)
    , EditedTimestamp(EditedTimestamp)
    , UserId(UserId)
    , Message(Message)
    , MessageId(MessageId)
{
}

MessageInfo::MessageInfo(const MessageInfo& MessageData)
    : ConversationId(MessageData.ConversationId)
    , IsConversation(MessageData.IsConversation)
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

    for (auto idx = 0; idx < MessagesAssetCollections.Size(); ++idx)
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
} // namespace csp::multiplayer
