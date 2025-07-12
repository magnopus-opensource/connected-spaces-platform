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
#include "CSP/Common/List.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "JSBindings.h"
#include "Multiplayer/Script/ComponentBinding/AnimatedModelSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/AvatarSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/ButtonSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CodeSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/ECommerceSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/ExternalLinkSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/FiducialMarkerSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/FogSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/GaussianSplatSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/ImageSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/LightSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/PortalSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/TextSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"
#include "Multiplayer/Script/EntityScriptInterface.h"
#include "ScriptHelpers.h"
#include "quickjspp.hpp"

namespace csp::multiplayer
{

// ConsoleInterface class to mimic JavaScript's console object
class ConsoleInterface
{
public:
    ConsoleInterface() { }

    void Log(qjs::rest<std::string> args)
    {
        std::stringstream ss;
        joinArgs(ss, args);
        CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "%s", ss.str().c_str());
    }

    void Warn(qjs::rest<std::string> args)
    {
        std::stringstream ss;
        joinArgs(ss, args);
        CSP_LOG_FORMAT(csp::systems::LogLevel::Warning, "%s", ss.str().c_str());
    }

    void Error(qjs::rest<std::string> args)
    {
        std::stringstream ss;
        joinArgs(ss, args);
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "%s", ss.str().c_str());
    }

private:
    void joinArgs(std::stringstream& ss, const qjs::rest<std::string>& args)
    {
        for (const auto& arg : args)
        {
            ss << arg << " ";
        }
    }
};

using SpaceEntityList = csp::common::List<SpaceEntity*>;

class EntitySystemScriptInterface
{
public:
    EntitySystemScriptInterface(SpaceEntitySystem* InEntitySystem = nullptr, qjs::Context* InContext = nullptr)
        : EntitySystem(InEntitySystem)
        , Context(InContext)
    {
    }

    std::vector<uint64_t> GetEntityIds()
    {
        std::vector<uint64_t> EntityIds;

        if (EntitySystem)
        {
            EntitySystem->LockEntityUpdate();

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                const SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);

                uint64_t Id = Entity->GetId();
                EntityIds.push_back(Id);
            }

