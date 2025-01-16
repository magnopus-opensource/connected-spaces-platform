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
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::multiplayer
{

class AssetCollection;

/// @ingroup Conversation System
/// @brief Data representation of fields shared by MessageInfo and CovnersationInfo.
class CSP_API BaseMessageInfo
{
public:
    csp::common::String ConversationId;
    csp::common::String Timestamp;
    csp::common::String UserID;
    csp::common::String UserDisplayName;
    csp::common::String Message;
    bool Edited;
};

/// @ingroup Conversation System
/// @brief Data representation of a message.
class CSP_API MessageInfo : public BaseMessageInfo
{
public:
    csp::common::String Id;
    explicit MessageInfo(void*) {};
    MessageInfo() = default;
    MessageInfo(const MessageInfo& MessageData);
};

/// @brief Data representation of a conversation.
class CSP_API ConversationInfo : public BaseMessageInfo
{
public:
    bool Resolved;
    SpaceTransform CameraPosition;
    explicit ConversationInfo(void*) {};
    ConversationInfo() = default;
    ConversationInfo(const ConversationInfo& ConversationData);
};

/// @brief Enum used to specify the type of a conversation system network event.
enum class ConversationMessageType
{
    NewMessage,
    DeleteMessage,
    DeleteConversation,
    ConversationInformation,
    MessageInformation
};

/// @ingroup Conversation System
/// @brief Data class used to contain information when a message is being retrieved
class CSP_API MessageResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;
    friend class ConversationSpaceComponent;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the message info object from this result.
    /// @retrun The message info.
    [[nodiscard]] MessageInfo& GetMessageInfo();

    /// @brief Gets the message info object from this result.
    /// @retrun The message info.
    [[nodiscard]] const MessageInfo& GetMessageInfo() const;

    CSP_NO_EXPORT MessageResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    explicit MessageResult(void*) {};
    MessageResult() = default;

    void FillMessageInfo(const csp::systems::AssetCollection& MessageAssetCollection);

    MessageInfo MsgInfo;
};

/// @ingroup Conversation System
/// @brief Data class used to contain information when retrieving a collection of messages
class CSP_API MessageCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the list of messages, as message info objects, from this result.
    /// @retrun Array of message info objects.
    [[nodiscard]] csp::common::Array<MessageInfo>& GetMessages();

    /// @brief Gets the list of messages, as message info objects, from this result.
    /// @retrun Array of message info objects.
    [[nodiscard]] const csp::common::Array<MessageInfo>& GetMessages() const;

    /// @brief Retrieves the total number of messages in the conversation.
    ///
    /// If the async operation was using pagination this count number represents the sum of how many messages exist in all pages.
    /// If the async operation is not using pagination this count number will be equal to the ConversationMessages array size.
    ///
    /// @return uint64_t : count number as described above.
    [[nodiscard]] uint64_t GetTotalCount() const;

    /// @brief Sets the value returned by `GetTotalCount()`
    CSP_NO_EXPORT void SetTotalCount(uint64_t Value);

    CSP_NO_EXPORT MessageCollectionResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    explicit MessageCollectionResult(void*) {};
    explicit MessageCollectionResult(uint64_t ResultTotalCount)
        : ResultTotalCount(ResultTotalCount) {};

    void FillMessageInfoCollection(const csp::common::Array<csp::systems::AssetCollection>& MessagesAssetCollections);

    csp::common::Array<MessageInfo> ConversationMessages;
    uint64_t ResultTotalCount = 0;
};

/// @ingroup Conversation System
/// @brief Data class used to contain information when retrieving a conversation.
class CSP_API ConversationResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the conversation info object from this result.
    /// @retrun The conversation info.
    [[nodiscard]] ConversationInfo& GetConversationInfo();

    /// @brief Gets the conversation info object from this result.
    /// @retrun The conversation info.
    [[nodiscard]] const ConversationInfo& GetConversationInfo() const;

    CSP_NO_EXPORT ConversationResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    explicit ConversationResult(void*) {};
    ConversationResult() = default;

    void FillConversationInfo(const csp::systems::AssetCollection& ConversationAssetCollection);

    ConversationInfo ConvoInfo;
};

// callback signatures
// Callback providing a result object with one message info object.
typedef std::function<void(const MessageResult& Result)> MessageResultCallback;

// Callback providing a result object with a collection of message info objects.
typedef std::function<void(const MessageCollectionResult& Result)> MessageCollectionResultCallback;

// Callback providing a result object with a conversation info object.
typedef std::function<void(const ConversationResult& Result)> ConversationResultCallback;

} // namespace csp::multiplayer
