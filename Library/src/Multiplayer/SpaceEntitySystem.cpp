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
#include "CSP/Multiplayer/SpaceEntitySystem.h"

#include "CSP/Common/List.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Memory/Memory.h"
#include "Multiplayer/Election/ClientElectionManager.h"
#include "Multiplayer/MultiplayerConstants.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Multiplayer/SignalRMsgPackEntitySerialiser.h"

#ifdef CSP_WASM
#include "Multiplayer/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.h"
#else
#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
#endif

#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <unordered_set>
#include <utility>

using namespace std::chrono_literals;

namespace
{

const csp::common::String SequenceTypeName = "EntityHierarchy";

uint64_t ParseGenerateObjectIDsResult(const signalr::value& Result)
{
    uint64_t EntityId = 0;

    if (Result.is_array())
    {
        const std::vector<signalr::value>& Ids = Result.as_array();

        // GenerateObjectIds can in theory create multiple Ids at once,
        // but we just assume one for now
        for (const signalr::value& IdValue : Ids)
        {
            if (IdValue.is_uinteger())
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Verbose, "Entity Id=%i", IdValue.as_uinteger());
                EntityId = IdValue.as_uinteger();
                break;
            }

            assert(false && "Unsupported Entity Id type!");
        }
    }

    return EntityId;
}

csp::common::String JSONStringFromDeltaTime(double DeltaTime)
{
    return csp::common::StringFormat("{"
                                     "\"deltaTimeMS\""
                                     ": %lf}",
        DeltaTime);
}

} // namespace

template class csp::common::List<csp::multiplayer::SpaceEntity*>;

namespace csp::multiplayer
{

constexpr uint64_t ENTITY_PAGE_LIMIT = 100;

class SpaceEntityEventHandler : public csp::events::EventListener
{
public:
    SpaceEntityEventHandler(SpaceEntitySystem* EntitySystem);

    void OnEvent(const csp::events::Event& InEvent) override;

private:
    SpaceEntitySystem* EntitySystem;
};

SpaceEntityEventHandler::SpaceEntityEventHandler(SpaceEntitySystem* EntitySystem)
    : EntitySystem(EntitySystem)
{
}

void SpaceEntityEventHandler::OnEvent(const csp::events::Event& InEvent)
{
    if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && EntitySystem->MultiplayerConnectionInst != nullptr
        && EntitySystem->MultiplayerConnectionInst->Connected)
    {
        EntitySystem->TickEntities();
    }
    else if (InEvent.GetId() == csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID)
    {
        auto Connection = EntitySystem->MultiplayerConnectionInst;
        csp::common::String Reason(InEvent.GetString("Reason"));

        auto Done = false;
        Connection->DisconnectWithReason(Reason, [&Done](ErrorCode Error) { Done = true; });

        int TimeoutCounter = 2000;

        while (!Done && TimeoutCounter > 0)
        {
            std::this_thread::sleep_for(1ms);

            --TimeoutCounter;
        }
    }
}

std::map<uint64_t, signalr::value> GetEntityTransformComponents(const SpaceEntity* InEntity)
{
    const SpaceTransform Transform(InEntity->GetTransform());
    const std::vector<signalr::value> Position { Transform.Position.X, Transform.Position.Y, Transform.Position.Z };
    const std::vector<signalr::value> Rotation { Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z, Transform.Rotation.W };
    const std::vector<signalr::value> Scale { Transform.Scale.X, Transform.Scale.Y, Transform.Scale.Z };

    std::map<uint64_t, signalr::value> Components { {
                                                        ENTITY_POSITION,
                                                        std::vector<signalr::value> {
                                                            msgpack_typeids::ItemComponentData::NULLABLE_FLOAT_ARRAY,
                                                            std::vector { signalr::value(Position) },
                                                        },
                                                    },
        {
            ENTITY_ROTATION,
            std::vector<signalr::value> {
                msgpack_typeids::ItemComponentData::NULLABLE_FLOAT_ARRAY,
                std::vector { signalr::value(Rotation) },
            },
        },
        {
            ENTITY_SCALE,
            std::vector<signalr::value> {
                msgpack_typeids::ItemComponentData::NULLABLE_FLOAT_ARRAY,
                std::vector { signalr::value(Scale) },
            },
        } };

    return Components;
}

class DirtyComponent;

using namespace std::chrono;

SpaceEntitySystem::SpaceEntitySystem(MultiplayerConnection* InMultiplayerConnection)
    : EntitiesLock(CSP_NEW std::recursive_mutex)
    , MultiplayerConnectionInst(InMultiplayerConnection)
    , Connection(nullptr)
    , EventHandler(CSP_NEW SpaceEntityEventHandler(this))
    , ElectionManager(nullptr)
    , TickEntitiesLock(CSP_NEW std::mutex)
    , PendingAdds(CSP_NEW(SpaceEntityQueue))
    , PendingRemoves(CSP_NEW(SpaceEntityQueue))
    , PendingOutgoingUpdateUniqueSet(CSP_NEW(SpaceEntitySet))
    , PendingIncomingUpdates(CSP_NEW(PatchMessageQueue))
    , EnableEntityTick(false)
    , LastTickTime(std::chrono::system_clock::now())
    , EntityPatchRate(90)
{
    Initialise();
}

SpaceEntitySystem::~SpaceEntitySystem()
{
    Shutdown();

    CSP_DELETE(EventHandler);

    CSP_DELETE(TickEntitiesLock);
    CSP_DELETE(EntitiesLock);

    CSP_DELETE(PendingAdds);
    CSP_DELETE(PendingRemoves);
    CSP_DELETE(PendingOutgoingUpdateUniqueSet);
    CSP_DELETE(PendingIncomingUpdates);
}

