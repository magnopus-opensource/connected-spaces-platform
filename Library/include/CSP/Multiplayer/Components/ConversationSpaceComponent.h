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
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IRotationComponent.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/Conversation/ConversationSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"

#include <optional>

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
    Date,
    NumberOfReplies,
    Num
};

/// @ingroup ConversationSpaceComponent
/// @brief Add a conversation with comment thread to your space. These conversations have a spatial representation.
class CSP_API ConversationSpaceComponent : public ComponentBase, public IPositionComponent, public IRotationComponent
{
public:
    /// @brief Constructs the conversation component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    ConversationSpaceComponent(SpaceEntity* Parent);

    /// @brief Create a new conversation
    /// @param Message csp::common::String : the message to be stored.
    /// @param Callback StringResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateConversation(const csp::common::String& Message, csp::systems::StringResultCallback Callback);

    /// @brief Deletes all the messages that are part of the conversation
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteConversation(csp::systems::NullResultCallback Callback);

    /// @brief Adds a message to conversation
    /// Make sure that the user has entered a space through SpaceSystem::EnterSpace() before calling this.
    /// @param Message csp::common::String : the message to be stored.
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void AddMessage(const csp::common::String& Message, MessageResultCallback Callback);

    /// @brief Deletes a particular message
    /// @param MessageId csp::common::String : if of the message that will be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteMessage(const csp::common::String& MessageId, csp::systems::NullResultCallback Callback);

    /// @brief Retrieves one particular message
    /// @param MessageId csp::common::String : id of the message to be retrieved
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetMessage(const csp::common::String& MessageId, MessageResultCallback Callback);

    /// @brief Retrieves all messages in conversation
    /// @param Callback MessageCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAllMessages(MessageCollectionResultCallback Callback);

    /// @brief Get Conversation Info
    /// @param Callback ConversationResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetConversationInfo(ConversationResultCallback Callback);

    /// @brief Set Conversation Info
    /// @param ConversationData ConversationInfo : Conversation Information
    /// @param Callback ConversationResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void SetConversationInfo(const ConversationInfo& ConversationData, ConversationResultCallback Callback);

    /// @brief Get Message Info
    /// @param MessageId csp::common::String : message Id
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetMessageInfo(const csp::common::String& MessageId, MessageResultCallback Callback);

    /// @brief Set Message Info
    /// @param MessageId csp::common::String : message Id
    /// @param MessageData MessageInfo : Conversation Information
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void SetMessageInfo(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback);

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

    /// @brief Moves the conversation associated with the other component to this one and remmove the association with the other component
    /// @param OtherConversationComponent - The component to move the conversation from.
    /// @return true if successful, false if there is already a conversation associated with this component
    bool MoveConversationFromComponent(ConversationSpaceComponent& OtherConversationComponent);
    /// @brief Sets the Title of the conversation.
    /// @param Value - The new title.
    void SetTitle(const csp::common::String& Value);
    /// @brief Gets the Title of the conversation.
    const csp::common::String& GetTitle() const;
    /// @brief Sets the Date of the conversation.
    /// @param Value - The new Date.
    void SetDate(const csp::common::String& Value);
    /// @brief Gets the Date of the conversation.
    const csp::common::String& GetDate() const;
    /// @brief Sets the Number Of Replies of the conversation.
    /// @param Value - The new Number Of Replies.
    void SetNumberOfReplies(const int64_t Value);
    /// @brief Gets the Number Of Replies of the conversation.
    const int64_t GetNumberOfReplies() const;

private:
    ConversationSystem* ConversationSystem;
    void SetConversationId(const csp::common::String& Value);
    void RemoveConversationId();
    const csp::common::String& GetConversationId() const;
};

} // namespace csp::multiplayer