            EntitySystem->UnlockEntityUpdate();
        }

        return EntityIds;
    }

    std::vector<EntityScriptInterface*> GetEntities()
    {
        std::vector<EntityScriptInterface*> Entities;

        if (EntitySystem)
        {
            EntitySystem->LockEntityUpdate();

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                Entities.push_back(Entity->GetScriptInterface());
            }

            EntitySystem->UnlockEntityUpdate();
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
            EntitySystem->LockEntityUpdate();

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                const SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                if (Entity->GetId() == static_cast<uint64_t>(EntityId))
                {
                    IndexOfEntity = static_cast<int32_t>(i);
                    break;
                }
            }

            EntitySystem->UnlockEntityUpdate();
        }

        return IndexOfEntity;
    }

    EntityScriptInterface* GetEntityById(int64_t EntityId)
    {
        EntityScriptInterface* ScriptInterface = nullptr;
        if (EntitySystem)
        {
            EntitySystem->LockEntityUpdate();

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                if (Entity->GetId() == static_cast<uint64_t>(EntityId))
                {
                    ScriptInterface = Entity->GetScriptInterface();
                    break;
                }
            }

            EntitySystem->UnlockEntityUpdate();
        }

        return ScriptInterface;
    }

    EntityScriptInterface* GetEntityByName(std::string EntityName)
    {
        EntityScriptInterface* ScriptInterface = nullptr;
        if (EntitySystem)
        {
            EntitySystem->LockEntityUpdate();

            for (size_t i = 0; i < EntitySystem->GetNumEntities(); ++i)
            {
                SpaceEntity* Entity = EntitySystem->GetEntityByIndex(i);
                if (Entity->GetName() == EntityName.c_str())
                {
                    ScriptInterface = Entity->GetScriptInterface();
                    break;
                }
            }

            EntitySystem->UnlockEntityUpdate();
        }

        return ScriptInterface;
    }

    std::vector<EntityScriptInterface*> GetRootHierarchyEntities()
    {
        std::vector<EntityScriptInterface*> RootHierarchyEntities;
        if (EntitySystem)
        {
            EntitySystem->LockEntityUpdate();

            for (size_t i = 0; i < EntitySystem->GetRootHierarchyEntities()->Size(); ++i)
            {
                SpaceEntity* Entity = (*EntitySystem->GetRootHierarchyEntities())[i];
                RootHierarchyEntities.push_back(Entity->GetScriptInterface());
            }

            EntitySystem->UnlockEntityUpdate();
        }

        return RootHierarchyEntities;
    }

    /**
     * Creates a new entity in the system and returns a promise to javascript.
     * TODO SCRIPTS MUST NOT be able to delete entities they didn't create. Adapt the api to only allow the local object to be passed to it
     * Restricting it to just local will suffice for this RnD project
     */
    qjs::Value DeleteLocalEntity(const uint32_t entityId)
    {
        qjs::Context* m_ctx = this->Context;
        uint64_t EntityId = static_cast<uint64_t>(entityId);
        SpaceEntity* EntityToDestroy = EntitySystem->FindSpaceEntityById(EntityId);
        if (EntityToDestroy == nullptr || EntityToDestroy->IsLocal() == false)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found for deletion.", EntityId);
            return qjs::Value(m_ctx->ctx, JS_UNDEFINED);
        }

        // 1. Create a Promise and get its resolving functions.
        // We use the stored context member 'm_ctx'.
        JSValue funcs[2];
        // create a promise, this can be used with async/await in JavaScript
        JSValue promise = JS_NewPromiseCapability(m_ctx->ctx, funcs);

        // called on success
        qjs::Value resolve_func(m_ctx->ctx, std::move(funcs[0]));
        // called on failure
        qjs::Value reject_func(m_ctx->ctx, std::move(funcs[1]));

        EntitySystem->DestroyEntity(EntityToDestroy,
            [resolve_func, reject_func, m_ctx](bool success)
            {
                // We may have already shut down the context, so nothing needed here
                if (m_ctx == nullptr)
                {
                    return;
                }
                if (success)
                {
                    // Call the 'resolve' function
                    JSValueConst argv[] = { JS_UNDEFINED };
                    JSValue result = JS_Call(m_ctx->ctx, resolve_func.v, JS_UNDEFINED, 0, argv);
                    JS_FreeValue(m_ctx->ctx, result);
                }
                else
                {
                    auto error = m_ctx->newValue("Failed to delete entity in C++");
                    JSValueConst argv[] = { error.v };
                    JSValue result = JS_Call(m_ctx->ctx, reject_func.v, JS_UNDEFINED, 1, argv);
                    JS_FreeValue(m_ctx->ctx, result);
                }
            });
        // Return the promise to JavaScript.
        return qjs::Value(m_ctx->ctx, std::move(promise));
    }

    void SetAttributeString(uint64_t id, csp::common::String key, uint32_t type, csp::common::String value)
    {
        if (key.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Cannot set attribute with empty key");
            return;
        }

        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        // Use dynamic_cast instead of static_cast to safely convert ComponentBase to CodeSpaceComponent
        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }
        if (codeComponent->HasAttribute(key))
        {
            CodeAttribute* attrib = codeComponent->GetAttribute(key);
            if (attrib->GetType() != CodePropertyType::STRING)
            {
                // If attribute type has changed, remove the old attribute and continue
                codeComponent->RemoveAttribute(key);
            }
            else
            {
                return;
            }
        }

        // Create a new CodeAttribute and set it on the CodeSpaceComponent
        // Since SetAttribute stores a serialized copy of the attribute,
        // we don't need to worry about ownership transfer
        CodeAttribute attribute;
        attribute.SetType(static_cast<CodePropertyType>(type));
        attribute.SetStringValue(value);

        // Pass the attribute by reference - the component will serialize it
        codeComponent->SetAttribute(key, attribute);
    }

    void SetAttributeFloat(uint64_t id, csp::common::String key, uint32_t type, float min, float max, float value)
    {
        if (key.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Cannot set attribute with empty key");
            return;
        }

        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        // Use dynamic_cast instead of static_cast to safely convert ComponentBase to CodeSpaceComponent
        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }

        if (codeComponent->HasAttribute(key))
        {
            CodeAttribute* attrib = codeComponent->GetAttribute(key);
            if (attrib->GetType() != CodePropertyType::NUMBER && attrib->GetType() != CodePropertyType::SLIDER)
            {
                // If attribute type has changed, remove the old attribute and continue
                codeComponent->RemoveAttribute(key);
            }
            else
            {
                return;
            }
        }

        // Create a new CodeAttribute and set it on the CodeSpaceComponent
        // Since SetAttribute stores a serialized copy of the attribute,
        // we don't need to worry about ownership transfer
        CodeAttribute attribute;
        attribute.SetType(static_cast<CodePropertyType>(type));
        CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Setting attribute %s with value %f and a min max or %f %f", key.c_str(), value, min, max);
        attribute.SetFloatValue(value);
        attribute.SetMin(min);
        attribute.SetMax(max);

        // Pass the attribute by reference - the component will serialize it
        codeComponent->SetAttribute(key, attribute);
    }

    // setAttributeBoolean
    void SetAttributeBoolean(uint64_t id, csp::common::String key, uint32_t type, bool value)
    {
        if (key.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Cannot set attribute with empty key");
            return;
        }

        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }

        if (codeComponent->HasAttribute(key))
        {
            CodeAttribute* attrib = codeComponent->GetAttribute(key);
            if (attrib->GetType() != CodePropertyType::BOOLEAN)
            {
                // If attribute type has changed, remove the old attribute and continue
                codeComponent->RemoveAttribute(key);
            }
            else
            {
                return;
            }
        }

        // Create a new CodeAttribute and set it on the CodeSpaceComponent
        // Since SetAttribute stores a serialized copy of the attribute,
        // we don't need to worry about ownership transfer
        CodeAttribute attribute;
        attribute.SetType(static_cast<CodePropertyType>(type));
        attribute.SetBoolValue(value);

        // Pass the attribute by reference - the component will serialize it
        codeComponent->SetAttribute(key, attribute);
    }

    // setAttributeVector2
    void SetAttributeVector2(uint64_t id, csp::common::String key, uint32_t type, const ComponentScriptInterface::Vector2& vec)
    {
        if (key.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Cannot set attribute with empty key");
            return;
        }

        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }

        if (codeComponent->HasAttribute(key))
        {
            CodeAttribute* attrib = codeComponent->GetAttribute(key);
            if (attrib->GetType() != CodePropertyType::VECTOR2)
            {
                // If attribute type has changed, remove the old attribute and continue
                codeComponent->RemoveAttribute(key);
            }
            else
            {
                return;
            }
        }

        // Create a new CodeAttribute and set it on the CodeSpaceComponent
        // Since SetAttribute stores a serialized copy of the attribute,
        // we don't need to worry about ownership transfer
        CodeAttribute attribute;
        attribute.SetType(static_cast<CodePropertyType>(type));
        csp::common::Vector2 Value(vec[0], vec[1]);
        attribute.SetVector2Value(Value);

        // Pass the attribute by reference - the component will serialize it
        codeComponent->SetAttribute(key, attribute);
    }

        // setAttributeVector3
    void SetAttributeVector3(uint64_t id, csp::common::String key, uint32_t type, const ComponentScriptInterface::Vector3& vec)
    {
        if (key.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Cannot set attribute with empty key");
            return;
        }

        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }

        if (codeComponent->HasAttribute(key))
        {
            CodeAttribute* attrib = codeComponent->GetAttribute(key);
            if (attrib->GetType() != CodePropertyType::VECTOR3)
            {
                // If attribute type has changed, remove the old attribute and continue
                codeComponent->RemoveAttribute(key);
            }
            else
            {
                return;
            }
        }

        // Create a new CodeAttribute and set it on the CodeSpaceComponent
        // Since SetAttribute stores a serialized copy of the attribute,
        // we don't need to worry about ownership transfer
        CodeAttribute attribute;
        attribute.SetType(static_cast<CodePropertyType>(type));
        csp::common::Vector3 Value(vec[0], vec[1], vec[2]);
        attribute.SetVector3Value(Value);

        // Pass the attribute by reference - the component will serialize it
        codeComponent->SetAttribute(key, attribute);
    }

        // setAttributeVector4
    void SetAttributeVector4(uint64_t id, csp::common::String key, uint32_t type, const ComponentScriptInterface::Vector4& vec)
    {
        if (key.IsEmpty())
        {
            CSP_LOG_ERROR_MSG("Cannot set attribute with empty key");
            return;
        }

        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }

        if (codeComponent->HasAttribute(key))
        {
            CodeAttribute* attrib = codeComponent->GetAttribute(key);
            if (attrib->GetType() != CodePropertyType::VECTOR4)
            {
                // If attribute type has changed, remove the old attribute and continue
                codeComponent->RemoveAttribute(key);
            }
            else
            {
                return;
            }
        }

        // Create a new CodeAttribute and set it on the CodeSpaceComponent
        // Since SetAttribute stores a serialized copy of the attribute,
        // we don't need to worry about ownership transfer
        CodeAttribute attribute;
        attribute.SetType(static_cast<CodePropertyType>(type));
        csp::common::Vector4 Value(vec[0], vec[1], vec[2], vec[3]);
        attribute.SetVector4Value(Value);

        // Pass the attribute by reference - the component will serialize it
        codeComponent->SetAttribute(key, attribute);
    }

    void ClearAttributes(uint64_t id)
    {
        SpaceEntity* entity = EntitySystem->FindSpaceEntityById(id);
        if (!entity)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu not found.", id);
            return;
        }

        ComponentBase* baseComponent = entity->FindFirstComponentOfType(ComponentType::Code);
        if (!baseComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu does not have a Code component.", id);
            return;
        }

        CodeSpaceComponent* codeComponent = static_cast<CodeSpaceComponent*>(baseComponent);
        if (!codeComponent)
        {
            CSP_LOG_ERROR_FORMAT("Entity with ID %llu has a Code component, but it's not a CodeSpaceComponent.", id);
            return;
        }

        codeComponent->ClearAttributes();
    }

    void FireEvent(uint64_t id, const std::string& eventName, qjs::Value& eventArgs)
    {
        auto* entity = EntitySystem->FindSpaceEntityById(id);
        if (entity)
        {
            entity->GetScriptInterface()->Fire(eventName, eventArgs);
        }
    }
    //

    /**
     * Creates a new entity in the system and returns a promise to javascript.
     */
    qjs::Value CreateLocalEntity(const std::string& name)
    {
        qjs::Context* m_ctx = this->Context;
        // 1. Create a Promise and get its resolving functions.
        // We use the stored context member 'm_ctx'.
        JSValue funcs[2];
        // create a promise, this can be used with async/await in JavaScript
        JSValue promise = JS_NewPromiseCapability(m_ctx->ctx, funcs);

        // called on success
        qjs::Value resolve_func(m_ctx->ctx, std::move(funcs[0]));
        // called on failure
        qjs::Value reject_func(m_ctx->ctx, std::move(funcs[1]));
        EntitySystem->LockEntityUpdate();
        EntitySystem->CreateLocalObject(name.c_str(), SpaceTransform(),
            [this, resolve_func, reject_func, m_ctx](SpaceEntity* entity)
            {
                this->EntitySystem->UnlockEntityUpdate();

                if (entity)
                {
                    // needed to ensure the system knows about the new entity
                    this->EntitySystem->FireEntityCreatedEvent(entity);
                    // Put out new entity into the promise resolution
                    auto js_entity = m_ctx->newValue(entity->GetScriptInterface());

                    // Call the 'resolve' function
                    JSValueConst argv[] = { js_entity.v };
                    JSValue result = JS_Call(m_ctx->ctx, resolve_func.v, JS_UNDEFINED, 1, argv);
                    JS_FreeValue(m_ctx->ctx, result);
                }
                else
                {
                    auto error = m_ctx->newValue("Failed to create entity in C++");
                    JSValueConst argv[] = { error.v };
                    JSValue result = JS_Call(m_ctx->ctx, reject_func.v, JS_UNDEFINED, 1, argv);
                    JS_FreeValue(m_ctx->ctx, result);
                }
            });
        // Return the promise to JavaScript.
        return qjs::Value(m_ctx->ctx, std::move(promise));
    }

    std::string GetFoundationVersion() { return csp::CSPFoundation::GetVersion().c_str(); }

