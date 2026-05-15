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
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"

#include "CSP/Common/List.h"
#include "CSP/Common/LoginState.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "MCS/MCSTypes.h"
#include "Multiplayer/ComponentSchemaRegistry.h"
#include "Multiplayer/Election/ClientElectionManager.h"
#include "Multiplayer/Election/ScopeLeadershipManager.h"
#include "Multiplayer/MultiplayerConstants.h"
#include "Multiplayer/RealtimeEngineUtils.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "Multiplayer/SignalR/ISignalRConnection.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include "Multiplayer/SpaceEntityStatePatcher.h"
#include "RealtimeEngineUtils.h"
#include "SignalRSerializer.h"
#ifdef CSP_WASM
#include "Multiplayer/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.h"
#else
#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
#endif

#include <chrono>
#include <exception>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <unordered_set>
#include <utility>

using namespace std::chrono_literals;

namespace
{

const csp::common::String SequenceTypeName = "EntityHierarchy";

uint64_t ParseGenerateObjectIDsResult(const signalr::value& result, csp::common::LogSystem& logSystem)
{
    uint64_t entityId = 0;

    if (result.is_array())
    {
        const std::vector<signalr::value>& ids = result.as_array();

        // GenerateObjectIds can in theory create multiple Ids at once,
        // but we just assume one for now
        for (const signalr::value& idValue : ids)
        {
            if (idValue.is_uinteger())
            {
                logSystem.LogMsg(csp::common::LogLevel::Verbose, fmt::format("Entity Id={}", idValue.as_uinteger()).c_str());
                entityId = idValue.as_uinteger();
                break;
            }

            assert(false && "Unsupported Entity Id type!");
        }
    }
    else
    {
        logSystem.LogMsg(csp::common::LogLevel::Verbose, "Recieved an ID result not formatted as an array");
    }

    return entityId;
}
} // namespace

template class csp::common::List<csp::multiplayer::SpaceEntity*>;

namespace csp::multiplayer
{

constexpr uint64_t ENTITY_PAGE_LIMIT = 100;

class SpaceEntityEventHandler : public csp::events::EventListener
{
public:
    SpaceEntityEventHandler(OnlineRealtimeEngine* entitySystem);

    void OnEvent(const csp::events::Event& inEvent) override;

private:
    OnlineRealtimeEngine* m_entitySystem;
};

SpaceEntityEventHandler::SpaceEntityEventHandler(OnlineRealtimeEngine* entitySystem)
    : m_entitySystem(entitySystem)
{
}

void SpaceEntityEventHandler::OnEvent(const csp::events::Event& inEvent)
{
    if (inEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && m_entitySystem->GetMultiplayerConnectionInstance() != nullptr
        && m_entitySystem->GetMultiplayerConnectionInstance()->IsConnected())
    {
        m_entitySystem->TickEntities();
    }
}

std::map<uint64_t, signalr::value> GetEntityTransformComponents(const SpaceEntity* inEntity)
{
    const SpaceTransform transform(inEntity->GetTransform());
    const std::vector<signalr::value> position { transform.Position.X, transform.Position.Y, transform.Position.Z };
    const std::vector<signalr::value> rotation { transform.Rotation.X, transform.Rotation.Y, transform.Rotation.Z, transform.Rotation.W };
    const std::vector<signalr::value> scale { transform.Scale.X, transform.Scale.Y, transform.Scale.Z };

    std::map<uint64_t, signalr::value> components { {
                                                        ENTITY_POSITION,
                                                        std::vector<signalr::value> {
                                                            static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_FLOAT_ARRAY),
                                                            std::vector { signalr::value(position) },
                                                        },
                                                    },
        {
            ENTITY_ROTATION,
            std::vector<signalr::value> {
                static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_FLOAT_ARRAY),
                std::vector { signalr::value(rotation) },
            },
        },
        {
            ENTITY_SCALE,
            std::vector<signalr::value> {
                static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_FLOAT_ARRAY),
                std::vector { signalr::value(scale) },
            },
        } };

    return components;
}

class DirtyComponent;

using namespace std::chrono;

OnlineRealtimeEngine::OnlineRealtimeEngine()
    : m_entitiesLock(new std::recursive_mutex)
    , m_multiplayerConnectionInst(nullptr)
    , m_logSystem(nullptr)
    , m_scriptBinding(nullptr)
    , m_eventHandler(nullptr)
    , m_electionManager(nullptr)
    , m_tickEntitiesLock(new std::recursive_mutex)
    , m_pendingAdds(nullptr)
    , m_pendingRemoves(nullptr)
    , m_pendingOutgoingUpdateUniqueSet(nullptr)
    , m_pendingIncomingUpdates(nullptr)
    , m_enableEntityTick(false)
    , m_lastTickTime(std::chrono::system_clock::now())
    , m_entityPatchRate(90)
    , m_scriptRunner(nullptr)
    , m_networkEventBus(nullptr)
{
}

OnlineRealtimeEngine::OnlineRealtimeEngine(MultiplayerConnection& inMultiplayerConnection, csp::common::LogSystem& logSystem,
    csp::multiplayer::NetworkEventBus& networkEventBus, csp::common::IJSScriptRunner& scriptRunner)
    : OnlineRealtimeEngine(inMultiplayerConnection, logSystem, networkEventBus, scriptRunner, { })
{
}

OnlineRealtimeEngine::OnlineRealtimeEngine(MultiplayerConnection& inMultiplayerConnection, csp::common::LogSystem& logSystem,
    csp::multiplayer::NetworkEventBus& networkEventBus, csp::common::IJSScriptRunner& scriptRunner,
    const csp::common::Array<ComponentSchema>& additionalComponents)
    : m_entitiesLock(new std::recursive_mutex)
    , m_multiplayerConnectionInst(&inMultiplayerConnection)
    , m_logSystem(&logSystem)
    , m_eventHandler(new SpaceEntityEventHandler(this))
    , m_electionManager(nullptr)
    , m_tickEntitiesLock(new std::recursive_mutex)
    , m_pendingAdds(new (std::deque<csp::multiplayer::SpaceEntity*>))
    , m_pendingRemoves(new (std::deque<csp::multiplayer::SpaceEntity*>))
    , m_pendingOutgoingUpdateUniqueSet(new (std::set<csp::multiplayer::SpaceEntity*>))
    , m_pendingIncomingUpdates(new (PatchMessageQueue))
    , m_enableEntityTick(false)
    , m_lastTickTime(std::chrono::system_clock::now())
    , m_entityPatchRate(90)
    , m_scriptRunner(&scriptRunner)
    , m_networkEventBus(&networkEventBus)
    , m_componentRegistry { MergeWithLegacyComponents(additionalComponents) }
{
    m_scriptBinding = std::unique_ptr<EntityScriptBinding>(EntityScriptBinding::BindEntitySystem(this, *this->m_logSystem, *this->m_scriptRunner));

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler);
}

OnlineRealtimeEngine::~OnlineRealtimeEngine()
{
    DisableLeaderElection();
    LocalDestroyAllEntities();

    EntityScriptBinding::RemoveBinding(m_scriptBinding.get(), *m_scriptRunner);

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler);

    delete (m_eventHandler);

    delete (m_tickEntitiesLock);
    delete (m_entitiesLock);

    delete (m_pendingAdds);
    delete (m_pendingRemoves);
    delete (m_pendingOutgoingUpdateUniqueSet);
    delete (m_pendingIncomingUpdates);
}

