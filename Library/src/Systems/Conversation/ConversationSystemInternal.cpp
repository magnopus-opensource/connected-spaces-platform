#include "ConversationSystemInternal.h"
#include "Systems/Conversation/ConversationSystemHelpers.h"

#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "Multiplayer/NetworkEventSerialisation.h"

#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Systems/ResultHelpers.h"

// This is temporary whilst we do the modularisation effort. This file needs rended in two to split the multiplayer/signalR logic from the REST logic.
// Only one of these files should be included.
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Systems/ContinuationUtils.h"

#include <fmt/format.h>

namespace csp::systems
{

namespace
{
    void SendConversationEvent(multiplayer::ConversationEventType eventType, const multiplayer::MessageInfo& eventInfo,
        multiplayer::NetworkEventBus* networkEventBus, multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler callback)
    {
        auto eventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray(eventType, eventInfo);
        networkEventBus->SendNetworkEvent("Conversation", eventParams, callback);
    }

    // Temp utility function until we adapt the new continuation pattern
    template <class InResultType, class OutResultType>
    bool HandleConversationResult(const InResultType& result, const char* errorMessage, std::function<void(const OutResultType&)> failCallback)
    {
        if (result.GetResultCode() == EResultCode::InProgress)
        {
            return false;
        }

        if (result.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_ERROR_FORMAT(
                (std::string(errorMessage) + "ResCode: %d, HttpResCode: %d").c_str(), (int)result.GetResultCode(), result.GetHttpResultCode());

            const OutResultType internalResult(result.GetResultCode(), result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(failCallback, internalResult);
            return false;
        }

        return true;
    }

    bool EnsureUserHasPermission(const common::String& userId, const common::String& conversationUserId, bool isConversation)
    {
        if (userId != conversationUserId)
        {
            if (isConversation)
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

    std::function<AssetCollectionResult(const AssetCollectionResult& result)> ValidateMessageAssetCollection(common::String conversationId)
    {
        return [conversationId](const AssetCollectionResult& result)
        {
            if (result.GetAssetCollection().ParentId != conversationId)
            {
                throw csp::common::continuations::ResultException(
                    "Given message doesn't exist on the conversation.", MakeInvalid<AssetCollectionResult>());
            }

            return result;
        };
    }

    std::function<void(const AssetCollectionResult& result)> SetMessageAssetCollection(std::shared_ptr<systems::AssetCollection> outAssetCollection)
    {
        return [outAssetCollection](const AssetCollectionResult& result) { *outAssetCollection = result.GetAssetCollection(); };
    }

    std::function<void(const AssetResult& result)> SetAnnotationAsset(std::shared_ptr<Asset> outAsset)
    {
        return [outAsset](const systems::AssetResult& result) { *outAsset = result.GetAsset(); };
    }

    std::function<void(const AssetsResult& result)> SetAnnotationAssetFromAssets(std::shared_ptr<Asset> outAsset)
    {
        return [outAsset](const systems::AssetsResult& result)
        {
            if (result.GetAssets().Size() == 1)
            {
                *outAsset = result.GetAssets()[0];
            }
            else
            {
                throw csp::common::continuations::ResultException("Result didn't contain a valid asset.", MakeInvalid<AssetsResult>());
            }
        };
    }

    std::function<async::task<UriResult>()> UploadAnnotationAssetData(AssetSystem* assetSystem, std::shared_ptr<systems::AssetCollection> collection,
        std::shared_ptr<systems::Asset> asset, const systems::BufferAssetDataSource& data, const csp::common::String& fileName)
    {
        return [assetSystem, collection, asset, data, fileName]()
        {
            asset->FileName = fileName;
            return assetSystem->UploadAssetDataEx(*collection, *asset, data, csp::common::CancellationToken::Dummy());
        };
    }

    std::function<multiplayer::AnnotationResult()> CreateAnnotationResult(std::shared_ptr<systems::AssetCollection> annotationAssetCollection,
        std::shared_ptr<systems::Asset> annotationAsset, std::shared_ptr<systems::Asset> annotationThumbnailAsset)
    {
        return [annotationAssetCollection, annotationAsset, annotationThumbnailAsset]()
        {
            multiplayer::AnnotationResult result(EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
            result.ParseAnnotationAssetData(*annotationAssetCollection);
            result.SetAnnotationAsset(*annotationAsset);
            result.SetAnnotationThumbnailAsset(*annotationThumbnailAsset);
            return result;
        };
    }

    std::function<async::task<AssetsResult>()> GetAnnotationAsset(AssetSystem* assetSystem, std::shared_ptr<systems::AssetCollection> collection)
    {
        return [assetSystem, collection]()
        { return assetSystem->GetAssetsByCriteria({ collection->Id }, nullptr, nullptr, csp::common::Array { EAssetType::ANNOTATION }); };
    }

    std::function<async::task<AssetsResult>()> GetAnnotationThumbnailAsset(AssetSystem* assetSystem, std::shared_ptr<AssetCollection> collection)
    {
        return [assetSystem, collection]()
        { return assetSystem->GetAssetsByCriteria({ collection->Id }, nullptr, nullptr, csp::common::Array { EAssetType::ANNOTATION_THUMBNAIL }); };
    }

    std::function<async::task<NullResult>(const AssetsResult&)> DeleteAnnotationAsset(
        AssetSystem* assetSystem, std::shared_ptr<AssetCollection> collection)
    {
        return [assetSystem, collection](const AssetsResult& result)
        {
            if (result.GetAssets().Size() == 1)
            {
                return assetSystem->DeleteAsset(*collection, result.GetAssets()[0]);
            }
            else if (result.GetAssets().Size() == 0)
            {
                throw csp::common::continuations::ResultException("Annotation asset doesn't exist", MakeInvalid<NullResult>());
            }
            else
            {
                throw csp::common::continuations::ResultException(
                    "Invalid number of annotation asset collections exist for this message", MakeInvalid<NullResult>());
            }
        };
    }

    std::function<void(const UriResult&)> SetAssetUri(std::shared_ptr<Asset> asset)
    {
        return [asset](const UriResult& result) { asset->Uri = result.GetUri(); };
    }

    std::function<async::task<AssetCollectionResult>(const csp::common::Map<csp::common::String, csp::common::String>& metadata)>
    AppendCommentMetadata(AssetSystem* assetSystem, std::shared_ptr<AssetCollection>& messageCollection)
    {
        return [assetSystem, messageCollection](const csp::common::Map<csp::common::String, csp::common::String>& metadata)
        {
            auto newMetadata = messageCollection->GetMetadataImmutable();

            ConversationSystemHelpers::AppendMetadata(newMetadata, metadata);

            return assetSystem->UpdateAssetCollectionMetadata(*messageCollection, newMetadata, nullptr);
        };
    }

    std::function<async::task<AssetCollectionResult>(const AssetCollectionResult&)> RemoveAnnotationMetadata(AssetSystem* assetSystem)
    {
        return [assetSystem](const AssetCollectionResult& result)
        {
            auto metadata = ConversationSystemHelpers::RemoveAnnotationMetadata(result.GetAssetCollection());
            return assetSystem->UpdateAssetCollectionMetadata(result.GetAssetCollection(), metadata, nullptr);
        };
    }

    std::function<async::task<AssetResult>()> GetOrCreateAnnotationAsset(
        AssetSystem* assetSystem, std::shared_ptr<systems::AssetCollection> collection, const csp::common::String& name, EAssetType type)
    {
        return [assetSystem, collection, name, type]()
        {
            return assetSystem->GetAssetsByCriteria({ collection->Id }, nullptr, nullptr, csp::common::Array { type })
                .then(
                    [assetSystem, collection, name, type](const AssetsResult& result)
                    {
                        if (result.GetAssets().Size() == 0)
                        {
                            return assetSystem->CreateAsset(*collection, name, nullptr, nullptr, type);
                        }
                        else if (result.GetAssets().Size() == 1)
                        {
                            CSP_LOG_MSG(
                                csp::common::LogLevel::Log, "ConversationSystemInternal::SetAnnotation, asset already exists, so not creating");

                            async::event_task<AssetResult> onCompleteEvent;
                            async::task<AssetResult> onCompleteTask = onCompleteEvent.get_task();

                            AssetResult newResult(result.GetResultCode(), result.GetHttpResultCode());
                            newResult.SetAsset(result.GetAssets()[0]);

                            onCompleteEvent.set(newResult);
                            return onCompleteTask;
                        }
                        else
                        {
                            throw csp::common::continuations::ResultException(
                                "Invalid number of annotation assets exist for this message", MakeInvalid<AssetsResult>());
                        }
                    });
        };
    }

    std::function<async::task<std::optional<csp::multiplayer::ErrorCode>>()> SendConversationEvent(multiplayer::ConversationEventType eventType,
        std::shared_ptr<AssetCollection> conersationCollection, multiplayer::NetworkEventBus* networkEventBus)
    {
        return [eventType, conersationCollection, networkEventBus]()
        {
            const multiplayer::MessageInfo& eventInfo
                = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(*conersationCollection);
            auto eventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray(eventType, eventInfo);
            return networkEventBus->SendNetworkEvent("Conversation", eventParams);
        };
    }

    std::function<async::task<std::optional<csp::multiplayer::ErrorCode>>()> SendConversationMessageEvent(
        multiplayer::ConversationEventType eventType, std::shared_ptr<AssetCollection> messageCollection,
        multiplayer::NetworkEventBus* networkEventBus)
    {
        return [eventType, messageCollection, networkEventBus]()
        {
            const multiplayer::MessageInfo& eventInfo = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(*messageCollection);
            auto eventParams = ConversationSystemHelpers::MessageInfoToReplicatedValueArray(eventType, eventInfo);
            return networkEventBus->SendNetworkEvent("Conversation", eventParams);
        };
    }

    std::function<async::task<AssetCollectionsResult>()> FindMessageAssetCollections(
        AssetSystem* assetSystem, const common::String& conversationId, const common::String& spaceId)
    {
        return [assetSystem, conversationId, spaceId]()
        {
            return assetSystem->FindAssetCollections(nullptr, conversationId, nullptr, common::Array { EAssetCollectionType::COMMENT }, nullptr,
                common::Array { spaceId }, nullptr, nullptr);
        };
    }

    std::function<AssetCollectionResult(const AssetCollectionResult&)> ValidateAnnotationMetadata()
    {
        return [](const AssetCollectionResult& result)
        {
            bool hasAnnotationData = ConversationSystemHelpers::HasAnnotationMetadata(result.GetAssetCollection());

            if (hasAnnotationData == false)
            {
                throw csp::common::continuations::ResultException(
                    "Message asset collection doesn't contain annotation data.", MakeInvalid<AssetCollectionResult>());
            }

            return result;
        };
    }

    std::function<std::unordered_map<std::string, std::string>(const AssetCollectionsResult&)> GetAnnotationAssetIdsFromCollections()
    {
        return [](const AssetCollectionsResult& result)
        {
            auto thumbnailIds = ConversationSystemHelpers::GetAnnotationThumbnailAssetIdsFromCollectionResult(result);

            if (thumbnailIds.size() == 0)
            {
                throw csp::common::continuations::ResultException("No Thumbnails exist.", MakeInvalid<AssetCollectionsResult>());
            }

            return thumbnailIds;
        };
    }

    std::function<async::task<AssetsResult>(const std::unordered_map<std::string, std::string>&)> GetThumbnailAssetsFromMap(AssetSystem* assetSystem)
    {
        return [assetSystem](const std::unordered_map<std::string, std::string>& result)
        {
            csp::common::Array<csp::common::String> messageIds(result.size());
            csp::common::Array<csp::common::String> assetIds(result.size());
            int i = 0;

            for (const auto& pair : result)
            {
                messageIds[i] = pair.first.c_str();
                assetIds[i] = pair.second.c_str();
                i++;
            }

            return assetSystem->GetAssetsByCriteria(messageIds, assetIds, nullptr, common::Array<EAssetType> { EAssetType::ANNOTATION_THUMBNAIL });
        };
    }

    std::function<multiplayer::AnnotationThumbnailCollectionResult(const AssetsResult&)> CreateAnnotationThumbnailCollectionResult()
    {
        return [](const AssetsResult& result)
        {
            multiplayer::AnnotationThumbnailCollectionResult thumbnailResult(
                EResultCode::Success, csp::web::EResponseCodes::ResponseOK, ERequestFailureReason::None);
            thumbnailResult.ParseAssets(result);
            return thumbnailResult;
        };
    }

    std::function<common::Map<common::String, common::String>()> GenerateAnnotationMetadata(
        const multiplayer::AnnotationUpdateParams& newData, std::shared_ptr<Asset> annotationAsset, std::shared_ptr<Asset> annotationThumbnailAsset)
    {
        return [newData, annotationAsset, annotationThumbnailAsset]()
        { return ConversationSystemHelpers::GenerateAnnotationAssetCollectionMetadata(newData, annotationAsset->Id, annotationThumbnailAsset->Id); };
    }
}

ConversationSystemInternal::ConversationSystemInternal(systems::AssetSystem* assetSystem, systems::SpaceSystem* spaceSystem,
    systems::UserSystem* userSystem, multiplayer::NetworkEventBus& eventBus, csp::common::LogSystem& logSystem)
    : SystemBase(&eventBus, &logSystem)
    , m_assetSystem { assetSystem }
    , m_spaceSystem { spaceSystem }
    , m_userSystem { userSystem }
{
    RegisterSystemCallback();
}

ConversationSystemInternal::~ConversationSystemInternal() { }

void ConversationSystemInternal::CreateConversation(const common::String& message, StringResultCallback callback)
{
    const Space& currentSpace = m_spaceSystem->GetCurrentSpace();
    const common::String& userId = m_userSystem->GetLoginState().UserId;
    const common::String& spaceId = currentSpace.Id;

    // 1. Create the comment container asset collection
    const AssetCollectionResultCallback addCommentContainerCallback
        = [this, callback, userId, message](const AssetCollectionResult& addCommentContainerResult)
    {
        if (HandleConversationResult(addCommentContainerResult, "The Comment Container asset collection creation was not successful.", callback)
            == false)
        {
            return;
        }

        // 2.Send multiplayer event
        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback
            = [callback, addCommentContainerResult](multiplayer::ErrorCode error)
        {
            if (error != multiplayer::ErrorCode::None)
            {
                const auto errorMsg = fmt::format("Create Conversation: SignalR connection error: {}", ErrorCodeToString(error));
                CSP_LOG_ERROR_MSG(errorMsg.c_str());

                INVOKE_IF_NOT_NULL(callback, MakeInvalid<StringResult>());
                return;
            }

            StringResult internalResult(addCommentContainerResult.GetResultCode(), addCommentContainerResult.GetHttpResultCode());
            internalResult.SetValue(addCommentContainerResult.GetAssetCollection().Id);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        };

        multiplayer::MessageInfo messageInfo
            = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(addCommentContainerResult.GetAssetCollection());

        SendConversationEvent(multiplayer::ConversationEventType::NewConversation, messageInfo, m_eventBusPtr, signalRCallback);
    };

    const auto uniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(spaceId, userId);
    multiplayer::MessageInfo defaultConversationInfo("", true, message);

    m_assetSystem->CreateAssetCollection(spaceId, nullptr, uniqueAssetCollectionName,
        ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(defaultConversationInfo), EAssetCollectionType::COMMENT_CONTAINER,
        nullptr, addCommentContainerCallback);
}

void ConversationSystemInternal::DeleteConversation(const common::String& conversationId, NullResultCallback callback)
{
    // 1. Get asset collection
    AssetCollectionResultCallback getConversationCallback = [this, conversationId, callback](const AssetCollectionResult& getConversationResult)
    {
        if (HandleConversationResult(getConversationResult, "The retrieval of Message asset collections was not successful.", callback) == false)
        {
            return;
        }

        AssetCollection conversationAssetCollection = getConversationResult.GetAssetCollection();

        // 2.Send multiplayer event
        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback
            = [this, callback, conversationId, conversationAssetCollection](multiplayer::ErrorCode error)
        {
            if (error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("DeleteConversation: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());
                return;
            }

            // 3. Delete the asset colleciton associated with this conversation
            NullResultCallback deleteAssetCollectionCallback = [callback](const NullResult& deleteAssetCollectionResult)
            {
                if (HandleConversationResult(
                        deleteAssetCollectionResult, "The deletion of the conversation asset collection was not successful.", callback)
                    == false)
                {
                    return;
                }

                callback(deleteAssetCollectionResult);
            };

            this->m_assetSystem->DeleteAssetCollection(conversationAssetCollection, deleteAssetCollectionCallback);
        };

        multiplayer::MessageInfo messageInfo
            = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(conversationAssetCollection);
        SendConversationEvent(multiplayer::ConversationEventType::DeleteConversation, messageInfo, m_eventBusPtr, signalRCallback);
    };

    m_assetSystem->GetAssetCollectionById(conversationId, getConversationCallback);
}

void ConversationSystemInternal::AddMessage(
    const common::String& conversationId, const common::String& message, multiplayer::MessageResultCallback callback)
{
    // 1. Store the conversation message
    const common::String& userId = m_userSystem->GetLoginState().UserId;

    const multiplayer::MessageResultCallback messageResultCallback
        = [this, callback, conversationId, userId, message](const multiplayer::MessageResult& messageResultCallbackResult)
    {
        if (HandleConversationResult(messageResultCallbackResult, "Failed to store conversation message.", callback) == false)
        {
            return;
        }

        // 2. Send multiplayer event
        const auto signalRCallback = [callback, messageResultCallbackResult](multiplayer::ErrorCode error)
        {
            if (error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("AddMessage: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(callback, MakeInvalid<multiplayer::MessageResult>());
                return;
            }

            INVOKE_IF_NOT_NULL(callback, messageResultCallbackResult);
        };

        const multiplayer::MessageInfo& messageInfo = messageResultCallbackResult.GetMessageInfo();
        SendConversationEvent(multiplayer::ConversationEventType::NewMessage, messageInfo, m_eventBusPtr, signalRCallback);
    };

    multiplayer::MessageInfo messageInfo(conversationId, false, message);

    const Space currentSpace = m_spaceSystem->GetCurrentSpace();

    StoreConversationMessage(messageInfo, currentSpace, messageResultCallback);
}

void ConversationSystemInternal::DeleteMessage(const common::String& conversationId, const common::String& messageId, NullResultCallback callback)
{
    // 1. Get asset collection
    AssetCollectionResultCallback getMessageCallback = [this, conversationId, messageId, callback](const AssetCollectionResult& getMessageResult)
    {
        if (HandleConversationResult(getMessageResult, "The retrieval of Message asset collections was not successful.", callback) == false)
        {
            return;
        }

        // Ensure client has correct permissions to delete the conversation
        multiplayer::MessageInfo info
            = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(getMessageResult.GetAssetCollection());

        if (EnsureUserHasPermission(m_userSystem->GetLoginState().UserId, info.UserId, false) == false)
        {
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());
            return;
        }

        // 2. Send multiplayer event
        const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback
            = [this, callback, conversationId, messageId](multiplayer::ErrorCode error)
        {
            if (error != multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_MSG("DeleteMessage: SignalR connection: Error");

                INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());
                return;
            }

            // 3. Delete the message asset collection
            AssetCollection messageAssetCollection;
            messageAssetCollection.Id = messageId;

            const NullResultCallback deleteAssetCollectionCallback = [callback](const NullResult& deleteAssetCollectionResult)
            {
                if (HandleConversationResult(deleteAssetCollectionResult, "Failed to delete Message asset collection.", callback) == false)
                {
                    return;
                }

                callback(deleteAssetCollectionResult);
            };

            this->m_assetSystem->DeleteAssetCollection(messageAssetCollection, deleteAssetCollectionCallback);
        };

        SendConversationEvent(multiplayer::ConversationEventType::DeleteMessage, info, m_eventBusPtr, signalRCallback);
    };

    m_assetSystem->GetAssetCollectionById(messageId, getMessageCallback);
}

void ConversationSystemInternal::GetMessagesFromConversation(const common::String& conversationId, const common::Optional<int>& resultsSkipNumber,
    const common::Optional<int>& resultsMaxNumber, multiplayer::MessageCollectionResultCallback callback)
{
    // 1. Find asset collections
    AssetCollectionsResultCallback getMessagesCallback = [callback](const AssetCollectionsResult& getMessagesResult)
    {
        if (HandleConversationResult(getMessagesResult, "The retrieval of Message asset collections was not successful.", callback) == false)
        {
            return;
        }

        // 2. Give result to caller
        multiplayer::MessageCollectionResult internalResult(getMessagesResult.GetResultCode(), getMessagesResult.GetHttpResultCode());
        internalResult.SetTotalCount(getMessagesResult.GetTotalCount());
        internalResult.FillMessageInfoCollection(getMessagesResult.GetAssetCollections());
        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    static const common::Array<EAssetCollectionType> prototypeTypes = { EAssetCollectionType::COMMENT };

    m_assetSystem->FindAssetCollections(
        nullptr, conversationId, nullptr, prototypeTypes, nullptr, nullptr, resultsSkipNumber, resultsMaxNumber, getMessagesCallback);
}

void ConversationSystemInternal::GetConversationInfo(const common::String& conversationId, multiplayer::ConversationResultCallback callback)
{
    // 1. Get asset collection
    AssetCollectionResultCallback getConversationCallback = [conversationId, callback](const AssetCollectionResult& getConversationResult)
    {
        if (HandleConversationResult(getConversationResult, "The retrieval of Message asset collections was not successful.", callback) == false)
        {
            return;
        }

        // 2. Give result to callers
        multiplayer::ConversationResult internalResult(getConversationResult.GetResultCode(), getConversationResult.GetHttpResultCode());
        internalResult.FillConversationInfo(getConversationResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    m_assetSystem->GetAssetCollectionById(conversationId, getConversationCallback);
}

void ConversationSystemInternal::UpdateConversation(
    const common::String& conversationId, const multiplayer::MessageUpdateParams& newData, multiplayer::ConversationResultCallback callback)
{
    // 1. Get asset collection
    AssetCollectionResultCallback getConversationCallback
        = [this, callback, conversationId, newData](const AssetCollectionResult& getConversationResult)
    {
        if (HandleConversationResult(getConversationResult, "The retrieval of Conversation asset collections was not successful.", callback) == false)
        {
            return;
        }

        // Ensure client has correct permissions to delete the conversation
        multiplayer::MessageInfo info
            = systems::ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(getConversationResult.GetAssetCollection());

        if (EnsureUserHasPermission(m_userSystem->GetLoginState().UserId, info.UserId, true) == false)
        {
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<multiplayer::ConversationResult>());
            return;
        }

        // 2. Update the conversations asset collection
        AssetCollectionResultCallback getUpdatedConversationCallback = [this, callback](const AssetCollectionResult& getUpdatedConversationResult)
        {
            if (HandleConversationResult(getUpdatedConversationResult, "The Update of Conversation asset collections was not successful.", callback)
                == false)
            {
                return;
            }

            // 3. Send multiplayer event
            const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback
                = [callback, getUpdatedConversationResult](multiplayer::ErrorCode error)
            {
                if (error != multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("SetConversationInfo: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(callback, MakeInvalid<multiplayer::ConversationResult>());
                    return;
                }

                multiplayer::ConversationResult result(
                    getUpdatedConversationResult.GetResultCode(), getUpdatedConversationResult.GetHttpResultCode());
                result.FillConversationInfo(getUpdatedConversationResult.GetAssetCollection());

                INVOKE_IF_NOT_NULL(callback, result);
            };

            auto updatedInfo = systems::ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(
                getUpdatedConversationResult.GetAssetCollection());

            SendConversationEvent(multiplayer::ConversationEventType::ConversationInformation, updatedInfo, m_eventBusPtr, signalRCallback);
        };

        const AssetCollection& conversationAssetCollection = getConversationResult.GetAssetCollection();
        common::Map<common::String, common::String> conversationAssetCollectionMetadata = conversationAssetCollection.GetMetadataImmutable();

        multiplayer::MessageInfo newConversationData
            = ConversationSystemHelpers::GetConversationInfoFromConversationAssetCollection(conversationAssetCollection);
        newConversationData.Message = newData.NewMessage;

        common::Map<common::String, common::String> newConversationMessageMetadata
            = ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(newConversationData);

        ConversationSystemHelpers::AppendMetadata(conversationAssetCollectionMetadata, newConversationMessageMetadata);

        this->m_assetSystem->UpdateAssetCollectionMetadata(
            conversationAssetCollection, conversationAssetCollectionMetadata, nullptr, getUpdatedConversationCallback);
    };

    m_assetSystem->GetAssetCollectionById(conversationId, getConversationCallback);
}

void ConversationSystemInternal::GetMessageInfo(
    const common::String& /*ConversationId*/, const common::String& messageId, multiplayer::MessageResultCallback callback)
{
    const AssetCollectionResultCallback getMessageCallback = [callback](const AssetCollectionResult& getMessageResult)
    {
        if (HandleConversationResult(getMessageResult, "The retrieval of the Message asset collection was not successful.", callback) == false)
        {
            return;
        }

        multiplayer::MessageResult internalResult(getMessageResult.GetResultCode(), getMessageResult.GetHttpResultCode());
        internalResult.FillMessageInfo(getMessageResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    m_assetSystem->GetAssetCollectionById(messageId, getMessageCallback);
}

void ConversationSystemInternal::UpdateMessage(const common::String& /*ConversationId*/, const common::String& messageId,
    const multiplayer::MessageUpdateParams& newData, multiplayer::MessageResultCallback callback)
{
    // 1. Get message asset collection
    AssetCollectionResultCallback getMessageCallback = [this, callback, messageId, newData](const AssetCollectionResult& getMessageResult)
    {
        if (HandleConversationResult(getMessageResult, "The retrieval of Conversation asset collections was not successful.", callback) == false)
        {
            return;
        }

        // Ensure client has correct permissions to delete the conversation
        multiplayer::MessageInfo info
            = systems::ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(getMessageResult.GetAssetCollection());

        if (EnsureUserHasPermission(m_userSystem->GetLoginState().UserId, info.UserId, false) == false)
        {
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<multiplayer::MessageResult>());
            return;
        }

        // 2. Update asset collections metadata
        AssetCollectionResultCallback getUpdatedMessageCallback
            = [this, callback, messageId, newData](const AssetCollectionResult& getUpdatedMessageResult)

        {
            if (HandleConversationResult(getUpdatedMessageResult, "The Update of Message asset collections was not successful.", callback) == false)
            {
                return;
            }

            multiplayer::MessageResult result(getUpdatedMessageResult.GetResultCode(), getUpdatedMessageResult.GetHttpResultCode());
            result.FillMessageInfo(getUpdatedMessageResult.GetAssetCollection());

            // 3. Send multiplayer event
            const multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback
                = [callback, getUpdatedMessageResult, newData, result](multiplayer::ErrorCode error)
            {
                if (error != multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_MSG("SetMessageInfo: SignalR connection: Error");

                    INVOKE_IF_NOT_NULL(callback, MakeInvalid<multiplayer::MessageResult>());
                    return;
                }

                INVOKE_IF_NOT_NULL(callback, result);
            };

            SendConversationEvent(multiplayer::ConversationEventType::MessageInformation, result.GetMessageInfo(), m_eventBusPtr, signalRCallback);
        };

        const AssetCollection& messageAssetCollection = getMessageResult.GetAssetCollection();
        common::Map<common::String, common::String> messageAssetCollectionMetadata = messageAssetCollection.GetMetadataImmutable();

        multiplayer::MessageInfo newMessageData = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(messageAssetCollection);
        newMessageData.Message = newData.NewMessage;

        common::Map<common::String, common::String> newMessageMetadata
            = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(newMessageData);

        ConversationSystemHelpers::AppendMetadata(messageAssetCollectionMetadata, newMessageMetadata);

        this->m_assetSystem->UpdateAssetCollectionMetadata(messageAssetCollection, messageAssetCollectionMetadata, nullptr, getUpdatedMessageCallback);
    };

    m_assetSystem->GetAssetCollectionById(messageId, getMessageCallback);
}

void ConversationSystemInternal::StoreConversationMessage(
    const multiplayer::MessageInfo& info, const Space& space, multiplayer::MessageResultCallback callback) const
{
    const AssetCollectionResultCallback addCommentCallback = [=](const AssetCollectionResult& addCommentResult)
    {
        if (HandleConversationResult(addCommentResult, "The Comment asset collection creation was not successful.", callback) == false)
        {
            return;
        }

        multiplayer::MessageResult result;
        result.FillMessageInfo(addCommentResult.GetAssetCollection());
        INVOKE_IF_NOT_NULL(callback, result);
    };

    const auto uniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(space.Id, info.UserId);
    const auto messageMetadata = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(info);

    m_assetSystem->CreateAssetCollection(
        space.Id, info.ConversationId, uniqueAssetCollectionName, messageMetadata, EAssetCollectionType::COMMENT, nullptr, addCommentCallback);
}

void ConversationSystemInternal::DeleteMessages(
    const common::String& /*ConversationId*/, common::Array<AssetCollection>& messages, NullResultCallback callback)
{
    if (messages.Size() == 0)
    {
        NullResult internalResult(EResultCode::Success, (uint16_t)web::EResponseCodes::ResponseNoContent);
        INVOKE_IF_NOT_NULL(callback, internalResult);

        return;
    }

    m_assetSystem->DeleteMultipleAssetCollections(messages, callback);
}

void ConversationSystemInternal::GetNumberOfReplies(const common::String& conversationId, csp::multiplayer::NumberOfRepliesResultCallback callback)
{
    auto getMessageCountCallback = [callback](const csp::systems::AssetCollectionCountResult& getMessageResult)
    {
        csp::multiplayer::NumberOfRepliesResult result(getMessageResult);
        result.m_count = getMessageResult.GetCount();
        callback(result);
    };

    static const csp::common::Array<csp::systems::EAssetCollectionType> prototypeTypes = { csp::systems::EAssetCollectionType::COMMENT };

    m_assetSystem->GetAssetCollectionCount(nullptr, conversationId, nullptr, prototypeTypes, nullptr, nullptr, getMessageCountCallback);
}

void ConversationSystemInternal::GetConversationAnnotation(const csp::common::String& conversationId, multiplayer::AnnotationResultCallback callback)
{
    auto conversationAssetCollection = std::make_shared<AssetCollection>();
    auto annotationAsset = std::make_shared<Asset>();
    auto annotationThumbnailAsset = std::make_shared<Asset>();

    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;

    // 1. Get conversation asset collection
    m_assetSystem->GetAssetCollectionById(conversationId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved message asset collection",
            "Failed to get message asset collection.", {}, {}, {}))
        .then(ValidateAnnotationMetadata())
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {},
            {}, {}))
        .then(SetMessageAssetCollection(conversationAssetCollection))
        // 3. Get annotation asset
        .then(GetAnnotationAsset(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {},
            {}, {}))
        .then(SetAnnotationAssetFromAssets(annotationAsset))
        // 4. Get annotation thumbnail asset
        .then(GetAnnotationThumbnailAsset(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetConversationAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAssetFromAssets(annotationThumbnailAsset))
        // 5. Process result
        .then(CreateAnnotationResult(conversationAssetCollection, annotationAsset, annotationThumbnailAsset))
        .then(systems::continuations::SendResult(callback, "Successfully retrieved annotation."))
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<multiplayer::AnnotationResult>(exception)); }));
}

void ConversationSystemInternal::SetConversationAnnotation(const csp::common::String& conversationId,
    const multiplayer::AnnotationUpdateParams& annotationParams, const systems::BufferAssetDataSource& annotation,
    const systems::BufferAssetDataSource& annotationThumbnail, multiplayer::AnnotationResultCallback callback)
{
    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;
    const csp::common::String userId = m_userSystem->GetLoginState().UserId;

    const common::String uniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueAnnotationAssetCollectionName(spaceId, userId);
    const csp::common::String uniqueAnnotationAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(spaceId, userId);
    const csp::common::String uniqueAnnotationThumbnailAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(spaceId, userId);

    const csp::common::String uniqueAnnotationAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(spaceId, userId)
        + SpaceSystemHelpers::GetAssetFileExtension(annotation.GetMimeType());
    const csp::common::String uniqueAnnotationThumbnailAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(spaceId, userId)
        + SpaceSystemHelpers::GetAssetFileExtension(annotationThumbnail.GetMimeType());

    auto conversationAssetCollection = std::make_shared<AssetCollection>();
    auto annotationAsset = std::make_shared<Asset>();
    auto annotationThumbnailAsset = std::make_shared<Asset>();

    // 1. Get conversation asset collection
    m_assetSystem->GetAssetCollectionById(conversationId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::SetConversationAnnotation, successfully retrieved message asset collection",
            "Failed to get message asset collection.", {}, {}, {}))
        .then(SetMessageAssetCollection(conversationAssetCollection))
        // 2. Create Annotation asset
        .then(GetOrCreateAnnotationAsset(m_assetSystem, conversationAssetCollection, uniqueAnnotationAssetName, EAssetType::ANNOTATION))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(
            "ConversationSystemInternal::SetConversationAnnotation, successfully created annotation asset", "Failed to create annotation asset.", {},
            {}, {}))
        .then(SetAnnotationAsset(annotationAsset))
        // 3. Upload Annotation asset data
        .then(UploadAnnotationAssetData(m_assetSystem, conversationAssetCollection, annotationAsset, annotation, uniqueAnnotationAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            "ConversationSystemInternal::SetConversationAnnotation, successfully uploaded annotation asset data",
            "Failed to upload annotation asset data.", {}, {}, {}))
        .then(SetAssetUri(annotationAsset))
        // 4. Create Annotation thumbnail asset
        .then(GetOrCreateAnnotationAsset(m_assetSystem, conversationAssetCollection, uniqueAnnotationAssetName, EAssetType::ANNOTATION_THUMBNAIL))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(
            "ConversationSystemInternal::SetConversationAnnotation, successfully created annotation thumbnail asset",
            "Failed to create annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAsset(annotationThumbnailAsset))
        // 5. Upload Annotation thumbnail asset data
        .then(UploadAnnotationAssetData(
            m_assetSystem, conversationAssetCollection, annotationThumbnailAsset, annotationThumbnail, uniqueAnnotationThumbnailAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            "ConversationSystemInternal::SetConversationAnnotation, successfully uploaded annotation thumbnail asset data",
            "Failed to upload annotation thumbnail asset data.", {}, {}, {}))
        .then(SetAssetUri(annotationThumbnailAsset))
        // 6. Update asset collection metadata
        .then(GenerateAnnotationMetadata(annotationParams, annotationAsset, annotationThumbnailAsset))
        .then(AppendCommentMetadata(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::SetConversationAnnotation, successfully updated message asset collection metadata",
            "Failed to update message asset collection metadata.", {}, {}, {}))
        .then(SetMessageAssetCollection(conversationAssetCollection))
        // 7. Send multiplayer event
        .then(SendConversationEvent(multiplayer::ConversationEventType::SetConversationAnnotation, conversationAssetCollection, m_eventBusPtr))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(
            "ConversationSystemInternal::SetConversationAnnotation, successfully sent multiplayer event",
            MakeInvalid<multiplayer::AnnotationResult>(), *m_logSystem))
        // 8. Process result
        .then(CreateAnnotationResult(conversationAssetCollection, annotationAsset, annotationThumbnailAsset))
        .then(systems::continuations::SendResult(callback, "Successfully set annotation."))
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<multiplayer::AnnotationResult>(exception)); }));
}

