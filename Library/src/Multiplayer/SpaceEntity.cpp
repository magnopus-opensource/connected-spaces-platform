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
#include "Debug/Logging.h"
#include "Memory/Memory.h"
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
    , IsPersistant(true)
    , OwnerId(0)
    , ParentId(nullptr)
    , ShouldUpdateParent(false)
    , Transform { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } }
    , ThirdPartyPlatform(csp::systems::EThirdPartyPlatform::NONE)
    , ThirdPartyRef("")
    , SelectedId(0)
    , Parent(nullptr)
    , NextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , Script(CSP_NEW EntityScript(this, nullptr))
    , ScriptInterface(CSP_NEW EntityScriptInterface(this))
    , EntityLock(CSP_NEW std::mutex)
    , ComponentsLock(CSP_NEW std::mutex)
    , PropertiesLock(CSP_NEW std::mutex)
    , RefCount(CSP_NEW std::atomic_int(0))
    , TimeOfLastPatch(0)
{
}

SpaceEntity::SpaceEntity(SpaceEntitySystem* InEntitySystem)
    : EntitySystem(InEntitySystem)
    , Type(SpaceEntityType::Avatar)
    , Id(0)
    , IsTransferable(true)
    , IsPersistant(true)
    , OwnerId(0)
    , ParentId(nullptr)
    , ShouldUpdateParent(false)
    , Transform { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } }
    , ThirdPartyPlatform(csp::systems::EThirdPartyPlatform::NONE)
    , ThirdPartyRef("")
    , SelectedId(0)
    , Parent(nullptr)
    , NextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , Script(CSP_NEW EntityScript(this, InEntitySystem))
    , ScriptInterface(CSP_NEW EntityScriptInterface(this))
    , EntityLock(CSP_NEW std::mutex)
    , ComponentsLock(CSP_NEW std::mutex)
    , PropertiesLock(CSP_NEW std::mutex)
    , RefCount(CSP_NEW std::atomic_int(0))
    , TimeOfLastPatch(0)
{
}

SpaceEntity::~SpaceEntity()
{
    auto& Keys = *Components.Keys();

    auto i = 0;
    for (i = 0; i < Keys.Size(); ++i)
    {
        CSP_DELETE(Components[Keys[i]]);
    }

    CSP_DELETE(Script);
    CSP_DELETE(ScriptInterface);

    CSP_DELETE(EntityLock);
    CSP_DELETE(ComponentsLock);
    CSP_DELETE(PropertiesLock);
    CSP_DELETE(RefCount);
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

    std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);

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

    std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);

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

    std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);

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

    std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_SCALE);

    if (Transform.Scale != Value)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_SCALE] = Value;
    }
}

bool SpaceEntity::GetIsTransient() const { return !IsPersistant; }

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

    std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);

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

    std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);

    DirtyProperties.Remove(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM);

    if (ThirdPartyPlatform != InThirdPartyPlatformType)
    {
        DirtyProperties[COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM] = ReplicatedValue(static_cast<int64_t>(InThirdPartyPlatformType));
    }
}

const csp::systems::EThirdPartyPlatform SpaceEntity::GetThirdPartyPlatformType() const { return ThirdPartyPlatform; }

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

ComponentBase* SpaceEntity::AddComponent(ComponentType Type)
{
    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

    if (Type == ComponentType::ScriptData)
    {
        ComponentBase* ScriptComponent = FindFirstComponentOfType(ComponentType::ScriptData, true);

        if (ScriptComponent)
        {
            CSP_LOG_MSG(csp::systems::LogLevel::Warning, "AddComponent: Script Component already exists on this entity.");

            // Return the existing script component
            return nullptr;
        }
    }

    auto ComponentId = GenerateComponentId();
    auto* Component = InstantiateComponent(ComponentId, Type);

    // If Component is null, component has not been instantiated, so is skipped.
    if (Component != nullptr)
    {
        DirtyComponents[ComponentId] = DirtyComponent { Component, ComponentUpdateType::Add };
    }

    return Component;
}

