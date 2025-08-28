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
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "MCS/MCSTypes.h"
#include "Multiplayer/Election/ClientElectionManager.h"
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

uint64_t ParseGenerateObjectIDsResult(const signalr::value& Result, csp::common::LogSystem& LogSystem)
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
                LogSystem.LogMsg(csp::common::LogLevel::Verbose, fmt::format("Entity Id={}", IdValue.as_uinteger()).c_str());
                EntityId = IdValue.as_uinteger();
                break;
            }

            assert(false && "Unsupported Entity Id type!");
        }
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, "Recieved an ID result not formatted as an array");
    }

    return EntityId;
}
} // namespace

template class csp::common::List<csp::multiplayer::SpaceEntity*>;

namespace csp::multiplayer
{

constexpr uint64_t ENTITY_PAGE_LIMIT = 100;

class SpaceEntityEventHandler : public csp::events::EventListener
{
public:
    SpaceEntityEventHandler(OnlineRealtimeEngine* EntitySystem);

    void OnEvent(const csp::events::Event& InEvent) override;

private:
    OnlineRealtimeEngine* EntitySystem;
};

SpaceEntityEventHandler::SpaceEntityEventHandler(OnlineRealtimeEngine* EntitySystem)
    : EntitySystem(EntitySystem)
{
}

void SpaceEntityEventHandler::OnEvent(const csp::events::Event& InEvent)
{
    if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && EntitySystem->GetMultiplayerConnectionInstance() != nullptr
        && EntitySystem->GetMultiplayerConnectionInstance()->IsConnected())
    {
        EntitySystem->TickEntities();
    }
    else if (InEvent.GetId() == csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID)
    {
        auto Connection = EntitySystem->GetMultiplayerConnectionInstance();
        csp::common::String Reason(InEvent.GetString("Reason"));

        auto Done = false;
        Connection->DisconnectWithReason(Reason, [&Done](ErrorCode /*Error*/) { Done = true; });

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
                                                            static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_FLOAT_ARRAY),
                                                            std::vector { signalr::value(Position) },
                                                        },
                                                    },
        {
            ENTITY_ROTATION,
            std::vector<signalr::value> {
                static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_FLOAT_ARRAY),
                std::vector { signalr::value(Rotation) },
            },
        },
        {
            ENTITY_SCALE,
            std::vector<signalr::value> {
                static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_FLOAT_ARRAY),
                std::vector { signalr::value(Scale) },
            },
        } };

    return Components;
}

class DirtyComponent;

using namespace std::chrono;

OnlineRealtimeEngine::OnlineRealtimeEngine()
    : EntitiesLock(new std::recursive_mutex)
    , MultiplayerConnectionInst(nullptr)
    , LogSystem(nullptr)
    , ScriptBinding(nullptr)
    , EventHandler(nullptr)
    , ElectionManager(nullptr)
    , TickEntitiesLock(new std::recursive_mutex)
    , PendingAdds(nullptr)
    , PendingRemoves(nullptr)
    , PendingOutgoingUpdateUniqueSet(nullptr)
    , PendingIncomingUpdates(nullptr)
    , EnableEntityTick(false)
    , LastTickTime(std::chrono::system_clock::now())
    , EntityPatchRate(90)
    , ScriptRunner(nullptr)
    , NetworkEventBus(nullptr)
{
}

OnlineRealtimeEngine::OnlineRealtimeEngine(MultiplayerConnection& InMultiplayerConnection, csp::common::LogSystem& LogSystem,
    csp::multiplayer::NetworkEventBus& NetworkEventBus, csp::common::IJSScriptRunner& ScriptRunner)
    : EntitiesLock(new std::recursive_mutex)
    , MultiplayerConnectionInst(&InMultiplayerConnection)
    , LogSystem(&LogSystem)
    , EventHandler(new SpaceEntityEventHandler(this))
    , ElectionManager(nullptr)
    , TickEntitiesLock(new std::recursive_mutex)
    , PendingAdds(new(std::deque<csp::multiplayer::SpaceEntity*>))
    , PendingRemoves(new(std::deque<csp::multiplayer::SpaceEntity*>))
    , PendingOutgoingUpdateUniqueSet(new(std::set<csp::multiplayer::SpaceEntity*>))
    , PendingIncomingUpdates(new(PatchMessageQueue))
    , EnableEntityTick(false)
    , LastTickTime(std::chrono::system_clock::now())
    , EntityPatchRate(90)
    , ScriptRunner(&ScriptRunner)
    , NetworkEventBus(&NetworkEventBus)
{
    EnableLeaderElection();

    ScriptBinding = EntityScriptBinding::BindEntitySystem(this, *this->LogSystem, *this->ScriptRunner);

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().RegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);
}

OnlineRealtimeEngine::~OnlineRealtimeEngine()
{

    DisableLeaderElection();
    LocalDestroyAllEntities();

    EntityScriptBinding::RemoveBinding(ScriptBinding, *ScriptRunner);

    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::MULTIPLAYERSYSTEM_DISCONNECT_EVENT_ID, EventHandler);

    delete (EventHandler);

    delete (TickEntitiesLock);
    delete (EntitiesLock);

    delete (PendingAdds);
    delete (PendingRemoves);
    delete (PendingOutgoingUpdateUniqueSet);
    delete (PendingIncomingUpdates);
}

