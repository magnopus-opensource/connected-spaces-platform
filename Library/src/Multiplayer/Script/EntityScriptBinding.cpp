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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/ComponentSchemaRegistry.h"
#include "Multiplayer/EntityQueryUtils.h"
#include "Multiplayer/Script/ComponentBinding/AnimatedModelSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CollisionSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentScriptHelpers.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"
#include "Multiplayer/Script/EntityScriptInterface.h"
#include "Multiplayer/Script/RuntimeMaterialScriptInterface.h"
#include "Multiplayer/Script/RuntimeMaterialTextureScriptInterface.h"
#include "ScriptHelpers.h"
#include "quickjspp.hpp"

#include <fmt/format.h>

#include <cstring>
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

    template <typename T> qjs::Value GetClassPrototype(qjs::Context& Context)
    {
        const auto ClassId = qjs::js_traits<std::shared_ptr<T>>::QJSClassId;
        return qjs::Value { Context.ctx, JS_GetClassProto(Context.ctx, ClassId) };
    }

    // Creates a JS prototype for a specific component type, baking in specialized accessors
    // that map directly to the correctly typed proxy property.
    template <typename ScriptInterfaceType = ComponentScriptInterface>
    qjs::Value MakeComponentPrototype(qjs::Context& Context, const ComponentSchema& Schema)
    {
        static_assert(std::is_base_of_v<ComponentScriptInterface, ScriptInterfaceType>);

        const auto BasePrototype = GetClassPrototype<ScriptInterfaceType>(Context);
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

    virtual ~EntitySystemScriptInterface() = default;

    virtual EntityScriptInterface* WrapEntity(SpaceEntity* Entity) { return Entity->GetScriptInterface(); }

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
                Entities.push_back(WrapEntity(Entity));
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
                Objects.push_back(WrapEntity(Entity));
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
                Avatars.push_back(WrapEntity(Entity));
            }
        }

        return Avatars;
    }

    EntityScriptInterface* GetLocalAvatar()
    {
        if (EntitySystem == nullptr)
        {
            return nullptr;
        }

        const uint64_t LocalClientId = ResolveLocalClientId(EntitySystem);

        for (size_t i = 0; i < EntitySystem->GetNumAvatars(); ++i)
        {
            SpaceEntity* Entity = EntitySystem->GetAvatarByIndex(i);
            if ((Entity != nullptr) && (Entity->GetOwnerId() == LocalClientId))
            {
                return WrapEntity(Entity);
            }
        }

        return nullptr;
    }

    static uint64_t ResolveLocalClientId(csp::common::IRealtimeEngine* Engine)
    {
        if ((Engine != nullptr) && (Engine->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online))
        {
            const auto* OnlineEngine = static_cast<const csp::multiplayer::OnlineRealtimeEngine*>(Engine);
            if (OnlineEngine != nullptr)
            {
                if (auto* Connection = OnlineEngine->GetMultiplayerConnectionInstance(); Connection != nullptr)
                {
                    return Connection->GetClientId();
                }
            }
        }

        return csp::multiplayer::OfflineRealtimeEngine::LocalClientId();
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
                    ScriptInterface = WrapEntity(Entity);
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
                    ScriptInterface = WrapEntity(Entity);
                    break;
                }
            }
        }

        return ScriptInterface;
    }

    std::vector<EntityScriptInterface*> GetEntitiesByQuery(qjs::Value Query)
    {
        std::vector<EntityScriptInterface*> Entities;
        if (!EntitySystem || !Query.ctx)
        {
            return Entities;
        }

        const std::string QueryJson = Query.toJSON();

        RAIILock EntityLock([&]() { EntitySystem->LockEntityUpdate(); }, [&]() { EntitySystem->UnlockEntityUpdate(); });

        std::vector<SpaceEntity*> AllEntities;
        AllEntities.reserve(EntitySystem->GetNumEntities());
        for (size_t Index = 0; Index < EntitySystem->GetNumEntities(); ++Index)
        {
            if (auto* Entity = EntitySystem->GetEntityByIndex(Index))
            {
                AllEntities.push_back(Entity);
            }
        }

        const auto MatchingIds = ResolveEntityIdsFromQueryJson(QueryJson, AllEntities);
        for (auto* Entity : AllEntities)
        {
            if ((Entity != nullptr) && (MatchingIds.find(Entity->GetId()) != MatchingIds.end()))
            {
                Entities.push_back(WrapEntity(Entity));
            }
        }

        return Entities;
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
                RootHierarchyEntities.push_back(WrapEntity(Entity));
            }
        }

        return RootHierarchyEntities;
    }

    std::string GetFoundationVersion() { return csp::CSPFoundation::GetVersion().c_str(); }