private:
    SpaceEntitySystem* EntitySystem;
    qjs::Context* Context;
};

void EntityScriptLog(qjs::rest<std::string> Args)
{
    std::stringstream Str;

    for (auto const& Arg : Args)
    {
        Str << Arg << " ";
    }

    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "%s", Str.str().c_str());
}

EntityScriptBinding::EntityScriptBinding(SpaceEntitySystem* InEntitySystem)
    : EntitySystem(InEntitySystem)
{
}

EntityScriptBinding* EntityScriptBinding::BindEntitySystem(SpaceEntitySystem* InEntitySystem)
{
    EntityScriptBinding* ScriptBinding = new EntityScriptBinding(InEntitySystem);
    csp::systems::ScriptSystem* ScriptSystem = csp::systems::SystemsManager::Get().GetScriptSystem();
    ScriptSystem->RegisterScriptBinding(ScriptBinding);
    return ScriptBinding;
}

void EntityScriptBinding::RemoveBinding(EntityScriptBinding* InEntityBinding)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        csp::systems::ScriptSystem* ScriptSystem = csp::systems::SystemsManager::Get().GetScriptSystem();
        ScriptSystem->UnregisterScriptBinding(InEntityBinding);
    }
}

#define PROPERTY_GET_SET(COMP, METHOD, PROP) property<&COMP##ScriptInterface::Get##METHOD, &COMP##ScriptInterface::Set##METHOD>(PROP)
#define PROPERTY_GET_SET_CLASS(COMP, METHOD, PROP) property<&COMP::Get##METHOD, &COMP::Set##METHOD>(PROP)
#define PROPERTY_GET(COMP, METHOD, PROP) property<&COMP##ScriptInterface::Get##METHOD>(PROP)

