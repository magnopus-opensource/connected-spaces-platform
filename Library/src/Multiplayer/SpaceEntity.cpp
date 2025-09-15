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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
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
#include "CSP/Multiplayer/Components/ScreenSharingSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/MCSComponentPacker.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "Multiplayer/Script/EntityScriptInterface.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "Multiplayer/SpaceEntityStatePatcher.h"
#include "RealtimeEngineUtils.h"
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
    , Position { csp::common::Vector3 { 0, 0, 0 } }
    , Rotation { csp::common::Vector4 { 0, 0, 0, 1 } }
    , Scale { csp::common::Vector3 { 1, 1, 1 } }
    , ThirdPartyPlatform(static_cast<int64_t>(csp::systems::EThirdPartyPlatform::NONE))
    , ThirdPartyRef("")
    , SelectedId(0ll)
    , Parent(nullptr)
    , EntityLock(static_cast<int64_t>(LockType::None))
    , NextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , Script(this, nullptr, nullptr, nullptr)
    , ScriptInterface(std::make_unique<EntityScriptInterface>(this))
    , LogSystem(nullptr)
    , StatePatcher(nullptr)
{
    RegisterProperties();
}

SpaceEntity::SpaceEntity(csp::common::IRealtimeEngine* InEntitySystem, csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem* LogSystem)
    : EntitySystem(InEntitySystem)
    , Type(SpaceEntityType::Avatar)
    , Id(0)
    , IsTransferable(true)
    , IsPersistent(true)
    , OwnerId(0)
    , ParentId(nullptr)
    , Position { csp::common::Vector3 { 0, 0, 0 } }
    , Rotation { csp::common::Vector4 { 0, 0, 0, 1 } }
    , Scale { csp::common::Vector3 { 1, 1, 1 } }
    , ThirdPartyPlatform(static_cast<int64_t>(csp::systems::EThirdPartyPlatform::NONE))
    , ThirdPartyRef("")
    , SelectedId(0ll)
    , Parent(nullptr)
    , EntityLock(static_cast<int64_t>(LockType::None))
    , NextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , Script(this, InEntitySystem, &ScriptRunner, LogSystem)
    , ScriptInterface(std::make_unique<EntityScriptInterface>(this))
    , LogSystem(LogSystem)
    , StatePatcher(nullptr)
{
    RegisterProperties();

    if (EntitySystem == nullptr)
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Warning, "Constructing a SpaceEntity with a null EntitySystem. This is generally inadvisable");
        }
    }
    else
    {
        // This is how we branch between doing deferred patch logic or just direct sets. The engine tells us if it uses a patch model or not, by
        // either returning a patcher, or not.
        StatePatcher = std::unique_ptr<SpaceEntityStatePatcher>(EntitySystem->MakeStatePatcher(*this));
    }
}

SpaceEntity::SpaceEntity(csp::common::IRealtimeEngine* EntitySystem, csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem* LogSystem,
    SpaceEntityType Type, uint64_t Id, const csp::common::String& Name, const csp::multiplayer::SpaceTransform& Transform, uint64_t OwnerId,
    csp::common::Optional<uint64_t> ParentId, bool IsTransferable, bool IsPersistent)
    : SpaceEntity(EntitySystem, ScriptRunner, LogSystem)
{
    RegisterProperties();

    this->Id = Id;
    this->Type = Type;
    this->Name = Name;
    this->Position = Transform.Position;
    this->Rotation = Transform.Rotation;
    this->Scale = Transform.Scale;
    this->OwnerId = OwnerId;
    this->IsTransferable = IsTransferable;
    this->IsPersistent = IsPersistent;
    this->ParentId = ParentId;
}

SpaceEntity::~SpaceEntity() { }

uint64_t SpaceEntity::GetId() const { return Id; }

uint64_t SpaceEntity::GetOwnerId() const { return OwnerId; }

const csp::common::String& SpaceEntity::GetName() const { return Name.GetString(); }