private:
    csp::common::IRealtimeEngine* EntitySystem;
};

class LocalEntitySystemScriptInterface : public EntitySystemScriptInterface
{
public:
    LocalEntitySystemScriptInterface(csp::common::IRealtimeEngine* InEntitySystem)
        : EntitySystemScriptInterface(InEntitySystem)
    {
    }

    ~LocalEntitySystemScriptInterface() override
    {
        for (auto& [Id, Iface] : EntityCache)
        {
            delete Iface;
        }
    }

    EntityScriptInterface* WrapEntity(SpaceEntity* Entity) override
    {
        const uint64_t Id = Entity->GetId();
        auto It = EntityCache.find(Id);
        if (It != EntityCache.end())
        {
            return It->second;
        }
        auto* Iface = new EntityScriptInterface(Entity, true);
        EntityCache[Id] = Iface;
        return Iface;
    }

private:
    std::unordered_map<uint64_t, EntityScriptInterface*> EntityCache;
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
        switch (ToComponentType(Schema.TypeId).value_or(ComponentType::Invalid))
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
        case ComponentType::StaticModel:
            return GetComponents<StaticModelSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::AnimatedModel:
            return GetComponents<AnimatedModelSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Collision:
            return GetComponents<CollisionSpaceComponentScriptInterface>(Context, Entity, Schema);
        default:
            break;
        };

        return GetComponents<ComponentScriptInterface>(Context, Entity, Schema);
    }

    // Adds a component of the schema's type to the entity and returns a single wrapped instance
    // using the same schema-derived prototype as GetComponents.
    qjs::Value AddComponent(qjs::Context& Context, EntityScriptInterface& Entity, const ComponentSchema& Schema)
    {
        switch (ToComponentType(Schema.TypeId).value_or(ComponentType::Invalid))
        {
        case ComponentType::VideoPlayer:
            return AddComponent<VideoPlayerSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Custom:
            return AddComponent<CustomSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Audio:
            return AddComponent<AudioSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Hotspot:
            return AddComponent<HotspotSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::CinematicCamera:
            return AddComponent<CinematicCameraSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::StaticModel:
            return AddComponent<StaticModelSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::AnimatedModel:
            return AddComponent<AnimatedModelSpaceComponentScriptInterface>(Context, Entity, Schema);
        case ComponentType::Collision:
            return AddComponent<CollisionSpaceComponentScriptInterface>(Context, Entity, Schema);
        default:
            break;
        };

        return AddComponent<ComponentScriptInterface>(Context, Entity, Schema);
    }

