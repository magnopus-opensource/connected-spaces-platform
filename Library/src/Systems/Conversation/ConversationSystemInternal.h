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

#include "CSP/CSPCommon.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Systems/SystemBase.h"

#include <unordered_set>

namespace csp::multiplayer
{
class ConversationSpaceComponent;
class MessageUpdateParams;
}

namespace csp::systems
{
class AssetSystem;
class SpaceSystem;
class UserSystem;

/*
This is an internal system used by ConversationComponent
Read the documentation for ConversationComponent for more information
*/
class CSP_API ConversationSystemInternal : public SystemBase
{
public:
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

    ConversationSystemInternal(csp::systems::AssetSystem* assetSystem, csp::systems::SpaceSystem* spaceSystem, csp::systems::UserSystem* userSystem,
        csp::multiplayer::NetworkEventBus& eventBus, csp::common::LogSystem& logSystem);

    ~ConversationSystemInternal();

    ConversationSystemInternal(const ConversationSystemInternal& other) = delete;
    ConversationSystemInternal& operator=(const ConversationSystemInternal& other) = delete;

    void CreateConversation(const csp::common::String& message, csp::systems::StringResultCallback callback);

    void DeleteConversation(const csp::common::String& conversationId, csp::systems::NullResultCallback callback);

    void AddMessage(const csp::common::String& conversationId, const csp::common::String& message, multiplayer::MessageResultCallback callback);

    void DeleteMessage(const csp::common::String& conversationId, const csp::common::String& messageId, systems::NullResultCallback callback);

    void GetMessagesFromConversation(const csp::common::String& conversationId, const csp::common::Optional<int>& resultsSkipNumber,
        const csp::common::Optional<int>& resultsMaxNumber, multiplayer::MessageCollectionResultCallback callback);

    void GetConversationInfo(const csp::common::String& conversationId, multiplayer::ConversationResultCallback callback);

    void UpdateConversation(
        const csp::common::String& conversationId, const multiplayer::MessageUpdateParams& newData, multiplayer::ConversationResultCallback callback);

    void GetMessageInfo(const csp::common::String& conversationId, const csp::common::String& messageId, multiplayer::MessageResultCallback callback);

    void UpdateMessage(const csp::common::String& conversationId, const csp::common::String& messageId,
        const multiplayer::MessageUpdateParams& newData, multiplayer::MessageResultCallback callback);

    void StoreConversationMessage(
        const multiplayer::MessageInfo& info, const csp::systems::Space& space, multiplayer::MessageResultCallback callback) const;

    void DeleteMessages(const csp::common::String& conversationId, csp::common::Array<csp::systems::AssetCollection>& messages,
        csp::systems::NullResultCallback callback);

    void GetNumberOfReplies(const common::String& conversationId, csp::multiplayer::NumberOfRepliesResultCallback callback);

    void GetConversationAnnotation(const csp::common::String& conversationId, multiplayer::AnnotationResultCallback callback);

    void SetConversationAnnotation(const csp::common::String& conversationId, const multiplayer::AnnotationUpdateParams& annotationParams,
        const systems::BufferAssetDataSource& annotation, const systems::BufferAssetDataSource& annotationThumbnail,
        multiplayer::AnnotationResultCallback callback);

    void DeleteConversationAnnotation(const csp::common::String& conversationId, systems::NullResultCallback callback);

    void GetAnnotation(
        const csp::common::String& conversationId, const csp::common::String& messageId, multiplayer::AnnotationResultCallback callback);

    void SetAnnotation(const csp::common::String& conversationId, const csp::common::String& messageId,
        const multiplayer::AnnotationUpdateParams& annotationParams, const systems::BufferAssetDataSource& annotation,
        const systems::BufferAssetDataSource& annotationThumbnail, multiplayer::AnnotationResultCallback callback);

    void DeleteAnnotation(const csp::common::String& conversationId, const csp::common::String& messageId, systems::NullResultCallback callback);

    void GetAnnotationThumbnailsForConversation(
        const csp::common::String& conversationId, multiplayer::AnnotationThumbnailCollectionResultCallback callback);

    void RegisterComponent(csp::multiplayer::ConversationSpaceComponent* component);
    void DeregisterComponent(csp::multiplayer::ConversationSpaceComponent* component);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;

    // Attempt to flush any events that haven't been sent.
    // They may fail to send in situtations where the conversation component hasn't been created before the creation event fires.
    void FlushEvents();

private:
    bool TrySendEvent(const csp::common::ConversationNetworkEventData& params);

    csp::systems::AssetSystem* m_assetSystem;
    csp::systems::SpaceSystem* m_spaceSystem;
    csp::systems::UserSystem* m_userSystem;

    std::unordered_set<csp::multiplayer::ConversationSpaceComponent*> m_components;
    std::vector<std::unique_ptr<csp::common::ConversationNetworkEventData>> m_events;
};

}
