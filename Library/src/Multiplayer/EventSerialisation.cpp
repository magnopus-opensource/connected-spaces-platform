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

#include "Common/Encode.h"
#include "Debug/Logging.h"
#include "Multiplayer/MultiplayerConstants.h"

#include <regex>

using namespace csp::multiplayer;

namespace
{
ESequenceUpdateType ESequenceUpdateIntToUpdateType(uint64_t UpdateType)
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
        CSP_LOG_ERROR_MSG("SequenceChangedEvent - Detected an unsupported update type.");
        break;
    }
    }

    return SequenceUpdateType;
}

std::string RemoveIdPrefix(const std::string& Id)
{
    if (Id.size() > 5)
    {
        return Id.substr(5);
    }

    return Id;
}

csp::common::String DecodeSequenceKey(csp::multiplayer::ReplicatedValue& RawValue)
{
    // Sequence keys are URI encoded to support reserved characters.
    return csp::common::Decode::URI(RawValue.GetString());
}

} // namespace

csp::common::String csp::multiplayer::GetSequenceKeyIndex(const csp::common::String& SequenceKey, unsigned int Index)
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

EventDeserialiser::EventDeserialiser()
    : SenderClientId(0)
{
}

void EventDeserialiser::ParseCommon(const std::vector<signalr::value>& EventValues)
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

    EventType = (csp::common::String)EventValues[0].as_string().c_str();
    SenderClientId = EventValues[1].as_uinteger();
}

void EventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
    ParseCommon(EventValues);

    /*
     * [3] map<uint, vec> Components
     */

    if (!EventValues[3].is_null())
    {
        const std::map<uint64_t, signalr::value>& Components = EventValues[3].as_uint_map();

        EventData = csp::common::Array<csp::multiplayer::ReplicatedValue>(Components.size());
        int i = 0;

        for (auto& Component : Components)
        {
            // Component is in form [TypeId, [Field0, Field1, ...]]
            auto Type = Component.second.as_array()[0].as_uinteger();
            auto& Value = Component.second.as_array()[1].as_array()[0]; // ItemComponentData<T> only has a single field
            EventData[i++] = ParseSignalRComponent(Type, Value);
        }
    }
}

csp::multiplayer::ReplicatedValue EventDeserialiser::ParseSignalRComponent(uint64_t TypeId, const signalr::value& Component) const
{
    csp::multiplayer::ReplicatedValue ReplicatedValue;

    // Prevents serialization crashes for optional values where the actual value is null.
    if (Component.type() == signalr::value_type::null)
    {
        return ReplicatedValue;
    }

    if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_BOOL)
    {
        ReplicatedValue = Component.as_bool();
    }
    else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_INT64)
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
    else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_DOUBLE)
    {
        ReplicatedValue = (float)Component.as_double();
    }
    else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::STRING)
    {
        ReplicatedValue = Component.as_string().c_str();
    }
    else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::FLOAT_ARRAY)
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
            CSP_LOG_ERROR_MSG("Unsupported event argument type: Only Vector3 and Vector4 float array arguments are accepted.");
        }
    }
    else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_UINT16)
    {
        ReplicatedValue = (int64_t)Component.as_uinteger();
    }
    else
    {
        CSP_LOG_ERROR_MSG("Unsupported event argument type.");
    }

    return ReplicatedValue;
}

void AssetChangedEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
    EventDeserialiser::Parse(EventValues);

    EventParams.ChangeType = EAssetChangeType::Invalid;

    if (EventData[0].GetInt() < static_cast<int64_t>(EAssetChangeType::Num))
    {
        EventParams.ChangeType = static_cast<EAssetChangeType>(EventData[0].GetInt());
    }
    else
    {
        CSP_LOG_ERROR_MSG("AssetDetailChangedEvent - AssetChangeType out of range of acceptable enum values.");
    }

    EventParams.AssetId = EventData[1].GetString();
    EventParams.Version = EventData[2].GetString();
    EventParams.AssetType = csp::systems::ConvertDTOAssetDetailType(EventData[3].GetString());
    EventParams.AssetCollectionId = EventData[4].GetString();
}

void ConversationEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
    EventDeserialiser::Parse(EventValues);

    EventParams.MessageType = static_cast<ConversationMessageType>(EventData[0].GetInt());
    EventParams.MessageValue = EventData[1].GetString();
}

void UserPermissionsChangedEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
    ParseCommon(EventValues);

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
            EventParams.SpaceId = ParseSignalRComponent(SpaceIdComponent[0].as_uinteger(), SpaceIdComponent[1].as_array()[0]).GetString();
        }

        {
            // Group Roles - needs specialised handling as the payload here contains an array of strings, which is atypical for events
            const std::vector<signalr::value>& RolesComponent(Components.at(GROUP_ROLES_ID).as_array());
            if (RolesComponent[0].as_uinteger() == csp::multiplayer::msgpack_typeids::ItemComponentData::STRING_ARRAY)
            {
                const std::vector<signalr::value>& RolesArrayComponent = RolesComponent[1].as_array()[0].as_array();

                int i = 0;
                EventParams.UserRoles = csp::common::Array<csp::systems::SpaceUserRole>(RolesArrayComponent.size());
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
                        CSP_LOG_ERROR_MSG("UserPermissionsChangedEvent - Detected an unsupported role type. Defaulting to Invalid role.");
                    }

                    EventParams.UserRoles[i++] = NewRole;
                }
            }
            else
            {
                CSP_LOG_ERROR_MSG("UserPermissionsChangedEvent - Failed to find the expected array of roles for a user when an event was received.");
            }
        }

        {
            const std::vector<signalr::value>& ChangeTypeComponent(Components.at(CHANGE_TYPE_ID).as_array());
            const csp::common::String ChangeTypeString(
                ParseSignalRComponent(ChangeTypeComponent[0].as_uinteger(), ChangeTypeComponent[1].as_array()[0]).GetString());

            EventParams.ChangeType = EPermissionChangeType::Invalid;

            if (ChangeTypeString == "Created")
            {
                EventParams.ChangeType = EPermissionChangeType::Created;
            }
            else if (ChangeTypeString == "Updated")
            {
                EventParams.ChangeType = EPermissionChangeType::Updated;
            }
            else if (ChangeTypeString == "Removed")
            {
                EventParams.ChangeType = EPermissionChangeType::Removed;
            }
            else
            {
                CSP_LOG_ERROR_MSG("UserPermissionsChangedEvent - Detected an unsupported kind of role change. Defaulting to kind of change.");
            }
        }

        {
            const std::vector<signalr::value>& UserIdComponent(Components.at(USER_ID).as_array());
            EventParams.UserId = ParseSignalRComponent(UserIdComponent[0].as_uinteger(), UserIdComponent[1].as_array()[0]).GetString();
        }
    }
}

void csp::multiplayer::SequenceChangedEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
    EventDeserialiser::Parse(EventValues);

    if (EventData.Size() != 3)
    {
        CSP_LOG_ERROR_MSG("SequenceChangedEvent - Invalid arguments.");
        return;
    }

    int64_t UpdateType = EventData[0].GetInt();

    EventParams.UpdateType = ESequenceUpdateIntToUpdateType(UpdateType);

    EventParams.Key = DecodeSequenceKey(EventData[1]);

    // Optional parameter for when a key is changed
    if (EventData[2].GetReplicatedValueType() == ReplicatedValueType::String)
    {
        // Sequence keys are URI encoded to support reserved characters.
        EventParams.NewKey = csp::common::Decode::URI(EventData[2].GetString());
    }
}

void SequenceHotspotChangedEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
    EventDeserialiser::Parse(EventValues);

    if (EventData.Size() != 3)
    {
        CSP_LOG_ERROR_FORMAT("SequenceHotspotChangedEvent - Invalid arguments. Expected 3 arguments but got %i.", EventData.Size());
        return;
    }

    int64_t UpdateType = EventData[0].GetInt();
    EventParams.UpdateType = ESequenceUpdateIntToUpdateType(UpdateType);

    const csp::common::String Key = DecodeSequenceKey(EventData[1]);
    EventParams.SpaceId = GetSequenceKeyIndex(Key, 1);
    EventParams.Name = GetSequenceKeyIndex(Key, 2);

    if (EventParams.UpdateType == ESequenceUpdateType::Rename)
    {
        // When a key is changed (renamed) then we get an additional parameter describing the new key.
        // The usual event data describing the name in this instance will describe the _old_ key.
        if (EventData[2].GetReplicatedValueType() == ReplicatedValueType::String)
        {
            const csp::common::String NewKey = DecodeSequenceKey(EventData[2]);
            EventParams.NewName = GetSequenceKeyIndex(NewKey, 2);
        }
        else
        {
            CSP_LOG_ERROR_MSG("SequenceHotspotChangedEvent - The expected new name of the hotspot sequence was not found in the event payload.");
        }
    }
}