private:
    template <typename ScriptInterface>
    qjs::Value AddComponent(qjs::Context& Context, EntityScriptInterface& Entity, const ComponentSchema& Schema)
    {
        auto* Iface = Entity.AddComponentForSchema(Schema.TypeId);
        if (Iface == nullptr)
        {
            return qjs::Value { Context.ctx, JS_NULL };
        }

        const auto Proto = GetOrCreate(Schema.TypeId, [&] { return MakeComponentPrototype<ScriptInterface>(Context, Schema); });

        auto Instance = qjs::js_traits<ScriptInterface*>::wrap(Proto.ctx, static_cast<ScriptInterface*>(Iface));
        JS_SetPrototype(Proto.ctx, Instance, Proto.v); // Overwrite the prototype

        return qjs::Value { Proto.ctx, std::move(Instance) };
    }

    template <typename ScriptInterface>
    std::vector<qjs::Value> GetComponents(qjs::Context& Context, EntityScriptInterface& Entity, const ComponentSchema& Schema)
    {
        const auto Proto = GetOrCreate(Schema.TypeId, [&] { return MakeComponentPrototype<ScriptInterface>(Context, Schema); });

        auto Wrapped = std::vector<qjs::Value>();

        for (auto* Component : Entity.GetComponentsOfType<ScriptInterface>(Schema.TypeId))
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

EntityScriptBinding::EntityScriptBinding(csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem, bool InLocalScope)
    : SchemaCache(std::make_unique<SchemaCacheImpl>())
    , EntitySystem(InEntitySystem)
    , LogSystem(LogSystem)
    , LocalScope(InLocalScope)
{
}

EntityScriptBinding::~EntityScriptBinding() = default;

EntityScriptBinding* EntityScriptBinding::BindEntitySystem(
    csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& ScriptRunner, bool LocalScope)
{
    EntityScriptBinding* ScriptBinding = new EntityScriptBinding(InEntitySystem, LogSystem, LocalScope);
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


    Module->class_<RuntimeMaterialTextureScriptInterface>("RuntimeMaterialTexture")
        .property<&RuntimeMaterialTextureScriptInterface::GetIsSet>("isSet")
        .property<&RuntimeMaterialTextureScriptInterface::GetSourceType>("sourceType")
        .property<&RuntimeMaterialTextureScriptInterface::GetAssetCollectionId>("assetCollectionId")
        .property<&RuntimeMaterialTextureScriptInterface::GetAssetId>("assetId")
        .property<&RuntimeMaterialTextureScriptInterface::GetEntityComponentId>("entityComponentId")
        .property<&RuntimeMaterialTextureScriptInterface::GetUVOffset, &RuntimeMaterialTextureScriptInterface::SetUVOffset>("uvOffset")
        .property<&RuntimeMaterialTextureScriptInterface::GetUVScale, &RuntimeMaterialTextureScriptInterface::SetUVScale>("uvScale")
        .property<&RuntimeMaterialTextureScriptInterface::GetUVRotation, &RuntimeMaterialTextureScriptInterface::SetUVRotation>("uvRotation")
        .property<&RuntimeMaterialTextureScriptInterface::GetTexCoord, &RuntimeMaterialTextureScriptInterface::SetTexCoord>("texCoord")
        .property<&RuntimeMaterialTextureScriptInterface::GetStereoVideoType, &RuntimeMaterialTextureScriptInterface::SetStereoVideoType>(
            "stereoVideoType")
        .property<&RuntimeMaterialTextureScriptInterface::GetIsStereoFlipped, &RuntimeMaterialTextureScriptInterface::SetIsStereoFlipped>(
            "isStereoFlipped")
        .fun<&RuntimeMaterialTextureScriptInterface::SetUVOffsetXY>("__setUVOffsetXY")
        .fun<&RuntimeMaterialTextureScriptInterface::SetUVScaleXY>("__setUVScaleXY")
        .fun<&RuntimeMaterialTextureScriptInterface::SetAssetSource>("setAssetSource")
        .fun<&RuntimeMaterialTextureScriptInterface::SetComponentSource>("setComponentSource")
        .fun<&RuntimeMaterialTextureScriptInterface::Clear>("clear");

    Module->class_<RuntimeMaterialScriptInterface>("RuntimeMaterial")
        .property<&RuntimeMaterialScriptInterface::GetStatus>("status")
        .property<&RuntimeMaterialScriptInterface::GetMaterialId>("materialId")
        .property<&RuntimeMaterialScriptInterface::GetMaterialPath>("materialPath")
        .property<&RuntimeMaterialScriptInterface::GetShaderType>("shaderType")
        .property<&RuntimeMaterialScriptInterface::GetName>("name")
        .property<&RuntimeMaterialScriptInterface::GetBaseColorTexture>("baseColorTexture")
        .property<&RuntimeMaterialScriptInterface::GetMetallicRoughnessTexture>("metallicRoughnessTexture")
        .property<&RuntimeMaterialScriptInterface::GetNormalTexture>("normalTexture")
        .property<&RuntimeMaterialScriptInterface::GetOcclusionTexture>("occlusionTexture")
        .property<&RuntimeMaterialScriptInterface::GetEmissiveTexture>("emissiveTexture")
        .property<&RuntimeMaterialScriptInterface::GetColorTexture>("colorTexture")
        .property<&RuntimeMaterialScriptInterface::GetBaseColorFactor, &RuntimeMaterialScriptInterface::SetBaseColorFactor>("baseColorFactor")
        .property<&RuntimeMaterialScriptInterface::GetMetallicFactor, &RuntimeMaterialScriptInterface::SetMetallicFactor>("metallicFactor")
        .property<&RuntimeMaterialScriptInterface::GetRoughnessFactor, &RuntimeMaterialScriptInterface::SetRoughnessFactor>("roughnessFactor")
        .property<&RuntimeMaterialScriptInterface::GetEmissiveFactor, &RuntimeMaterialScriptInterface::SetEmissiveFactor>("emissiveFactor")
        .property<&RuntimeMaterialScriptInterface::GetEmissiveStrength, &RuntimeMaterialScriptInterface::SetEmissiveStrength>("emissiveStrength")
        .property<&RuntimeMaterialScriptInterface::GetAlphaCutoff, &RuntimeMaterialScriptInterface::SetAlphaCutoff>("alphaCutoff")
        .property<&RuntimeMaterialScriptInterface::GetTint, &RuntimeMaterialScriptInterface::SetTint>("tint")
        .property<&RuntimeMaterialScriptInterface::GetAlphaFactor, &RuntimeMaterialScriptInterface::SetAlphaFactor>("alphaFactor")
        .property<&RuntimeMaterialScriptInterface::GetEmissiveIntensity, &RuntimeMaterialScriptInterface::SetEmissiveIntensity>("emissiveIntensity")
        .property<&RuntimeMaterialScriptInterface::GetFresnelFactor, &RuntimeMaterialScriptInterface::SetFresnelFactor>("fresnelFactor")
        .property<&RuntimeMaterialScriptInterface::GetAlphaMask, &RuntimeMaterialScriptInterface::SetAlphaMask>("alphaMask")
        .property<&RuntimeMaterialScriptInterface::GetDoubleSided, &RuntimeMaterialScriptInterface::SetDoubleSided>("doubleSided")
        .property<&RuntimeMaterialScriptInterface::GetIsEmissive, &RuntimeMaterialScriptInterface::SetIsEmissive>("isEmissive")
        .property<&RuntimeMaterialScriptInterface::GetBlendMode, &RuntimeMaterialScriptInterface::SetBlendMode>("blendMode")
        .property<&RuntimeMaterialScriptInterface::GetReadAlphaFromChannel, &RuntimeMaterialScriptInterface::SetReadAlphaFromChannel>(
            "readAlphaFromChannel")
        .fun<&RuntimeMaterialScriptInterface::Reset>("reset")
        .fun<&RuntimeMaterialScriptInterface::Save>("save");

    Module->class_<AnimatedModelSpaceComponentScriptInterface>("AnimatedModelSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&AnimatedModelSpaceComponentScriptInterface::GetMaterialPaths>("__getMaterialPaths")
        .fun<&AnimatedModelSpaceComponentScriptInterface::GetMaterial>("__getMaterial");


    Module->class_<VideoPlayerSpaceComponentScriptInterface>("VideoPlayerSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Volume, "volume"); // we can't express value ranges (min, max) in schemas yet, so manually bind




    Module->class_<CinematicCameraSpaceComponentScriptInterface>("CinematicCameraSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CinematicCameraSpaceComponentScriptInterface::GetFov>("getFov");

    Module->class_<CollisionSpaceComponentScriptInterface>("CollisionSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CollisionSpaceComponentScriptInterface::SetKinematicPose>("setKinematicPose")
        .fun<&CollisionSpaceComponentScriptInterface::SetKinematicPosition>("setKinematicPosition")
        .fun<&CollisionSpaceComponentScriptInterface::SetKinematicRotation>("setKinematicRotation")
        .fun<&CollisionSpaceComponentScriptInterface::ResetKinematicPose>("resetKinematicPose")
        .fun<&CollisionSpaceComponentScriptInterface::ApplyImpulse>("applyImpulse")
        .fun<&CollisionSpaceComponentScriptInterface::ApplyTorqueImpulse>("applyTorqueImpulse")
        .fun<&CollisionSpaceComponentScriptInterface::SetLinearVelocity>("setLinearVelocity")
        .fun<&CollisionSpaceComponentScriptInterface::SetAngularVelocity>("setAngularVelocity");



    Module->class_<StaticModelSpaceComponentScriptInterface>("StaticModelSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&StaticModelSpaceComponentScriptInterface::GetMaterialPaths>("__getMaterialPaths")
        .fun<&StaticModelSpaceComponentScriptInterface::GetMaterial>("__getMaterial");


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
        .fun<&AudioSpaceComponentScriptInterface::PlaySound>("playSound")
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

    const auto RegisterDynamicComponentGetters = [this, Context](auto Proto)
    {
        for (const auto& Schema : EntitySystem->GetComponentSchemaRegistry()->GetAll())
        {
            if (IsScriptable(Schema))
            {
                const auto& ComponentScriptName = Schema.Name;
                const auto GetterName = fmt::format("get{}Components", ComponentScriptName.c_str());

                const auto GetterImpl
                    = [this, Schema, Context](EntityScriptInterface* Entity) { return SchemaCache->GetComponents(*Context, *Entity, Schema); };

                const auto Getter
                    = [](JSContext* Context, JSValueConst This, int /*ArgC*/, JSValueConst* /*ArgV*/, int /*Magic*/, JSValue* FnData) -> JSValue
                {
                    const auto GetterImpl = FnData[0];
                    return JS_Call(Context, GetterImpl, JS_UNDEFINED, 1, &This);
                };

                auto FnData = Context->newValue(GetterImpl);

                JS_SetPropertyStr(Context->ctx, Proto.v, GetterName.c_str(), JS_NewCFunctionData(Context->ctx, Getter, 0, 0, 1, &FnData.v));
            }
        }
    };

    // Registers an add<Name>Component function per scriptable schema, mirroring the dynamic getters.
    // The returned instance is wrapped with the same schema-derived prototype as get<Name>Components.
    const auto RegisterDynamicComponentAdders = [this, Context](auto Proto)
    {
        for (const auto& Schema : EntitySystem->GetComponentSchemaRegistry()->GetAll())
        {
            if (!IsScriptable(Schema))
            {
                continue;
            }

            const auto RegisterAdder = [this, Context, &Proto](const ComponentSchema& Schema, const std::string& AdderName)
            {
                const auto AdderImpl
                    = [this, Schema, Context](EntityScriptInterface* Entity) { return SchemaCache->AddComponent(*Context, *Entity, Schema); };

                const auto Adder
                    = [](JSContext* Context, JSValueConst This, int /*ArgC*/, JSValueConst* /*ArgV*/, int /*Magic*/, JSValue* FnData) -> JSValue
                {
                    const auto AdderImpl = FnData[0];
                    return JS_Call(Context, AdderImpl, JS_UNDEFINED, 1, &This);
                };

                auto FnData = Context->newValue(AdderImpl);

                JS_SetPropertyStr(Context->ctx, Proto.v, AdderName.c_str(), JS_NewCFunctionData(Context->ctx, Adder, 0, 0, 1, &FnData.v));
            };

            RegisterAdder(Schema, fmt::format("add{}Component", Schema.Name.c_str()));

            // Legacy alias: scripts historically used addVideoComponent for the VideoPlayer component.
            if (std::strcmp(Schema.Name.c_str(), "VideoPlayer") == 0)
            {
                RegisterAdder(Schema, "addVideoComponent");
            }
        }
    };

    Module->class_<EntityScriptInterface>("Entity")
        .constructor<>()
        .fun<&EntityScriptInterface::SubscribeToPropertyChange>("subscribeToPropertyChange")
        .fun<&EntityScriptInterface::SubscribeToMessage>("subscribeToMessage")
        .fun<&EntityScriptInterface::PostMessageToScript>("postMessage")
        .fun<&EntityScriptInterface::ClaimScriptOwnership>("claimScriptOwnership")
        .fun<&EntityScriptInterface::On>("on")
        .fun<&EntityScriptInterface::Off>("off")
        .fun<&EntityScriptInterface::Fire>("fire")
        .fun<&EntityScriptInterface::GetComponents>("getComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<SplineSpaceComponentScriptInterface, ComponentType::Spline>>("getSplineComponents")
        .fun<&EntityScriptInterface::RemoveParentEntity>("removeParentEntity")
        .fun<&EntityScriptInterface::GetChildEntitiesByQuery>("getChildEntitiesByQuery")
        .property<&EntityScriptInterface::GetPosition, &EntityScriptInterface::SetPosition>("position")
        .property<&EntityScriptInterface::GetGlobalPosition>("globalPosition")
        .property<&EntityScriptInterface::GetRotation, &EntityScriptInterface::SetRotation>("rotation")
        .property<&EntityScriptInterface::GetGlobalRotation>("globalRotation")
        .property<&EntityScriptInterface::GetScale, &EntityScriptInterface::SetScale>("scale")
        .property<&EntityScriptInterface::GetGlobalScale>("globalScale")
        .property<&EntityScriptInterface::GetParentEntity>("parentEntity")
        .property<&EntityScriptInterface::GetId>("id")
        .property<&EntityScriptInterface::IsLocal, &EntityScriptInterface::SetLocal>("isLocal")
        .property<&EntityScriptInterface::GetName>("name")
        .property<&EntityScriptInterface::GetParentId, &EntityScriptInterface::SetParentId>("parentId")
        .fun<&EntityScriptInterface::GetTags>("getTags")
        .fun<&EntityScriptInterface::SetTags>("setTags")
        .fun<&EntityScriptInterface::HasTag>("hasTag")
        .fun<&EntityScriptInterface::AddTag>("addTag")
        .fun<&EntityScriptInterface::RemoveTag>("removeTag");

    RegisterDynamicComponentGetters(GetClassPrototype<EntityScriptInterface>(*Context));
    RegisterDynamicComponentAdders(GetClassPrototype<EntityScriptInterface>(*Context));

    Module->class_<ComponentScriptInterface>("Component")
        .constructor<>()
        .property<&ComponentScriptInterface::GetComponentId>("id")
        .property<&ComponentScriptInterface::GetComponentType>("type")
        .property<&ComponentScriptInterface::GetComponentName, &ComponentScriptInterface::SetComponentName>("name")
        .fun<&ComponentScriptInterface::Destroy>("destroy")
        .fun<&ComponentScriptInterface::SubscribeToPropertyChange>("subscribeToPropertyChange")
        .fun<&ComponentScriptInterface::InvokeAction>("invokeAction");

    BindComponents(Module);

    Module->class_<EntitySystemScriptInterface>("EntitySystem")
        .constructor<>()
        .fun<&EntitySystemScriptInterface::GetFoundationVersion>("getFoundationVersion")
        .fun<&EntitySystemScriptInterface::GetEntities>("getEntities")
        .fun<&EntitySystemScriptInterface::GetObjects>("getObjects")
        .fun<&EntitySystemScriptInterface::GetAvatars>("getAvatars")
        .fun<&EntitySystemScriptInterface::GetLocalAvatar>("getLocalAvatar")
        .fun<&EntitySystemScriptInterface::GetEntityById>("getEntityById")
        .fun<&EntitySystemScriptInterface::GetEntityByName>("getEntityByName")
        .fun<&EntitySystemScriptInterface::GetEntitiesByQuery>("getEntitiesByQuery")
        .fun<&EntitySystemScriptInterface::GetIndexOfEntity>("getIndexOfEntity")
        .fun<&EntitySystemScriptInterface::GetRootHierarchyEntities>("getRootHierarchyEntities");

    SpaceEntity* ThisEntityPtr = EntitySystem->FindSpaceEntityById(ContextId);

    if (LocalScope)
    {
        // Expose local-scoped wrapper via registered base type so quickjspp can wrap it.
        Context->global()["TheEntitySystem"] = static_cast<EntitySystemScriptInterface*>(new LocalEntitySystemScriptInterface(EntitySystem));
        Context->global()["ThisEntity"] = new EntityScriptInterface(ThisEntityPtr, true);
    }
    else
    {
        Context->global()["TheEntitySystem"] = new EntitySystemScriptInterface(EntitySystem);
        Context->global()["ThisEntity"] = new EntityScriptInterface(ThisEntityPtr);
    }

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