void SpaceEntitySystem::Initialise()
{
    if (IsInitialised)
    {
        return;
    }

    EnableLeaderElection();

    ScriptBinding = EntityScriptBinding::BindEntitySystem(this);

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().RegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);

    IsInitialised = true;
}

void SpaceEntitySystem::Shutdown()
{
    if (!IsInitialised)
    {
        return;
    }

    DisableLeaderElection();
    LocalDestroyAllEntities();

    EntityScriptBinding::RemoveBinding(ScriptBinding);

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);

    IsInitialised = false;
}

void SpaceEntitySystem::CreateAvatar(const csp::common::String& InName, const SpaceTransform& InSpaceTransform, AvatarState InState,
    const csp::common::String& InAvatarId, AvatarPlayMode InAvatarPlayMode, EntityCreatedCallback Callback)
{
    const std::function<void(signalr::value, std::exception_ptr)> LocalIDCallback
        = [this, InName, InSpaceTransform, InState, InAvatarId, InAvatarPlayMode, Callback](
              const signalr::value& Result, const std::exception_ptr& Except)
    {
        try
        {
            if (Except)
            {
                std::rethrow_exception(Except);
            }
        }
        catch (const std::exception& e)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to generate object ID. Exception: %s", e.what());
            Callback(nullptr);
        }

        const auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

        auto* NewAvatar = CSP_NEW SpaceEntity(this);
        NewAvatar->Type = SpaceEntityType::Avatar;
        const auto ID = ParseGenerateObjectIDsResult(Result);
        NewAvatar->Id = ID;
        NewAvatar->Name = InName;
        NewAvatar->Transform = InSpaceTransform;
        NewAvatar->OwnerId = MultiplayerConnectionInst->GetClientId();
        NewAvatar->IsTransferable = false;
        NewAvatar->IsPersistant = false;

        auto* AvatarComponent = static_cast<AvatarSpaceComponent*>(NewAvatar->AddComponent(ComponentType::AvatarData));
        AvatarComponent->SetAvatarId(InAvatarId);
        AvatarComponent->SetState(InState);
        AvatarComponent->SetAvatarPlayMode(InAvatarPlayMode);
        AvatarComponent->SetUserId(UserSystem->GetLoginState().UserId);

        SignalRMsgPackEntitySerialiser Serialiser;

        NewAvatar->Serialise(Serialiser);
        const auto SerialisedUser = Serialiser.Finalise();

        const std::vector InvokeArguments = { SerialisedUser };

        const std::function LocalSendCallback = [this, Callback, NewAvatar](const signalr::value& /*Result*/, const std::exception_ptr& Except)
        {
            try
            {
                if (Except)
                {
                    std::rethrow_exception(Except);
                }
            }
            catch (const std::exception& e)
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to create Avatar. Exception: %s", e.what());
                Callback(nullptr);
            }

            std::scoped_lock EntitiesLocker(*EntitiesLock);

            Entities.Append(NewAvatar);
            Avatars.Append(NewAvatar);
            NewAvatar->ApplyLocalPatch(false);

            if (ElectionManager != nullptr)
            {
                ElectionManager->OnLocalClientAdd(NewAvatar, Avatars);
            }
            Callback(NewAvatar);
        };

        Connection->Invoke("SendObjectMessage", InvokeArguments, LocalSendCallback);
    };

    // ReSharper disable once CppRedundantCastExpression, this is needed for Android builds to play nice
    const signalr::value Param1((uint64_t)1ULL);
    const std::vector Arr { Param1 };

    const signalr::value Params(Arr);
    Connection->Invoke("GenerateObjectIds", Params, LocalIDCallback);
}

void SpaceEntitySystem::CreateObject(const csp::common::String& InName, const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback)
{
    CreateObjectInternal(InName, nullptr, InSpaceTransform, Callback);
}

void SpaceEntitySystem::DestroyEntity(SpaceEntity* Entity, CallbackHandler Callback)
{
    const auto& Children = Entity->ChildEntities;

    const std::function LocalCallback = [this, Callback, Children](const signalr::value& /*EntityMessage*/, const std::exception_ptr& Except)
    {
        try
        {
            if (Except)
            {
                std::rethrow_exception(Except);
            }
        }
        catch (const std::exception& e)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to destroy entity. Exception: %s", e.what());
            Callback(false);
        }

        Callback(true);
    };

    std::vector<signalr::value> ObjectPatches;

    const std::vector<signalr::value> DeletionPatch { Entity->GetId(), MultiplayerConnectionInst->GetClientId(), true,
        std::vector<signalr::value> {
            false,
            signalr::value_type::null,
        },
        {} };

    ObjectPatches.push_back(signalr::value { DeletionPatch });

    // Move children to the root in the same patch
    for (size_t i = 0; i < Children.Size(); ++i)
    {
        const std::vector<signalr::value> ChildParentIdPatch { Children[i]->GetId(), MultiplayerConnectionInst->GetClientId(), false,
            std::vector<signalr::value> {
                true, // Update Parent
                signalr::value_type::null, // Move to root
            },
            {} };

        ObjectPatches.push_back(signalr::value { ChildParentIdPatch });
    }

    auto EntityComponents = Entity->GetComponents();
    auto Keys = EntityComponents->Keys();

    for (size_t i = 0; i < Keys->Size(); ++i)
    {
        auto EntityComponent = Entity->GetComponent((*Keys)[i]);
        EntityComponent->OnLocalDelete();
    }

    CSP_DELETE(Keys);

    csp::common::Array<ComponentUpdateInfo> Info;

    RootHierarchyEntities.RemoveItem(Entity);

    // Manually process the parent updates locally
    // We want this callback to fire before the deletion so clients can react to children first
    auto ChildrenToUpdate = Children;

    for (size_t i = 0; i < ChildrenToUpdate.Size(); ++i)
    {
        ChildrenToUpdate[i]->ParentId = nullptr;
        ResolveEntityHierarchy(ChildrenToUpdate[i]);

        if (ChildrenToUpdate[i]->EntityUpdateCallback)
        {
            ChildrenToUpdate[i]->EntityUpdateCallback(ChildrenToUpdate[i], UPDATE_FLAGS_PARENT, Info);
        }
    }

    // We break the usual pattern of not considering local state to be true until we get the ack back from CHS here
    // and instead immediately delete the local view of the entity before issuing the delete for the remote view.
    // We do this so that clients can immediately respond to the deletion and avoid sending further updates for the
    // entity that has been scheduled for deletion.
    LocalDestroyEntity(Entity);

    const std::vector InvokeArguments = { signalr::value(ObjectPatches) };
    Connection->Invoke("SendObjectPatches", InvokeArguments, LocalCallback);
}

