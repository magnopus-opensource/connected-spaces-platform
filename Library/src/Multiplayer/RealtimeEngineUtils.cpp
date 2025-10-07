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
#include <fmt/format.h>

namespace
{
csp::common::String JSONStringFromDeltaTime(double DeltaTime) { return fmt::format("{{\"deltaTimeMS\": {}}}", DeltaTime).c_str(); }
}

namespace csp::multiplayer::RealtimeEngineUtils
{
csp::multiplayer::SpaceEntity* FindSpaceEntity(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name)
{
    for (size_t i = 0; i < RealtimeEngine.GetNumEntities(); ++i)
    {
        if (RealtimeEngine.GetEntityByIndex(i)->GetName() == Name)
        {
            return RealtimeEngine.GetEntityByIndex(i);
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* FindSpaceEntityById(csp::common::IRealtimeEngine& RealtimeEngine, uint64_t EntityId)
{
    for (size_t i = 0; i < RealtimeEngine.GetNumEntities(); ++i)
    {
        if (RealtimeEngine.GetEntityByIndex(i)->GetId() == EntityId)
        {
            return RealtimeEngine.GetEntityByIndex(i);
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* FindSpaceAvatar(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name)
{
    for (size_t i = 0; i < RealtimeEngine.GetNumAvatars(); ++i)
    {
        if (RealtimeEngine.GetAvatarByIndex(i)->GetName() == Name)
        {
            return RealtimeEngine.GetAvatarByIndex(i);
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* FindSpaceObject(csp::common::IRealtimeEngine& RealtimeEngine, const csp::common::String& Name)
{
    for (size_t i = 0; i < RealtimeEngine.GetNumObjects(); ++i)
    {
        if (RealtimeEngine.GetObjectByIndex(i)->GetName() == Name)
        {
            return RealtimeEngine.GetObjectByIndex(i);
        }
    }

    return nullptr;
}

std::unique_ptr<csp::multiplayer::SpaceEntity> BuildNewAvatar(const csp::common::String& UserId, csp::common::IRealtimeEngine& RealtimeEngine,
    csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem& LogSystem, uint64_t NetworkId, const csp::common::String& Name,
    const csp::multiplayer::SpaceTransform& Transform, bool IsVisible, uint64_t OwnerId, bool IsTransferable, bool IsPersistent,
    const csp::common::String& AvatarId, csp::multiplayer::AvatarState AvatarState, csp::multiplayer::AvatarPlayMode AvatarPlayMode)
{
    auto NewAvatar = std::unique_ptr<csp::multiplayer::SpaceEntity>(new csp::multiplayer::SpaceEntity(
        &RealtimeEngine, ScriptRunner, &LogSystem, SpaceEntityType::Avatar, NetworkId, Name, Transform, OwnerId, {}, IsTransferable, IsPersistent));

    auto* AvatarComponent = static_cast<AvatarSpaceComponent*>(NewAvatar->AddComponent(ComponentType::AvatarData));
    AvatarComponent->SetAvatarId(AvatarId);
    AvatarComponent->SetState(AvatarState);
    AvatarComponent->SetAvatarPlayMode(AvatarPlayMode);
    AvatarComponent->SetUserId(UserId);
    AvatarComponent->SetIsVisible(IsVisible);

    return NewAvatar;
}

bool EntityIsInRootHierarchy(csp::common::IRealtimeEngine& RealtimeEngine, SpaceEntity* Entity)
{
    for (size_t i = 0; i < RealtimeEngine.GetRootHierarchyEntities()->Size(); ++i)
    {
        if ((*RealtimeEngine.GetRootHierarchyEntities())[i]->GetId() == Entity->GetId())
        {
            return true;
        }
    }

    return false;
}

void ResolveEntityHierarchy(csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Entity)
{
    // Feels weird this not having a mutex lock, relies on the caller setting the entities lock

    if (Entity->GetParentId().HasValue())
    {
        for (size_t i = 0; i < RootHierarchyEntities.Size(); ++i)
        {
            if (RootHierarchyEntities[i]->GetId() == Entity->GetId())
            {
                RootHierarchyEntities.Remove(i);
                break;
            }
        }
    }
    else
    {
        if (EntityIsInRootHierarchy(RealtimeEngine, Entity) == false)
        {
            RootHierarchyEntities.Append(Entity);
        }
    }

    Entity->ResolveParentChildRelationship();
}

void RemoveParentChildRelationshipsFromEntity(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Entity)
{
    if (Entity->GetParentEntity())
    {
        Entity->RemoveAsChildFromParent();
    }

    const auto& ChildEntities = Entity->GetChildEntities()->ToArray();
    for (size_t i = 0; i < ChildEntities.Size(); ++i)
    {
        Entity->RemoveParentFromChildEntity(i);

        ResolveEntityHierarchy(RealtimeEngine, RootHierarchyEntities, ChildEntities[i]);
    }
}

void LocalProcessChildUpdates(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, csp::multiplayer::SpaceEntity* Entity)
{
    // Messy, taken from existing cleanup code. Needs a conceptual facelift

    // Manually process the parent updates locally
    // We want this callback to fire before the deletion so clients can react to children first
    auto ChildrenToUpdate = Entity->GetChildEntities()->ToArray();

    for (size_t i = 0; i < ChildrenToUpdate.Size(); ++i)
    {
        ChildrenToUpdate[i]->RemoveParentId();
        ResolveEntityHierarchy(RealtimeEngine, RootHierarchyEntities, ChildrenToUpdate[i]);

        if (ChildrenToUpdate[i]->GetEntityUpdateCallback())
        {
            csp::common::Array<ComponentUpdateInfo> Empty;
            ChildrenToUpdate[i]->GetEntityUpdateCallback()(ChildrenToUpdate[i], UPDATE_FLAGS_PARENT, Empty);
        }
    }
}

// You should lock the entities mutex before calling this, and probably have processed entity operations
void InitialiseEntityScripts(csp::common::List<SpaceEntity*>& Entities)
{
    // Register all scripts for import
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        EntityScript& Script = Entities[i]->GetScript();
        Script.RegisterSourceAsModule();
    }

    // Bind and invoke all scripts
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        if (EntityScript& Script = Entities[i]->GetScript(); Script.HasEntityScriptComponent())
        {
            Script.Bind();
            Script.Invoke();
        }
    }

    // Tell all scripts that all entities are now loaded
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        EntityScript& Script = Entities[i]->GetScript();
        Script.PostMessageToScript(SCRIPT_MSG_ENTITIES_LOADED);
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
void DetermineScriptOwners(const csp::common::List<SpaceEntity*>& Entities, uint64_t ClientId)
{
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        ClaimScriptOwnership(Entities[i], ClientId);
    }
}

void ClaimScriptOwnership(SpaceEntity* Entity, uint64_t ClientId)
{
    EntityScript& Script = Entity->GetScript();
    Script.SetOwnerId(ClientId);
}

std::chrono::system_clock::time_point TickEntityScripts(std::recursive_mutex& EntitiesLock, csp::common::RealtimeEngineType RealtimeEngineType,
    const csp::common::List<SpaceEntity*>& Entities, std::chrono::system_clock::time_point LastTickTime,
    csp::common::Optional<csp::multiplayer::ClientElectionManager*> ElectionManager)
{
    std::scoped_lock EntitiesLocker(EntitiesLock);

    const auto CurrentTime = std::chrono::system_clock::now();
    const auto DeltaTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(CurrentTime - LastTickTime).count();

    const csp::common::String DeltaTimeJSON = JSONStringFromDeltaTime(static_cast<double>(DeltaTimeMS));

    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        // Ownership is not a concern for offline realtime engines
        const bool IsNotOnlineEngine = RealtimeEngineType != csp::common::RealtimeEngineType::Online;

        // The script leader is always responsible for running scripts
        bool IsLocalClientScriptLeader = ElectionManager.HasValue() && (*ElectionManager)->IsLocalClientLeader();

        if (IsNotOnlineEngine || IsLocalClientScriptLeader)
        {
            Entities[i]->GetScript().PostMessageToScript(SCRIPT_MSG_ENTITY_TICK, DeltaTimeJSON);
        }
    }

    return CurrentTime;
}
}
