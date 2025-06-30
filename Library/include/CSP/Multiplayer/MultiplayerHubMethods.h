/*
 * Copyright 2025 Magnopus LLC

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

#include <string>
#include <unordered_map>

namespace csp::multiplayer
{

/// @brief Enum used to specify the SignalR method to invoke in the multiplayer connection.
enum class MultiplayerHubMethod
{
    DELETE_OBJECTS,
    GENERATE_OBJECT_IDS,
    GET_CLIENT_ID,
    PAGE_SCOPED_OBJECTS,
    RESET_SCOPES,
    SEND_EVENT_MESSAGE,
    SEND_OBJECT_MESSAGE,
    SEND_OBJECT_NOT_FOUND,
    SEND_OBJECT_PATCH,
    SEND_OBJECT_PATCHES,
    SET_ALLOW_SELF_MESSAGING,
    SET_SCOPES,
    START_LISTENING,
    STOP_LISTENING
};

/// @brief Utility class to map input values from MultiplayerHubMethod to string representations.
struct MultiplayerHubMethodMap : public std::unordered_map<MultiplayerHubMethod, std::string>
{
    explicit MultiplayerHubMethodMap()
    {
        this->insert({ MultiplayerHubMethod::DELETE_OBJECTS, "DeleteObjects" });
        this->insert({ MultiplayerHubMethod::GENERATE_OBJECT_IDS, "GenerateObjectIds" });
        this->insert({ MultiplayerHubMethod::GET_CLIENT_ID, "GetClientId" });
        this->insert({ MultiplayerHubMethod::PAGE_SCOPED_OBJECTS, "PageScopedObjects" });
        this->insert({ MultiplayerHubMethod::RESET_SCOPES, "ResetScopes" });
        this->insert({ MultiplayerHubMethod::SEND_EVENT_MESSAGE, "SendEventMessage" });
        this->insert({ MultiplayerHubMethod::SEND_OBJECT_MESSAGE, "SendObjectMessage" });
        this->insert({ MultiplayerHubMethod::SEND_OBJECT_NOT_FOUND, "SendObjectNotFound" });
        this->insert({ MultiplayerHubMethod::SEND_OBJECT_PATCH, "SendObjectPatch" });
        this->insert({ MultiplayerHubMethod::SEND_OBJECT_PATCHES, "SendObjectPatches" });
        this->insert({ MultiplayerHubMethod::SET_ALLOW_SELF_MESSAGING, "SetAllowSelfMessaging" });
        this->insert({ MultiplayerHubMethod::SET_SCOPES, "SetScopes" });
        this->insert({ MultiplayerHubMethod::START_LISTENING, "StartListening" });
        this->insert({ MultiplayerHubMethod::STOP_LISTENING, "StopListening" });
    }

    ~MultiplayerHubMethodMap() { }

    std::string Get(const MultiplayerHubMethod& Method) const
    {
        if (auto search = this->find(Method); search != this->end())
            return search->second;

        return "";
    }
};

} // namespace csp::multiplayer