void SpaceEntitySystem::LocalDestroyEntity(SpaceEntity* Entity)
{
    if (Entity != nullptr)
    {
        if (Entity->EntityDestroyCallback != nullptr)
        {
            Entity->EntityDestroyCallback(true);
        }

        RemoveEntity(Entity);
    }
}

SpaceEntity* SpaceEntitySystem::FindSpaceEntity(const csp::common::String& InName)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        if (Entities[i]->GetName() == InName)
        {
            return Entities[i];
        }
    }

    return nullptr;
}

SpaceEntity* SpaceEntitySystem::FindSpaceEntityById(uint64_t EntityId)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        if (Entities[i]->GetId() == EntityId)
        {
            return Entities[i];
        }
    }

    return nullptr;
}

SpaceEntity* SpaceEntitySystem::FindSpaceAvatar(const csp::common::String& InName)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    for (size_t i = 0; i < Avatars.Size(); ++i)
    {
        if (Avatars[i]->GetName() == InName)
        {
            return Avatars[i];
        }
    }

    return nullptr;
}

SpaceEntity* SpaceEntitySystem::FindSpaceObject(const csp::common::String& InName)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    for (size_t i = 0; i < Objects.Size(); ++i)
    {
        if (Objects[i]->GetName() == InName)
        {
            return Objects[i];
        }
    }

    return nullptr;
}

void SpaceEntitySystem::RegisterEntityScriptAsModule(SpaceEntity* NewEntity)
{
    EntityScript* Script = NewEntity->GetScript();
    Script->RegisterSourceAsModule();
}

void SpaceEntitySystem::BindNewEntityToScript(SpaceEntity* NewEntity)
{
    EntityScript* Script = NewEntity->GetScript();
    Script->Bind();
    Script->Invoke();
}

void SpaceEntitySystem::SetEntityCreatedCallback(EntityCreatedCallback Callback)
{
    if (SpaceEntityCreatedCallback)
    {
        CSP_LOG_WARN_MSG("SpaceEntityCreatedCallback has already been set. Previous callback overwritten.");
    }

    SpaceEntityCreatedCallback = std::move(Callback);
}

void SpaceEntitySystem::SetInitialEntitiesRetrievedCallback(CallbackHandler Callback)
{
    if (InitialEntitiesRetrievedCallback)
    {
        CSP_LOG_WARN_MSG("InitialEntitiesRetrievedCallback has already been set. Previous callback overwritten.");
    }

    InitialEntitiesRetrievedCallback = std::move(Callback);
}

void SpaceEntitySystem::SetScriptSystemReadyCallback(CallbackHandler Callback)
{
    if (ScriptSystemReadyCallback)
    {
        CSP_LOG_WARN_MSG("ScriptSystemReadyCallback has already been set. Previous callback overwritten.");
    }

    ScriptSystemReadyCallback = std::move(Callback);

    if (ElectionManager)
    {
        ElectionManager->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);
    }
}

static SpaceEntity* CreateRemotelyRetrievedEntity(const signalr::value& EntityMessage, SpaceEntitySystem* EntitySystem)
{
    SignalRMsgPackEntityDeserialiser Deserialiser(EntityMessage);

    const auto NewEntity = CSP_NEW SpaceEntity(EntitySystem);
    NewEntity->Deserialise(Deserialiser);

    EntitySystem->AddEntity(NewEntity);

    return NewEntity;
}

void SpaceEntitySystem::BindOnObjectMessage()
{
    Connection->On("OnObjectMessage",
        [this](const signalr::value& Params)
        {
            // Params is an array of all params sent, so grab the first
            auto& EntityMessage = Params.as_array()[0];

            SpaceEntity* NewEntity = CreateRemotelyRetrievedEntity(EntityMessage, this);

            if (SpaceEntityCreatedCallback)
            {
                SpaceEntityCreatedCallback(NewEntity);
            }
            else
            {
                CSP_LOG_WARN_MSG("Called SpaceEntityCreatedCallback without it being set! Call SetEntityCreatedCallback first!");
            }
        });
}

void SpaceEntitySystem::BindOnObjectPatch()
{
    Connection->On("OnObjectPatch",
        [this](const signalr::value& Params)
        {
            std::scoped_lock EntitiesLocker(*EntitiesLock);

            // Params is an array of all params sent, so grab the first
            auto& EntityMessage = Params.as_array()[0];

            PendingIncomingUpdates->emplace_back(CSP_NEW signalr::value(EntityMessage));
        });
}

