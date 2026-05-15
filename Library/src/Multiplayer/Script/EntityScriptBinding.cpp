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

        RAIILock(LockFunc&& lock, UnlockFunc&& unlock)
            : m_lockFunc(std::forward<LockFunc>(lock))
            , m_unlockFunc(std::forward<UnlockFunc>(unlock))
        {
            std::invoke(m_lockFunc);
        }

        ~RAIILock() noexcept { std::invoke(m_unlockFunc); }

        RAIILock(const RAIILock&) = delete;
        RAIILock& operator=(const RAIILock&) = delete;
        RAIILock(RAIILock&&) = delete;
        RAIILock& operator=(RAIILock&&) = delete;

    private:
        LockFuncValueT m_lockFunc;
        UnlockFuncValueT m_unlockFunc;
    };

    // Creates a JS prototype for a specific component type, baking in specialized accessors
    // that map directly to the correctly typed proxy property.
    template <typename ScriptInterfaceType = ComponentScriptInterface>
    qjs::Value MakeComponentPrototype(qjs::Context& context, const ComponentSchema& schema)
    {
        static_assert(std::is_base_of_v<ComponentScriptInterface, ScriptInterfaceType>);

        const auto classId = qjs::js_traits<std::shared_ptr<ScriptInterfaceType>>::QJSClassId;
        const auto basePrototype = qjs::Value { context.ctx, JS_GetClassProto(context.ctx, classId) };
        const auto schemaDerivedPrototype = qjs::Value { context.ctx, JS_NewObjectProto(context.ctx, basePrototype.v) };

        auto createAccessors = [](const auto& v) -> std::optional<std::pair<JSCFunctionMagic*, JSCFunctionMagic*>>
        {
            using T = std::decay_t<decltype(v)>;

            if constexpr (IsScriptableV<T>)
            {
                const auto getter = [](JSContext* context, JSValueConst This, int /*ArgC*/, JSValueConst* /*ArgV*/, int magic) -> JSValue
                {
                    const auto self = qjs::js_traits<std::optional<ScriptInterfaceType*>>::unwrap(context, This);
                    if (!self)
                    {
                        return JS_EXCEPTION;
                    }

                    auto result = (*self)->GetProperty(static_cast<ComponentProperty::KeyType>(magic));
                    if (!result)
                    {
                        return JS_EXCEPTION;
                    }

                    auto* checkedResult = std::get_if<ScriptTypeT<T>>(&*result);
                    if (checkedResult == nullptr)
                    {
                        return JS_EXCEPTION;
                    }

                    return qjs::js_traits<ScriptTypeT<T>>::wrap(context, std::move(*checkedResult));
                };

                const auto setter = [](JSContext* context, JSValueConst This, int argC, JSValueConst* argV, int magic) -> JSValue
                {
                    if (argC <= 0)
                    {
                        return JS_EXCEPTION;
                    }

                    const auto self = qjs::js_traits<std::optional<ScriptInterfaceType*>>::unwrap(context, This);
                    if (!self)
                    {
                        return JS_EXCEPTION;
                    }

                    auto result = qjs::js_traits<std::optional<ScriptTypeT<T>>>::unwrap(context, argV[0]);
                    if (!result)
                    {
                        return JS_EXCEPTION;
                    }

                    (*self)->SetProperty(static_cast<ComponentProperty::KeyType>(magic), std::move(*result));

                    return JS_UNDEFINED;
                };

                return std::make_pair(getter, setter);
            }
            else
            {
                return std::nullopt;
            }
        };

        for (const auto& property : schema.Properties)
        {
            if (!IsScriptable(property))
            {
                continue;
            }

            auto maybeAccessors = std::visit(createAccessors, property.DefaultValue.GetValue());
            if (!maybeAccessors)
            {
                continue;
            }

            const auto [Getter, Setter] = *maybeAccessors;

            const auto magic = static_cast<int>(property.Key);
            const auto propertyName = JS_NewAtom(context.ctx, property.Name.c_str());

            JS_DefinePropertyGetSet(context.ctx, schemaDerivedPrototype.v, propertyName,
                JS_NewCFunctionMagic(context.ctx, Getter, "get", 0, JS_CFUNC_generic_magic, magic),
                JS_NewCFunctionMagic(context.ctx, Setter, "set", 1, JS_CFUNC_generic_magic, magic), JS_PROP_C_W_E);

            JS_FreeAtom(context.ctx, propertyName);
        }

        return schemaDerivedPrototype;
    }
}

class EntitySystemScriptInterface
{
public:
    EntitySystemScriptInterface(csp::common::IRealtimeEngine* inEntitySystem = nullptr)
        : m_entitySystem(inEntitySystem)
    {
    }