bool SpaceEntity::SetName(const csp::common::String& Value) { return SetProperty(COMPONENT_KEY_VIEW_ENTITYNAME, GetName(), Value); }

SpaceTransform SpaceEntity::GetTransform() const { return SpaceTransform { Position.GetVector3(), Rotation.GetVector4(), Scale.GetVector3() }; }

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
    return GetTransform();
}

const csp::common::Vector3& SpaceEntity::GetPosition() const { return Position.GetVector3(); }

csp::common::Vector3 SpaceEntity::GetGlobalPosition() const
{
    const csp::common::Vector3 Pos = GetPosition();

    if (Parent != nullptr)
    {
        const glm::mat4 ParentTransform = computeParentMat4(Parent->GetGlobalTransform());
        const glm::vec3 GlobalEntityPosition = ParentTransform * glm::vec4(Pos.X, Pos.Y, Pos.Z, 1.0f);

        return { GlobalEntityPosition.x, GlobalEntityPosition.y, GlobalEntityPosition.z };
    }
    else
        return Pos;
}

bool SpaceEntity::SetPosition(const csp::common::Vector3& Value) { return SetProperty(COMPONENT_KEY_VIEW_POSITION, GetPosition(), Value); }

const csp::common::Vector4& SpaceEntity::GetRotation() const { return Rotation.GetVector4(); }

csp::common::Vector4 SpaceEntity::GetGlobalRotation() const
{
    const csp::common::Vector4 Rot = GetRotation();

    if (Parent != nullptr)
    {
        const csp::common::Vector4 GlobalRotation = Parent->GetGlobalRotation();
        const glm::quat Orientation { Rot.W, Rot.X, Rot.Y, Rot.Z };
        const glm::quat GlobalOrientation(GlobalRotation.W, GlobalRotation.X, GlobalRotation.Y, GlobalRotation.Z);

        const glm::quat FinalOrientation = GlobalOrientation * Orientation;
        return csp::common::Vector4 { FinalOrientation.x, FinalOrientation.y, FinalOrientation.z, FinalOrientation.w };
    }
    else
        return Rot;
}

bool SpaceEntity::SetRotation(const csp::common::Vector4& Value) { return SetProperty(COMPONENT_KEY_VIEW_ROTATION, GetRotation(), Value); }

const csp::common::Vector3& SpaceEntity::GetScale() const { return Scale.GetDefaultVector3(); }

csp::common::Vector3 SpaceEntity::GetGlobalScale() const
{
    const csp::common::Vector3 Sc = GetScale();

    if (Parent != nullptr)
        return Parent->GetGlobalScale() * Sc;
    return Sc;
}

bool SpaceEntity::SetScale(const csp::common::Vector3& Value) { return SetProperty(COMPONENT_KEY_VIEW_SCALE, GetScale(), Value); }

bool SpaceEntity::GetIsTransient() const { return !IsPersistent; }

const csp::common::String& SpaceEntity::GetThirdPartyRef() const { return ThirdPartyRef.GetString(); }

bool SpaceEntity::SetThirdPartyRef(const csp::common::String& InThirdPartyRef)
{
    return SetProperty(COMPONENT_KEY_VIEW_THIRDPARTYREF, GetThirdPartyRef(), InThirdPartyRef);
}

bool SpaceEntity::SetThirdPartyPlatformType(const csp::systems::EThirdPartyPlatform InThirdPartyPlatformType)
{
    return SetProperty(
        COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, static_cast<int64_t>(GetThirdPartyPlatformType()), static_cast<int64_t>(InThirdPartyPlatformType));
}

csp::systems::EThirdPartyPlatform SpaceEntity::GetThirdPartyPlatformType() const
{
    return static_cast<csp::systems::EThirdPartyPlatform>(ThirdPartyPlatform.GetInt());
}

SpaceEntityType SpaceEntity::GetEntityType() const { return Type; }

