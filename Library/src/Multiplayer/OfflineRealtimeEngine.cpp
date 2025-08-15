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

#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/CSPSceneDescription.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Common/UUIDGenerator.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Multiplayer/RealtimeEngineUtils.h"
#include "Multiplayer/Script/EntityScriptBinding.h"

#include <unordered_set>

namespace csp::multiplayer
{

// Handler to listen to tick events, and do the script update
class OfflineSpaceEntityEventHandler : public csp::events::EventListener
{
public:
    OfflineSpaceEntityEventHandler(OfflineRealtimeEngine* EntitySystem);

    void OnEvent(const csp::events::Event& InEvent) override;

private:
    OfflineRealtimeEngine* EntitySystem;
    std::chrono::system_clock::time_point LastTickTime;
};

OfflineSpaceEntityEventHandler::OfflineSpaceEntityEventHandler(OfflineRealtimeEngine* EntitySystem)
    : EntitySystem(EntitySystem)
    , LastTickTime(std::chrono::system_clock::now())
{
}

void OfflineSpaceEntityEventHandler::OnEvent(const csp::events::Event& InEvent)
{
    if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID)
    {
        LastTickTime = RealtimeEngineUtils::TickEntityScripts(EntitySystem->GetEntitiesLock(), EntitySystem->GetRealtimeEngineType(),
            OfflineRealtimeEngine::LocalClientId(), *EntitySystem->GetAllEntities(), LastTickTime);
    }
}

csp::common::RealtimeEngineType OfflineRealtimeEngine::GetRealtimeEngineType() const { return csp::common::RealtimeEngineType::Offline; }

namespace
{
    // This was initially just a random number, however, we need keep the ID's low
    // This is because of a gnarly latent bug with EntityID's. The scripting system (QuickJS currently)
    // binds EntityID's to native JS `numbers`, which are 64 bit floating points.
    // Anything too high precision will not be bound properly by that ... it's a proper problem, we just havn't
    // hit it because we normally keep our ID's in reasonably high-accuracy number space (relatively) near to zero.
    // If you're worrying how much to wonder about this, I think it's 9,007,199,254,740,991, (2^53 - 1) where doubles stop being able to represent
    // ints 1:1.
    uint64_t NextId()
    {
        static std::atomic<uint64_t> counter { 0 };
        static constexpr std::uint64_t Precision53Bits = 9007199254740991;
        assert(counter < Precision53Bits && "Id's need to be able to be represented at double precision, because of JS bindings.");
        return counter++;
    }

}

OfflineRealtimeEngine::OfflineRealtimeEngine(
    const CSPSceneDescription& SceneDescription, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner)
    : OfflineRealtimeEngine(LogSystem, RemoteScriptRunner)
{
    std::scoped_lock EntitiesLocker(EntitiesLock);

    auto DeserializedEntities = SceneDescription.CreateEntities(*this, LogSystem, RemoteScriptRunner);

    for (size_t i = 0; i < DeserializedEntities.Size(); ++i)
    {
        AddEntity(DeserializedEntities[i]);
    }
}

OfflineRealtimeEngine::OfflineRealtimeEngine(csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner)
    : LogSystem { &LogSystem }
    , ScriptRunner { &RemoteScriptRunner }
{
    ScriptBinding = EntityScriptBinding::BindEntitySystem(this, *this->LogSystem, *this->ScriptRunner);

    // Is this undefined behaviour? Probably only if we actually use the pointer during construction
    EventHandler = std::make_unique<csp::multiplayer::OfflineSpaceEntityEventHandler>(this);

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler.get());
}

OfflineRealtimeEngine::~OfflineRealtimeEngine()
{
    EntityScriptBinding::RemoveBinding(ScriptBinding, *ScriptRunner);

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler.get());
}

void csp::multiplayer::OfflineRealtimeEngine::CreateAvatar(const csp::common::String& Name, const csp::common::String& UserId,
    const csp::multiplayer::SpaceTransform& Transform, bool IsVisible, csp::multiplayer::AvatarState State, const csp::common::String& AvatarId,
    csp::multiplayer::AvatarPlayMode AvatarPlayMode, csp::multiplayer::EntityCreatedCallback Callback)
{
    // Some of our interfaces use int64_t ... real bugs here.
    const uint64_t Id = NextId();

    std::unique_ptr<csp::multiplayer::SpaceEntity> NewAvatar
        = BuildNewAvatar(UserId, *this, *ScriptRunner, *LogSystem, Id, Name, Transform, IsVisible, 0, false, false, AvatarId, State, AvatarPlayMode);

    std::scoped_lock EntitiesLocker(EntitiesLock);

    Entities.Append(NewAvatar.get());
    Avatars.Append(NewAvatar.get());

    auto* Avatar = NewAvatar.release();

    Avatar->ApplyLocalPatch(false, false);

    if (SpaceEntityCreatedCallback)
    {
        SpaceEntityCreatedCallback(NewAvatar.get());
    }

    Callback(NewAvatar.release());
}

