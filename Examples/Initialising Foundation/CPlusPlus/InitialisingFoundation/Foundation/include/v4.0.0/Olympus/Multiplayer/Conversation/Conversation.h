#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/SpaceTransform.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Assets/AssetCollection.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_multiplayer
{

class AssetCollection;

/// @ingroup Conversation System
/// @brief Data representation of fields shared by MessageInfo and CovnersationInfo
class OLY_API BaseMessageInfo
{
public:
    oly_common::String ConversationId;
    oly_common::String Timestamp;
    oly_common::String UserID;
    oly_common::String UserDisplayName;
    oly_common::String Message;
    bool Edited;
};

/// @ingroup Conversation System
/// @brief Data representation of a message
class OLY_API MessageInfo : public BaseMessageInfo
{
public:
    oly_common::String Id;
    explicit MessageInfo(void*) {};
    MessageInfo() = default;
    MessageInfo(const MessageInfo& MessageData);
};

class OLY_API ConversationInfo : public BaseMessageInfo
{
public:
    bool Resolved;
    SpaceTransform CameraPosition;
    explicit ConversationInfo(void*) {};
    ConversationInfo() = default;
    ConversationInfo(const ConversationInfo& ConversationData);
};

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
class OLY_API MessageResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;
    friend class ConversationSpaceComponent;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] MessageInfo& GetMessageInfo();
    [[nodiscard]] const MessageInfo& GetMessageInfo() const;

    /// @brief Creates an invalid MessageResult instance that can be used to notify the user of an error.
    /// @return MessageResult : invalid MessageResult instance
    OLY_NO_EXPORT static MessageResult Invalid();

private:
    explicit MessageResult(void*) {};
    MessageResult() = default;
    MessageResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void FillMessageInfo(const oly_systems::AssetCollection& MessageAssetCollection);

    MessageInfo MsgInfo;
};

/// @ingroup Conversation System
/// @brief Data class used to contain information when retrieving a collection of messages
class OLY_API MessageCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] oly_common::Array<MessageInfo>& GetMessages();
    [[nodiscard]] const oly_common::Array<MessageInfo>& GetMessages() const;

    /// @brief Creates an invalid MessageCollectionResult instance that can be used to notify the user of an error.
    /// @return MessageCollectionResult : invalid MessageCollectionResult instance
    OLY_NO_EXPORT static MessageCollectionResult Invalid();

    /// @brief Retrieves the async operation total number of result messages.
    /// If the async operation was using pagination this count number represents the sum of how many messages exist in all pages.
    /// If the async operation is not using pagination this count number will be equal to the ConversationMessages array size.
    /// @return uint64_t : count number as described above
    [[nodiscard]] uint64_t GetTotalCount() const;

private:
    explicit MessageCollectionResult(void*) {};
    explicit MessageCollectionResult(uint64_t ResultTotalCount)
        : ResultTotalCount(ResultTotalCount) {};
    MessageCollectionResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void FillMessageInfoCollection(const oly_common::Array<oly_systems::AssetCollection>& MessagesAssetCollections);

    oly_common::Array<MessageInfo> ConversationMessages;
    uint64_t ResultTotalCount = 0;
};

class OLY_API ConversationResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] ConversationInfo& GetConversationInfo();
    [[nodiscard]] const ConversationInfo& GetConversationInfo() const;

    /// @brief Creates an invalid ConversationResult instance that can be used to notify the user of an error.
    /// @return ConversationResult : invalid ConversationResult instance
    OLY_NO_EXPORT static ConversationResult Invalid();

private:
    explicit ConversationResult(void*) {};
    ConversationResult() = default;
    ConversationResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void FillConversationInfo(const oly_systems::AssetCollection& ConversationAssetCollection);

    ConversationInfo ConvoInfo;
};

// callback signatures
typedef std::function<void(const MessageResult& Result)> MessageResultCallback;
typedef std::function<void(const MessageCollectionResult& Result)> MessageCollectionResultCallback;
typedef std::function<void(const ConversationResult& Result)> ConversationResultCallback;

} // namespace oly_multiplayer
