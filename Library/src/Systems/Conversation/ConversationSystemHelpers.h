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
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
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
    common::String GetUniqueConversationContainerAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::String GetUniqueMessageAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::String GetUniqueAnnotationAssetCollectionName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::String GetUniqueAnnotationAssetName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::String GetUniqueAnnotationThumbnailAssetName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::String GetUniqueAnnotationAssetFileName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::String GetUniqueAnnotationThumbnailFileName(const common::String& SpaceId, const common::String& CreatorUserId);
    common::Map<common::String, common::String> GenerateMessageAssetCollectionMetadata(const multiplayer::MessageInfo& MessageData);
    common::Map<common::String, common::String> GenerateConversationAssetCollectionMetadata(const multiplayer::MessageInfo& ConversationData);
    common::Map<common::String, common::String> GenerateAnnotationAssetCollectionMetadata(const multiplayer::AnnotationUpdateParams& AnnotationData,
        const csp::common::String& AnnotationId, const csp::common::String& AnnotationThumbnailId);
    common::Map<common::String, common::String> RemoveAnnotationMetadata(const AssetCollection& MessageAssetCollection);
    multiplayer::MessageInfo GetConversationInfoFromConversationAssetCollection(const AssetCollection& ConversationAssetCollection);
    multiplayer::MessageInfo GetMessageInfoFromMessageAssetCollection(const AssetCollection& MessageAssetCollection);
    multiplayer::AnnotationData GetAnnotationDataFromMessageAssetCollection(const AssetCollection& MessageAssetCollection);
    bool HasAnnotationMetadata(const AssetCollection& MessageAssetCollection);
    CSP_NO_EXPORT std::unordered_map<std::string, std::string> GetAnnotationThumbnailAssetIdsFromCollectionResult(
        const AssetCollectionsResult& Result);

    csp::common::Array<multiplayer::ReplicatedValue> MessageInfoToReplicatedValueArray(const multiplayer::ConversationEventParams& Params);
}

} // namespace csp::multiplayer
