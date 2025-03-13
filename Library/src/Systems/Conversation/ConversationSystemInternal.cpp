#include "ConversationSystemInternal.h"
#include "Systems/Conversation/ConversationSystemHelpers.h"

#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Users/UserSystem.h"

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "Multiplayer/EventSerialisation.h"

#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Systems/ResultHelpers.h"

namespace csp::systems
{

namespace
{
    void SendConversationEvent(multiplayer::ConversationEventType EventType, const multiplayer::MessageInfo& EventInfo,
        multiplayer::EventBus* EventBus, multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler Callback)
    {
        auto EventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray({ EventType, EventInfo });
        EventBus->SendNetworkEvent("Conversation", EventParams, Callback);
    }

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
}

ConversationSystemInternal::ConversationSystemInternal(
    systems::AssetSystem* AssetSystem, systems::SpaceSystem* SpaceSystem, systems::UserSystem* UserSystem, multiplayer::EventBus* EventBus)
    : SystemBase(EventBus)
    , AssetSystem { AssetSystem }
    , SpaceSystem { SpaceSystem }
    , UserSystem { UserSystem }
    , EventBus { EventBus }
{
    RegisterSystemCallback();
}

ConversationSystemInternal::~ConversationSystemInternal() { DeregisterSystemCallback(); }

void ConversationSystemInternal::CreateConversation(const common::String& Message, StringResultCallback Callback)
{
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
        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
            = [this, Callback, AddCommentContainerResult](multiplayer::ErrorCode Error)
        {
            if (Error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("Create Conversation: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<StringResult>());
                return;
            }

            StringResult InternalResult(AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());
            InternalResult.SetValue(AddCommentContainerResult.GetAssetCollection().Id);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        };

        multiplayer::MessageInfo MessageInfo
            = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(AddCommentContainerResult.GetAssetCollection());

        SendConversationEvent(multiplayer::ConversationEventType::NewConversation, MessageInfo, EventBus, SignalRCallback);
    };

    const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(SpaceId, UserId);
    multiplayer::MessageInfo DefaultConversationInfo("", true, Message);

    AssetSystem->CreateAssetCollection(SpaceId, nullptr, UniqueAssetCollectionName,
        ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(DefaultConversationInfo), EAssetCollectionType::COMMENT_CONTAINER,
        nullptr, AddCommentContainerCallback);
}

void ConversationSystemInternal::DeleteConversation(const common::String& ConversationId, NullResultCallback Callback)
{
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

        this->AssetSystem->DeleteAssetCollection(ConversationAssetCollection, DeleteAssetCollectionCallback);
    };

    multiplayer::MessageInfo MessageInfo(ConversationId, true, "");
    SendConversationEvent(multiplayer::ConversationEventType::DeleteConversation, MessageInfo, EventBus, SignalRCallback);
}

void ConversationSystemInternal::AddMessage(
    const common::String& ConversationId, const common::String& Message, multiplayer::MessageResultCallback Callback)
{
    // 1. Store the conversation message
    const common::String& UserId = UserSystem->GetLoginState().UserId;

    const multiplayer::MessageResultCallback MessageResultCallback
        = [this, Callback, ConversationId, UserId, Message](const multiplayer::MessageResult& MessageResultCallbackResult)
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

        const multiplayer::MessageInfo& MessageInfo = MessageResultCallbackResult.GetMessageInfo();
        SendConversationEvent(multiplayer::ConversationEventType::NewMessage, MessageInfo, EventBus, SignalRCallback);
    };

    multiplayer::MessageInfo MessageInfo(ConversationId, false, Message);

    const Space CurrentSpace = SpaceSystem->GetCurrentSpace();

    StoreConversationMessage(MessageInfo, CurrentSpace, MessageResultCallback);
}

