#include "ConversationSystemInternal.h"
#include "Systems/Conversation/ConversationSystemHelpers.h"

#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "Multiplayer/EventSerialisation.h"

#include "Common/CallHelpers.h"
#include "Debug/Logging.h"
#include "Systems/ResultHelpers.h"

// This is temporary whilst we do the modularisation effort. This file needs rended in two to split the multiplayer/signalR logic from the REST logic.
// Only one of these files should be included.
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Systems/ContinuationUtils.h"

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

    std::function<AssetCollectionResult(const AssetCollectionResult& Result)> ValidateMessageAssetCollection(common::String ConversationId)
    {
        return [ConversationId](const AssetCollectionResult& Result)
        {
            if (Result.GetAssetCollection().ParentId != ConversationId)
            {
                CSP_LOG_ERROR_MSG("Given message doesn't exist on the conversation.");
                throw std::exception();
            }

            return Result;
        };
    }

    std::function<void(const AssetCollectionResult& Result)> SetMessageAssetCollection(std::shared_ptr<systems::AssetCollection> OutAssetCollection)
    {
        return [OutAssetCollection](const AssetCollectionResult& Result) { *OutAssetCollection = Result.GetAssetCollection(); };
    }

    std::function<void(const AssetResult& Result)> SetAnnotationAsset(std::shared_ptr<Asset> OutAsset)
    {
        return [OutAsset](const systems::AssetResult& Result) { *OutAsset = Result.GetAsset(); };
    }

    std::function<void(const AssetsResult& Result)> SetAnnotationAssetFromAssets(std::shared_ptr<Asset> OutAsset)
    {
        return [OutAsset](const systems::AssetsResult& Result)
        {
            if (Result.GetAssets().Size() == 1)
            {
                *OutAsset = Result.GetAssets()[0];
            }
            else
            {
                CSP_LOG_ERROR_MSG("Result didn't contain a valid asset.");
                throw std::exception();
            }
        };
    }

    std::function<async::task<UriResult>()> UploadAnnotationAssetData(AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Collection,
        std::shared_ptr<systems::Asset> Asset, const systems::BufferAssetDataSource& Data, const csp::common::String& FileName)
    {
        return [AssetSystem, Collection, Asset, Data, FileName]()
        {
            Asset->FileName = FileName;
            return AssetSystem->UploadAssetDataEx(*Collection, *Asset, Data, csp::common::CancellationToken::Dummy());
        };
    }

    std::function<multiplayer::AnnotationResult()> CreateAnnotationResult(std::shared_ptr<systems::AssetCollection> AnnotationAssetCollection,
        std::shared_ptr<systems::Asset> AnnotationAsset, std::shared_ptr<systems::Asset> AnnotationThumbnailAsset)
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

    std::function<async::task<AssetsResult>()> GetAnnotationAsset(AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Collection)
    {
        return [AssetSystem, Collection]()
        { return AssetSystem->GetAssetsByCriteria({ Collection->Id }, nullptr, nullptr, csp::common::Array { EAssetType::ANNOTATION }); };
    }

    std::function<async::task<AssetsResult>()> GetAnnotationThumbnailAsset(AssetSystem* AssetSystem, std::shared_ptr<AssetCollection> Collection)
    {
        return [AssetSystem, Collection]()
        { return AssetSystem->GetAssetsByCriteria({ Collection->Id }, nullptr, nullptr, csp::common::Array { EAssetType::ANNOTATION_THUMBNAIL }); };
    }

    std::function<async::task<NullResult>(const AssetsResult&)> DeleteAnnotationAsset(
        AssetSystem* AssetSystem, std::shared_ptr<AssetCollection> Collection)
    {
        return [AssetSystem, Collection](const AssetsResult& Result)
        {
            if (Result.GetAssets().Size() == 1)
            {
                return AssetSystem->DeleteAsset(*Collection, Result.GetAssets()[0]);
            }
            else if (Result.GetAssets().Size() == 0)
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log, "Annotation asset doesn't exist");
            }
            else
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log, "Invalid number of annotation asset collections exist for this message");
            }

            throw std::exception();
        };
    }

    std::function<void(const UriResult&)> SetAssetUri(std::shared_ptr<Asset> Asset)
    {
        return [Asset](const UriResult& Result) { Asset->Uri = Result.GetUri(); };
    }

    std::function<async::task<AssetCollectionResult>(const csp::common::Map<csp::common::String, csp::common::String>& Metadata)>
    AppendCommentMetadata(AssetSystem* AssetSystem, std::shared_ptr<AssetCollection>& MessageCollection)
    {
        return [AssetSystem, MessageCollection](const csp::common::Map<csp::common::String, csp::common::String>& Metadata)
        {
            auto NewMetadata = MessageCollection->GetMetadataImmutable();

            std::unique_ptr<common::Array<common::String>> Keys(const_cast<common::Array<common::String>*>(Metadata.Keys()));

            for (const auto& Key : *Keys)
            {
                NewMetadata[Key] = Metadata[Key];
            }

            return AssetSystem->UpdateAssetCollectionMetadata(*MessageCollection, NewMetadata, nullptr);
        };
    }

    std::function<async::task<AssetCollectionResult>(const AssetCollectionResult&)> RemoveAnnotationMetadata(AssetSystem* AssetSystem)
    {
        return [AssetSystem](const AssetCollectionResult& Result)
        {
            auto Metadata = ConversationSystemHelpers::RemoveAnnotationMetadata(Result.GetAssetCollection());
            return AssetSystem->UpdateAssetCollectionMetadata(Result.GetAssetCollection(), Metadata, nullptr);
        };
    }

    std::function<async::task<AssetResult>()> GetOrCreateAnnotationAsset(
        AssetSystem* AssetSystem, std::shared_ptr<systems::AssetCollection> Collection, const csp::common::String& Name, EAssetType Type)
    {
        return [AssetSystem, Collection, Name, Type]()
        {
            return AssetSystem->GetAssetsByCriteria({ Collection->Id }, nullptr, nullptr, csp::common::Array { Type })
                .then(
                    [AssetSystem, Collection, Name, Type](const AssetsResult& Result)
                    {
                        if (Result.GetAssets().Size() == 0)
                        {
                            return AssetSystem->CreateAsset(*Collection, Name, nullptr, nullptr, Type);
                        }
                        else if (Result.GetAssets().Size() == 1)
                        {
                            CSP_LOG_MSG(
                                csp::common::LogLevel::Log, "ConversationSystemInternal::SetAnnotation, asset already exists, so not creating");

                            async::event_task<AssetResult> OnCompleteEvent;
                            async::task<AssetResult> OnCompleteTask = OnCompleteEvent.get_task();

                            AssetResult NewResult(Result.GetResultCode(), Result.GetHttpResultCode());
                            NewResult.SetAsset(Result.GetAssets()[0]);

                            OnCompleteEvent.set(NewResult);
                            return OnCompleteTask;
                        }
                        else
                        {
                            CSP_LOG_MSG(csp::common::LogLevel::Log, "Invalid number of annotation assets exist for this message");

                            throw std::exception();
                        }
                    });
        };
    }

    std::function<async::task<std::optional<csp::multiplayer::ErrorCode>>()> SendConversationEvent(
        multiplayer::ConversationEventType EventType, std::shared_ptr<AssetCollection> ConersationCollection, multiplayer::EventBus* EventBus)
    {
        return [EventType, ConersationCollection, EventBus]()
        {
            const multiplayer::MessageInfo& EventInfo
                = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(*ConersationCollection);
            auto EventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray({ EventType, EventInfo });
            return EventBus->SendNetworkEvent("Conversation", EventParams);
        };
    }

    std::function<async::task<std::optional<csp::multiplayer::ErrorCode>>()> SendConversationMessageEvent(
        multiplayer::ConversationEventType EventType, std::shared_ptr<AssetCollection> MessageCollection, multiplayer::EventBus* EventBus)
    {
        return [EventType, MessageCollection, EventBus]()
        {
            const multiplayer::MessageInfo& EventInfo = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(*MessageCollection);
            auto EventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray({ EventType, EventInfo });
            return EventBus->SendNetworkEvent("Conversation", EventParams);
        };
    }

    std::function<async::task<AssetCollectionsResult>()> FindMessageAssetCollections(
        AssetSystem* AssetSystem, const common::String& ConversationId, const common::String& SpaceId)
    {
        return [AssetSystem, ConversationId, SpaceId]()
        {
            return AssetSystem->FindAssetCollections(nullptr, ConversationId, nullptr, common::Array { EAssetCollectionType::COMMENT }, nullptr,
                common::Array { SpaceId }, nullptr, nullptr);
        };
    }

    std::function<AssetCollectionResult(const AssetCollectionResult&)> ValidateAnnotationMetadata()
    {
        return [](const AssetCollectionResult& Result)
        {
            bool HasAnnotationData = ConversationSystemHelpers::HasAnnotationMetadata(Result.GetAssetCollection());

            if (HasAnnotationData == false)
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log, "Message asset collection doesn't contain annotation data.");
                throw std::exception();
            }

            return Result;
        };
    }

    std::function<std::unordered_map<std::string, std::string>(const AssetCollectionsResult&)> GetAnnotationAssetIdsFromCollections()
    {
        return [](const AssetCollectionsResult& Result)
        {
            auto ThumbnailIds = ConversationSystemHelpers::GetAnnotationThumbnailAssetIdsFromCollectionResult(Result);

            if (ThumbnailIds.size() == 0)
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log, "No Thumbnails exist.");
                throw std::exception();
            }

            return ThumbnailIds;
        };
    }

    std::function<async::task<AssetsResult>(const std::unordered_map<std::string, std::string>&)> GetThumbnailAssetsFromMap(AssetSystem* AssetSystem)
    {
        return [AssetSystem](const std::unordered_map<std::string, std::string>& Result)
        {
            csp::common::Array<csp::common::String> MessageIds(Result.size());
            csp::common::Array<csp::common::String> AssetIds(Result.size());
            int i = 0;

            for (const auto& Pair : Result)
            {
                MessageIds[i] = Pair.first.c_str();
                AssetIds[i] = Pair.second.c_str();
                i++;
            }

            return AssetSystem->GetAssetsByCriteria(MessageIds, AssetIds, nullptr, common::Array<EAssetType> { EAssetType::ANNOTATION_THUMBNAIL });
        };
    }

    std::function<multiplayer::AnnotationThumbnailCollectionResult(const AssetsResult&)> CreateAnnotationThumbnailCollectionResult()
    {
        return [](const AssetsResult& Result)
        {
            multiplayer::AnnotationThumbnailCollectionResult ThumbnailResult(
                EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
            ThumbnailResult.ParseAssets(Result);
            return ThumbnailResult;
        };
    }

    std::function<common::Map<common::String, common::String>()> GenerateAnnotationMetadata(
        const multiplayer::AnnotationUpdateParams& NewData, std::shared_ptr<Asset> AnnotationAsset, std::shared_ptr<Asset> AnnotationThumbnailAsset)
    {
        return [NewData, AnnotationAsset, AnnotationThumbnailAsset]()
        { return ConversationSystemHelpers::GenerateAnnotationAssetCollectionMetadata(NewData, AnnotationAsset->Id, AnnotationThumbnailAsset->Id); };
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
            = [Callback, AddCommentContainerResult](multiplayer::ErrorCode Error)
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
            NullResultCallback DeleteAssetCollectionCallback = [Callback](const NullResult& DeleteAssetCollectionResult)
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
        const auto SignalRCallback = [Callback, MessageResultCallbackResult](multiplayer::ErrorCode Error)
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

            const NullResultCallback DeleteAssetCollectionCallback = [Callback](const NullResult& DeleteAssetCollectionResult)
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
    const common::String& /*ConversationId*/, const common::String& MessageId, multiplayer::MessageResultCallback Callback)
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

void ConversationSystemInternal::UpdateMessage(const common::String& /*ConversationId*/, const common::String& MessageId,
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
    const common::String& /*ConversationId*/, common::Array<AssetCollection>& Messages, NullResultCallback Callback)
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

void ConversationSystemInternal::GetConversationAnnotation(const csp::common::String& ConversationId, multiplayer::AnnotationResultCallback Callback)
{
    auto ConversationAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAsset = std::make_shared<Asset>();
    auto AnnotationThumbnailAsset = std::make_shared<Asset>();

    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;

    // 1. Get conversation asset collection
    AssetSystem->GetAssetCollectionById(ConversationId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved message asset collection",
            "Failed to get message asset collection.", {}, {}, {}))
        .then(ValidateAnnotationMetadata())
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {},
            {}, {}))
        .then(SetMessageAssetCollection(ConversationAssetCollection))
        // 3. Get annotation asset
        .then(GetAnnotationAsset(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {},
            {}, {}))
        .then(SetAnnotationAssetFromAssets(AnnotationAsset))
        // 4. Get annotation thumbnail asset
        .then(GetAnnotationThumbnailAsset(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAssetFromAssets(AnnotationThumbnailAsset))
        // 5. Process result
        .then(CreateAnnotationResult(ConversationAssetCollection, AnnotationAsset, AnnotationThumbnailAsset))
        .then(systems::continuations::SendResult(Callback, "Successfully retrieved annotation."))
        .then(common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/)
            { Callback(MakeInvalid<multiplayer::AnnotationResult>()); },
            csp::systems::SystemsManager::Get().GetLogSystem()));
}