void SpaceEntitySystem::BindOnRequestToSendObject()
{
    Connection->On("OnRequestToSendObject",
        [this](const signalr::value& Params)
        {
            const uint64_t EntityID = Params.as_array()[0].as_uinteger();

            // TODO: add ability to check for ID or get by ID from Entity List (maybe change to Map<EntityID, Entity> ?)
            if (SpaceEntity* MatchedEntity = FindSpaceEntityById(EntityID))
            {
                SignalRMsgPackEntitySerialiser Serialiser;

                MatchedEntity->Serialise(Serialiser);
                const auto SerialisedObject = Serialiser.Finalise();

                std::vector const InvokeArguments = { SerialisedObject };

                const std::function LocalSendCallback = [this](const signalr::value&, const std::exception_ptr& Except)
                { HandleException(Except, "Failed to send server requested object."); };

                Connection->Invoke("SendObjectMessage", InvokeArguments, LocalSendCallback);
            }
            else
            {
                std::vector<signalr::value> const InvokeArguments = { EntityID };

                Connection->Invoke("SendObjectNotFound", InvokeArguments);
            }
        });
}

void SpaceEntitySystem::BindOnRequestToDisconnect() const
{
    Connection->On("OnRequestToDisconnect",
        [this](const signalr::value& Params)
        {
            const std::string Reason = Params.as_array()[0].as_string();

            csp::events::Event* DisconnectEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID);
            DisconnectEvent->AddString("Reason", Reason.c_str());
            csp::events::EventSystem::Get().EnqueueEvent(DisconnectEvent);
        });
}

void SpaceEntitySystem::SetConnection(csp::multiplayer::SignalRConnection* InConnection)
{
    Connection = InConnection;

    BindOnObjectMessage();

    BindOnObjectPatch();

    BindOnRequestToSendObject();

    BindOnRequestToDisconnect();
}

void SpaceEntitySystem::GetEntitiesPaged(int Skip, int Limit, const std::function<void(const signalr::value&, std::exception_ptr)>& Callback)
{
    std::vector<signalr::value> ParamsVec;
    ParamsVec.push_back(signalr::value(true)); // excludeClientOwned
    ParamsVec.push_back(signalr::value(true)); // includeClientOwnedPersistentObjects
    ParamsVec.push_back(signalr::value((uint64_t)Skip)); // skip
    ParamsVec.push_back(signalr::value((uint64_t)Limit)); // limit
    const auto Params = signalr::value(std::move(ParamsVec));

    Connection->Invoke("PageScopedObjects", Params, Callback);
}

std::function<void(const signalr::value&, std::exception_ptr)> SpaceEntitySystem::CreateRetrieveAllEntitiesCallback(int Skip)
{
    const std::function Callback = [this, Skip](const signalr::value& Result, std::exception_ptr Except)
    {
        HandleException(Except, "Failed to retrieve paged entities.");

        const auto& Results = Result.as_array();
        const auto& Items = Results[0].as_array();
        auto ItemTotalCount = Results[1].as_uinteger();

        for (const auto& EntityMessage : Items)
        {
            SpaceEntity* NewEntity = CreateRemotelyRetrievedEntity(EntityMessage, this);

            if (SpaceEntityCreatedCallback)
            {
                SpaceEntityCreatedCallback(NewEntity);
            }
            else
            {
                CSP_LOG_WARN_MSG("Called SpaceEntityCreatedCallback without it being set! Call SetEntityCreatedCallback first!");
            }
        }

        int CurrentEntityCount = Skip + static_cast<int>(Items.size());

        if (CurrentEntityCount < ItemTotalCount)
        {
            GetEntitiesPaged(CurrentEntityCount, ENTITY_PAGE_LIMIT, CreateRetrieveAllEntitiesCallback(CurrentEntityCount));
        }
        else
        {
            OnAllEntitiesCreated();
        }
    };

    return Callback;
};

void SpaceEntitySystem::RetrieveAllEntities()
{
    if (Connection == nullptr)
    {
        return;
    }

    GetEntitiesPaged(0, ENTITY_PAGE_LIMIT, CreateRetrieveAllEntitiesCallback(0)); // Get at most ENTITY_PAGE_LIMIT entities at a time
}

void SpaceEntitySystem::LocalDestroyAllEntities()
{
    LockEntityUpdate();

    const auto NumEntities = GetNumEntities();

    for (size_t i = 0; i < NumEntities; ++i)
    {
        SpaceEntity* Entity = GetEntityByIndex(i);

        // We automatically invoke SignalR deletion for all transient entities that were owned by this local client
        // as these are only ever valid for a single connected session
        if (Entity->GetIsTransient() && Entity->GetOwnerId() == csp::systems::SystemsManager::Get().GetMultiplayerConnection()->GetClientId())
        {
            DestroyEntity(Entity, [](auto Ok) {});
        }
        // Otherwise we clear up all all locally represented entities
        else
        {
            LocalDestroyEntity(Entity);
        }

        CSP_DELETE(Entity);
    }

    Entities.Clear();
    Objects.Clear();
    Avatars.Clear();
    RootHierarchyEntities.Clear();

    // Clear adds/removes, we don't want to mutate if we're cleaning everything else.
    PendingAdds->clear();
    PendingRemoves->clear();
    PendingIncomingUpdates->clear();

    UnlockEntityUpdate();
}