void SpaceEntity::SetParentId(uint64_t InParentId)
{
    // No lock, parents may always be changed. (Seems odd)
    return StatePatcher != nullptr ? StatePatcher->SetNewParentId(InParentId) : SetParentIdDirect(InParentId, true);
}

void SpaceEntity::RemoveParentEntity()
{
    if (ParentId.HasValue())
    {
        // Need to send a full optional containing an empty optional to mean "There's a new ID, but it's to no-parent"
        StatePatcher != nullptr ? StatePatcher->SetNewParentId(csp::common::Optional<uint64_t> { csp::common::Optional<uint64_t> {} })
                                : SetParentIdDirect({}, true);
    }
}

csp::common::Optional<uint64_t> SpaceEntity::GetParentId() const { return ParentId; }

bool SpaceEntity::GetIsTransferable() const { return IsTransferable; }

bool SpaceEntity::GetIsPersistent() const { return IsPersistent; }

SpaceEntity* SpaceEntity::GetParentEntity() const { return Parent; }

void SpaceEntity::CreateChildEntity(const csp::common::String& InName, const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback)
{
    EntitySystem->CreateEntity(InName, InSpaceTransform, GetId(), Callback);
}

const csp::common::List<SpaceEntity*>* SpaceEntity::GetChildEntities() const { return &ChildEntities; }

void SpaceEntity::Destroy(CallbackHandler Callback)
{
    if (EntitySystem)
    {
        EntitySystem->DestroyEntity(this, Callback);
    }
}

void SpaceEntity::SetUpdateCallback(UpdateCallback Callback) { EntityUpdateCallback = Callback; }

void SpaceEntity::SetDestroyCallback(DestroyCallback Callback) { EntityDestroyCallback = Callback; }

void SpaceEntity::SetPatchSentCallback(CallbackHandler Callback)
{
    if (StatePatcher == nullptr)
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Warning,
                "Attempting to register a patch callback to a SpaceEntity without a StatePatcher. This is an operation that should only be performed "
                "in an online context. Taking no action.");
        }
        return;
    }

    StatePatcher->SetPatchSentCallback(Callback);
}

const csp::common::Map<uint16_t, ComponentBase*>* SpaceEntity::GetComponents() const { return &Components; }

ComponentBase* SpaceEntity::GetComponent(uint16_t Key)
{
    std::scoped_lock ScopedComponentsLock { ComponentsLock };
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
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "Entity is locked. New components can not be added to a locked Entity.");
        }

        return nullptr;
    }

    std::scoped_lock ScopedComponentsLock { ComponentsLock };

    // Only allow one script component
    if (AddType == ComponentType::ScriptData)
    {
        ComponentBase* ScriptComponent = FindFirstComponentOfType(ComponentType::ScriptData);

        if (ScriptComponent)
        {
            if (LogSystem != nullptr)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "AddComponent: Script Component already exists on this entity.");
            }

            // Return the existing script component
            return ScriptComponent;
        }
    }

    auto ComponentId = GenerateComponentId();
    auto* Component = InstantiateComponent(ComponentId, AddType);

    // If Component is null, component has not been instantiated, so is skipped. (Can this ever be null... seems a bit of a footgun not to just
    // assert)
    if (Component != nullptr)
    {
        // Either add the component to the patch, or just directly insert it.
        StatePatcher != nullptr
            ? StatePatcher->SetDirtyComponent(ComponentId, SpaceEntityStatePatcher::DirtyComponent { Component, ComponentUpdateType::Add })
            : AddComponentDirect(ComponentId, Component, true);

        Component->OnCreated();
    }

    return Component;
}

bool SpaceEntity::UpdateComponent(ComponentBase* Component)
{
    // You'd think there would be an IsModifiable check here, but component property updates are notification only as far as SpaceEntity is concerned,
    // as the data is set directly on components. This is done in ComponentBase.h instead. Seems confused and inconsistent to me.

    return StatePatcher != nullptr
        ? StatePatcher->SetDirtyComponent(Component->GetId(), SpaceEntityStatePatcher::DirtyComponent { Component, ComponentUpdateType::Update })
        : UpdateComponentDirect(Component->GetId(), Component, true);
}