std::deque<csp::multiplayer::SpaceEntity*>* OnlineRealtimeEngine::GetPendingAdds() { return PendingAdds; }

MultiplayerConnection* OnlineRealtimeEngine::GetMultiplayerConnectionInstance() const { return MultiplayerConnectionInst; }

csp::common::RealtimeEngineType OnlineRealtimeEngine::GetRealtimeEngineType() const { return csp::common::RealtimeEngineType::Online; }

namespace
{
    std::unique_ptr<csp::multiplayer::SpaceEntity> BuildNewAvatar(const csp::common::String& UserId,
        csp::multiplayer::OnlineRealtimeEngine& OnlineRealtimeEngine, csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem& LogSystem,
        uint64_t NetworkId, const csp::common::String& Name, const csp::multiplayer::SpaceTransform& Transform, bool IsVisible, uint64_t OwnerId,
        bool IsTransferable, bool IsPersistent, const csp::common::String& AvatarId, csp::multiplayer::AvatarState AvatarState,
        csp::multiplayer::AvatarPlayMode AvatarPlayMode)
    {
        auto NewAvatar = std::unique_ptr<csp::multiplayer::SpaceEntity>(new csp::multiplayer::SpaceEntity(&OnlineRealtimeEngine, ScriptRunner,
            &LogSystem, SpaceEntityType::Avatar, NetworkId, Name, Transform, OwnerId, {}, IsTransferable, IsPersistent));

        auto* AvatarComponent = static_cast<AvatarSpaceComponent*>(NewAvatar->AddComponent(ComponentType::AvatarData));
        AvatarComponent->SetAvatarId(AvatarId);
        AvatarComponent->SetState(AvatarState);
        AvatarComponent->SetAvatarPlayMode(AvatarPlayMode);
        AvatarComponent->SetUserId(UserId);
        AvatarComponent->SetIsVisible(IsVisible);

        return NewAvatar;
    }

}

async::shared_task<uint64_t> OnlineRealtimeEngine::RemoteGenerateNewAvatarId()
{
    // ReSharper disable once CppRedundantCastExpression, this is needed for Android builds to play nice
    // I suspect literally no one knows if this is still neccesary.
    const signalr::value Param1((uint64_t)1ULL);
    const std::vector Arr { Param1 };
    const signalr::value Params(Arr);

    return MultiplayerConnectionInst->GetSignalRConnection()
        ->Invoke(MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), Params, {})
        .then(multiplayer::continuations::UnwrapSignalRResultOrThrow())
        .then(
            [LogSystem = this->LogSystem](const signalr::value& Result) // Parse the ID from the server and pass it along the chain
            {
                const auto NetworkId = ParseGenerateObjectIDsResult(Result, *LogSystem);
                return NetworkId;
            })
        .share();
}

std::function<async::task<std::tuple<signalr::value, std::exception_ptr>>(uint64_t)> OnlineRealtimeEngine::SendNewAvatarObjectMessage(
    const csp::common::String& Name, const csp::common::String& UserId, const SpaceTransform& Transform, bool IsVisible,
    const csp::common::String& AvatarId, AvatarState AvatarState, AvatarPlayMode AvatarPlayMode)
{
    return [Name, UserId, Transform, IsVisible, AvatarId, AvatarState, AvatarPlayMode, this](uint64_t NetworkId) // Serialize Avatar
    {
        auto NewAvatar = BuildNewAvatar(UserId, *this, *this->ScriptRunner, *LogSystem, NetworkId, Name, Transform, IsVisible,
            MultiplayerConnectionInst->GetClientId(), false, false, AvatarId, AvatarState, AvatarPlayMode);

        const mcs::ObjectMessage Message = NewAvatar->GetStatePatcher()->CreateObjectMessage();

        SignalRSerializer Serializer;
        Serializer.WriteValue(std::vector<mcs::ObjectMessage> { Message });

        // Explicitly specify types when dealing with signalr values, initializer list schenanigans abound.
        return MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
            MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), Serializer.Get());
    };
}

std::function<void(std::tuple<async::shared_task<uint64_t>, async::task<void>>)> OnlineRealtimeEngine::CreateNewLocalAvatar(
    const csp::common::String& Name, const csp::common::String& UserId, const SpaceTransform& Transform, bool IsVisible,
    const csp::common::String& AvatarId, AvatarState AvatarState, AvatarPlayMode AvatarPlayMode, EntityCreatedCallback Callback)
{
    return [Name, UserId, Transform, IsVisible, AvatarId, AvatarState, AvatarPlayMode, this, Callback](
               std::tuple<async::shared_task<uint64_t>, async::task<void>> NetworkIdFromChain)
    {
        uint64_t NetworkId = std::get<0>(NetworkIdFromChain).get();

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
        std::unique_ptr<csp::multiplayer::SpaceEntity> NewAvatar = BuildNewAvatar(UserId, *this, *ScriptRunner, *LogSystem, NetworkId, Name,
            Transform, IsVisible, MultiplayerConnectionInst->GetClientId(), false, false, AvatarId, AvatarState, AvatarPlayMode);

        std::scoped_lock EntitiesLocker(*EntitiesLock);
        // Release to vague ownership. True ownership is blurry here. It could be shared between both Entities and Objects, or just owned by Entities.
        SpaceEntity* ReleasedAvatar = NewAvatar.release();
        Entities.Append(ReleasedAvatar);
        Avatars.Append(ReleasedAvatar);
        ReleasedAvatar->ApplyLocalPatch(false, GetMultiplayerConnectionInstance()->GetAllowSelfMessagingFlag());

        if (ElectionManager != nullptr)
        {
            ElectionManager->OnLocalClientAdd(ReleasedAvatar, Avatars, *this->NetworkEventBus);
        }
        Callback(ReleasedAvatar);
    };
}

