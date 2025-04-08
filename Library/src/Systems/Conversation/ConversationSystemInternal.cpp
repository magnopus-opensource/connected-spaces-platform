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

#include "Common/Continuations.h"

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

    bool EnsureUserHasPermission(const common::String& UserId, const common::String& ConversationUserId, bool IsConversation)
    {
        if (UserId != ConversationUserId)
        {
            if (IsConversation)
            {
                CSP_LOG_ERROR_MSG("User does not have permission to modify this conversation.");
            }
            else
            {
                CSP_LOG_ERROR_MSG("User does not have permission to modify this message.");
            }

            return false;
        }

        return true;
    }

    auto CreateAnnotationAssetCollection(AssetSystem* AssetSystem, const csp::common::String& SpaceId, const csp::common::String& Name,
        const csp::common::Map<csp::common::String, csp::common::String>& Metadata)
    {
        return [AssetSystem, SpaceId, Name, Metadata](const AssetCollectionResult& Result) {
            return AssetSystem->CreateAssetCollection(
                SpaceId, Result.GetAssetCollection().Id, Name, Metadata, EAssetCollectionType::ANNOTATION, nullptr);
        };
    }

    auto SetAnnotationAssetCollection(std::shared_ptr<systems::AssetCollection> OutAssetCollection)
    {
        return [OutAssetCollection](const AssetCollectionResult& Result) mutable
        {
            *OutAssetCollection = Result.GetAssetCollection();
            return Result;
        };
    }

    auto SetAnnotationAssetCollectionFromCollections(std::shared_ptr<systems::AssetCollection> OutAssetCollection)
    {
        return [OutAssetCollection](const AssetCollectionsResult& Result) mutable
        {
            if (Result.GetAssetCollections().Size() == 1)
            {
                *OutAssetCollection = Result.GetAssetCollections()[0];
            }

            // TODO: error if non found
            return *OutAssetCollection;
        };
    }

    auto CreateAnnotationAsset(AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Parent, const csp::common::String& Name)
    {
        return [AssetSystem, Parent, Name]() { return AssetSystem->CreateAsset(*Parent, Name, nullptr, nullptr, EAssetType::ANNOTATION); };
    }

    auto CreateAnnotationThumbnailAsset(AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Parent, const csp::common::String& Name)
    {
        return [AssetSystem, Parent, Name]() { return AssetSystem->CreateAsset(*Parent, Name, nullptr, nullptr, EAssetType::ANNOTATION_THUMBNAIL); };
    }

    auto UploadAnnotationAssetData(AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Collection,
        const systems::BufferAssetDataSource& Data, const csp::common::String& FileName)
    {
        return [AssetSystem, Data, Collection, FileName](const AssetResult& Result)
        {
            Asset UploadAsset = Result.GetAsset();
            UploadAsset.FileName = FileName;
            return AssetSystem->UploadAssetDataEx(*Collection, UploadAsset, Data, csp::common::CancellationToken::Dummy());
        };
    }

    auto CreateAnnotationResult(std::shared_ptr<systems::AssetCollection> AnnotationAssetCollection, std::shared_ptr<systems::Asset> AnnotationAsset,
        std::shared_ptr<systems::Asset> AnnotationThumbnailAsset)
    {
        return [AnnotationAssetCollection, AnnotationAsset, AnnotationThumbnailAsset]()
        {
            multiplayer::AnnotationResult Result(EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
            Result.ParseAnnotationAssetData(*AnnotationAssetCollection);
            Result.SetAnnotationAsset(*AnnotationAsset);
            Result.SetAnnotationThumbnailAsset(*AnnotationThumbnailAsset);
            return Result;
        };
    }

    auto GetAnnotationAsset(AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Collection)
    {
        return [AssetSystem, Collection]()
        { return AssetSystem->GetAssetsByCriteria({ Collection->Id }, nullptr, nullptr, csp::common::Array { EAssetType::ANNOTATION }); };
    }

    auto GetAnnotationThumbnailAsset(AssetSystem* AssetSystem, std::shared_ptr<AssetCollection> Collection)
    {
        return [AssetSystem, Collection]()
        { return AssetSystem->GetAssetsByCriteria({ Collection->Id }, nullptr, nullptr, csp::common::Array { EAssetType::ANNOTATION_THUMBNAIL }); };
    }

    auto SetAnnotationAsset(std::shared_ptr<Asset> OutAsset)
    {
        return [OutAsset](const systems::AssetsResult& Result) mutable
        {
            if (Result.GetAssets().Size() == 1)
            {
                auto ResultAsset = Result.GetAssets()[0];
                *OutAsset = Result.GetAssets()[0];
            }

            return Result;
        };
    }

    auto DeleteAnnotationAssetCollection(AssetSystem* AssetSystem)
    {
        return [AssetSystem](const AssetCollection& Collection) { return AssetSystem->DeleteAssetCollection(Collection); };
    }

    auto SetAssetUri(std::shared_ptr<Asset> Asset)
    {
        return [Asset](const UriResult& Result)
        {
            Asset->Uri = Result.GetUri();
            return Result;
        };
    }

    auto FindAnnotationAssetCollection(AssetSystem* AssetSystem, const common::String& ParentId, const common::String& SpaceId)
    {
        return [AssetSystem, ParentId, SpaceId]()
        {
            return AssetSystem->FindAssetCollections(
                nullptr, ParentId, nullptr, common::Array { EAssetCollectionType::ANNOTATION }, nullptr, common::Array { SpaceId }, nullptr, nullptr);
        };
    }

    auto SendConversationEvent(
        multiplayer::ConversationEventType EventType, std::shared_ptr<AssetCollection> MessageCollection, multiplayer::EventBus* EventBus)
    {
        return [EventType, MessageCollection, EventBus]()
        {
            const multiplayer::MessageInfo& EventInfo = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(*MessageCollection);
            auto EventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray({ EventType, EventInfo });
            return EventBus->SendNetworkEvent("Conversation", EventParams);
        };
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
    // 1. Get asset collection
    AssetCollectionResultCallback GetConversationCallback = [this, ConversationId, Callback](const AssetCollectionResult& GetConversationResult)
    {
        if (HandleConversationResult(GetConversationResult, "The retrieval of Message asset collections was not successful.", Callback) == false)
        {
            return;
        }

        AssetCollection ConversationAssetCollection = GetConversationResult.GetAssetCollection();

        // 2.Send multiplayer event
        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
            = [this, Callback, ConversationId, ConversationAssetCollection](multiplayer::ErrorCode Error)
        {
            if (Error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("DeleteConversation: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
                return;
            }

            // 3. Delete the asset colleciton associated with this conversation
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

        multiplayer::MessageInfo MessageInfo
            = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(ConversationAssetCollection);
        SendConversationEvent(multiplayer::ConversationEventType::DeleteConversation, MessageInfo, EventBus, SignalRCallback);
    };

    AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
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
    // 1. Get asset collection
    AssetCollectionResultCallback GetMessageCallback = [this, ConversationId, MessageId, Callback](const AssetCollectionResult& GetMessageResult)
    {
        if (HandleConversationResult(GetMessageResult, "The retrieval of Message asset collections was not successful.", Callback) == false)
        {
            return;
        }

        // Ensure client has correct permissions to delete the conversation
        multiplayer::MessageInfo Info
            = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection());

        if (EnsureUserHasPermission(UserSystem->GetLoginState().UserId, Info.UserId, false) == false)
        {
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
            return;
        }

        // 2. Send multiplayer event
        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback
            = [this, Callback, ConversationId, MessageId](multiplayer::ErrorCode Error)
        {
            if (Error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("DeleteMessage: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
                return;
            }

            // 3. Delete the message asset collection
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

        SendConversationEvent(multiplayer::ConversationEventType::DeleteMessage, Info, EventBus, SignalRCallback);
    };

    AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
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

        // Ensure client has correct permissions to delete the conversation
        multiplayer::MessageInfo Info
            = systems::ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(GetConversationResult.GetAssetCollection());

        if (EnsureUserHasPermission(UserSystem->GetLoginState().UserId, Info.UserId, true) == false)
        {
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<multiplayer::ConversationResult>());
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

        // Ensure client has correct permissions to delete the conversation
        multiplayer::MessageInfo Info
            = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection());

        if (EnsureUserHasPermission(UserSystem->GetLoginState().UserId, Info.UserId, false) == false)
        {
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<multiplayer::MessageResult>());
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

void ConversationSystemInternal::GetAnnotation(
    const csp::common::String& ConversationId, const csp::common::String& MessageId, multiplayer::AnnotationResultCallback Callback)
{
    auto AnnotationAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAsset = std::make_shared<Asset>();
    auto AnnotationThumbnailAsset = std::make_shared<Asset>();

    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;
    // TODO: ensure messages parent is conversation
    // 1. Find annotation asset collection
    FindAnnotationAssetCollection(AssetSystem, MessageId, SpaceId)()
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetAnnotationAssetCollectionFromCollections(AnnotationAssetCollection))
        // 2. Get annotation asset
        .then(GetAnnotationAsset(AssetSystem, AnnotationAssetCollection))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetAnnotationAsset(AnnotationAsset))
        // 3. Get annotation thumbnail asset
        .then(GetAnnotationThumbnailAsset(AssetSystem, AnnotationAssetCollection))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAsset(AnnotationThumbnailAsset))
        // 4. Process result
        .then(CreateAnnotationResult(AnnotationAssetCollection, AnnotationAsset, AnnotationThumbnailAsset))
        .then(common::continuations::SendResult(Callback, "Successfully retrieved annotation."));
}

void ConversationSystemInternal::SetAnnotation(const csp::common::String& ConversationId, const csp::common::String& MessageId,
    const multiplayer::AnnotationData& AnnotationData, const systems::BufferAssetDataSource& Annotation,
    const systems::BufferAssetDataSource& AnnotationThumbnail, multiplayer::AnnotationResultCallback Callback)
{
    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;
    const csp::common::String UserId = UserSystem->GetLoginState().UserId;

    const common::String UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueAnnotationAssetCollectionName(SpaceId, UserId);
    const auto AnnotationMetadata = ConversationSystemHelpers::GenerateAnnotationAssetCollectionMetadata(AnnotationData);
    const csp::common::String UniqueAnnotationAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(SpaceId, UserId);
    const csp::common::String UniqueAnnotationThumbnailAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(SpaceId, UserId);

    const csp::common::String UniqueAnnotationAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(SpaceId, UserId);
    const csp::common::String UniqueAnnotationThumbnailAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(SpaceId, UserId);

    auto MessageAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAsset = std::make_shared<Asset>();
    auto AnnotationThumbnailAsset = std::make_shared<Asset>();

    // TODO: ensure messages parent is conversation
    // 1. Get message asset collection
    AssetSystem->GetAssetCollectionById(MessageId)
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully retrieved asset collection", "Failed to get asset collection.", {}, {}, {}))
        .then(SetAnnotationAssetCollection(MessageAssetCollection))
        // 2. Create annotation asset collection
        .then(CreateAnnotationAssetCollection(this->AssetSystem, SpaceId, UniqueAssetCollectionName, AnnotationMetadata))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully created annotation asset collection", "Failed to create asset collection.", {},
            {}, {}))
        .then(SetAnnotationAssetCollection(AnnotationAssetCollection))
        // 3. Create Annotation asset
        .then(CreateAnnotationAsset(AssetSystem, AnnotationAssetCollection, UniqueAnnotationAssetName))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully created annotation asset", "Failed to create annotation asset.", {}, {}, {}))
        // 4. Upload Annotation asset data
        .then(UploadAnnotationAssetData(AssetSystem, AnnotationAssetCollection, Annotation, UniqueAnnotationAssetFileName))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully uploaded annotation asset data", "Failed to upload annotation asset data.", {},
            {}, {}))
        .then(SetAssetUri(AnnotationAsset))
        // 6. Create Annotation thumbnail asset
        .then(CreateAnnotationThumbnailAsset(AssetSystem, AnnotationAssetCollection, UniqueAnnotationThumbnailAssetName))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully created annotation thumbnail asset",
            "Failed to create annotation thumbnail asset.", {}, {}, {}))
        // 7. Upload Annotation thumbnail asset data
        .then(UploadAnnotationAssetData(AssetSystem, AnnotationAssetCollection, AnnotationThumbnail, UniqueAnnotationThumbnailAssetFileName))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully uploaded annotation thumbnail asset data",
            "Failed to upload annotation thumbnail asset data.", {}, {}, {}))
        .then(SetAssetUri(AnnotationThumbnailAsset))
        // 8. Send multiplayer event
        .then(SendConversationEvent(multiplayer::ConversationEventType::SetAnnotation, MessageAssetCollection, EventBus))
        .then(csp::common::continuations::AssertRequestSuccessOrErrorFromErrorCode(
            Callback, "ConversationSystemInternal::SetAnnotation, successfully sent multiplayer event", std::nullopt, std::nullopt, std::nullopt))
        // 9. Process result
        .then(CreateAnnotationResult(AnnotationAssetCollection, AnnotationAsset, AnnotationThumbnailAsset))
        .then(common::continuations::SendResult(Callback, "Successfully set annotation."));
}