std::deque<csp::multiplayer::SpaceEntity*>* OnlineRealtimeEngine::GetPendingAdds() { return m_pendingAdds; }

MultiplayerConnection* OnlineRealtimeEngine::GetMultiplayerConnectionInstance() const { return m_multiplayerConnectionInst; }

csp::common::RealtimeEngineType OnlineRealtimeEngine::GetRealtimeEngineType() const { return csp::common::RealtimeEngineType::Online; }

async::task<uint64_t> OnlineRealtimeEngine::RemoteGenerateNewAvatarId()
{
    // ReSharper disable once CppRedundantCastExpression, this is needed for Android builds to play nice
    // I suspect literally no one knows if this is still neccesary.
    const signalr::value param1((uint64_t)1ULL);
    const std::vector arr { param1 };
    const signalr::value params(arr);

    return m_multiplayerConnectionInst->GetSignalRConnection()
        ->Invoke(m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), params,
            [](const signalr::value&, std::exception_ptr) { })
        .then(multiplayer::continuations::UnwrapSignalRResultOrThrow())
        .then(
            [logSystem = this->m_logSystem](const signalr::value& result) // Parse the ID from the server and pass it along the chain
            {
                const auto networkId = ParseGenerateObjectIDsResult(result, *logSystem);
                return networkId;
            });
}

std::function<async::task<uint64_t>(uint64_t)> OnlineRealtimeEngine::SendNewAvatarObjectMessage(const csp::common::String& name,
    const csp::common::String& userId, const SpaceTransform& transform, bool isVisible, const csp::common::String& avatarId, AvatarState avatarState,
    AvatarPlayMode avatarPlayMode, LocomotionModel locomotionModel)
{
    return [name, userId, transform, isVisible, avatarId, avatarState, avatarPlayMode, locomotionModel, this](uint64_t networkId) // Serialize Avatar
    {
        auto newAvatar = RealtimeEngineUtils::BuildNewAvatar(userId, *this, *this->m_scriptRunner, *m_logSystem, networkId, name, transform,
            isVisible, m_multiplayerConnectionInst->GetClientId(), false, false, avatarId, avatarState, avatarPlayMode, locomotionModel);

        const mcs::ObjectMessage message = newAvatar->GetStatePatcher()->CreateObjectMessage();

        SignalRSerializer serializer;
        serializer.WriteValue(std::vector<mcs::ObjectMessage> { message });

        // Explicitly specify types when dealing with signalr values, initializer list schenanigans abound.
        return m_multiplayerConnectionInst->GetSignalRConnection()
            ->Invoke(m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), serializer.Get(),
                [](const signalr::value&, std::exception_ptr) { })
            .then(multiplayer::continuations::UnwrapSignalRResultOrThrow())
            .then([networkId]() { return networkId; });
    };
}

std::function<void(uint64_t)> OnlineRealtimeEngine::CreateNewLocalAvatar(const csp::common::String& name, const csp::common::String& userId,
    const SpaceTransform& transform, bool isVisible, const csp::common::String& avatarId, AvatarState avatarState, AvatarPlayMode avatarPlayMode,
    LocomotionModel locomotionModel, EntityCreatedCallback callback)
{
    return [name, userId, transform, isVisible, avatarId, avatarState, avatarPlayMode, locomotionModel, this, callback](uint64_t networkId)
    {
        /* Note we're constructing the avatar redundantly, both here and in the serialization.
         * The reasons for this are because the gap between where we would first need to construct the avatar prior to serialization and here
         * is too large and has a network call in between it. We don't want the possibility for a leak.
         * A better solution would be to chain proper ownership structures down the continuation, but we can't do that due to a lack
         * of a place to put them, we can't release to the client because we need shared ownership to do safe threadless chaining, but we also
         * cant chain shared ownership because we don't have a sink to put it.
         *
         * The solution to this is to make `Entities` (ie csp::common::List<SpaceEntity*>) a true owner and have it contain shared_ptrs.
         * Then, we'd make the other containers (Avatars, etc), store weak_ptrs, the idea being that for any given object,
         * CSP owns it "uniquely" (In that there's one container in CSP memory space that has a true owning type, shared_ptr in this
         * instance), but also clients may own it, sharing the ownership with us. This interacts well with garbage collectors and removes
         * pretty much all ownership consideration from clients.
         * To do this, we would want to create our own CSP pointer type that is backed by a shared_ptr, so we can provide that to
         * clients via the callback, and have shared ownership of this sort of thing across the DLL/SO boundary.
         *
         * Note also however, that we don't double fetch the network ID, which is the main cost of constructing these things anyhow.
         * (Stricter interface segregation for our serializers would also have solved this problem, but only in the local sense)
         */
        auto newAvatar = RealtimeEngineUtils::BuildNewAvatar(userId, *this, *m_scriptRunner, *m_logSystem, networkId, name, transform, isVisible,
            m_multiplayerConnectionInst->GetClientId(), false, false, avatarId, avatarState, avatarPlayMode, locomotionModel);

        std::scoped_lock entitiesLocker(*m_entitiesLock);
        // Release to vague ownership. True ownership is blurry here. It could be shared between both Entities and Objects, or just owned by
        // Entities.
        SpaceEntity* releasedAvatar = newAvatar.release();
        m_entities.Append(releasedAvatar);
        m_avatars.Append(releasedAvatar);
        releasedAvatar->ApplyLocalPatch(false, GetMultiplayerConnectionInstance()->GetAllowSelfMessagingFlag());

        if (m_electionManager != nullptr)
        {
            m_electionManager->OnLocalClientAdd(releasedAvatar, m_avatars, *this->m_networkEventBus);
        }

        callback(releasedAvatar);
    };
}

void OnlineRealtimeEngine::CreateAvatar(const csp::common::String& name, const csp::common::String& userId,
    const csp::multiplayer::SpaceTransform& spaceTransform, bool isVisible, csp::multiplayer::AvatarState avatarState,
    const csp::common::String& avatarId, csp::multiplayer::AvatarPlayMode avatarPlayMode, csp::multiplayer::LocomotionModel locomotionModel,
    csp::multiplayer::EntityCreatedCallback callback)
{
    // Ask the server for an avatar Id via "GenerateObjectIds"
    RemoteGenerateNewAvatarId()
        .then(SendNewAvatarObjectMessage(name, userId, spaceTransform, isVisible, avatarId, avatarState, avatarPlayMode, locomotionModel))
        .then(CreateNewLocalAvatar(name, userId, spaceTransform, isVisible, avatarId, avatarState, avatarPlayMode, locomotionModel, callback))
        .then(csp::common::continuations::InvokeIfExceptionInChain(*m_logSystem,
            [callback, logSystem = this->m_logSystem]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            {
                logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to create Avatar. Exception: {}", exception.what()).c_str());
                callback(nullptr);
            }));
}

