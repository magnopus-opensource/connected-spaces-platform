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

csp::common::ESequenceUpdateType ESequenceUpdateIntToUpdateType(uint64_t updateType, csp::common::LogSystem& logSystem)
{
    using namespace csp::common;
    ESequenceUpdateType sequenceUpdateType = ESequenceUpdateType::Invalid;

    switch (updateType)
    {
    case 0:
    {
        sequenceUpdateType = ESequenceUpdateType::Create;
        break;
    }
    case 1:
    {
        sequenceUpdateType = ESequenceUpdateType::Update;
        break;
    }
    case 2:
    {
        logSystem.LogMsg(csp::common::LogLevel::Warning, "SequenceChangedEvent - Rename is no longer a supported update type.");
        break;
    }
    case 3:
    {
        sequenceUpdateType = ESequenceUpdateType::Delete;
        break;
    }
    default:
    {
        logSystem.LogMsg(csp::common::LogLevel::Error, "SequenceChangedEvent - Detected an unsupported update type.");
        break;
    }
    }

    return sequenceUpdateType;
}

csp::common::String DecodeSequenceKey(csp::common::ReplicatedValue& rawValue)
{
    // Sequence keys are URI encoded to support reserved characters.
    return csp::common::Decode::URI(rawValue.GetString());
}

// forward declaring the ParseSignalRComponent method.
csp::common::ReplicatedValue ParseSignalRComponent(uint64_t typeId, const signalr::value& component, csp::common::LogSystem& logSystem);

csp::common::ReplicatedValue ParseSignalRComponentFromItemComponent(const signalr::value& itemComponentData, csp::common::LogSystem& logSystem)
{
    const auto& itemComponentDataArray = itemComponentData.as_array();

    uint64_t type = itemComponentDataArray[0].as_uinteger();
    const signalr::value& value = itemComponentDataArray[1].as_array()[0]; // ItemComponentData<T> only has a single field

    return ParseSignalRComponent(type, value, logSystem);
}

csp::common::ReplicatedValue ParseSignalRComponent(uint64_t typeId, const signalr::value& component, csp::common::LogSystem& logSystem)
{
    csp::common::ReplicatedValue replicatedValue;

    // Prevents serialization crashes for optional values where the actual value is null.
    if (component.type() == signalr::value_type::null)
    {
        return replicatedValue;
    }

    if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::BOOL))
    {
        replicatedValue = component.as_bool();
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_BOOL))
    {
        if (!component.is_null())
        {
            replicatedValue = component.as_bool();
        }
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_INT64))
    {
        if (component.is_integer())
        {
            replicatedValue = (int64_t)component.as_integer();
        }
        else
        {
            replicatedValue = (int64_t)component.as_uinteger();
        }
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_DOUBLE))
    {
        replicatedValue = (float)component.as_double();
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::STRING))
    {
        replicatedValue = component.as_string().c_str();
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::FLOAT_ARRAY))
    {
        auto& array = component.as_array();

        if (array.size() == 3)
        {
            replicatedValue = csp::common::Vector3 { (float)array[0].as_double(), (float)array[1].as_double(), (float)array[2].as_double() };
        }
        else if (array.size() == 4)
        {
            replicatedValue = csp::common::Vector4 { (float)array[0].as_double(), (float)array[1].as_double(), (float)array[2].as_double(),
                (float)array[3].as_double() };
        }
        else
        {
            logSystem.LogMsg(
                csp::common::LogLevel::Error, "Unsupported event argument type: Only Vector3 and Vector4 float array arguments are accepted.");
        }
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::NULLABLE_UINT16))
    {
        replicatedValue = (int64_t)component.as_uinteger();
    }
    else if (typeId == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::STRING_DICTIONARY))
    {
        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> resultMap;
        const auto& stringMap = component.as_string_map();

        for (const auto& [Key, ItemComponentData] : stringMap)
        {
            resultMap[Key.c_str()] = ParseSignalRComponentFromItemComponent(ItemComponentData, logSystem);
        }

        replicatedValue = csp::common::ReplicatedValue { resultMap };
    }
    else
    {
        logSystem.LogMsg(csp::common::LogLevel::Error, "Unsupported event argument type.");
    }

    return replicatedValue;
}

