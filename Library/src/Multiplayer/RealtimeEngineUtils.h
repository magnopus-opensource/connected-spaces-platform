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
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace csp::common
{
class IRealtimeEngine;
class IJSScriptRunner;
class LogSystem;
}

namespace csp::multiplayer
{
class ClientElectionManager;
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
}

namespace csp::multiplayer::RealtimeEngineUtils
{

static const std::unordered_map<ModifiableStatus, csp::common::String> ModifiableStatusErrors {
    { ModifiableStatus::Modifiable, "" },
    { ModifiableStatus::EntityLocked, "Entity is locked" },
    { ModifiableStatus::EntityNotOwnedAndUntransferable, "Entity is not owned by this client and isn't transferable" },
};

csp::common::String ModifiableStatusToString(ModifiableStatus Failure);

// Finds a space entity in the Entities container. Returns nullptr if the entity is not found.
csp::multiplayer::SpaceEntity* FindSpaceEntity(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name);

csp::multiplayer::SpaceEntity* FindSpaceEntityById(csp::common::IRealtimeEngine& RealtimeEngine, uint64_t EntityId);

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

// "Resolves" the entity heirarchy, which is really a bit of an unhelpful catch all term to mean
// "Walk the entity tree, and make sure all our internal buffers are set up to have the right pointers in them".
// Sets the entities in the root entity heirarchy list, as well as calling SpaceEntity::ResolveParentChildRelationship,
// which also "resolves" itself, by setting the `Parent` pointer to the correct entity, and making sure it's list of children is correctly populated.
void ResolveEntityHierarchy(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Entity);

// Unparents any child entities from the entity and remove the parent relationship. Need to call this before deleting an entity
void RemoveParentChildRelationshipsFromEntity(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Entity);

// Ensures components attached to the entity are notified of deletion by calling OnLocalDelete.
// It also fires the entity patch callback, notifying clients that the child entities have been reparented.
void LocalProcessChildUpdates(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, csp::multiplayer::SpaceEntity* Entity);

// You should lock the entities mutex before calling this, and probably have processed entity operations
void InitialiseEntityScripts(csp::common::List<SpaceEntity*>& Entities);

// ClientID is the ID that comes from the multiplayerConnection
void DetermineScriptOwners(const csp::common::List<SpaceEntity*>& Entities, uint64_t ClientId);

// ClientID is the ID that comes from the multiplayerConnection
void ClaimScriptOwnership(SpaceEntity* Entity, uint64_t ClientId);

// TODO: remove in OF-1785
std::chrono::system_clock::time_point TickEntityScripts(std::recursive_mutex& EntitiesLock, csp::common::RealtimeEngineType RealtimeEngineType,
    const csp::common::List<SpaceEntity*>& Entities, std::chrono::system_clock::time_point LastTickTime,
    csp::common::Optional<csp::multiplayer::ClientElectionManager*> ElectionManager);

// Returns the current time, meant to be set as LastTickTime. If an offline engine, will not bother checking whether the local client is the leader.
std::chrono::system_clock::time_point TickEntityScripts(
    std::recursive_mutex& EntitiesLock, const csp::common::List<SpaceEntity*>& Entities, std::chrono::system_clock::time_point LastTickTime);

}