void ConversationSystemInternal::DeleteConversationAnnotation(const csp::common::String& conversationId, systems::NullResultCallback callback)
{
    auto conversationAssetCollection = std::make_shared<AssetCollection>();

    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;

    // 1. Get conversation asset collection
    m_assetSystem->GetAssetCollectionById(conversationId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::DeleteAnnotation, successfully retrieved asset collection", "Failed to get asset collection.", {}, {}, {}))
        .then(ValidateAnnotationMetadata())
        // 2. Remove annotation metadata
        .then(RemoveAnnotationMetadata(m_assetSystem))
        .then(SetMessageAssetCollection(conversationAssetCollection))
        // 3. Send multiplayer event
        .then(SendConversationEvent(multiplayer::ConversationEventType::DeleteConversationAnnotation, conversationAssetCollection, m_eventBusPtr))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(
            "ConversationSystemInternal::DeleteAnnotation, successfully sent multiplayer event", MakeInvalid<systems::NullResult>(), *m_logSystem))
        // 4. Delete annoation asset
        .then(GetAnnotationAsset(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation asset", "Failed to deleted annotation asset.", {}, {}, {}))
        // 5. Delete annoation thumbnail asset
        .then(GetAnnotationThumbnailAsset(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(m_assetSystem, conversationAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation thumbnail asset",
            "Failed to deleted annotation thumbnail asset.", {}, {}, {}))
        // 6. Process result
        .then(systems::continuations::ReportSuccess(callback, "Successfully deleted annotation."))
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<NullResult>(exception)); }));
}

