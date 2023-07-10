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


#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"


/// <summary>
/// Creates an instance of `MultiplayerConnection` on the heap and returns it.
/// Queues a deletion of the connection to be executed after the test exits, unless otherwise specified.
/// </summary>
csp::multiplayer::MultiplayerConnection* CreateMultiplayerConnection(const csp::common::String& SpaceId, bool ShouldPushCleanupFunction = true);

void Disconnect(csp::multiplayer::MultiplayerConnection* Connection);

/// <summary>
/// Calls `Connect` and `InitialiseConnection` on the provided `MultiplayerConnection` instance.
/// Queues a call to `Disconnect` to be executed after the test exits, unless otherwise specified.
/// </summary>
void Connect(csp::multiplayer::MultiplayerConnection* Connection, bool ShouldPushCleanupFunction = true);

void DeleteEntity(csp::multiplayer::SpaceEntitySystem* EntitySystem, csp::multiplayer::SpaceEntity* Entity);

/// <summary>
/// Creates an instance of an Object type `SpaceEntity` on the heap and returns it.
/// Queues a call to `DeleteEntity` to be executed after the test exits, unless otherwise specified.
/// </summary>
csp::multiplayer::SpaceEntity* CreateObject(csp::multiplayer::SpaceEntitySystem* EntitySystem,
											const csp::common::String& Name,
											csp::common::Optional<csp::multiplayer::SpaceTransform> Transform = nullptr,
											bool ShouldPushCleanupFunction									  = true);