    ~EntitySystemScriptInterface() { }

    std::vector<uint64_t> GetEntityIds()
    {
        std::vector<uint64_t> entityIds;

        if (m_entitySystem)
        {
            RAIILock entityLock([&]() { m_entitySystem->LockEntityUpdate(); }, [&]() { m_entitySystem->UnlockEntityUpdate(); });
            for (size_t i = 0; i < m_entitySystem->GetNumEntities(); ++i)
            {
                const SpaceEntity* entity = m_entitySystem->GetEntityByIndex(i);

                uint64_t id = entity->GetId();
                entityIds.push_back(id);
            }
        }

        return entityIds;
    }

    std::vector<EntityScriptInterface*> GetEntities()
    {
        std::vector<EntityScriptInterface*> entities;

        if (m_entitySystem)
        {
            RAIILock entityLock([&]() { m_entitySystem->LockEntityUpdate(); }, [&]() { m_entitySystem->UnlockEntityUpdate(); });
            for (size_t i = 0; i < m_entitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* entity = m_entitySystem->GetEntityByIndex(i);
                entities.push_back(entity->GetScriptInterface());
            }
        }

        return entities;
    }

    std::vector<EntityScriptInterface*> GetObjects()
    {
        std::vector<EntityScriptInterface*> objects;

        if (m_entitySystem)
        {
            for (size_t i = 0; i < m_entitySystem->GetNumObjects(); ++i)
            {
                SpaceEntity* entity = m_entitySystem->GetObjectByIndex(i);
                objects.push_back(entity->GetScriptInterface());
            }
        }

        return objects;
    }

    std::vector<EntityScriptInterface*> GetAvatars()
    {
        std::vector<EntityScriptInterface*> avatars;

        if (m_entitySystem)
        {
            for (size_t i = 0; i < m_entitySystem->GetNumAvatars(); ++i)
            {
                SpaceEntity* entity = m_entitySystem->GetAvatarByIndex(i);
                avatars.push_back(entity->GetScriptInterface());
            }
        }

        return avatars;
    }

