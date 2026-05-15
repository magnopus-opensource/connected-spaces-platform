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
#include "Multiplayer/ComponentSchemaRegistry.h"
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
    OfflineSpaceEntityEventHandler(OfflineRealtimeEngine* entitySystem);

    void OnEvent(const csp::events::Event& inEvent) override;

private:
    OfflineRealtimeEngine* m_entitySystem;
    std::chrono::system_clock::time_point m_lastTickTime;
};

OfflineSpaceEntityEventHandler::OfflineSpaceEntityEventHandler(OfflineRealtimeEngine* entitySystem)
    : m_entitySystem(entitySystem)
    , m_lastTickTime(std::chrono::system_clock::now())
{
}

void OfflineSpaceEntityEventHandler::OnEvent(const csp::events::Event& inEvent)
{
    if (inEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID)
    {
        m_lastTickTime = RealtimeEngineUtils::TickEntityScripts(
            m_entitySystem->GetEntitiesLock(), m_entitySystem->GetRealtimeEngineType(), *m_entitySystem->GetAllEntities(), m_lastTickTime, nullptr);
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
    const CSPSceneDescription& sceneDescription, csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& remoteScriptRunner)
    : OfflineRealtimeEngine(logSystem, remoteScriptRunner)
{
    std::scoped_lock entitiesLocker(m_entitiesLock);

    auto deserializedEntities = sceneDescription.CreateEntities(*this, logSystem, remoteScriptRunner);

    for (size_t i = 0; i < deserializedEntities.Size(); ++i)
    {
        AddEntity(deserializedEntities[i]);
    }

    // Needs to be after all the adds, we normally assume any entity add must parent to an existing entity,
    // but because we load all at once, not here.
    // This smells a bit, why must this be different compared to loading Online state?
    for (SpaceEntity* entity : *GetAllEntities())
    {
        ResolveEntityHierarchy(entity);
    }
}

OfflineRealtimeEngine::OfflineRealtimeEngine(csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& remoteScriptRunner)
    : OfflineRealtimeEngine(logSystem, remoteScriptRunner, {})
{
}

OfflineRealtimeEngine::OfflineRealtimeEngine(csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& remoteScriptRunner,
    const csp::common::Array<ComponentSchema>& additionalComponents)
    : m_logSystem { &logSystem }
    , m_scriptRunner { &remoteScriptRunner }
    , m_componentRegistry { MergeWithLegacyComponents(additionalComponents) }
{
    m_scriptBinding = std::unique_ptr<EntityScriptBinding>(EntityScriptBinding::BindEntitySystem(this, *this->m_logSystem, *this->m_scriptRunner));

    // Is this undefined behaviour? Probably only if we actually use the pointer during construction
    m_eventHandler = std::make_unique<csp::multiplayer::OfflineSpaceEntityEventHandler>(this);

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler.get());
}

OfflineRealtimeEngine::~OfflineRealtimeEngine()
{
    EntityScriptBinding::RemoveBinding(m_scriptBinding.get(), *m_scriptRunner);

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler.get());
}

void csp::multiplayer::OfflineRealtimeEngine::CreateAvatar(const csp::common::String& name, const csp::common::String& userId,
    const csp::multiplayer::SpaceTransform& transform, bool isVisible, csp::multiplayer::AvatarState state, const csp::common::String& avatarId,
    csp::multiplayer::AvatarPlayMode avatarPlayMode, csp::multiplayer::LocomotionModel locomotionModel,
    csp::multiplayer::EntityCreatedCallback callback)
{
    // Some of our interfaces use int64_t ... real bugs here.
    const uint64_t id = NextId();

    auto newAvatar = RealtimeEngineUtils::BuildNewAvatar(
        userId, *this, *m_scriptRunner, *m_logSystem, id, name, transform, isVisible, 0, false, false, avatarId, state, avatarPlayMode, locomotionModel);

    std::scoped_lock entitiesLocker(m_entitiesLock);

    m_entities.Append(newAvatar.get());
    m_avatars.Append(newAvatar.get());

    callback(newAvatar.release());
}