void OfflineRealtimeEngine::CreateEntity(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& Transform,
    const csp::common::Optional<uint64_t>& ParentID, csp::multiplayer::EntityCreatedCallback Callback)
{
    // Some of our interfaces use int64_t ... real bugs here.
    uint64_t Id = NextId();
    // Client id doesnt matter for single player
    uint64_t ClientId = 0;

    auto* NewEntity
        = new SpaceEntity { this, *ScriptRunner, LogSystem, SpaceEntityType::Object, Id, Name, Transform, ClientId, ParentID, false, false };

    std::scoped_lock EntitiesLocker { EntitiesLock };

    ResolveEntityHierarchy(NewEntity);

    Entities.Append(NewEntity);
    Objects.Append(NewEntity);

    if (SpaceEntityCreatedCallback)
    {
        SpaceEntityCreatedCallback(NewEntity);
    }

    Callback(NewEntity);
}

void OfflineRealtimeEngine::AddEntity(SpaceEntity* EntityToAdd)
{
    std::scoped_lock EntitiesLocker(EntitiesLock);
    AddPendingEntity(EntityToAdd);
}

void OfflineRealtimeEngine::DestroyEntity(csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::CallbackHandler Callback)
{
    std::scoped_lock EntitiesLocker(EntitiesLock);

    StartEntityDeletion(*this, RootHierarchyEntities, Entity);

    RemovePendingEntity(Entity);

    Callback(true);
}

void OfflineRealtimeEngine::SetEntityCreatedCallback(csp::multiplayer::EntityCreatedCallback Callback)
{
    if (SpaceEntityCreatedCallback)
    {
        LogSystem->LogMsg(common::LogLevel::Warning, "OfflineRealtimeEngine has already been set. Previous callback overwritten.");
    }

    SpaceEntityCreatedCallback = std::move(Callback);
}

bool OfflineRealtimeEngine::AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
{
    if (!SelectedEntities.Contains(Entity))
    {
        SelectedEntities.Append(Entity);
        return true;
    }
    return false;
}