bool SpaceEntity::RemoveComponent(uint16_t Key)
{
    if (!IsModifiable())
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "Entity is locked. Components can not be removed from a locked Entity.");
        }

        return false;
    }

    return StatePatcher != nullptr ? StatePatcher->RemoveDirtyComponent(Key, *GetComponents()) : RemoveComponentDirect(Key, true);
}

void SpaceEntity::RemoveAsChildFromParent()
{
    // Pretty naff way to do this, entity heirarchy must be fragile. Wouldn't be hard to redesign and make solid.
    if (Parent != nullptr)
    {
        Parent->ChildEntities.RemoveItem(this);
        Parent = nullptr;
    }
}

void SpaceEntity::RemoveParentFromChildEntity(size_t Index)
{
    if (Index < ChildEntities.Size())
    {
        ChildEntities[Index]->RemoveParentEntity();
        ChildEntities[Index]->Parent = nullptr;
    }
}

void SpaceEntity::RemoveParentId() { ParentId = nullptr; }

void SpaceEntity::ApplyLocalPatch(bool InvokeUpdateCallback, bool AllowSelfMessaging)
{
    if (StatePatcher == nullptr)
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Warning,
                "Attempting to apply a patch to a SpaceEntity without a StatePatcher. This is an operation that should only be performed in "
                "an online context. Taking no action.");
        }
        return;
    }

    /// If we're sending patches to ourselves, don't apply local patches, as we'll be directly deserialising the data instead.
    if (!AllowSelfMessaging)
    {
        auto [UpdateFlags, ComponentUpdates] = StatePatcher->ApplyLocalPatch();

        if (InvokeUpdateCallback && EntityUpdateCallback != nullptr)
        {
            EntityUpdateCallback(this, UpdateFlags, ComponentUpdates);
        }
    }
}

SpaceEntity::UpdateCallback SpaceEntity::GetEntityUpdateCallback() { return EntityUpdateCallback; }

SpaceEntity::DestroyCallback SpaceEntity::GetEntityDestroyCallback() { return EntityDestroyCallback; }

SpaceEntity* SpaceEntity::GetParent() { return Parent; }

void SpaceEntity::SetParent(SpaceEntity* InParent) { Parent = InParent; }

uint16_t SpaceEntity::GenerateComponentId()
{
    auto NextId = NextComponentId;

    // We want to also account for dirty components. If we're in a context that has them (online, patching), account for them, otherwise we can just
    // use an empty container (ie, ignore it)
    const auto& DirtyComponents
        = StatePatcher != nullptr ? StatePatcher->GetDirtyComponents() : std::unordered_map<uint16_t, SpaceEntityStatePatcher::DirtyComponent> {};

    for (;;)
    {
        if (!Components.HasKey(NextId) && !(DirtyComponents.count(NextId) > 0))
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
        Component = new StaticModelSpaceComponent(LogSystem, this);
        break;
    case ComponentType::AnimatedModel:
        Component = new AnimatedModelSpaceComponent(LogSystem, this);
        break;
    case ComponentType::VideoPlayer:
        Component = new VideoPlayerSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Image:
        Component = new ImageSpaceComponent(LogSystem, this);
        break;
    case ComponentType::ExternalLink:
        Component = new ExternalLinkSpaceComponent(LogSystem, this);
        break;
    case ComponentType::AvatarData:
        Component = new AvatarSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Light:
        Component = new LightSpaceComponent(LogSystem, this);
        break;
    case ComponentType::ScriptData:
        Component = new ScriptSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Button:
        Component = new ButtonSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Custom:
        Component = new CustomSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Portal:
        Component = new PortalSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Conversation:
        Component = new ConversationSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Audio:
        Component = new AudioSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Spline:
        Component = new SplineSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Collision:
        Component = new CollisionSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Reflection:
        Component = new ReflectionSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Fog:
        Component = new FogSpaceComponent(LogSystem, this);
        break;
    case ComponentType::ECommerce:
        Component = new ECommerceSpaceComponent(LogSystem, this);
        break;
    case ComponentType::CinematicCamera:
        Component = new CinematicCameraSpaceComponent(LogSystem, this);
        break;
    case ComponentType::FiducialMarker:
        Component = new FiducialMarkerSpaceComponent(LogSystem, this);
        break;
    case ComponentType::GaussianSplat:
        Component = new GaussianSplatSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Text:
        Component = new TextSpaceComponent(LogSystem, this);
        break;
    case ComponentType::Hotspot:
        Component = new HotspotSpaceComponent(LogSystem, this);
        break;
    case ComponentType::ScreenSharing:
        Component = new ScreenSharingSpaceComponent(LogSystem, this);
        break;
    default:
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(
                csp::common::LogLevel::Warning, fmt::format("Unknown Component type of value: {}", static_cast<uint32_t>(InstantiateType)).c_str());
        }
        return nullptr;
    }
    }

    Component->Id = InstantiateId;

    return Component;
}

