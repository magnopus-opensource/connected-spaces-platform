#include "Multiplayer/RealtimeEngineUtils.h"
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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/Election/ClientElectionManager.h"
#include "Multiplayer/Election/ScopeLeadershipManager.h"
#include <fmt/format.h>

namespace
{
csp::common::String JSONStringFromDeltaTime(double deltaTime) { return fmt::format("{{\"deltaTimeMS\": {}}}", deltaTime).c_str(); }
}

namespace csp::multiplayer::RealtimeEngineUtils
{
csp::common::String ModifiableStatusToString(ModifiableStatus failure)
{
    if (auto it = ModifiableStatusErrors.find(failure); it != ModifiableStatusErrors.end())
    {
        return it->second;
    }
    return "No log specified for modifiable status";
}

csp::multiplayer::SpaceEntity* FindSpaceEntity(csp::common::IRealtimeEngine& realtimeEngine, const csp::common::String& name)
{
    for (size_t i = 0; i < realtimeEngine.GetNumEntities(); ++i)
    {
        if (realtimeEngine.GetEntityByIndex(i)->GetName() == name)
        {
            return realtimeEngine.GetEntityByIndex(i);
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* FindSpaceEntityById(csp::common::IRealtimeEngine& realtimeEngine, uint64_t entityId)
{
    for (size_t i = 0; i < realtimeEngine.GetNumEntities(); ++i)
    {
        if (realtimeEngine.GetEntityByIndex(i)->GetId() == entityId)
        {
            return realtimeEngine.GetEntityByIndex(i);
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* FindSpaceAvatar(csp::common::IRealtimeEngine& realtimeEngine, const csp::common::String& name)
{
    for (size_t i = 0; i < realtimeEngine.GetNumAvatars(); ++i)
    {
        if (realtimeEngine.GetAvatarByIndex(i)->GetName() == name)
        {
            return realtimeEngine.GetAvatarByIndex(i);
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* FindSpaceObject(csp::common::IRealtimeEngine& realtimeEngine, const csp::common::String& name)
{
    for (size_t i = 0; i < realtimeEngine.GetNumObjects(); ++i)
    {
        if (realtimeEngine.GetObjectByIndex(i)->GetName() == name)
        {
            return realtimeEngine.GetObjectByIndex(i);
        }
    }

    return nullptr;
}

std::unique_ptr<csp::multiplayer::SpaceEntity> BuildNewAvatar(const csp::common::String& userId, csp::common::IRealtimeEngine& realtimeEngine,
    csp::common::IJSScriptRunner& scriptRunner, csp::common::LogSystem& logSystem, uint64_t networkId, const csp::common::String& name,
    const csp::multiplayer::SpaceTransform& transform, bool isVisible, uint64_t ownerId, bool isTransferable, bool isPersistent,
    const csp::common::String& avatarId, csp::multiplayer::AvatarState avatarState, csp::multiplayer::AvatarPlayMode avatarPlayMode,
    csp::multiplayer::LocomotionModel locomotionModel)
{
    auto newAvatar = std::unique_ptr<csp::multiplayer::SpaceEntity>(new csp::multiplayer::SpaceEntity(
        &realtimeEngine, scriptRunner, &logSystem, SpaceEntityType::Avatar, networkId, name, transform, ownerId, {}, isTransferable, isPersistent));

    auto* avatarComponent = static_cast<AvatarSpaceComponent*>(newAvatar->AddComponent(ComponentType::AvatarData));
    avatarComponent->SetAvatarId(avatarId);
    avatarComponent->SetState(avatarState);
    avatarComponent->SetAvatarPlayMode(avatarPlayMode);
    avatarComponent->SetLocomotionModel(locomotionModel);
    avatarComponent->SetUserId(userId);
    avatarComponent->SetIsVisible(isVisible);

    return newAvatar;
}

bool EntityIsInRootHierarchy(csp::common::IRealtimeEngine& realtimeEngine, SpaceEntity* entity)
{
    for (size_t i = 0; i < realtimeEngine.GetRootHierarchyEntities()->Size(); ++i)
    {
        if ((*realtimeEngine.GetRootHierarchyEntities())[i]->GetId() == entity->GetId())
        {
            return true;
        }
    }

    return false;
}

void ResolveEntityHierarchy(csp::common::IRealtimeEngine& realtimeEngine, csp::common::List<SpaceEntity*>& rootHierarchyEntities, SpaceEntity* entity)
{
    // Feels weird this not having a mutex lock, relies on the caller setting the entities lock

    if (entity->GetParentId().HasValue())
    {
        for (size_t i = 0; i < rootHierarchyEntities.Size(); ++i)
        {
            if (rootHierarchyEntities[i]->GetId() == entity->GetId())
            {
                rootHierarchyEntities.Remove(i);
                break;
            }
        }
    }
    else
    {
        if (EntityIsInRootHierarchy(realtimeEngine, entity) == false)
        {
            rootHierarchyEntities.Append(entity);
        }
    }

    entity->ResolveParentChildRelationship();
}

void RemoveParentChildRelationshipsFromEntity(
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::List<SpaceEntity*>& rootHierarchyEntities, SpaceEntity* entity)
{
    if (entity->GetParentEntity())
    {
        entity->RemoveAsChildFromParent();
    }

    const auto& childEntities = entity->GetChildEntities()->ToArray();
    for (size_t i = 0; i < childEntities.Size(); ++i)
    {
        entity->RemoveParentFromChildEntity(i);

        ResolveEntityHierarchy(realtimeEngine, rootHierarchyEntities, childEntities[i]);
    }
}

void LocalProcessChildUpdates(
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::List<SpaceEntity*>& rootHierarchyEntities, csp::multiplayer::SpaceEntity* entity)
{
    // Messy, taken from existing cleanup code. Needs a conceptual facelift

    // Manually process the parent updates locally
    // We want this callback to fire before the deletion so clients can react to children first
    auto childrenToUpdate = entity->GetChildEntities()->ToArray();

    for (size_t i = 0; i < childrenToUpdate.Size(); ++i)
    {
        childrenToUpdate[i]->RemoveParentId();
        ResolveEntityHierarchy(realtimeEngine, rootHierarchyEntities, childrenToUpdate[i]);

        if (childrenToUpdate[i]->GetEntityUpdateCallback())
        {
            csp::common::Array<ComponentUpdateInfo> empty;
            childrenToUpdate[i]->GetEntityUpdateCallback()(childrenToUpdate[i], UPDATE_FLAGS_PARENT, empty);
        }
    }
}

// You should lock the entities mutex before calling this, and probably have processed entity operations
void InitialiseEntityScripts(csp::common::List<SpaceEntity*>& entities)
{
    // Register all scripts for import
    for (size_t i = 0; i < entities.Size(); ++i)
    {
        EntityScript& script = entities[i]->GetScript();
        script.RegisterSourceAsModule();
    }

    // Bind and invoke all scripts
    for (size_t i = 0; i < entities.Size(); ++i)
    {
        if (EntityScript& script = entities[i]->GetScript(); script.HasEntityScriptComponent())
        {
            script.Bind();
            script.Invoke();
        }
    }

    // Tell all scripts that all entities are now loaded
    for (size_t i = 0; i < entities.Size(); ++i)
    {
        EntityScript& script = entities[i]->GetScript();
        script.PostMessageToScript(SCRIPT_MSG_ENTITIES_LOADED);
    }
}

// @brief Simple script ownership
//
// Simple MVP script ownership for testing:
// * Everyone 'claims' ownership of scripts on connection
// * Last person to do so 'wins'
//
// @note this does not currently handle when the owner leaves the session
// when the owner will need to be re-assigned, although ownership will also
// be claimed by anyone who interacts with an object
//
void DetermineScriptOwners(const csp::common::List<SpaceEntity*>& entities, uint64_t clientId)
{
    for (size_t i = 0; i < entities.Size(); ++i)
    {
        ClaimScriptOwnership(entities[i], clientId);
    }
}

void ClaimScriptOwnership(SpaceEntity* entity, uint64_t clientId)
{
    EntityScript& script = entity->GetScript();
    script.SetOwnerId(clientId);
}

std::chrono::system_clock::time_point TickEntityScripts(std::recursive_mutex& entitiesLock, csp::common::RealtimeEngineType realtimeEngineType,
    const csp::common::List<SpaceEntity*>& entities, std::chrono::system_clock::time_point lastTickTime,
    csp::common::Optional<csp::multiplayer::ClientElectionManager*> electionManager)
{
    std::scoped_lock entitiesLocker(entitiesLock);

    const auto currentTime = std::chrono::system_clock::now();
    const auto deltaTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTickTime).count();

    const csp::common::String deltaTimeJson = JSONStringFromDeltaTime(static_cast<double>(deltaTimeMs));

    for (size_t i = 0; i < entities.Size(); ++i)
    {
        bool canRunScripts = true;

        // Note that offline realtime engines may always run scripts.
        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            // If this is an online engine with leadership election enabled, then only the script leader may run scripts.
            // If there is no leadership election, then we assume all clients may run scripts.
            csp::multiplayer::ClientElectionManager* electionManagerPtr = electionManager.HasValue() ? *electionManager : nullptr;
            if (electionManagerPtr != nullptr)
            {
                canRunScripts = electionManagerPtr->IsLocalClientLeader();
            }
        }

        if (canRunScripts)
        {
            entities[i]->GetScript().PostMessageToScript(SCRIPT_MSG_ENTITY_TICK, deltaTimeJson);
        }
    }

    return currentTime;
}

std::chrono::system_clock::time_point TickEntityScripts(
    std::recursive_mutex& entitiesLock, const csp::common::List<SpaceEntity*>& entities, std::chrono::system_clock::time_point lastTickTime)
{
    std::scoped_lock entitiesLocker(entitiesLock);

    const auto currentTime = std::chrono::system_clock::now();
    const auto deltaTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTickTime).count();

    const csp::common::String deltaTimeJson = JSONStringFromDeltaTime(static_cast<double>(deltaTimeMs));

    for (size_t i = 0; i < entities.Size(); ++i)
    {
        entities[i]->GetScript().PostMessageToScript(SCRIPT_MSG_ENTITY_TICK, deltaTimeJson);
    }

    return currentTime;
}
}