void OnlineRealtimeEngine::CreateAvatar(const csp::common::String& Name, const csp::common::String& UserId,
    const csp::multiplayer::SpaceTransform& SpaceTransform, bool IsVisible, csp::multiplayer::AvatarState AvatarState,
    const csp::common::String& AvatarId, csp::multiplayer::AvatarPlayMode AvatarPlayMode, csp::multiplayer::EntityCreatedCallback Callback)
{
    // Ask the server for an avatar Id via "GenerateObjectIds"
    async::shared_task<uint64_t> GetAvatarNetworkIdChain = RemoteGenerateNewAvatarId();

    // Use the object ID to construct a serialized avatar and send it to the server, "SendObjectMessage"
    async::task<void> SerializeAndSendChain
        = GetAvatarNetworkIdChain.then(SendNewAvatarObjectMessage(Name, UserId, SpaceTransform, IsVisible, AvatarId, AvatarState, AvatarPlayMode))
              .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>());

    // Once the server has acknowledged our new avatar, add it to local state and give it to the client.
    // Note: The when_all is so we can reuse the remote avatar ID without having to refetch it
    async::when_all(GetAvatarNetworkIdChain, SerializeAndSendChain)
        .then(CreateNewLocalAvatar(Name, UserId, SpaceTransform, IsVisible, AvatarId, AvatarState, AvatarPlayMode, Callback))
        .then(csp::common::continuations::InvokeIfExceptionInChain(*LogSystem,
            [Callback, LogSystem = this->LogSystem]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to create Avatar. Exception: {}", exception.what()).c_str());
                Callback(nullptr);
            }));
}

void OnlineRealtimeEngine::CreateEntity(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& SpaceTransform,
    const csp::common::Optional<uint64_t>& ParentID, csp::multiplayer::EntityCreatedCallback Callback)
{
    const std::function LocalIDCallback = [this, Name, SpaceTransform, ParentID, Callback, &LogSystem = this->LogSystem](
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
            LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to generate object ID. Exception: {}", e.what()).c_str());
            Callback(nullptr);
        }

        auto ID = ParseGenerateObjectIDsResult(Result, *LogSystem);
        auto* NewObject = new SpaceEntity(this, *ScriptRunner, LogSystem, SpaceEntityType::Object, ID, Name, SpaceTransform,
            MultiplayerConnectionInst->GetClientId(), ParentID, true, true);

        const mcs::ObjectMessage Message = NewObject->GetStatePatcher()->CreateObjectMessage();

        SignalRSerializer Serializer;
        Serializer.WriteValue(std::vector<mcs::ObjectMessage> { Message });

        const std::function<void(signalr::value, std::exception_ptr)> LocalSendCallback
            = [this, Callback, NewObject, &LogSystem = this->LogSystem](const signalr::value& /*Result*/, const std::exception_ptr& Except)
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
                LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to create object. Exception: {}", e.what()).c_str());
                Callback(nullptr);
            }

            std::scoped_lock EntitiesLocker(*EntitiesLock);

            ResolveEntityHierarchy(NewObject);

            Entities.Append(NewObject);
            Objects.Append(NewObject);
            Callback(NewObject);
        };

        MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
            MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), Serializer.Get(),
            LocalSendCallback);
    };

    // ReSharper disable once CppRedundantCastExpression, this is needed for Android builds to play nice
    const signalr::value Param1((uint64_t)1ULL);
    const std::vector Arr { Param1 };

    const signalr::value Params(Arr);
    MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
        MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), Params, LocalIDCallback);
}

void OnlineRealtimeEngine::DestroyEntity(SpaceEntity* Entity, CallbackHandler Callback)
{
    const auto& Children = Entity->GetChildEntities()->ToArray();

    const std::function LocalCallback
        = [Callback, &LogSystem = this->LogSystem](const signalr::value& /*EntityMessage*/, const std::exception_ptr& Except)
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
            LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Failed to destroy entity. Exception: {}", e.what()).c_str());
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

    delete (Keys);

    RootHierarchyEntities.RemoveItem(Entity);

    RealtimeEngineUtils::LocalProcessChildUpdates(*this, RootHierarchyEntities, Entity);

    // We break the usual pattern of not considering local state to be true until we get the ack back from CHS here
    // and instead immediately delete the local view of the entity before issuing the delete for the remote view.
    // We do this so that clients can immediately respond to the deletion and avoid sending further updates for the
    // entity that has been scheduled for deletion.
    LocalDestroyEntity(Entity);

    const std::vector InvokeArguments = { signalr::value(ObjectPatches) };
    MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
        MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_PATCHES), InvokeArguments, LocalCallback);
}

