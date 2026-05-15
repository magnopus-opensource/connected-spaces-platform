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

#pragma once

#include "CSP/Common/Map.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/SpaceTransform.h"

#include <unordered_map>

namespace csp::multiplayer
{

class MessageInfo;
class AnnotationData;
}

namespace csp::systems
{

class AssetCollection;

namespace ConversationSystemHelpers
{
    common::String GetUniqueConversationContainerAssetCollectionName(const common::String& spaceId, const common::String& creatorUserId);
    common::String GetUniqueMessageAssetCollectionName(const common::String& spaceId, const common::String& creatorUserId);
    common::String GetUniqueAnnotationAssetCollectionName(const common::String& spaceId, const common::String& creatorUserId);
    common::String GetUniqueAnnotationAssetName(const common::String& spaceId, const common::String& creatorUserId);
    common::String GetUniqueAnnotationThumbnailAssetName(const common::String& spaceId, const common::String& creatorUserId);
    common::String GetUniqueAnnotationAssetFileName(const common::String& spaceId, const common::String& creatorUserId);
    common::String GetUniqueAnnotationThumbnailFileName(const common::String& spaceId, const common::String& creatorUserId);
    common::Map<common::String, common::String> GenerateMessageAssetCollectionMetadata(const multiplayer::MessageInfo& messageData);
    common::Map<common::String, common::String> GenerateConversationAssetCollectionMetadata(const multiplayer::MessageInfo& conversationData);
    common::Map<common::String, common::String> GenerateAnnotationAssetCollectionMetadata(const multiplayer::AnnotationUpdateParams& annotationData,
        const csp::common::String& annotationId, const csp::common::String& annotationThumbnailId);
    common::Map<common::String, common::String> RemoveAnnotationMetadata(const AssetCollection& messageAssetCollection);
    multiplayer::MessageInfo GetConversationInfoFromConversationAssetCollection(const AssetCollection& conversationAssetCollection);
    multiplayer::MessageInfo GetMessageInfoFromMessageAssetCollection(const AssetCollection& messageAssetCollection);
    multiplayer::AnnotationData GetAnnotationDataFromMessageAssetCollection(const AssetCollection& messageAssetCollection);
    bool HasAnnotationMetadata(const AssetCollection& messageAssetCollection);
    CSP_NO_EXPORT std::unordered_map<std::string, std::string> GetAnnotationThumbnailAssetIdsFromCollectionResult(
        const AssetCollectionsResult& result);

    csp::common::Array<common::ReplicatedValue> MessageInfoToReplicatedValueArray(
        csp::multiplayer::ConversationEventType messageName, const multiplayer::MessageInfo& messageInfo);

    void AppendMetadata(csp::common::Map<csp::common::String, csp::common::String>& metadataToUpdate,
        const csp::common::Map<csp::common::String, csp::common::String>& newMetadataValues);
}

} // namespace csp::multiplayer
