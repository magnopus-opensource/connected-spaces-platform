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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

#include <memory>

namespace csp::common
{
class IRealtimeEngine;
class IJSScriptRunner;
class LogSystem;
}

/*
    These functions encapsulate the shared functionality between Online and Offline RealtimeEngine.
    A better pattern sould be established to ensure shared funcitonality is reused across both versions,
    as OnlineRealtimeEngine is essentially and extension of Offline, adding replication functionality.
*/

namespace csp::multiplayer
{
class SpaceEntity;
class SpaceTransform;
class AvatarSpaceComponent;
enum class AvatarState;
enum class AvatarPlayMode;

// Finds a space entity in the Entities container. Returns nullptr if the entity is not found.
csp::multiplayer::SpaceEntity* FindSpaceEntity(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name);

// Finds an avatar entity in the Avatar container. Returns nullptr if the avatar is not found.
csp::multiplayer::SpaceEntity* FindSpaceAvatar(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name);

// Finds an object in the Objects container. Returns nullptr if the object is not found.
csp::multiplayer::SpaceEntity* FindSpaceObject(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name);

// Creates a space entity with an avatar component.
std::unique_ptr<csp::multiplayer::SpaceEntity> BuildNewAvatar(const csp::common::String& UserId, csp::common::IRealtimeEngine& RealtimeEngine,
    csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem& LogSystem, uint64_t NetworkId, const csp::common::String& Name,
    const csp::multiplayer::SpaceTransform& Transform, bool IsVisible, uint64_t OwnerId, bool IsTransferable, bool IsPersistent,
    const csp::common::String& AvatarId, csp::multiplayer::AvatarState AvatarState, csp::multiplayer::AvatarPlayMode AvatarPlayMode);

// Checks if an entity exists within the root hierarchy array
bool EntityIsInRootHierarchy(csp::common::IRealtimeEngine& RealtimeEngine, SpaceEntity* Entity);

// Adds or removed entity from the root hierarchy list and calls SpaceEntity::ResolveParentChildRelationship
void ResolveEntityHierarchy(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Entity);

// Unparents any child entities from the entity to be deleted and removed the parent relationship.
void ResolveParentChildForDeletion(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Deletion);

// Ensures compoennts attached to the entitiy are notified of deletion by calling OnLocalDelete.
// It also fires the entity patch callback, notifying clients that the child entities have been reparented.
void StartEntityDeletion(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, csp::multiplayer::SpaceEntity* Entity);
}
