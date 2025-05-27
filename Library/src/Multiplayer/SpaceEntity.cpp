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
#include "CSP/Multiplayer/SpaceEntity.h"

#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/Components/ButtonSpaceComponent.h"
#include "CSP/Multiplayer/Components/CinematicCameraSpaceComponent.h"
#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/Components/ECommerceSpaceComponent.h"
#include "CSP/Multiplayer/Components/ExternalLinkSpaceComponent.h"
#include "CSP/Multiplayer/Components/FiducialMarkerSpaceComponent.h"
#include "CSP/Multiplayer/Components/FogSpaceComponent.h"
#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"
#include "CSP/Multiplayer/Components/PortalSpaceComponent.h"
#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "Common/Convert.h"
#include "Debug/Logging.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/MCSComponentPacker.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "Multiplayer/Script/EntityScriptInterface.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "signalrclient/signalr_value.h"

#include <chrono>
#include <glm/gtc/quaternion.hpp>
#include <thread>

using namespace std::chrono;

// Queue pending script property updates
// #define CSP_ENTITY_SCRIPT_UPDATE_DEFERRED

namespace csp::multiplayer
{

glm::mat4 computeParentMat4(const SpaceTransform& ParentSpaceTransform)
{
    glm::mat4 ParentTranslate = glm::translate(
        glm::mat4(1.0f), glm::vec3(ParentSpaceTransform.Position.X, ParentSpaceTransform.Position.Y, ParentSpaceTransform.Position.Z));
    glm::quat ParentOrientation { ParentSpaceTransform.Rotation.W, ParentSpaceTransform.Rotation.X, ParentSpaceTransform.Rotation.Y,
        ParentSpaceTransform.Rotation.Z };

    glm::mat4 ParentRotation(ParentOrientation);
    glm::mat4 ParentScale
        = glm::scale(glm::mat4(1.0f), glm::vec3(ParentSpaceTransform.Scale.X, ParentSpaceTransform.Scale.Y, ParentSpaceTransform.Scale.Z));
    glm::mat4 ParentTransform = ParentTranslate * ParentRotation * ParentScale;
    return ParentTransform;
}

inline uint32_t CheckedUInt64ToUint32(uint64_t Value)
{
    assert(Value <= UINT32_MAX);
    uint32_t ValueAsUInt32 = static_cast<uint32_t>(Value);
    return ValueAsUInt32;
}

SpaceEntity::SpaceEntity()
    : EntitySystem(nullptr)
    , Type(SpaceEntityType::Avatar)
    , Id(0)
    , IsTransferable(true)
    , IsPersistent(true)
    , OwnerId(0)
    , ParentId(nullptr)
    , ShouldUpdateParent(false)
    , Transform { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } }
    , ThirdPartyPlatform(csp::systems::EThirdPartyPlatform::NONE)
    , ThirdPartyRef("")
    , SelectedId(0)
    , Parent(nullptr)
    , EntityLock(LockType::None)
    , NextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , Script(this, nullptr)
    , ScriptInterface(std::make_unique<EntityScriptInterface>(this))
    , TimeOfLastPatch(0)
{
}

SpaceEntity::SpaceEntity(SpaceEntitySystem* InEntitySystem)
    : EntitySystem(InEntitySystem)
    , Type(SpaceEntityType::Avatar)
    , Id(0)
    , IsTransferable(true)
    , IsPersistent(true)
    , OwnerId(0)
    , ParentId(nullptr)
    , ShouldUpdateParent(false)
    , Transform { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } }
    , ThirdPartyPlatform(csp::systems::EThirdPartyPlatform::NONE)
    , ThirdPartyRef("")
    , SelectedId(0)
    , Parent(nullptr)
    , EntityLock(LockType::None)
    , NextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , Script(this, InEntitySystem)
    , ScriptInterface(std::make_unique<EntityScriptInterface>(this))
    , TimeOfLastPatch(0)
{
}

SpaceEntity::SpaceEntity(SpaceEntitySystem* EntitySystem, SpaceEntityType Type, uint64_t Id, const csp::common::String& Name,
    const csp::multiplayer::SpaceTransform& Transform, uint64_t OwnerId, bool IsTransferable, bool IsPersistent)
    : SpaceEntity(EntitySystem)
{
    this->Id = Id;
    this->Type = Type;
    this->Name = Name;
    this->Transform = Transform;
    this->OwnerId = OwnerId;
    this->IsTransferable = IsTransferable;
    this->IsPersistent = IsPersistent;
}

SpaceEntity::~SpaceEntity()
{
    auto& Keys = *Components.Keys();

    size_t i = 0;
    for (i = 0; i < Keys.Size(); ++i)
    {
        delete (Components[Keys[i]]);
    }
}

uint64_t SpaceEntity::GetId() const { return Id; }

uint64_t SpaceEntity::GetOwnerId() const { return OwnerId; }

const csp::common::String& SpaceEntity::GetName() const { return Name; }

void SpaceEntity::SetName(const csp::common::String& Value)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_ENTITYNAME);

    if (Name != Value)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_ENTITYNAME] = Value;
    }
}

const SpaceTransform& SpaceEntity::GetTransform() const { return Transform; }

