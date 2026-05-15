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

AnnotationData::AnnotationData(const csp::common::String& annotationId, const csp::common::String& annotationThumbnailId, double inVerticalFov,
    const csp::common::Vector3& inAuthorCameraPosition, const csp::common::Vector4& inAuthorCameraRotation)
    : AnnotationId(annotationId)
    , AnnotationThumbnailId(annotationThumbnailId)
    , VerticalFov(inVerticalFov)
    , AuthorCameraPosition(inAuthorCameraPosition)
    , AuthorCameraRotation(inAuthorCameraRotation)
{
}

AnnotationData::AnnotationData(const AnnotationData& inAnnotationData)
    : VerticalFov(inAnnotationData.VerticalFov)
    , AuthorCameraPosition(inAnnotationData.AuthorCameraPosition)
    , AuthorCameraRotation(inAnnotationData.AuthorCameraRotation)
{
}

AnnotationData& AnnotationData::operator=(const AnnotationData& inAnnotationData)
{
    if (this != &inAnnotationData)
    {
        VerticalFov = inAnnotationData.VerticalFov;
        AuthorCameraPosition = inAnnotationData.AuthorCameraPosition;
        AuthorCameraRotation = inAnnotationData.AuthorCameraRotation;
    }

    return *this;
}

MessageInfo& MessageResult::GetMessageInfo() { return m_msgInfo; }

const MessageInfo& MessageResult::GetMessageInfo() const { return m_msgInfo; }

MessageInfo::MessageInfo()
    : ConversationId("")
    , CreatedTimestamp("")
    , EditedTimestamp("")
    , UserId("")
    , Message("")
    , MessageId("")
{
}

MessageInfo::MessageInfo(const csp::common::String& conversationId, bool /*IsConversation*/, const csp::common::String& message)
    : ConversationId(conversationId)
    , Message(message)
{
}

MessageInfo::MessageInfo(
    const csp::common::String& conversationId, bool /*IsConversation*/, const csp::common::String& message, const csp::common::String& messageId)
    : ConversationId(conversationId)
    , Message(message)
    , MessageId(messageId)
{
}

MessageInfo::MessageInfo(const MessageInfo& messageData)
    : ConversationId(messageData.ConversationId)
    , CreatedTimestamp(messageData.CreatedTimestamp)
    , EditedTimestamp(messageData.EditedTimestamp)
    , UserId(messageData.UserId)
    , Message(messageData.Message)
    , MessageId(messageData.MessageId)
{
}

MessageInfo& MessageInfo::operator=(const MessageInfo& messageData)
{
    if (this != &messageData)
    {
        ConversationId = messageData.ConversationId;
        CreatedTimestamp = messageData.CreatedTimestamp;
        EditedTimestamp = messageData.EditedTimestamp;
        UserId = messageData.UserId;
        Message = messageData.Message;
        MessageId = messageData.MessageId;
    }

    return *this;
}

bool MessageInfo::operator==(const MessageInfo& other) const
{
    return ConversationId == other.ConversationId && CreatedTimestamp == other.CreatedTimestamp && EditedTimestamp == other.EditedTimestamp
        && UserId == other.UserId && Message == other.Message && MessageId == other.MessageId;
}

bool MessageInfo::operator!=(const MessageInfo& other) const { return !(*this == other); }

void MessageResult::FillMessageInfo(const csp::systems::AssetCollection& messageAssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    m_msgInfo = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(messageAssetCollection);
}

csp::common::Array<MessageInfo>& MessageCollectionResult::GetMessages() { return m_conversationMessages; }

const csp::common::Array<MessageInfo>& MessageCollectionResult::GetMessages() const { return m_conversationMessages; }

uint64_t MessageCollectionResult::GetTotalCount() const { return m_resultTotalCount; }

void MessageCollectionResult::SetTotalCount(uint64_t value) { m_resultTotalCount = value; }

void MessageCollectionResult::FillMessageInfoCollection(const csp::common::Array<csp::systems::AssetCollection>& messagesAssetCollections)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    m_conversationMessages = csp::common::Array<MessageInfo>(messagesAssetCollections.Size());

    MessageInfo msgInfo;

    for (size_t idx = 0; idx < messagesAssetCollections.Size(); ++idx)
    {
        msgInfo = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(messagesAssetCollections[idx]);
        m_conversationMessages[idx] = msgInfo;
    }
}

MessageInfo& ConversationResult::GetConversationInfo() { return m_convoInfo; }

const MessageInfo& ConversationResult::GetConversationInfo() const { return m_convoInfo; }

void ConversationResult::FillConversationInfo(const csp::systems::AssetCollection& conversationAssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    m_convoInfo = systems::ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(conversationAssetCollection);
}

uint64_t NumberOfRepliesResult::GetCount() const { return m_count; }

void NumberOfRepliesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse) { ResultBase::OnResponse(apiResponse); }

void AnnotationResult::ParseAnnotationAssetData(const systems::AssetCollection& assetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    m_data = systems::ConversationSystemHelpers::GetAnnotationDataFromMessageAssetCollection(assetCollection);
}

const AnnotationData& AnnotationResult::GetAnnotationData() const { return m_data; }

const csp::systems::Asset& AnnotationResult::GetAnnotationAsset() const { return m_annotationAsset; }

const csp::systems::Asset& AnnotationResult::GetAnnotationThumbnailAsset() const { return m_annotationThumbnailAsset; }

void AnnotationResult::OnResponse(const csp::services::ApiResponseBase* apiResponse) { ResultBase::OnResponse(apiResponse); }

const csp::common::Map<csp::common::String, csp::systems::Asset>& AnnotationThumbnailCollectionResult::GetAnnotationThumbnailAssetsMap() const
{
    return m_annotationThumbnailAssetsMap;
}

uint64_t AnnotationThumbnailCollectionResult::GetTotalCount() const { return m_annotationThumbnailAssetsMap.Size(); }

void AnnotationThumbnailCollectionResult::ParseAssets(const systems::AssetsResult& assetResult)
{
    common::Array<systems::Asset> assets = assetResult.GetAssets();

    for (size_t i = 0; i < assets.Size(); ++i)
    {
        m_annotationThumbnailAssetsMap[assets[i].AssetCollectionId] = assets[i];
    }
}
} // namespace csp::multiplayer