void SpaceEntity::RemoveComponent(uint16_t Key)
{
    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

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

void SpaceEntity::SerialisePatch(IEntitySerialiser& Serialiser) const
{
    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

    Serialiser.BeginEntity();
    {
        /*
         * class EntityMessagePatch
         * [0] uint Id
         * [1] uint OwnerId
         * [2] bool Destroy
         * [3] [bool, uint?] ParentId    ((true, id) to set new parent, (true, null) to remove parent, (false, null) to not change parent)
         * [4] map<uint, vec> Components
         */
        Serialiser.WriteUInt64(Id);
        Serialiser.WriteUInt64(OwnerId);
        Serialiser.WriteBool(false); // Destroy
        Serialiser.BeginArray(); // ParentId
        {
            Serialiser.WriteBool(ShouldUpdateParent);
            ParentId.HasValue() ? Serialiser.WriteUInt64(*ParentId) : Serialiser.WriteNull();
        }
        Serialiser.EndArray();

        Serialiser.BeginComponents();
        {
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_ENTITYNAME))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_ENTITYNAME, DirtyProperties[COMPONENT_KEY_VIEW_ENTITYNAME].GetString());
            }
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_POSITION))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_POSITION, DirtyProperties[COMPONENT_KEY_VIEW_POSITION].GetVector3());
            }
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_ROTATION))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_ROTATION, DirtyProperties[COMPONENT_KEY_VIEW_ROTATION].GetVector4());
            }
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_SCALE))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_SCALE, DirtyProperties[COMPONENT_KEY_VIEW_SCALE].GetVector3());
            }
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_SELECTEDCLIENTID))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, DirtyProperties[COMPONENT_KEY_VIEW_SELECTEDCLIENTID].GetInt());
            }
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_THIRDPARTYREF))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYREF, DirtyProperties[COMPONENT_KEY_VIEW_THIRDPARTYREF].GetString());
            }
            if (DirtyProperties.HasKey(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM))
            {
                Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, DirtyProperties[COMPONENT_KEY_VIEW_THIRDPARTYREF].GetInt());
            }

            assert(DirtyComponents.Size() < COMPONENT_KEY_END_COMPONENTS - COMPONENT_KEY_START_COMPONENTS);

            const csp::common::Array<uint16_t>& DirtyComponentKeys = *DirtyComponents.Keys();

            int i = 0;
            for (i = 0; i < DirtyComponentKeys.Size(); ++i)
            {
                if (DirtyComponents[DirtyComponentKeys[i]].Component != nullptr)
                {
                    auto* Component = DirtyComponents[DirtyComponentKeys[i]].Component;

                    SerialiseComponent(Serialiser, Component);
                }
                else
                {
                    assert(DirtyComponents[DirtyComponentKeys[i]].Component != nullptr && "DirtyComponent given a null component!");
                }
            }

            ComponentBase DeletionComponent(ComponentType::Invalid, const_cast<SpaceEntity*>(this));

            for (i = 0; i < TransientDeletionComponentIds.Size(); ++i)
            {
                DeletionComponent.Id = TransientDeletionComponentIds[i];
                SerialiseComponent(Serialiser, &DeletionComponent);
            }

            CSP_DELETE(&DirtyComponentKeys);
        }
        Serialiser.EndComponents();
    }
    Serialiser.EndEntity();
}

void SpaceEntity::Serialise(IEntitySerialiser& Serialiser)
{
    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

    Serialiser.BeginEntity();
    {
        Serialiser.WriteUInt64(Id);
        Serialiser.WriteUInt64((uint64_t)Type); // PrefabId
        Serialiser.WriteBool(IsTransferable); // IsTransferable
        Serialiser.WriteBool(IsPersistant);
        Serialiser.WriteUInt64(OwnerId);
        ParentId.HasValue() ? Serialiser.WriteUInt64(*ParentId) : Serialiser.WriteNull(); // ParentId

        Serialiser.BeginComponents();
        {
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_ENTITYNAME, Name);
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_POSITION, Transform.Position);
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_ROTATION, Transform.Rotation);
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_SCALE, Transform.Scale);
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, static_cast<int64_t>(SelectedId));
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, static_cast<int64_t>(ThirdPartyPlatform));
            Serialiser.AddViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYREF, ThirdPartyRef);

            assert(DirtyComponents.Size() < COMPONENT_KEY_END_COMPONENTS - COMPONENT_KEY_START_COMPONENTS);

            const csp::common::Array<uint16_t>& DirtyComponentKeys = *DirtyComponents.Keys();

            for (int i = 0; i < DirtyComponentKeys.Size(); ++i)
            {
                auto* Component = DirtyComponents[DirtyComponentKeys[i]].Component;

                SerialiseComponent(Serialiser, Component);
            }

            CSP_DELETE(&DirtyComponentKeys);
        }
        Serialiser.EndComponents();
    }
    Serialiser.EndEntity();
}