EntityScript& SpaceEntity::GetScript() { return Script; }

bool SpaceEntity::IsSelected() const { return SelectedId.GetInt() != 0; }

uint64_t SpaceEntity::GetSelectingClientID() const { return SelectedId.GetInt(); }

bool SpaceEntity::Select()
{
    std::scoped_lock EntitiesLocker(EntityMutexLock);
    return InternalSetSelectionStateOfEntity(true);
}

bool SpaceEntity::Deselect()
{
    std::scoped_lock EntitiesLocker(EntityMutexLock);
    return InternalSetSelectionStateOfEntity(false);
}

bool SpaceEntity::IsModifiable() const
{
    if (EntitySystem == nullptr)
    {
        // Return true here so entities that arent attached to the entity system can be modified.
        // This is currently used for testing.
        return true;
    }

    if (EntitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Offline)
    {
        if (GetLockType() == LockType::UserAgnostic)
        {
            return false;
        }

        return true;
    }

    auto* OnlineRealtimeEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(EntitySystem);

    // This should definately be true at this point, but be defensive
    if (StatePatcher == nullptr)
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Fatal, "Unexpected lack of StatePatcher in online context, SpaceEntity::IsModifiable()");
        }
        return false;
    }

    // In the case where we are about to unlock a locked entity we want to treat it as if it's unlocked so we can modify it,
    // so we skip the lock check they are about to unlock.
    // We know they are going to unlock if EntityLock is set and they have COMPONENT_KEY_VIEW_LOCKTYPE in DirtyProperties
    // Note : This will stop working if we ever add another lock type
    const bool AboutToUnlock = StatePatcher->GetDirtyProperties().count(COMPONENT_KEY_VIEW_LOCKTYPE) > 0;
    if (GetLockType() == LockType::UserAgnostic && !AboutToUnlock)
    {
        return false;
    }

    return (OwnerId == OnlineRealtimeEngine->GetMultiplayerConnectionInstance()->GetClientId() || IsTransferable);
}

bool SpaceEntity::Lock()
{
    if (IsLocked())
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "Entity is already locked.");
        }
        return false;
    }

    return SetProperty(COMPONENT_KEY_VIEW_LOCKTYPE, EntityLock.GetInt(), static_cast<int64_t>(LockType::UserAgnostic));
}

bool SpaceEntity::Unlock()
{
    if (IsLocked() == false)
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "Entity is not currently locked.");
        }
        return false;
    }

    // For unlocking, we dont want to call SetProperty, as it does an internal IsModifiable check,
    // which will always fail due to the entity being locked.
    if (StatePatcher != nullptr)
    {
        return StatePatcher->SetDirtyProperty(COMPONENT_KEY_VIEW_LOCKTYPE, EntityLock.GetInt(), static_cast<int64_t>(LockType::None));
    }
    else
    {
        SetPropertyDirect(COMPONENT_KEY_VIEW_LOCKTYPE, static_cast<int64_t>(LockType::None));
        return true;
    }
}