void OnlineRealtimeEngine::CreateEntity(const csp::common::String& name, const csp::multiplayer::SpaceTransform& spaceTransform,
    const csp::common::Optional<uint64_t>& parentId, csp::multiplayer::EntityCreatedCallback callback)
{
    const std::function localIdCallback = [this, name, spaceTransform, parentId, callback, &logSystem = this->m_logSystem](
                                              const signalr::value& result, const std::exception_ptr& except)
    {
        try
        {
            if (except)
            {
                std::rethrow_exception(except);
            }
        }
        catch (const std::exception& e)
        {
            logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to generate object ID. Exception: {}", e.what()).c_str());
            callback(nullptr);
            return;
        }

        auto id = ParseGenerateObjectIDsResult(result, *logSystem);
        auto* newObject = new SpaceEntity(this, *m_scriptRunner, logSystem, SpaceEntityType::Object, id, name, spaceTransform,
            m_multiplayerConnectionInst->GetClientId(), parentId, true, true);

        const mcs::ObjectMessage message = newObject->GetStatePatcher()->CreateObjectMessage();

        SignalRSerializer serializer;
        serializer.WriteValue(std::vector<mcs::ObjectMessage> { message });

        const std::function<void(signalr::value, std::exception_ptr)> localSendCallback
            = [this, callback, newObject, &logSystem = m_logSystem](const signalr::value& /*Result*/, const std::exception_ptr& except)
        {
            try
            {
                if (except)
                {
                    std::rethrow_exception(except);
                }
            }
            catch (const std::exception& e)
            {
                logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to create object. Exception: {}", e.what()).c_str());
                callback(nullptr);
                return;
            }

            std::scoped_lock entitiesLocker(*m_entitiesLock);

            ResolveEntityHierarchy(newObject);

            m_entities.Append(newObject);
            m_objects.Append(newObject);
            callback(newObject);
        };

        m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
            m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), serializer.Get(),
            localSendCallback);
    };

    // ReSharper disable once CppRedundantCastExpression, this is needed for Android builds to play nice
    const signalr::value param1((uint64_t)1ULL);
    const std::vector arr { param1 };

    const signalr::value params(arr);
    m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
        m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), params, localIdCallback);
}

void OnlineRealtimeEngine::DestroyEntity(SpaceEntity* entity, CallbackHandler callback)
{
    const auto& children = entity->GetChildEntities()->ToArray();

    const std::function localCallback
        = [callback, &logSystem = this->m_logSystem](const signalr::value& /*EntityMessage*/, const std::exception_ptr& except)
    {
        try
        {
            if (except)
            {
                std::rethrow_exception(except);
            }
        }
        catch (const std::exception& e)
        {
            logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to destroy entity. Exception: {}", e.what()).c_str());
            callback(false);
            return;
        }

        callback(true);
    };

    std::vector<signalr::value> objectPatches;

    const std::vector<signalr::value> deletionPatch { entity->GetId(), m_multiplayerConnectionInst->GetClientId(), true,
        std::vector<signalr::value> {
            false,
            signalr::value_type::null,
        },
        { } };

    objectPatches.push_back(signalr::value { deletionPatch });

    // Move children to the root in the same patch
    for (size_t i = 0; i < children.Size(); ++i)
    {
        const std::vector<signalr::value> childParentIdPatch { children[i]->GetId(), m_multiplayerConnectionInst->GetClientId(), false,
            std::vector<signalr::value> {
                true, // Update Parent
                signalr::value_type::null, // Move to root
            },
            { } };

        objectPatches.push_back(signalr::value { childParentIdPatch });
    }

    auto entityComponents = entity->GetComponents();
    auto keys = entityComponents->Keys();

    for (size_t i = 0; i < keys->Size(); ++i)
    {
        auto entityComponent = entity->GetComponent((*keys)[i]);
        entityComponent->OnLocalDelete();
    }

    delete (keys);

    m_rootHierarchyEntities.RemoveItem(entity);

    RealtimeEngineUtils::LocalProcessChildUpdates(*this, m_rootHierarchyEntities, entity);

    // We break the usual pattern of not considering local state to be true until we get the ack back from CHS here
    // and instead immediately delete the local view of the entity before issuing the delete for the remote view.
    // We do this so that clients can immediately respond to the deletion and avoid sending further updates for the
    // entity that has been scheduled for deletion.
    LocalDestroyEntity(entity);

    const std::vector invokeArguments = { signalr::value(objectPatches) };
    m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
        m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_PATCHES), invokeArguments, localCallback);
}

void OnlineRealtimeEngine::LocalDestroyEntity(SpaceEntity* entity)
{
    if (entity != nullptr)
    {
        if (entity->GetEntityDestroyCallback() != nullptr)
        {
            entity->GetEntityDestroyCallback()(true);
        }

        RemoveEntity(entity);
    }
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceEntity(const csp::common::String& inName)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return RealtimeEngineUtils::FindSpaceEntity(*this, inName);
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceEntityById(uint64_t entityId)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return RealtimeEngineUtils::FindSpaceEntityById(*this, entityId);
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceAvatar(const csp::common::String& inName)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return RealtimeEngineUtils::FindSpaceAvatar(*this, inName);
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceObject(const csp::common::String& inName)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return RealtimeEngineUtils::FindSpaceObject(*this, inName);
}

void OnlineRealtimeEngine::SetRemoteEntityCreatedCallback(EntityCreatedCallback callback)
{
    if (m_remoteSpaceEntityCreatedCallback)
    {
        m_logSystem->LogMsg(common::LogLevel::Warning, "RemoteSpaceEntityCreatedCallback has already been set. Previous callback overwritten.");
    }

    m_remoteSpaceEntityCreatedCallback = std::move(callback);
}

bool OnlineRealtimeEngine::AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* entity)
{
    if (!m_selectedEntities.Contains(entity))
    {
        m_selectedEntities.Append(entity);
        return true;
    }
    return false;
}

bool OnlineRealtimeEngine::RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* entity)
{
    if (m_selectedEntities.Contains(entity))
    {
        m_selectedEntities.RemoveItem(entity);
        return true;
    }
    return false;
}

void OnlineRealtimeEngine::SetScriptLeaderReadyCallback(CallbackHandler callback)
{
    if (m_scriptSystemReadyCallback)
    {
        m_logSystem->LogMsg(common::LogLevel::Warning, "ScriptLeaderReadyCallback has already been set. Previous callback overwritten.");
    }

    m_scriptSystemReadyCallback = callback;

    if (m_electionManager)
    {
        m_electionManager->SetScriptLeaderReadyCallback(m_scriptSystemReadyCallback);
    }
}

void OnlineRealtimeEngine::SetOnElectedScopeLeaderCallback(ScopeLeaderCallback callback) { m_onElectedScopeLeaderCallback = callback; }

void OnlineRealtimeEngine::SetOnVacatedAsScopeLeaderCallback(ScopeLeaderCallback callback) { m_onVacatedAsScopeLeaderCallback = callback; }

namespace
{
    void FireRemoteSpaceEntityCreatedCallback(
        SpaceEntity* spaceEntity, csp::multiplayer::EntityCreatedCallback remoteSpaceEntityCreatedCallback, csp::common::LogSystem& logSystem)
    {
        if (remoteSpaceEntityCreatedCallback)
        {
            remoteSpaceEntityCreatedCallback(spaceEntity);
        }
        else
        {
            logSystem.LogMsg(common::LogLevel::Warning,
                "Called RemoteSpaceEntityCreatedCallback without it being set! Call SetRemoteEntityCreatedCallback first!");
        }
    }
}

