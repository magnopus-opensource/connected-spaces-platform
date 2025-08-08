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

/// @file ConversationSpaceComponent.h
/// @brief Definitions and support for conversation components.

#pragma once

// undef GetMessage is used because it clashes with a win32 macro
#undef GetMessage

#include "CSP/CSPCommon.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IRotationComponent.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"

CSP_START_IGNORE
#ifdef CSP_TESTS
class CSPEngine_ConversationTests_ConversationComponentPropertyTest_Test;
class CSPEngine_ConversationTests_ConversationComponentEventTest_Test;
class CSPEngine_ConversationTests_ConversationComponentAnnotationEventTest_Test;
class CSPEngine_ConversationSystemTests_ConversationSystemEventTest_Test;
class CSPEngine_ConversationSystemTests_ConversationSystemEventDelayTest_Test;
#endif
CSP_END_IGNORE

namespace csp::systems
{
class ConversationSystemInternal;
}

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a conversation component.
enum class ConversationPropertyKeys
{
    ConversationId = 0,
    IsVisible,
    IsActive,
    Position,
    Rotation,
    Title,
    Date_DEPRECATED,
    NumberOfReplies_DEPRECATED,
    Resolved,
    ConversationCameraPosition,
    ConversationCameraRotation,
    Num
};

/// @ingroup ConversationSpaceComponent
/// @brief Add a conversation with comment thread to your space. These conversations have a spatial representation.
class CSP_API ConversationSpaceComponent : public ComponentBase, public IPositionComponent, public IRotationComponent
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;

#ifdef CSP_TESTS
    friend class ::CSPEngine_ConversationTests_ConversationComponentPropertyTest_Test;
    friend class ::CSPEngine_ConversationTests_ConversationComponentEventTest_Test;
    friend class ::CSPEngine_ConversationTests_ConversationComponentAnnotationEventTest_Test;
    friend class ::CSPEngine_ConversationSystemTests_ConversationSystemEventTest_Test;
    friend class ::CSPEngine_ConversationSystemTests_ConversationSystemEventDelayTest_Test;