void SpaceEntity::Deserialise(IEntityDeserialiser& Deserialiser)
{
    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

    Deserialiser.EnterEntity();
    {
        Id = Deserialiser.ReadUInt64();
        Type = (SpaceEntityType)Deserialiser.ReadUInt64();
        IsTransferable = Deserialiser.ReadBool();
        IsPersistant = Deserialiser.ReadBool();
        OwnerId = Deserialiser.ReadUInt64();

        if (Deserialiser.NextValueIsNull())
        {
            ParentId = nullptr;
            Deserialiser.Skip();
        }
        else
        {
            ParentId = Deserialiser.ReadUInt64();
        }

        assert(Id > 0);

        Deserialiser.EnterComponents();
        {
            [[maybe_unused]] auto ComponentCount = Deserialiser.GetNumComponents();
            auto RealComponentCount = Deserialiser.GetNumRealComponents();

            assert(ComponentCount >= 3 && "SpaceObject should have at least 4 components!");
            // assert(RealComponentCount >= 1 && "SpaceObject should have at least 1 non-view component!");

            uint16_t ComponentId;
            uint64_t _ComponentType;

            while (RealComponentCount--)
            {
                Deserialiser.EnterComponent(ComponentId, _ComponentType);
                {
                    auto Type = (ComponentType)_ComponentType;

                    if (Type != ComponentType::Invalid)
                    {
                        auto* Component = InstantiateComponent(ComponentId, Type);

                        // if Component == nullptr component has not been instantiated, so is skipped.
                        if (Component != nullptr)
                        {
                            Components[ComponentId] = Component;

                            for (int i = 0; i < Deserialiser.GetNumProperties(); ++i)
                            {
                                uint64_t PropertyKey;
                                auto PropertyValue = Deserialiser.ReadProperty(PropertyKey);

                                uint32_t Key = CheckedUInt64ToUint32(PropertyKey);
                                Component->Properties[Key] = PropertyValue;
                            }
                        }
                    }
                }
                Deserialiser.LeaveComponent();
            }

            Name = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_ENTITYNAME).GetString();
            Transform.Position = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_POSITION).GetVector3();
            Transform.Rotation = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_ROTATION).GetVector4();
            Transform.Scale = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_SCALE).GetVector3();

            const ReplicatedValue SelectedIdValue = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_SELECTEDCLIENTID);

            if (SelectedIdValue.GetReplicatedValueType() != ReplicatedValueType::InvalidType)
            {
                SelectedId = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_SELECTEDCLIENTID).GetInt();
            }

            // Space entities created with versions of Connected Spaces Platform prior to 3.16 do not have these properties
            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM))
            {
                auto Value = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM).GetInt();
                ThirdPartyPlatform = static_cast<csp::systems::EThirdPartyPlatform>(Value);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYREF))
            {
                ThirdPartyRef = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYREF).GetString();
            }
        }
        Deserialiser.LeaveComponents();
    }
    Deserialiser.LeaveEntity();
}