void BindComponents(qjs::Context::Module* Module)
{
    Module->class_<ButtonSpaceComponentScriptInterface>("ButtonSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(ButtonSpaceComponent, LabelText, "labelText")
        .PROPERTY_GET_SET(ButtonSpaceComponent, IconAssetId, "iconAssetId")
        .PROPERTY_GET_SET(ButtonSpaceComponent, AssetCollectionId, "assetCollectionId")
        .PROPERTY_GET_SET(ButtonSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(ButtonSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(ButtonSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(ButtonSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(ButtonSpaceComponent, IsEnabled, "isEnabled");

    Module->class_<LightSpaceComponentScriptInterface>("LightSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(LightSpaceComponent, LightType, "lightType")
        .PROPERTY_GET_SET(LightSpaceComponent, Intensity, "Intensity")
        .PROPERTY_GET_SET(LightSpaceComponent, Intensity, "intensity")
        .PROPERTY_GET_SET(LightSpaceComponent, Range, "range")
        .PROPERTY_GET_SET(LightSpaceComponent, InnerConeAngle, "innerConeAngle")
        .PROPERTY_GET_SET(LightSpaceComponent, OuterConeAngle, "outerConeAngle")
        .PROPERTY_GET_SET(LightSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(LightSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(LightSpaceComponent, Color, "color")
        .PROPERTY_GET_SET(LightSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(LightSpaceComponent, LightCookieAssetId, "cookieAssetId")
        .PROPERTY_GET_SET(LightSpaceComponent, LightCookieType, "lightCookieType");

    Module->class_<AnimatedModelSpaceComponentScriptInterface>("AnimatedModelSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, ExternalResourceAssetId, "modelAssetId")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, ExternalResourceAssetCollectionId, "assetCollectionId")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, ExternalResourceAssetId, "externalResourceAssetId")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, ExternalResourceAssetCollectionId, "externalResourceAssetCollectionId")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, IsLoopPlayback, "isLoopPlayback")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, IsPlaying, "isPlaying")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(AnimatedModelSpaceComponent, AnimationIndex, "animationIndex");

    Module->class_<VideoPlayerSpaceComponentScriptInterface>("VideoPlayerSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Name, "name")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, VideoAssetId, "videoAssetId")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, VideoAssetURL, "videoAssetURL")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, AssetCollectionId, "assetCollectionId")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, IsStateShared, "isStateShared")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, IsLoopPlayback, "isLoopPlayback")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, IsAutoResize, "isAutoResize")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, PlaybackState, "playbackState")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, CurrentPlayheadPosition, "currentPlayheadPosition")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, TimeSincePlay, "timeSincePlay")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, VideoPlayerSourceType, "videoPlayerSourceType")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(VideoPlayerSpaceComponent, IsEnabled, "isEnabled");

    Module->class_<AvatarSpaceComponentScriptInterface>("AvatarSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(AvatarSpaceComponent, AvatarId, "avatarId")
        .PROPERTY_GET_SET(AvatarSpaceComponent, UserId, "userId")
        .PROPERTY_GET_SET(AvatarSpaceComponent, State, "state")
        .PROPERTY_GET_SET(AvatarSpaceComponent, AvatarMeshIndex, "avatarMeshIndex")
        .PROPERTY_GET_SET(AvatarSpaceComponent, AgoraUserId, "agoraUserId")
        .PROPERTY_GET_SET(AvatarSpaceComponent, CustomAvatarUrl, "customAvatarUrl")
        .PROPERTY_GET_SET(AvatarSpaceComponent, IsHandIKEnabled, "isHandIKEnabled")
        .PROPERTY_GET_SET(AvatarSpaceComponent, TargetHandIKTargetLocation, "targetHandIKTargetLocation")
        .PROPERTY_GET_SET(AvatarSpaceComponent, HandRotation, "handRotation")
        .PROPERTY_GET_SET(AvatarSpaceComponent, HeadRotation, "headRotation")
        .PROPERTY_GET_SET(AvatarSpaceComponent, WalkRunBlendPercentage, "walkRunBlendPercentage")
        .PROPERTY_GET_SET(AvatarSpaceComponent, TorsoTwistAlpha, "torsoTwistAlpha")
        .PROPERTY_GET_SET(AvatarSpaceComponent, AvatarPlayMode, "avatarPlayMode");

    Module->class_<ExternalLinkSpaceComponentScriptInterface>("ExternalLinkSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, Name, "name")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, LinkUrl, "linkUrl")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, DisplayText, "displayText")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, IsEnabled, "isEnabled")
        .PROPERTY_GET_SET(ExternalLinkSpaceComponent, IsVisible, "isVisible");

    Module->class_<FogSpaceComponentScriptInterface>("FogSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(FogSpaceComponent, FogMode, "fogMode")
        .PROPERTY_GET_SET(FogSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(FogSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(FogSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(FogSpaceComponent, StartDistance, "startDistance")
        .PROPERTY_GET_SET(FogSpaceComponent, EndDistance, "endDistance")
        .PROPERTY_GET_SET(FogSpaceComponent, Color, "color")
        .PROPERTY_GET_SET(FogSpaceComponent, Density, "density")
        .PROPERTY_GET_SET(FogSpaceComponent, HeightFalloff, "heightFalloff")
        .PROPERTY_GET_SET(FogSpaceComponent, MaxOpacity, "maxOpacity")
        .PROPERTY_GET_SET(FogSpaceComponent, IsVolumetric, "isVolumetric");

    Module->class_<CinematicCameraSpaceComponentScriptInterface>("CinematicCameraSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&CinematicCameraSpaceComponentScriptInterface::GetFov>("getFov")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, FocalLength, "focalLength")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, AspectRatio, "aspectRatio")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, SensorSize, "sensorSize")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, NearClip, "nearClip")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, FarClip, "farClip")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, Iso, "iso")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, ShutterSpeed, "shutterSpeed")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, Aperture, "aperture")
        .PROPERTY_GET_SET(CinematicCameraSpaceComponent, IsViewerCamera, "isViewerCamera");

    Module->class_<ImageSpaceComponentScriptInterface>("ImageSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(ImageSpaceComponent, Name, "name")
        .PROPERTY_GET_SET(ImageSpaceComponent, ImageAssetId, "imageAssetId")
        .PROPERTY_GET_SET(ImageSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(ImageSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(ImageSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(ImageSpaceComponent, BillboardMode, "billboardMode")
        .PROPERTY_GET_SET(ImageSpaceComponent, DisplayMode, "displayMode")
        .PROPERTY_GET_SET(ImageSpaceComponent, IsEmissive, "isEmissive")
        .PROPERTY_GET_SET(ImageSpaceComponent, IsVisible, "isVisible");

    Module->class_<TextSpaceComponentScriptInterface>("TextSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(TextSpaceComponent, Text, "text")
        .PROPERTY_GET_SET(TextSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(TextSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(TextSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(TextSpaceComponent, TextColor, "textColor")
        .PROPERTY_GET_SET(TextSpaceComponent, BackgroundColor, "backgroundColor")
        .PROPERTY_GET_SET(TextSpaceComponent, IsBackgroundVisible, "isBackgroundVisible")
        .PROPERTY_GET_SET(TextSpaceComponent, Width, "width")
        .PROPERTY_GET_SET(TextSpaceComponent, Height, "height")
        .PROPERTY_GET_SET(TextSpaceComponent, BillboardMode, "billboardMode")
        .PROPERTY_GET_SET(TextSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(TextSpaceComponent, IsARVisible, "isARVisible");

    Module->class_<StaticModelSpaceComponentScriptInterface>("StaticModelSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(StaticModelSpaceComponent, ExternalResourceAssetId, "modelAssetId")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, ExternalResourceAssetCollectionId, "assetCollectionId")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, ExternalResourceAssetId, "externalResourceAssetId")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, ExternalResourceAssetCollectionId, "externalResourceAssetCollectionId")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(StaticModelSpaceComponent, IsVisible, "isVisible");

    Module->class_<PortalSpaceComponentScriptInterface>("PortalSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(PortalSpaceComponent, SpaceId, "spaceId")
        .PROPERTY_GET_SET(PortalSpaceComponent, IsEnabled, "isEnabled")
        .PROPERTY_GET_SET(PortalSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(PortalSpaceComponent, Radius, "radius");

    Module->class_<CustomSpaceComponentScriptInterface>("CustomSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(CustomSpaceComponent, ApplicationOrigin, "applicationOrigin")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomPropertySubscriptionKey>("getCustomPropertySubscriptionKey")
        .fun<&CustomSpaceComponentScriptInterface::HasCustomProperty>("hasCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::RemoveCustomProperty>("removeCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomProperty>("getCustomProperty")
        .fun<&CustomSpaceComponentScriptInterface::GetCustomPropertyKeys>("getCustomPropertyKeys")
        .fun<&CustomSpaceComponentScriptInterface::SetCustomProperty>("setCustomProperty");

    Module->class_<CodeSpaceComponentScriptInterface>("CodeSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET(CodeSpaceComponent, ScriptAssetPath, "scriptAssetPath")
        .fun<&CodeSpaceComponentScriptInterface::HasAttribute>("hasAttribute")
        .fun<&CodeSpaceComponentScriptInterface::GetAttribute>("getAttribute")
        .fun<&CodeSpaceComponentScriptInterface::GetAttributeKeys>("getAttributeKeys");

    Module->class_<SplineSpaceComponentScriptInterface>("SplineSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&SplineSpaceComponentScriptInterface::SetWaypoints>("setWaypoints")
        .fun<&SplineSpaceComponentScriptInterface::GetWaypoints>("getWaypoints")
        .fun<&SplineSpaceComponentScriptInterface::GetLocationAlongSpline>("getLocationAlongSpline");

    Module->class_<ConversationSpaceComponentScriptInterface>("ConversationSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(ConversationSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(ConversationSpaceComponent, IsActive, "isActive")
        .PROPERTY_GET_SET(ConversationSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(ConversationSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(ConversationSpaceComponent, Title, "title")
        .PROPERTY_GET_SET(ConversationSpaceComponent, Resolved, "resolved")
        .PROPERTY_GET_SET(ConversationSpaceComponent, ConversationCameraPosition, "conversationCameraPosition")
        .PROPERTY_GET_SET(ConversationSpaceComponent, ConversationCameraRotation, "conversationCameraRotation");

    Module->class_<AudioSpaceComponentScriptInterface>("AudioSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(AudioSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(AudioSpaceComponent, PlaybackState, "playbackState")
        .PROPERTY_GET_SET(AudioSpaceComponent, AudioType, "audioType")
        .PROPERTY_GET_SET(AudioSpaceComponent, AudioAssetId, "audioAssetId")
        .PROPERTY_GET_SET(AudioSpaceComponent, AssetCollectionId, "assetCollectionId")
        .PROPERTY_GET_SET(AudioSpaceComponent, AttenuationRadius, "attenuationRadius")
        .PROPERTY_GET_SET(AudioSpaceComponent, IsLoopPlayback, "isLoopPlayback")
        .PROPERTY_GET_SET(AudioSpaceComponent, TimeSincePlay, "timeSincePlay")
        .PROPERTY_GET_SET(AudioSpaceComponent, Volume, "volume");

    Module->class_<ECommerceSpaceComponentScriptInterface>("ECommerceSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(ECommerceSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(ECommerceSpaceComponent, ProductId, "productId");

    Module->class_<FiducialMarkerSpaceComponentScriptInterface>("FiducialMarkerSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(FiducialMarkerSpaceComponent, Name, "name")
        .PROPERTY_GET_SET(FiducialMarkerSpaceComponent, MarkerAssetId, "markerAssetId")
        .PROPERTY_GET_SET(FiducialMarkerSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(FiducialMarkerSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(FiducialMarkerSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(FiducialMarkerSpaceComponent, IsVisible, "isVisible");

    Module->class_<GaussianSplatSpaceComponentScriptInterface>("GaussianSplatSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, ExternalResourceAssetId, "externalResourceAssetId")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, ExternalResourceAssetCollectionId, "externalResourceAssetCollectionId")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, Scale, "scale")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, IsARVisible, "isARVisible")
        .PROPERTY_GET_SET(GaussianSplatSpaceComponent, Tint, "tint");

    Module->class_<HotspotSpaceComponentScriptInterface>("HotspotSpaceComponent")
        .constructor<>()
        .base<ComponentScriptInterface>()
        .fun<&HotspotSpaceComponentScriptInterface::GetUniqueComponentId>("getUniqueComponentId")
        .PROPERTY_GET_SET(HotspotSpaceComponent, Position, "position")
        .PROPERTY_GET_SET(HotspotSpaceComponent, Rotation, "rotation")
        .PROPERTY_GET_SET(HotspotSpaceComponent, IsVisible, "isVisible")
        .PROPERTY_GET_SET(HotspotSpaceComponent, IsARVisible, "isARVisible")
        .PROPERTY_GET_SET(HotspotSpaceComponent, IsTeleportPoint, "isTeleportPoint")
        .PROPERTY_GET_SET(HotspotSpaceComponent, IsSpawnPoint, "isSpawnPoint");
}

void BindInternal(qjs::Context::Module* Module)
{
    Module->function<&EntityScriptLog>("Log");

    Module->class_<EntityScriptInterface>("Entity")
        .constructor<>()
        .fun<&EntityScriptInterface::SubscribeToPropertyChange>("subscribeToPropertyChange")
        .fun<&EntityScriptInterface::SubscribeToMessage>("subscribeToMessage")
        .fun<&EntityScriptInterface::PostMessageToScript>("postMessage")
        .fun<&EntityScriptInterface::GetComponents>("getComponents")
        .fun<&EntityScriptInterface::CreateComponentOfType<CinematicCameraSpaceComponentScriptInterface, ComponentType::CinematicCamera>>("createCameraComponent")
        .fun<&EntityScriptInterface::CreateComponentOfType<LightSpaceComponentScriptInterface, ComponentType::Light>>("createLightComponent")
        .fun<&EntityScriptInterface::CreateComponentOfType<AudioSpaceComponentScriptInterface, ComponentType::Audio>>("createAudioComponent")
        .fun<&EntityScriptInterface::CreateComponentOfType<StaticModelSpaceComponentScriptInterface, ComponentType::StaticModel>>(
            "createStaticModelComponent")
        .fun<&EntityScriptInterface::CreateComponentOfType<AnimatedModelSpaceComponentScriptInterface, ComponentType::AnimatedModel>>(
            "createAnimatedModelComponent")
        .fun<&EntityScriptInterface::GetComponentsOfType<LightSpaceComponentScriptInterface, ComponentType::Light>>("getLightComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<ButtonSpaceComponentScriptInterface, ComponentType::Button>>("getButtonComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<CodeSpaceComponentScriptInterface, ComponentType::Code>>("getCodeComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<VideoPlayerSpaceComponentScriptInterface, ComponentType::VideoPlayer>>(
            "getVideoPlayerComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<AnimatedModelSpaceComponentScriptInterface, ComponentType::AnimatedModel>>(
            "getAnimatedModelComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<AvatarSpaceComponentScriptInterface, ComponentType::AvatarData>>("getAvatarComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<ExternalLinkSpaceComponentScriptInterface, ComponentType::ExternalLink>>(
            "getExternalLinkComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<StaticModelSpaceComponentScriptInterface, ComponentType::StaticModel>>(
            "getStaticModelComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<ImageSpaceComponentScriptInterface, ComponentType::Image>>("getImageComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<CustomSpaceComponentScriptInterface, ComponentType::Custom>>("getCustomComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<PortalSpaceComponentScriptInterface, ComponentType::Portal>>("getPortalComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<ConversationSpaceComponentScriptInterface, ComponentType::Conversation>>(
            "getConversationComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<AudioSpaceComponentScriptInterface, ComponentType::Audio>>("getAudioComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<SplineSpaceComponentScriptInterface, ComponentType::Spline>>("getSplineComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<FogSpaceComponentScriptInterface, ComponentType::Fog>>("getFogComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<CinematicCameraSpaceComponentScriptInterface, ComponentType::CinematicCamera>>(
            "getCinematicCameraComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<ECommerceSpaceComponentScriptInterface, ComponentType::ECommerce>>("getECommerceComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<FiducialMarkerSpaceComponentScriptInterface, ComponentType::FiducialMarker>>(
            "getFiducialMarkerComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<GaussianSplatSpaceComponentScriptInterface, ComponentType::GaussianSplat>>(
            "getGaussianSplatComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<TextSpaceComponentScriptInterface, ComponentType::Text>>("getTextComponents")
        .fun<&EntityScriptInterface::GetComponentsOfType<HotspotSpaceComponentScriptInterface, ComponentType::Hotspot>>("getHotspotComponents")
        .fun<&EntityScriptInterface::GetParentEntity>("getParentEntity")
        .fun<&EntityScriptInterface::GetChildEntities>("getChildEntities")
        .fun<&EntityScriptInterface::RemoveParentEntity>("removeParentEntity")
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
        .property<&EntityScriptInterface::IsLocal, &EntityScriptInterface::SetLocal>("isLocal")
        .property<&EntityScriptInterface::GetParentId, &EntityScriptInterface::SetParentId>("parentId")
        .fun<&EntityScriptInterface::On>("on")
        .fun<&EntityScriptInterface::Off>("off");

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
        .fun<&EntitySystemScriptInterface::CreateLocalEntity>("createLocalEntity")
        .fun<&EntitySystemScriptInterface::DeleteLocalEntity>("deleteLocalEntity")
        .fun<&EntitySystemScriptInterface::SetAttributeString>("setAttributeString")
        .fun<&EntitySystemScriptInterface::SetAttributeFloat>("setAttributeFloat")
        .fun<&EntitySystemScriptInterface::SetAttributeBoolean>("setAttributeBoolean")
        .fun<&EntitySystemScriptInterface::SetAttributeVector2>("setAttributeVector2")
        .fun<&EntitySystemScriptInterface::SetAttributeVector3>("setAttributeVector3")
        .fun<&EntitySystemScriptInterface::SetAttributeVector4>("setAttributeVector4")
        .fun<&EntitySystemScriptInterface::ClearAttributes>("clearAttributes")
        .fun<&EntitySystemScriptInterface::GetEntities>("getEntities")
        .fun<&EntitySystemScriptInterface::GetObjects>("getObjects")
        .fun<&EntitySystemScriptInterface::GetAvatars>("getAvatars")
        .fun<&EntitySystemScriptInterface::GetEntityById>("getEntityById")
        .fun<&EntitySystemScriptInterface::GetEntityByName>("getEntityByName")
        .fun<&EntitySystemScriptInterface::GetIndexOfEntity>("getIndexOfEntity")
        .fun<&EntitySystemScriptInterface::GetRootHierarchyEntities>("getRootHierarchyEntities");

    Module->class_<CodeAttributeScriptInterface>("CodeAttribute")
        .constructor<>()
        .PROPERTY_GET(CodeAttribute, Type, "type")
        .PROPERTY_GET(CodeAttribute, StringValue, "stringValue")
        .PROPERTY_GET(CodeAttribute, AssetCollectionValue, "assetCollectionValue")
        .PROPERTY_GET(CodeAttribute, AssetValue, "assetValue")
        .PROPERTY_GET(CodeAttribute, FloatValue, "floatValue")
        .PROPERTY_GET(CodeAttribute, IntValue, "intValue")
        .PROPERTY_GET(CodeAttribute, BoolValue, "boolValue")
        .PROPERTY_GET(CodeAttribute, Vector2Value, "vector2Value")
        .PROPERTY_GET(CodeAttribute, Vector3Value, "vector3Value")
        .PROPERTY_GET(CodeAttribute, Vector4Value, "vector4Value")
        .PROPERTY_GET(CodeAttribute, Min, "min")
        .PROPERTY_GET(CodeAttribute, Max, "max");

    Module->class_<ConsoleInterface>("Console")
        .constructor<>()
        .fun<&ConsoleInterface::Log>("log")
        .fun<&ConsoleInterface::Warn>("warn")
        .fun<&ConsoleInterface::Error>("error");
}

void EntityScriptBinding::Bind(int64_t ContextId, csp::systems::ScriptSystem* ScriptSystem)
{
    if (ScriptSystem == nullptr)
    {
        ScriptSystem = csp::systems::SystemsManager::Get().GetScriptSystem();
    }

    qjs::Context* Context = (qjs::Context*)ScriptSystem->GetContext(ContextId);
    qjs::Context::Module* Module = (qjs::Context::Module*)ScriptSystem->GetModule(ContextId, csp::systems::SCRIPT_NAMESPACE);

    BindInternal(Module);

    Context->global()["TheEntitySystem"] = new EntitySystemScriptInterface(EntitySystem, Context);
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

void EntityScriptBinding::BindLocalScriptRoot(qjs::Context* Context, qjs::Context::Module* Module)
{
    BindInternal(Module);
    // This is temporary
    Context->global()["TheEntitySystem"] = new EntitySystemScriptInterface(EntitySystem, Context);
    Context->global()["console"] = new ConsoleInterface(); // Add console here as well
}

} // namespace csp::multiplayer

// Add the js_traits specialization after the namespace close
// This needs to be in the global namespace for QuickJS to find it
namespace qjs
{
// Add specialization for CodeAttributeScriptInterface
template <> struct js_traits<csp::multiplayer::CodeAttributeScriptInterface, void>
{
    static JSValue wrap(JSContext* ctx, csp::multiplayer::CodeAttributeScriptInterface val)
    {
        // Create a new object with the class ID
        JSValue obj = JS_NewObjectClass(ctx, csp::multiplayer::CodeAttributeScriptInterface::js_class_id);

        // Store a copy of the val in the object
        if (!JS_IsException(obj))
        {
            // Allocate memory for the CodeAttributeScriptInterface
            auto* p = new csp::multiplayer::CodeAttributeScriptInterface(val);

            // Set the C++ object pointer as property of the JS object
            JS_SetOpaque(obj, p);
        }

        return obj;
    }
};
}