void ConversationSystemInternal::SetConversationAnnotation(const csp::common::String& ConversationId,
    const multiplayer::AnnotationUpdateParams& AnnotationParams, const systems::BufferAssetDataSource& Annotation,
    const systems::BufferAssetDataSource& AnnotationThumbnail, multiplayer::AnnotationResultCallback Callback)
{
    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;
    const csp::common::String UserId = UserSystem->GetLoginState().UserId;

    const common::String UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueAnnotationAssetCollectionName(SpaceId, UserId);
    const csp::common::String UniqueAnnotationAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(SpaceId, UserId);
    const csp::common::String UniqueAnnotationThumbnailAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(SpaceId, UserId);

    const csp::common::String UniqueAnnotationAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(SpaceId, UserId)
        + SpaceSystemHelpers::GetAssetFileExtension(Annotation.GetMimeType());
    const csp::common::String UniqueAnnotationThumbnailAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(SpaceId, UserId)
        + SpaceSystemHelpers::GetAssetFileExtension(AnnotationThumbnail.GetMimeType());

    auto ConversationAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAsset = std::make_shared<Asset>();
    auto AnnotationThumbnailAsset = std::make_shared<Asset>();

    // 1. Get conversation asset collection
    AssetSystem->GetAssetCollectionById(ConversationId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully retrieved message asset collection",
            "Failed to get message asset collection.", {}, {}, {}))
        .then(SetMessageAssetCollection(ConversationAssetCollection))
        // 2. Create Annotation asset
        .then(GetOrCreateAnnotationAsset(AssetSystem, ConversationAssetCollection, UniqueAnnotationAssetName, EAssetType::ANNOTATION))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully created annotation asset", "Failed to create annotation asset.", {},
            {}, {}))
        .then(SetAnnotationAsset(AnnotationAsset))
        // 3. Upload Annotation asset data
        .then(UploadAnnotationAssetData(AssetSystem, ConversationAssetCollection, AnnotationAsset, Annotation, UniqueAnnotationAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully uploaded annotation asset data",
            "Failed to upload annotation asset data.", {}, {}, {}))
        .then(SetAssetUri(AnnotationAsset))
        // 4. Create Annotation thumbnail asset
        .then(GetOrCreateAnnotationAsset(AssetSystem, ConversationAssetCollection, UniqueAnnotationAssetName, EAssetType::ANNOTATION_THUMBNAIL))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully created annotation thumbnail asset",
            "Failed to create annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAsset(AnnotationThumbnailAsset))
        // 5. Upload Annotation thumbnail asset data
        .then(UploadAnnotationAssetData(
            AssetSystem, ConversationAssetCollection, AnnotationThumbnailAsset, AnnotationThumbnail, UniqueAnnotationThumbnailAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully uploaded annotation thumbnail asset data",
            "Failed to upload annotation thumbnail asset data.", {}, {}, {}))
        .then(SetAssetUri(AnnotationThumbnailAsset))
        // 6. Update asset collection metadata
        .then(GenerateAnnotationMetadata(AnnotationParams, AnnotationAsset, AnnotationThumbnailAsset))
        .then(AppendCommentMetadata(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully updated message asset collection metadata",
            "Failed to update message asset collection metadata.", {}, {}, {}))
        .then(SetMessageAssetCollection(ConversationAssetCollection))
        // 7. Send multiplayer event
        .then(SendConversationEvent(multiplayer::ConversationEventType::SetConversationAnnotation, ConversationAssetCollection, EventBus))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(Callback,
            "ConversationSystemInternal::SetConversationAnnotation, successfully sent multiplayer event",
            MakeInvalid<multiplayer::AnnotationResult>(), csp::systems::SystemsManager::Get().GetLogSystem()))
        // 8. Process result
        .then(CreateAnnotationResult(ConversationAssetCollection, AnnotationAsset, AnnotationThumbnailAsset))
        .then(systems::continuations::SendResult(Callback, "Successfully set annotation."))
        .then(common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/)
            { Callback(MakeInvalid<multiplayer::AnnotationResult>()); },
            csp::systems::SystemsManager::Get().GetLogSystem()));
}