#endif
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Constructs the conversation component, and associates it with the specified Parent space entity.
    /// This constructor should not be called directly. Instead, use the SpaceEntity::AddComponent function.
    /// @param Parent csp::multiplayer::SpaceEntity* : The Space entity that owns this component. This will also register the component to the entity.
    /// @pre Parent must not be null.
    ConversationSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    /// @brief Creates a conversation represented by this component.
    /// @param Message const csp::common::String& : The message to be stored.
    /// @param Callback csp::systems::StringResultCallback : Callback when asynchronous task finishes.
    /// @pre The conversation must not already exist (component must not have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    /// @post The ConversationId property is now internally set when the callback is fired.
    /// This component should now be replicated, so other clients receive the update.
    CSP_ASYNC_RESULT void CreateConversation(const csp::common::String& Message, csp::systems::StringResultCallback Callback);

    /// @brief Deletes this conversation, including all of its messages. This function is called internally when the component is deleted.
    /// @param Callback csp::systems::NullResultCallback : callback when asynchronous task finishes
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void DeleteConversation(csp::systems::NullResultCallback Callback);

    /// @brief Adds a message to the conversation.
    /// @param Message const csp::common::String& : The message to be stored.
    /// @param Callback MessageResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void AddMessage(const csp::common::String& Message, MessageResultCallback Callback);

    /// @brief Deletes a particular message.
    /// @param MessageId const csp::common::String& : Id of the message to delete.
    /// The id can be retreived from the result callback when the message was added.
    /// @param Callback csp::systems::NullResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    /// @pre Client should be logged in with the same user account that created the message (ClientId of the message should match the current logged
    /// in user).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void DeleteMessage(const csp::common::String& MessageId, csp::systems::NullResultCallback Callback);

    /// @brief Retrieves message details that are represented by this component.
    /// This doesn't include the original message that created the conversation.
    /// @param ResultsSkipNumber const csp::common::Optional<int>& : Optional parameter representing the number of result entries that will be skipped
    /// from the result. For no skip pass an empty optional.
    /// @param ResultsMaxNumber const csp::common::Optional<int>& : Optional parameter representing the maximum number of result entries to be
    /// retrieved. For all available result entries pass an empty optional.
    /// @param Callback csp::multiplayer::MessageCollectionResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetMessagesFromConversation(const csp::common::Optional<int>& ResultsSkipNumber,
        const csp::common::Optional<int>& ResultsMaxNumber, MessageCollectionResultCallback Callback);

    /// @brief Retrieves message details for the root message in the conversation.
    /// @param Callback csp::multiplayer::ConversationResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetConversationInfo(ConversationResultCallback Callback);

    /// @brief Updates information for the root message in the conversation.
    /// @param NewData const csp::multiplayer::MessageUpdateParams& : The information to update the root message with.
    /// @param Callback csp::multiplayer::ConversationResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    /// @pre Client should be logged in with the same user account that created the message (ClientId of the message should match the current logged
    /// in user).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void UpdateConversation(const MessageUpdateParams& NewData, ConversationResultCallback Callback);

    /// @brief Retrieves message details for a specified message in this conversation.
    /// @param MessageId const csp::common::String& : The message id to retrieve information for.
    /// The id can be retreived from the result callback when the message was added.
    /// @param Callback csp::multiplayer::MessageResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetMessageInfo(const csp::common::String& MessageId, MessageResultCallback Callback);

    /// @brief Updates information for a specified message in the conversation.
    /// @param MessageId const csp::common::String& : The message id to update information for.
    /// The id can be retreived from the result callback when the message was added.
    /// @param NewData const csp::multiplayer::MessageUpdateParams& : The information to update the specified message with.
    /// @param Callback csp::multiplayer::MessageResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    /// @pre Client should be logged in with the same user account that created the message (ClientId of the message should match the current logged
    /// in user).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void UpdateMessage(const csp::common::String& MessageId, const MessageUpdateParams& NewData, MessageResultCallback Callback);

    /// @brief Gets the Number Of Replies in the conversation.
    /// @param Callback csp::multiplayer::NumberOfRepliesResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetNumberOfReplies(NumberOfRepliesResultCallback Callback);

    /// @brief Gets the annotation for the root message in the conversation.
    /// @param Callback csp::multiplayer::NumberOfRepliesResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    /// @pre The conversation must have an annotation previously attached using SetConversationAnnotation.
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetConversationAnnotation(AnnotationResultCallback Callback);

    /// @brief Associates an annotation with the root message in the conversation.
    /// If an annotation already exists on the conversation, it will be overwritten.
    /// @param UpdateParams const AnnotationUpdateParams& : The annotation data for this annotation.
    /// @param Annotation const systems::BufferAssetDataSource& : The annotation image data for this annotation.
    /// @param AnnotationThumbnail const systems::BufferAssetDataSource& : The annotation thumbnail image data for this annotation.
    /// @param Callback csp::multiplayer::AnnotationResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void SetConversationAnnotation(const AnnotationUpdateParams& AnnotationParams,
        const csp::systems::BufferAssetDataSource& Annotation, const csp::systems::BufferAssetDataSource& AnnotationThumbnail,
        AnnotationResultCallback Callback);

    /// @brief Deletes an annotation associated with the root message in the conversation.
    /// @param Callback csp::systems::NullResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response. if this condition is not met,
    /// with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void DeleteConversationAnnotation(csp::systems::NullResultCallback Callback);

    /// @brief Gets an annotation associated with a message.
    /// @param MessageId const csp::common::String& : The message id the annotation is associated with.
    /// @param Callback csp::multiplayer::AnnotationResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    /// @pre The message must have an annotation previously attached using SetAnnotation.
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetAnnotation(const csp::common::String& MessageId, AnnotationResultCallback Callback);

    /// @brief Associates an annotation with a message.
    /// If an annotation already exists on the message, it will be overwritten.
    /// @param MessageId const csp::common::String& : The message id to attach an annotation to.
    /// @param UpdateParams const AnnotationUpdateParams& : The annotation data for this annotation.
    /// @param Annotation const systems::BufferAssetDataSource& : The annotation image data for this annotation.
    /// @param AnnotationThumbnail const systems::BufferAssetDataSource& : The annotation thumbnail image data for this annotation.
    /// @param Callback csp::multiplayer::AnnotationResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void SetAnnotation(const csp::common::String& MessageId, const AnnotationUpdateParams& UpdateParams,
        const csp::systems::BufferAssetDataSource& Annotation, const csp::systems::BufferAssetDataSource& AnnotationThumbnail,
        AnnotationResultCallback Callback);

    /// @brief Deletes an annotation associated with a message.
    /// @param MessageId const csp::common::String& : The message id whose annotation to delete.
    /// @param Callback csp::systems::NullResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void DeleteAnnotation(const csp::common::String& MessageId, csp::systems::NullResultCallback Callback);

    /// @brief Gets all thumbnails in the conversation.
    /// @param Callback csp::multiplayer::AnnotationThumbnailCollectionResultCallback : Callback when asynchronous task finishes.
    /// @pre This component must contain a valid conversation id (component must have a conversation id that isn't an empty string).
    /// A CSP error will be sent to the LogSystem if this condition is not met, with a EResultCode::Failed response.
    CSP_ASYNC_RESULT void GetAnnotationThumbnailsForConversation(AnnotationThumbnailCollectionResultCallback Callback);

    typedef std::function<void(const csp::common::ConversationNetworkEventData&)> ConversationUpdateCallbackHandler;

    /// @brief Sets a callback for when the conversation is updated by another client.
    /// @param Callback ConversationUpdateCallbackHandler: Callback to receive data for the conversation that has been changed.
    CSP_EVENT void SetConversationUpdateCallback(ConversationUpdateCallbackHandler Callback);

    /// \addtogroup IPositionComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @}

    /// \addtogroup IRotationComponent
    /// @{
    /// @copydoc IRotationComponent::GetRotation()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IRotationComponent::SetRotation()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @}

    bool GetIsVisible() const;
    void SetIsVisible(bool Value);
    bool GetIsActive() const;
    void SetIsActive(bool Value);

    /// @brief Sets the Title of the conversation.
    /// @param Value - The new title.
    void SetTitle(const csp::common::String& Value);
    /// @brief Gets the Title of the conversation.
    const csp::common::String& GetTitle() const;

    /// @brief Sets the resolved value for indicating that a conversation is resolved.
    /// @param Value - The resolved state.
    void SetResolved(bool Value);
    /// @brief Gets the resolved value of the conversation.
    bool GetResolved() const;

    /// @brief Sets the value for the camera position used to view the conversation.
    /// @param InValue const csp::common::Vector3& : The position for the camera.
    void SetConversationCameraPosition(const csp::common::Vector3& InValue);
    /// @brief Gets the value for the camera position of the conversation.
    /// @return const csp::common::Vector3& : The camera view position.
    const csp::common::Vector3& GetConversationCameraPosition() const;

    /// @brief Sets the value for the camera rotation used to view the conversation.
    /// @param InValue const csp::common::Vector4& : The rotation for the camera.
    void SetConversationCameraRotation(const csp::common::Vector4& InValue);
    /// @brief Gets the value for the camera rotation of the conversation.
    /// @return const csp::common::Vector4& : The camera view rotation.
    const csp::common::Vector4& GetConversationCameraRotation() const;

    CSP_NO_EXPORT void OnLocalDelete() override;

protected:
    void OnCreated() override;
    void OnRemove() override;

    void SetPropertyFromPatch(uint32_t Key, const csp::common::ReplicatedValue& Value) override;

private:
    void SetConversationId(const csp::common::String& Value);
    void RemoveConversationId();
    const csp::common::String& GetConversationId() const;

    ConversationUpdateCallbackHandler ConversationUpdateCallback;
};

} // namespace csp::multiplayer