void OnlineRealtimeEngine::LocalDestroyEntity(SpaceEntity* Entity)
{
    if (Entity != nullptr)
    {
        if (Entity->GetEntityDestroyCallback() != nullptr)
        {
            Entity->GetEntityDestroyCallback()(true);
        }

        RemoveEntity(Entity);
    }
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceEntity(const csp::common::String& InName)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return RealtimeEngineUtils::FindSpaceEntity(*this, InName);
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceEntityById(uint64_t EntityId)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return RealtimeEngineUtils::FindSpaceEntityById(*this, EntityId);
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceAvatar(const csp::common::String& InName)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return RealtimeEngineUtils::FindSpaceAvatar(*this, InName);
}

SpaceEntity* OnlineRealtimeEngine::FindSpaceObject(const csp::common::String& InName)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return RealtimeEngineUtils::FindSpaceObject(*this, InName);
}

void OnlineRealtimeEngine::SetRemoteEntityCreatedCallback(EntityCreatedCallback Callback)
{
    if (RemoteSpaceEntityCreatedCallback)
    {
        LogSystem->LogMsg(common::LogLevel::Warning, "RemoteSpaceEntityCreatedCallback has already been set. Previous callback overwritten.");
    }

    RemoteSpaceEntityCreatedCallback = std::move(Callback);
}

bool OnlineRealtimeEngine::AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
{
    if (!SelectedEntities.Contains(Entity))
    {
        SelectedEntities.Append(Entity);
        return true;
    }
    return false;
}

bool OnlineRealtimeEngine::RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
{
    if (SelectedEntities.Contains(Entity))
    {
        SelectedEntities.RemoveItem(Entity);
        return true;
    }
    return false;
}

void OnlineRealtimeEngine::SetScriptLeaderReadyCallback(CallbackHandler Callback)
{
    if (ScriptSystemReadyCallback)
    {
        LogSystem->LogMsg(common::LogLevel::Warning, "ScriptLeaderReadyCallback has already been set. Previous callback overwritten.");
    }

    ScriptSystemReadyCallback = std::move(Callback);

    if (ElectionManager)
    {
        ElectionManager->SetScriptLeaderReadyCallback(ScriptSystemReadyCallback);
    }
}

namespace
{
    void FireRemoteSpaceEntityCreatedCallback(
        SpaceEntity* SpaceEntity, csp::multiplayer::EntityCreatedCallback RemoteSpaceEntityCreatedCallback, csp::common::LogSystem& LogSystem)
    {
        if (RemoteSpaceEntityCreatedCallback)
        {
            RemoteSpaceEntityCreatedCallback(SpaceEntity);
        }
        else
        {
            LogSystem.LogMsg(common::LogLevel::Warning,
                "Called RemoteSpaceEntityCreatedCallback without it being set! Call SetRemoteEntityCreatedCallback first!");
        }
    }
}

SpaceEntity* OnlineRealtimeEngine::CreateRemotelyRetrievedEntity(const signalr::value& EntityMessage)
{
    //  Create object message from signalr value
    mcs::ObjectMessage Message;
    SignalRDeserializer Deserializer { EntityMessage };
    Deserializer.ReadValue(Message);

    const auto NewEntity = SpaceEntityStatePatcher::NewFromObjectMessage(Message, *this, *ScriptRunner, *LogSystem);

    AddEntity(NewEntity);

    return NewEntity;
}

void OnlineRealtimeEngine::OnObjectMessage(const signalr::value& Params)
{
    // Params is an array of all params sent, so grab the first
    auto& EntityMessage = Params.as_array()[0];

    SpaceEntity* NewEntity = CreateRemotelyRetrievedEntity(EntityMessage);

    if (NewEntity)
    {
        FireRemoteSpaceEntityCreatedCallback(NewEntity, RemoteSpaceEntityCreatedCallback, *LogSystem);
    }
}

void OnlineRealtimeEngine::OnObjectPatch(const signalr::value& Params)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);

    // Params is an array of all params sent, so grab the first
    auto& EntityMessage = Params.as_array()[0];

    PendingIncomingUpdates->emplace_back(new signalr::value(EntityMessage));
}

void OnlineRealtimeEngine::OnRequestToSendObject(const signalr::value& Params)
{
    const uint64_t EntityID = Params.as_array()[0].as_uinteger();

    // TODO: add ability to check for ID or get by ID from Entity List (maybe change to Map<EntityID, Entity> ?)
    if (SpaceEntity* MatchedEntity = FindSpaceEntityById(EntityID))
    {
        mcs::ObjectMessage Message = MatchedEntity->GetStatePatcher()->CreateObjectMessage();

        SignalRSerializer Serializer;
        Serializer.WriteValue(std::vector<mcs::ObjectMessage> { Message });

        const std::function LocalSendCallback
            = [this](const signalr::value&, const std::exception_ptr& Except) { HandleException(Except, "Failed to send server requested object."); };

        MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
            MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), Serializer.Get(),
            LocalSendCallback);
    }
    else
    {
        std::vector<signalr::value> const InvokeArguments = { EntityID };

        MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
            MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_NOT_FOUND), InvokeArguments);
    }
}

