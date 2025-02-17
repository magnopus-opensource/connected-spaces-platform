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

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Conversation/ConversationSystemHelpers.h"
#include "Multiplayer/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h"
#include "Systems/ResultHelpers.h"

#include <msgpack/v1/object_fwd_decl.hpp>

using namespace csp::systems;

namespace csp::multiplayer
{

namespace
{
    // Temp utility function until we adapt the new continuation pattern
    template <class InResultType, class OutResultType>
    bool HandleConversationResult(const InResultType& Result, const char* ErrorMessage, std::function<void(const OutResultType&)> FailCallback)
    {
        if (Result.GetResultCode() == EResultCode::InProgress)
        {
            return false;
        }

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_ERROR_FORMAT(
                (std::string(ErrorMessage) + "ResCode: %d, HttpResCode: %d").c_str(), (int)Result.GetResultCode(), Result.GetHttpResultCode());

            const OutResultType InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(FailCallback, InternalResult);
            return false;
        }

        return true;
    }

    bool EnsureValidConversationId(const csp::common::String& ConversationId)
    {
        if (ConversationId.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("This component does not have an associated conversation."
                              "Call CreateConversation to crete a new conversation for this component");
            return false;
        }

        return true;
    }
}

csp::multiplayer::ConversationSpaceComponent::ConversationSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Conversation, Parent)
{
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsActive)] = true;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Title)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Resolved)] = false;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition)] = csp::common::Vector3 { 0, 0, 0 };

    SetScriptInterface(CSP_NEW ConversationSpaceComponentScriptInterface(this));
}