void ConversationSystemInternal::DeleteMessage(const common::String& ConversationId, const common::String& MessageId, NullResultCallback Callback)
{
    // 1. Send multiplayer event
    const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
        = [this, Callback, ConversationId, MessageId](multiplayer::ErrorCode Error)
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

        const NullResultCallback DeleteAssetCollectionCallback
            = [this, Callback, ConversationId, MessageId](const NullResult& DeleteAssetCollectionResult)
        {
            if (HandleConversationResult(DeleteAssetCollectionResult, "Failed to delete Message asset collection.", Callback) == false)
            {
                return;
            }

            Callback(DeleteAssetCollectionResult);
        };

        this->AssetSystem->DeleteAssetCollection(MessageAssetCollection, DeleteAssetCollectionCallback);
    };

    multiplayer::MessageInfo MessageInfo(ConversationId, false, "", MessageId);
    SendConversationEvent(multiplayer::ConversationEventType::DeleteMessage, MessageInfo, EventBus, SignalRCallback);
}

void ConversationSystemInternal::GetMessagesFromConversation(const common::String& ConversationId, const common::Optional<int>& ResultsSkipNumber,
    const common::Optional<int>& ResultsMaxNumber, multiplayer::MessageCollectionResultCallback Callback)
{
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

    AssetSystem->FindAssetCollections(
        nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, ResultsSkipNumber, ResultsMaxNumber, GetMessagesCallback);
}