void OnlineRealtimeEngine::GetEntitiesPaged(int Skip, int Limit, const std::function<void(const signalr::value&, std::exception_ptr)>& Callback)
{
    std::vector<signalr::value> ParamsVec;
    ParamsVec.push_back(signalr::value(true)); // excludeClientOwned
    ParamsVec.push_back(signalr::value(true)); // includeClientOwnedPersistentObjects
    ParamsVec.push_back(signalr::value((uint64_t)Skip)); // skip
    ParamsVec.push_back(signalr::value((uint64_t)Limit)); // limit
    const auto Params = signalr::value(std::move(ParamsVec));

    MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
        MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::PAGE_SCOPED_OBJECTS), Params, Callback);
}

std::function<void(const signalr::value&, std::exception_ptr)> OnlineRealtimeEngine::CreateRetrieveAllEntitiesCallback(
    int Skip, csp::common::EntityFetchCompleteCallback FetchCompleteCallback)
{
    const std::function Callback = [this, Skip, FetchCompleteCallback](const signalr::value& Result, std::exception_ptr Except)
    {
        HandleException(Except, "Failed to retrieve paged entities.");

        const auto& Results = Result.as_array();
        const auto& Items = Results[0].as_array();
        auto ItemTotalCount = Results[1].as_uinteger();

        for (const auto& EntityMessage : Items)
        {
            SpaceEntity* NewEntity = CreateRemotelyRetrievedEntity(EntityMessage);
            FireRemoteSpaceEntityCreatedCallback(NewEntity, RemoteSpaceEntityCreatedCallback, *LogSystem);
        }

        int CurrentEntityCount = Skip + static_cast<int>(Items.size());

        if (static_cast<uint64_t>(CurrentEntityCount) < ItemTotalCount)
        {
            GetEntitiesPaged(CurrentEntityCount, ENTITY_PAGE_LIMIT, CreateRetrieveAllEntitiesCallback(CurrentEntityCount, FetchCompleteCallback));
        }
        else
        {
            std::scoped_lock EntitiesLocker(*EntitiesLock);
            // Ensure entity list is up to date
            ProcessPendingEntityOperations();

            RealtimeEngineUtils::InitialiseEntityScripts(Entities);
            EnableEntityTick = true;

            // Start leader election
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
                RealtimeEngineUtils::DetermineScriptOwners(Entities, GetMultiplayerConnectionInstance()->GetClientId());
            }

            if (FetchCompleteCallback)
            {
                FetchCompleteCallback(CurrentEntityCount);
            }
        }
    };

    return Callback;
};

void OnlineRealtimeEngine::FetchAllEntitiesAndPopulateBuffers(
    const csp::common::String& SpaceId, csp::common::EntityFetchStartedCallback FetchStartedCallback)
{
    /* Refresh the multiplayer connection to force the scopes to change
     * This is wrapping a yet-to-be refactored method that uses nested callbacks, hence the event, and shared pointer for lifetime */
    auto RefreshMultiplayerConnectionEvent = std::make_shared<async::event_task<std::optional<csp::multiplayer::ErrorCode>>>();
    auto RefreshMultiplayerConnectionContinuation = RefreshMultiplayerConnectionEvent->get_task();

    /* Investigate whether this needs to happen at all, it 's overwhelmingly complex... If you' re doing anything AOI,
     * this probably wants rewritten or removed along with your work. */
    RefreshMultiplayerConnectionToEnactScopeChange(SpaceId, RefreshMultiplayerConnectionEvent);

    RefreshMultiplayerConnectionContinuation.then(async::inline_scheduler(),
        [this, FetchStartedCallback]()
        {
            this->RetrieveAllEntities(EntityFetchCompleteCallback);
            FetchStartedCallback();
        });
}

void OnlineRealtimeEngine::LockEntityUpdate() { EntitiesLock->lock(); }

bool OnlineRealtimeEngine::TryLockEntityUpdate() { return EntitiesLock->try_lock(); }

void OnlineRealtimeEngine::UnlockEntityUpdate() { EntitiesLock->unlock(); }

void OnlineRealtimeEngine::RetrieveAllEntities(csp::common::EntityFetchCompleteCallback FetchCompleteCallback)
{
    if ((MultiplayerConnectionInst == nullptr) || (MultiplayerConnectionInst->GetSignalRConnection() == nullptr))
    {
        return;
    }

    GetEntitiesPaged(
        0, ENTITY_PAGE_LIMIT, CreateRetrieveAllEntitiesCallback(0, FetchCompleteCallback)); // Get at most ENTITY_PAGE_LIMIT entities at a time
}