SpaceEntity* OnlineRealtimeEngine::CreateRemotelyRetrievedEntity(const signalr::value& entityMessage)
{
    //  Create object message from signalr value
    mcs::ObjectMessage message;
    SignalRDeserializer deserializer { entityMessage };
    deserializer.ReadValue(message);

    auto newEntity = SpaceEntityStatePatcher::NewFromObjectMessage(message, *this, *m_scriptRunner, *m_logSystem);

    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return m_pendingAdds->emplace_back(newEntity.release());
}

void OnlineRealtimeEngine::OnObjectMessage(const signalr::value& params)
{
    // Params is an array of all params sent, so grab the first
    auto& entityMessage = params.as_array()[0];

    SpaceEntity* newEntity = CreateRemotelyRetrievedEntity(entityMessage);

    if (newEntity)
    {
        FireRemoteSpaceEntityCreatedCallback(newEntity, m_remoteSpaceEntityCreatedCallback, *m_logSystem);
    }
}

void OnlineRealtimeEngine::OnObjectPatch(const signalr::value& params)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);

    // Params is an array of all params sent, so grab the first
    auto& entityMessage = params.as_array()[0];

    m_pendingIncomingUpdates->emplace_back(new signalr::value(entityMessage));
}

void OnlineRealtimeEngine::OnRequestToSendObject(const signalr::value& params)
{
    const uint64_t entityId = params.as_array()[0].as_uinteger();

    // TODO: add ability to check for ID or get by ID from Entity List (maybe change to Map<EntityID, Entity> ?)
    if (SpaceEntity* matchedEntity = FindSpaceEntityById(entityId))
    {
        mcs::ObjectMessage message = matchedEntity->GetStatePatcher()->CreateObjectMessage();

        SignalRSerializer serializer;
        serializer.WriteValue(std::vector<mcs::ObjectMessage> { message });

        const std::function localSendCallback
            = [this](const signalr::value&, const std::exception_ptr& except) { HandleException(except, "Failed to send server requested object."); };

        m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
            m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), serializer.Get(),
            localSendCallback);
    }
    else
    {
        std::vector<signalr::value> const invokeArguments = { entityId };

        m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
            m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_NOT_FOUND), invokeArguments);
    }
}

void OnlineRealtimeEngine::OnElectedScopeLeader(const signalr::value& params)
{
    // This could be nullptr if server-side leader election is enabled for the scope within the backend services, but turned off client-side by
    // calling DisableLeadershipElection. These checks could be removed if the election events were bound inside the ScopeLeadershipManager. However,
    // I decided to follow our standard pattern of binding to events once, inside the MultiplayerConnection initialization.
    if (m_leaderElectionManager == nullptr)
    {
        return;
    }

    const std::vector<signalr::value> paramsV = params.as_array();

    const std::string scopeId = paramsV[0].as_string();
    const std::string userId = paramsV[1].as_string();
    const uint64_t clientId = paramsV[2].as_uinteger();

    m_leaderElectionManager->OnElectedScopeLeader(scopeId, clientId);

    if (m_onElectedScopeLeaderCallback)
    {
        m_onElectedScopeLeaderCallback(scopeId.c_str(), userId.c_str());
    }
}

void OnlineRealtimeEngine::OnVacatedAsScopeLeader(const signalr::value& params)
{
    // This could be nullptr if server-side leader election is enabled for the scope within the backend services, but turned off client-side by
    // calling DisableLeadershipElection. These checks could be removed if the election events were bound inside the ScopeLeadershipManager. However,
    // I decided to follow our standard pattern of binding to events once, inside the MultiplayerConnection initialization.
    if (m_leaderElectionManager == nullptr)
    {
        return;
    }

    const std::vector<signalr::value> paramsV = params.as_array();

    const std::string scopeId = paramsV[0].as_string();
    const std::string userId = paramsV[1].as_string();

    m_leaderElectionManager->OnVacatedAsScopeLeader(scopeId);

    if (m_onVacatedAsScopeLeaderCallback)
    {
        m_onVacatedAsScopeLeaderCallback(scopeId.c_str(), userId.c_str());
    }
}

void OnlineRealtimeEngine::GetEntitiesPaged(int skip, int limit, const std::function<void(const signalr::value&, std::exception_ptr)>& callback)
{
    std::vector<signalr::value> paramsVec;
    paramsVec.push_back(signalr::value(true)); // excludeClientOwned
    paramsVec.push_back(signalr::value(true)); // includeClientOwnedPersistentObjects
    paramsVec.push_back(signalr::value((uint64_t)skip)); // skip
    paramsVec.push_back(signalr::value((uint64_t)limit)); // limit
    const auto params = signalr::value(std::move(paramsVec));

    m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
        m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::PAGE_SCOPED_OBJECTS), params, callback);
}

std::function<void(const signalr::value&, std::exception_ptr)> OnlineRealtimeEngine::CreateRetrieveAllEntitiesCallback(
    int skip, csp::common::EntityFetchCompleteCallback fetchCompleteCallback)
{
    const std::function callback = [this, skip, fetchCompleteCallback](const signalr::value& result, std::exception_ptr except)
    {
        HandleException(except, "Failed to retrieve paged entities.");

        const auto& results = result.as_array();
        const auto& items = results[0].as_array();
        auto itemTotalCount = results[1].as_uinteger();

        for (const auto& entityMessage : items)
        {
            SpaceEntity* newEntity = CreateRemotelyRetrievedEntity(entityMessage);
            FireRemoteSpaceEntityCreatedCallback(newEntity, m_remoteSpaceEntityCreatedCallback, *m_logSystem);
        }

        int currentEntityCount = skip + static_cast<int>(items.size());

        if (static_cast<uint64_t>(currentEntityCount) < itemTotalCount)
        {
            GetEntitiesPaged(currentEntityCount, ENTITY_PAGE_LIMIT, CreateRetrieveAllEntitiesCallback(currentEntityCount, fetchCompleteCallback));
        }
        else
        {
            std::scoped_lock entitiesLocker(*m_entitiesLock);
            // Ensure entity list is up to date
            ProcessPendingEntityOperations();

            RealtimeEngineUtils::InitialiseEntityScripts(m_entities);
            m_enableEntityTick = true;

            // This is a suboptimal fix. We shouldn't be doing much of the things we do here. Remember this is the
            // "Space has finished hydrating" call, when all the assets have been fetched. You can be in a space
            // and moving around before this.
            // Without this lock, calling "DisableLeadershipElection" after entering a space creates a race condition.
            // As this function can be called at any point after entering a space.

            std::scoped_lock leaderElectionLocker(m_leadershipElectionLock);

            if (IsLeaderElectionEnabled())
            {
                if (m_serverSideElectionEnabled)
                {
                    // For server-side leader election, we want to listen for script run requests from other clients.
                    // We will receive these if we are the leader and another client modifies a script or sends an event.
                    this->m_networkEventBus->ListenNetworkEvent(
                        csp::multiplayer::NetworkEventRegistration { "CSPInternal::ClientElectionManager", RemoteRunScriptMessage },
                        [this](const csp::common::NetworkEventData& eventData) { this->OnRemoteRunScriptEvent(eventData.EventValues); });

                    // To match the behaviour of the client-side leader election, the ScriptSystemReadyCallback should fire here.
                    // We may want to move this to earlier in the initialization in the future.
                    if (m_scriptSystemReadyCallback)
                    {
                        m_scriptSystemReadyCallback(true);
                    }
                }
                else
                {
                    // Start listening for election events
                    //
                    // If we are the first client to connect then this
                    // will also set this client as the leader
                    m_electionManager->OnConnect(m_avatars, m_objects);
                }
            }
            else
            {
                // Leader election not enabled, set ourselves as the script owner.
                RealtimeEngineUtils::DetermineScriptOwners(m_entities, GetMultiplayerConnectionInstance()->GetClientId());
            }

            if (fetchCompleteCallback)
            {
                fetchCompleteCallback(currentEntityCount);
            }
        }
    };

    return callback;
};