void SpaceEntitySystem::QueueEntityUpdate(SpaceEntity* EntityToUpdate)
{
    // If we have nothing to update, don't allow a patch to be sent.
    if (EntityToUpdate->DirtyComponents.Size() == 0 && EntityToUpdate->DirtyProperties.Size() == 0
        && EntityToUpdate->TransientDeletionComponentIds.Size() == 0 && EntityToUpdate->ShouldUpdateParent == false)
    {
        // TODO: consider making this a callback that informs the user what the status of the request is 'Success, SignalRException, NoChanges', etc.
        // CSP_LOG_MSG(csp::systems::LogLevel::Log, "Skipped patch message send as no data changed");
        return;
    }

    // If the entity is not owned by us, and not a transferable entity, it is not allowed to modify the entity.
    if (!EntityToUpdate->IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Error: Update attempted on a non-owned entity that is marked as non-transferable. Skipping update. Entity name: %s",
            EntityToUpdate->GetName().c_str());
        return;
    }

    // Note that calling Queue many times will be ignored by this emplace call, but may have performance impact in some situations.
    // TODO: consider enabling clients to queue at sensible rates by exposing update rates/next update callbacks/timings.
    PendingOutgoingUpdateUniqueSet->emplace(EntityToUpdate);
}

void SpaceEntitySystem::RemoveEntity(SpaceEntity* EntityToRemove)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    PendingRemoves->emplace_back(EntityToRemove);

    // Remove from the unique set to indicate it could be queued again if needed.
    PendingOutgoingUpdateUniqueSet->erase(EntityToRemove);
}

void SpaceEntitySystem::TickEntities()
{
    ProcessPendingEntityOperations();

    if (EnableEntityTick)
    {
        TickEntityScripts();
    }

    {
        std::scoped_lock TickEntitiesLocker(*TickEntitiesLock);

        // Remove any duplicate Entities
        constexpr std::less<SpaceEntity*> PointCmp;
        TickUpdateEntities.sort(PointCmp);
        TickUpdateEntities.unique(PointCmp);

        for (const auto Entity : TickUpdateEntities)
        {
            QueueEntityUpdate(Entity);
        }

        TickUpdateEntities.clear();
    }
}

void SpaceEntitySystem::MarkEntityForUpdate(SpaceEntity* Entity)
{
    std::scoped_lock TickEntitiesLocker(*TickEntitiesLock);

    // Duplicates can be added here and will be ignored when sending updates
    TickUpdateEntities.push_back(Entity);
}

void SpaceEntitySystem::OnAllEntitiesCreated()
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    // Ensure entity list is up to date
    ProcessPendingEntityOperations();

    // Register all scripts for import
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        EntityScript* Script = Entities[i]->GetScript();
        Script->RegisterSourceAsModule();
    }

    // Bind and invoke all scripts
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        EntityScript* Script = Entities[i]->GetScript();

        Script->Bind();
        Script->Invoke();
    }

    // Tell all scripts that all entities are now loaded
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        EntityScript* Script = Entities[i]->GetScript();
        Script->PostMessageToScript(SCRIPT_MSG_ENTITIES_LOADED);
    }

    if (IsLeaderElectionEnabled())
    {
        // Start listening for election events
        //
        // If we are the first client to connect then this
        // will also set this client as the leader
        ElectionManager->OnConnect(Avatars, Objects);
    }
    else
    {
        DetermineScriptOwners();
    }

    // Enable entity tick events
    EnableEntityTick = true;

    if (InitialEntitiesRetrievedCallback)
    {
        InitialEntitiesRetrievedCallback(true);
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
void SpaceEntitySystem::DetermineScriptOwners()
{
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        ClaimScriptOwnership(Entities[i]);
    }
}

void SpaceEntitySystem::ResolveParentChildForDeletion(SpaceEntity* Deletion)
{
    if (Deletion->GetParentEntity())
    {
        Deletion->GetParentEntity()->ChildEntities.RemoveItem(Deletion);
    }

    for (size_t i = 0; i < Deletion->ChildEntities.Size(); ++i)
    {
        Deletion->ChildEntities[i]->RemoveParentEntity();
        Deletion->ChildEntities[i]->Parent = nullptr;
        ResolveEntityHierarchy(Deletion->ChildEntities[i]);
    }
}

void SpaceEntitySystem::ResolveEntityHierarchy(SpaceEntity* Entity)
{
    if (Entity->ParentId.HasValue())
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
        if (EntityIsInRootHierarchy(Entity) == false)
        {
            RootHierarchyEntities.Append(Entity);
        }
    }

    Entity->ResolveParentChildRelationship();
}

bool SpaceEntitySystem::EntityIsInRootHierarchy(SpaceEntity* Entity)
{
    for (size_t i = 0; i < RootHierarchyEntities.Size(); ++i)
    {
        if (RootHierarchyEntities[i]->GetId() == Entity->GetId())
        {
            return true;
        }
    }

    return false;
}

void SpaceEntitySystem::ClaimScriptOwnershipFromClient(uint64_t ClientId)
{
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        if (Entities[i]->GetScript()->GetOwnerId() == ClientId)
        {
            ClaimScriptOwnership(Entities[i]);
        }
    }
}

void SpaceEntitySystem::TickEntityScripts()
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    const auto CurrentTime = std::chrono::system_clock::now();
    const auto DeltaTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(CurrentTime - LastTickTime).count();
    LastTickTime = CurrentTime;

    const csp::common::String DeltaTimeJSON = JSONStringFromDeltaTime(static_cast<double>(DeltaTimeMS));

    if (IsLeaderElectionEnabled())
    {
        if (ElectionManager->IsLocalClientLeader())
        {
            for (size_t i = 0; i < Entities.Size(); ++i)
            {
                Entities[i]->GetScript()->PostMessageToScript(SCRIPT_MSG_ENTITY_TICK, DeltaTimeJSON);
            }
        }
    }
    else
    {
        const uint64_t ClientId = MultiplayerConnectionInst->GetClientId();

        for (size_t i = 0; i < Entities.Size(); ++i)
        {
            if (ClientId == Entities[i]->GetScript()->GetOwnerId())
            {
                Entities[i]->GetScript()->PostMessageToScript(SCRIPT_MSG_ENTITY_TICK, DeltaTimeJSON);
            }
        }
    }
}

