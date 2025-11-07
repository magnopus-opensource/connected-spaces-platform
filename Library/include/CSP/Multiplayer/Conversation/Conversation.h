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
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{
class ConversationSystemInternal;
}

namespace csp::multiplayer
{

class AssetCollection;

/// @ingroup Conversation
/// @brief Contains information about a conversation message.
class CSP_API MessageInfo
{
public:
    /// @brief The id of the conversation.
    csp::common::String ConversationId;

    /// @brief The time the message was created
    csp::common::String CreatedTimestamp;

    /// @brief The time the message was last edited
    csp::common::String EditedTimestamp;

    /// @brief The user id that triggered the event
    csp::common::String UserId;

    /// @brief The message contents
    csp::common::String Message;

    /// @brief The unique identifier of the message
    csp::common::String MessageId;

    MessageInfo();
    MessageInfo(const csp::common::String& ConversationId, bool IsConversation, const csp::common::String& Message);
    MessageInfo(
        const csp::common::String& ConversationId, bool IsConversation, const csp::common::String& Message, const csp::common::String& MessageId);

    MessageInfo(const MessageInfo& MessageData);
    MessageInfo& operator=(const MessageInfo& MessageData);
};

/// @ingroup Conversation
/// @brief Information used to update a message
class CSP_API MessageUpdateParams
{
public:
    /// @brief The contents of the new message
    csp::common::String NewMessage;
};

/// @ingroup Conversation
/// @brief Information used to update an annotation
class CSP_API AnnotationUpdateParams
{
public:
    /// @brief The vertical fov of the camera when the annotation is created
    double VerticalFov;
    /// @brief The position of the camera when the annotation is created
    csp::common::Vector3 AuthorCameraPosition;
    /// @brief The rotation of the camera when the annotation is created
    csp::common::Vector4 AuthorCameraRotation;
};

/// @ingroup Conversation
/// @brief Data for an Annotation, used to help display the annotation in a consistent way to all end users.
class CSP_API AnnotationData
{
public:
    AnnotationData();
    AnnotationData(const csp::common::String& AnnotationId, const csp::common::String& AnnotationThumbnailId, double InVerticalFov,
        const csp::common::Vector3& InAuthorCameraPosition, const csp::common::Vector4& InAuthorCameraRotation);
    AnnotationData(const AnnotationData& InAnnotationData);
    AnnotationData& operator=(const AnnotationData& InAnnotationData);

    csp::common::String AnnotationId;
    csp::common::String AnnotationThumbnailId;
    double VerticalFov;
    csp::common::Vector3 AuthorCameraPosition;
    csp::common::Vector4 AuthorCameraRotation;
};

/// @brief Enum used to specify the type of a conversation system network event.
enum class ConversationEventType
{
    NewConversation,
    NewMessage,
    DeleteMessage,
    DeleteConversation,
    ConversationInformation,
    MessageInformation,
    SetAnnotation,
    DeleteAnnotation,
    SetConversationAnnotation,
    DeleteConversationAnnotation
};

/// @ingroup Conversation
/// @brief Data class used to contain information when a message is being retrieved
class CSP_API MessageResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;
    friend class ConversationSpaceComponent;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the message info object from this result.
    /// @return The message info.
    [[nodiscard]] MessageInfo& GetMessageInfo();

    /// @brief Gets the message info object from this result.
    /// @return The message info.
    [[nodiscard]] const MessageInfo& GetMessageInfo() const;

    CSP_NO_EXPORT MessageResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    explicit MessageResult(void*) {};
    MessageResult() = default;

    void FillMessageInfo(const csp::systems::AssetCollection& MessageAssetCollection);

    MessageInfo MsgInfo;
};

/// @ingroup Conversation
/// @brief Data class used to contain information when retrieving a collection of messages
class CSP_API MessageCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;
    friend class ConversationSpaceComponent;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the list of messages, as message info objects, from this result.
    /// @return Array of message info objects.
    [[nodiscard]] csp::common::Array<MessageInfo>& GetMessages();

    /// @brief Gets the list of messages, as message info objects, from this result.
    /// @return Array of message info objects.
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

/// @ingroup Conversation
/// @brief Data class used to contain information when retrieving a conversation.
class CSP_API ConversationResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the message info object from this result.
    /// @return The MessageInfo object representing conversation info.
    [[nodiscard]] MessageInfo& GetConversationInfo();

    /// @brief Gets the message info object from this result.
    /// @return The MessageInfo object representing conversation info.
    [[nodiscard]] const MessageInfo& GetConversationInfo() const;

    CSP_NO_EXPORT ConversationResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    explicit ConversationResult(void*) {};
    ConversationResult() = default;

    void FillConversationInfo(const csp::systems::AssetCollection& ConversationAssetCollection);

    MessageInfo ConvoInfo;
};

