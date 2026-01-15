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
#include "CSP/Common/SharedConstants.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/CSPSceneDescription.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Common/UUIDGenerator.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Multiplayer/RealtimeEngineUtils.h"
#include "Multiplayer/Script/EntityScriptBinding.h"

#include "CSP/Common/fmt_Formatters.h"

#include <fmt/format.h>
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
        LastTickTime = RealtimeEngineUtils::TickEntityScripts(
            EntitySystem->GetEntitiesLock(), EntitySystem->GetRealtimeEngineType(), *EntitySystem->GetAllEntities(), LastTickTime, nullptr);
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
        static std::atomic<uint64_t> counter { 1 }; // Start at 1 to be safe as sometimes people inappropriately use 0 to express nullness.
        assert(counter < csp::common::Precision53Bits && "Id's need to be able to be represented at double precision, because of JS bindings.");
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

    // Needs to be after all the adds, we normally assume any entity add must parent to an existing entity,
    // but because we load all at once, not here.
    // This smells a bit, why must this be different compared to loading Online state?
    for (SpaceEntity* Entity : *GetAllEntities())
    {
        ResolveEntityHierarchy(Entity);
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

    std::unique_ptr<csp::multiplayer::SpaceEntity> NewAvatar = RealtimeEngineUtils::BuildNewAvatar(
        UserId, *this, *ScriptRunner, *LogSystem, Id, Name, Transform, IsVisible, 0, false, false, AvatarId, State, AvatarPlayMode);

    std::scoped_lock EntitiesLocker(EntitiesLock);

    Entities.Append(NewAvatar.get());
    Avatars.Append(NewAvatar.get());

    Callback(NewAvatar.release());
}

void OfflineRealtimeEngine::CreateEntity(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& Transform,
    const csp::common::Optional<uint64_t>& ParentID, csp::multiplayer::EntityCreatedCallback Callback)
{
    // Some of our interfaces use int64_t ... real bugs here.
    uint64_t Id = NextId();

    auto* NewEntity = new SpaceEntity { this, *ScriptRunner, LogSystem, SpaceEntityType::Object, Id, Name, Transform,
        OfflineRealtimeEngine::LocalClientId(), ParentID, false, false };

    std::scoped_lock EntitiesLocker { EntitiesLock };

    ResolveEntityHierarchy(NewEntity);

    Entities.Append(NewEntity);
    Objects.Append(NewEntity);

    Callback(NewEntity);
}

void OfflineRealtimeEngine::DestroyEntity(csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::CallbackHandler Callback)
{
    if (!Entities.Contains(Entity))
    {
        LogSystem->LogMsg(
            csp::common::LogLevel::Warning, fmt::format("Attempting to delete unknown Entity `{}`. Aborting operation.", Entity->GetName()).c_str());
        Callback(false);
        return;
    }

    csp::common::List<csp::multiplayer::SpaceEntity*>& AvatarOrObjectList = Entity->GetEntityType() == SpaceEntityType::Avatar ? Avatars : Objects;
    if (!AvatarOrObjectList.Contains(Entity))
    {
        LogSystem->LogMsg(csp::common::LogLevel::Fatal,
            fmt::format("Entity `{}` is not in the expected Avatar/Object container. Aborting operation.", Entity->GetName()).c_str());
        Callback(false);
        return;
    }

    std::scoped_lock EntitiesLocker(EntitiesLock);

    // At time of writing, the only reason to do this is to call cleanup behaviour in ConversationSpaceComponent.
    // This feels like an unfortunate pattern break and an unnecesary concept (OnLocalDelete). An opportunity to
    // refactor. This also happens in OnlineRealtimeEngine.
    auto EntityComponents = Entity->GetComponents();

    using KeysType = std::remove_pointer_t<decltype(EntityComponents)>::MapType::key_type;
    std::unique_ptr<const csp::common::Array<KeysType>> Keys(EntityComponents->Keys());

    for (size_t i = 0; i < Keys->Size(); ++i)
    {
        auto EntityComponent = Entity->GetComponent((*Keys)[i]);
        EntityComponent->OnLocalDelete();
    }

    // We want to do heirarchy changes before destroy notification, there _seems_ to be some assertion that this is a platform requirement, although
    // I'm personally dubious. Nonetheless, we have tests that assert this ordering.
    RootHierarchyEntities.RemoveItem(Entity);
    RealtimeEngineUtils::LocalProcessChildUpdates(*this, RootHierarchyEntities, Entity);

    if (Entity->GetEntityDestroyCallback() != nullptr)
    {
        Entity->GetEntityDestroyCallback()(true);
    }

    AvatarOrObjectList.RemoveItem(Entity);
    RealtimeEngineUtils::RemoveParentChildRelationshipsFromEntity(*this, RootHierarchyEntities, Entity);
    Entities.RemoveItem(Entity);

    delete (Entity);

    Callback(true);
}