// Parse the parts common to all events, extracting the event type (string) and the sender client id (uint)
void PopulateCommonEventData(
    const std::vector<signalr::value>& eventValues, csp::common::NetworkEventData& outEventData, csp::common::LogSystem& logSystem)
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

    outEventData.EventName = eventValues[0].as_string().c_str();
    outEventData.SenderClientId = eventValues[1].as_uinteger();

    /*
     * [3] map<uint, vec> Components
     */

    if (!eventValues[3].is_null())
    {
        const std::map<uint64_t, signalr::value>& components = eventValues[3].as_uint_map();

        outEventData.EventValues = csp::common::Array<csp::common::ReplicatedValue>(components.size());
        int i = 0;

        for (auto& [Key, ItemComponentData] : components)
        {
            outEventData.EventValues[i++] = ParseSignalRComponentFromItemComponent(ItemComponentData, logSystem);
        }
    }
}

} // namespace

namespace csp::multiplayer
{
csp::common::String GetSequenceKeyIndex(const csp::common::String& sequenceKey, unsigned int index)
{
    const std::string sequenceKeyString(sequenceKey.c_str());
    // Match item after second ':' to get our parent Id.
    // See CreateKey in HotSpotSequenceSystem for more info on the pattern.
    const std::regex expression(R"(^(?:[^:]*\:){)" + std::to_string(index) + R"(}([^:]*))");
    std::smatch match;
    const bool found = std::regex_search(std::begin(sequenceKeyString), std::end(sequenceKeyString), match, expression);

    if (found == false)
    {
        return "";
    }

    std::string parentIdString = match[1];

    if (parentIdString.empty())
    {
        return "";
    }

    return parentIdString.c_str();
}

csp::common::NetworkEventData DeserializeGeneralPurposeEvent(const std::vector<signalr::value>& eventValues, csp::common::LogSystem& logSystem)
{
    csp::common::NetworkEventData parsedEvent {};
    PopulateCommonEventData(eventValues, parsedEvent, logSystem);
    return parsedEvent;
}

csp::common::AssetDetailBlobChangedNetworkEventData DeserializeAssetDetailBlobChangedEvent(
    const std::vector<signalr::value>& eventValues, csp::common::LogSystem& logSystem)
{
    csp::common::AssetDetailBlobChangedNetworkEventData parsedEvent {};
    PopulateCommonEventData(eventValues, parsedEvent, logSystem);

    parsedEvent.ChangeType = csp::common::EAssetChangeType::Invalid;

    if (parsedEvent.EventValues[0].GetInt() < static_cast<int64_t>(csp::common::EAssetChangeType::Num))
    {
        parsedEvent.ChangeType = static_cast<csp::common::EAssetChangeType>(parsedEvent.EventValues[0].GetInt());
    }
    else
    {
        logSystem.LogMsg(csp::common::LogLevel::Error, "AssetDetailChangedEvent - AssetChangeType out of range of acceptable enum values.");
    }

    parsedEvent.AssetId = parsedEvent.EventValues[1].GetString();
    parsedEvent.Version = parsedEvent.EventValues[2].GetString();
    parsedEvent.AssetType = csp::systems::ConvertDTOAssetDetailType(parsedEvent.EventValues[3].GetString());
    parsedEvent.AssetCollectionId = parsedEvent.EventValues[4].GetString();

    return parsedEvent;
}

csp::common::ConversationNetworkEventData DeserializeConversationEvent(
    const std::vector<signalr::value>& eventValues, csp::common::LogSystem& logSystem)
{
    csp::common::ConversationNetworkEventData parsedEvent {};
    PopulateCommonEventData(eventValues, parsedEvent, logSystem);

    parsedEvent.MessageType = static_cast<ConversationEventType>(parsedEvent.EventValues[0].GetInt());
    parsedEvent.MessageInfo.ConversationId = parsedEvent.EventValues[1].GetString();
    parsedEvent.MessageInfo.CreatedTimestamp = parsedEvent.EventValues[2].GetString();
    parsedEvent.MessageInfo.EditedTimestamp = parsedEvent.EventValues[3].GetString();
    parsedEvent.MessageInfo.UserId = parsedEvent.EventValues[4].GetString();
    parsedEvent.MessageInfo.Message = parsedEvent.EventValues[5].GetString();
    parsedEvent.MessageInfo.MessageId = parsedEvent.EventValues[6].GetString();

    return parsedEvent;
}

csp::common::AccessControlChangedNetworkEventData DeserializeAccessControlChangedEvent(
    const std::vector<signalr::value>& eventValues, csp::common::LogSystem& logSystem)
{
    csp::common::AccessControlChangedNetworkEventData parsedEvent {};
    PopulateCommonEventData(eventValues, parsedEvent, logSystem);

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

    if (!eventValues[3].is_null())
    {
        const uint64_t spaceId = 1;
        const uint64_t groupRolesId = 100;
        const uint64_t changeTypeId = 101;
        const uint64_t userId = 102;
        const std::map<uint64_t, signalr::value>& components = eventValues[3].as_uint_map();

        {
            const std::vector<signalr::value>& spaceIdComponent(components.at(spaceId).as_array());
            parsedEvent.SpaceId = ParseSignalRComponent(spaceIdComponent[0].as_uinteger(), spaceIdComponent[1].as_array()[0], logSystem).GetString();
        }

        {
            // Group Roles - needs specialised handling as the payload here contains an array of strings, which is atypical for events
            const std::vector<signalr::value>& rolesComponent(components.at(groupRolesId).as_array());
            if (rolesComponent[0].as_uinteger() == static_cast<uint64_t>(csp::multiplayer::mcs::ItemComponentDataType::STRING_ARRAY))
            {
                const std::vector<signalr::value>& rolesArrayComponent = rolesComponent[1].as_array()[0].as_array();

                int i = 0;
                parsedEvent.UserRoles = csp::common::Array<csp::systems::SpaceUserRole>(rolesArrayComponent.size());
                for (auto& roleValue : rolesArrayComponent)
                {
                    csp::systems::SpaceUserRole newRole = csp::systems::SpaceUserRole::Invalid;
                    const std::string& roleValueString = roleValue.as_string();

                    if (roleValueString == "viewer")
                    {
                        newRole = csp::systems::SpaceUserRole::User;
                    }
                    else if (roleValueString == "creator")
                    {
                        newRole = csp::systems::SpaceUserRole::Moderator;
                    }
                    else if (roleValueString == "owner")
                    {
                        newRole = csp::systems::SpaceUserRole::Owner;
                    }
                    else
                    {
                        logSystem.LogMsg(csp::common::LogLevel::Error,
                            "UserPermissionsChangedEvent - Detected an unsupported role type. Defaulting to Invalid role.");
                    }

                    parsedEvent.UserRoles[i++] = newRole;
                }
            }
            else
            {
                logSystem.LogMsg(csp::common::LogLevel::Error,
                    "UserPermissionsChangedEvent - Failed to find the expected array of roles for a user when an event was received.");
            }
        }

        {
            const std::vector<signalr::value>& changeTypeComponent(components.at(changeTypeId).as_array());
            const csp::common::String changeTypeString(
                ParseSignalRComponent(changeTypeComponent[0].as_uinteger(), changeTypeComponent[1].as_array()[0], logSystem).GetString());

            parsedEvent.ChangeType = csp::common::EPermissionChangeType::Invalid;

            if (changeTypeString == "Created")
            {
                parsedEvent.ChangeType = csp::common::EPermissionChangeType::Created;
            }
            else if (changeTypeString == "Updated")
            {
                parsedEvent.ChangeType = csp::common::EPermissionChangeType::Updated;
            }
            else if (changeTypeString == "Removed")
            {
                parsedEvent.ChangeType = csp::common::EPermissionChangeType::Removed;
            }
            else
            {
                logSystem.LogMsg(csp::common::LogLevel::Error,
                    "UserPermissionsChangedEvent - Detected an unsupported kind of role change. Defaulting to kind of change.");
            }
        }

        {
            const std::vector<signalr::value>& userIdComponent(components.at(userId).as_array());
            parsedEvent.UserId = ParseSignalRComponent(userIdComponent[0].as_uinteger(), userIdComponent[1].as_array()[0], logSystem).GetString();
        }
    }
    else
    {
        throw std::invalid_argument("Unexpected null eventvalues in DeserializeAccessControlChangedEvent");
    }

    return parsedEvent;
}

csp::common::SequenceChangedNetworkEventData DeserializeSequenceChangedEvent(
    const std::vector<signalr::value>& eventValues, csp::common::LogSystem& logSystem)
{
    csp::common::SequenceChangedNetworkEventData parsedEvent {};
    PopulateCommonEventData(eventValues, parsedEvent, logSystem);

    if (parsedEvent.EventValues.Size() != 3)
    {
        logSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("SequenceChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", parsedEvent.EventValues.Size()).c_str());
        throw std::invalid_argument(
            fmt::format("SequenceChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", parsedEvent.EventValues.Size()).c_str());
    }