void ConversationSystemInternal::GetAnnotation(
    const csp::common::String& conversationId, const csp::common::String& messageId, multiplayer::AnnotationResultCallback callback)
{
    auto messageAssetCollection = std::make_shared<AssetCollection>();
    auto annotationAsset = std::make_shared<Asset>();
    auto annotationThumbnailAsset = std::make_shared<Asset>();

    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;

    // 1. Get message asset collection
    m_assetSystem->GetAssetCollectionById(messageId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved message asset collection", "Failed to get message asset collection.",
            {}, {}, {}))
        .then(ValidateMessageAssetCollection(conversationId))
        .then(ValidateAnnotationMetadata())
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetMessageAssetCollection(messageAssetCollection))
        // 3. Get annotation asset
        .then(GetAnnotationAsset(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(SetAnnotationAssetFromAssets(annotationAsset))
        // 4. Get annotation thumbnail asset
        .then(GetAnnotationThumbnailAsset(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAssetFromAssets(annotationThumbnailAsset))
        // 5. Process result
        .then(CreateAnnotationResult(messageAssetCollection, annotationAsset, annotationThumbnailAsset))
        .then(systems::continuations::SendResult(callback, "Successfully retrieved annotation."))
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<multiplayer::AnnotationResult>(exception)); }));
}