bool OfflineRealtimeEngine::RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
{
    if (SelectedEntities.Contains(Entity))
    {
        SelectedEntities.RemoveItem(Entity);
        return true;
    }
    return false;
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceEntity(const csp::common::String& Name)
{
    return csp::multiplayer::FindSpaceEntity(*this, Name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceEntityById(uint64_t EntityId)
{
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        if (Entities[i]->GetId() == EntityId)
        {
            return Entities[i];
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceAvatar(const csp::common::String& Name)
{
    for (size_t i = 0; i < Avatars.Size(); ++i)
    {
        if (Avatars[i]->GetName() == Name)
        {
            return Avatars[i];
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceObject(const csp::common::String& Name)
{
    for (size_t i = 0; i < Objects.Size(); ++i)
    {
        if (Objects[i]->GetName() == Name)
        {
            return Objects[i];
        }
    }

    return nullptr;
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetEntityByIndex(size_t EntityIndex) { return Entities[EntityIndex]; }

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetAvatarByIndex(size_t AvatarIndex) { return Avatars[AvatarIndex]; }

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetObjectByIndex(size_t ObjectIndex) { return Objects[ObjectIndex]; }

size_t OfflineRealtimeEngine::GetNumEntities() const { return Entities.Size(); }

size_t OfflineRealtimeEngine::GetNumAvatars() const { return Avatars.Size(); }

size_t OfflineRealtimeEngine::GetNumObjects() const { return Objects.Size(); }

const csp::common::List<csp::multiplayer::SpaceEntity*>* OfflineRealtimeEngine::GetRootHierarchyEntities() const { return &RootHierarchyEntities; }

void OfflineRealtimeEngine::ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* Entity)
{
    csp::multiplayer::ResolveEntityHierarchy(*this, RootHierarchyEntities, Entity);
}

void OfflineRealtimeEngine::QueueEntityUpdate(csp::multiplayer::SpaceEntity* Entity) { EntitiesToUpdate->insert(Entity); }

void OfflineRealtimeEngine::ProcessPendingEntityOperations()
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    // Process any changes that have been made to entities.
    for (const auto& Entity : *EntitiesToUpdate)
    {
        Entity->ApplyLocalPatch(true, false);
    }

    EntitiesToUpdate->clear();
}

void OfflineRealtimeEngine::FetchAllEntitiesAndPopulateBuffers(const csp::common::String&, csp::common::EntityFetchStartedCallback Callback)
{
    // Entities are populated in the constructor, so can immediately call back.
    Callback();

    // ClientID dosen't matter
    static constexpr uint64_t ClientId = 0;

    InitialiseEntityScripts(Entities);
    DetermineScriptOwners(Entities, ClientId);

    EntityFetchCompleteCallback(static_cast<uint32_t>(Entities.Size()));
}

void OfflineRealtimeEngine::LockEntityUpdate() { return EntitiesLock.lock(); }

bool OfflineRealtimeEngine::TryLockEntityUpdate() { return EntitiesLock.try_lock(); }

void OfflineRealtimeEngine::UnlockEntityUpdate() { EntitiesLock.unlock(); }

OfflineRealtimeEngine::OfflineRealtimeEngine(
    const CSPSceneDescription& SceneDescription, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner)
    : OfflineRealtimeEngine(LogSystem, RemoteScriptRunner)
{
    std::scoped_lock EntitiesLocker(EntitiesLock);

    auto DeserializedEntities = SceneDescription.CreateEntities(*this, LogSystem, RemoteScriptRunner);

    for (size_t i = 0; i < DeserializedEntities.Size(); ++i)
    {
        AddPendingEntity(DeserializedEntities[i]);
    }
}

OfflineRealtimeEngine::OfflineRealtimeEngine(csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner)
    : LogSystem { &LogSystem }
    , ScriptRunner { &RemoteScriptRunner }
    , EntitiesToUpdate { std::make_unique<std::set<csp::multiplayer::SpaceEntity*>>() }
{
    ScriptBinding = EntityScriptBinding::BindEntitySystem(this, *this->LogSystem, *this->ScriptRunner);

    // Is this undefined behaviour? Probably only if we actually use the pointer during construction
    EventHandler = std::make_unique<csp::multiplayer::OfflineSpaceEntityEventHandler>(this);

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler.get());
    csp::events::EventSystem::Get().RegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler.get());
}

OfflineRealtimeEngine::~OfflineRealtimeEngine()
{
    EntityScriptBinding::RemoveBinding(ScriptBinding, *ScriptRunner);

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler.get());
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler.get());
}

csp::common::IRealtimeEngine::SpaceEntityList& OfflineRealtimeEngine::GetAllEntities() { return Entities; }

std::recursive_mutex& OfflineRealtimeEngine::GetEntitiesLock() { return EntitiesLock; }

void OfflineRealtimeEngine::AddPendingEntity(SpaceEntity* EntityToAdd)
{
    if (FindSpaceEntityById(EntityToAdd->GetId()) == nullptr)
    {
        Entities.Append(EntityToAdd);

        switch (EntityToAdd->GetEntityType())
        {
        case SpaceEntityType::Avatar:
            Avatars.Append(EntityToAdd);
            break;

        case SpaceEntityType::Object:
            Objects.Append(EntityToAdd);
            break;
        }
    }
    else
    {
        LogSystem->LogMsg(common::LogLevel::Error, "Attempted to add a pending entity that we already have!");
    }
}

void OfflineRealtimeEngine::RemovePendingEntity(SpaceEntity* EntityToRemove)
{
    assert(Entities.Contains(EntityToRemove));

    switch (EntityToRemove->GetEntityType())
    {
    case SpaceEntityType::Avatar:
        assert(Avatars.Contains(EntityToRemove));
        Avatars.RemoveItem(EntityToRemove);
        break;

    case SpaceEntityType::Object:
        assert(Objects.Contains(EntityToRemove));
        Objects.RemoveItem(EntityToRemove);
        break;

    default:
        assert(false && "Unhandled entity type encountered during its destruction!");
        break;
    }

    RootHierarchyEntities.RemoveItem(EntityToRemove);
    ResolveParentChildForDeletion(*this, RootHierarchyEntities, EntityToRemove);

    Entities.RemoveItem(EntityToRemove);

    delete (EntityToRemove);
}
}