void SpaceEntity::DeserialiseFromPatch(IEntityDeserialiser& Deserialiser)
{
    SpaceEntityUpdateFlags UpdateFlags = SpaceEntityUpdateFlags(0);

    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

    csp::common::Array<ComponentUpdateInfo> ComponentUpdates(0);

    if (!Deserialiser.NextValueIsNull()) // It is valid for entities to not have components
    {
        Deserialiser.EnterComponents();
        {
            auto RealComponentCount = Deserialiser.GetNumRealComponents();
            ComponentUpdates = csp::common::Array<ComponentUpdateInfo>(RealComponentCount);

            uint16_t ComponentKey;
            uint64_t _ComponentType;

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_ENTITYNAME))
            {
                Name = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_ENTITYNAME).GetString();
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_NAME);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_POSITION))
            {
                Transform.Position = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_POSITION).GetVector3();
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_POSITION);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_ROTATION))
            {
                Transform.Rotation = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_ROTATION).GetVector4();
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_ROTATION);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_SCALE))
            {
                Transform.Scale = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_SCALE).GetVector3();
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_SCALE);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_SELECTEDCLIENTID))
            {
                SelectedId = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_SELECTEDCLIENTID).GetInt();
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_SELECTION_ID);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYREF))
            {
                ThirdPartyRef = Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYREF).GetString();
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_REF);
            }

            if (Deserialiser.HasViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM))
            {
                ThirdPartyPlatform
                    = static_cast<csp::systems::EThirdPartyPlatform>(Deserialiser.GetViewComponent(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM).GetInt());
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_PLATFORM);
            }

            if (RealComponentCount > 0)
            {
                UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_COMPONENTS);

                while (RealComponentCount--)
                {
                    Deserialiser.EnterComponent(ComponentKey, _ComponentType);
                    {
                        auto UpdateType = ComponentUpdateType::Update;

                        if (!Components.HasKey(ComponentKey))
                        {
                            UpdateType = ComponentUpdateType::Add;
                        }
                        else if (Components[ComponentKey]->GetComponentType() != (ComponentType)_ComponentType)
                        {
                            UpdateType = ComponentUpdateType::Delete;
                        }

                        ComponentUpdates[RealComponentCount].ComponentId = ComponentKey;
                        ComponentUpdates[RealComponentCount].UpdateType = UpdateType;

                        switch (UpdateType)
                        {
                        case ComponentUpdateType::Update:
                        {
                            auto* Component = Components[ComponentKey];

                            for (int i = 0; i < Deserialiser.GetNumProperties(); ++i)
                            {
                                uint64_t PropertyKey;
                                auto PropertyValue = Deserialiser.ReadProperty(PropertyKey);

                                uint32_t Key = CheckedUInt64ToUint32(PropertyKey);
                                Component->SetPropertyFromPatch(Key, PropertyValue);
                            }

                            break;
                        }
                        case ComponentUpdateType::Add:
                        {
                            auto* Component = InstantiateComponent(ComponentKey, (ComponentType)_ComponentType);
                            // if Component != nullptr component has not been Instantiate, so is skipped.
                            if (Component != nullptr)
                            {
                                for (int i = 0; i < Deserialiser.GetNumProperties(); ++i)
                                {
                                    uint64_t PropertyKey;
                                    auto PropertyValue = Deserialiser.ReadProperty(PropertyKey);

                                    uint32_t Key = CheckedUInt64ToUint32(PropertyKey);
                                    Component->SetPropertyFromPatch(Key, PropertyValue);
                                }

                                Components[ComponentKey] = Component;
                            }
                            break;
                        }
                        case ComponentUpdateType::Delete:
                            DestroyComponent(ComponentKey);
                            break;
                        default:
                            assert(false && "Unknown component update type!");
                            break;
                        }
                    }
                    Deserialiser.LeaveComponent();
                }
            }
        }
        Deserialiser.LeaveComponents();
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

