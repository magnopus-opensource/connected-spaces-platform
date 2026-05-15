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

csp::common::String ModifiableStatusToString(ModifiableStatus failure);

// Finds a space entity in the Entities container. Returns nullptr if the entity is not found.
csp::multiplayer::SpaceEntity* FindSpaceEntity(csp::common::IRealtimeEngine& realtimeEngine, const csp::common::String& name);

csp::multiplayer::SpaceEntity* FindSpaceEntityById(csp::common::IRealtimeEngine& realtimeEngine, uint64_t entityId);

// Finds an avatar entity in the Avatar container. Returns nullptr if the avatar is not found.
csp::multiplayer::SpaceEntity* FindSpaceAvatar(csp::common::IRealtimeEngine& realtimeEngine, const csp::common::String& name);

// Finds an object in the Objects container. Returns nullptr if the object is not found.
csp::multiplayer::SpaceEntity* FindSpaceObject(csp::common::IRealtimeEngine& realtimeEngine, const csp::common::String& name);

// Creates a space entity with an avatar component.
std::unique_ptr<csp::multiplayer::SpaceEntity> BuildNewAvatar(const csp::common::String& userId, csp::common::IRealtimeEngine& realtimeEngine,
    csp::common::IJSScriptRunner& scriptRunner, csp::common::LogSystem& logSystem, uint64_t networkId, const csp::common::String& name,
    const csp::multiplayer::SpaceTransform& transform, bool isVisible, uint64_t ownerId, bool isTransferable, bool isPersistent,
    const csp::common::String& avatarId, csp::multiplayer::AvatarState avatarState, csp::multiplayer::AvatarPlayMode avatarPlayMode,
    csp::multiplayer::LocomotionModel locomotionModel);

// Checks if an entity exists within the root hierarchy array
bool EntityIsInRootHierarchy(csp::common::IRealtimeEngine& realtimeEngine, SpaceEntity* entity);

// "Resolves" the entity heirarchy, which is really a bit of an unhelpful catch all term to mean
// "Walk the entity tree, and make sure all our internal buffers are set up to have the right pointers in them".
// Sets the entities in the root entity heirarchy list, as well as calling SpaceEntity::ResolveParentChildRelationship,
// which also "resolves" itself, by setting the `Parent` pointer to the correct entity, and making sure it's list of children is correctly populated.
void ResolveEntityHierarchy(
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::List<SpaceEntity*>& rootHierarchyEntities, SpaceEntity* entity);

// Unparents any child entities from the entity and remove the parent relationship. Need to call this before deleting an entity
void RemoveParentChildRelationshipsFromEntity(
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::List<SpaceEntity*>& rootHierarchyEntities, SpaceEntity* entity);

// Ensures components attached to the entity are notified of deletion by calling OnLocalDelete.
// It also fires the entity patch callback, notifying clients that the child entities have been reparented.
void LocalProcessChildUpdates(
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::List<SpaceEntity*>& rootHierarchyEntities, csp::multiplayer::SpaceEntity* entity);

// You should lock the entities mutex before calling this, and probably have processed entity operations
void InitialiseEntityScripts(csp::common::List<SpaceEntity*>& entities);

// ClientID is the ID that comes from the multiplayerConnection
void DetermineScriptOwners(const csp::common::List<SpaceEntity*>& entities, uint64_t clientId);

// ClientID is the ID that comes from the multiplayerConnection
void ClaimScriptOwnership(SpaceEntity* entity, uint64_t clientId);

// TODO: remove in OF-1785
std::chrono::system_clock::time_point TickEntityScripts(std::recursive_mutex& entitiesLock, csp::common::RealtimeEngineType realtimeEngineType,
    const csp::common::List<SpaceEntity*>& entities, std::chrono::system_clock::time_point lastTickTime,
    csp::common::Optional<csp::multiplayer::ClientElectionManager*> electionManager);

// Returns the current time, meant to be set as LastTickTime. If an offline engine, will not bother checking whether the local client is the leader.
std::chrono::system_clock::time_point TickEntityScripts(
    std::recursive_mutex& entitiesLock, const csp::common::List<SpaceEntity*>& entities, std::chrono::system_clock::time_point lastTickTime);

}
