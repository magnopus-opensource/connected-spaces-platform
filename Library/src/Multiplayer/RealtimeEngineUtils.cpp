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
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/RealtimeEngineUtils.h"

namespace csp::multiplayer
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

std::unique_ptr<csp::multiplayer::SpaceEntity> BuildNewAvatar(const csp::common::String& UserId, csp::common::IRealtimeEngine&,
    csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem& LogSystem, uint64_t NetworkId, const csp::common::String& Name,
    const csp::multiplayer::SpaceTransform& Transform, bool IsVisible, uint64_t OwnerId, bool IsTransferable, bool IsPersistent,
    const csp::common::String& AvatarId, csp::multiplayer::AvatarState AvatarState, csp::multiplayer::AvatarPlayMode AvatarPlayMode)
{
    auto NewAvatar = std::unique_ptr<csp::multiplayer::SpaceEntity>(new csp::multiplayer::SpaceEntity(
        nullptr, ScriptRunner, &LogSystem, SpaceEntityType::Avatar, NetworkId, Name, Transform, OwnerId, IsTransferable, IsPersistent));

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

void ResolveParentChildForDeletion(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, SpaceEntity* Deletion)
{
    if (Deletion->GetParentEntity())
    {
        Deletion->RemoveChildEntities();
    }

    auto ChildEntities = Deletion->GetChildEntities()->ToArray();

    for (size_t i = 0; i < ChildEntities.Size(); ++i)
    {
        Deletion->RemoveParentFromChildEntity(i);

        ResolveEntityHierarchy(RealtimeEngine, RootHierarchyEntities, ChildEntities[i]);
    }
}

void StartEntityDeletion(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::List<SpaceEntity*>& RootHierarchyEntities, csp::multiplayer::SpaceEntity* Entity)
{
    auto EntityComponents = Entity->GetComponents();
    std::unique_ptr<common::Array<uint16_t>> Keys(const_cast<common::Array<uint16_t>*>(EntityComponents->Keys()));

    for (size_t i = 0; i < Keys->Size(); ++i)
    {
        auto EntityComponent = Entity->GetComponent((*Keys)[i]);
        EntityComponent->OnLocalDelete();
    }

    csp::common::Array<ComponentUpdateInfo> Info;

    RootHierarchyEntities.RemoveItem(Entity);

    // Manually process the parent updates locally
    // We want this callback to fire before the deletion so clients can react to children first
    auto ChildrenToUpdate = Entity->GetChildEntities()->ToArray();

    for (size_t i = 0; i < ChildrenToUpdate.Size(); ++i)
    {
        ChildrenToUpdate[i]->RemoveParentId();
        ResolveEntityHierarchy(RealtimeEngine, RootHierarchyEntities, ChildrenToUpdate[i]);

        if (ChildrenToUpdate[i]->GetEntityUpdateCallback())
        {
            ChildrenToUpdate[i]->SetEntityUpdateCallbackParams(ChildrenToUpdate[i], UPDATE_FLAGS_PARENT, Info);
        }
    }
}
}