void OfflineRealtimeEngine::CreateEntity(const csp::common::String& name, const csp::multiplayer::SpaceTransform& transform,
    const csp::common::Optional<uint64_t>& parentId, csp::multiplayer::EntityCreatedCallback callback)
{
    // Some of our interfaces use int64_t ... real bugs here.
    uint64_t id = NextId();

    auto* newEntity = new SpaceEntity { this, *m_scriptRunner, m_logSystem, SpaceEntityType::Object, id, name, transform,
        OfflineRealtimeEngine::LocalClientId(), parentId, false, false };

    std::scoped_lock entitiesLocker { m_entitiesLock };

    ResolveEntityHierarchy(newEntity);

    m_entities.Append(newEntity);
    m_objects.Append(newEntity);

    callback(newEntity);
}

void OfflineRealtimeEngine::DestroyEntity(csp::multiplayer::SpaceEntity* entity, csp::multiplayer::CallbackHandler callback)
{
    if (!m_entities.Contains(entity))
    {
        m_logSystem->LogMsg(
            csp::common::LogLevel::Warning, fmt::format("Attempting to delete unknown Entity `{}`. Aborting operation.", entity->GetName()).c_str());
        callback(false);
        return;
    }

    csp::common::List<csp::multiplayer::SpaceEntity*>& avatarOrObjectList = entity->GetEntityType() == SpaceEntityType::Avatar ? m_avatars : m_objects;
    if (!avatarOrObjectList.Contains(entity))
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Fatal,
            fmt::format("Entity `{}` is not in the expected Avatar/Object container. Aborting operation.", entity->GetName()).c_str());
        callback(false);
        return;
    }

    std::scoped_lock entitiesLocker(m_entitiesLock);

    // At time of writing, the only reason to do this is to call cleanup behaviour in ConversationSpaceComponent.
    // This feels like an unfortunate pattern break and an unnecesary concept (OnLocalDelete). An opportunity to
    // refactor. This also happens in OnlineRealtimeEngine.
    auto entityComponents = entity->GetComponents();

    using KeysType = std::remove_pointer_t<decltype(entityComponents)>::MapType::key_type;
    std::unique_ptr<const csp::common::Array<KeysType>> keys(entityComponents->Keys());

    for (size_t i = 0; i < keys->Size(); ++i)
    {
        auto entityComponent = entity->GetComponent((*keys)[i]);
        entityComponent->OnLocalDelete();
    }

    // We want to do heirarchy changes before destroy notification, there _seems_ to be some assertion that this is a platform requirement, although
    // I'm personally dubious. Nonetheless, we have tests that assert this ordering.
    m_rootHierarchyEntities.RemoveItem(entity);
    RealtimeEngineUtils::LocalProcessChildUpdates(*this, m_rootHierarchyEntities, entity);

    if (entity->GetEntityDestroyCallback() != nullptr)
    {
        entity->GetEntityDestroyCallback()(true);
    }

    avatarOrObjectList.RemoveItem(entity);
    RealtimeEngineUtils::RemoveParentChildRelationshipsFromEntity(*this, m_rootHierarchyEntities, entity);
    m_entities.RemoveItem(entity);

    delete (entity);

    callback(true);
}

bool OfflineRealtimeEngine::AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* entity)
{
    if (entity == nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Warning, "Attempting to add null entity to selected entities. Aborting operation.");
        return false;
    }

    if (!m_selectedEntities.Contains(entity))
    {
        m_selectedEntities.Append(entity);
        return true;
    }
    return false;
}