    int64_t updateType = parsedEvent.EventValues[0].GetInt();

    parsedEvent.UpdateType = ESequenceUpdateIntToUpdateType(updateType, logSystem);

    parsedEvent.Key = DecodeSequenceKey(parsedEvent.EventValues[1]);

    // Optional parameter for when a key is changed
    if (parsedEvent.EventValues[2].GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
    {
        // Sequence keys are URI encoded to support reserved characters.
        parsedEvent.NewKey = csp::common::Decode::URI(parsedEvent.EventValues[2].GetString());
    }

    return parsedEvent;
}

csp::common::AsyncCallCompletedEventData DeserializeAsyncCallCompletedEvent(
    const std::vector<signalr::value>& eventValues, csp::common::LogSystem& logSystem)
{
    csp::common::AsyncCallCompletedEventData parsedEvent {};
    PopulateCommonEventData(eventValues, parsedEvent, logSystem);

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

    parsedEvent.OperationName = parsedEvent.EventValues[0].GetString();

    // Check to see if this event has the new structure that includes a Reference map in place of the old ReferenceId and ReferenceType. As part of
    // the transition, we will populate both the new and old properties to maintain backwards compatibility with clients.
    bool isNewReferenceMapFormat = parsedEvent.EventValues[1].GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap;

    if (isNewReferenceMapFormat)
    {
        const auto& referencesStringMap = parsedEvent.EventValues[1].GetStringMap();

        for (const auto& [Key, ReplicatedValue] : referencesStringMap)
        {
            parsedEvent.References[Key] = ReplicatedValue.GetString();

            // For backwards compatibility, we will also populate the old ReferenceId and ReferenceType properties.
            // Please note: As part of this change the backend services have changed the ReferenceType from "GroupId" > "SpaceId".
            if (Key == "SpaceId")
            {
                parsedEvent.ReferenceId = ReplicatedValue.GetString();
                parsedEvent.ReferenceType = "GroupId";
            }
        }

        parsedEvent.Success = parsedEvent.EventValues[2].GetBool();
        parsedEvent.StatusReason = parsedEvent.EventValues[3].GetString();
    }
    else
    {
        // This event uses the old structure with ReferenceId and ReferenceType properties, so we will populate those for backwards compatibility.
        parsedEvent.ReferenceId = parsedEvent.EventValues[1].GetString();
        parsedEvent.ReferenceType = parsedEvent.EventValues[2].GetString();
    }

    return parsedEvent;
}
}
