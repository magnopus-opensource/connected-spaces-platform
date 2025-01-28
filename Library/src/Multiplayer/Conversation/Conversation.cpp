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

#include "ConversationSystemHelpers.h"
#include "Web/HttpResponse.h"

namespace csp::multiplayer
{

MessageInfo& MessageResult::GetMessageInfo() { return MsgInfo; }

const MessageInfo& MessageResult::GetMessageInfo() const { return MsgInfo; }

MessageInfo::MessageInfo(const MessageInfo& MessageData)
{
    Id = MessageData.Id;
    ConversationId = MessageData.ConversationId;
    Timestamp = MessageData.Timestamp;
    UserID = MessageData.UserID;
    UserDisplayName = MessageData.UserDisplayName;
    Message = MessageData.Message;
    Edited = MessageData.Edited;
}

ConversationInfo::ConversationInfo(const ConversationInfo& ConversationData)
{
    ConversationId = ConversationData.ConversationId;
    Timestamp = ConversationData.Timestamp;
    UserID = ConversationData.UserID;
    UserDisplayName = ConversationData.UserDisplayName;
    Message = ConversationData.Message;
    Edited = ConversationData.Edited;
    Resolved = ConversationData.Resolved;
    CameraPosition = ConversationData.CameraPosition;
}

void MessageResult::FillMessageInfo(const csp::systems::AssetCollection& MessageAssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    MsgInfo = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(MessageAssetCollection);
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
        MsgInfo = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(MessagesAssetCollections[idx]);
        ConversationMessages[idx] = MsgInfo;
    }
}

ConversationInfo& ConversationResult::GetConversationInfo() { return ConvoInfo; }

const ConversationInfo& ConversationResult::GetConversationInfo() const { return ConvoInfo; }

void ConversationResult::FillConversationInfo(const csp::systems::AssetCollection& ConversationAssetCollection)
{
    SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    ConvoInfo = ConversationSystemHelpers::GetConvosationInfoFromConvosationAssetCollection(ConversationAssetCollection);
}
} // namespace csp::multiplayer