SpaceTransform SpaceEntity::GetGlobalTransform() const
{
    if (Parent != nullptr)
    {
        SpaceTransform GlobalTransform;
        GlobalTransform.Position = GetGlobalPosition();
        GlobalTransform.Rotation = GetGlobalRotation();
        GlobalTransform.Scale = GetGlobalScale();
        return GlobalTransform;
    }
    return Transform;
}

const csp::common::Vector3& SpaceEntity::GetPosition() const { return Transform.Position; }

csp::common::Vector3 SpaceEntity::GetGlobalPosition() const
{
    if (Parent != nullptr)
    {
        glm::mat4 ParentTransform = computeParentMat4(Parent->GetGlobalTransform());

        glm::vec3 GlobalEntityPosition = ParentTransform * glm::vec4(Transform.Position.X, Transform.Position.Y, Transform.Position.Z, 1.0f);

        return { GlobalEntityPosition.x, GlobalEntityPosition.y, GlobalEntityPosition.z };
    }
    else
        return Transform.Position;
}

void SpaceEntity::SetPosition(const csp::common::Vector3& Value)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_POSITION);

    if (Transform.Position != Value)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_POSITION] = Value;
    }
}

const csp::common::Vector4& SpaceEntity::GetRotation() const { return Transform.Rotation; }

csp::common::Vector4 SpaceEntity::GetGlobalRotation() const
{
    if (Parent != nullptr)
    {
        csp::common::Vector4 GlobalRotation = Parent->GetGlobalRotation();
        glm::quat Orientation { Transform.Rotation.W, Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z };
        glm::quat GlobalOrientation(GlobalRotation.W, GlobalRotation.X, GlobalRotation.Y, GlobalRotation.Z);

        glm::quat FinalOrientation = GlobalOrientation * Orientation;
        return csp::common::Vector4 { FinalOrientation.x, FinalOrientation.y, FinalOrientation.z, FinalOrientation.w };
    }
    else
        return Transform.Rotation;
}

void SpaceEntity::SetRotation(const csp::common::Vector4& Value)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_ROTATION);

    if (Transform.Rotation != Value)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_ROTATION] = Value;
    }
}

const csp::common::Vector3& SpaceEntity::GetScale() const { return Transform.Scale; }

csp::common::Vector3 SpaceEntity::GetGlobalScale() const
{
    if (Parent != nullptr)
        return Parent->GetGlobalScale() * Transform.Scale;
    return Transform.Scale;
}

void SpaceEntity::SetScale(const csp::common::Vector3& Value)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_SCALE);

    if (Transform.Scale != Value)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_SCALE] = Value;
    }
}

bool SpaceEntity::GetIsTransient() const { return !IsPersistent; }

const csp::common::String& SpaceEntity::GetThirdPartyRef() const { return ThirdPartyRef; }

void SpaceEntity::SetThirdPartyRef(const csp::common::String& InThirdPartyRef)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_THIRDPARTYREF);

    if (ThirdPartyRef != InThirdPartyRef)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_THIRDPARTYREF] = InThirdPartyRef;
    }
}

void SpaceEntity::SetThirdPartyPlatformType(const csp::systems::EThirdPartyPlatform InThirdPartyPlatformType)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM);

    if (ThirdPartyPlatform != InThirdPartyPlatformType)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM] = ReplicatedValue(static_cast<int64_t>(InThirdPartyPlatformType));
    }
}

csp::systems::EThirdPartyPlatform SpaceEntity::GetThirdPartyPlatformType() const { return ThirdPartyPlatform; }

SpaceEntityType SpaceEntity::GetEntityType() const { return Type; }

SpaceEntitySystem* SpaceEntity::GetSpaceEntitySystem() { return EntitySystem; }

void SpaceEntity::SetParentId(uint64_t InParentId)
{
    // If the current parentid differs from the input
    if (ParentId.HasValue() == false || InParentId != *ParentId)
    {
        ParentId = InParentId;
        ShouldUpdateParent = true;
    }
}

csp::common::Optional<uint64_t> SpaceEntity::GetParentId() { return ParentId; }

void SpaceEntity::RemoveParentEntity()
{
    if (ParentId.HasValue())
    {
        ParentId = nullptr;
        ShouldUpdateParent = true;
    }
}

SpaceEntity* SpaceEntity::GetParentEntity() const { return Parent; }

void SpaceEntity::CreateChildEntity(const csp::common::String& InName, const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback)
{
    EntitySystem->CreateObjectInternal(InName, GetId(), InSpaceTransform, Callback);
}

const csp::common::List<SpaceEntity*>* SpaceEntity::GetChildEntities() const { return &ChildEntities; }

void SpaceEntity::QueueUpdate()
{
    if (EntitySystem)
    {
        EntitySystem->QueueEntityUpdate(this);
    }
}

void SpaceEntity::Destroy(CallbackHandler Callback)
{
    if (EntitySystem)
    {
        EntitySystem->DestroyEntity(this, Callback);
    }
}

void SpaceEntity::SetUpdateCallback(UpdateCallback Callback) { EntityUpdateCallback = Callback; }

void SpaceEntity::SetDestroyCallback(DestroyCallback Callback) { EntityDestroyCallback = Callback; }

void SpaceEntity::SetPatchSentCallback(CallbackHandler Callback) { EntityPatchSentCallback = Callback; }