void ConversationSystemInternal::DeleteConversationAnnotation(const csp::common::String& ConversationId, systems::NullResultCallback Callback)
{
    auto ConversationAssetCollection = std::make_shared<AssetCollection>();

    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;

    // 1. Get conversation asset collection
    AssetSystem->GetAssetCollectionById(ConversationId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully retrieved asset collection", "Failed to get asset collection.", {}, {}, {}))
        .then(ValidateAnnotationMetadata())
        // 2. Remove annotation metadata
        .then(RemoveAnnotationMetadata(AssetSystem))
        .then(SetMessageAssetCollection(ConversationAssetCollection))
        // 3. Send multiplayer event
        .then(SendConversationEvent(multiplayer::ConversationEventType::DeleteConversationAnnotation, ConversationAssetCollection, EventBus))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully sent multiplayer event", MakeInvalid<systems::NullResult>(),
            csp::systems::SystemsManager::Get().GetLogSystem()))
        // 4. Delete annoation asset
        .then(GetAnnotationAsset(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation asset", "Failed to deleted annotation asset.", {}, {}, {}))
        // 5. Delete annoation thumbnail asset
        .then(GetAnnotationThumbnailAsset(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(AssetSystem, ConversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation thumbnail asset",
            "Failed to deleted annotation thumbnail asset.", {}, {}, {}))
        // 6. Process result
        .then(systems::continuations::ReportSuccess(Callback, "Successfully deleted annotation."))
        .then(
            common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/) { Callback(MakeInvalid<NullResult>()); },
                csp::systems::SystemsManager::Get().GetLogSystem()));
}

