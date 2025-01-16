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
#include "CSP/Common/String.h"

namespace csp::events
{

class CSP_API EventId
{
public:
    EventId(const char* EventNameSpace, const char* EventName);

    bool operator==(const EventId& other) const;

#if DEBUG
    char EventNamespaceDebug[64];
    char EventNameDebug[64];
#endif

    size_t EventNamespace;
    size_t EventName;
};

// Pre-defined Global Events
const EventId USERSERVICE_LOGIN_EVENT_ID = EventId("UserService", "Login");
const EventId USERSERVICE_LOGOUT_EVENT_ID = EventId("UserService", "Logout");

const EventId SPACESYSTEM_ENTER_SPACE_EVENT_ID = EventId("SpaceSystem", "Enter");
const EventId SPACESYSTEM_EXIT_SPACE_EVENT_ID = EventId("SpaceSystem", "Exit");

const EventId MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID = EventId("MultiplayerSystem", "Disconnect");

const EventId FOUNDATION_TICK_EVENT_ID = EventId("Foundation", "Tick");

const EventId ENTITYSYSTEM_ADD_ENTITY_EVENT_ID = EventId("EntitySystem", "AddEntity");
const EventId ENTITYSYSTEM_REMOVE_ENTITY_EVENT_ID = EventId("EntitySystem", "RemoveEntity");

} // namespace csp::events