bool SpaceEntitySystem::SetSelectionStateOfEntity(const bool SelectedState, SpaceEntity* Entity)
{

    if (SelectedState && !Entity->IsSelected())
    {
        if (Entity->InternalSetSelectionStateOfEntity(SelectedState, MultiplayerConnectionInst->GetClientId()))
        {
            if (!SelectedEntities.Contains(Entity))
            {
                SelectedEntities.Append(Entity);
            }
            return true;
        }

        return false;
    }

    if (!SelectedState && Entity->GetSelectingClientID() == MultiplayerConnectionInst->GetClientId())
    {
        if (Entity->InternalSetSelectionStateOfEntity(SelectedState, 0))
        {
            SelectedEntities.RemoveItem(Entity);
            return true;
        }

        return false;
    }

    return false;
}

void SpaceEntitySystem::EnableLeaderElection()
{
    if (ElectionManager == nullptr)
    {
        ElectionManager = CSP_NEW ClientElectionManager(this);
    }
}

void SpaceEntitySystem::DisableLeaderElection()
{
    if (ElectionManager != nullptr)
    {
        CSP_DELETE(ElectionManager);
        ElectionManager = nullptr;
    }
}

bool SpaceEntitySystem::IsLeaderElectionEnabled() const { return (ElectionManager != nullptr); }

uint64_t SpaceEntitySystem::GetLeaderId() const
{
    if (ElectionManager != nullptr && ElectionManager->GetLeader() != nullptr)
    {
        return ElectionManager->GetLeader()->GetId();
    }
    else
    {
        return 0;
    }
}

ComponentBase* SpaceEntitySystem::FindComponentById(uint16_t Id)
{
    // Search for component id across all entites
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        ComponentBase* Component = Entities[i]->GetComponents()->operator[](Id);

        if (Component)
        {
            return Component;
        }
    }

    CSP_LOG_ERROR_FORMAT("FindComponentById: Component with id: %s doesn't exist!", std::to_string(Id).c_str());

    return nullptr;
}

const bool SpaceEntitySystem::GetEntityPatchRateLimitEnabled() const { return EntityPatchRateLimitEnabled; }

void SpaceEntitySystem::SetEntityPatchRateLimitEnabled(bool Enabled) { EntityPatchRateLimitEnabled = Enabled; }

const csp::common::List<SpaceEntity*>* SpaceEntitySystem::GetRootHierarchyEntities() const { return &RootHierarchyEntities; }

bool SpaceEntitySystem::CheckIfWeShouldRunScriptsLocally() const
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
        return ElectionManager->IsLocalClientLeader();
    }
}

void SpaceEntitySystem::RunScriptRemotely(int64_t ContextId, const csp::common::String& ScriptText)
{
    // Run script on a remote leader...
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "RunScriptRemotely Script='%s'", ScriptText.c_str());

    ClientProxy* LeaderProxy = ElectionManager->GetLeader();

    if (LeaderProxy)
    {
        LeaderProxy->RunScript(ContextId, ScriptText);
    }
}

void SpaceEntitySystem::ClaimScriptOwnership(SpaceEntity* Entity) const
{
    const uint64_t ClientId = MultiplayerConnectionInst->GetClientId();
    EntityScript* Script = Entity->GetScript();
    Script->SetOwnerId(ClientId);
}

void SpaceEntitySystem::LockEntityUpdate() const { EntitiesLock->lock(); }

void SpaceEntitySystem::UnlockEntityUpdate() const { EntitiesLock->unlock(); }

size_t SpaceEntitySystem::GetNumEntities() const
{
    std::scoped_lock<std::recursive_mutex> EntitiesLocker(*EntitiesLock);
    return Entities.Size();
}

size_t SpaceEntitySystem::GetNumAvatars() const
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Avatars.Size();
}

size_t SpaceEntitySystem::GetNumObjects() const
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Objects.Size();
}

SpaceEntity* SpaceEntitySystem::GetEntityByIndex(const size_t EntityIndex)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Entities[EntityIndex];
}

SpaceEntity* SpaceEntitySystem::GetAvatarByIndex(const size_t AvatarIndex)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Avatars[AvatarIndex];
}

SpaceEntity* SpaceEntitySystem::GetObjectByIndex(const size_t ObjectIndex)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Objects[ObjectIndex];
}

void SpaceEntitySystem::AddEntity(SpaceEntity* EntityToAdd)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    PendingAdds->emplace_back(EntityToAdd);
}

void SendPatches(csp::multiplayer::SignalRConnection* Connection, const csp::common::List<SpaceEntity*> PendingEntities)
{
    const std::function LocalCallback = [](const signalr::value& /*Result*/, const std::exception_ptr& Except)
    {
        try
        {
            if (Except)
            {
                std::rethrow_exception(Except);
            }
        }
        catch (const std::exception& e)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to send list of entity update due to a signalr exception! Exception: %s", e.what());
        }
    };

    SignalRMsgPackEntitySerialiser Serialiser;
    std::vector<signalr::value> ObjectPatches;

    for (int i = 0; i < PendingEntities.Size(); ++i)
    {
        PendingEntities[i]->SerialisePatch(Serialiser);
        auto SerialisedEntity = Serialiser.Finalise();
        ObjectPatches.push_back(SerialisedEntity);
    }

    const std::vector InvokeArguments = { signalr::value(ObjectPatches) };

    Connection->Invoke("SendObjectPatches", InvokeArguments, LocalCallback);
}