void OnlineRealtimeEngine::FetchAllEntitiesAndPopulateBuffers(
    const csp::common::String&, csp::common::EntityFetchStartedCallback fetchStartedCallback)
{
    this->RetrieveAllEntities(m_entityFetchCompleteCallback);
    fetchStartedCallback();
}

void OnlineRealtimeEngine::LockEntityUpdate() { m_entitiesLock->lock(); }

bool OnlineRealtimeEngine::TryLockEntityUpdate() { return m_entitiesLock->try_lock(); }

void OnlineRealtimeEngine::UnlockEntityUpdate() { m_entitiesLock->unlock(); }

csp::multiplayer::SpaceEntityStatePatcher* OnlineRealtimeEngine::MakeStatePatcher(csp::multiplayer::SpaceEntity& spaceEntity) const
{
    return new SpaceEntityStatePatcher(m_logSystem, spaceEntity);
}

ModifiableStatus OnlineRealtimeEngine::IsEntityModifiable(const csp::multiplayer::SpaceEntity* spaceEntity) const
{
    // This should definitely be true at this point, but be defensive.
    assert(spaceEntity->GetStatePatcher() != nullptr);

    // In order to unlock an entity, we need to modify it.
    // So we need to check if we are about to unlock the entity, and treat it as modifiabe if so, otherwise we cannot unlock a locked entity.
    // Note : This will stop working if we ever add another lock type
    const bool aboutToUnlock = spaceEntity->GetStatePatcher()->GetDirtyProperties().count(SpaceEntityComponentKey::LockType) > 0;
    if (spaceEntity->GetLockType() == LockType::UserAgnostic && !aboutToUnlock)
    {
        return ModifiableStatus::EntityLocked;
    }

    // If the entity isn't owned by this client, and ownership cannot be transfered, then we cannot modify this entity.
    if (spaceEntity->GetOwnerId() != GetMultiplayerConnectionInstance()->GetClientId() && spaceEntity->GetIsTransferable() == false)
    {
        return ModifiableStatus::EntityNotOwnedAndUntransferable;
    }

    return ModifiableStatus::Modifiable;
}

const csp::multiplayer::ComponentSchemaRegistry* OnlineRealtimeEngine::GetComponentSchemaRegistry() const { return &m_componentRegistry; }

void OnlineRealtimeEngine::RetrieveAllEntities(csp::common::EntityFetchCompleteCallback fetchCompleteCallback)
{
    if ((m_multiplayerConnectionInst == nullptr) || (m_multiplayerConnectionInst->GetSignalRConnection() == nullptr))
    {
        return;
    }

    GetEntitiesPaged(
        0, ENTITY_PAGE_LIMIT, CreateRetrieveAllEntitiesCallback(0, fetchCompleteCallback)); // Get at most ENTITY_PAGE_LIMIT entities at a time
}

void OnlineRealtimeEngine::LocalDestroyAllEntities()
{
    LockEntityUpdate();

    const auto numEntities = GetNumEntities();

    for (size_t i = 0; i < numEntities; ++i)
    {
        SpaceEntity* entity = GetEntityByIndex(i);

        // We automatically invoke SignalR deletion for all transient entities that were owned by this local client
        // as these are only ever valid for a single connected session
        if (entity->GetIsTransient() && entity->GetOwnerId() == GetMultiplayerConnectionInstance()->GetClientId())
        {
            DestroyEntity(entity, [](bool /*Ok*/) { });
        }
        // Otherwise we clear up all all locally represented entities
        else
        {
            LocalDestroyEntity(entity);
        }

        delete (entity);
    }

    m_entities.Clear();
    m_objects.Clear();
    m_avatars.Clear();
    m_rootHierarchyEntities.Clear();

    // Clear adds/removes, we don't want to mutate if we're cleaning everything else.
    m_pendingAdds->clear();
    m_pendingRemoves->clear();
    m_pendingIncomingUpdates->clear();

    UnlockEntityUpdate();
}

void OnlineRealtimeEngine::QueueEntityUpdate(SpaceEntity* entityToUpdate)
{
    // If we have nothing to update, don't allow a patch to be sent.
    if (!entityToUpdate->GetStatePatcher()->HasPendingPatch())
    {
        // TODO: consider making this a callback that informs the user what the status of the request is 'Success, SignalRException, NoChanges',
        // etc. CSP_LOG_MSG(csp::common::LogLevel::Log, "Skipped patch message send as no data changed");
        return;
    }

    // Ensure we can modify the entity. The criteria for this can be found on the specific RealtimeEngine::IsEntityModifiable overloads.
    ModifiableStatus modifiable = entityToUpdate->IsModifiable();
    if (modifiable != ModifiableStatus::Modifiable)
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Failed to queue entity update: {0}. Entity name: {1}", RealtimeEngineUtils::ModifiableStatusToString(modifiable),
                    entityToUpdate->GetName())
                    .c_str());
        }

        return;
    }

    // Note that calling Queue many times will be ignored by this emplace call, but may have performance impact in some situations.
    // TODO: consider enabling clients to queue at sensible rates by exposing update rates/next update callbacks/timings.
    m_pendingOutgoingUpdateUniqueSet->emplace(entityToUpdate);
}

void OnlineRealtimeEngine::RemoveEntity(SpaceEntity* entityToRemove)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    m_pendingRemoves->emplace_back(entityToRemove);

    // Remove from the unique set to indicate it could be queued again if needed.
    m_pendingOutgoingUpdateUniqueSet->erase(entityToRemove);
}

void OnlineRealtimeEngine::TickEntities()
{
    ProcessPendingEntityOperations();

    if (m_enableEntityTick)
    {
        // If this is an online engine with leadership election enabled, then only the script leader may run scripts.
        // If there is no leadership election, then we assume all clients may run scripts.
        bool canRunScripts = IsLocalClientLeader();

        if (canRunScripts)
        {
            m_lastTickTime = RealtimeEngineUtils::TickEntityScripts(*m_tickEntitiesLock, m_entities, m_lastTickTime);
        }
        else
        {
            m_lastTickTime = std::chrono::system_clock::now();
        }

        if (m_leaderElectionManager)
        {
            // If we are using server-side leader election, we need to send heartbeats if we are the leader of any scopes.
            m_leaderElectionManager->SendHeartbeatIfElectedScopeLeader();
        }
    }

    {
        std::scoped_lock tickEntitiesLocker(*m_tickEntitiesLock);

        // Remove any duplicate Entities
        constexpr std::less<SpaceEntity*> pointCmp;
        m_tickUpdateEntities.sort(pointCmp);
        m_tickUpdateEntities.unique(pointCmp);

        for (const auto entity : m_tickUpdateEntities)
        {
            QueueEntityUpdate(entity);
        }

        m_tickUpdateEntities.clear();
    }
}

