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
#include <map>
#include <regex>
#include <string>

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

csp::common::String DecodeSequenceKey(const csp::multiplayer::mcs::ItemComponentData& RawValue)
{
    // Sequence keys are URI encoded to support reserved characters.
    return csp::common::Decode::URI(std::get<std::string>(RawValue.GetValue()).c_str());
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
    const mcs::EventMessage& Event, csp::common::LogSystem& LogSystem)
{
    csp::common::AssetDetailBlobChangedNetworkEventData ParsedEvent {};

    ParsedEvent.ChangeType = csp::common::EAssetChangeType::Invalid;

    const auto& Components = Event.GetComponents().value();

    auto ChangeTypeV = std::get<std::optional<uint16_t>>(Components.at(1).GetValue());

    if (ChangeTypeV.has_value() == false)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "AssetDetailChangedEvent - AssetChangeType wasn't specified.");
    }
    else
    {
        uint16_t ChangeType = ChangeTypeV.value();

        if (ChangeType < static_cast<uint16_t>(csp::common::EAssetChangeType::Num))
        {
            ParsedEvent.ChangeType = static_cast<csp::common::EAssetChangeType>(ChangeType);
        }
        else
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "AssetDetailChangedEvent - AssetChangeType out of range of acceptable enum values.");
        }
    }

    ParsedEvent.AssetId = std::get<std::string>(Components.at(100).GetValue()).c_str();
    ParsedEvent.Version = std::get<std::string>(Components.at(101).GetValue()).c_str();
    ParsedEvent.AssetType = csp::systems::ConvertDTOAssetDetailType(std::get<std::string>(Components.at(102).GetValue()).c_str());
    ParsedEvent.AssetCollectionId = std::get<std::string>(Components.at(200).GetValue()).c_str();

    return ParsedEvent;
}

csp::common::ConversationNetworkEventData DeserializeConversationEvent(const mcs::EventMessage& Event, csp::common::LogSystem&)
{
    csp::common::ConversationNetworkEventData ParsedEvent {};

    const auto& Components = Event.GetComponents().value();

    ParsedEvent.MessageType = static_cast<ConversationEventType>(std::get<uint64_t>(Components.at(0).GetValue()));
    ParsedEvent.MessageInfo.ConversationId = std::get<std::string>(Components.at(1).GetValue()).c_str();
    ParsedEvent.MessageInfo.CreatedTimestamp = std::get<std::string>(Components.at(2).GetValue()).c_str();
    ParsedEvent.MessageInfo.EditedTimestamp = std::get<std::string>(Components.at(3).GetValue()).c_str();
    ParsedEvent.MessageInfo.UserId = std::get<std::string>(Components.at(4).GetValue()).c_str();
    ParsedEvent.MessageInfo.Message = std::get<std::string>(Components.at(5).GetValue()).c_str();
    ParsedEvent.MessageInfo.MessageId = std::get<std::string>(Components.at(6).GetValue()).c_str();

    return ParsedEvent;
}

