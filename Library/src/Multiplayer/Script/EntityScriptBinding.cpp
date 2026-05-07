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
#include "Multiplayer/Script/EntityScriptBinding.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentScriptHelpers.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"
#include "Multiplayer/Script/EntityScriptInterface.h"

#include "ScriptHelpers.h"
#include "quickjspp.hpp"

#include <fmt/format.h>

#include <unordered_map>
#include <utility>

namespace csp::multiplayer
{

using SpaceEntityList = csp::common::List<SpaceEntity*>;

namespace
{
    // Because we don't put std::types across interface boundaries, we can't get a direct std::mutex to use scoped_lock. Adapt.
    template <typename LockFunc, typename UnlockFunc> class RAIILock
    {
    public:
        // A bit paranoid, but make sure that we're storing value types to prevent potential dangling/pointer lifetime issues if you're doing
        // something funky with funciton pointers.
        using LockFuncValueT = std::decay_t<LockFunc>;
        using UnlockFuncValueT = std::decay_t<UnlockFunc>;

        RAIILock(LockFunc&& Lock, UnlockFunc&& Unlock)
            : _LockFunc(std::forward<LockFunc>(Lock))
            , _UnlockFunc(std::forward<UnlockFunc>(Unlock))
        {
            std::invoke(_LockFunc);
        }

        ~RAIILock() noexcept { std::invoke(_UnlockFunc); }

        RAIILock(const RAIILock&) = delete;
        RAIILock& operator=(const RAIILock&) = delete;
        RAIILock(RAIILock&&) = delete;
        RAIILock& operator=(RAIILock&&) = delete;

    private:
        LockFuncValueT _LockFunc;
        UnlockFuncValueT _UnlockFunc;
    };

    // Creates a JS prototype for a specific component type, baking in specialized accessors
    // that map directly to the correctly typed proxy property.
    template <typename ScriptInterfaceType = ComponentScriptInterface>
    qjs::Value MakeComponentPrototype(qjs::Context& Context, const ComponentSchema& Schema)
    {
        static_assert(std::is_base_of_v<ComponentScriptInterface, ScriptInterfaceType>);

        const auto ClassId = qjs::js_traits<std::shared_ptr<ScriptInterfaceType>>::QJSClassId;
        const auto BasePrototype = qjs::Value { Context.ctx, JS_GetClassProto(Context.ctx, ClassId) };
        const auto SchemaDerivedPrototype = qjs::Value { Context.ctx, JS_NewObjectProto(Context.ctx, BasePrototype.v) };

        auto CreateAccessors = [](const auto& V) -> std::optional<std::pair<JSCFunctionMagic*, JSCFunctionMagic*>>
        {
            using T = std::decay_t<decltype(V)>;

            if constexpr (IsScriptableV<T>)
            {
                const auto Getter = [](JSContext* Context, JSValueConst This, int /*ArgC*/, JSValueConst* /*ArgV*/, int Magic) -> JSValue
                {
                    const auto Self = qjs::js_traits<std::optional<ScriptInterfaceType*>>::unwrap(Context, This);
                    if (!Self)
                    {
                        return JS_EXCEPTION;
                    }

                    auto Result = (*Self)->GetProperty(static_cast<ComponentProperty::KeyType>(Magic));
                    if (!Result)
                    {
                        return JS_EXCEPTION;
                    }

                    auto* CheckedResult = std::get_if<ScriptTypeT<T>>(&*Result);
                    if (CheckedResult == nullptr)
                    {
                        return JS_EXCEPTION;
                    }

                    return qjs::js_traits<ScriptTypeT<T>>::wrap(Context, std::move(*CheckedResult));
                };

                const auto Setter = [](JSContext* Context, JSValueConst This, int ArgC, JSValueConst* ArgV, int Magic) -> JSValue
                {
                    if (ArgC <= 0)
                    {
                        return JS_EXCEPTION;
                    }

                    const auto Self = qjs::js_traits<std::optional<ScriptInterfaceType*>>::unwrap(Context, This);
                    if (!Self)
                    {
                        return JS_EXCEPTION;
                    }

                    auto Result = qjs::js_traits<std::optional<ScriptTypeT<T>>>::unwrap(Context, ArgV[0]);
                    if (!Result)
                    {
                        return JS_EXCEPTION;
                    }

                    (*Self)->SetProperty(static_cast<ComponentProperty::KeyType>(Magic), std::move(*Result));

                    return JS_UNDEFINED;
                };

                return std::make_pair(Getter, Setter);
            }
            else
            {
                return std::nullopt;
            }
        };

        for (const auto& Property : Schema.Properties)
        {
            if (!IsScriptable(Property))
            {
                continue;
            }

            auto MaybeAccessors = std::visit(CreateAccessors, Property.DefaultValue.GetValue());
            if (!MaybeAccessors)
            {
                continue;
            }

            const auto [Getter, Setter] = *MaybeAccessors;

            const auto Magic = static_cast<int>(Property.Key);
            const auto PropertyName = JS_NewAtom(Context.ctx, Property.Name.c_str());

            JS_DefinePropertyGetSet(Context.ctx, SchemaDerivedPrototype.v, PropertyName,
                JS_NewCFunctionMagic(Context.ctx, Getter, "get", 0, JS_CFUNC_generic_magic, Magic),
                JS_NewCFunctionMagic(Context.ctx, Setter, "set", 1, JS_CFUNC_generic_magic, Magic), JS_PROP_C_W_E);

            JS_FreeAtom(Context.ctx, PropertyName);
        }

        return SchemaDerivedPrototype;
    }
}

class EntitySystemScriptInterface
{
public:
    EntitySystemScriptInterface(csp::common::IRealtimeEngine* InEntitySystem = nullptr)
        : EntitySystem(InEntitySystem)
    {
    }