void SpaceEntitySystem::ProcessPendingEntityOperations()
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    csp::common::List<SpaceEntity*> PendingEntities;
    // we run pending entity operations in a specific order
    // 1 - flush pending adds - we do this first to ensure any attempts to apply updates after are successful
    // 2 - flush pending updates - first the local representation, then the remote representation (with rate limiting)
    // 3 - flush pending removes - we do this last so any pending updates can still mutate state on entities that are pending removal

    // adds
    std::unordered_set<SpaceEntity*> AddedEntities;
    while (PendingAdds->empty() == false)
    {
        SpaceEntity* PendingAddEntity = PendingAdds->front();

        // we only want to add an entity once, even though a client could have queued it for updates multiple times
        if (AddedEntities.find(PendingAddEntity) == AddedEntities.end())
        {
            AddPendingEntity(PendingAddEntity);
            AddedEntities.emplace(PendingAddEntity);

            ResolveEntityHierarchy(PendingAddEntity);
        }
        PendingAdds->pop_front();
    }

    // local updates
    while (PendingIncomingUpdates->empty() == false)
    {
        ApplyIncomingPatch(PendingIncomingUpdates->front());
        PendingIncomingUpdates->pop_front();
    }

    // remote updates
    {
        for (auto it = PendingOutgoingUpdateUniqueSet->begin(); it != PendingOutgoingUpdateUniqueSet->end();)
        {
            SpaceEntity* PendingEntity = *it;

            const milliseconds CurrentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

            if (CurrentTime - PendingEntity->TimeOfLastPatch >= EntityPatchRate || !EntityPatchRateLimitEnabled)
            {
                // If the entity is not owned by us, and not a transferable entity, it is not allowed to modify the entity.
                if (!PendingEntity->IsModifiable())
                {
                    CSP_LOG_ERROR_FORMAT(
                        "Error: Update attempted on a non-owned entity that is marked as non-transferable. Skipping update. Entity name: %s",
                        PendingEntity->GetName().c_str());

                    it = PendingOutgoingUpdateUniqueSet->erase(it);
                    continue;
                }

                // since we are aiming to mutate the data for this entity remotely, we need to claim ownership over it
                PendingEntity->OwnerId = MultiplayerConnectionInst->GetClientId();
                ClaimScriptOwnership(PendingEntity);

                PendingEntities.Append(PendingEntity);

                if (PendingEntity->EntityPatchSentCallback != nullptr)
                {
                    PendingEntity->EntityPatchSentCallback(true);
                }

                PendingEntity->TimeOfLastPatch = CurrentTime;
                it = PendingOutgoingUpdateUniqueSet->erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Only send if there are patches in list
        if (PendingEntities.Size() != 0)
        {
            // Send list of PendingEntities to chs
            SendPatches(Connection, PendingEntities);

            // Loop through and apply local patches from generated list
            for (int i = 0; i < PendingEntities.Size(); ++i)
            {
                PendingEntities[i]->ApplyLocalPatch(true);
            }
        }
    }

    // removes
    std::unordered_set<SpaceEntity*> RemovedEntities;
    while (PendingRemoves->empty() == false)
    {
        SpaceEntity* PendingRemoveEntity = PendingRemoves->front();

        // we only want to remove an entity once, even though a client could have queued it for updates multiple times
        if (RemovedEntities.find(PendingRemoveEntity) == RemovedEntities.end())
        {
            RemovedEntities.emplace(PendingRemoveEntity);

            RemovePendingEntity(PendingRemoves->front());
        }
        PendingRemoves->pop_front();
    }
}

void SpaceEntitySystem::AddPendingEntity(SpaceEntity* EntityToAdd)
{
    if (FindSpaceEntityById(EntityToAdd->GetId()) == nullptr)
    {
        Entities.Append(EntityToAdd);

        switch (EntityToAdd->GetEntityType())
        {
        case SpaceEntityType::Avatar:
            Avatars.Append(EntityToAdd);
            OnAvatarAdd(EntityToAdd, Avatars);
            break;

        case SpaceEntityType::Object:
            Objects.Append(EntityToAdd);
            OnObjectAdd(EntityToAdd, Objects);
            break;
        }
    }
    else
    {
        CSP_LOG_ERROR_MSG("Attempted to add a pending entity that we already have!");
    }
}

void SpaceEntitySystem::RemovePendingEntity(SpaceEntity* EntityToRemove)
{
    assert(Entities.Contains(EntityToRemove));

    switch (EntityToRemove->GetEntityType())
    {
    case SpaceEntityType::Avatar:
        assert(Avatars.Contains(EntityToRemove));
        OnAvatarRemove(EntityToRemove, Avatars);
        Avatars.RemoveItem(EntityToRemove);
        break;

    case SpaceEntityType::Object:
        assert(Objects.Contains(EntityToRemove));
        OnObjectRemove(EntityToRemove, Objects);
        Objects.RemoveItem(EntityToRemove);
        break;

    default:
        assert(false && "Unhandled entity type encountered during its destruction!");
        break;
    }

    RootHierarchyEntities.RemoveItem(EntityToRemove);
    ResolveParentChildForDeletion(EntityToRemove);

    Entities.RemoveItem(EntityToRemove);

    CSP_DELETE(EntityToRemove);
}

void SpaceEntitySystem::OnAvatarAdd(const SpaceEntity* Avatar, const SpaceEntityList& Avatars)
{
    if (ElectionManager != nullptr)
    {
        // Note we are assuming Avatar==Client,
        // which is true now but may not be in the future
        ElectionManager->OnClientAdd(Avatar, Avatars);
    }
}

void SpaceEntitySystem::OnAvatarRemove(const SpaceEntity* Avatar, const SpaceEntityList& Avatars)
{
    if (ElectionManager != nullptr)
    {
        ElectionManager->OnClientRemove(Avatar, Avatars);
    }
}

void SpaceEntitySystem::OnObjectAdd(const SpaceEntity* Object, const SpaceEntityList& Objects)
{
    if (ElectionManager != nullptr)
    {
        ElectionManager->OnObjectAdd(Object, Objects);
    }
}

void SpaceEntitySystem::OnObjectRemove(const SpaceEntity* Object, const SpaceEntityList& Objects)
{
    if (ElectionManager != nullptr)
    {
        ElectionManager->OnObjectRemove(Object, Objects);
    }
}

void SpaceEntitySystem::CreateObjectInternal(const csp::common::String& InName, csp::common::Optional<uint64_t> InParent,
    const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback)
{
    const std::function LocalIDCallback
        = [this, InName, InParent, InSpaceTransform, Callback](const signalr::value& Result, const std::exception_ptr& Except)
    {
        try
        {
            if (Except)
            {
                std::rethrow_exception(Except);
            }
        }
        catch (const std::exception& e)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to generate object ID. Exception: %s", e.what());
            Callback(nullptr);
        }

        auto* NewObject = CSP_NEW SpaceEntity(this);
        NewObject->Type = SpaceEntityType::Object;
        auto ID = ParseGenerateObjectIDsResult(Result);
        NewObject->Id = ID;
        NewObject->Name = InName;
        NewObject->Transform = InSpaceTransform;
        NewObject->OwnerId = MultiplayerConnectionInst->GetClientId();
        NewObject->IsTransferable = true;

        if (InParent.HasValue())
        {
            NewObject->SetParentId(*InParent);
        }

        SignalRMsgPackEntitySerialiser Serialiser;

        NewObject->Serialise(Serialiser);
        const auto SerialisedObject = Serialiser.Finalise();

        const std::vector InvokeArguments = { SerialisedObject };

        const std::function<void(signalr::value, std::exception_ptr)> LocalSendCallback
            = [this, Callback, NewObject](const signalr::value& /*Result*/, const std::exception_ptr& Except)
        {
            try
            {
                if (Except)
                {
                    std::rethrow_exception(Except);
                }
            }
            catch (const std::exception& e)
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to create object. Exception: %s", e.what());
                Callback(nullptr);
            }

            std::scoped_lock EntitiesLocker(*EntitiesLock);

            ResolveEntityHierarchy(NewObject);

            Entities.Append(NewObject);
            Objects.Append(NewObject);
            Callback(NewObject);
        };

        Connection->Invoke("SendObjectMessage", InvokeArguments, LocalSendCallback);
    };

    // ReSharper disable once CppRedundantCastExpression, this is needed for Android builds to play nice
    const signalr::value Param1((uint64_t)1ULL);
    const std::vector Arr { Param1 };

    const signalr::value Params(Arr);
    Connection->Invoke("GenerateObjectIds", Params, LocalIDCallback);
}

