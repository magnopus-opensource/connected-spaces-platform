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

#include "Multiplayer/NetworkEventSerialisation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "MCS/MCSTypes.h"

#include "Common/Encode.h"
#include "Multiplayer/MultiplayerConstants.h"

#include <fmt/format.h>
#include <regex>

namespace
{
using namespace csp::multiplayer;

csp::common::ESequenceUpdateType ESequenceUpdateIntToUpdateType(uint64_t UpdateType, csp::common::LogSystem& LogSystem)
{
    using namespace csp::common;
    ESequenceUpdateType SequenceUpdateType = ESequenceUpdateType::Invalid;

    switch (UpdateType)
    {
    case 0:
    {
        SequenceUpdateType = ESequenceUpdateType::Create;
        break;
    }
    case 1:
    {
        SequenceUpdateType = ESequenceUpdateType::Update;
        break;
    }
    case 2:
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "SequenceChangedEvent - Rename is no longer a supported update type.");
        break;
    }
    case 3:
    {
        SequenceUpdateType = ESequenceUpdateType::Delete;
        break;
    }
    default:
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "SequenceChangedEvent - Detected an unsupported update type.");
        break;
    }
    }

    return SequenceUpdateType;
}

csp::common::String DecodeSequenceKey(csp::common::ReplicatedValue& RawValue)
{
    // Sequence keys are URI encoded to support reserved characters.
    return csp::common::Decode::URI(RawValue.GetString());
}

// forward declaring the ParseSignalRComponent method.
csp::common::ReplicatedValue ParseSignalRComponent(uint64_t TypeId, const signalr::value& Component, csp::common::LogSystem& LogSystem);

csp::common::ReplicatedValue ParseSignalRComponentFromItemComponent(const signalr::value& ItemComponentData, csp::common::LogSystem& LogSystem)
{
    const auto& ItemComponentDataArray = ItemComponentData.as_array();

    uint64_t Type = ItemComponentDataArray[0].as_uinteger();
    const signalr::value& Value = ItemComponentDataArray[1].as_array()[0]; // ItemComponentData<T> only has a single field

    return ParseSignalRComponent(Type, Value, LogSystem);
}

csp::common::ReplicatedValue ParseSignalRComponent(uint64_t TypeId, const signalr::value& Component, csp::common::LogSystem& LogSystem)
{
    csp::common::ReplicatedValue ReplicatedValue;

    // Prevents serialization crashes for optional values where the actual value is null.
    if (Component.type() == signalr::value_type::null)
    {
        return ReplicatedValue;
    }

    if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_BOOL))
    {
        ReplicatedValue = Component.as_bool();
    }
    else if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_INT64))
    {
        if (Component.is_integer())
        {
            ReplicatedValue = (int64_t)Component.as_integer();
        }
        else
        {
            ReplicatedValue = (int64_t)Component.as_uinteger();
        }
    }
    else if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_DOUBLE))
    {
        ReplicatedValue = (float)Component.as_double();
    }
    else if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::STRING))
    {
        ReplicatedValue = Component.as_string().c_str();
    }
    else if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::FLOAT_ARRAY))
    {
        auto& Array = Component.as_array();

        if (Array.size() == 3)
        {
            ReplicatedValue = csp::common::Vector3 { (float)Array[0].as_double(), (float)Array[1].as_double(), (float)Array[2].as_double() };
        }
        else if (Array.size() == 4)
        {
            ReplicatedValue = csp::common::Vector4 { (float)Array[0].as_double(), (float)Array[1].as_double(), (float)Array[2].as_double(),
                (float)Array[3].as_double() };
        }
        else
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Error, "Unsupported event argument type: Only Vector3 and Vector4 float array arguments are accepted.");
        }
    }
    else if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_UINT16))
    {
        ReplicatedValue = (int64_t)Component.as_uinteger();
    }
    else if (TypeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::STRING_DICTIONARY))
    {
        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> ResultMap;
        const auto& StringMap = Component.as_string_map();

        for (const auto& [Key, ItemComponentData] : StringMap)
        {
            ResultMap[Key.c_str()] = ParseSignalRComponentFromItemComponent(ItemComponentData, LogSystem);
        }

        ReplicatedValue = csp::common::ReplicatedValue { ResultMap };
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Unsupported event argument type.");
    }

    return ReplicatedValue;
}