    ~EntitySystemScriptInterface() { }

    std::vector<uint64_t> GetEntityIds()
    {
        std::vector<uint64_t> EntityIds;

        if (EntitySystem)
        {
            RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });
            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                const SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);

                uint64_t Id = Entity->GetId();
                EntityIds.push_back(Id);
            }
        }

        return EntityIds;
    }

    std::vector<EntityScriptInterface*> GetEntities()
    {
        std::vector<EntityScriptInterface*> Entities;

        if (EntitySystem)
        {
            RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });
            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                Entities.push_back(Entity->GetScriptInterface());
            }
        }

        return Entities;
    }

    std::vector<EntityScriptInterface*> GetObjects()
    {
        std::vector<EntityScriptInterface*> Objects;

        if (EntitySystem)
        {
            for (size_t i = 0; i < EntitySystem->GetNumObjects(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetObjectByIndex(i);
                Objects.push_back(Entity->GetScriptInterface());
            }
        }

        return Objects;
    }

    std::vector<EntityScriptInterface*> GetAvatars()
    {
        std::vector<EntityScriptInterface*> Avatars;

        if (EntitySystem)
        {
            for (size_t i = 0; i < EntitySystem->GetNumAvatars(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetAvatarByIndex(i);
                Avatars.push_back(Entity->GetScriptInterface());
            }
        }

        return Avatars;
    }

    int32_t GetIndexOfEntity(int64_t EntityId)
    {
        int32_t IndexOfEntity = -1;

        if (EntitySystem)
        {
            RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                const SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);

                if (Entity->GetId() == static_cast<uint64_t>(EntityId))
                {
                    IndexOfEntity = static_cast<int32_t>(i);
                    break;
                }
            }
        }

        return IndexOfEntity;
    }

    EntityScriptInterface* GetEntityById(int64_t EntityId)
    {
        EntityScriptInterface* ScriptInterface = nullptr;
        if (EntitySystem)
        {
            RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                if (Entity->GetId() == static_cast<uint64_t>(EntityId))
                {
                    ScriptInterface = Entity->GetScriptInterface();
                    break;
                }
            }
        }

        return ScriptInterface;
    }

    EntityScriptInterface* GetEntityByName(std::string EntityName)
    {
        EntityScriptInterface* ScriptInterface = nullptr;
        if (EntitySystem)
        {
            RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                if (Entity->GetName() == EntityName.c_str())
                {
                    ScriptInterface = Entity->GetScriptInterface();
                    break;
                }
            }
        }

        return ScriptInterface;
    }

    std::vector<EntityScriptInterface*> GetRootHierarchyEntities()
    {
        std::vector<EntityScriptInterface*> RootHierarchyEntities;
        if (EntitySystem)
        {
            RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < EntitySystem->GetRootHierarchyEntities()->Size(); ++i)
            {
                SpaceEntity* Entity = (*EntitySystem->GetRootHierarchyEntities())[i];
                RootHierarchyEntities.push_back(Entity->GetScriptInterface());
            }
        }

        return RootHierarchyEntities;
    }

    std::string GetFoundationVersion() { return csp::CSPFoundation::GetVersion().c_str(); }