void SpaceEntitySystem::ApplyIncomingPatch(const signalr::value* EntityMessage)
{
    SignalRMsgPackEntityDeserialiser Deserialiser(*EntityMessage);

    Deserialiser.EnterEntity();
    {
        const uint64_t EntityID = Deserialiser.ReadUInt64();
        const uint64_t OwnerID = Deserialiser.ReadUInt64();
        const bool Destroy = Deserialiser.ReadBool();
        bool ShouldUpdateParent = false;
        csp::common::Optional<uint64_t> ParentId = nullptr;

        if (Deserialiser.NextValueIsArray())
        {
            uint32_t size = 0;
            Deserialiser.EnterArray(size);
            {
                ShouldUpdateParent = Deserialiser.ReadBool();

                if (Deserialiser.NextValueIsNull())
                {
                    Deserialiser.Skip();
                }
                else
                {
                    ParentId = Deserialiser.ReadUInt64();
                }
            }
            Deserialiser.LeaveArray();
        }

        if (Destroy)
        {
            // Deletion
            for (int i = 0; i < Entities.Size(); ++i)
            {
                SpaceEntity* Entity = Entities[i];

                if (Entity->GetId() == EntityID)
                {
                    if (Entity->GetEntityType() == SpaceEntityType::Avatar)
                    {
                        // All clients will take ownership of deleted avatars scripts
                        // Last client which receives patch will end up with ownership
                        ClaimScriptOwnershipFromClient(Entity->GetOwnerId());

                        // Loop through all entities and check if the deleted avatar owned any of them. If they did, deselect them.
                        // This covers disconnected clients as their avatar gets cleaned up after timing out.
                        for (int j = 0; j < Entities.Size(); ++j)
                        {
                            if (Entities[j]->GetSelectingClientID() == EntityID)
                            {
                                Entities[j]->Deselect();
                                SelectedEntities.RemoveItem(Entities[j]);
                            }
                        }
                    }

                    LocalDestroyEntity(Entity);
                }
            }
        }
        else
        {
            bool EntityFound = false;

            // Update
            for (int i = 0; i < Entities.Size(); ++i)
            {
                if (Entities[i]->GetId() == EntityID)
                {
                    EntityFound = true;
                    Entities[i]->ShouldUpdateParent = ShouldUpdateParent;
                    Entities[i]->ParentId = ParentId;
                    Entities[i]->DeserialiseFromPatch(Deserialiser);
                    Entities[i]->OwnerId = OwnerID;
                }
            }

            if (!EntityFound)
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to find an entity with ID %d when recieved a patch message.", EntityID);
            }
        }
    }
    Deserialiser.LeaveEntity();
}

void SpaceEntitySystem::HandleException(const std::exception_ptr& Except, const std::string& ExceptionDescription)
{
    try
    {
        if (Except)
        {
            std::rethrow_exception(Except);
        }
    }
    catch (const std::exception& e)
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "%s Exception: %s", ExceptionDescription.c_str(), e.what());
    }
}
} // namespace csp::multiplayer
