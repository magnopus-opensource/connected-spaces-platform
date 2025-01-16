#pragma once
// undef GetMessage is used because it clashes with a win32 macro
#undef GetMessage
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Conversation/Conversation.h"
#include "Olympus/Multiplayer/Conversation/ConversationSystem.h"
#include "Olympus/Multiplayer/MultiPlayerConnection.h"
#include "Olympus/OlympusCommon.h"

#include <optional>

namespace oly_multiplayer
{

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
/// @brief Data representation of an ConversationSpaceComponent.
class OLY_API ConversationSpaceComponent : public ComponentBase
{
public:
    ConversationSpaceComponent(SpaceEntity* Parent);

    /// @brief Create a new conversation
    /// @param Message oly_common::String : the message to be stored.
    /// @param Callback StringResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateConversation(const oly_common::String& Message, oly_systems::StringResultCallback Callback);

    /// @brief Deletes all the messages that are part of the conversation
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteConversation(oly_systems::NullResultCallback Callback);

    /// @brief Adds a message to conversation
    /// Make sure that the user has entered a space through SpaceSystem::EnterSpace() before calling this.
    /// @param Message oly_common::String : the message to be stored.
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddMessage(const oly_common::String& Message, MessageResultCallback Callback);

    /// @brief Deletes a particular message
    /// @param MessageId oly_common::String : if of the message that will be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteMessage(const oly_common::String& MessageId, oly_systems::NullResultCallback Callback);

    /// @brief Retrieves one particular message
    /// @param MessageId oly_common::String : id of the message to be retrieved
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetMessage(const oly_common::String& MessageId, MessageResultCallback Callback);

    /// @brief Retrieves all messages in conversation
    /// @param Callback MessageCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAllMessages(MessageCollectionResultCallback Callback);

    /// @brief Get Conversation Info
    /// @param Callback ConversationResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetConversationInfo(ConversationResultCallback Callback);

    /// @brief Set Conversation Info
    /// @param ConversationData ConversationInfo : Conversation Information
    /// @param Callback ConversationResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void SetConversationInfo(const ConversationInfo& ConversationData, ConversationResultCallback Callback);

    /// @brief Get Message Info
    /// @param MessageId oly_common::String : message Id
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetMessageInfo(const oly_common::String& MessageId, MessageResultCallback Callback);

    /// @brief Set Message Info
    /// @param MessageId oly_common::String : message Id
    /// @param MessageData MessageInfo : Conversation Information
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void SetMessageInfo(const oly_common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback);

    /// @brief Gets the relative 3D position of this component.
    const oly_common::Vector3& GetPosition() const;
    /// @brief Sets the relative 3D position of this component.
    /// @param Value - The new 3D position assigned to the origin of this component.
    void SetPosition(const oly_common::Vector3& Value);

    /// @brief Gets the quaternion of the rotation of the origin of this component.
    const oly_common::Vector4& GetRotation() const;
    /// @brief Sets the quaternion of the rotation of the origin of this component.
    /// @param Value - The new rotation assigned to the origin of this component.
    void SetRotation(const oly_common::Vector4& Value);

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
    void SetTitle(const oly_common::String& Value);
    /// @brief Gets the Title of the conversation.
    const oly_common::String& GetTitle() const;
    /// @brief Sets the Date of the conversation.
    /// @param Value - The new Date.
    void SetDate(const oly_common::String& Value);
    /// @brief Gets the Date of the conversation.
    const oly_common::String& GetDate() const;
    /// @brief Sets the Number Of Replies of the conversation.
    /// @param Value - The new Number Of Replies.
    void SetNumberOfReplies(const int64_t Value);
    /// @brief Gets the Number Of Replies of the conversation.
    const int64_t GetNumberOfReplies() const;

private:
    ConversationSystem* ConversationSystem;
    void SetConversationId(const oly_common::String& Value);
    void RemoveConversationId();
    const oly_common::String& GetConversationId() const;
};

} // namespace oly_multiplayer