void OnlineRealtimeEngine::LocalDestroyAllEntities()
{
    LockEntityUpdate();

    const auto NumEntities = GetNumEntities();

    for (size_t i = 0; i < NumEntities; ++i)
    {
        SpaceEntity* Entity = GetEntityByIndex(i);

        // We automatically invoke SignalR deletion for all transient entities that were owned by this local client
        // as these are only ever valid for a single connected session
        if (Entity->GetIsTransient() && Entity->GetOwnerId() == GetMultiplayerConnectionInstance()->GetClientId())
        {
            DestroyEntity(Entity, [](bool /*Ok*/) {});
        }
        // Otherwise we clear up all all locally represented entities
        else
        {
            LocalDestroyEntity(Entity);
        }

        delete (Entity);
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

void OnlineRealtimeEngine::QueueEntityUpdate(SpaceEntity* EntityToUpdate)
{
    // If we have nothing to update, don't allow a patch to be sent.
    if (!EntityToUpdate->GetStatePatcher()->HasPendingPatch())
    {
        // TODO: consider making this a callback that informs the user what the status of the request is 'Success, SignalRException, NoChanges',
        // etc. CSP_LOG_MSG(csp::common::LogLevel::Log, "Skipped patch message send as no data changed");
        return;
    }

    // If the entity is not owned by us, and not a transferable entity, it is not allowed to modify the entity.
    if (!EntityToUpdate->IsModifiable())
    {
        LogSystem->LogMsg(common::LogLevel::Error,
            fmt::format("Error: Update attempted on a non-owned entity that is marked as non-transferable. Skipping update. Entity name: {}",
                EntityToUpdate->GetName())
                .c_str());
        return;
    }

    // Note that calling Queue many times will be ignored by this emplace call, but may have performance impact in some situations.
    // TODO: consider enabling clients to queue at sensible rates by exposing update rates/next update callbacks/timings.
    PendingOutgoingUpdateUniqueSet->emplace(EntityToUpdate);
}

void OnlineRealtimeEngine::RemoveEntity(SpaceEntity* EntityToRemove)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    PendingRemoves->emplace_back(EntityToRemove);

    // Remove from the unique set to indicate it could be queued again if needed.
    PendingOutgoingUpdateUniqueSet->erase(EntityToRemove);
}

void OnlineRealtimeEngine::TickEntities()
{
    ProcessPendingEntityOperations();

    if (EnableEntityTick)
    {
        LastTickTime = RealtimeEngineUtils::TickEntityScripts(
            *TickEntitiesLock, GetRealtimeEngineType(), MultiplayerConnectionInst->GetClientId(), Entities, LastTickTime);
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

bool OnlineRealtimeEngine::EntityIsInRootHierarchy(SpaceEntity* Entity)
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

void OnlineRealtimeEngine::ClaimScriptOwnershipFromClient(uint64_t ClientId)
{
    for (size_t i = 0; i < Entities.Size(); ++i)
    {
        if (Entities[i]->GetScript().GetOwnerId() == ClientId)
        {
            RealtimeEngineUtils::ClaimScriptOwnership(Entities[i], GetMultiplayerConnectionInstance()->GetClientId());
        }
    }
}

void OnlineRealtimeEngine::ClaimScriptOwnership(SpaceEntity* Entity) const
{
    RealtimeEngineUtils::ClaimScriptOwnership(Entity, GetMultiplayerConnectionInstance()->GetClientId());
}

void OnlineRealtimeEngine::EnableLeaderElection()
{
    if (ElectionManager == nullptr)
    {
        ElectionManager = new ClientElectionManager(this, *LogSystem, *ScriptRunner);
    }
}

void OnlineRealtimeEngine::DisableLeaderElection()
{
    if (ElectionManager != nullptr)
    {
        delete (ElectionManager);
        ElectionManager = nullptr;
    }
}

bool OnlineRealtimeEngine::IsLeaderElectionEnabled() const { return (ElectionManager != nullptr); }

uint64_t OnlineRealtimeEngine::GetLeaderId() const
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

bool OnlineRealtimeEngine::GetEntityPatchRateLimitEnabled() const { return EntityPatchRateLimitEnabled; }

void OnlineRealtimeEngine::SetEntityPatchRateLimitEnabled(bool Enabled) { EntityPatchRateLimitEnabled = Enabled; }

const csp::common::List<SpaceEntity*>* OnlineRealtimeEngine::GetRootHierarchyEntities() const { return &RootHierarchyEntities; }

void OnlineRealtimeEngine::ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* Entity)
{
    RealtimeEngineUtils::ResolveEntityHierarchy(*this, RootHierarchyEntities, Entity);
}

void OnlineRealtimeEngine::RefreshMultiplayerConnectionToEnactScopeChange(
    csp::common::String SpaceId, std::shared_ptr<async::event_task<std::optional<csp::multiplayer::ErrorCode>>> RefreshMultiplayerContinuationEvent)
{
    // A refactor to a regular continuation would be appreciated ... assuming we need to keep this method at all.

    // Unfortunately we have to stop listening in order for our scope change to take effect, then start again once done.
    // This hopefully will change in a future version when CHS support it.
    MultiplayerConnectionInst->StopListening(
        [MultiplayerConnection = MultiplayerConnectionInst, &LogSystem = this->LogSystem, SpaceId, RefreshMultiplayerContinuationEvent](
            csp::multiplayer::ErrorCode Error)
        {
            if (Error != csp::multiplayer::ErrorCode::None)
            {
                RefreshMultiplayerContinuationEvent->set(Error);
                return;
            }

            LogSystem->LogMsg(csp::common::LogLevel::Log, "MultiplayerConnection->StopListening success");
            MultiplayerConnection->SetScopes(SpaceId,
                [MultiplayerConnection, RefreshMultiplayerContinuationEvent, &LogSystem](csp::multiplayer::ErrorCode Error)
                {
                    LogSystem->LogMsg(csp::common::LogLevel::Verbose, "SetScopes callback");
                    if (Error != csp::multiplayer::ErrorCode::None)
                    {
                        RefreshMultiplayerContinuationEvent->set(Error);
                        return;
                    }
                    else
                    {
                        LogSystem->LogMsg(csp::common::LogLevel::Verbose, "SetScopes was called successfully");
                    }

                    MultiplayerConnection->StartListening()()
                        .then(async::inline_scheduler(),
                            [RefreshMultiplayerContinuationEvent, &LogSystem]()
                            {
                                LogSystem->LogMsg(csp::common::LogLevel::Log, " MultiplayerConnection->StartListening success");

                                // Success!
                                RefreshMultiplayerContinuationEvent->set({});
                            })
                        .then(async::inline_scheduler(),
                            csp::common::continuations::InvokeIfExceptionInChain(*LogSystem,
                                [&RefreshMultiplayerContinuationEvent](
                                    [[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
                                {
                                    // Error case
                                    auto [Error, ExceptionMsg] = csp::multiplayer::MultiplayerConnection::ParseMultiplayerError(exception);
                                    RefreshMultiplayerContinuationEvent->set(Error);
                                    return;
                                }));
                });
        });
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
        return ElectionManager->IsLocalClientLeader();
    }
}

void OnlineRealtimeEngine::RunScriptRemotely(int64_t ContextId, const csp::common::String& ScriptText)
{
    // Run script on a remote leader...
    LogSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("RunScriptRemotely Script='{}'", ScriptText).c_str());

    ClientProxy* LeaderProxy = ElectionManager->GetLeader();

    if (LeaderProxy)
    {
        LeaderProxy->RunScript(ContextId, ScriptText);
    }
}

size_t OnlineRealtimeEngine::GetNumEntities() const
{
    std::scoped_lock<std::recursive_mutex> EntitiesLocker(*EntitiesLock);
    return Entities.Size();
}

size_t OnlineRealtimeEngine::GetNumAvatars() const
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Avatars.Size();
}

size_t OnlineRealtimeEngine::GetNumObjects() const
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Objects.Size();
}

SpaceEntity* OnlineRealtimeEngine::GetEntityByIndex(const size_t EntityIndex)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Entities[EntityIndex];
}