/// @ingroup Conversation
/// @brief Data class used to contain information for GetNumberOfReplies.
class CSP_API NumberOfRepliesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Gets the number of replies from the result
    /// @return : The number of replies
    uint64_t GetCount() const;

    CSP_NO_EXPORT NumberOfRepliesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    explicit NumberOfRepliesResult(void*) {};
    NumberOfRepliesResult() = default;

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    CSP_NO_EXPORT NumberOfRepliesResult(const csp::systems::ResultBase& InResult)
        : csp::systems::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    uint64_t Count;
};

class CSP_API AnnotationResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    CSP_NO_EXPORT AnnotationResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    CSP_NO_EXPORT AnnotationResult(
        csp::systems::EResultCode ResCode, csp::web::EResponseCodes HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, static_cast<std::underlying_type<csp::web::EResponseCodes>::type>(HttpResCode), Reason) {};

    CSP_NO_EXPORT void ParseAnnotationAssetData(const csp::systems::AssetCollection& AssetCollection);
    CSP_NO_EXPORT void SetAnnotationAsset(const csp::systems::Asset& Asset) { AnnotationAsset = Asset; }
    CSP_NO_EXPORT void SetAnnotationThumbnailAsset(const csp::systems::Asset& Asset) { AnnotationThumbnailAsset = Asset; }

    /// @brief Gets the information about the annotation.
    /// @return const AnnotationData&
    const AnnotationData& GetAnnotationData() const;

    /// @brief Gets the asset containing the annotation data.
    /// @return const csp::systems::Asset&
    const csp::systems::Asset& GetAnnotationAsset() const;

    /// @brief Gets the asset containing the annotation thumbnail data.
    /// @return const csp::systems::Asset&
    const csp::systems::Asset& GetAnnotationThumbnailAsset() const;

private:
    explicit AnnotationResult(void*) {};
    AnnotationResult() = default;

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    CSP_NO_EXPORT AnnotationResult(const csp::systems::ResultBase& InResult)
        : csp::systems::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    AnnotationData Data;
    systems::Asset AnnotationAsset;
    systems::Asset AnnotationThumbnailAsset;
};

class CSP_API AnnotationThumbnailCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::ConversationSystemInternal;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    CSP_NO_EXPORT AnnotationThumbnailCollectionResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    CSP_NO_EXPORT AnnotationThumbnailCollectionResult(
        csp::systems::EResultCode ResCode, csp::web::EResponseCodes HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, static_cast<std::underlying_type<csp::web::EResponseCodes>::type>(HttpResCode), Reason) {};

    /// @brief gets the annotation thumbnails that exist within the conversation.
    /// @return const csp::common::Map<csp::common::String, csp::systems::Asset>&
    const csp::common::Map<csp::common::String, csp::systems::Asset>& GetAnnotationThumbnailAssetsMap() const;

    /// @brief Gets the number of asset thumbnails returned from GetAnnotationThumbnailAssetsMap.
    /// @return uint64_t
    uint64_t GetTotalCount() const;

    CSP_NO_EXPORT void ParseAssets(const systems::AssetsResult& Result);

private:
    explicit AnnotationThumbnailCollectionResult(void*) {};
    AnnotationThumbnailCollectionResult() = default;

    csp::common::Map<csp::common::String, csp::systems::Asset> AnnotationThumbnailAssetsMap;
};

/// @brief Callback containing number of replies.
/// @param Result NumberOfRepliesResult : result class
typedef std::function<void(const NumberOfRepliesResult& Result)> NumberOfRepliesResultCallback;

// callback signatures
// Callback providing a result object with one message info object.
typedef std::function<void(const MessageResult& Result)> MessageResultCallback;

// Callback providing a result object with a collection of message info objects.
typedef std::function<void(const MessageCollectionResult& Result)> MessageCollectionResultCallback;

// Callback providing a result object with a message info object representing the conversation.
typedef std::function<void(const ConversationResult& Result)> ConversationResultCallback;

// Callback providing a result object with a annotations result object representing the conversation.
typedef std::function<void(const AnnotationResult& Result)> AnnotationResultCallback;

// Callback providing a result object with a annotations thumbnail collection result object .
typedef std::function<void(const AnnotationThumbnailCollectionResult& Result)> AnnotationThumbnailCollectionResultCallback;

} // namespace csp::multiplayer