void ConversationSpaceComponent::CreateConversation(const csp::common::String& Message, StringResultCallback Callback)
{
    if (!GetConversationId().IsEmpty())
    {
        CSP_LOG_WARN_MSG("This component already has an associated conversation! No new conversation was created as a result.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<StringResult>());

        return;
    }

    const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    if (UserSystem->GetLoginState().State != csp::systems::ELoginState::LoggedIn)
    {
        CSP_LOG_ERROR_MSG("ConversationSpaceComponent::CreateConversation() failed: Log in before adding a message");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::StringResult>());

        return;
    };

    auto* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();

    const Space& CurrentSpace = SpaceSystem->GetCurrentSpace();
    const common::String& UserId = UserSystem->GetLoginState().UserId;
    const common::String& SpaceId = CurrentSpace.Id;

    // 1. Create the comment container asset collection
    const AssetCollectionResultCallback AddCommentContainerCallback
        = [this, Callback, UserId, Message](const AssetCollectionResult& AddCommentContainerResult)
    {
        if (HandleConversationResult(AddCommentContainerResult, "The Comment Container asset collection creation was not successful.", Callback)
            == false)
        {
            return;
        }

        // 2.Send multiplayer event
        common::String ConversationId = AddCommentContainerResult.GetAssetCollection().Id;

        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
            = [this, Callback, AddCommentContainerResult, ConversationId](multiplayer::ErrorCode Error)
        {
            if (Error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("AddMessage: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<StringResult>());
                return;
            }

            StringResult InternalResult(AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());
            InternalResult.SetValue(ConversationId);
            SetConversationId(ConversationId);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        };

        multiplayer::MessageInfo MessageInfo(ConversationId, true, "", "", UserId, Message, "");

        auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
        EventBus->SendNetworkEvent(
            "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::NewMessage), ConversationId }, SignalRCallback);
    };

    const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(SpaceId, UserId);
    multiplayer::MessageInfo DefaultConversationInfo("", true, "", "", UserId, Message, "");

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    AssetSystem->CreateAssetCollection(SpaceId, nullptr, UniqueAssetCollectionName,
        ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(DefaultConversationInfo), EAssetCollectionType::COMMENT_CONTAINER,
        nullptr, AddCommentContainerCallback);
}

void ConversationSpaceComponent::DeleteConversation(csp::systems::NullResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<StringResult>());
        return;
    }

    // 1.Send multiplayer event
    const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
        = [this, Callback, ConversationId](multiplayer::ErrorCode Error)
    {
        if (Error != multiplayer::ErrorCode::None)
        {
            CSP_LOG_ERROR_MSG("DeleteConversation: SignalR connection: Error");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
            return;
        }

        // 2. Delete the asset colleciton associated with this conversation
        AssetCollection ConversationAssetCollection;
        ConversationAssetCollection.Id = ConversationId;

        NullResultCallback DeleteAssetCollectionCallback = [Callback, this](const NullResult& DeleteAssetCollectionResult)
        {
            if (HandleConversationResult(
                    DeleteAssetCollectionResult, "The deletion of the conversation asset collection was not successful.", Callback)
                == false)
            {
                return;
            }

            Callback(DeleteAssetCollectionResult);
        };

        auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
        AssetSystem->DeleteAssetCollection(ConversationAssetCollection, DeleteAssetCollectionCallback);
    };

    auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
    EventBus->SendNetworkEvent(
        "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::DeleteConversation), ConversationId }, SignalRCallback);
}

void ConversationSpaceComponent::AddMessage(const csp::common::String& Message, MessageResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());
        return;
    }

    // 1. Store the conversation message
    const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
    const common::String& UserId = UserSystem->GetLoginState().UserId;

    const multiplayer::MessageResultCallback MessageResultCallback
        = [this, Callback, ConversationId, UserId](const multiplayer::MessageResult& MessageResultCallbackResult)
    {
        if (HandleConversationResult(MessageResultCallbackResult, "Failed to store conversation message.", Callback) == false)
        {
            return;
        }

        // 2. Send multiplayer event
        const auto SignalRCallback = [this, Callback, MessageResultCallbackResult](multiplayer::ErrorCode Error)
        {
            if (Error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("AddMessage: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<multiplayer::MessageResult>());
                return;
            }

            INVOKE_IF_NOT_NULL(Callback, MessageResultCallbackResult);
        };

        auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
        EventBus->SendNetworkEvent(
            "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::NewMessage), ConversationId }, SignalRCallback);
    };

    auto* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();
    const Space CurrentSpace = SpaceSystem->GetCurrentSpace();
    StoreConversationMessage(CurrentSpace, UserId, Message, MessageResultCallback);
}

void ConversationSpaceComponent::DeleteMessage(const csp::common::String& MessageId, NullResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());
        return;
    }

    // 1. Send multiplayer event
    const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [this, Callback, MessageId](multiplayer::ErrorCode Error)
    {
        if (Error != multiplayer::ErrorCode::None)
        {
            CSP_LOG_ERROR_MSG("DeleteMessage: SignalR connection: Error");

            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
            return;
        }

        // 2. Delete the message asset collection
        AssetCollection MessageAssetCollection;
        MessageAssetCollection.Id = MessageId;

        const NullResultCallback DeleteAssetCollectionCallback = [this, Callback, MessageId](const NullResult& DeleteAssetCollectionResult)
        {
            if (HandleConversationResult(DeleteAssetCollectionResult, "Failed to delete Message asset collection.", Callback) == false)
            {
                return;
            }

            Callback(DeleteAssetCollectionResult);
        };

        auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
        AssetSystem->DeleteAssetCollection(MessageAssetCollection, DeleteAssetCollectionCallback);
    };

    auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
    EventBus->SendNetworkEvent(
        "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::DeleteMessage), ConversationId }, SignalRCallback);
}

bool ConversationSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)); }

void ConversationSpaceComponent::SetIsVisible(const bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible), Value); }

bool ConversationSpaceComponent::GetIsActive() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive)); }

/* IPositionComponent */

const csp::common::Vector3& ConversationSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::Position));
}

void ConversationSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Position), Value);
}

/* IRotationComponent */

const csp::common::Vector4& ConversationSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ConversationPropertyKeys::Rotation));
}

void ConversationSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Rotation), Value);
}

void ConversationSpaceComponent::GetMessagesFromConversation(
    const csp::common::Optional<int>& ResultsSkipNumber, const csp::common::Optional<int>& ResultsMaxNumber, MessageCollectionResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageCollectionResult>());
        return;
    }

    // 1. Find asset collections
    AssetCollectionsResultCallback GetMessagesCallback = [Callback](const AssetCollectionsResult& GetMessagesResult)
    {
        if (HandleConversationResult(GetMessagesResult, "The retrieval of Message asset collections was not successful.", Callback) == false)
        {
            return;
        }

        // 2. Give result to caller
        multiplayer::MessageCollectionResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());
        InternalResult.SetTotalCount(GetMessagesResult.GetTotalCount());
        InternalResult.FillMessageInfoCollection(GetMessagesResult.GetAssetCollections());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    static const common::Array<EAssetCollectionType> PrototypeTypes = { EAssetCollectionType::COMMENT };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->FindAssetCollections(
        nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, ResultsSkipNumber, ResultsMaxNumber, GetMessagesCallback);
}