// Parse the parts common to all events, extracting the event type (string) and the sender client id (uint)
void PopulateCommonEventData(
    const std::vector<signalr::value>& EventValues, csp::common::NetworkEventData& OutEventData, csp::common::LogSystem& LogSystem)
{
    /*
     * class EventMessage
     * [0] string EventType
     * [1] uint SenderClientId
     * [2] uint? RecipientClientId
     *
     * RecipientClientId can be processed if needed, but currently not required, though note it is a nullable uint,
       null for an all-client broadcast, and a uint for the intended receiving client's Id : RecipientClientId = EventValues[2];
    */

    OutEventData.EventName = EventValues[0].as_string().c_str();
    OutEventData.SenderClientId = EventValues[1].as_uinteger();

    /*
     * [3] map<uint, vec> Components
     */

    if (!EventValues[3].is_null())
    {
        const std::map<uint64_t, signalr::value>& Components = EventValues[3].as_uint_map();

        OutEventData.EventValues = csp::common::Array<csp::common::ReplicatedValue>(Components.size());
        int i = 0;

        for (auto& [Key, ItemComponentData] : Components)
        {
            OutEventData.EventValues[i++] = ParseSignalRComponentFromItemComponent(ItemComponentData, LogSystem);
        }
    }
}

} // namespace

namespace csp::multiplayer
{
csp::common::String GetSequenceKeyIndex(const csp::common::String& SequenceKey, unsigned int Index)
{
    const std::string SequenceKeyString(SequenceKey.c_str());
    // Match item after second ':' to get our parent Id.
    // See CreateKey in HotSpotSequenceSystem for more info on the pattern.
    const std::regex Expression(R"(^(?:[^:]*\:){)" + std::to_string(Index) + R"(}([^:]*))");
    std::smatch Match;
    const bool Found = std::regex_search(std::begin(SequenceKeyString), std::end(SequenceKeyString), Match, Expression);

    if (Found == false)
    {
        return "";
    }

    std::string ParentIdString = Match[1];

    if (ParentIdString.empty())
    {
        return "";
    }

    return ParentIdString.c_str();
}

csp::common::NetworkEventData DeserializeGeneralPurposeEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    csp::common::NetworkEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);
    return ParsedEvent;
}

csp::common::AssetDetailBlobChangedNetworkEventData DeserializeAssetDetailBlobChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    csp::common::AssetDetailBlobChangedNetworkEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    ParsedEvent.ChangeType = csp::common::EAssetChangeType::Invalid;

    if (ParsedEvent.EventValues[0].GetInt() < static_cast<int64_t>(csp::common::EAssetChangeType::Num))
    {
        ParsedEvent.ChangeType = static_cast<csp::common::EAssetChangeType>(ParsedEvent.EventValues[0].GetInt());
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "AssetDetailChangedEvent - AssetChangeType out of range of acceptable enum values.");
    }

    ParsedEvent.AssetId = ParsedEvent.EventValues[1].GetString();
    ParsedEvent.Version = ParsedEvent.EventValues[2].GetString();
    ParsedEvent.AssetType = csp::systems::ConvertDTOAssetDetailType(ParsedEvent.EventValues[3].GetString());
    ParsedEvent.AssetCollectionId = ParsedEvent.EventValues[4].GetString();

    return ParsedEvent;
}

csp::common::ConversationNetworkEventData DeserializeConversationEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    csp::common::ConversationNetworkEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    ParsedEvent.MessageType = static_cast<ConversationEventType>(ParsedEvent.EventValues[0].GetInt());
    ParsedEvent.MessageInfo.ConversationId = ParsedEvent.EventValues[1].GetString();
    ParsedEvent.MessageInfo.CreatedTimestamp = ParsedEvent.EventValues[2].GetString();
    ParsedEvent.MessageInfo.EditedTimestamp = ParsedEvent.EventValues[3].GetString();
    ParsedEvent.MessageInfo.UserId = ParsedEvent.EventValues[4].GetString();
    ParsedEvent.MessageInfo.Message = ParsedEvent.EventValues[5].GetString();
    ParsedEvent.MessageInfo.MessageId = ParsedEvent.EventValues[6].GetString();

    return ParsedEvent;
}