bool OfflineRealtimeEngine::AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
{
    if (Entity == nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Warning, "Attempting to add null entity to selected entities. Aborting operation.");
        return false;
    }

    if (!SelectedEntities.Contains(Entity))
    {
        SelectedEntities.Append(Entity);
        return true;
    }
    return false;
}

bool OfflineRealtimeEngine::RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
{
    if (Entity == nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Warning, "Attempting to remove null entity from selected entities. Aborting operation.");
        return false;
    }

    if (SelectedEntities.Contains(Entity))
    {
        SelectedEntities.RemoveItem(Entity);
        return true;
    }
    return false;
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceEntity(const csp::common::String& Name)
{
    return RealtimeEngineUtils::FindSpaceEntity(*this, Name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceEntityById(uint64_t EntityId)
{
    return RealtimeEngineUtils::FindSpaceEntityById(*this, EntityId);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceAvatar(const csp::common::String& Name)
{
    return RealtimeEngineUtils::FindSpaceAvatar(*this, Name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceObject(const csp::common::String& Name)
{
    return RealtimeEngineUtils::FindSpaceObject(*this, Name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetEntityByIndex(size_t EntityIndex) { return Entities[EntityIndex]; }

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetAvatarByIndex(size_t AvatarIndex) { return Avatars[AvatarIndex]; }

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetObjectByIndex(size_t ObjectIndex) { return Objects[ObjectIndex]; }

const csp::common::List<csp::multiplayer::SpaceEntity*>* OfflineRealtimeEngine::GetAllEntities() const { return &Entities; }

size_t OfflineRealtimeEngine::GetNumEntities() const { return Entities.Size(); }

size_t OfflineRealtimeEngine::GetNumAvatars() const { return Avatars.Size(); }

size_t OfflineRealtimeEngine::GetNumObjects() const { return Objects.Size(); }

const csp::common::List<csp::multiplayer::SpaceEntity*>* OfflineRealtimeEngine::GetRootHierarchyEntities() const { return &RootHierarchyEntities; }

void OfflineRealtimeEngine::ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* Entity)
{
    RealtimeEngineUtils::ResolveEntityHierarchy(*this, RootHierarchyEntities, Entity);
}

void OfflineRealtimeEngine::FetchAllEntitiesAndPopulateBuffers(const csp::common::String&, csp::common::EntityFetchStartedCallback Callback)
{
    // Entities are populated in the constructor, so can immediately call back.
    Callback();

    RealtimeEngineUtils::InitialiseEntityScripts(Entities);
    RealtimeEngineUtils::DetermineScriptOwners(Entities, OfflineRealtimeEngine::LocalClientId());

    EntityFetchCompleteCallback(static_cast<uint32_t>(Entities.Size()));
}

void OfflineRealtimeEngine::LockEntityUpdate() { return EntitiesLock.lock(); }

bool OfflineRealtimeEngine::TryLockEntityUpdate() { return EntitiesLock.try_lock(); }

void OfflineRealtimeEngine::UnlockEntityUpdate() { EntitiesLock.unlock(); }

SpaceEntityStatePatcher* OfflineRealtimeEngine::MakeStatePatcher(csp::multiplayer::SpaceEntity& /*SpaceEntity*/) const { return nullptr; }

ModifiableFailure OfflineRealtimeEngine::IsEntityModifiable(const csp::multiplayer::SpaceEntity* SpaceEntity) const
{
    if (SpaceEntity->GetLockType() == LockType::UserAgnostic)
    {
        return ModifiableFailure::EntityLocked;
    }
    else
    {
        return ModifiableFailure::None;
    }
}

std::recursive_mutex& OfflineRealtimeEngine::GetEntitiesLock() { return EntitiesLock; }

uint64_t OfflineRealtimeEngine::LocalClientId() { return csp::common::LocalClientID; }

void OfflineRealtimeEngine::AddEntity(SpaceEntity* EntityToAdd)
{
    if (EntityToAdd == nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Warning, "Attempting to add null entity. Aborting operation.");
        return;
    }

    std::scoped_lock EntitiesLocker(EntitiesLock);

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
        LogSystem->LogMsg(common::LogLevel::Error, "Attempted to add an entity already known to the RealtimeEngine. Aborting operation.");
    }
}
}