csp::common::AccessControlChangedNetworkEventData DeserializeAccessControlChangedEvent(
    const mcs::EventMessage& Event, csp::common::LogSystem& LogSystem)
{
    csp::common::AccessControlChangedNetworkEventData ParsedEvent {};

    /*
     * | Name              | Component ID | Type         | Notes (units, ranges)                                                     |
     * |-------------------|--------------|--------------|---------------------------------------------------------------------------|
     * | **SpaceId**       | 1            | String       | Id of the space that has updated permissions                              |
     * | **UserRoles**     | 100          | String Array | Array of user permissions (viewer,creator,owner) that belongs to the user |
     * | **ChangeType**    | 101          | String       | Created, Updated, Removed                                                 |
     * | **UserId**        | 102          | String       | The userId that was changed                                               |
     * |-------------------|--------------|--------------|---------------------------------------------------------------------------|
     */

    const uint16_t SPACE_ID = 1;
    const uint16_t GROUP_ROLES_ID = 100;
    const uint16_t CHANGE_TYPE_ID = 101;
    const uint16_t USER_ID = 102;

    const auto& Components = Event.GetComponents().value();

    {
        ParsedEvent.SpaceId = std::get<std::string>(Components.at(SPACE_ID).GetValue()).c_str();
    }

    {
        auto UserRoles = std::get<std::vector<std::string>>(Components.at(GROUP_ROLES_ID).GetValue());
        int i = 0;
        ParsedEvent.UserRoles = csp::common::Array<csp::systems::SpaceUserRole>(UserRoles.size());

        for (const auto& RoleValue : UserRoles)
        {
            csp::systems::SpaceUserRole NewRole = csp::systems::SpaceUserRole::Invalid;

            if (RoleValue == "viewer")
            {
                NewRole = csp::systems::SpaceUserRole::User;
            }
            else if (RoleValue == "creator")
            {
                NewRole = csp::systems::SpaceUserRole::Moderator;
            }
            else if (RoleValue == "owner")
            {
                NewRole = csp::systems::SpaceUserRole::Owner;
            }
            else
            {
                LogSystem.LogMsg(
                    csp::common::LogLevel::Error, "UserPermissionsChangedEvent - Detected an unsupported role type. Defaulting to Invalid role.");
            }

            ParsedEvent.UserRoles[i++] = NewRole;
        }
    }

    {
        const csp::common::String ChangeTypeString = std::get<std::string>(Components.at(CHANGE_TYPE_ID).GetValue()).c_str();

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
        ParsedEvent.UserId = std::get<std::string>(Components.at(USER_ID).GetValue()).c_str();
    }

    return ParsedEvent;
}

csp::common::SequenceChangedNetworkEventData DeserializeSequenceChangedEvent(const mcs::EventMessage& Event, csp::common::LogSystem& LogSystem)
{
    csp::common::SequenceChangedNetworkEventData ParsedEvent {};

    const auto& Components = Event.GetComponents().value();

    auto UpdateTypeV = std::get<std::optional<uint16_t>>(Components.at(1).GetValue());

    if (UpdateTypeV.has_value() == false)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "SequenceChangeEvent - SequenceUpdateType wasn't specified.");
    }
    else
    {
        uint16_t UpdateType = UpdateTypeV.value();
        ParsedEvent.UpdateType = ESequenceUpdateIntToUpdateType(UpdateType, LogSystem);
    }

    ParsedEvent.Key = DecodeSequenceKey(Components.at(100).GetValue());

    // Optional parameter for when a key is changed
    if (Components.size() == 3 && std::holds_alternative<std::string>(Components.at(101).GetValue()))
    {
        // Sequence keys are URI encoded to support reserved characters.
        ParsedEvent.NewKey = csp::common::Decode::URI(std::get<std::string>(Components.at(101).GetValue()).c_str());
    }

    return ParsedEvent;
}

csp::common::AsyncCallCompletedEventData DeserializeAsyncCallCompletedEvent(const mcs::EventMessage& Event, csp::common::LogSystem&)
{
    csp::common::AsyncCallCompletedEventData ParsedEvent {};

    const auto& Components = Event.GetComponents().value();

    ParsedEvent.OperationName = std::get<std::string>(Components.at(1).GetValue()).c_str();

    const auto& ReferencesStringMap = std::get<std::map<std::string, mcs::ItemComponentData>>(Components.at(2).GetValue());

    for (const auto& [Key, Component] : ReferencesStringMap)
    {
        ParsedEvent.References[Key.c_str()] = std::get<std::string>(Component.GetValue()).c_str();
    }

    ParsedEvent.Success = std::get<std::optional<bool>>(Components.at(3).GetValue()).value_or(false);
    ParsedEvent.StatusReason = std::get<std::string>(Components.at(4).GetValue()).c_str();

    return ParsedEvent;
}
}