csp::multiplayer::LockType SpaceEntity::GetLockType() const { return static_cast<LockType>(EntityLock.GetInt()); }

bool SpaceEntity::IsLocked() const { return GetLockType() != LockType::None; }

void SpaceEntity::QueueUpdate()
{
    if (EntitySystem)
    {
        if (EntitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        {
            static_cast<csp::multiplayer::OnlineRealtimeEngine*>(EntitySystem)->QueueEntityUpdate(this);
        }
    }
}

bool SpaceEntity::InternalSetSelectionStateOfEntity(const bool SelectedState)
{
    uint64_t LocalClientId = (EntitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        ? static_cast<csp::multiplayer::OnlineRealtimeEngine*>(EntitySystem)->GetMultiplayerConnectionInstance()->GetClientId()
        : csp::multiplayer::OfflineRealtimeEngine::LocalClientId();

    if (SelectedState)
    {
        if (!IsSelected())
        {
            // Set a pending selection property
            if (StatePatcher != nullptr)
            {
                // Weird! Needs to be an int rather than a uint.
                StatePatcher->SetDirtyProperty(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, GetSelectingClientID(), static_cast<int64_t>(LocalClientId));
            }

            bool Added = EntitySystem->AddEntityToSelectedEntities(this);
            if (Added)
            {
                SetPropertyDirect(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, static_cast<int64_t>(LocalClientId), true);
                return true;
            }
        }
        return false;
    }

    if (LocalClientId == GetSelectingClientID())
    {
        if (IsSelected())
        {
            // Set a pending selection property (deselection I'm guessing, being zero)
            if (StatePatcher != nullptr)
            {
                // Weird! Needs to be an int rather than a uint.
                StatePatcher->SetDirtyProperty(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, GetSelectingClientID(), static_cast<int64_t>(0));
            }

            bool Removed = EntitySystem->RemoveEntityFromSelectedEntities(this);
            if (Removed)
            {
                SetPropertyDirect(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, 0ll, true);
                return true;
            }
        }

        return false;
    }

    return false;
}

std::chrono::milliseconds SpaceEntity::GetTimeOfLastPatch() { return StatePatcher != nullptr ? StatePatcher->GetTimeOfLastPatch() : 0ms; }

// Not dirtyable because it is a mandatory type in ObjectMessage. It's sent no matter what.
// It may still be nicer to make this in-pattern and do loopbacks like the dirtyable props, but no formal need.
void SpaceEntity::SetOwnerId(uint64_t InOwnerId) { OwnerId = InOwnerId; }

void SpaceEntity::SetParentIdDirect(csp::common::Optional<uint64_t> Value, bool CallNotifyingCallback)
{
    ParentId = Value;
    EntitySystem->ResolveEntityHierarchy(this);

    if (EntityUpdateCallback && CallNotifyingCallback)
    {
        csp::common::Array<ComponentUpdateInfo> Empty;
        EntityUpdateCallback(this, UPDATE_FLAGS_PARENT, Empty);
    }
}

void SpaceEntity::SetPropertyDirect(uint16_t PropertyKey, const csp::common::ReplicatedValue& Value, bool CallNotifyingCallback)
{
    auto It = PropertyMap.find(PropertyKey);

    if (It == PropertyMap.end())
    {
        // TODO: Error
        return;
    }

    *It->second.Value = Value;

    if (CallNotifyingCallback && EntityUpdateCallback)
    {
        csp::common::Array<ComponentUpdateInfo> Empty;
        EntityUpdateCallback(this, It->second.UpdateFlag, Empty);
    }
}

const std::map<uint16_t, EntityProperty>& SpaceEntity::GetProperties() const { return PropertyMap; }

bool SpaceEntity::AddComponentDirect(uint16_t ComponentKey, ComponentBase* Component, bool CallNotifyingCallback)
{
    std::scoped_lock ComponentsLocker(ComponentsLock);
    Components[ComponentKey] = Component;

    if (CallNotifyingCallback && EntityUpdateCallback)
    {
        csp::common::Array<ComponentUpdateInfo> UpdateInfo(1);
        UpdateInfo[0] = ComponentUpdateInfo { ComponentKey, ComponentUpdateType::Add };
        EntityUpdateCallback(this, UPDATE_FLAGS_COMPONENTS, UpdateInfo);
    }

    return true;
}

bool SpaceEntity::UpdateComponentDirect(uint16_t ComponentKey, ComponentBase* Component, bool CallNotifyingCallback)
{
    std::scoped_lock ComponentsLocker(ComponentsLock); // Still lock, friendly to the pattern, and to scripting updates

    // You're probably wondering why this sets no data? It's a bit of an unfortunate break from the pattern this one. Think of this more as a
    // "NotifySomethingAboutComponentHasChanged", as the actual property updates happen on the component itself.
    if (CallNotifyingCallback)
    {
        if (EntityUpdateCallback)
        {
            csp::common::Array<ComponentUpdateInfo> UpdateInfo(1);
            UpdateInfo[0] = ComponentUpdateInfo { ComponentKey, ComponentUpdateType::Update };
            EntityUpdateCallback(this, UPDATE_FLAGS_COMPONENTS, UpdateInfo);
        }

        // Todo, rename so it is known this is about script properties
        OnPropertyChanged(Component, ComponentKey);
    }

    return true;
}

bool SpaceEntity::RemoveComponentDirect(uint16_t ComponentKey, bool CallNotifyingCallback)
{
    std::scoped_lock ComponentsLocker(ComponentsLock);

    if (Components.HasKey(ComponentKey))
    {
        Components[ComponentKey]->OnRemove();
        Components.Remove(ComponentKey);

        if (CallNotifyingCallback && EntityUpdateCallback)
        {
            csp::common::Array<ComponentUpdateInfo> UpdateInfo(1);
            UpdateInfo[0] = ComponentUpdateInfo { ComponentKey, ComponentUpdateType::Delete };
            EntityUpdateCallback(this, UPDATE_FLAGS_COMPONENTS, UpdateInfo);
        }
        return true;
    }
    else
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "DestroyComponent: Key Does Not Exist");
        }
        return false;
    }
}