SpaceEntity* OnlineRealtimeEngine::GetAvatarByIndex(const size_t AvatarIndex)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Avatars[AvatarIndex];
}

SpaceEntity* OnlineRealtimeEngine::GetObjectByIndex(const size_t ObjectIndex)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    return Objects[ObjectIndex];
}

const csp::common::List<SpaceEntity*>* OnlineRealtimeEngine::GetAllEntities() const { return &Entities; }

void OnlineRealtimeEngine::AddEntity(SpaceEntity* EntityToAdd)
{
    std::scoped_lock EntitiesLocker(*EntitiesLock);
    PendingAdds->emplace_back(EntityToAdd);
}

void OnlineRealtimeEngine::SendPatches(const csp::common::List<SpaceEntity*> PendingEntities)
{
    const std::function LocalCallback = [&LogSystem = this->LogSystem](const signalr::value& /*Result*/, const std::exception_ptr& Except)
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
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("Failed to send list of entity update due to a signalr exception! Exception: {}", e.what()).c_str());
        }
    };

    std::vector<mcs::ObjectPatch> Patches;
    SignalRSerializer Serializer;

    for (size_t i = 0; i < PendingEntities.Size(); ++i)
    {
        Patches.push_back(PendingEntities[i]->GetStatePatcher()->CreateObjectPatch());
    }

    // We are writing multiple patches, so we need an additional nested array.
    Serializer.StartWriteArray();
    {
        Serializer.WriteValue(Patches);
    }
    Serializer.EndWriteArray();

    MultiplayerConnectionInst->GetSignalRConnection()->Invoke(
        MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_PATCHES), Serializer.Get(), LocalCallback);
}