csp::common::AccessControlChangedNetworkEventData DeserializeAccessControlChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    csp::common::AccessControlChangedNetworkEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    /*
     * [3] map<uint, vec> Components, where Components is structured as follows:
     * | Name              | Component ID | Type         | Notes (units, ranges)                                                     |
     * |-------------------|--------------|--------------|---------------------------------------------------------------------------|
     * | **SpaceId**       | 1            | String       | Id of the space that has updated permissions                              |
     * | **UserRoles**     | 100          | String Array | Array of user permissions (viewer,creator,owner) that belongs to the user |
     * | **ChangeType**    | 101          | String       | Created, Updated, Removed                                                 |
     * | **UserId**        | 102          | String       | The userId that was changed                                               |
     * |-------------------|--------------|--------------|---------------------------------------------------------------------------|
     */

    if (!EventValues[3].is_null())
    {
        const uint64_t SPACE_ID = 1;
        const uint64_t GROUP_ROLES_ID = 100;
        const uint64_t CHANGE_TYPE_ID = 101;
        const uint64_t USER_ID = 102;
        const std::map<uint64_t, signalr::value>& Components = EventValues[3].as_uint_map();

        {
            const std::vector<signalr::value>& SpaceIdComponent(Components.at(SPACE_ID).as_array());
            ParsedEvent.SpaceId = ParseSignalRComponent(SpaceIdComponent[0].as_uinteger(), SpaceIdComponent[1].as_array()[0], LogSystem).GetString();
        }

        {
            // Group Roles - needs specialised handling as the payload here contains an array of strings, which is atypical for events
            const std::vector<signalr::value>& RolesComponent(Components.at(GROUP_ROLES_ID).as_array());
            if (RolesComponent[0].as_uinteger() == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::STRING_ARRAY))
            {
                const std::vector<signalr::value>& RolesArrayComponent = RolesComponent[1].as_array()[0].as_array();

                int i = 0;
                ParsedEvent.UserRoles = csp::common::Array<csp::systems::SpaceUserRole>(RolesArrayComponent.size());
                for (auto& RoleValue : RolesArrayComponent)
                {
                    csp::systems::SpaceUserRole NewRole = csp::systems::SpaceUserRole::Invalid;
                    const std::string& RoleValueString = RoleValue.as_string();

                    if (RoleValueString == "viewer")
                    {
                        NewRole = csp::systems::SpaceUserRole::User;
                    }
                    else if (RoleValueString == "creator")
                    {
                        NewRole = csp::systems::SpaceUserRole::Moderator;
                    }
                    else if (RoleValueString == "owner")
                    {
                        NewRole = csp::systems::SpaceUserRole::Owner;
                    }
                    else
                    {
                        LogSystem.LogMsg(csp::common::LogLevel::Error,
                            "UserPermissionsChangedEvent - Detected an unsupported role type. Defaulting to Invalid role.");
                    }

                    ParsedEvent.UserRoles[i++] = NewRole;
                }
            }
            else
            {
                LogSystem.LogMsg(csp::common::LogLevel::Error,
                    "UserPermissionsChangedEvent - Failed to find the expected array of roles for a user when an event was received.");
            }
        }

        {
            const std::vector<signalr::value>& ChangeTypeComponent(Components.at(CHANGE_TYPE_ID).as_array());
            const csp::common::String ChangeTypeString(
                ParseSignalRComponent(ChangeTypeComponent[0].as_uinteger(), ChangeTypeComponent[1].as_array()[0], LogSystem).GetString());

            ParsedEvent.ChangeType = csp::common::EPermissionChangeType::Invalid;

            if (ChangeTypeString == "Created")
            {
                ParsedEvent.ChangeType = csp::common::EPermissionChangeType::Created;
            }
            else if (ChangeTypeString == "Updated")
            {
                ParsedEvent.ChangeType = csp::common::EPermissionChangeType::Updated;
            }
            else if (ChangeTypeString == "Removed")
            {
                ParsedEvent.ChangeType = csp::common::EPermissionChangeType::Removed;
            }
            else
            {
                LogSystem.LogMsg(csp::common::LogLevel::Error,
                    "UserPermissionsChangedEvent - Detected an unsupported kind of role change. Defaulting to kind of change.");
            }
        }

        {
            const std::vector<signalr::value>& UserIdComponent(Components.at(USER_ID).as_array());
            ParsedEvent.UserId = ParseSignalRComponent(UserIdComponent[0].as_uinteger(), UserIdComponent[1].as_array()[0], LogSystem).GetString();
        }
    }
    else
    {
        throw std::invalid_argument("Unexpected null eventvalues in DeserializeAccessControlChangedEvent");
    }

    return ParsedEvent;
}