void ConversationSpaceComponent::GetConversationInfo(ConversationResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());
        return;
    }

    // 1. Get asset collection
    AssetCollectionResultCallback GetConversationCallback = [ConversationId, Callback](const AssetCollectionResult& GetConversationResult)
    {
        if (HandleConversationResult(GetConversationResult, "The retrieval of Message asset collections was not successful.", Callback) == false)
        {
            return;
        }

        // 2. Give result to callers
        multiplayer::ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());
        InternalResult.FillConversationInfo(GetConversationResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSpaceComponent::SetConversationInfo(const MessageInfo& ConversationData, ConversationResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());
        return;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    // 1. Get asset collection
    AssetCollectionResultCallback GetConversationCallback
        = [this, Callback, ConversationId, ConversationData, AssetSystem](const AssetCollectionResult& GetConversationResult)
    {
        if (HandleConversationResult(GetConversationResult, "The retrieval of Conversation asset collections was not successful.", Callback) == false)
        {
            return;
        }

        // 2. Update the conversations asset collection
        AssetCollectionResultCallback GetUpdatedConversationCallback = [this, Callback](const AssetCollectionResult& GetUpdatedConversationResult)
        {
            if (HandleConversationResult(GetUpdatedConversationResult, "The Update of Conversation asset collections was not successful.", Callback)
                == false)
            {
                return;
            }

            // 3. Send multiplayer event
            const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
                = [Callback, GetUpdatedConversationResult](multiplayer::ErrorCode Error)
            {
                if (Error != multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("SetConversationInfo: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<multiplayer::ConversationResult>());
                    return;
                }

                multiplayer::ConversationResult Result(
                    GetUpdatedConversationResult.GetResultCode(), GetUpdatedConversationResult.GetHttpResultCode());
                Result.FillConversationInfo(GetUpdatedConversationResult.GetAssetCollection());

                INVOKE_IF_NOT_NULL(Callback, Result);
            };

            const auto ConversationId = GetConversationId();

            auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
            EventBus->SendNetworkEvent("ConversationSystem",
                { ReplicatedValue((int64_t)ConversationMessageType::ConversationInformation), ConversationId }, SignalRCallback);
        };

        multiplayer::MessageInfo NewConversationData(ConversationData);

        // Create a timestamp if non was specified
        if (NewConversationData.EditedTimestamp.IsEmpty())
        {
            const auto CurrentConversationData
                = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetConversationResult.GetAssetCollection());
            NewConversationData.EditedTimestamp = CurrentConversationData.EditedTimestamp;
        }

        AssetSystem->UpdateAssetCollectionMetadata(GetConversationResult.GetAssetCollection(),
            ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(NewConversationData), nullptr, GetUpdatedConversationCallback);
    };

    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSpaceComponent::GetMessageInfo(const csp::common::String& MessageId, MessageResultCallback Callback)
{
    const AssetCollectionResultCallback GetMessageCallback = [Callback](const AssetCollectionResult& GetMessageResult)
    {
        if (HandleConversationResult(GetMessageResult, "The retrieval of the Message asset collection was not successful.", Callback) == false)
        {
            return;
        }

        multiplayer::MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
        InternalResult.FillMessageInfo(GetMessageResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSpaceComponent::SetMessageInfo(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback)
{
    const auto ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId) == false)
    {
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());
        return;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    // 1. Get message assey collection
    AssetCollectionResultCallback GetMessageCallback
        = [this, Callback, MessageId, MessageData, AssetSystem](const AssetCollectionResult& GetMessageResult)
    {
        if (HandleConversationResult(GetMessageResult, "The retrieval of Conversation asset collections was not successful.", Callback) == false)
        {
            return;
        }

        // 2. Update asset collections metadata
        AssetCollectionResultCallback GetUpdatedMessageCallback
            = [this, Callback, MessageId, GetMessageResult, MessageData](const AssetCollectionResult& GetUpdatedMessageResult)

        {
            if (HandleConversationResult(GetMessageResult, "The Update of Message asset collections was not successful.", Callback) == false)
            {
                return;
            }

            // 3. Send multiplayer event
            const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
                = [Callback, GetUpdatedMessageResult, MessageData](multiplayer::ErrorCode Error)
            {
                if (Error != multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("SetMessageInfo: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<multiplayer::MessageResult>());
                    return;
                }

                multiplayer::MessageResult Result(GetUpdatedMessageResult.GetResultCode(), GetUpdatedMessageResult.GetHttpResultCode());
                Result.FillMessageInfo(GetUpdatedMessageResult.GetAssetCollection());
                INVOKE_IF_NOT_NULL(Callback, Result);
            };

            const auto ConversationId = GetConversationId();

            auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
            EventBus->SendNetworkEvent(
                "ConversationSystem", { ReplicatedValue((int64_t)ConversationMessageType::MessageInformation), ConversationId }, SignalRCallback);
        };

        multiplayer::MessageInfo NewMessageData(MessageData);

        if (MessageData.EditedTimestamp.IsEmpty())
        {
            multiplayer::MessageInfo MsgInfo
                = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection());
            NewMessageData.EditedTimestamp = MsgInfo.EditedTimestamp;
        }

        AssetSystem->UpdateAssetCollectionMetadata(GetMessageResult.GetAssetCollection(),
            ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(NewMessageData), nullptr, GetUpdatedMessageCallback);
    };

    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSpaceComponent::SetIsActive(const bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive), Value); }

void ConversationSpaceComponent::SetTitle(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title), Value);
}