    int32_t GetIndexOfEntity(int64_t entityId)
    {
        int32_t indexOfEntity = -1;

        if (m_entitySystem)
        {
            RAIILock entityLock([&]() { m_entitySystem->LockEntityUpdate(); }, [&]() { m_entitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < m_entitySystem->GetNumEntities(); ++i)
            {
                const SpaceEntity* entity = m_entitySystem->GetEntityByIndex(i);

                if (entity->GetId() == static_cast<uint64_t>(entityId))
                {
                    indexOfEntity = static_cast<int32_t>(i);
                    break;
                }
            }
        }

        return indexOfEntity;
    }

    EntityScriptInterface* GetEntityById(int64_t entityId)
    {
        EntityScriptInterface* scriptInterface = nullptr;
        if (m_entitySystem)
        {
            RAIILock entityLock([&]() { m_entitySystem->LockEntityUpdate(); }, [&]() { m_entitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < m_entitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* entity = m_entitySystem->GetEntityByIndex(i);
                if (entity->GetId() == static_cast<uint64_t>(entityId))
                {
                    scriptInterface = entity->GetScriptInterface();
                    break;
                }
            }
        }

        return scriptInterface;
    }

    EntityScriptInterface* GetEntityByName(std::string entityName)
    {
        EntityScriptInterface* scriptInterface = nullptr;
        if (m_entitySystem)
        {
            RAIILock entityLock([&]() { m_entitySystem->LockEntityUpdate(); }, [&]() { m_entitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < m_entitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* entity = m_entitySystem->GetEntityByIndex(i);
                if (entity->GetName() == entityName.c_str())
                {
                    scriptInterface = entity->GetScriptInterface();
                    break;
                }
            }
        }

        return scriptInterface;
    }

    std::vector<EntityScriptInterface*> GetRootHierarchyEntities()
    {
        std::vector<EntityScriptInterface*> rootHierarchyEntities;
        if (m_entitySystem)
        {
            RAIILock entityLock([&]() { m_entitySystem->LockEntityUpdate(); }, [&]() { m_entitySystem->UnlockEntityUpdate(); });

            for (size_t i = 0; i < m_entitySystem->GetRootHierarchyEntities()->Size(); ++i)
            {
                SpaceEntity* entity = (*m_entitySystem->GetRootHierarchyEntities())[i];
                rootHierarchyEntities.push_back(entity->GetScriptInterface());
            }
        }

        return rootHierarchyEntities;
    }

    std::string GetFoundationVersion() { return csp::CSPFoundation::GetVersion().c_str(); }

private:
    csp::common::IRealtimeEngine* m_entitySystem;
};

void EntityScriptLog(qjs::rest<std::string> args, csp::common::LogSystem& logSystem)
{
    std::stringstream str;

    for (auto const& arg : args)
    {
        str << arg << " ";
    }

    logSystem.LogMsg(csp::common::LogLevel::Log, str.str().c_str());
}

class EntityScriptBinding::SchemaCacheImpl
{
public:
    std::vector<qjs::Value> GetComponents(qjs::Context& context, EntityScriptInterface& entity, const ComponentSchema& schema)
    {
        switch (static_cast<ComponentType>(schema.TypeId))
        {
        case ComponentType::VideoPlayer:
            return GetComponents<VideoPlayerSpaceComponentScriptInterface>(context, entity, schema);
        case ComponentType::Custom:
            return GetComponents<CustomSpaceComponentScriptInterface>(context, entity, schema);
        case ComponentType::Audio:
            return GetComponents<AudioSpaceComponentScriptInterface>(context, entity, schema);
        case ComponentType::Hotspot:
            return GetComponents<HotspotSpaceComponentScriptInterface>(context, entity, schema);
        case ComponentType::CinematicCamera:
            return GetComponents<CinematicCameraSpaceComponentScriptInterface>(context, entity, schema);
        default:
            break;
        };

        return GetComponents<ComponentScriptInterface>(context, entity, schema);
    }

private:
    template <typename ScriptInterface>
    std::vector<qjs::Value> GetComponents(qjs::Context& context, EntityScriptInterface& entity, const ComponentSchema& schema)
    {
        const auto proto = GetOrCreate(schema.TypeId, [&] { return MakeComponentPrototype<ScriptInterface>(context, schema); });

        const auto componentType = static_cast<csp::multiplayer::ComponentType>(schema.TypeId);

        auto wrapped = std::vector<qjs::Value>();

        for (auto* component : entity.GetComponentsOfType<ScriptInterface>(componentType))
        {
            auto instance = qjs::js_traits<ScriptInterface*>::wrap(proto.ctx, component);
            JS_SetPrototype(proto.ctx, instance, proto.v); // Overwrite the prototype

            wrapped.push_back({ proto.ctx, std::move(instance) });
        }

        return wrapped;
    }

    template <typename FactoryFn> const qjs::Value& GetOrCreate(ComponentSchema::TypeIdType typeId, FactoryFn&& create)
    {
        const auto it = m_cache.find(typeId);
        if (it != m_cache.end())
        {
            return it->second;
        }

        return m_cache.emplace(typeId, create()).first->second;
    }

    std::unordered_map<ComponentSchema::TypeIdType, qjs::Value> m_cache;
};

EntityScriptBinding::EntityScriptBinding(csp::common::IRealtimeEngine* inEntitySystem, csp::common::LogSystem& logSystem)
    : m_schemaCache(std::make_unique<SchemaCacheImpl>())
    , m_entitySystem(inEntitySystem)
    , m_logSystem(logSystem)
{
}

EntityScriptBinding::~EntityScriptBinding() = default;

EntityScriptBinding* EntityScriptBinding::BindEntitySystem(
    csp::common::IRealtimeEngine* inEntitySystem, csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& scriptRunner)
{
    EntityScriptBinding* scriptBinding = new EntityScriptBinding(inEntitySystem, logSystem);
    scriptRunner.RegisterScriptBinding(scriptBinding);
    return scriptBinding;
}

void EntityScriptBinding::RemoveBinding(EntityScriptBinding* inEntityBinding, csp::common::IJSScriptRunner& scriptRunner)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        scriptRunner.UnregisterScriptBinding(inEntityBinding);
    }
}

#define PROPERTY_GET_SET(COMP, METHOD, PROP) property<&COMP##ScriptInterface::Get##METHOD, &COMP##ScriptInterface::Set##METHOD>(PROP)
#define PROPERTY_GET(COMP, METHOD, PROP) property<&COMP##ScriptInterface::Get##METHOD>(PROP)

void BindComponents(qjs::Context::Module* module)
{
    module->class_<VideoPlayerSpaceComponentScriptInterface>("VideoPlayerSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Volume, "volume"); // we can't express value ranges (min, max) in schemas yet, so manually bind

    module->class_<CinematicCameraSpaceComponentScriptInterface>("CinematicCameraSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CinematicCameraSpaceComponentScriptInterface::GetFov>("getFov");

    module->class_<CustomSpaceComponentScriptInterface>("CustomSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CustomSpaceComponentScriptInterface::GetCustomPropertySubscriptionKey>("getCustomPropertySubscriptionKey")
        .fun<&CustomSpaceComponentScriptInterface::HasCustomProperty>("hasCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::RemoveCustomProperty>("removeCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomProperty>("getCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomPropertyKeys>("getCustomPropertyKeys")
        .fun<&CustomSpaceComponentScriptInterface::SetCustomProperty>("setCustomProperty");

    module->class_<SplineSpaceComponentScriptInterface>("SplineSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&SplineSpaceComponentScriptInterface::SetWaypoints>("setWaypoints")
        .fun<&SplineSpaceComponentScriptInterface::GetWaypoints>("getWaypoints")
        .fun<&SplineSpaceComponentScriptInterface::GetLocationAlongSpline>("getLocationAlongSpline");

    module->class_<AudioSpaceComponentScriptInterface>("AudioSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(AudioSpaceComponent, Volume, "volume"); // we can't express value ranges (min, max) in schemas yet, so manually bind

    module->class_<HotspotSpaceComponentScriptInterface>("HotspotSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&HotspotSpaceComponentScriptInterface::GetUniqueComponentId>("getUniqueComponentId");
}

void EntityScriptBinding::Bind(int64_t contextId, csp::common::IJSScriptRunner& scriptRunner)
{
    qjs::Context* context = (qjs::Context*)scriptRunner.GetContext(contextId);
    qjs::Context::Module* module = (qjs::Context::Module*)scriptRunner.GetModule(contextId, csp::systems::SCRIPT_NAMESPACE);

    module->function("Log", [&logSystem = this->m_logSystem](qjs::rest<std::string> args) { EntityScriptLog(std::move(args), logSystem); });

    const auto registerDynamicComponentGetters = [this, context, contextId](auto&& classRegistrar) -> decltype(auto)
    {
        for (const auto& [TypeId, Schema] : m_entitySystem->GetComponentSchemaRegistry()->GetUnderlying())
        {
            if (IsScriptable(Schema))
            {
                const auto& componentScriptName = Schema.Name;
                const auto getterName = fmt::format("get{}Components", componentScriptName.c_str());

                classRegistrar.fun(getterName.c_str(),
                    [this, schema = Schema, context, contextId]() -> std::vector<qjs::Value>
                    {
                        if (auto* entity = m_entitySystem->FindSpaceEntityById(contextId))
                        {
                            return m_schemaCache->GetComponents(*context, *entity->GetScriptInterface(), schema);
                        }

                        return {};
                    });
            }
        }

        return std::forward<decltype(classRegistrar)>(classRegistrar);
    };

    registerDynamicComponentGetters(module->class_<EntityScriptInterface>("Entity"))
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

    module->class_<ComponentScriptInterface>("Component")
        .constructor<>()
        .property<&ComponentScriptInterface::GetComponentId>("id")
        .property<&ComponentScriptInterface::GetComponentType>("type")
        .property<&ComponentScriptInterface::GetComponentName, &ComponentScriptInterface::SetComponentName>("name")
        .fun<&ComponentScriptInterface::SubscribeToPropertyChange>("subscribeToPropertyChange")
        .fun<&ComponentScriptInterface::InvokeAction>("invokeAction");

    BindComponents(module);

    module->class_<EntitySystemScriptInterface>("EntitySystem")
        .constructor<>()
        .fun<&EntitySystemScriptInterface::GetFoundationVersion>("getFoundationVersion")
        .fun<&EntitySystemScriptInterface::GetEntities>("getEntities")
        .fun<&EntitySystemScriptInterface::GetObjects>("getObjects")
        .fun<&EntitySystemScriptInterface::GetAvatars>("getAvatars")
        .fun<&EntitySystemScriptInterface::GetEntityById>("getEntityById")
        .fun<&EntitySystemScriptInterface::GetEntityByName>("getEntityByName")
        .fun<&EntitySystemScriptInterface::GetIndexOfEntity>("getIndexOfEntity")
        .fun<&EntitySystemScriptInterface::GetRootHierarchyEntities>("getRootHierarchyEntities");

    context->global()["TheEntitySystem"] = new EntitySystemScriptInterface(m_entitySystem);
    context->global()["ThisEntity"] = new EntityScriptInterface(m_entitySystem->FindSpaceEntityById(contextId));

    // Always import OKO module into scripts
    std::stringstream ss;
    ss << "import * as " << csp::systems::SCRIPT_NAMESPACE << " from \"" << csp::systems::SCRIPT_NAMESPACE << "\"; globalThis."
       << csp::systems::SCRIPT_NAMESPACE << " = " << csp::systems::SCRIPT_NAMESPACE << ";";
    context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);

    // For script backwards compatibility
    ss.clear();
    ss << "globalThis." << csp::systems::OLD_SCRIPT_NAMESPACE << " = " << csp::systems::SCRIPT_NAMESPACE;
    context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
}

} // namespace csp::multiplayer