void ConversationSystemInternal::GetAnnotation(
    const csp::common::String& ConversationId, const csp::common::String& MessageId, multiplayer::AnnotationResultCallback Callback)
{
    auto MessageAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAsset = std::make_shared<Asset>();
    auto AnnotationThumbnailAsset = std::make_shared<Asset>();

    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;

    // 1. Get message asset collection
    AssetSystem->GetAssetCollectionById(MessageId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved message asset collection", "Failed to get message asset collection.",
            {}, {}, {}))
        .then(ValidateMessageAssetCollection(ConversationId))
        .then(ValidateAnnotationMetadata())
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetMessageAssetCollection(MessageAssetCollection))
        // 3. Get annotation asset
        .then(GetAnnotationAsset(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetAnnotationAssetFromAssets(AnnotationAsset))
        // 4. Get annotation thumbnail asset
        .then(GetAnnotationThumbnailAsset(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAssetFromAssets(AnnotationThumbnailAsset))
        // 5. Process result
        .then(CreateAnnotationResult(MessageAssetCollection, AnnotationAsset, AnnotationThumbnailAsset))
        .then(systems::continuations::SendResult(Callback, "Successfully retrieved annotation."))
        .then(common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/)
            { Callback(MakeInvalid<multiplayer::AnnotationResult>()); },
            csp::systems::SystemsManager::Get().GetLogSystem()));
}