void SpaceEntity::MarkForUpdate()
{
    if (EntitySystem)
    {
#if defined(CSP_ENTITY_SCRIPT_UPDATE_DEFERRED)
        EntitySystem->MarkEntityForUpdate(this);
#else
        EntitySystem->QueueEntityUpdate(this);
#endif
    }
    else
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Warning, "Space Entity not marked for update, no local EntitySystem found.");
    }
}

const csp::common::Map<uint16_t, ComponentBase*>* SpaceEntity::GetComponents() const { return &Components; }

ComponentBase* SpaceEntity::GetComponent(uint16_t Key)
{
    if (Components.HasKey(Key))
    {
        return Components[Key];
    }
    else
    {
        return nullptr;
    }
}

ComponentBase* SpaceEntity::AddComponent(ComponentType AddType)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_MSG("Entity is locked. New components can not be added to a locked Entity.");

        return nullptr;
    }

    std::scoped_lock<std::mutex> ComponentsLocker(ComponentsLock);

    if (AddType == ComponentType::ScriptData)
    {
        ComponentBase* ScriptComponent = FindFirstComponentOfType(ComponentType::ScriptData, true);

        if (ScriptComponent)
        {
            CSP_LOG_MSG(csp::systems::LogLevel::Warning, "AddComponent: Script Component already exists on this entity.");

            // Return the existing script component
            return ScriptComponent;
        }
    }

    auto ComponentId = GenerateComponentId();
    auto* Component = InstantiateComponent(ComponentId, AddType);

    // If Component is null, component has not been instantiated, so is skipped.
    if (Component != nullptr)
    {
        DirtyComponents[ComponentId] = DirtyComponent { Component, ComponentUpdateType::Add };
        Component->OnCreated();
    }

    return Component;
}

void SpaceEntity::RemoveComponent(uint16_t Key)
{
    if (!IsModifiable())
    {
        CSP_LOG_ERROR_MSG("Entity is locked. Components can not be removed from a locked Entity.");

        return;
    }

    std::scoped_lock<std::mutex> ComponentsLocker(ComponentsLock);

    if (!TransientDeletionComponentIds.Contains(Key) || Components.HasKey(Key))
    {
        DirtyComponents.Remove(Key);
        TransientDeletionComponentIds.Append(Key);
    }
    else
    {
        CSP_LOG_ERROR_MSG("RemoveComponent: No Component with the specified key found!");
    }
}

void SpaceEntity::RemoveChildEntities() { GetParentEntity()->ChildEntities.RemoveItem(this); }

void SpaceEntity::RemoveParentFromChildEntity(size_t Index)
{
    if (Index < ChildEntities.Size())
    {
        ChildEntities[Index]->RemoveParentEntity();
        ChildEntities[Index]->Parent = nullptr;
    }
}

void SpaceEntity::RemoveParentId() { ParentId = nullptr; }