ComponentBase* SpaceEntity::FindFirstComponentOfType(ComponentType FindType) const
{
    ComponentBase* LocatedComponent = nullptr;

    for (const std::pair<const uint16_t, ComponentBase*>& Component : Components.GetUnderlying())
    {

        if (Component.second->GetComponentType() == FindType)
        {
            LocatedComponent = Component.second;
            break;
        }
    }

    // If we're in an online context, we also want to check any pending (dirty) components
    if (LocatedComponent == nullptr && StatePatcher != nullptr)
    {
        // Not interested in deletes, as they're going away anyhow.
        LocatedComponent = StatePatcher->GetFirstPendingComponentOfType(FindType, { ComponentUpdateType::Add, ComponentUpdateType::Update });
    }

    return LocatedComponent;
}

void SpaceEntity::AddChildEntity(SpaceEntity* ChildEntity) { ChildEntities.Append(ChildEntity); }

void SpaceEntity::RegisterProperties()
{
    PropertyMap[COMPONENT_KEY_VIEW_ENTITYNAME] = EntityProperty { this->Name, UPDATE_FLAGS_NAME };
    PropertyMap[COMPONENT_KEY_VIEW_POSITION] = EntityProperty { this->Position, UPDATE_FLAGS_POSITION };
    PropertyMap[COMPONENT_KEY_VIEW_ROTATION] = EntityProperty { this->Rotation, UPDATE_FLAGS_ROTATION };
    PropertyMap[COMPONENT_KEY_VIEW_SCALE] = EntityProperty { this->Scale, UPDATE_FLAGS_SCALE };
    PropertyMap[COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM] = EntityProperty { this->ThirdPartyPlatform, UPDATE_FLAGS_THIRD_PARTY_PLATFORM };
    PropertyMap[COMPONENT_KEY_VIEW_THIRDPARTYREF] = EntityProperty { this->ThirdPartyRef, UPDATE_FLAGS_THIRD_PARTY_REF };
    PropertyMap[COMPONENT_KEY_VIEW_SELECTEDCLIENTID] = EntityProperty { this->SelectedId, UPDATE_FLAGS_SELECTION_ID };
    PropertyMap[COMPONENT_KEY_VIEW_LOCKTYPE] = EntityProperty { this->EntityLock, UPDATE_FLAGS_LOCK_TYPE };
}

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
        Parent = EntitySystem->FindSpaceEntityById(*ParentId);

        if (EntitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        {
            if (Parent == nullptr)
            {
                // For the OnlineRealtimeSystem, it's possible for parents to exist within the PendingAdds array,
                // so we also need to check here.
                auto* OnlineRealtimeEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(EntitySystem);

                // Parent may not have been added yet
                // so check pending entities
                for (const auto& PendingParent : *OnlineRealtimeEngine->GetPendingAdds())
                {
                    if (PendingParent->Id == *ParentId)
                    {
                        Parent = PendingParent;
                        break;
                    }
                }
            }
        }

        if (Parent != nullptr)
        {
            Parent->ChildEntities.Append(this);
        }
        else
        {
            if (LogSystem != nullptr)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Error,
                    fmt::format("SpaceEntity unable to find parent for entity: {}. Please report if this issue is encountered.", GetId()).c_str());
            }
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