void ConversationSystemInternal::SetAnnotation(const csp::common::String& conversationId, const csp::common::String& messageId,
    const multiplayer::AnnotationUpdateParams& annotationParams, const systems::BufferAssetDataSource& annotation,
    const systems::BufferAssetDataSource& annotationThumbnail, multiplayer::AnnotationResultCallback callback)
{
    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;
    const csp::common::String userId = m_userSystem->GetLoginState().UserId;

    const common::String uniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueAnnotationAssetCollectionName(spaceId, userId);
    const csp::common::String uniqueAnnotationAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(spaceId, userId);
    const csp::common::String uniqueAnnotationThumbnailAssetName = ConversationSystemHelpers::GetUniqueAnnotationAssetName(spaceId, userId);

    const csp::common::String uniqueAnnotationAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(spaceId, userId)
        + SpaceSystemHelpers::GetAssetFileExtension(annotation.GetMimeType());
    const csp::common::String uniqueAnnotationThumbnailAssetFileName = ConversationSystemHelpers::GetUniqueAnnotationAssetFileName(spaceId, userId)
        + SpaceSystemHelpers::GetAssetFileExtension(annotationThumbnail.GetMimeType());

    auto messageAssetCollection = std::make_shared<AssetCollection>();
    auto annotationAsset = std::make_shared<Asset>();
    auto annotationThumbnailAsset = std::make_shared<Asset>();

    // 1. Get message asset collection
    m_assetSystem->GetAssetCollectionById(messageId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::SetAnnotation, successfully retrieved message asset collection", "Failed to get message asset collection.",
            {}, {}, {}))
        .then(ValidateMessageAssetCollection(conversationId))
        .then(SetMessageAssetCollection(messageAssetCollection))
        // 2. Create Annotation asset
        .then(GetOrCreateAnnotationAsset(m_assetSystem, messageAssetCollection, uniqueAnnotationAssetName, EAssetType::ANNOTATION))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(
            "ConversationSystemInternal::SetAnnotation, successfully created annotation asset", "Failed to create annotation asset.", {}, {}, {}))
        .then(SetAnnotationAsset(annotationAsset))
        // 3. Upload Annotation asset data
        .then(UploadAnnotationAssetData(m_assetSystem, messageAssetCollection, annotationAsset, annotation, uniqueAnnotationAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            "ConversationSystemInternal::SetAnnotation, successfully uploaded annotation asset data", "Failed to upload annotation asset data.", {},
            {}, {}))
        .then(SetAssetUri(annotationAsset))
        // 4. Create Annotation thumbnail asset
        .then(GetOrCreateAnnotationAsset(m_assetSystem, messageAssetCollection, uniqueAnnotationAssetName, EAssetType::ANNOTATION_THUMBNAIL))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(
            "ConversationSystemInternal::SetAnnotation, successfully created annotation thumbnail asset",
            "Failed to create annotation thumbnail asset.", {}, {}, {}))
        .then(SetAnnotationAsset(annotationThumbnailAsset))
        // 5. Upload Annotation thumbnail asset data
        .then(UploadAnnotationAssetData(
            m_assetSystem, messageAssetCollection, annotationThumbnailAsset, annotationThumbnail, uniqueAnnotationThumbnailAssetFileName))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            "ConversationSystemInternal::SetAnnotation, successfully uploaded annotation thumbnail asset data",
            "Failed to upload annotation thumbnail asset data.", {}, {}, {}))
        .then(SetAssetUri(annotationThumbnailAsset))
        // 6. Update asset collection metadata
        .then(GenerateAnnotationMetadata(annotationParams, annotationAsset, annotationThumbnailAsset))
        .then(AppendCommentMetadata(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::SetAnnotation, successfully updated message asset collection metadata",
            "Failed to update message asset collection metadata.", {}, {}, {}))
        .then(SetMessageAssetCollection(messageAssetCollection))
        // 7. Send multiplayer event
        .then(SendConversationMessageEvent(multiplayer::ConversationEventType::SetAnnotation, messageAssetCollection, m_eventBusPtr))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(
            "ConversationSystemInternal::SetAnnotation, successfully sent multiplayer event", MakeInvalid<multiplayer::AnnotationResult>(),
            *m_logSystem))
        // 8. Process result
        .then(CreateAnnotationResult(messageAssetCollection, annotationAsset, annotationThumbnailAsset))
        .then(systems::continuations::SendResult(callback, "Successfully set annotation."))
        .then(common::continuations::InvokeIfExceptionInChain(
            *m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<multiplayer::AnnotationResult>(exception)); },
            [callback]([[maybe_unused]] const std::exception& exception) { callback(MakeInvalid<multiplayer::AnnotationResult>()); }));
}

