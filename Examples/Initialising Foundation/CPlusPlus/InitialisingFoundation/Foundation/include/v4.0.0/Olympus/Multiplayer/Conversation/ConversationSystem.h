#pragma once

#include "Olympus/Common/Optional.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/Conversation/Conversation.h"
#include "Olympus/Multiplayer/MultiPlayerConnection.h"
#include "Olympus/Multiplayer/ReplicatedValue.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_multiplayer
{

class Space;
/// @ingroup Conversation System
/// @brief Public facing system that can handle conversations taking place between users of a space in the form of thread messages
class OLY_API OLY_NO_DISPOSE ConversationSystem
{
    /** @cond DO_NOT_DOCUMENT */
    /** @endcond */

public:
    ConversationSystem(MultiplayerConnection* InMultiPlayerConnection);

    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */
    /// @brief Create a new conversation
    /// @param Message oly_common::String : the message to be stored.
    /// @param Callback StringResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateConversation(const oly_common::String& Message, oly_systems::StringResultCallback Callback);

    /// @brief Adds a message to a brand new conversation or to an already existing one
    /// Make sure that the user has entered a space through SpaceSystem::EnterSpace() before calling this.
    /// sends a network event acknowledgement that can be listened for called "ConversationSystem" containing:
    /// {ConversationSystemParams of type ReplicatedType : ConversationMessageType::NewMessage, ConversationId of type ReplicatedType : String}
    /// @param ConversationId oly_common::String : A new message will be linked to the provided conversation id.
    /// @param SenderDisplayName oly_common::String : the display name of the message sender.
    /// @param Message oly_common::String : the message to be stored.
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddMessageToConversation(const oly_common::String& ConversationId, const oly_common::String& SenderDisplayName,
        const oly_common::String& Message, MessageResultCallback Callback);

    /// @brief Retrieves messages that are linked to the provided Conversation ID
    /// @param ConversationId oly_common::String : conversation Id
    /// @param ResultsSkipNumber int : optional param representing the number of result entries that will be skipped from the result. For no skip pass
    /// nullptr.
    /// @param ResultsMaxNumber int : optional param representing the maximum number of result entries to be retrieved. For all available result
    /// entries pass nullptr.
    /// @param Callback MessageCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetMessagesFromConversation(const oly_common::String& ConversationId, const oly_common::Optional<int>& ResultsSkipNumber,
        const oly_common::Optional<int>& ResultsMaxNumber, MessageCollectionResultCallback Callback);

    /// @brief Retrieves Conversation Information
    /// @param ConversationId oly_common::String : conversation Id
    /// @param Callback ConversationResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetConversationInformation(const oly_common::String& ConversationId, ConversationResultCallback Callback);

    /// @brief Sets Conversation Information
    /// @param ConversationId oly_common::String : conversation Id
    /// @param ConversationData ConversationInfo : Conversation Data
    /// @param Callback ConversationResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void SetConversationInformation(
        const oly_common::String& ConversationId, const ConversationInfo& ConversationData, ConversationResultCallback Callback);

    /// @brief Retrieves one particular message
    /// @param MessageId oly_common::String : id of the message to be retrieved
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetMessage(const oly_common::String& MessageId, MessageResultCallback Callback);

    /// @brief Sets Message Information
    /// @param MessageId oly_common::String : id of the message to be retrieved
    /// @param MessageData MessageInfo : message Data
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void SetMessageInformation(const oly_common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback);

    /// @brief Retrieves Conversation Information
    /// @param MessageId oly_common::String : message Id
    /// @param Callback MessageResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetMessageInformation(const oly_common::String& MessageId, MessageResultCallback Callback);

    /// @brief Deletes all the messages that are part of the conversation
    /// sends a network event acknowledgement that can be listened for called "ConversationSystem" containing:
    /// {ConversationSystemParams of type ReplicatedType : ConversationMessageType::DeleteConversation, ConversationId of type ReplicatedType :
    /// String}
    /// @param ConversationId oly_common::String : id of the conversation that will be deleted. After this operation finishes successful this Id will
    /// not be valid anymore.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteConversation(const oly_common::String& ConversationId, oly_systems::NullResultCallback Callback);

    /// @brief Deletes a particular message
    /// sends a network event acknowledgement that can be listened for called "ConversationSystem" containing:
    /// { ConversationSystemParams of type ReplicatedType : ConversationMessageType::DeleteMessage, MessageId of type ReplicatedType : String }
    /// @param MessageId oly_common::String : if of the message that will be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteMessage(const oly_common::String& MessageId, oly_systems::NullResultCallback Callback);

    ///@}
    /**
     * @brief Sets a local pointer to the connection for communication with the endpoints, this should be called as early as possible.
     * Note that this is already called in MultiplayerConnection::Connect, so this shouldn't need to be called anywhere else.
     * This should not be called by client code directly, marked as No Export.
     */
    OLY_NO_EXPORT void SetConnection(oly_multiplayer::SignalRConnection* InConnection);

private:
    void StoreConversationMessage(const oly_common::String& ConversationId, const oly_systems::Space& Space, const oly_common::String& UserId,
        const oly_common::String& SenderDisplayName, const oly_common::String& Message, MessageResultCallback Callback) const;

    void DeleteMessages(const oly_common::Array<oly_systems::AssetCollection>& Messages, oly_systems::NullResultCallback Callback);

    MultiplayerConnection* MultiPlayerConnection;
    SignalRConnection* Connection;
};

} // namespace oly_multiplayer