void OnlineRealtimeEngine::RegisterDefaultScope(const std::string& scopeId, const std::optional<uint64_t>& leaderId)
{
    if (IsLeaderElectionEnabled() && m_serverSideElectionEnabled)
    {
        m_leaderElectionManager->RegisterScope(scopeId, leaderId);
        m_defaultScopeId = scopeId.c_str();
    }
    else
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Warning, "Tried to register scope when server-side leader election was disabled");
    }
}

void OnlineRealtimeEngine::__AssumeScopeLeadership(const std::string& scopeId, std::function<void(bool)> callback)
{
    auto cb = [this, callback](signalr::value, std::exception_ptr e)
    {
        if (e)
        {
            try
            {
                std::rethrow_exception(e);
            }
            catch (const std::exception& exception)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Error,
                    fmt::format("OnlineRealtimeEngine::__AssumeScopeLeadership Failed to send AssumeScopeLeadership with error: {}", exception.what(),
                        exception.what())
                        .c_str());
            }
            catch (...)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Error,
                    "OnlineRealtimeEngine::__AssumeScopeLeadership Failed to send AssumeScopeLeadership with an unknown error.");
            }

            callback(false);
        }
        else
        {
            callback(true);
        }
    };

    std::vector<signalr::value> params;
    params.push_back({ scopeId });

    m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
        m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::ASSUME_SCOPE_LEADERSHIP), signalr::value { params }, cb);
}

void OnlineRealtimeEngine::SetServerSideElectionEnabled(bool value) { m_serverSideElectionEnabled = value; }

bool OnlineRealtimeEngine::EntityIsInRootHierarchy(SpaceEntity* entity)
{
    for (size_t i = 0; i < m_rootHierarchyEntities.Size(); ++i)
    {
        if (m_rootHierarchyEntities[i]->GetId() == entity->GetId())
        {
            return true;
        }
    }

    return false;
}

void OnlineRealtimeEngine::OnRemoteRunScriptEvent(const csp::common::Array<csp::common::ReplicatedValue>& data)
{
    // @Note This needs to be kept in sync with any changes to message format
    const int64_t contextId = static_cast<int64_t>(data[0].GetInt());
    const csp::common::String& scriptText = data[1].GetString();

    m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("ClientElectionManager::OnRemoteRunScriptEvent called. ContextId={0}, Script={1}", contextId, scriptText.c_str()).c_str());

    if (m_leaderElectionManager->IsLocalClientLeader(m_defaultScopeId.c_str()))
    {
        m_scriptRunner->RunScript(contextId, scriptText);
    }
    else
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error,
            fmt::format("Client {} has received remote script event but is not the Leader", m_multiplayerConnectionInst->GetClientId()).c_str());
    }
}

void OnlineRealtimeEngine::SendRemoteRunScriptEvent(int64_t targetClientId, int64_t contextId, const csp::common::String& scriptText)
{
    const MultiplayerConnection::ErrorCodeCallbackHandler signalRCallback = [&logSystem = this->m_logSystem](ErrorCode error)
    {
        if (error != ErrorCode::None)
        {
            logSystem->LogMsg(csp::common::LogLevel::Error, "ClientProxy::SendEvent: SignalR connection: Error");
        }
    };

    m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
        fmt::format("SendRemoteRunScriptEvent Target={0} ContextId={1} Script='{2}'", targetClientId, contextId, scriptText).c_str());

    m_networkEventBus->SendNetworkEventToClient(RemoteRunScriptMessage,
        { csp::common::ReplicatedValue(contextId), csp::common::ReplicatedValue(scriptText) }, targetClientId, signalRCallback);
}

void OnlineRealtimeEngine::ClaimScriptOwnershipFromClient(uint64_t clientId)
{
    for (size_t i = 0; i < m_entities.Size(); ++i)
    {
        if (m_entities[i]->GetScript().GetOwnerId() == clientId)
        {
            RealtimeEngineUtils::ClaimScriptOwnership(m_entities[i], GetMultiplayerConnectionInstance()->GetClientId());
        }
    }
}

bool OnlineRealtimeEngine::IsLocalClientLeader() const
{
    if (IsLeaderElectionEnabled() == false)
    {
        return true;
    }

    if (m_serverSideElectionEnabled)
    {
        return m_leaderElectionManager->IsLocalClientLeader(m_defaultScopeId.c_str());
    }
    else
    {
        return m_electionManager->IsLocalClientLeader();
    }
}

void OnlineRealtimeEngine::ClaimScriptOwnership(SpaceEntity* entity) const
{
    RealtimeEngineUtils::ClaimScriptOwnership(entity, GetMultiplayerConnectionInstance()->GetClientId());
}

void OnlineRealtimeEngine::EnableLeaderElection()
{
    DisableLeaderElection();

    std::scoped_lock leaderElectionLocker(m_leadershipElectionLock);

    if (m_serverSideElectionEnabled)
    {
        m_leaderElectionManager = std::make_unique<multiplayer::ScopeLeadershipManager>(*m_multiplayerConnectionInst, *m_logSystem);
    }
    else
    {
        m_electionManager = new ClientElectionManager(this, *m_logSystem, *m_scriptRunner);
        m_electionManager->SetScriptLeaderReadyCallback(m_scriptSystemReadyCallback);
    }
}

void OnlineRealtimeEngine::DisableLeaderElection()
{
    std::scoped_lock leaderElectionLocker(m_leadershipElectionLock);

    if (m_leaderElectionManager != nullptr)
    {
        m_networkEventBus->StopListenNetworkEvent(
            csp::multiplayer::NetworkEventRegistration("CSPInternal::ClientElectionManager", RemoteRunScriptMessage));

        m_leaderElectionManager.reset(nullptr);
    }
    if (m_electionManager != nullptr)
    {
        delete (m_electionManager);
        m_electionManager = nullptr;
    }
}

bool OnlineRealtimeEngine::IsLeaderElectionEnabled() const { return (m_electionManager != nullptr) || (m_leaderElectionManager.get() != nullptr); }

uint64_t OnlineRealtimeEngine::GetLeaderId() const
{
    if (IsLeaderElectionEnabled())
    {
        if (m_serverSideElectionEnabled)
        {
            std::optional<uint64_t> leaderId = m_leaderElectionManager->GetLeaderClientId(m_defaultScopeId.c_str());
            return leaderId.has_value() ? *leaderId : 0;
        }
        else
        {
            return m_electionManager->GetLeader()->GetId();
        }
    }
    else
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Warning, "OnlineRealtimeEngine::GetLeaderId Called when leader election isn't enabled.");
        return 0;
    }
}

