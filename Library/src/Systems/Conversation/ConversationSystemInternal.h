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
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/SystemBase.h"

#include <unordered_set>

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

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
    friend void csp::memory::Delete<ConversationSystemInternal>(ConversationSystemInternal* Ptr);
    /** @endcond */
    CSP_END_IGNORE

    ConversationSystemInternal(csp::systems::AssetSystem* AssetSystem, csp::systems::SpaceSystem* SpaceSystem, csp::systems::UserSystem* UserSystem,
        csp::multiplayer::EventBus* InEventBus);

    ~ConversationSystemInternal();

    void CreateConversation(const csp::common::String& Message, csp::systems::StringResultCallback Callback);

    void DeleteConversation(const csp::common::String& ConversationId, csp::systems::NullResultCallback Callback);

    void AddMessage(const csp::common::String& ConversationId, const csp::common::String& Message, multiplayer::MessageResultCallback Callback);

    void DeleteMessage(const csp::common::String& ConversationId, const csp::common::String& MessageId, systems::NullResultCallback Callback);

    void GetMessagesFromConversation(const csp::common::String& ConversationId, const csp::common::Optional<int>& ResultsSkipNumber,
        const csp::common::Optional<int>& ResultsMaxNumber, multiplayer::MessageCollectionResultCallback Callback);

    void GetConversationInfo(const csp::common::String& ConversationId, multiplayer::ConversationResultCallback Callback);

    void UpdateConversation(
        const csp::common::String& ConversationId, const multiplayer::MessageUpdateParams& NewData, multiplayer::ConversationResultCallback Callback);

    void GetMessageInfo(const csp::common::String& ConversationId, const csp::common::String& MessageId, multiplayer::MessageResultCallback Callback);

    void UpdateMessage(const csp::common::String& ConversationId, const csp::common::String& MessageId,
        const multiplayer::MessageUpdateParams& NewData, multiplayer::MessageResultCallback Callback);

    void StoreConversationMessage(
        const multiplayer::MessageInfo& Info, const csp::systems::Space& Space, multiplayer::MessageResultCallback Callback) const;

    void DeleteMessages(const csp::common::String& ConversationId, csp::common::Array<csp::systems::AssetCollection>& Messages,
        csp::systems::NullResultCallback Callback);

    void GetNumberOfReplies(const common::String& ConversationId, csp::multiplayer::NumberOfRepliesResultCallback Callback);

    void GetAnnotation(
        const csp::common::String& ConversationId, const csp::common::String& MessageId, multiplayer::AnnotationResultCallback Callback);

    void SetAnnotation(const csp::common::String& ConversationId, const csp::common::String& MessageId,
        const multiplayer::AnnotationUpdateParams& AnnotationParams, const systems::BufferAssetDataSource& Annotation,
        const csp::common::String& AnnotationFileExtension, const systems::BufferAssetDataSource& AnnotationThumbnail,
        const csp::common::String& ThumbnailFileExtension, multiplayer::AnnotationResultCallback Callback);

    void DeleteAnnotation(const csp::common::String& ConversationId, const csp::common::String& MessageId, systems::NullResultCallback Callback);

    void GetAnnotationThumbnailsForConversation(
        const csp::common::String& ConversationId, multiplayer::AnnotationThumbnailCollectionResultCallback Callback);

    void RegisterComponent(csp::multiplayer::ConversationSpaceComponent* Component);
    void DeregisterComponent(csp::multiplayer::ConversationSpaceComponent* Component);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;
    /// @brief Deregisters the system from listening for the named event.
    void DeregisterSystemCallback() override;
    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT void OnEvent(const std::vector<signalr::value>& EventValues) override;

    // Attempt to flush any events that haven't been sent.
    // They may fail to send in situtations where the conversation component hasn't been created before the creation event fires.
    void FlushEvents();

private:
    bool TrySendEvent(const csp::multiplayer::ConversationEventParams& Params);

    csp::systems::AssetSystem* AssetSystem;
    csp::systems::SpaceSystem* SpaceSystem;
    csp::systems::UserSystem* UserSystem;

    csp::multiplayer::EventBus* EventBus;

    std::unordered_set<csp::multiplayer::ConversationSpaceComponent*> Components;
    std::vector<csp::multiplayer::ConversationEventParams> Events;
};

}
