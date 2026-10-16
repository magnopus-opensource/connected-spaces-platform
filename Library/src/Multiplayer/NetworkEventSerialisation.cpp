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

csp::common::String DecodeSequenceKey(const csp::common::ReplicatedValue& RawValue)
{
    // Sequence keys are URI encoded to support reserved characters.
    return csp::common::Decode::URI(RawValue.GetString());
}

}

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

csp::common::AssetDetailBlobChangedNetworkEventData DeserializeAssetDetailBlobChangedEvent(
    const csp::common::NetworkEventData& Event, csp::common::LogSystem& LogSystem)
{
    csp::common::AssetDetailBlobChangedNetworkEventData ParsedEvent {};

    ParsedEvent.ChangeType = csp::common::EAssetChangeType::Invalid;

    if (Event.EventValues[0].GetInt() < static_cast<int64_t>(csp::common::EAssetChangeType::Num))
    {
        ParsedEvent.ChangeType = static_cast<csp::common::EAssetChangeType>(Event.EventValues[0].GetInt());
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "AssetDetailChangedEvent - AssetChangeType out of range of acceptable enum values.");
    }

    ParsedEvent.AssetId = Event.EventValues[1].GetString();
    ParsedEvent.Version = Event.EventValues[2].GetString();
    ParsedEvent.AssetType = csp::systems::ConvertDTOAssetDetailType(Event.EventValues[3].GetString());
    ParsedEvent.AssetCollectionId = Event.EventValues[4].GetString();

    // Temporarily set redundant data to not break tests
    ParsedEvent.EventName = Event.EventName;
    ParsedEvent.EventValues = Event.EventValues;
    ParsedEvent.SenderClientId = Event.SenderClientId;

    return ParsedEvent;
}

csp::common::ConversationNetworkEventData DeserializeConversationEvent(const csp::common::NetworkEventData& Event, csp::common::LogSystem&)
{
    csp::common::ConversationNetworkEventData ParsedEvent {};

    ParsedEvent.MessageType = static_cast<ConversationEventType>(Event.EventValues[0].GetInt());
    ParsedEvent.MessageInfo.ConversationId = Event.EventValues[1].GetString();
    ParsedEvent.MessageInfo.CreatedTimestamp = Event.EventValues[2].GetString();
    ParsedEvent.MessageInfo.EditedTimestamp = Event.EventValues[3].GetString();
    ParsedEvent.MessageInfo.UserId = Event.EventValues[4].GetString();
    ParsedEvent.MessageInfo.Message = Event.EventValues[5].GetString();
    ParsedEvent.MessageInfo.MessageId = Event.EventValues[6].GetString();

    // Temporarily set redundant data to not break tests
    ParsedEvent.EventName = Event.EventName;
    ParsedEvent.EventValues = Event.EventValues;
    ParsedEvent.SenderClientId = Event.SenderClientId;

    return ParsedEvent;
}

csp::common::AccessControlChangedNetworkEventData DeserializeAccessControlChangedEvent(
    const csp::common::NetworkEventData&, csp::common::LogSystem&)
{
    csp::common::AccessControlChangedNetworkEventData ParsedEvent {};

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

    /* const uint64_t SPACE_ID = 1;
     const uint64_t GROUP_ROLES_ID = 100;
     const uint64_t CHANGE_TYPE_ID = 101;
     const uint64_t USER_ID = 102;

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
     }*/

    return ParsedEvent;
}

csp::common::SequenceChangedNetworkEventData DeserializeSequenceChangedEvent(const csp::common::NetworkEventData& Event, csp::common::LogSystem& LogSystem)
{
    csp::common::SequenceChangedNetworkEventData ParsedEvent {};

    if (Event.EventValues.Size() != 3)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("SequenceChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", Event.EventValues.Size()).c_str());
        throw std::invalid_argument(
            fmt::format("SequenceChangedEvent - Invalid arguments. Expected 3 arguments but got {}.", Event.EventValues.Size()).c_str());
    }

    int64_t UpdateType = Event.EventValues[0].GetInt();

    ParsedEvent.UpdateType = ESequenceUpdateIntToUpdateType(UpdateType, LogSystem);

    ParsedEvent.Key = DecodeSequenceKey(Event.EventValues[1]);

    // Optional parameter for when a key is changed
    if (ParsedEvent.EventValues[2].GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
    {
        // Sequence keys are URI encoded to support reserved characters.
        ParsedEvent.NewKey = csp::common::Decode::URI(Event.EventValues[2].GetString());
    }

    // Temporarily set redundant data to not break tests
    ParsedEvent.EventName = Event.EventName;
    ParsedEvent.EventValues = Event.EventValues;
    ParsedEvent.SenderClientId = Event.SenderClientId;

    return ParsedEvent;
}

csp::common::AsyncCallCompletedEventData DeserializeAsyncCallCompletedEvent(const csp::common::NetworkEventData& Event, csp::common::LogSystem&)
{
    csp::common::AsyncCallCompletedEventData ParsedEvent {};

    ParsedEvent.OperationName = Event.EventValues[0].GetString();

    const auto& ReferencesStringMap = Event.EventValues[1].GetStringMap();

    for (const auto& [Key, ReplicatedValue] : ReferencesStringMap)
    {
        ParsedEvent.References[Key] = ReplicatedValue.GetString();
    }

    ParsedEvent.Success = Event.EventValues[2].GetBool();
    ParsedEvent.StatusReason = Event.EventValues[3].GetString();

    // Temporarily set redundant data to not break tests
    ParsedEvent.EventName = Event.EventName;
    ParsedEvent.EventValues = Event.EventValues;
    ParsedEvent.SenderClientId = Event.SenderClientId;

    return ParsedEvent;
}
}