bool OnlineRealtimeEngine::GetEntityPatchRateLimitEnabled() const { return m_entityPatchRateLimitEnabled; }

void OnlineRealtimeEngine::SetEntityPatchRateLimitEnabled(bool enabled) { m_entityPatchRateLimitEnabled = enabled; }

const csp::common::List<SpaceEntity*>* OnlineRealtimeEngine::GetRootHierarchyEntities() const { return &m_rootHierarchyEntities; }

void OnlineRealtimeEngine::ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* entity)
{
    RealtimeEngineUtils::ResolveEntityHierarchy(*this, m_rootHierarchyEntities, entity);
}

async::task<void> OnlineRealtimeEngine::RefreshMultiplayerConnectionToEnactScopeChange(csp::common::String spaceId)
{

    // Unfortunately we have to stop listening in order for our scope change to take effect, then start again once done.
    // This hopefully will change in a future version when CHS support it.
    return m_multiplayerConnectionInst->StopListening()
        .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>())
        .then(async::inline_scheduler(), [this, spaceId]() { return m_multiplayerConnectionInst->SetScopes(spaceId); })
        .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>())
        .then(async::inline_scheduler(), [this]() { return m_multiplayerConnectionInst->StartListening(); })
        .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>());
}

bool OnlineRealtimeEngine::CheckIfWeShouldRunScriptsLocally() const
{
    if (!IsLeaderElectionEnabled())
    {
        // Retain existing behavior if feature disabled
        // (Run scripts locally if client is object owner)
        return true;
    }
    else
    {
        // Only run script locally if we are the Leader
        if (m_serverSideElectionEnabled)
        {
            return m_leaderElectionManager->IsLocalClientLeader(m_defaultScopeId.c_str());
        }
        else
        {
            return m_electionManager->IsLocalClientLeader();
        }
    }
}

void OnlineRealtimeEngine::RunScriptRemotely(int64_t contextId, const csp::common::String& scriptText)
{
    // Run script on a remote leader...
    m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("OnlineRealtimeEngine::RunScriptRemotely Script='{}'", scriptText).c_str());

    if (m_serverSideElectionEnabled)
    {
        std::optional<uint64_t> leaderId = m_leaderElectionManager->GetLeaderClientId(m_defaultScopeId.c_str());

        if (leaderId.has_value())
        {
            // Note: This is cast to an int64. This is because we only support sending signed integers over the network.
            SendRemoteRunScriptEvent(static_cast<int64_t>(*leaderId), contextId, scriptText);
        }
        else
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error,
                "OnlineRealtimeEngine::RunScriptRemotely failed due to receiving a script run event for a scope it is not the leader of.");
        }
    }
    else
    {
        ClientProxy* leaderProxy = m_electionManager->GetLeader();
        if (leaderProxy)
        {
            // This client is the leader, so run the script.
            leaderProxy->RunScript(contextId, scriptText);
        }
    }
}

size_t OnlineRealtimeEngine::GetNumEntities() const
{
    std::scoped_lock<std::recursive_mutex> entitiesLocker(*m_entitiesLock);
    return m_entities.Size();
}

size_t OnlineRealtimeEngine::GetNumAvatars() const
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return m_avatars.Size();
}

size_t OnlineRealtimeEngine::GetNumObjects() const
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return m_objects.Size();
}

SpaceEntity* OnlineRealtimeEngine::GetEntityByIndex(const size_t entityIndex)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return m_entities[entityIndex];
}

SpaceEntity* OnlineRealtimeEngine::GetAvatarByIndex(const size_t avatarIndex)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return m_avatars[avatarIndex];
}

SpaceEntity* OnlineRealtimeEngine::GetObjectByIndex(const size_t objectIndex)
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    return m_objects[objectIndex];
}

const csp::common::List<SpaceEntity*>* OnlineRealtimeEngine::GetAllEntities() const { return &m_entities; }

void OnlineRealtimeEngine::SendPatches(const csp::common::List<SpaceEntity*> pendingEntities)
{
    const std::function localCallback = [&logSystem = this->m_logSystem](const signalr::value& /*Result*/, const std::exception_ptr& except)
    {
        try
        {
            if (except)
            {
                std::rethrow_exception(except);
            }
        }
        catch (const std::exception& e)
        {
            logSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("Failed to send list of entity update due to a signalr exception! Exception: {}", e.what()).c_str());
        }
    };

    std::vector<mcs::ObjectPatch> patches;
    SignalRSerializer serializer;

    for (size_t i = 0; i < pendingEntities.Size(); ++i)
    {
        patches.push_back(pendingEntities[i]->GetStatePatcher()->CreateObjectPatch());
    }

    // We are writing multiple patches, so we need an additional nested array.
    serializer.StartWriteArray();
    {
        serializer.WriteValue(patches);
    }
    serializer.EndWriteArray();

    m_multiplayerConnectionInst->GetSignalRConnection()->Invoke(
        m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_PATCHES), serializer.Get(), localCallback);
}

void OnlineRealtimeEngine::ProcessPendingEntityOperations()
{
    std::scoped_lock entitiesLocker(*m_entitiesLock);
    csp::common::List<SpaceEntity*> pendingEntities;
    // we run pending entity operations in a specific order
    // 1 - flush pending adds - we do this first to ensure any attempts to apply updates after are successful
    // 2 - flush pending updates - first the local representation, then the remote representation (with rate limiting)
    // 3 - flush pending removes - we do this last so any pending updates can still mutate state on entities that are pending removal

    // adds
    std::unordered_set<SpaceEntity*> addedEntities;
    while (m_pendingAdds->empty() == false)
    {
        SpaceEntity* pendingAddEntity = m_pendingAdds->front();

        // we only want to add an entity once, even though a client could have queued it for updates multiple times
        if (addedEntities.find(pendingAddEntity) == addedEntities.end())
        {
            AddPendingEntity(pendingAddEntity);
            addedEntities.emplace(pendingAddEntity);

            ResolveEntityHierarchy(pendingAddEntity);
        }
        m_pendingAdds->pop_front();
    }

    // local updates
    while (m_pendingIncomingUpdates->empty() == false)
    {
        ApplyIncomingPatch(m_pendingIncomingUpdates->front());
        m_pendingIncomingUpdates->pop_front();
    }

    // remote updates
    {
        for (auto it = m_pendingOutgoingUpdateUniqueSet->begin(); it != m_pendingOutgoingUpdateUniqueSet->end();)
        {
            SpaceEntity* pendingEntity = *it;

            const milliseconds currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

            if (currentTime - pendingEntity->GetTimeOfLastPatch() >= m_entityPatchRate || !m_entityPatchRateLimitEnabled)
            {
                // Ensure we can modify the entity. The criteria for this can be found on the specific RealtimeEngine::IsEntityModifiable overloads.
                ModifiableStatus modifiable = pendingEntity->IsModifiable();
                if (modifiable != ModifiableStatus::Modifiable)
                {
                    if (m_logSystem != nullptr)
                    {
                        m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                            fmt::format("Failed to send patch for entity: {0}. Entity name: {1}",
                                RealtimeEngineUtils::ModifiableStatusToString(modifiable), pendingEntity->GetName())
                                .c_str());
                    }

                    it = m_pendingOutgoingUpdateUniqueSet->erase(it);
                    continue;
                }

                // since we are aiming to mutate the data for this entity remotely, we need to claim ownership over it
                pendingEntity->SetOwnerId(m_multiplayerConnectionInst->GetClientId());
                RealtimeEngineUtils::ClaimScriptOwnership(pendingEntity, GetMultiplayerConnectionInstance()->GetClientId());

                pendingEntities.Append(pendingEntity);

                if (pendingEntity->GetStatePatcher()->GetEntityPatchSentCallback() != nullptr)
                {
                    pendingEntity->GetStatePatcher()->CallEntityPatchSentCallback(true);
                }

                pendingEntity->GetStatePatcher()->SetTimeOfLastPatch(currentTime);
                it = m_pendingOutgoingUpdateUniqueSet->erase(it);
            }
            else
            {
                m_logSystem->LogMsg(common::LogLevel::VeryVerbose,
                    "Skipping patch send in ProcessPendingEntityOperations as not enough time has passed since the last patch");
                ++it;
            }
        }

        // Only send if there are patches in list
        if (pendingEntities.Size() != 0)
        {
            // Send list of PendingEntities to chs
            SendPatches(pendingEntities);

            // Loop through and apply local patches from generated list
            for (size_t i = 0; i < pendingEntities.Size(); ++i)
            {
                pendingEntities[i]->ApplyLocalPatch(true, GetMultiplayerConnectionInstance()->GetAllowSelfMessagingFlag());
            }
        }
    }

    // removes
    std::unordered_set<SpaceEntity*> removedEntities;
    while (m_pendingRemoves->empty() == false)
    {
        SpaceEntity* pendingRemoveEntity = m_pendingRemoves->front();

        // we only want to remove an entity once, even though a client could have queued it for updates multiple times
        if (removedEntities.find(pendingRemoveEntity) == removedEntities.end())
        {
            removedEntities.emplace(pendingRemoveEntity);

            RemovePendingEntity(m_pendingRemoves->front());
        }
        m_pendingRemoves->pop_front();
    }
}