const csp::common::String& ConversationSpaceComponent::GetTitle() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title));
}

void ConversationSpaceComponent::SetResolved(bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved), Value); }

bool ConversationSpaceComponent::GetResolved() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved)); }

void ConversationSpaceComponent::SetConversationCameraPosition(const csp::common::Vector3& InValue)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition), InValue);
}

const csp::common::Vector3& ConversationSpaceComponent::GetConversationCameraPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition));
}

const int64_t ConversationSpaceComponent::GetNumberOfReplies() const
{
    // TODO: Implement getNumberOfReplies - OF-1385
    return 0;
}

void ConversationSpaceComponent::SetConversationId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId), Value);
}

void ConversationSpaceComponent::RemoveConversationId() { RemoveProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)); }

const csp::common::String& ConversationSpaceComponent::GetConversationId() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId));
}

void ConversationSpaceComponent::StoreConversationMessage(
    const csp::systems::Space& Space, const csp::common::String& UserId, const csp::common::String& Message, MessageResultCallback Callback) const
{
    const auto ConversationId = GetConversationId();

    const csp::systems::AssetCollectionResultCallback AddCommentCallback = [=](const csp::systems::AssetCollectionResult& AddCommentResult)
    {
        if (AddCommentResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (AddCommentResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "The Comment asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(AddCommentResult.GetResultCode()), AddCommentResult.GetHttpResultCode());

            const MessageResult InternalResult(AddCommentResult.GetResultCode(), AddCommentResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        MessageResult Result;
        Result.FillMessageInfo(AddCommentResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, Result);
    };

    const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(Space.Id, UserId);
    MessageInfo DefaultMessageInfo = MessageInfo();
    DefaultMessageInfo.EditedTimestamp = "";
    DefaultMessageInfo.Message = Message;
    DefaultMessageInfo.UserId = UserId;
    DefaultMessageInfo.IsConversation = false;
    const auto MessageMetadata = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(DefaultMessageInfo);

    AssetSystem->CreateAssetCollection(Space.Id, ConversationId, UniqueAssetCollectionName, MessageMetadata,
        csp::systems::EAssetCollectionType::COMMENT, nullptr, AddCommentCallback);
}

void ConversationSpaceComponent::DeleteMessages(
    csp::common::Array<csp::systems::AssetCollection>& Messages, csp::systems::NullResultCallback Callback)
{
    const auto MessagesCount = Messages.Size();

    if (MessagesCount == 0)
    {
        csp::systems::NullResult InternalResult(csp::systems::EResultCode::Success, (uint16_t)csp::web::EResponseCodes::ResponseNoContent);
        INVOKE_IF_NOT_NULL(Callback, InternalResult);

        return;
    }

    const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    AssetSystem->DeleteMultipleAssetCollections(Messages, Callback);
}

void ConversationSpaceComponent::OnLocalDelete()
{
    const auto Callback = [](const NullResult& Result) {};
    DeleteConversation(Callback);
}

} // namespace csp::multiplayer