void SpaceEntity::ApplyLocalPatch(bool InvokeUpdateCallback)
{
    /// If we're sending patches to ourselves, don't apply local patches, as we'll be directly deserialising the data instead.
    if (!csp::systems::SystemsManager::Get().GetMultiplayerConnection()->GetAllowSelfMessagingFlag())
    {
        std::scoped_lock<std::mutex> PropertiesLocker(*PropertiesLock);
        std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

        auto UpdateFlags = static_cast<SpaceEntityUpdateFlags>(0);

        const csp::common::Array<uint16_t>* DirtyComponentKeys = DirtyComponents.Keys();

        // Allocate a ComponentUpdates array (to pass update info to the client), with
        // sufficient size for all dirty components and scheduled deletions.
        csp::common::Array<ComponentUpdateInfo> ComponentUpdates(DirtyComponentKeys->Size() + TransientDeletionComponentIds.Size());

        if (DirtyComponentKeys->Size() > 0)
        {
            UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_COMPONENTS);

            for (int i = 0; i < DirtyComponentKeys->Size(); ++i)
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

                    for (int j = 0; j < DirtyComponentPropertyKeys->Size(); j++)
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

        CSP_DELETE(DirtyComponentKeys);

        if (DirtyProperties.Size() > 0)
        {
            const csp::common::Array<uint16_t>* DirtyViewKeys = DirtyProperties.Keys();

            for (int i = 0; i < DirtyViewKeys->Size(); ++i)
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
                default:
                    break;
                }
            }

            DirtyProperties.Clear();
            CSP_DELETE(DirtyViewKeys);
        }

        if (TransientDeletionComponentIds.Size() > 0)
        {
            DirtyComponentKeys = DirtyComponents.Keys();

            for (int i = 0; i < TransientDeletionComponentIds.Size(); ++i)
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
            CSP_DELETE(DirtyComponentKeys);
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

uint16_t SpaceEntity::GenerateComponentId()
{
    auto Id = NextComponentId;

    for (;;)
    {
        if (!Components.HasKey(Id) && !DirtyComponents.HasKey(Id))
        {
            NextComponentId = Id + 1;

            return Id;
        }

        ++Id;

        if (Id == COMPONENT_KEY_END_COMPONENTS)
        {
            Id = COMPONENT_KEY_START_COMPONENTS;
        }
    }
}

ComponentBase* SpaceEntity::InstantiateComponent(uint16_t Id, ComponentType Type)
{
    ComponentBase* Component;

    switch (Type)
    {
    case ComponentType::StaticModel:
        Component = CSP_NEW StaticModelSpaceComponent(this);
        break;
    case ComponentType::AnimatedModel:
        Component = CSP_NEW AnimatedModelSpaceComponent(this);
        break;
    case ComponentType::VideoPlayer:
        Component = CSP_NEW VideoPlayerSpaceComponent(this);
        break;
    case ComponentType::Image:
        Component = CSP_NEW ImageSpaceComponent(this);
        break;
    case ComponentType::ExternalLink:
        Component = CSP_NEW ExternalLinkSpaceComponent(this);
        break;
    case ComponentType::AvatarData:
        Component = CSP_NEW AvatarSpaceComponent(this);
        break;
    case ComponentType::Light:
        Component = CSP_NEW LightSpaceComponent(this);
        break;
    case ComponentType::ScriptData:
        Component = CSP_NEW ScriptSpaceComponent(this);
        break;
    case ComponentType::Button:
        Component = CSP_NEW ButtonSpaceComponent(this);
        break;
    case ComponentType::Custom:
        Component = CSP_NEW CustomSpaceComponent(this);
        break;
    case ComponentType::Portal:
        Component = CSP_NEW PortalSpaceComponent(this);
        break;
    case ComponentType::Conversation:
        Component = CSP_NEW ConversationSpaceComponent(this);
        break;
    case ComponentType::Audio:
        Component = CSP_NEW AudioSpaceComponent(this);
        break;
    case ComponentType::Spline:
        Component = CSP_NEW SplineSpaceComponent(this);
        break;
    case ComponentType::Collision:
        Component = CSP_NEW CollisionSpaceComponent(this);
        break;
    case ComponentType::Reflection:
        Component = CSP_NEW ReflectionSpaceComponent(this);
        break;
    case ComponentType::Fog:
        Component = CSP_NEW FogSpaceComponent(this);
        break;
    case ComponentType::ECommerce:
        Component = CSP_NEW ECommerceSpaceComponent(this);
        break;
    case ComponentType::CinematicCamera:
        Component = CSP_NEW CinematicCameraSpaceComponent(this);
        break;
    case ComponentType::FiducialMarker:
        Component = CSP_NEW FiducialMarkerSpaceComponent(this);
        break;
    case ComponentType::GaussianSplat:
        Component = CSP_NEW GaussianSplatSpaceComponent(this);
        break;
    case ComponentType::Text:
        Component = CSP_NEW TextSpaceComponent(this);
        break;
    case ComponentType::Hotspot:
        Component = CSP_NEW HotspotSpaceComponent(this);
        break;
    default:
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Warning, csp::common::StringFormat("Unknown Component type of value: %d", static_cast<uint32_t>(Type)));
        return nullptr;
    }
    }

    Component->Id = Id;

    return Component;
}