void SpaceEntity::ApplyLocalPatch(bool InvokeUpdateCallback)
{
    /// If we're sending patches to ourselves, don't apply local patches, as we'll be directly deserialising the data instead.
    if (!csp::systems::SystemsManager::Get().GetMultiplayerConnection()->GetAllowSelfMessagingFlag())
    {
        std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);
        std::scoped_lock<std::mutex> ComponentsLocker(ComponentsLock);

        auto UpdateFlags = static_cast<SpaceEntityUpdateFlags>(0);

        const csp::common::Array<uint16_t>* DirtyComponentKeys = DirtyComponents.Keys();

        // Allocate a ComponentUpdates array (to pass update info to the client), with
        // sufficient size for all dirty components and scheduled deletions.
        csp::common::Array<ComponentUpdateInfo> ComponentUpdates(DirtyComponentKeys->Size() + TransientDeletionComponentIds.Size());

        if (DirtyComponentKeys->Size() > 0)
        {
            UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_COMPONENTS);

            for (size_t i = 0; i < DirtyComponentKeys->Size(); ++i)
            {
                uint16_t ComponentKey = DirtyComponentKeys->operator[](i);

                switch (DirtyComponents[ComponentKey].UpdateType)
                {
                case ComponentUpdateType::Add:
                    Components[ComponentKey] = DirtyComponents[ComponentKey].Component;
                    ComponentUpdates[i].ComponentId = DirtyComponents[ComponentKey].Component->GetId();
                    ComponentUpdates[i].UpdateType = ComponentUpdateType::Add;
                    break;
                case ComponentUpdateType::Delete:
                    DestroyComponent(ComponentKey);
                    ComponentUpdates[i].ComponentId = ComponentKey;
                    ComponentUpdates[i].UpdateType = ComponentUpdateType::Delete;
                    break;
                case ComponentUpdateType::Update:
                {
                    ComponentUpdates[i].ComponentId = DirtyComponents[ComponentKey].Component->GetId();
                    ComponentUpdates[i].UpdateType = ComponentUpdateType::Update;

                    // TODO: For the moment, we update all properties on a dirty component, in future we need to change this to per property
                    // replication. Components[DirtyComponents[i].Component->GetId()]->Properties = DirtyComponents[i].Component->DirtyProperties;

                    /*const csp::common::Map<uint32_t, ReplicatedValue> DirtyComponentProperties = DirtyComponents[i].Component->DirtyProperties;
                    const csp::common::Array<uint32_t>* DirtyComponentPropertyKeys			  = DirtyComponentProperties.Keys();

                    for (size_t j = 0; j < DirtyComponentPropertyKeys->Size(); j++)
                    {
                            uint32_t PropertyKey							  = DirtyComponentPropertyKeys->operator[](j);
                            Components[ComponentKey]->Properties[PropertyKey] = DirtyComponentProperties[PropertyKey];

                            ComponentUpdates[i].PropertyInfo[j].PropertyId = PropertyKey;
                            ComponentUpdates[i].PropertyInfo[j].UpdateType = ComponentUpdateType::Update;
                    }

                    DirtyComponents[i].Component->DirtyProperties.Clear();*/
                    break;
                }
                default:
                    break;
                }
            }

            DirtyComponents.Clear();
        }

        delete (DirtyComponentKeys);

        if (DirtyProperties.Size() > 0)
        {
            const csp::common::Array<uint16_t>* DirtyViewKeys = DirtyProperties.Keys();

            for (size_t i = 0; i < DirtyViewKeys->Size(); ++i)
            {
                uint16_t PropertyKey = DirtyViewKeys->operator[](i);
                switch (PropertyKey)
                {
                case COMPONENT_KEY_VIEW_ENTITYNAME:
                    Name = DirtyProperties[PropertyKey].GetString();
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_NAME);
                    break;
                case COMPONENT_KEY_VIEW_POSITION:
                    Transform.Position = DirtyProperties[PropertyKey].GetVector3();
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_POSITION);
                    break;
                case COMPONENT_KEY_VIEW_ROTATION:
                    Transform.Rotation = DirtyProperties[PropertyKey].GetVector4();
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_ROTATION);
                    break;
                case COMPONENT_KEY_VIEW_SCALE:
                    Transform.Scale = DirtyProperties[PropertyKey].GetVector3();
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_SCALE);
                    break;
                case COMPONENT_KEY_VIEW_SELECTEDCLIENTID:
                    SelectedId = DirtyProperties[PropertyKey].GetInt();
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_SELECTION_ID);
                    break;
                case COMPONENT_KEY_VIEW_THIRDPARTYREF:
                    ThirdPartyRef = DirtyProperties[PropertyKey].GetString();
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_REF);
                    break;
                case COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM:
                    ThirdPartyPlatform = static_cast<csp::systems::EThirdPartyPlatform>(DirtyProperties[PropertyKey].GetInt());
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_PLATFORM);
                    break;
                case COMPONENT_KEY_VIEW_LOCKTYPE:
                    EntityLock = static_cast<LockType>(DirtyProperties[PropertyKey].GetInt());
                    UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_LOCK_TYPE);
                    break;
                default:
                    break;
                }
            }

            DirtyProperties.Clear();
            delete (DirtyViewKeys);
        }

        if (TransientDeletionComponentIds.Size() > 0)
        {
            DirtyComponentKeys = DirtyComponents.Keys();

            for (size_t i = 0; i < TransientDeletionComponentIds.Size(); ++i)
            {
                if (Components.HasKey(TransientDeletionComponentIds[i]))
                {
                    ComponentBase* Component = GetComponent(TransientDeletionComponentIds[i]);
                    Component->OnLocalDelete();

                    DestroyComponent(TransientDeletionComponentIds[i]);

                    // Start indexing from the end of the section reserved for DirtyComponents.
                    // We start adding DirtyComponents to ComponentUpdates first, so here we need to respect that
                    // and start at an offset to add our deletion updates.
                    ComponentUpdates[DirtyComponentKeys->Size() + i].ComponentId = TransientDeletionComponentIds[i];
                    ComponentUpdates[DirtyComponentKeys->Size() + i].UpdateType = ComponentUpdateType::Delete;
                }
            }

            TransientDeletionComponentIds.Clear();
            delete (DirtyComponentKeys);
        }

        if (ShouldUpdateParent)
        {
            EntitySystem->ResolveEntityHierarchy(this);
            UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_PARENT);
            ShouldUpdateParent = false;
        }

        if (InvokeUpdateCallback && EntityUpdateCallback != nullptr)
        {
            EntityUpdateCallback(this, UpdateFlags, ComponentUpdates);
        }
    }
}

SpaceEntity::UpdateCallback SpaceEntity::GetEntityUpdateCallback() { return EntityUpdateCallback; }

void SpaceEntity::SetEntityUpdateCallbackParams(SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& Info)
{
    EntityUpdateCallback(Entity, Flags, Info);
}

SpaceEntity::DestroyCallback SpaceEntity::GetEntityDestroyCallback() { return EntityDestroyCallback; }

void SpaceEntity::SetEntityDestroyCallbackParams(bool Boolean) { EntityDestroyCallback(Boolean); }

SpaceEntity::CallbackHandler SpaceEntity::GetEntityPatchSentCallback() { return EntityPatchSentCallback; }

void SpaceEntity::SetEntityPatchSentCallbackParams(bool Boolean) { EntityPatchSentCallback(Boolean); }

csp::common::Map<uint16_t, SpaceEntity::DirtyComponent> SpaceEntity::GetDirtyComponents() { return DirtyComponents; }

csp::common::Map<uint16_t, csp::multiplayer::ReplicatedValue> SpaceEntity::GetDirtyProperties() { return DirtyProperties; }

csp::common::List<uint16_t> SpaceEntity::GetTransientDeletionComponentIds() { return TransientDeletionComponentIds; }

bool SpaceEntity::GetShouldUpdateParent() { return ShouldUpdateParent; }