void ConversationSystemInternal::DeleteAnnotation(
    const csp::common::String& conversationId, const csp::common::String& messageId, systems::NullResultCallback callback)
{
    auto messageAssetCollection = std::make_shared<AssetCollection>();

    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;

    // 1. Get message asset collection
    m_assetSystem->GetAssetCollectionById(messageId)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "ConversationSystemInternal::DeleteAnnotation, successfully retrieved asset collection", "Failed to get asset collection.", {}, {}, {}))
        .then(ValidateMessageAssetCollection(conversationId))
        .then(ValidateAnnotationMetadata())
        // 2. Remove annotation metadata
        .then(RemoveAnnotationMetadata(m_assetSystem))
        .then(SetMessageAssetCollection(messageAssetCollection))
        // 3. Send multiplayer event
        .then(SendConversationMessageEvent(multiplayer::ConversationEventType::DeleteAnnotation, messageAssetCollection, m_eventBusPtr))
        .then(common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(
            "ConversationSystemInternal::DeleteAnnotation, successfully sent multiplayer event", MakeInvalid<systems::NullResult>(), *m_logSystem))
        // 4. Delete annoation asset
        .then(GetAnnotationAsset(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation asset", "Failed to get annotation asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation asset", "Failed to deleted annotation asset.", {}, {}, {}))
        // 5. Delete annoation thumbnail asset
        .then(GetAnnotationThumbnailAsset(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotation, successfully retrieved annotation thumbnail asset",
            "Failed to get annotation thumbnail asset.", {}, {}, {}))
        .then(DeleteAnnotationAsset(m_assetSystem, messageAssetCollection))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            "ConversationSystemInternal::GetAnnotation, successfully deleted annotation thumbnail asset",
            "Failed to deleted annotation thumbnail asset.", {}, {}, {}))
        // 6. Process result
        .then(systems::continuations::ReportSuccess(callback, "Successfully deleted annotation."))
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<NullResult>(exception)); }));
}