void ConversationSystemInternal::SetAnnotation(const csp::common::String& ConversationId, const csp::common::String& MessageId,
    const multiplayer::AnnotationUpdateParams& AnnotationParams, const systems::BufferAssetDataSource& Annotation,
    const systems::BufferAssetDataSource& AnnotationThumbnail, multiplayer::AnnotationResultCallback Callback)
{
    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;
    const csp::common::String UserId = UserSystem->GetLoginState().UserId;

    const common::String UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueAnnotationAssetCollectionName(SpaceId, UserId);
    const csp::common::String UniqueAnnotationAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(SpaceId, UserId);
    const csp::common::String UniqueAnnotationThumbnailAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(SpaceId, UserId);

    const csp::common::String UniqueAnnotationAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(SpaceId, UserId)
        + SpaceSystemHelpers::GetAssetFileExtension(Annotation.GetMimeType());
    const csp::common::String UniqueAnnotationThumbnailAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(SpaceId, UserId)
        + SpaceSystemHelpers::GetAssetFileExtension(AnnotationThumbnail.GetMimeType());

    auto MessageAssetCollection = std::make_shared<AssetCollection>();
    auto AnnotationAsset = std::make_shared<Asset>();
    auto AnnotationThumbnailAsset = std::make_shared<Asset>();

    // 1. Get message asset collection
    AssetSystem->GetAssetCollectionById(MessageId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully retrieved message asset collection", "Failed to get message asset collection.",
            {}, {}, {}))
        .then(ValidateMessageAssetCollection(ConversationId))
        .then(SetMessageAssetCollection(MessageAssetCollection))
        // 2. Create Annotation asset
        .then(GetOrCreateAnnotationAsset(AssetSystem, MessageAssetCollection, UniqueAnnotationAssetName, EAssetType::ANNOTATION))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully created annotation asset", "Failed to create annotation asset.", {}, {}, {}))
        .then(SetAnnotationAsset(AnnotationAsset))
        // 3. Upload Annotation asset data
        .then(UploadAnnotationAssetData(AssetSystem, MessageAssetCollection, AnnotationAsset, Annotation, UniqueAnnotationAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully uploaded annotation asset data", "Failed to upload annotation asset data.", {},
            {}, {}))
        .then(SetAssetUri(AnnotationAsset))
        // 4. Create Annotation thumbnail asset
        .then(GetOrCreateAnnotationAsset(AssetSystem, MessageAssetCollection, UniqueAnnotationAssetName, EAssetType::ANNOTATION_THUMBNAIL))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully created annotation thumbnail asset",
            "Failed to create annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAsset(AnnotationThumbnailAsset))
        // 5. Upload Annotation thumbnail asset data
        .then(UploadAnnotationAssetData(
            AssetSystem, MessageAssetCollection, AnnotationThumbnailAsset, AnnotationThumbnail, UniqueAnnotationThumbnailAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully uploaded annotation thumbnail asset data",
            "Failed to upload annotation thumbnail asset data.", {}, {}, {}))
        .then(SetAssetUri(AnnotationThumbnailAsset))
        // 6. Update asset collection metadata
        .then(GenerateAnnotationMetadata(AnnotationParams, AnnotationAsset, AnnotationThumbnailAsset))
        .then(AppendCommentMetadata(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully updated message asset collection metadata",
            "Failed to update message asset collection metadata.", {}, {}, {}))
        .then(SetMessageAssetCollection(MessageAssetCollection))
        // 7. Send multiplayer event
        .then(SendConversationMessageEvent(multiplayer::ConversationEventType::SetAnnotation, MessageAssetCollection, EventBus))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(Callback,
            "ConversationSystemInternal::SetAnnotation, successfully sent multiplayer event", MakeInvalid<multiplayer::AnnotationResult>(),
            csp::systems::SystemsManager::Get().GetLogSystem()))
        // 8. Process result
        .then(CreateAnnotationResult(MessageAssetCollection, AnnotationAsset, AnnotationThumbnailAsset))
        .then(systems::continuations::SendResult(Callback, "Successfully set annotation."))
        .then(common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/)
            { Callback(MakeInvalid<multiplayer::AnnotationResult>()); },
            csp::systems::SystemsManager::Get().GetLogSystem()));
}