void ConversationSystemInternal::DeleteAnnotation(
    const csp::common::String& ConversationId, const csp::common::String& MessageId, systems::NullResultCallback Callback)
{
    auto MessageAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAssetCollection = std::make_shared<AssetCollection>();

    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;

    // 1. Get message asset collection
    AssetSystem->GetAssetCollectionById(MessageId)
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully retrieved asset collection", "Failed to get asset collection.", {}, {}, {}))
        .then(SetAnnotationAssetCollection(MessageAssetCollection))
        // 2. Send multiplayer event
        .then(SendConversationEvent(multiplayer::ConversationEventType::DeleteAnnotation, MessageAssetCollection, EventBus))
        .then(csp::common::continuations::AssertRequestSuccessOrErrorFromErrorCode(
            Callback, "ConversationSystemInternal::DeleteAnnotation, successfully sent multiplayer event", std::nullopt, std::nullopt, std::nullopt))
        // 3. Find annotation asset collection
        .then(FindAnnotationAssetCollection(AssetSystem, MessageId, SpaceId))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionsResult>(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetAnnotationAssetCollectionFromCollections(AnnotationAssetCollection))
        // 4. Delete annoation asset collection
        .then(DeleteAnnotationAssetCollection(this->AssetSystem))
        .then(common::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully deleted annotation asset collection.",
            "Failed to delete annotation asset collection.", {}, {}, {}))
        // 5. Process result
        .then(common::continuations::ReportSuccess(Callback, "Successfully deleted annotation."));
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