void SpaceEntity::SetShouldUpdateParent(const bool Boolean) { ShouldUpdateParent = Boolean; }

SpaceEntity* SpaceEntity::GetParent() { return Parent; }

void SpaceEntity::SetParent(SpaceEntity* InParent) { Parent = InParent; }

uint16_t SpaceEntity::GenerateComponentId()
{
    auto NextId = NextComponentId;

    for (;;)
    {
        if (!Components.HasKey(NextId) && !DirtyComponents.HasKey(NextId))
        {
            NextComponentId = NextId + 1;

            return NextId;
        }

        ++NextId;

        if (NextId == COMPONENT_KEY_END_COMPONENTS)
        {
            NextId = COMPONENT_KEY_START_COMPONENTS;
        }
    }
}

ComponentBase* SpaceEntity::InstantiateComponent(uint16_t InstantiateId, ComponentType InstantiateType)
{
    ComponentBase* Component;

    switch (InstantiateType)
    {
    case ComponentType::StaticModel:
        Component = new StaticModelSpaceComponent(this);
        break;
    case ComponentType::AnimatedModel:
        Component = new AnimatedModelSpaceComponent(this);
        break;
    case ComponentType::VideoPlayer:
        Component = new VideoPlayerSpaceComponent(this);
        break;
    case ComponentType::Image:
        Component = new ImageSpaceComponent(this);
        break;
    case ComponentType::ExternalLink:
        Component = new ExternalLinkSpaceComponent(this);
        break;
    case ComponentType::AvatarData:
        Component = new AvatarSpaceComponent(this);
        break;
    case ComponentType::Light:
        Component = new LightSpaceComponent(this);
        break;
    case ComponentType::ScriptData:
        Component = new ScriptSpaceComponent(this);
        break;
    case ComponentType::Button:
        Component = new ButtonSpaceComponent(this);
        break;
    case ComponentType::Custom:
        Component = new CustomSpaceComponent(this);
        break;
    case ComponentType::Portal:
        Component = new PortalSpaceComponent(this);
        break;
    case ComponentType::Conversation:
        Component = new ConversationSpaceComponent(this);
        break;
    case ComponentType::Audio:
        Component = new AudioSpaceComponent(this);
        break;
    case ComponentType::Spline:
        Component = new SplineSpaceComponent(this);
        break;
    case ComponentType::Collision:
        Component = new CollisionSpaceComponent(this);
        break;
    case ComponentType::Reflection:
        Component = new ReflectionSpaceComponent(this);
        break;
    case ComponentType::Fog:
        Component = new FogSpaceComponent(this);
        break;
    case ComponentType::ECommerce:
        Component = new ECommerceSpaceComponent(this);
        break;
    case ComponentType::CinematicCamera:
        Component = new CinematicCameraSpaceComponent(this);
        break;
    case ComponentType::FiducialMarker:
        Component = new FiducialMarkerSpaceComponent(this);
        break;
    case ComponentType::GaussianSplat:
        Component = new GaussianSplatSpaceComponent(this);
        break;
    case ComponentType::Text:
        Component = new TextSpaceComponent(this);
        break;
    case ComponentType::Hotspot:
        Component = new HotspotSpaceComponent(this);
        break;
    default:
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Warning,
            csp::common::StringFormat("Unknown Component type of value: %d", static_cast<uint32_t>(InstantiateType)));
        return nullptr;
    }
    }

    Component->Id = InstantiateId;

    return Component;
}

void SpaceEntity::AddDirtyComponent(ComponentBase* Component)
{
    std::scoped_lock<std::mutex> ComponentsLocker(ComponentsLock);

    if (DirtyComponents.HasKey(Component->GetId()))
    {
        return;
    }

    DirtyComponents[Component->GetId()] = DirtyComponent { Component, ComponentUpdateType::Update };
}

EntityScript& SpaceEntity::GetScript() { return Script; }

bool SpaceEntity::IsSelected() const { return SelectedId != 0; }

uint64_t SpaceEntity::GetSelectingClientID() const { return SelectedId; }

bool SpaceEntity::Select()
{
    std::scoped_lock EntitiesLocker(EntityMutexLock);
    return EntitySystem->SetSelectionStateOfEntity(true, this);
}

bool SpaceEntity::Deselect()
{
    std::scoped_lock EntitiesLocker(EntityMutexLock);
    return EntitySystem->SetSelectionStateOfEntity(false, this);
}

bool SpaceEntity::IsModifiable() const
{
    if (EntitySystem == nullptr || csp::systems::SystemsManager::Get().GetMultiplayerConnection() == nullptr)
    {
        // Return true here so entities that arent attached to the entity system can be modified.
        // This is currently used for testing.
        return true;
    }

    if (EntityLock == LockType::UserAgnostic &&
        // In the case where we are about to unlock a locked entity we want to treat it as if it's unlocked so we can modify it,
        // so we skip the lock check they are about to unlock.
        // We know they are going to unlock if EntityLock is set and they have COMPONENT_KEY_VIEW_LOCKTYPE in DirtyProperties
        (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_LOCKTYPE) == false))
    {
        return false;
    }

    return (OwnerId == csp::systems::SystemsManager::Get().GetMultiplayerConnection()->GetClientId() || IsTransferable);
}