bool OfflineRealtimeEngine::RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* entity)
{
    if (entity == nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Warning, "Attempting to remove null entity from selected entities. Aborting operation.");
        return false;
    }

    if (m_selectedEntities.Contains(entity))
    {
        m_selectedEntities.RemoveItem(entity);
        return true;
    }
    return false;
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceEntity(const csp::common::String& name)
{
    return RealtimeEngineUtils::FindSpaceEntity(*this, name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceEntityById(uint64_t entityId)
{
    return RealtimeEngineUtils::FindSpaceEntityById(*this, entityId);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceAvatar(const csp::common::String& name)
{
    return RealtimeEngineUtils::FindSpaceAvatar(*this, name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::FindSpaceObject(const csp::common::String& name)
{
    return RealtimeEngineUtils::FindSpaceObject(*this, name);
}

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetEntityByIndex(size_t entityIndex) { return m_entities[entityIndex]; }

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetAvatarByIndex(size_t avatarIndex) { return m_avatars[avatarIndex]; }

csp::multiplayer::SpaceEntity* OfflineRealtimeEngine::GetObjectByIndex(size_t objectIndex) { return m_objects[objectIndex]; }

const csp::common::List<csp::multiplayer::SpaceEntity*>* OfflineRealtimeEngine::GetAllEntities() const { return &m_entities; }

size_t OfflineRealtimeEngine::GetNumEntities() const { return m_entities.Size(); }

size_t OfflineRealtimeEngine::GetNumAvatars() const { return m_avatars.Size(); }

size_t OfflineRealtimeEngine::GetNumObjects() const { return m_objects.Size(); }

const csp::common::List<csp::multiplayer::SpaceEntity*>* OfflineRealtimeEngine::GetRootHierarchyEntities() const { return &m_rootHierarchyEntities; }

void OfflineRealtimeEngine::ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* entity)
{
    RealtimeEngineUtils::ResolveEntityHierarchy(*this, m_rootHierarchyEntities, entity);
}

void OfflineRealtimeEngine::FetchAllEntitiesAndPopulateBuffers(const csp::common::String&, csp::common::EntityFetchStartedCallback callback)
{
    // Entities are populated in the constructor, so can immediately call back.
    callback();

    RealtimeEngineUtils::InitialiseEntityScripts(m_entities);
    RealtimeEngineUtils::DetermineScriptOwners(m_entities, OfflineRealtimeEngine::LocalClientId());

    m_entityFetchCompleteCallback(static_cast<uint32_t>(m_entities.Size()));
}

void OfflineRealtimeEngine::LockEntityUpdate() { return m_entitiesLock.lock(); }

bool OfflineRealtimeEngine::TryLockEntityUpdate() { return m_entitiesLock.try_lock(); }

void OfflineRealtimeEngine::UnlockEntityUpdate() { m_entitiesLock.unlock(); }

SpaceEntityStatePatcher* OfflineRealtimeEngine::MakeStatePatcher(csp::multiplayer::SpaceEntity& /*SpaceEntity*/) const { return nullptr; }

ModifiableStatus OfflineRealtimeEngine::IsEntityModifiable(const csp::multiplayer::SpaceEntity* spaceEntity) const
{
    if (spaceEntity->GetLockType() == LockType::UserAgnostic)
    {
        return ModifiableStatus::EntityLocked;
    }
    else
    {
        return ModifiableStatus::Modifiable;
    }
}

const csp::multiplayer::ComponentSchemaRegistry* OfflineRealtimeEngine::GetComponentSchemaRegistry() const { return &m_componentRegistry; }

std::recursive_mutex& OfflineRealtimeEngine::GetEntitiesLock() { return m_entitiesLock; }

uint64_t OfflineRealtimeEngine::LocalClientId() { return csp::common::LocalClientID; }

void OfflineRealtimeEngine::AddEntity(SpaceEntity* entityToAdd)
{
    if (entityToAdd == nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Warning, "Attempting to add null entity. Aborting operation.");
        return;
    }

    std::scoped_lock entitiesLocker(m_entitiesLock);

    if (FindSpaceEntityById(entityToAdd->GetId()) == nullptr)
    {
        m_entities.Append(entityToAdd);

        switch (entityToAdd->GetEntityType())
        {
        case SpaceEntityType::Avatar:
            m_avatars.Append(entityToAdd);
            break;

        case SpaceEntityType::Object:
            m_objects.Append(entityToAdd);
            break;
        }
    }
    else
    {
        m_logSystem->LogMsg(common::LogLevel::Error, "Attempted to add an entity already known to the RealtimeEngine. Aborting operation.");
    }
}
}