void OnlineRealtimeEngine::AddPendingEntity(SpaceEntity* entityToAdd)
{
    if (FindSpaceEntityById(entityToAdd->GetId()) == nullptr)
    {
        m_entities.Append(entityToAdd);

        switch (entityToAdd->GetEntityType())
        {
        case SpaceEntityType::Avatar:
            m_avatars.Append(entityToAdd);
            OnAvatarAdd(entityToAdd, m_avatars);
            break;

        case SpaceEntityType::Object:
            m_objects.Append(entityToAdd);
            OnObjectAdd(entityToAdd, m_objects);
            break;
        }
    }
    else
    {
        m_logSystem->LogMsg(common::LogLevel::Error, "Attempted to add a pending entity that we already have!");
    }
}

void OnlineRealtimeEngine::RemovePendingEntity(SpaceEntity* entityToRemove)
{
    assert(m_entities.Contains(entityToRemove));

    switch (entityToRemove->GetEntityType())
    {
    case SpaceEntityType::Avatar:
        assert(m_avatars.Contains(entityToRemove));
        OnAvatarRemove(entityToRemove, m_avatars);
        m_avatars.RemoveItem(entityToRemove);
        break;

    case SpaceEntityType::Object:
        assert(m_objects.Contains(entityToRemove));
        OnObjectRemove(entityToRemove, m_objects);
        m_objects.RemoveItem(entityToRemove);
        break;

    default:
        assert(false && "Unhandled entity type encountered during its destruction!");
        break;
    }

    m_rootHierarchyEntities.RemoveItem(entityToRemove);
    RealtimeEngineUtils::RemoveParentChildRelationshipsFromEntity(*this, m_rootHierarchyEntities, entityToRemove);

    m_entities.RemoveItem(entityToRemove);

    delete (entityToRemove);
}

void OnlineRealtimeEngine::OnAvatarAdd(const SpaceEntity* avatar, const csp::common::List<SpaceEntity*>& addedAvatars)
{
    if (m_electionManager != nullptr)
    {
        // Note we are assuming Avatar==Client,
        // which is true now but may not be in the future
        m_electionManager->OnClientAdd(avatar, addedAvatars, *this->m_networkEventBus);
    }
}

void OnlineRealtimeEngine::OnAvatarRemove(const SpaceEntity* avatar, const csp::common::List<SpaceEntity*>& removedAvatars)
{
    if (m_electionManager != nullptr)
    {
        m_electionManager->OnClientRemove(avatar, removedAvatars);
    }
}

void OnlineRealtimeEngine::OnObjectAdd(const SpaceEntity* object, const csp::common::List<SpaceEntity*>& addedObjects)
{
    if (m_electionManager != nullptr)
    {
        m_electionManager->OnObjectAdd(object, addedObjects);
    }
}

void OnlineRealtimeEngine::OnObjectRemove(const SpaceEntity* object, const csp::common::List<SpaceEntity*>& removedObjects)
{
    if (m_electionManager != nullptr)
    {
        m_electionManager->OnObjectRemove(object, removedObjects);
    }
}

void OnlineRealtimeEngine::ApplyIncomingPatch(const signalr::value* entityMessage)
{
    mcs::ObjectPatch patch;
    SignalRDeserializer deserializer { *entityMessage };
    deserializer.ReadValue(patch);

    if (patch.GetDestroy())
    {
        // This is an entity deletion.
        for (size_t i = 0; i < m_entities.Size(); ++i)
        {
            SpaceEntity* entity = m_entities[i];

            if (entity->GetId() == patch.GetId())
            {
                if (entity->GetEntityType() == SpaceEntityType::Avatar)
                {
                    // This can be removed as part of OF-1785.
                    if (m_serverSideElectionEnabled == false)
                    {
                        // All clients will take ownership of deleted avatars scripts
                        // Last client which receives patch will end up with ownership
                        ClaimScriptOwnershipFromClient(entity->GetOwnerId());
                    }

                    // Loop through all entities and check if the deleted avatar owned any of them. If they did, deselect them.
                    // This covers disconnected clients as their avatar gets cleaned up after timing out.
                    for (size_t j = 0; j < m_entities.Size(); ++j)
                    {
                        if (m_entities[j]->GetSelectingClientID() == patch.GetId())
                        {
                            m_entities[j]->Deselect();
                            m_selectedEntities.RemoveItem(m_entities[j]);
                        }
                    }
                }

                LocalDestroyEntity(entity);
            }
        }
    }
    else
    {
        bool entityFound = false;

        // Update
        for (SpaceEntity* entity : m_entities)
        {
            if (entity->GetId() == patch.GetId())
            {
                entity->GetStatePatcher()->ApplyPatchFromObjectPatch(patch);
                entityFound = true;
            }
        }

        if (!entityFound)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("Failed to find an entity with ID {} when received a patch message.", patch.GetId()).c_str());
        }
    }
}

void OnlineRealtimeEngine::HandleException(const std::exception_ptr& except, const std::string& exceptionDescription)
{
    try
    {
        if (except)
        {
            std::rethrow_exception(except);
        }
    }
    catch (const std::exception& e)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("{0} Exception: {1}", exceptionDescription.c_str(), e.what()).c_str());
    }
}
} // namespace csp::multiplayer