void ConversationSystemInternal::GetAnnotationThumbnailsForConversation(
    const csp::common::String& conversationId, multiplayer::AnnotationThumbnailCollectionResultCallback callback)
{
    const csp::common::String spaceId = m_spaceSystem->GetCurrentSpace().Id;

    // 1. Get all message asset collections
    FindMessageAssetCollections(m_assetSystem, conversationId, spaceId)()
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionsResult>(
            "ConversationSystemInternal::GetAnnotationThumbnailsForConversation, successfully retrieved message asset collections",
            "Failed to get message asset collections.", {}, {}, {}))
        // 2. Get all annotation thumbnail assets
        .then(GetAnnotationAssetIdsFromCollections())
        .then(GetThumbnailAssetsFromMap(m_assetSystem))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetsResult>(
            "ConversationSystemInternal::GetAnnotationThumbnailsForConversation, successfully retrieved thumbnail assets",
            "Failed to get thumbnail assets.", {}, {}, {}))
        // 3. Process result
        .then(CreateAnnotationThumbnailCollectionResult())
        .then(systems::continuations::SendResult(callback, "Successfully retrieved annotation thumbnails."))
        .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            { callback(csp::common::continuations::GetResultExceptionOrInvalid<multiplayer::AnnotationThumbnailCollectionResult>(exception)); }));
}

