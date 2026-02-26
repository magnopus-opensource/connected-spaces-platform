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

#include "CSP/Common/ReplicatedValue.h"

// These will be broken in OF-1704, and the need for them (the inherited types) will dissapear, as that deserialisation logic will be done in the
// individual systems. Can't be done until the actual deserialisation functions don't take signalr values as inputs.
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Spaces/UserRoles.h"

namespace csp::common
{

/// @brief Enum specifying the type of change that occured to an asset.
enum class EAssetChangeType
{
    Created,
    Updated,
    MusubiFailed,
    Deleted,
    Invalid,
    Num
};

/// @brief Enum specifying the type of change that occured to a user's permissions whilst in a space.
enum class EPermissionChangeType
{
    Created,
    Updated,
    Removed,
    Invalid,
};

/// @brief Data deserialized from a general purpose event. Serves as the base type for all custom deserialized events.
class CSP_API NetworkEventData
{
public:
    virtual ~NetworkEventData() { }

    /// @brief The name of the event that sent this NetworkEventData
    csp::common::String EventName;

    /// @brief The ID of the client that sent this NetworkEventData
    uint64_t SenderClientId;

    /// @brief The collection of values sent with this Event. May be empty.
    csp::common::Array<csp::common::ReplicatedValue> EventValues;
};

/// @brief Describes the changes an asset has undergone when the client application is connected to a space.
class CSP_API AssetDetailBlobChangedNetworkEventData : public NetworkEventData
{
public:
    /// @brief The type of change this asset has undergone.
    EAssetChangeType ChangeType;

    /// @brief The unique identifer of the asset that has changed.
    csp::common::String AssetId;

    /// @brief The current version of the asset that has changed.
    csp::common::String Version;

    /// @brief The type of the asset that has changed.
    csp::systems::EAssetType AssetType;

    /// @brief The unique identifer of the asset collection the asset that has changed belongs to.
    csp::common::String AssetCollectionId;
};

/// @brief Class used to provide details of a conversation message that has been received whilst the client application is connected to a space.
class CSP_API ConversationNetworkEventData : public NetworkEventData
{
public:
    csp::multiplayer::ConversationEventType MessageType;
    csp::multiplayer::MessageInfo MessageInfo;
};

/// @brief Class used to provide details of a permission change that has happened to a user whilst the client application is connected to a space.
class CSP_API AccessControlChangedNetworkEventData : public NetworkEventData
{
public:
    /// @brief The unique identifier of the space for which a user's permissions have changed.
    csp::common::String SpaceId;

    /// @brief The roles that a user has for the given space
    csp::common::Array<csp::systems::SpaceUserRole> UserRoles;

    /// @brief The type of permissions change that has occurred the user.
    EPermissionChangeType ChangeType;

    /// @brief The unique identifier of the user whose permissions have been changed.
    csp::common::String UserId;
};

enum class ESequenceUpdateType
{
    Create = 0,
    Update = 1,
    Delete = 3,
    Invalid = 4
};

/// @brief The SequenceSystem allows ordered sequences of items to be created and managed in a space.
/// The HotspotSequenceSystem is a wrapper around the SequenceSystem that makes it easier to manage sequences of Hotspots.
/// Sequences can therefore represent either default sequences, or hotspot sequences. This enum is used to differentiate the two types.
enum class ESequenceType
{
    Default = 0,
    Hotspot = 1
};

class CSP_API SequenceChangedNetworkEventData : public NetworkEventData
{
public:
    /// @brief The type of update to the sequence.
    ESequenceUpdateType UpdateType;

    /// @brief The type of sequence this data represents.
    ESequenceType SequenceType;

    /// @brief The key of the sequence which was updated.
    csp::common::String Key;

    /// @brief If a sequence is renamed using the RenameSequence function, this will be the new key.
    csp::common::String NewKey;

    /// @brief The Id of the Space this sequence is associated with. This will only be set if the SequenceType is Hotspot.
    csp::common::String SpaceId;
};

// @brief Data for an event signalling the completion of an async operation.
// This is general purpose event data that can be used by any system exposing async operations.
class CSP_API AsyncCallCompletedEventData : public NetworkEventData
{
public:
    /*
    Please note:
    The structure of the AsyncCallCompleted event has been updated by the backend services to include some additional properties.
    ReferenceId and ReferenceType are being replaced by a Map called References, and new Status and StatusReason properties have been added.
    This change is currently behind an backend feature flag, ready to be switched over. As this will be a breaking change we have added these new
    properties to the event, but will temporarily be keeping the old ones. We will populate both the old and new properties when we deserialise the
    SignalR event values, which means that Clients will continue to be able to consume the event as before.

    Once this CSP change has been adopted by clients and is confirmed working, we can:
    - Get the backend services to update the flag and send the new AsyncCallCompletedEvent structure.
    - Ask client teams to update to use the new event properties.
    - Remove the old event properties and temporary logic, including this comment - this last step is captured by ticket OF-1835.
    */

    /// @brief The name of the async operation that has been completed.
    csp::common::String OperationName;

    /// @brief An Id related to the async operation that has been completed.
    /// This could for example be a group Id, if this were an async duplicate group operation.
    csp::common::String ReferenceId;

    /// @brief The type that the Id represents.
    /// In the previous example this would be "GroupId".
    csp::common::String ReferenceType;

    /// @brief A string map containing reference information related to this operation.
    /// Each key:value pair in this map represents a reference name and its corresponding Id.
    /// The contents of this map will differ based on the specific Async Call, but it is intended to provide additional context and
    /// information about the completed operation.
    /// For example, in the case of the DuplicatedSpaceAsync operation, this map would contain the following:
    /// - "OrignalSpaceId": Id of the original Space being duplicated.
    /// - "SpaceId": Id of the newly duplicated Space.
    csp::common::Map<csp::common::String, csp::common::String> References;

    /// @brief Whether the operation completed successfully or not.
    bool Success;

    /// @brief This will be an empty string if the operation was successful, but if the operation failed it will contain the failure status.
    csp::common::String StatusReason;
};

// TODO, this should not be here. It's not an event data, it's just a type for a callback used in the AssetSystem.
// The annoyance is that ChangeType is defined for these EventDatas, gotta break that at some point.
// I don't really know where this should go at the moment.
class CSP_API MaterialChangedParams
{
public:
    /// @brief The collection id for the material
    /// This will be redundant in the future
    csp::common::String MaterialCollectionId;

    /// @brief The id for the material
    csp::common::String MaterialId;

    /// @brief The type of change this material has undergone.
    EAssetChangeType ChangeType;
};

} // namespace csp::multiplayer
