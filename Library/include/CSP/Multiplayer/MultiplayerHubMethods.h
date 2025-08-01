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

#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"

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
    STOP_LISTENING,
    ON_OBJECT_MESSAGE,
    ON_OBJECT_PATCH,
    ON_REQUEST_TO_SEND_OBJECT,
    ON_REQUEST_TO_DISCONNECT
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
        this->insert({ MultiplayerHubMethod::ON_OBJECT_MESSAGE, "OnObjectMessage" });
        this->insert({ MultiplayerHubMethod::ON_OBJECT_PATCH, "OnObjectPatch" });
        this->insert({ MultiplayerHubMethod::ON_REQUEST_TO_SEND_OBJECT, "OnRequestToSendObject" });
        this->insert({ MultiplayerHubMethod::ON_REQUEST_TO_DISCONNECT, "OnRequestToDisconnect" });
    }

    ~MultiplayerHubMethodMap() { }

    std::string Get(const MultiplayerHubMethod& Method) const
    {
        if (auto search = this->find(Method); search != this->end())
            return search->second;

        return "";
    }

    /// @brief Validates that all required multiplayer hub methods are available.
    /// This function compares the provided array of method names (`MethodNames`) against a set of available multiplayer hub methods.
    /// It ensures that every method in the input array exists in the available set.
    /// @param MethodNames An array of method names that are expected to be present.
    /// @return bool : true if all available in-use multiplayer hub methods match, false otherwise.
    bool CheckPrerequisites(const csp::common::Array<csp::common::String>& MethodNames) const;
};

} // namespace csp::multiplayer