void ConversationSystemInternal::GetConversationInfo(const common::String& ConversationId, multiplayer::ConversationResultCallback Callback)
{
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

    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSystemInternal::UpdateConversation(
    const common::String& ConversationId, const multiplayer::MessageUpdateParams& NewData, multiplayer::ConversationResultCallback Callback)
{
    // 1. Get asset collection
    AssetCollectionResultCallback GetConversationCallback
        = [this, Callback, ConversationId, NewData](const AssetCollectionResult& GetConversationResult)
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

            auto UpdatedInfo = systems::ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(
                GetUpdatedConversationResult.GetAssetCollection());

            SendConversationEvent(multiplayer::ConversationEventType::ConversationInformation, UpdatedInfo, EventBus, SignalRCallback);
        };

        multiplayer::MessageInfo NewConversationData
            = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(GetConversationResult.GetAssetCollection());

        NewConversationData.Message = NewData.NewMessage;

        this->AssetSystem->UpdateAssetCollectionMetadata(GetConversationResult.GetAssetCollection(),
            ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(NewConversationData), nullptr, GetUpdatedConversationCallback);
    };

    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSystemInternal::GetMessageInfo(
    const common::String& ConversationId, const common::String& MessageId, multiplayer::MessageResultCallback Callback)
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

    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystemInternal::UpdateMessage(const common::String& ConversationId, const common::String& MessageId,
    const multiplayer::MessageUpdateParams& NewData, multiplayer::MessageResultCallback Callback)
{
    // 1. Get message asset collection
    AssetCollectionResultCallback GetMessageCallback = [this, Callback, MessageId, NewData](const AssetCollectionResult& GetMessageResult)
    {
        if (HandleConversationResult(GetMessageResult, "The retrieval of Conversation asset collections was not successful.", Callback) == false)
        {
            return;
        }

        // 2. Update asset collections metadata
        AssetCollectionResultCallback GetUpdatedMessageCallback
            = [this, Callback, MessageId, NewData](const AssetCollectionResult& GetUpdatedMessageResult)

        {
            if (HandleConversationResult(GetUpdatedMessageResult, "The Update of Message asset collections was not successful.", Callback) == false)
            {
                return;
            }

            multiplayer::MessageResult Result(GetUpdatedMessageResult.GetResultCode(), GetUpdatedMessageResult.GetHttpResultCode());
            Result.FillMessageInfo(GetUpdatedMessageResult.GetAssetCollection());

            // 3. Send multiplayer event
            const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
                = [Callback, GetUpdatedMessageResult, NewData, Result](multiplayer::ErrorCode Error)
            {
                if (Error != multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("SetMessageInfo: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<multiplayer::MessageResult>());
                    return;
                }

                INVOKE_IF_NOT_NULL(Callback, Result);
            };

            SendConversationEvent(multiplayer::ConversationEventType::MessageInformation, Result.GetMessageInfo(), EventBus, SignalRCallback);
        };

        multiplayer::MessageInfo NewMessageData
            = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection());

        NewMessageData.Message = NewData.NewMessage;

        this->AssetSystem->UpdateAssetCollectionMetadata(GetMessageResult.GetAssetCollection(),
            ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(NewMessageData), nullptr, GetUpdatedMessageCallback);
    };

    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystemInternal::StoreConversationMessage(
    const multiplayer::MessageInfo& Info, const Space& Space, multiplayer::MessageResultCallback Callback) const
{
    const AssetCollectionResultCallback AddCommentCallback = [=](const AssetCollectionResult& AddCommentResult)
    {
        if (HandleConversationResult(AddCommentResult, "The Comment asset collection creation was not successful.", Callback) == false)
        {
            return;
        }

        multiplayer::MessageResult Result;
        Result.FillMessageInfo(AddCommentResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(Callback, Result);
    };

    const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(Space.Id, Info.UserId);
    const auto MessageMetadata = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(Info);

    AssetSystem->CreateAssetCollection(
        Space.Id, Info.ConversationId, UniqueAssetCollectionName, MessageMetadata, EAssetCollectionType::COMMENT, nullptr, AddCommentCallback);
}

void ConversationSystemInternal::DeleteMessages(
    const common::String& ConversationId, common::Array<AssetCollection>& Messages, NullResultCallback Callback)
{
    if (Messages.Size() == 0)
    {
        NullResult InternalResult(EResultCode::Success, (uint16_t)web::EResponseCodes::ResponseNoContent);
        INVOKE_IF_NOT_NULL(Callback, InternalResult);

        return;
    }

    AssetSystem->DeleteMultipleAssetCollections(Messages, Callback);
}

void ConversationSystemInternal::GetNumberOfReplies(const common::String& ConversationId, csp::multiplayer::NumberOfRepliesResultCallback Callback)
{
    auto GetMessageCountCallback = [Callback](const csp::systems::AssetCollectionCountResult& GetMessageResult)
    {
        csp::multiplayer::NumberOfRepliesResult Result(GetMessageResult);
        Result.Count = GetMessageResult.GetCount();
        Callback(Result);
    };

    static const csp::common::Array<csp::systems::EAssetCollectionType> PrototypeTypes = { csp::systems::EAssetCollectionType::COMMENT };

    AssetSystem->GetAssetCollectionCount(nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, GetMessageCountCallback);
}

void ConversationSystemInternal::RegisterComponent(csp::multiplayer::ConversationSpaceComponent* Component)
{
    Components.insert(Component);
    FlushEvents();
}
void ConversationSystemInternal::DeregisterComponent(csp::multiplayer::ConversationSpaceComponent* Component) { Components.erase(Component); }

void ConversationSystemInternal::RegisterSystemCallback()
{
    if (!EventBusPtr)
    {
        CSP_LOG_ERROR_MSG("Error: Failed to register ConversationSystemInternal. EventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    EventBusPtr->ListenNetworkEvent("Conversation", this);
}

void ConversationSystemInternal::DeregisterSystemCallback()
{
    if (EventBusPtr)
    {
        EventBusPtr->StopListenNetworkEvent("Conversation");
    }
}

void ConversationSystemInternal::OnEvent(const std::vector<signalr::value>& EventValues)
{
    csp::multiplayer::ConversationEventDeserialiser Deserialiser;
    Deserialiser.Parse(EventValues);

    const multiplayer::ConversationEventParams& Params = Deserialiser.GetEventParams();

    if (TrySendEvent(Params) == false)
    {
        // If component doesn't exist, add it to the queue for processing later
        Events.push_back(Params);
    }
}

void ConversationSystemInternal::FlushEvents()
{
    for (auto It = std::begin(Events); It != std::end(Events);)
    {
        if (TrySendEvent(*It))
        {
            // Event has now been processed, so remove
            Events.erase(It);
        }
        else
        {
            ++It;
        }
    }
}

bool ConversationSystemInternal::TrySendEvent(const csp::multiplayer::ConversationEventParams& Params)
{
    for (const auto& Component : Components)
    {
        if (Component->GetConversationId() == Params.MessageInfo.ConversationId)
        {
            if (Component->ConversationUpdateCallback != nullptr)
            {
                Component->ConversationUpdateCallback(Params);
                return true;
            }
        }
    }

    return false;
}

}