void SpaceEntity::Lock()
{
    if (IsLocked())
    {
        CSP_LOG_ERROR_MSG("Entity is already locked.")
        return;
    }

    if (!IsModifiable())
    {
        CSP_LOG_ERROR_FORMAT("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                             "owner of. Entity name: %s",
            Name.c_str());
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_LOCKTYPE);
    DirtyProperties[COMPONENT_KEY_VIEW_LOCKTYPE] = ReplicatedValue(static_cast<int64_t>(LockType::UserAgnostic));
}

void SpaceEntity::Unlock()
{
    if (IsLocked() == false)
    {
        CSP_LOG_ERROR_MSG("Entity is not currently locked.")
        return;
    }

    std::scoped_lock<std::mutex> PropertiesLocker(PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_LOCKTYPE);
    DirtyProperties[COMPONENT_KEY_VIEW_LOCKTYPE] = ReplicatedValue(static_cast<int64_t>(LockType::None));
}

bool SpaceEntity::IsLocked() const { return EntityLock != LockType::None; }

bool SpaceEntity::InternalSetSelectionStateOfEntity(const bool SelectedState, uint64_t ClientID)
{
    if (SelectedState)
    {
        if (!IsSelected())
        {
            DirtyProperties.Remove(COMPONENT_KEY_VIEW_SELECTEDCLIENTID);

            if (SelectedId != ClientID)
            {
                DirtyProperties[COMPONENT_KEY_VIEW_SELECTEDCLIENTID] = static_cast<int64_t>(ClientID);
            }

            SelectedId = ClientID;

            return true;
        }
        return false;
    }

    if (IsSelected())
    {
        DirtyProperties.Remove(COMPONENT_KEY_VIEW_SELECTEDCLIENTID);

        if (SelectedId != 0)
        {
            DirtyProperties[COMPONENT_KEY_VIEW_SELECTEDCLIENTID] = static_cast<int64_t>(0);
        }

        SelectedId = 0;
        return true;
    }

    return false;
}

std::chrono::milliseconds SpaceEntity::GetTimeOfLastPatch() { return TimeOfLastPatch; }

void SpaceEntity::SetTimeOfLastPatch(std::chrono::milliseconds NewTime) { TimeOfLastPatch = NewTime; }

void SpaceEntity::SetOwnerId(uint64_t InOwnerId) { OwnerId = InOwnerId; }

void SpaceEntity::DestroyComponent(uint16_t Key)
{
    if (Components.HasKey(Key))
    {
        Components[Key]->OnRemove();
        Components.Remove(Key);
    }
    else
    {
        CSP_LOG_ERROR_MSG("DestroyComponent: Key Does Not Exist")
    }
}

ComponentBase* SpaceEntity::FindFirstComponentOfType(ComponentType FindType, bool SearchDirtyComponents) const
{
    const csp::common::Array<uint16_t>* ComponentKeys = Components.Keys();
    ComponentBase* LocatedComponent = nullptr;

    for (size_t i = 0; i < ComponentKeys->Size(); ++i)
    {
        ComponentBase* Component = Components[ComponentKeys->operator[](i)];

        if (Component->GetComponentType() == FindType)
        {
            LocatedComponent = Component;
            break;
        }
    }

    delete (ComponentKeys);

    if (LocatedComponent == nullptr && SearchDirtyComponents)
    {
        const csp::common::Array<uint16_t>* DirtyComponentKeys = DirtyComponents.Keys();

        for (size_t i = 0; i < DirtyComponentKeys->Size(); ++i)
        {
            const DirtyComponent& Component = DirtyComponents[DirtyComponentKeys->operator[](i)];

            if (Component.UpdateType != ComponentUpdateType::Delete && Component.Component->GetComponentType() == FindType)
            {
                LocatedComponent = Component.Component;
                break;
            }
        }

        delete (DirtyComponentKeys);
    }

    return LocatedComponent;
}

void SpaceEntity::AddChildEntity(SpaceEntity* ChildEntity) { ChildEntities.Append(ChildEntity); }

void SpaceEntity::ResolveParentChildRelationship()
{
    // Entity has been re-parented
    if (ParentId.HasValue())
    {
        // Entity was previously parented to another object, so cleanup
        if (Parent != nullptr)
        {
            Parent->ChildEntities.RemoveItem(this);
        }

        // Set our new parent
        Parent = GetSpaceEntitySystem()->FindSpaceEntityById(*ParentId);

        if (Parent == nullptr)
        {
            // Parent may not have been added yet
            // so check pending entities
            for (const auto& PendingParent : *EntitySystem->GetPendingAdds())
            {
                if (PendingParent->Id == *ParentId)
                {
                    Parent = PendingParent;
                    break;
                }
            }
        }

        if (Parent != nullptr)
        {
            Parent->ChildEntities.Append(this);
        }
        else
        {
            CSP_LOG_ERROR_FORMAT(
                "SpaceEntity unable to find parent for entity: %s. Please report if this issue is encountered.", std::to_string(GetId()).c_str());
            return;
        }
    }
    else
    {
        if (Parent != nullptr)
        {
            Parent->ChildEntities.RemoveItem(this);
            Parent = nullptr;
        }
    }
}