private:
    csp::common::IRealtimeEngine* EntitySystem;
};

void EntityScriptLog(qjs::rest<std::string> Args, csp::common::LogSystem& LogSystem)
{
    std::stringstream Str;

    for (auto const& Arg : Args)
    {
        Str << Arg << " ";
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log, Str.str().c_str());
}

class EntityScriptBinding::SchemaCacheImpl
{
public:
    std::vector<qjs::Value> GetComponents(qjs::Context& Context, EntityScriptInterface& Entity, const ComponentSchema& Schema)
    {
        switch (static_cast<ComponentType>(Schema.TypeId))
        {
        case ComponentType::VideoPlayer:
            return GetComponents<VideoPlayerSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Custom:
            return GetComponents<CustomSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Audio:
            return GetComponents<AudioSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Hotspot:
            return GetComponents<HotspotSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::CinematicCamera:
            return GetComponents<CinematicCameraSpaceComponentScriptInterface>(Context, Entity, Schema);
        default:
            break;
        };

        return GetComponents<ComponentScriptInterface>(Context, Entity, Schema);
    }

private:
    template <typename ScriptInterface>
    std::vector<qjs::Value> GetComponents(qjs::Context& Context, EntityScriptInterface& Entity, const ComponentSchema& Schema)
    {
        const auto Proto = GetOrCreate(Schema.TypeId, [&] { return MakeComponentPrototype<ScriptInterface>(Context, Schema); });

        const auto ComponentType = static_cast<csp::multiplayer::ComponentType>(Schema.TypeId);

        auto Wrapped = std::vector<qjs::Value>();

        for (auto* Component : Entity.GetComponentsOfType<ScriptInterface>(ComponentType))
        {
            auto Instance = qjs::js_traits<ScriptInterface*>::wrap(Proto.ctx, Component);
            JS_SetPrototype(Proto.ctx, Instance, Proto.v); // Overwrite the prototype

            Wrapped.push_back({ Proto.ctx, std::move(Instance) });
        }

        return Wrapped;
    }

    template <typename FactoryFn> const qjs::Value& GetOrCreate(ComponentSchema::TypeIdType TypeId, FactoryFn&& Create)
    {
        const auto It = Cache.find(TypeId);
        if (It != Cache.end())
        {
            return It->second;
        }

        return Cache.emplace(TypeId, Create()).first->second;
    }

    std::unordered_map<ComponentSchema::TypeIdType, qjs::Value> Cache;
};

EntityScriptBinding::EntityScriptBinding(csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem)
    : SchemaCache(std::make_unique<SchemaCacheImpl>())
    , EntitySystem(InEntitySystem)
    , LogSystem(LogSystem)
{
}

EntityScriptBinding::~EntityScriptBinding() = default;