csp::common::SequenceChangedNetworkEventData DeserializeSequenceChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    csp::common::SequenceChangedNetworkEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    if (ParsedEvent.EventValues.Size() != 3)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("SequenceChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", ParsedEvent.EventValues.Size()).c_str());
        throw std::invalid_argument(
            fmt::format("SequenceChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", ParsedEvent.EventValues.Size()).c_str());
    }

    int64_t UpdateType = ParsedEvent.EventValues[0].GetInt();

    ParsedEvent.UpdateType = ESequenceUpdateIntToUpdateType(UpdateType, LogSystem);

    ParsedEvent.Key = DecodeSequenceKey(ParsedEvent.EventValues[1]);

    // Optional parameter for when a key is changed
    if (ParsedEvent.EventValues[2].GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
    {
        // Sequence keys are URI encoded to support reserved characters.
        ParsedEvent.NewKey = csp::common::Decode::URI(ParsedEvent.EventValues[2].GetString());
    }

    return ParsedEvent;
}

csp::common::AsyncCallCompletedEventData DeserializeAsyncCallCompletedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    csp::common::AsyncCallCompletedEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    /*
    Please note:
    The structure of the AsyncCallCompleted event has been updated by the backend services to include some additional properties.
    ReferenceId and ReferenceType are being replaced by a Map called References, and new Status and StatusReason properties have been added.
    This change is currently behind an backend feature flag, ready to be switched over. As this will be a breaking change we have added these
    new properties to the event, but will temporarily be keeping the old ones. We are populating both the old and new properties when we
    deserialise the SignalR event values, which means that Clients will continue to be able to consume the event as before.

    Once this CSP change has been adopted by clients and is confirmed working, we can:
    - Get the backend services to update the flag and send the new AsyncCallCompletedEvent structure.
    - Ask client teams to update to use the new event properties.
    - Remove the old event properties and temporary logic, including this comment - this last step is captured by ticket OF-1835.
    */

    ParsedEvent.OperationName = ParsedEvent.EventValues[0].GetString();

    // Check to see if this event has the new structure that includes a Reference map in place of the old ReferenceId and ReferenceType. As part of
    // the transition, we will populate both the new and old properties to maintain backwards compatibility with clients.
    bool IsNewReferenceMapFormat = ParsedEvent.EventValues[1].GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap;

    if (IsNewReferenceMapFormat)
    {
        const auto& ReferencesStringMap = ParsedEvent.EventValues[1].GetStringMap();

        for (const auto& [Key, ReplicatedValue] : ReferencesStringMap)
        {
            ParsedEvent.References[Key] = ReplicatedValue.GetString();

            // For backwards compatibility, we will also populate the old ReferenceId and ReferenceType properties.
            // Please note: As part of this change the backend services have changed the ReferenceType from "GroupId" > "SpaceId".
            if (Key == "SpaceId")
            {
                ParsedEvent.ReferenceId = ReplicatedValue.GetString();
                ParsedEvent.ReferenceType = "GroupId";
            }
        }

        ParsedEvent.Success = ParsedEvent.EventValues[2].GetBool();
        ParsedEvent.StatusReason = ParsedEvent.EventValues[3].GetString();
    }
    else
    {
        // This event uses the old structure with ReferenceId and ReferenceType properties, so we will populate those for backwards compatibility.
        ParsedEvent.ReferenceId = ParsedEvent.EventValues[1].GetString();
        ParsedEvent.ReferenceType = ParsedEvent.EventValues[2].GetString();
    }

    return ParsedEvent;
}
}
