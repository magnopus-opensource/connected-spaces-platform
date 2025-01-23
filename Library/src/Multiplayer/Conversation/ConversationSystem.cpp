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
#include "CSP/Multiplayer/Conversation/ConversationSystem.h"

#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "ConversationSystemHelpers.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/EventSerialisation.h"
#include "Systems/ResultHelpers.h"
#include "Web/HttpResponse.h"

namespace csp::multiplayer
{

ConversationSystem::ConversationSystem(MultiplayerConnection* Connection)
    : SystemBase(Connection->EventBusPtr)
    , Connection(nullptr)
{
    RegisterSystemCallback();
}

ConversationSystem::~ConversationSystem() { DeregisterSystemCallback(); }

void ConversationSystem::SetConnection(csp::multiplayer::SignalRConnection* InConnection) { Connection = InConnection; }

void ConversationSystem::StoreConversationMessage(const csp::common::String& ConversationId, const csp::systems::Space& Space,
    const csp::common::String& UserId, const csp::common::String& SenderDisplayName, const csp::common::String& Message,
    MessageResultCallback Callback) const
{
    const csp::systems::AssetCollectionResultCallback AddCommentCallback = [=](const csp::systems::AssetCollectionResult& AddCommentResult)
    {
        if (AddCommentResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (AddCommentResult.GetResultCode() == csp::systems::EResultCode::Success)
        {
            MessageResult Result;
            Result.FillMessageInfo(AddCommentResult.GetAssetCollection());
            INVOKE_IF_NOT_NULL(Callback, Result);
        }
        else
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "The Comment asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(AddCommentResult.GetResultCode()), AddCommentResult.GetHttpResultCode());

            const MessageResult InternalResult(AddCommentResult.GetResultCode(), AddCommentResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(Space.Id, UserId);
    MessageInfo DefaultMessageInfo = MessageInfo();
    DefaultMessageInfo.Edited = false;
    DefaultMessageInfo.Message = Message;
    DefaultMessageInfo.UserDisplayName = SenderDisplayName;
    const auto MessageMetadata = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(DefaultMessageInfo);

    AssetSystem->CreateAssetCollection(Space.Id, ConversationId, UniqueAssetCollectionName, MessageMetadata,
        csp::systems::EAssetCollectionType::COMMENT, nullptr, AddCommentCallback);
}

void ConversationSystem::DeleteMessages(const csp::common::Array<csp::systems::AssetCollection>& Messages, csp::systems::NullResultCallback Callback)
{
    const auto MessagesCount = Messages.Size();

    if (MessagesCount == 0)
    {
        csp::systems::NullResult InternalResult(csp::systems::EResultCode::Success, (uint16_t)csp::web::EResponseCodes::ResponseNoContent);
        INVOKE_IF_NOT_NULL(Callback, InternalResult);

        return;
    }

    const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    auto DeletionCounterDeleter = [](size_t* ptr) { CSP_DELETE(ptr); };

    std::shared_ptr<size_t> DeletionCounter(CSP_NEW size_t, DeletionCounterDeleter);
    *DeletionCounter = 0;

    for (auto idx = 0; idx < MessagesCount; ++idx)
    {
        const csp::systems::NullResultCallback DeleteCommentCallback = [=](const csp::systems::NullResult& DeleteCommentResult)
        {
            if (DeleteCommentResult.GetResultCode() == csp::systems::EResultCode::Success)
            {
                ++*DeletionCounter;

                if (*DeletionCounter == MessagesCount)
                {
                    INVOKE_IF_NOT_NULL(Callback, DeleteCommentResult);
                }
            }
            else if (DeleteCommentResult.GetResultCode() == csp::systems::EResultCode::Failed)
            {

                CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Delete asset collection for message ID %s has failed. ResCode: %d, HttpResCode: %d",
                    Messages[idx].Id.c_str(), static_cast<int>(DeleteCommentResult.GetResultCode()), DeleteCommentResult.GetHttpResultCode());

                ++*DeletionCounter;

                if (*DeletionCounter == MessagesCount)
                {
                    INVOKE_IF_NOT_NULL(Callback, DeleteCommentResult);
                }
            }
        };

        AssetSystem->DeleteAssetCollection(Messages[idx], DeleteCommentCallback);
    }
}

void ConversationSystem::CreateConversation(const csp::common::String& Message, csp::systems::StringResultCallback Callback)
{
    auto* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();
    const auto& CurrentSpace = SpaceSystem->GetCurrentSpace();

    if (CurrentSpace.Id.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("ConversationSystem::CreateConversation() failed: Enter a space before adding a message");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::StringResult>());

        return;
    }

    const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    if (UserSystem->GetLoginState().State != csp::systems::ELoginState::LoggedIn)
    {
        CSP_LOG_ERROR_MSG("ConversationSystem::CreateConversation() failed: Log in before adding a message");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::StringResult>());

        return;
    };

    const auto& UserId = UserSystem->GetLoginState().UserId;
    const auto& SpaceId = CurrentSpace.Id;

    csp::systems::ProfileResultCallback GetProfileCallback = [Callback, UserId, SpaceId, Message](const csp::systems::ProfileResult& GetProfileResult)
    {
        if (GetProfileResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (GetProfileResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            const csp::systems::StringResult InternalResult(GetProfileResult.GetResultCode(), GetProfileResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        // We need to firstly create the comment container asset collection
        const csp::systems::AssetCollectionResultCallback AddCommentContainerCallback
            = [=](const csp::systems::AssetCollectionResult& AddCommentContainerResult)
        {
            if (AddCommentContainerResult.GetResultCode() == csp::systems::EResultCode::InProgress)
            {
                return;
            }

            if (AddCommentContainerResult.GetResultCode() == csp::systems::EResultCode::Success)
            {
                const auto& ConversationId = AddCommentContainerResult.GetAssetCollection().Id;

                csp::systems::StringResult InternalResult(AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());
                InternalResult.SetValue(ConversationId);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            }
            else
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
                    "The Comment Container asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                    (int)AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());

                const csp::systems::StringResult InternalResult(
                    AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            }
        };

        const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(SpaceId, UserId);
        ConversationInfo DefaultConversationInfo = ConversationInfo();
        DefaultConversationInfo.UserDisplayName = GetProfileResult.GetProfile().DisplayName;
        DefaultConversationInfo.Message = Message;
        DefaultConversationInfo.Edited = false;
        DefaultConversationInfo.Resolved = false;
        DefaultConversationInfo.CameraPosition = SpaceTransform();

        auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
        AssetSystem->CreateAssetCollection(SpaceId, nullptr, UniqueAssetCollectionName,
            ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(DefaultConversationInfo),
            csp::systems::EAssetCollectionType::COMMENT_CONTAINER, nullptr, AddCommentContainerCallback);
    };

    UserSystem->GetProfileByUserId(UserId, GetProfileCallback);
}

void ConversationSystem::AddMessageToConversation(const csp::common::String& ConversationId, const csp::common::String& SenderDisplayName,
    const csp::common::String& Message, MessageResultCallback Callback)
{
    auto* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();
    const auto& CurrentSpace = SpaceSystem->GetCurrentSpace();

    if (CurrentSpace.Id.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("ConversationSystem::AddMessageToConversation() failed: Enter a space before adding a message");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

        return;
    }

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    if (UserSystem->GetLoginState().State != csp::systems::ELoginState::LoggedIn)
    {
        CSP_LOG_ERROR_MSG("ConversationSystem::AddMessageToConversation() failed: Log in before adding a message");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

        return;
    }

    const auto& UserId = UserSystem->GetLoginState().UserId;

    const MessageResultCallback MessageResultCallback = [Callback, ConversationId, this](const MessageResult& MessageResultCallbackResult)
    {
        const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [Callback, MessageResultCallbackResult, this](ErrorCode Error)
        {
            if (Error != ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("AddMessageToConversation: SignalR connection: Error");

                const MessageResult InternalResult(
                    csp::systems::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseInternalServerError));
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            INVOKE_IF_NOT_NULL(Callback, MessageResultCallbackResult);
        };

        EventBusPtr->SendNetworkEvent(
            "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::NewMessage), ConversationId }, signalRCallback);
    };

    StoreConversationMessage(ConversationId, CurrentSpace, UserId, SenderDisplayName, Message, MessageResultCallback);
}

void ConversationSystem::GetMessagesFromConversation(const csp::common::String& ConversationId, const csp::common::Optional<int>& ResultsSkipNumber,
    const csp::common::Optional<int>& ResultsMaxNumber, MessageCollectionResultCallback Callback)
{
    csp::systems::AssetCollectionsResultCallback GetMessagesCallback = [Callback](const csp::systems::AssetCollectionsResult& GetMessagesResult)
    {
        if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        MessageCollectionResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());

        if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(GetMessagesResult.GetResultCode()), GetMessagesResult.GetHttpResultCode());

            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        InternalResult.SetTotalCount(GetMessagesResult.GetTotalCount());
        InternalResult.FillMessageInfoCollection(GetMessagesResult.GetAssetCollections());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    Array<csp::systems::EAssetCollectionType> PrototypeTypes = { csp::systems::EAssetCollectionType::COMMENT };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->FindAssetCollections(
        nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, ResultsSkipNumber, ResultsMaxNumber, GetMessagesCallback);
}

void ConversationSystem::GetMessage(const csp::common::String& MessageId, MessageResultCallback Callback)
{
    const csp::systems::AssetCollectionResultCallback GetMessageCallback = [Callback](const csp::systems::AssetCollectionResult& GetMessageResult)
    {
        if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());

        if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
                "The retrieval of the Message asset collection was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(GetMessageResult.GetResultCode()), GetMessageResult.GetHttpResultCode());

            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        InternalResult.FillMessageInfo(GetMessageResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystem::SetMessageInformation(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback)
{
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    csp::systems::AssetCollectionResultCallback GetMessageCallback
        = [Callback, MessageId, MessageData, AssetSystem, this](const csp::systems::AssetCollectionResult& GetMessageResult)
    {
        csp::systems::AssetCollectionResultCallback GetUpdatedMessageCallback
            = [Callback, MessageId, GetMessageResult, this](const csp::systems::AssetCollectionResult& GetUpdatedMessageResult)

        {
            if (GetUpdatedMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
            {
                return;
            }

            if (GetUpdatedMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
                    "The Update of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
                    static_cast<int>(GetMessageResult.GetResultCode()), GetMessageResult.GetHttpResultCode());

                const MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [Callback, GetUpdatedMessageResult](ErrorCode Error)
            {
                if (Error != ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("UpdatedMessageInfo: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

                    return;
                }

                MessageResult Result(GetUpdatedMessageResult.GetResultCode(), GetUpdatedMessageResult.GetHttpResultCode());
                Result.FillMessageInfo(GetUpdatedMessageResult.GetAssetCollection());
                INVOKE_IF_NOT_NULL(Callback, Result);
            };

            EventBusPtr->SendNetworkEvent(
                "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::MessageInformation), MessageId }, signalRCallback);
        };

        if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(GetMessageResult.GetResultCode()), GetMessageResult.GetHttpResultCode());

            const MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        MessageInfo NewMessageData(MessageData);

        if (MessageData.Edited != true)
        {
            if (ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection()).Message
                != NewMessageData.Message)
            {
                NewMessageData.Edited = true;
            }
        }

        AssetSystem->UpdateAssetCollectionMetadata(GetMessageResult.GetAssetCollection(),
            ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(NewMessageData), nullptr, GetUpdatedMessageCallback);
    };

    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystem::GetMessageInformation(const csp::common::String& MessageId, MessageResultCallback Callback)
{
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    csp::systems::AssetCollectionResultCallback GetMessageCallback = [Callback](const csp::systems::AssetCollectionResult& GetMessageResult)
    {
        if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());

        if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        InternalResult.FillMessageInfo(GetMessageResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystem::DeleteConversation(const csp::common::String& ConversationId, csp::systems::NullResultCallback Callback)
{
    csp::systems::AssetCollectionsResultCallback GetMessagesCallback
        = [Callback, ConversationId, this](const csp::systems::AssetCollectionsResult& GetMessagesResult)
    {
        if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
                (int)GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());

            csp::systems::NullResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const csp::systems::NullResultCallback DeleteMessagesCallback
            = [Callback, ConversationId, this](const csp::systems::NullResult& DeleteMessagesResult)
        {
            if (DeleteMessagesResult.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Not all Message asset collections were deleted. ResCode: %d, HttpResCode: %d",
                    (int)DeleteMessagesResult.GetResultCode(), DeleteMessagesResult.GetHttpResultCode());

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

                return;
            }

            const csp::systems::NullResultCallback DeleteConversationAssetCollectionCallback
                = [Callback](const csp::systems::NullResult& DeleteConversationAssetCollectionResult)
            {
                if (DeleteConversationAssetCollectionResult.GetResultCode() == csp::systems::EResultCode::InProgress)
                {
                    return;
                }

                if (DeleteConversationAssetCollectionResult.GetResultCode() == csp::systems::EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
                        "The deletion of the Conversation asset collection was not successful. ResCode: %d, HttpResCode: %d",
                        (int)DeleteConversationAssetCollectionResult.GetResultCode(), DeleteConversationAssetCollectionResult.GetHttpResultCode());

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

                    return;
                }

                INVOKE_IF_NOT_NULL(Callback, DeleteConversationAssetCollectionResult);
            };

            // Delete also the conversation asset collection
            csp::systems::AssetCollection ConversationAssetCollection;
            ConversationAssetCollection.Id = ConversationId;

            DeleteMessages({ ConversationAssetCollection }, DeleteConversationAssetCollectionCallback);
        };

        const csp::systems::NullResultCallback NullResultCallback
            = [Callback, ConversationId, this](const csp::systems::NullResult& NullResultCallbackResult)
        {
            const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [Callback, NullResultCallbackResult](ErrorCode Error)
            {
                if (Error != ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("DeleteConversation: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

                    return;
                }

                INVOKE_IF_NOT_NULL(Callback, NullResultCallbackResult);
            };

            EventBusPtr->SendNetworkEvent(
                "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::DeleteConversation), ConversationId }, signalRCallback);
        };

        const auto& Messages = GetMessagesResult.GetAssetCollections();
        DeleteMessages(Messages, NullResultCallback);
    };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    Array<csp::systems::EAssetCollectionType> PrototypeTypes = { csp::systems::EAssetCollectionType::COMMENT };

    // Retrieve first the messages from this conversation
    AssetSystem->FindAssetCollections(nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, nullptr, nullptr, GetMessagesCallback);
}

void ConversationSystem::DeleteMessage(const csp::common::String& MessageId, csp::systems::NullResultCallback Callback)
{
    csp::systems::AssetCollection MessageAssetCollection;
    MessageAssetCollection.Id = MessageId;

    const csp::systems::NullResultCallback NullCallback = [Callback, MessageId, this](const csp::systems::NullResult& NullCallbackResult)
    {
        const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [Callback, NullCallbackResult](ErrorCode Error)
        {
            if (Error != ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("DeleteMessage: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

                return;
            }

            INVOKE_IF_NOT_NULL(Callback, NullCallbackResult);
        };

        EventBusPtr->SendNetworkEvent(
            "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::DeleteMessage), MessageId }, signalRCallback);
    };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->DeleteAssetCollection(MessageAssetCollection, NullCallback);
}

void ConversationSystem::SetConversationInformation(
    const csp::common::String& ConversationId, const ConversationInfo& ConversationData, ConversationResultCallback Callback)
{
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    csp::systems::AssetCollectionResultCallback GetConversationCallback
        = [Callback, ConversationId, ConversationData, AssetSystem, this](const csp::systems::AssetCollectionResult& GetConversationResult)
    {
        if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
                "The retrieval of Conversation asset collections was not successful. ResCode: %d, HttpResCode: %d",
                (int)GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());

            const ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        csp::systems::ProfileResultCallback GetProfileCallback = [Callback, GetConversationResult, ConversationId, ConversationData, AssetSystem,
                                                                     this](const csp::systems::ProfileResult& GetProfileResult)
        {
            if (GetProfileResult.GetResultCode() == csp::systems::EResultCode::InProgress)
            {
                return;
            }

            if (GetProfileResult.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                const ConversationResult InternalResult(GetProfileResult.GetResultCode(), GetProfileResult.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            csp::systems::AssetCollectionResultCallback GetUpdatedConversationCallback
                = [Callback, GetConversationResult, ConversationId, this](const csp::systems::AssetCollectionResult& GetUpdatedConversationResult)

            {
                if (GetUpdatedConversationResult.GetResultCode() == csp::systems::EResultCode::InProgress)
                {
                    return;
                }

                if (GetUpdatedConversationResult.GetResultCode() == csp::systems::EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
                        "The Update of Conversation asset collections was not successful. ResCode: %d, HttpResCode: %d",
                        (int)GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());

                    const ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());
                    INVOKE_IF_NOT_NULL(Callback, InternalResult);

                    return;
                }

                const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [Callback, GetUpdatedConversationResult](ErrorCode Error)
                {
                    if (Error != ErrorCode::None)
                    {
                        CSP_LOG_ERROR_MSG("AddMessageToConversation: SignalR connection: Error");

                        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());

                        return;
                    }

                    ConversationResult Result(GetUpdatedConversationResult.GetResultCode(), GetUpdatedConversationResult.GetHttpResultCode());
                    Result.FillConversationInfo(GetUpdatedConversationResult.GetAssetCollection());
                    INVOKE_IF_NOT_NULL(Callback, Result);
                };

                EventBusPtr->SendNetworkEvent("ConversationSystem",
                    { ReplicatedValue((int64_t)ConversationMessageType::ConversationInformation), ConversationId }, signalRCallback);
            };

            ConversationInfo NewConversationData(ConversationData);
            NewConversationData.UserDisplayName = GetProfileResult.GetProfile().DisplayName;

            if (!NewConversationData.Edited)
            {
                const auto CurrentConversationData
                    = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetConversationResult.GetAssetCollection());
                NewConversationData.Edited = CurrentConversationData.Message != NewConversationData.Message;
            }

            AssetSystem->UpdateAssetCollectionMetadata(GetConversationResult.GetAssetCollection(),
                ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(NewConversationData), nullptr, GetUpdatedConversationCallback);
        };

        auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
        UserSystem->GetProfileByUserId(UserSystem->GetLoginState().UserId, GetProfileCallback);
    };

    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSystem::GetConversationInformation(const csp::common::String& ConversationId, ConversationResultCallback Callback)
{
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    csp::systems::AssetCollectionResultCallback GetConversationCallback = [=](const csp::systems::AssetCollectionResult& GetConversationResult)
    {
        if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());

        if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        InternalResult.FillConversationInfo(GetConversationResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

CSP_EVENT void ConversationSystem::SetConversationSystemCallback(ConversationSystemCallbackHandler Callback)
{
    ConversationSystemCallback = Callback;
    RegisterSystemCallback();
}

void ConversationSystem::RegisterSystemCallback()
{
    if (!ConversationSystemCallback)
    {
        return;
    }

    EventBusPtr->ListenNetworkEvent("ConversationSystem", this);
}

void ConversationSystem::DeregisterSystemCallback()
{
    if (EventBusPtr)
    {
        EventBusPtr->StopListenNetworkEvent("ConversationSystem");
    }
}

void ConversationSystem::OnEvent(const std::vector<signalr::value>& EventValues)
{
    if (!ConversationSystemCallback)
    {
        return;
    }

    ConversationEventDeserialiser Deserialiser;
    Deserialiser.Parse(EventValues);
    ConversationSystemCallback(Deserialiser.GetEventParams());
}

} // namespace csp::multiplayer
