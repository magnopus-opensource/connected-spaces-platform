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

#include "Multiplayer/EventSerialisation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "MCS/MCSTypes.h"

#include "Common/Encode.h"
#include "Multiplayer/MultiplayerConstants.h"

#include <fmt/format.h>
#include <regex>

namespace
{
using namespace csp::multiplayer;

ESequenceUpdateType ESequenceUpdateIntToUpdateType(uint64_t UpdateType, csp::common::LogSystem& LogSystem)
{
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
        SequenceUpdateType = ESequenceUpdateType::Rename;
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
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "Unsupported event argument type.");
    }

    return ReplicatedValue;
}

// Parse the parts common to all events, extracting the event type (string) and the sender client id (uint)
void PopulateCommonEventData(const std::vector<signalr::value>& EventValues, EventData& OutEventData, csp::common::LogSystem& LogSystem)
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

        for (auto& Component : Components)
        {
            // Component is in form [TypeId, [Field0, Field1, ...]]
            auto Type = Component.second.as_array()[0].as_uinteger();
            auto& Value = Component.second.as_array()[1].as_array()[0]; // ItemComponentData<T> only has a single field
            OutEventData.EventValues[i++] = ParseSignalRComponent(Type, Value, LogSystem);
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

EventData DeserializeGeneralPurposeEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    EventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);
    return ParsedEvent;
}

AssetDetailBlobChangedEventData DeserializeAssetDetailBlobChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    AssetDetailBlobChangedEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    ParsedEvent.ChangeType = EAssetChangeType::Invalid;

    if (ParsedEvent.EventValues[0].GetInt() < static_cast<int64_t>(EAssetChangeType::Num))
    {
        ParsedEvent.ChangeType = static_cast<EAssetChangeType>(ParsedEvent.EventValues[0].GetInt());
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

ConversationEventData DeserializeConversationEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    ConversationEventData ParsedEvent {};
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

AccessControlChangedEventData DeserializeAccessControlChangedEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    AccessControlChangedEventData ParsedEvent {};
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

            ParsedEvent.ChangeType = EPermissionChangeType::Invalid;

            if (ChangeTypeString == "Created")
            {
                ParsedEvent.ChangeType = EPermissionChangeType::Created;
            }
            else if (ChangeTypeString == "Updated")
            {
                ParsedEvent.ChangeType = EPermissionChangeType::Updated;
            }
            else if (ChangeTypeString == "Removed")
            {
                ParsedEvent.ChangeType = EPermissionChangeType::Removed;
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

SequenceChangedEventData DeserializeSequenceChangedEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    SequenceChangedEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent, LogSystem);

    if (ParsedEvent.EventValues.Size() != 3)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "SequenceChangedEvent - Invalid arguments.");
        throw std::invalid_argument("SequenceChangedEvent - Invalid arguments.");
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

SequenceHotspotChangedEventData DeserializeSequenceHotspotChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem)
{
    SequenceHotspotChangedEventData ParsedEvent {};
    PopulateCommonEventData(EventValues, ParsedEvent);

    if (ParsedEvent.EventValues.Size() != 3)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("SequenceHotspotChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", ParsedEvent.EventValues.Size()).c_str());
        throw std::invalid_argument(
            fmt::format("SequenceHotspotChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", ParsedEvent.EventValues.Size()).c_str());
    }

    int64_t UpdateType = ParsedEvent.EventValues[0].GetInt();
    ParsedEvent.UpdateType = ESequenceUpdateIntToUpdateType(UpdateType, LogSystem);

    const csp::common::String Key = DecodeSequenceKey(ParsedEvent.EventValues[1]);
    ParsedEvent.SpaceId = GetSequenceKeyIndex(Key, 1);
    ParsedEvent.Name = GetSequenceKeyIndex(Key, 2);

    if (ParsedEvent.UpdateType == ESequenceUpdateType::Rename)
    {
        // When a key is changed (renamed) then we get an additional parameter describing the new key.
        // The usual event data describing the name in this instance will describe the _old_ key.
        if (ParsedEvent.EventValues[2].GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
        {
            const csp::common::String NewKey = DecodeSequenceKey(ParsedEvent.EventValues[2]);
            ParsedEvent.NewName = GetSequenceKeyIndex(NewKey, 2);
        }
        else
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error,
                "SequenceHotspotChangedEvent - The expected new name of the hotspot sequence was not found in the event payload.");
        }
    }

    return ParsedEvent;
}
}