const std::unique_ptr<SpaceEntityStatePatcher>& SpaceEntity::GetStatePatcher() { return StatePatcher; }

void SpaceEntity::AddComponentFromItemComponentData(uint16_t ComponentId, const mcs::ItemComponentData& ComponentData)
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

                csp::common::ReplicatedValue Property;
                MCSComponentUnpacker::CreateReplicatedValueFromType(PatchComponentPair.second, Property);

                Component->Properties[PatchComponentPair.first] = Property;
                Component->OnCreated();
            }
            std::scoped_lock ComponentsLocker(ComponentsLock);
            Components[ComponentId] = Component;
        }
    }
}

ComponentUpdateInfo SpaceEntity::AddComponentFromItemComponentDataPatch(uint16_t ComponentId, const mcs::ItemComponentData& ComponentData)
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

    std::scoped_lock ComponentsLocker(ComponentsLock);

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

            csp::common::ReplicatedValue Property;
            MCSComponentUnpacker::CreateReplicatedValueFromType(PatchComponentPair.second, Property);

            // UpdateComponentDirect(false);
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

                csp::common::ReplicatedValue Property;
                MCSComponentUnpacker::CreateReplicatedValueFromType(PatchComponentPair.second, Property);

                // UpdateComponentDirect(false);?
                Component->SetPropertyFromPatch(PatchComponentPair.first, Property);
            }

            AddComponentDirect(ComponentId, Component, false);
            Component->OnCreated();
        }
        break;
    }
    case ComponentUpdateType::Delete:
        RemoveComponentDirect(ComponentId);
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

void SpaceEntity::ClaimScriptOwnership()
{
    if (EntitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
    {
        auto* OnlineRealtimeEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(EntitySystem);
        OnlineRealtimeEngine->ClaimScriptOwnership(this);
    }
}

void SpaceEntity::OnPropertyChanged(ComponentBase* DirtyComponent, int32_t PropertyKey)
{
    Script.OnPropertyChanged(DirtyComponent->GetId(), PropertyKey);
}

} // namespace csp::multiplayer