void OnlineRealtimeEngine::ProcessPendingEntityOperations()
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

            if (CurrentTime - PendingEntity->GetTimeOfLastPatch() >= EntityPatchRate || !EntityPatchRateLimitEnabled)
            {
                // If the entity is not owned by us, and not a transferable entity, it is not allowed to modify the entity.
                if (!PendingEntity->IsModifiable())
                {
                    LogSystem->LogMsg(common::LogLevel::Error,
                        fmt::format(
                            "Error: Update attempted on a non-owned entity that is marked as non-transferable. Skipping update. Entity name: {}",
                            PendingEntity->GetName())
                            .c_str());

                    it = PendingOutgoingUpdateUniqueSet->erase(it);
                    continue;
                }

                // since we are aiming to mutate the data for this entity remotely, we need to claim ownership over it
                PendingEntity->SetOwnerId(MultiplayerConnectionInst->GetClientId());
                RealtimeEngineUtils::ClaimScriptOwnership(PendingEntity, GetMultiplayerConnectionInstance()->GetClientId());

                PendingEntities.Append(PendingEntity);

                if (PendingEntity->GetStatePatcher()->GetEntityPatchSentCallback() != nullptr)
                {
                    PendingEntity->GetStatePatcher()->CallEntityPatchSentCallback(true);
                }

                PendingEntity->GetStatePatcher()->SetTimeOfLastPatch(CurrentTime);
                it = PendingOutgoingUpdateUniqueSet->erase(it);
            }
            else
            {
                LogSystem->LogMsg(common::LogLevel::Verbose,
                    "Skipping patch send in ProcessPendingEntityOperations as not enough time has passed since the last patch");
                ++it;
            }
        }

        // Only send if there are patches in list
        if (PendingEntities.Size() != 0)
        {
            // Send list of PendingEntities to chs
            SendPatches(PendingEntities);

            // Loop through and apply local patches from generated list
            for (size_t i = 0; i < PendingEntities.Size(); ++i)
            {
                PendingEntities[i]->ApplyLocalPatch(true, GetMultiplayerConnectionInstance()->GetAllowSelfMessagingFlag());
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

void OnlineRealtimeEngine::AddPendingEntity(SpaceEntity* EntityToAdd)
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
        LogSystem->LogMsg(common::LogLevel::Error, "Attempted to add a pending entity that we already have!");
    }
}

void OnlineRealtimeEngine::RemovePendingEntity(SpaceEntity* EntityToRemove)
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
    RealtimeEngineUtils::RemoveParentChildRelationshipsFromEntity(*this, RootHierarchyEntities, EntityToRemove);

    Entities.RemoveItem(EntityToRemove);

    delete (EntityToRemove);
}

void OnlineRealtimeEngine::OnAvatarAdd(const SpaceEntity* Avatar, const csp::common::List<SpaceEntity*>& AddedAvatars)
{
    if (ElectionManager != nullptr)
    {
        // Note we are assuming Avatar==Client,
        // which is true now but may not be in the future
        ElectionManager->OnClientAdd(Avatar, AddedAvatars, *this->NetworkEventBus);
    }
}

void OnlineRealtimeEngine::OnAvatarRemove(const SpaceEntity* Avatar, const csp::common::List<SpaceEntity*>& RemovedAvatars)
{
    if (ElectionManager != nullptr)
    {
        ElectionManager->OnClientRemove(Avatar, RemovedAvatars);
    }
}

void OnlineRealtimeEngine::OnObjectAdd(const SpaceEntity* Object, const csp::common::List<SpaceEntity*>& AddedObjects)
{
    if (ElectionManager != nullptr)
    {
        ElectionManager->OnObjectAdd(Object, AddedObjects);
    }
}

void OnlineRealtimeEngine::OnObjectRemove(const SpaceEntity* Object, const csp::common::List<SpaceEntity*>& RemovedObjects)
{
    if (ElectionManager != nullptr)
    {
        ElectionManager->OnObjectRemove(Object, RemovedObjects);
    }
}

void OnlineRealtimeEngine::ApplyIncomingPatch(const signalr::value* EntityMessage)
{
    mcs::ObjectPatch Patch;
    SignalRDeserializer Deserializer { *EntityMessage };
    Deserializer.ReadValue(Patch);

    if (Patch.GetDestroy())
    {
        // This is an entity deletion.
        for (size_t i = 0; i < Entities.Size(); ++i)
        {
            SpaceEntity* Entity = Entities[i];

            if (Entity->GetId() == Patch.GetId())
            {
                if (Entity->GetEntityType() == SpaceEntityType::Avatar)
                {
                    // All clients will take ownership of deleted avatars scripts
                    // Last client which receives patch will end up with ownership
                    ClaimScriptOwnershipFromClient(Entity->GetOwnerId());

                    // Loop through all entities and check if the deleted avatar owned any of them. If they did, deselect them.
                    // This covers disconnected clients as their avatar gets cleaned up after timing out.
                    for (size_t j = 0; j < Entities.Size(); ++j)
                    {
                        if (Entities[j]->GetSelectingClientID() == Patch.GetId())
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
        for (SpaceEntity* Entity : Entities)
        {
            if (Entity->GetId() == Patch.GetId())
            {
                Entity->GetStatePatcher()->ApplyPatchFromObjectPatch(Patch);
                EntityFound = true;
            }
        }

        if (!EntityFound)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("Failed to find an entity with ID {} when received a patch message.", Patch.GetId()).c_str());
        }
    }
}

void OnlineRealtimeEngine::HandleException(const std::exception_ptr& Except, const std::string& ExceptionDescription)
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
        LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("{0} Exception: {1}", ExceptionDescription.c_str(), e.what()).c_str());
    }
}
} // namespace csp::multiplayer