void ConversationSystemInternal::DeleteAnnotation(
    const csp::common::String& ConversationId, const csp::common::String& MessageId, systems::NullResultCallback Callback)
{
    auto MessageAssetCollection = std::make_shared<AssetCollection>();

    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;

    // 1. Get message asset collection
    AssetSystem->GetAssetCollectionById(MessageId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully retrieved asset collection", "Failed to get asset collection.", {}, {}, {}))
        .then(ValidateMessageAssetCollection(ConversationId))
        .then(ValidateAnnotationMetadata())
        // 2. Remove annotation metadata
        .then(RemoveAnnotationMetadata(AssetSystem))
        .then(SetMessageAssetCollection(MessageAssetCollection))
        // 3. Send multiplayer event
        .then(SendConversationMessageEvent(multiplayer::ConversationEventType::DeleteAnnotation, MessageAssetCollection, EventBus))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(Callback,
            "ConversationSystemInternal::DeleteAnnotation, successfully sent multiplayer event", MakeInvalid<systems::NullResult>(),
            csp::systems::SystemsManager::Get().GetLogSystem()))
        // 4. Delete annoation asset
        .then(GetAnnotationAsset(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation asset", "Failed to deleted annotation asset.", {}, {}, {}))
        // 5. Delete annoation thumbnail asset
        .then(GetAnnotationThumbnailAsset(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(AssetSystem, MessageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(Callback,
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation thumbnail asset",
            "Failed to deleted annotation thumbnail asset.", {}, {}, {}))
        // 6. Process result
        .then(systems::continuations::ReportSuccess(Callback, "Successfully deleted annotation."))
        .then(
            common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/) { Callback(MakeInvalid<NullResult>()); },
                csp::systems::SystemsManager::Get().GetLogSystem()));
}

void ConversationSystemInternal::GetAnnotationThumbnailsForConversation(
    const csp::common::String& ConversationId, multiplayer::AnnotationThumbnailCollectionResultCallback Callback)
{
    const csp::common::String SpaceId = SpaceSystem->GetCurrentSpace().Id;

    // 1. Get all message asset collections
    FindMessageAssetCollections(AssetSystem, ConversationId, SpaceId)()
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionsResult>(Callback,
            "ConversationSystemInternal::GetAnnotationThumbnailsForConversation, successfully retrieved message asset collections",
            "Failed to get message asset collections.", {}, {}, {}))
        // 2. Get all annotation thumbnail assets
        .then(GetAnnotationAssetIdsFromCollections())
        .then(GetThumbnailAssetsFromMap(AssetSystem))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(Callback,
            "ConversationSystemInternal::GetAnnotationThumbnailsForConversation, successfully retrieved thumbnail assets",
            "Failed to get thumbnail assets.", {}, {}, {}))
        // 3. Process result
        .then(CreateAnnotationThumbnailCollectionResult())
        .then(systems::continuations::SendResult(Callback, "Successfully retrieved annotation thumbnails."))
        .then(common::continuations::InvokeIfExceptionInChain([Callback](const std::exception& /*exception*/)
            { Callback(MakeInvalid<multiplayer::AnnotationThumbnailCollectionResult>()); },
            csp::systems::SystemsManager::Get().GetLogSystem()));
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