EntityScriptBinding* EntityScriptBinding::BindEntitySystem(
    csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& ScriptRunner)
{
    EntityScriptBinding* ScriptBinding = new EntityScriptBinding(InEntitySystem, LogSystem);
    ScriptRunner.RegisterScriptBinding(ScriptBinding);
    return ScriptBinding;
}

void EntityScriptBinding::RemoveBinding(EntityScriptBinding* InEntityBinding, csp::common::IJSScriptRunner& ScriptRunner)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        ScriptRunner.UnregisterScriptBinding(InEntityBinding);
    }
}

#define PROPERTY_GET_SET(COMP, METHOD, PROP) property<&COMP##ScriptInterface::Get##METHOD, &COMP##ScriptInterface::Set##METHOD>(PROP)
#define PROPERTY_GET(COMP, METHOD, PROP) property<&COMP##ScriptInterface::Get##METHOD>(PROP)

void BindComponents(qjs::Context::Module* Module)
{
    Module->class_<VideoPlayerSpaceComponentScriptInterface>("VideoPlayerSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Volume, "volume"); // we can't express value ranges (min, max) in schemas yet, so manually bind

    Module->class_<CinematicCameraSpaceComponentScriptInterface>("CinematicCameraSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CinematicCameraSpaceComponentScriptInterface::GetFov>("getFov");

    Module->class_<CustomSpaceComponentScriptInterface>("CustomSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CustomSpaceComponentScriptInterface::GetCustomPropertySubscriptionKey>("getCustomPropertySubscriptionKey")
        .fun<&CustomSpaceComponentScriptInterface::HasCustomProperty>("hasCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::RemoveCustomProperty>("removeCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomProperty>("getCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomPropertyKeys>("getCustomPropertyKeys")
        .fun<&CustomSpaceComponentScriptInterface::SetCustomProperty>("setCustomProperty");

    Module->class_<SplineSpaceComponentScriptInterface>("SplineSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&SplineSpaceComponentScriptInterface::SetWaypoints>("setWaypoints")
        .fun<&SplineSpaceComponentScriptInterface::GetWaypoints>("getWaypoints")
        .fun<&SplineSpaceComponentScriptInterface::GetLocationAlongSpline>("getLocationAlongSpline");

    Module->class_<AudioSpaceComponentScriptInterface>("AudioSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(AudioSpaceComponent, Volume, "volume"); // we can't express value ranges (min, max) in schemas yet, so manually bind

    Module->class_<HotspotSpaceComponentScriptInterface>("HotspotSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&HotspotSpaceComponentScriptInterface::GetUniqueComponentId>("getUniqueComponentId");
}

void EntityScriptBinding::Bind(int64_t ContextId, csp::common::IJSScriptRunner& ScriptRunner)
{
    qjs::Context* Context = (qjs::Context*)ScriptRunner.GetContext(ContextId);
    qjs::Context::Module* Module = (qjs::Context::Module*)ScriptRunner.GetModule(ContextId, csp::systems::SCRIPT_NAMESPACE);

    Module->function("Log", [&LogSystem = this->LogSystem](qjs::rest<std::string> Args) { EntityScriptLog(std::move(Args), LogSystem); });

    const auto RegisterDynamicComponentGetters = [this, Context, ContextId](auto&& ClassRegistrar) -> decltype(auto)
    {
        for (const auto& Schema : EntitySystem->GetComponentSchemaRegistry()->GetAll())
        {
            if (IsScriptable(Schema))
            {
                const auto& ComponentScriptName = Schema.Name;
                const auto GetterName = fmt::format("get{}Components", ComponentScriptName.c_str());

                ClassRegistrar.fun(GetterName.c_str(),
                    [this, Schema = Schema, Context, ContextId]() -> std::vector<qjs::Value>
                    {
                        if (auto* Entity = EntitySystem->FindSpaceEntityById(ContextId))
                        {
                            return SchemaCache->GetComponents(*Context, *Entity->GetScriptInterface(), Schema);
                        }

                        return {};
                    });
            }
        }

        return std::forward<decltype(ClassRegistrar)>(ClassRegistrar);
    };

    RegisterDynamicComponentGetters(Module->class_<EntityScriptInterface>("Entity"))
        .constructor<>()
        .fun<&EntityScriptInterface::SubscribeToPropertyChange>("subscribeToPropertyChange")
        .fun<&EntityScriptInterface::SubscribeToMessage>("subscribeToMessage")
        .fun<&EntityScriptInterface::PostMessageToScript>("postMessage")
        .fun<&EntityScriptInterface::ClaimScriptOwnership>("claimScriptOwnership")
        .fun<&EntityScriptInterface::GetComponents>("getComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<SplineSpaceComponentScriptInterface, ComponentType::Spline>>("getSplineComponents")
        .fun<&EntityScriptInterface::RemoveParentEntity>("removeParentEntity")
        .property<&EntityScriptInterface::GetPosition, &EntityScriptInterface::SetPosition>("position")
        .property<&EntityScriptInterface::GetGlobalPosition>("globalPosition")
        .property<&EntityScriptInterface::GetRotation, &EntityScriptInterface::SetRotation>("rotation")
        .property<&EntityScriptInterface::GetGlobalRotation>("globalRotation")
        .property<&EntityScriptInterface::GetScale, &EntityScriptInterface::SetScale>("scale")
        .property<&EntityScriptInterface::GetGlobalScale>("globalScale")
        .property<&EntityScriptInterface::GetParentEntity>("parentEntity")
        .property<&EntityScriptInterface::GetId>("id")
        .property<&EntityScriptInterface::GetName>("name")
        .property<&EntityScriptInterface::GetParentId, &EntityScriptInterface::SetParentId>("parentId");

    Module->class_<ComponentScriptInterface>("Component")
        .constructor<>()
        .property<&ComponentScriptInterface::GetComponentId>("id")
        .property<&ComponentScriptInterface::GetComponentType>("type")
        .property<&ComponentScriptInterface::GetComponentName, &ComponentScriptInterface::SetComponentName>("name")
        .fun<&ComponentScriptInterface::SubscribeToPropertyChange>("subscribeToPropertyChange")
        .fun<&ComponentScriptInterface::InvokeAction>("invokeAction");

    BindComponents(Module);

    Module->class_<EntitySystemScriptInterface>("EntitySystem")
        .constructor<>()
        .fun<&EntitySystemScriptInterface::GetFoundationVersion>("getFoundationVersion")
        .fun<&EntitySystemScriptInterface::GetEntities>("getEntities")
        .fun<&EntitySystemScriptInterface::GetObjects>("getObjects")
        .fun<&EntitySystemScriptInterface::GetAvatars>("getAvatars")
        .fun<&EntitySystemScriptInterface::GetEntityById>("getEntityById")
        .fun<&EntitySystemScriptInterface::GetEntityByName>("getEntityByName")
        .fun<&EntitySystemScriptInterface::GetIndexOfEntity>("getIndexOfEntity")
        .fun<&EntitySystemScriptInterface::GetRootHierarchyEntities>("getRootHierarchyEntities");

    Context->global()["TheEntitySystem"] = new EntitySystemScriptInterface(EntitySystem);
    Context->global()["ThisEntity"] = new EntityScriptInterface(EntitySystem->FindSpaceEntityById(ContextId));

    // Always import OKO module into scripts
    std::stringstream ss;
    ss << "import * as " << csp::systems::SCRIPT_NAMESPACE << " from \"" << csp::systems::SCRIPT_NAMESPACE << "\"; globalThis."
       << csp::systems::SCRIPT_NAMESPACE << " = " << csp::systems::SCRIPT_NAMESPACE << ";";
    Context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);

    // For script backwards compatibility
    ss.clear();
    ss << "globalThis." << csp::systems::OLD_SCRIPT_NAMESPACE << " = " << csp::systems::SCRIPT_NAMESPACE;
    Context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
}

} // namespace csp::multiplayer