mcs::ObjectMessage SpaceEntity::CreateObjectMessage()
{
    // 1. Convert all of our view components to mcs compatible types.
    MCSComponentPacker ComponentPacker;

    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_ENTITYNAME, GetName());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_POSITION, GetPosition());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_ROTATION, GetRotation());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_SCALE, GetScale());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, GetSelectingClientID());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, GetThirdPartyPlatformType());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_THIRDPARTYREF, GetThirdPartyRef());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_LOCKTYPE, EntityLock);

    // 2. Convert all of our runtime components to mcs compatible types.
    std::unique_ptr<common::Array<uint16_t>> Keys(const_cast<common::Array<uint16_t>*>(DirtyComponents.Keys()));

    // Loop through all components and convert to ItemComponentData.
    for (uint16_t Key : *Keys)
    {
        assert(DirtyComponents[Key].Component != nullptr && "DirtyComponent given a null component!");

        if (DirtyComponents[Key].Component != nullptr)
        {
            auto* Component = DirtyComponents[Key].Component;
            ComponentPacker.WriteValue(Key, Component);
        }
    }

    // 3. Create the object message using the reqired properties and our created components.
    return mcs::ObjectMessage { Id, static_cast<uint64_t>(Type), IsTransferable, IsPersistent, OwnerId, Convert(ParentId),
        ComponentPacker.GetComponents() };
}

mcs::ObjectPatch SpaceEntity::CreateObjectPatch()
{
    MCSComponentPacker ComponentPacker;

    // 1. Convert our modified view components to mcs compatible types.
    {
        // Get dirty property keys.
        std::unique_ptr<common::Array<uint16_t>> Keys(const_cast<common::Array<uint16_t>*>(DirtyProperties.Keys()));

        // Loop through modfied view components and convert to ItemComponentData.
        for (uint16_t Key : *Keys)
        {
            ComponentPacker.WriteValue(Key, DirtyProperties[Key]);
        }
    }

    // 2. Convert all of our runtime components to mcs compatible types.
    {
        // Get component keys.
        std::unique_ptr<common::Array<uint16_t>> Keys(const_cast<common::Array<uint16_t>*>(DirtyComponents.Keys()));

        // Loop through all components and convert to ItemComponentData.
        for (uint16_t Key : *Keys)
        {
            assert(DirtyComponents[Key].Component != nullptr && "DirtyComponent given a null component!");

            if (DirtyComponents[Key].Component != nullptr)
            {
                auto* Component = DirtyComponents[Key].Component;
                ComponentPacker.WriteValue(Key, Component);
            }
        }
    }

    // 3. Handle any component deletions
    ComponentBase DeletionComponent(ComponentType::Delete, const_cast<SpaceEntity*>(this));

    for (size_t i = 0; i < TransientDeletionComponentIds.Size(); ++i)
    {
        DeletionComponent.Id = TransientDeletionComponentIds[i];
        ComponentPacker.WriteValue(DeletionComponent.Id, &DeletionComponent);
    }

    // 4. Create the object patch using the required properties and our created components.
    return mcs::ObjectPatch { Id, OwnerId, false, ShouldUpdateParent, Convert(ParentId), ComponentPacker.GetComponents() };
}

void SpaceEntity::FromObjectMessage(const mcs::ObjectMessage& Message)
{
    Id = Message.GetId();
    Type = static_cast<SpaceEntityType>(Message.GetType());
    IsTransferable = Message.GetIsTransferable();
    IsPersistent = Message.GetIsPersistent();
    OwnerId = Message.GetOwnerId();
    ParentId = common::Convert(Message.GetParentId());

    auto MessageComponents = Message.GetComponents();

    if (MessageComponents.has_value())
    {
        // Get view components
        MCSComponentUnpacker ComponentUnpacker { *Message.GetComponents() };

        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_ENTITYNAME, Name);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_POSITION, Transform.Position);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_ROTATION, Transform.Rotation);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_SCALE, Transform.Scale);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, SelectedId);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, ThirdPartyPlatform);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_THIRDPARTYREF, ThirdPartyRef);
        ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_LOCKTYPE, EntityLock);

        for (const auto& ComponentDataPair : *MessageComponents)
        {
            if (ComponentDataPair.first >= COMPONENT_KEY_END_COMPONENTS)
            {
                // This is the end of our components
                break;
            }

            ComponentFromItemComponentData(ComponentDataPair.first, ComponentDataPair.second);
        }
    }
}

void SpaceEntity::FromObjectPatch(const mcs::ObjectPatch& Patch)
{
    OwnerId = Patch.GetOwnerId();
    ShouldUpdateParent = Patch.GetShouldUpdateParent();
    ParentId = common::Convert(Patch.GetParentId());

    SpaceEntityUpdateFlags UpdateFlags = SpaceEntityUpdateFlags(0);
    csp::common::Array<ComponentUpdateInfo> ComponentUpdates(0);

    auto PatchComponents = Patch.GetComponents();

    if (PatchComponents.has_value())
    {
        MCSComponentUnpacker ComponentUnpacker { *Patch.GetComponents() };

        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_ENTITYNAME, Name))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_NAME);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_POSITION, Transform.Position))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_POSITION);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_ROTATION, Transform.Rotation))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_ROTATION);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_SCALE, Transform.Scale))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_SCALE);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, SelectedId))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_SELECTION_ID);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, ThirdPartyPlatform))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_PLATFORM);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_THIRDPARTYREF, ThirdPartyRef))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_REF);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_LOCKTYPE, EntityLock))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_LOCK_TYPE);
        }

        uint64_t ComponentCount = ComponentUnpacker.GetRuntimeComponentsCount();

        ComponentUpdates = csp::common::Array<ComponentUpdateInfo>(ComponentCount);
        size_t ComponentIndex = 0;

        for (const auto& ComponentDataPair : *PatchComponents)
        {
            if (ComponentDataPair.first >= COMPONENT_KEY_END_COMPONENTS)
            {
                // This is the end of our components
                break;
            }

            ComponentUpdateInfo UpdateInfo = ComponentFromItemComponentDataPatch(ComponentDataPair.first, ComponentDataPair.second);
            ComponentUpdates[ComponentIndex] = UpdateInfo;
            ComponentIndex++;
        }
    }

    if (ShouldUpdateParent)
    {
        EntitySystem->ResolveEntityHierarchy(this);
        UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_PARENT);
        ShouldUpdateParent = false;
    }

    if (UpdateFlags != 0 && EntityUpdateCallback != nullptr)
    {
        EntityUpdateCallback(this, UpdateFlags, ComponentUpdates);
    }
}