void ConversationSystemInternal::RegisterComponent(csp::multiplayer::ConversationSpaceComponent* component)
{
    m_components.insert(component);
    FlushEvents();
}

void ConversationSystemInternal::DeregisterComponent(csp::multiplayer::ConversationSpaceComponent* component) { m_components.erase(component); }

void ConversationSystemInternal::RegisterSystemCallback()
{
    if (!m_eventBusPtr)
    {
        CSP_LOG_ERROR_MSG(
            "Error: Failed to register ConversationSystemInternal. NetworkEventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    m_eventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::ConversationSystemInternal",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::Conversation)),
        [this](const csp::common::NetworkEventData& networkEventData)
        {
            const csp::common::ConversationNetworkEventData& conversationNetworkEventData
                = static_cast<const csp::common::ConversationNetworkEventData&>(networkEventData);
            if (TrySendEvent(conversationNetworkEventData) == false)
            {
                // If component doesn't exist, add it to the queue for processing later
                std::unique_ptr<csp::common::ConversationNetworkEventData> eventDataCopy
                    = std::make_unique<csp::common::ConversationNetworkEventData>(conversationNetworkEventData);
                m_events.push_back(std::move(eventDataCopy));
            }
        });
}

void ConversationSystemInternal::FlushEvents()
{
    for (auto it = std::begin(m_events); it != std::end(m_events);)
    {
        if (TrySendEvent(**it))
        {
            // Event has now been processed, so remove
            it = m_events.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool ConversationSystemInternal::TrySendEvent(const csp::common::ConversationNetworkEventData& params)
{
    for (const auto& component : m_components)
    {
        if (component->GetConversationId() == params.MessageInfo.ConversationId)
        {
            if (component->m_conversationUpdateCallback != nullptr)
            {
                component->m_conversationUpdateCallback(params);
                return true;
            }
        }
    }

    return false;
}
}
