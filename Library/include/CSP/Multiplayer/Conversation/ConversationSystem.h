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
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::multiplayer
{

/// @ingroup Conversation System
/// @brief Public facing system that can handle conversations taking place between users of a space in the form of thread messages.
class CSP_API CSP_NO_DISPOSE ConversationSystem : public csp::systems::SystemBase
{
public:
    /// @brief Constructs a conversation system instance that uses the given multiplayer connection.
    /// @param InMultiPlayerConnection MultiplayerConnection : The connection to be used.
    ConversationSystem(MultiplayerConnection* InMultiPlayerConnection);

    ~ConversationSystem();

    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */
    /// @brief Creates a new conversation with the initial message and provides the conversation ID to the given callback.
    /// @param Message csp::common::String : The message to be stored.
    /// @param Callback StringResultCallback : Callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateConversation(const csp::common::String& Message, csp::systems::StringResultCallback Callback);

    /// @brief Adds a message to a brand new conversation or to an already existing one
    ///
    /// Make sure that the user has entered a space through SpaceSystem::EnterSpace() before calling this.
    /// Sends a network event acknowledgement that can be listened for called "ConversationSystem" containing:
    /// {ConversationSystemParams of type ReplicatedType : ConversationMessageType::NewMessage, ConversationId of type ReplicatedType : String}.
    ///
    /// @param ConversationId csp::common::String : A new message will be linked to the provided conversation id.
    /// @param SenderDisplayName csp::common::String : The display name of the message sender.
    /// @param Message csp::common::String : The message to be stored.
    /// @param Callback MessageResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void AddMessageToConversation(const csp::common::String& ConversationId, const csp::common::String& SenderDisplayName,
        const csp::common::String& Message, MessageResultCallback Callback);

    /// @brief Retrieves messages that are linked to the provided Conversation ID.
    /// @param ConversationId csp::common::String : Conversation ID.
    /// @param ResultsSkipNumber Optional<int> : Optional parameter representing the number of result entries that will be skipped from the result.
    /// For no skip pass nullptr.
    /// @param ResultsMaxNumber Optional<int> : Optional parameter representing the maximum number of result entries to be retrieved. For all
    /// available result entries pass nullptr.
    /// @param Callback MessageCollectionResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMessagesFromConversation(const csp::common::String& ConversationId, const csp::common::Optional<int>& ResultsSkipNumber,
        const csp::common::Optional<int>& ResultsMaxNumber, MessageCollectionResultCallback Callback);

    /// @brief Retrieves the conversation information.
    /// @param ConversationId csp::common::String : Conversation ID.
    /// @param Callback ConversationResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetConversationInformation(const csp::common::String& ConversationId, ConversationResultCallback Callback);

    /// @brief Sets the conversation information.
    /// @param ConversationId csp::common::String : Conversation ID.
    /// @param ConversationData ConversationInfo : Conversation Data.
    /// @param Callback ConversationResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void SetConversationInformation(
        const csp::common::String& ConversationId, const ConversationInfo& ConversationData, ConversationResultCallback Callback);

    /// @brief Retrieves one particular message.
    /// @param MessageId csp::common::String : ID of the message to be retrieved.
    /// @param Callback MessageResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMessage(const csp::common::String& MessageId, MessageResultCallback Callback);

    /// @brief Sets the message information.
    /// @param MessageId csp::common::String : ID of the message to be retrieved.
    /// @param MessageData MessageInfo : Message Data.
    /// @param Callback MessageResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void SetMessageInformation(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback);

    /// @brief Retrieves the message information.
    /// @param MessageId csp::common::String : Message ID.
    /// @param Callback MessageResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMessageInformation(const csp::common::String& MessageId, MessageResultCallback Callback);

    /// @brief Deletes all the messages that are part of the conversation.
    ///
    /// Sends a network event acknowledgement that can be listened for called "ConversationSystem" containing:
    /// {ConversationSystemParams of type ReplicatedType : ConversationMessageType::DeleteConversation, ConversationId of type ReplicatedType :
    /// String}.
    ///
    /// @param ConversationId csp::common::String : ID of the conversation that will be deleted. After this operation finishes successful this ID will
    /// not be valid anymore.
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void DeleteConversation(const csp::common::String& ConversationId, csp::systems::NullResultCallback Callback);

    /// @brief Deletes a particular message.
    ///
    /// Sends a network event acknowledgement that can be listened for called "ConversationSystem" containing:
    /// { ConversationSystemParams of type ReplicatedType : ConversationMessageType::DeleteMessage, MessageId of type ReplicatedType : String }.
    ///
    /// @param MessageId csp::common::String : ID of the message that will be deleted.
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void DeleteMessage(const csp::common::String& MessageId, csp::systems::NullResultCallback Callback);

    ///@}

    /// @brief Sets a local pointer to the connection for communication with the endpoints, this should be called as early as possible.
    ///
    /// Note that this is already called in MultiplayerConnection::Connect, so this shouldn't need to be called anywhere else.
    /// This should not be called by client code directly, marked as No Export.
    ///
    /// @param InConnection csp::multiplayer::SignalRConnection : The connection to be used by the conversation system.
    CSP_NO_EXPORT void SetConnection(csp::multiplayer::SignalRConnection* InConnection);

    // Callback to receive ConversationSystem Data when a message is sent.
    typedef std::function<void(const csp::multiplayer::ConversationSystemParams&)> ConversationSystemCallbackHandler;

    /// @brief Sets a callback for a conversation new message event.
    /// @param Callback ConversationMessageCallbackHandler: Callback to receive ConversationSystem Data when a message is sent.
    /// Callback will have to reset the callback passed to the system to avoid "dangling objects" after use.
    CSP_EVENT void SetConversationSystemCallback(ConversationSystemCallbackHandler Callback);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;
    /// @brief Deregisters the system from listening for the named event.
    void DeregisterSystemCallback() override;
    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT void OnEvent(const std::vector<signalr::value>& EventValues) override;

private:
    void StoreConversationMessage(const csp::common::String& ConversationId, const csp::systems::Space& Space, const csp::common::String& UserId,
        const csp::common::String& SenderDisplayName, const csp::common::String& Message, MessageResultCallback Callback) const;

    void DeleteMessages(const csp::common::Array<csp::systems::AssetCollection>& Messages, csp::systems::NullResultCallback Callback);

    SignalRConnection* Connection;

    ConversationSystemCallbackHandler ConversationSystemCallback;
};

} // namespace csp::multiplayer