void SpaceEntity::ComponentFromItemComponentData(uint16_t ComponentId, const mcs::ItemComponentData& ComponentData)
{
    auto ComponentDataMap = std::get<std::map<uint16_t, mcs::ItemComponentData>>(ComponentData.GetValue());
    ComponentType MessageComponentType = static_cast<ComponentType>(std::get<uint64_t>(ComponentDataMap[COMPONENT_KEY_COMPONENTTYPE].GetValue()));

    if (MessageComponentType != ComponentType::Invalid)
    {
        auto* Component = InstantiateComponent(ComponentId, MessageComponentType);

        // if Component == nullptr component has not been instantiated, so is skipped.
        if (Component != nullptr)
        {
            for (const auto& PatchComponentPair : ComponentDataMap)
            {
                if (PatchComponentPair.first == COMPONENT_KEY_COMPONENTTYPE)
                {
                    // We don't store the type inside our component properties
                    continue;
                }

                ReplicatedValue Property;
                MCSComponentUnpacker::CreateReplicatedValueFromType(PatchComponentPair.second, Property);

                Component->Properties[PatchComponentPair.first] = Property;
                Component->OnCreated();
            }

            Components[ComponentId] = Component;
        }
    }
}

ComponentUpdateInfo SpaceEntity::ComponentFromItemComponentDataPatch(uint16_t ComponentId, const mcs::ItemComponentData& ComponentData)
{
    auto ComponentDataMap = std::get<std::map<uint16_t, mcs::ItemComponentData>>(ComponentData.GetValue());
    ComponentType PatchComponentType = static_cast<ComponentType>(std::get<uint64_t>(ComponentDataMap[COMPONENT_KEY_COMPONENTTYPE].GetValue()));

    auto UpdateType = ComponentUpdateType::Update;

    if (!Components.HasKey(ComponentId))
    {
        UpdateType = ComponentUpdateType::Add;
    }
    else if (Components[ComponentId]->GetComponentType() != PatchComponentType)
    {
        UpdateType = ComponentUpdateType::Delete;
    }

    switch (UpdateType)
    {
    case ComponentUpdateType::Update:
    {
        auto* Component = Components[ComponentId];

        for (const auto& PatchComponentPair : ComponentDataMap)
        {
            if (PatchComponentPair.first == COMPONENT_KEY_COMPONENTTYPE)
            {
                // We don't store the type inside our component properties
                continue;
            }

            ReplicatedValue Property;
            MCSComponentUnpacker::CreateReplicatedValueFromType(PatchComponentPair.second, Property);

            Component->SetPropertyFromPatch(PatchComponentPair.first, Property);
        }

        break;
    }
    case ComponentUpdateType::Add:
    {
        auto* Component = InstantiateComponent(ComponentId, PatchComponentType);
        // if Component != nullptr component has not been Instantiate, so is skipped.
        if (Component != nullptr)
        {
            for (const auto& PatchComponentPair : ComponentDataMap)
            {
                if (PatchComponentPair.first == COMPONENT_KEY_COMPONENTTYPE)
                {
                    // We don't store the type inside our component properties
                    continue;
                }

                ReplicatedValue Property;
                MCSComponentUnpacker::CreateReplicatedValueFromType(PatchComponentPair.second, Property);

                Component->SetPropertyFromPatch(PatchComponentPair.first, Property);
            }

            Components[ComponentId] = Component;
            Component->OnCreated();
        }
        break;
    }
    case ComponentUpdateType::Delete:
        DestroyComponent(ComponentId);
        break;
    default:
        assert(false && "Unknown component update type!");
        break;
    }

    ComponentUpdateInfo UpdateInfo;
    UpdateInfo.ComponentId = ComponentId;
    UpdateInfo.UpdateType = UpdateType;
    return UpdateInfo;
}

csp::multiplayer::EntityScriptInterface* SpaceEntity::GetScriptInterface() { return ScriptInterface.get(); }

void SpaceEntity::ClaimScriptOwnership() { EntitySystem->ClaimScriptOwnership(this); }

void SpaceEntity::OnPropertyChanged(ComponentBase* DirtyComponent, int32_t PropertyKey)
{
    Script.OnPropertyChanged(DirtyComponent->GetId(), PropertyKey);
}

} // namespace csp::multiplayer