void SpaceEntity::SerialiseComponent(IEntitySerialiser& Serialiser, ComponentBase* Component) const
{
    Serialiser.BeginComponent(Component->Id, (uint64_t)Component->Type);
    {
        auto& Properties = Component->Properties;
        const auto& Keys = Properties.Keys();

        for (int j = 0; j < Keys->Size(); ++j)
        {
            auto& Key = Keys->operator[](j);
            auto& Property = Properties[Key];

            Serialiser.WriteProperty(Key, Property);
        }
    }
    Serialiser.EndComponent();
}

void SpaceEntity::AddDirtyComponent(ComponentBase* Component)
{
    std::scoped_lock<std::mutex> ComponentsLocker(*ComponentsLock);

    if (DirtyComponents.HasKey(Component->GetId()))
    {
        return;
    }

    DirtyComponents[Component->GetId()] = DirtyComponent { Component, ComponentUpdateType::Update };
}

EntityScript* SpaceEntity::GetScript() { return Script; }

bool SpaceEntity::IsSelected() const { return SelectedId != 0; }

uint64_t SpaceEntity::GetSelectingClientID() const { return SelectedId; }

bool SpaceEntity::Select()
{
    std::scoped_lock EntitiesLocker(*EntityLock);
    return EntitySystem->SetSelectionStateOfEntity(true, this);
}

bool SpaceEntity::Deselect()
{
    std::scoped_lock EntitiesLocker(*EntityLock);
    return EntitySystem->SetSelectionStateOfEntity(false, this);
}

bool SpaceEntity::IsModifiable()
{
    if (EntitySystem != nullptr && csp::systems::SystemsManager::Get().GetMultiplayerConnection() != nullptr)
    {
        return (OwnerId == csp::systems::SystemsManager::Get().GetMultiplayerConnection()->GetClientId() || IsTransferable);
    }
    else
    {
        return true;
    }
}

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

ComponentBase* SpaceEntity::FindFirstComponentOfType(ComponentType Type, bool SearchDirtyComponents) const
{
    const csp::common::Array<uint16_t>* ComponentKeys = Components.Keys();
    ComponentBase* LocatedComponent = nullptr;

    for (int i = 0; i < ComponentKeys->Size(); ++i)
    {
        ComponentBase* Component = Components[ComponentKeys->operator[](i)];

        if (Component->GetComponentType() == Type)
        {
            LocatedComponent = Component;
            break;
        }
    }

    CSP_DELETE(ComponentKeys);

    if (LocatedComponent == nullptr && SearchDirtyComponents)
    {
        const csp::common::Array<uint16_t>* DirtyComponentKeys = DirtyComponents.Keys();

        for (int i = 0; i < DirtyComponentKeys->Size(); ++i)
        {
            const DirtyComponent& Component = DirtyComponents[DirtyComponentKeys->operator[](i)];

            if (Component.UpdateType != ComponentUpdateType::Delete && Component.Component->GetComponentType() == Type)
            {
                LocatedComponent = Component.Component;
                break;
            }
        }

        CSP_DELETE(DirtyComponentKeys);
    }

    return LocatedComponent;
}

void SpaceEntity::AddChildEntitiy(SpaceEntity* ChildEntity) { ChildEntities.Append(ChildEntity); }

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
            for (const auto& PendingParent : *EntitySystem->PendingAdds)
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

csp::multiplayer::EntityScriptInterface* SpaceEntity::GetScriptInterface() { return ScriptInterface; }

void SpaceEntity::ClaimScriptOwnership() { EntitySystem->ClaimScriptOwnership(this); }

void SpaceEntity::OnPropertyChanged(ComponentBase* DirtyComponent, int32_t PropertyKey)
{
    Script->OnPropertyChanged(DirtyComponent->GetId(), PropertyKey);
}

void SpaceEntity::AddRef() { ++(*RefCount); }

void SpaceEntity::RemoveRef() { --(*RefCount); }

std::atomic_int* SpaceEntity::GetRefCount() { return RefCount; }

} // namespace csp::multiplayer
