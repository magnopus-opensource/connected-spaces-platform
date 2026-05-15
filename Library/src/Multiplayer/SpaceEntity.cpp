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
#include "CSP/Multiplayer/Components/AIChatbotComponent.h"
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
#include "Multiplayer/PatchUtils.h"
#include "Multiplayer/RealtimeEngineUtils.h"
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

glm::mat4 computeParentMat4(const SpaceTransform& parentSpaceTransform)
{
    glm::mat4 parentTranslate = glm::translate(
        glm::mat4(1.0f), glm::vec3(parentSpaceTransform.Position.X, parentSpaceTransform.Position.Y, parentSpaceTransform.Position.Z));
    glm::quat parentOrientation { parentSpaceTransform.Rotation.W, parentSpaceTransform.Rotation.X, parentSpaceTransform.Rotation.Y,
        parentSpaceTransform.Rotation.Z };

    glm::mat4 parentRotation(parentOrientation);
    glm::mat4 parentScale
        = glm::scale(glm::mat4(1.0f), glm::vec3(parentSpaceTransform.Scale.X, parentSpaceTransform.Scale.Y, parentSpaceTransform.Scale.Z));
    glm::mat4 parentTransform = parentTranslate * parentRotation * parentScale;
    return parentTransform;
}

inline uint32_t CheckedUInt64ToUint32(uint64_t value)
{
    assert(value <= UINT32_MAX);
    uint32_t valueAsUInt32 = static_cast<uint32_t>(value);
    return valueAsUInt32;
}

SpaceEntity::SpaceEntity()
    : m_entitySystem(nullptr)
    , m_type(SpaceEntityType::Avatar)
    , m_id(0)
    , m_isTransferable(true)
    , m_isPersistent(true)
    , m_ownerId(0)
    , m_parentId(nullptr)
    , m_transform { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } }
    , m_thirdPartyRef("")
    , m_selectedId(0)
    , m_parent(nullptr)
    , m_entityLock(LockType::None)
    , m_nextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , m_script(this, nullptr, nullptr, nullptr)
    , m_scriptInterface(std::make_unique<EntityScriptInterface>(this))
    , m_logSystem(nullptr)
    , m_statePatcher(nullptr)
{
}

SpaceEntity::SpaceEntity(csp::common::IRealtimeEngine* inEntitySystem, csp::common::IJSScriptRunner& scriptRunner, csp::common::LogSystem* logSystem)
    : m_entitySystem(inEntitySystem)
    , m_type(SpaceEntityType::Avatar)
    , m_id(0)
    , m_isTransferable(true)
    , m_isPersistent(true)
    , m_ownerId(0)
    , m_parentId(nullptr)
    , m_transform { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } }
    , m_thirdPartyRef("")
    , m_selectedId(0)
    , m_parent(nullptr)
    , m_entityLock(LockType::None)
    , m_nextComponentId(COMPONENT_KEY_START_COMPONENTS)
    , m_script(this, inEntitySystem, &scriptRunner, logSystem)
    , m_scriptInterface(std::make_unique<EntityScriptInterface>(this))
    , m_logSystem(logSystem)
    , m_statePatcher(nullptr)
{
    if (m_entitySystem == nullptr)
    {
        if (logSystem)
        {
            logSystem->LogMsg(csp::common::LogLevel::Warning, "Constructing a SpaceEntity with a null EntitySystem. This is generally inadvisable");
        }
    }
    else
    {
        // This is how we branch between doing deferred patch logic or just direct sets. The engine tells us if it uses a patch model or not, by
        // either returning a patcher, or not.
        m_statePatcher = std::unique_ptr<SpaceEntityStatePatcher>(m_entitySystem->MakeStatePatcher(*this));

        if (m_statePatcher)
        {
            m_statePatcher->RegisterProperties(CreateReplicatedProperties());
        }
    }
}

SpaceEntity::SpaceEntity(csp::common::IRealtimeEngine* entitySystem, csp::common::IJSScriptRunner& scriptRunner, csp::common::LogSystem* logSystem,
    SpaceEntityType type, uint64_t id, const csp::common::String& name, const csp::multiplayer::SpaceTransform& transform, uint64_t ownerId,
    csp::common::Optional<uint64_t> parentId, bool isTransferable, bool isPersistent)
    : SpaceEntity(entitySystem, scriptRunner, logSystem)
{
    this->m_id = id;
    this->m_type = type;
    this->m_name = name;
    this->m_transform = transform;
    this->m_ownerId = ownerId;
    this->m_isTransferable = isTransferable;
    this->m_isPersistent = isPersistent;
    this->m_parentId = parentId;
}

SpaceEntity::~SpaceEntity() { }

uint64_t SpaceEntity::GetId() const { return m_id; }

uint64_t SpaceEntity::GetOwnerId() const { return m_ownerId; }

const csp::common::String& SpaceEntity::GetName() const { return m_name; }

bool SpaceEntity::SetName(const csp::common::String& value)
{
    return SetProperty(*this, m_name, value, SpaceEntityComponentKey::Name, UPDATE_FLAGS_NAME, m_logSystem);
}

const SpaceTransform& SpaceEntity::GetTransform() const { return m_transform; }

SpaceTransform SpaceEntity::GetGlobalTransform() const
{
    if (m_parent != nullptr)
    {
        SpaceTransform globalTransform;
        globalTransform.Position = GetGlobalPosition();
        globalTransform.Rotation = GetGlobalRotation();
        globalTransform.Scale = GetGlobalScale();
        return globalTransform;
    }
    return m_transform;
}

const csp::common::Vector3& SpaceEntity::GetPosition() const { return m_transform.Position; }

csp::common::Vector3 SpaceEntity::GetGlobalPosition() const
{
    if (m_parent != nullptr)
    {
        glm::mat4 parentTransform = computeParentMat4(m_parent->GetGlobalTransform());

        glm::vec3 globalEntityPosition = parentTransform * glm::vec4(m_transform.Position.X, m_transform.Position.Y, m_transform.Position.Z, 1.0f);

        return { globalEntityPosition.x, globalEntityPosition.y, globalEntityPosition.z };
    }
    else
        return m_transform.Position;
}

bool SpaceEntity::SetPosition(const csp::common::Vector3& value)
{
    return SetProperty(*this, m_transform.Position, value, SpaceEntityComponentKey::Position, UPDATE_FLAGS_POSITION, m_logSystem);
}

const csp::common::Vector4& SpaceEntity::GetRotation() const { return m_transform.Rotation; }

csp::common::Vector4 SpaceEntity::GetGlobalRotation() const
{
    if (m_parent != nullptr)
    {
        csp::common::Vector4 globalRotation = m_parent->GetGlobalRotation();
        glm::quat orientation { m_transform.Rotation.W, m_transform.Rotation.X, m_transform.Rotation.Y, m_transform.Rotation.Z };
        glm::quat globalOrientation(globalRotation.W, globalRotation.X, globalRotation.Y, globalRotation.Z);

        glm::quat finalOrientation = globalOrientation * orientation;
        return csp::common::Vector4 { finalOrientation.x, finalOrientation.y, finalOrientation.z, finalOrientation.w };
    }
    else
        return m_transform.Rotation;
}

bool SpaceEntity::SetRotation(const csp::common::Vector4& value)
{
    return SetProperty(*this, m_transform.Rotation, value, SpaceEntityComponentKey::Rotation, UPDATE_FLAGS_ROTATION, m_logSystem);
}

const csp::common::Vector3& SpaceEntity::GetScale() const { return m_transform.Scale; }

csp::common::Vector3 SpaceEntity::GetGlobalScale() const
{
    if (m_parent != nullptr)
        return m_parent->GetGlobalScale() * m_transform.Scale;
    return m_transform.Scale;
}

bool SpaceEntity::SetScale(const csp::common::Vector3& value)
{
    return SetProperty(*this, m_transform.Scale, value, SpaceEntityComponentKey::Scale, UPDATE_FLAGS_SCALE, m_logSystem);
}

bool SpaceEntity::GetIsTransient() const { return !m_isPersistent; }

const csp::common::String& SpaceEntity::GetThirdPartyRef() const { return m_thirdPartyRef; }

bool SpaceEntity::SetThirdPartyRef(const csp::common::String& inThirdPartyRef)
{
    return SetProperty(*this, m_thirdPartyRef, inThirdPartyRef, SpaceEntityComponentKey::ThirdPartyRef, UPDATE_FLAGS_THIRD_PARTY_REF, m_logSystem);
}

SpaceEntityType SpaceEntity::GetEntityType() const { return m_type; }

void SpaceEntity::SetParentId(uint64_t inParentId)
{
    // No lock, parents may always be changed. (Seems odd)
    return m_statePatcher != nullptr ? m_statePatcher->SetNewParentId(inParentId) : SetParentIdDirect(inParentId, true);
}

void SpaceEntity::RemoveParentEntity()
{
    if (m_parentId.HasValue())
    {
        // Need to send a full optional containing an empty optional to mean "There's a new ID, but it's to no-parent"
        m_statePatcher != nullptr ? m_statePatcher->SetNewParentId(csp::common::Optional<uint64_t> { csp::common::Optional<uint64_t> {} })
                                : SetParentIdDirect({}, true);
    }
}

csp::common::Optional<uint64_t> SpaceEntity::GetParentId() const { return m_parentId; }

bool SpaceEntity::GetIsTransferable() const { return m_isTransferable; }

bool SpaceEntity::GetIsPersistent() const { return m_isPersistent; }

SpaceEntity* SpaceEntity::GetParentEntity() const { return m_parent; }

void SpaceEntity::CreateChildEntity(const csp::common::String& inName, const SpaceTransform& inSpaceTransform, EntityCreatedCallback callback)
{
    m_entitySystem->CreateEntity(inName, inSpaceTransform, GetId(), callback);
}

const csp::common::List<SpaceEntity*>* SpaceEntity::GetChildEntities() const { return &m_childEntities; }

void SpaceEntity::Destroy(CallbackHandler callback)
{
    if (m_entitySystem)
    {
        m_entitySystem->DestroyEntity(this, callback);
    }
}

void SpaceEntity::SetUpdateCallback(UpdateCallback callback) { m_entityUpdateCallback = callback; }

void SpaceEntity::SetDestroyCallback(DestroyCallback callback) { m_entityDestroyCallback = callback; }

void SpaceEntity::SetPatchSentCallback(CallbackHandler callback)
{
    if (m_statePatcher == nullptr)
    {
        if (m_logSystem)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                "Attempting to register a patch callback to a SpaceEntity without a StatePatcher. This is an operation that should only be performed "
                "in an online context. Taking no action.");
        }
        return;
    }

    m_statePatcher->SetPatchSentCallback(callback);
}

const csp::common::Map<uint16_t, ComponentBase*>* SpaceEntity::GetComponents() const { return &m_components; }

ComponentBase* SpaceEntity::GetComponent(uint16_t key)
{
    std::scoped_lock scopedComponentsLock { m_componentsLock };
    if (m_components.HasKey(key))
    {
        return m_components[key];
    }
    else
    {
        return nullptr;
    }
}

ComponentBase* SpaceEntity::AddComponent(ComponentType addType)
{
    // Ensure we can modify the entity. The criteria for this can be found on the specific RealtimeEngine::IsEntityModifiable overloads.
    ModifiableStatus modifiable = IsModifiable();
    if (modifiable != ModifiableStatus::Modifiable)
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Failed to add component: {0}, skipping update. Entity name: {1}",
                    RealtimeEngineUtils::ModifiableStatusToString(modifiable), GetName())
                    .c_str());
        }

        return nullptr;
    }

    std::scoped_lock scopedComponentsLock { m_componentsLock };

    // Only allow one script component
    if (addType == ComponentType::ScriptData)
    {
        ComponentBase* scriptComponent = FindFirstComponentOfType(ComponentType::ScriptData);

        if (scriptComponent)
        {
            if (m_logSystem != nullptr)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Warning, "AddComponent: Script Component already exists on this entity.");
            }

            // Return the existing script component
            return scriptComponent;
        }
    }

    auto componentId = GenerateComponentId();
    auto* component = InstantiateComponent(componentId, addType);

    // If Component is null, component has not been instantiated, so is skipped. (Can this ever be null... seems a bit of a footgun not to just
    // assert)
    if (component != nullptr)
    {
        // Either add the component to the patch, or just directly insert it.
        m_statePatcher != nullptr
            ? m_statePatcher->SetDirtyComponent(componentId, SpaceEntityStatePatcher::DirtyComponent { component, ComponentUpdateType::Add })
            : AddComponentDirect(componentId, component, true);

        component->OnCreated();
    }

    return component;
}

bool SpaceEntity::UpdateComponent(ComponentBase* component)
{
    // You'd think there would be an IsModifiable check here, but component property updates are notification only as far as SpaceEntity is concerned,
    // as the data is set directly on components. This is done in ComponentBase.h instead. Seems confused and inconsistent to me.

    return m_statePatcher != nullptr
        ? m_statePatcher->SetDirtyComponent(component->GetId(), SpaceEntityStatePatcher::DirtyComponent { component, ComponentUpdateType::Update })
        : UpdateComponentDirect(component->GetId(), component, true);
}

bool SpaceEntity::RemoveComponent(uint16_t key)
{
    // Ensure we can modify the entity. The criteria for this can be found on the specific RealtimeEngine::IsEntityModifiable overloads.
    ModifiableStatus modifiable = IsModifiable();
    if (modifiable != ModifiableStatus::Modifiable)
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Failed to remove component: {0}, skipping update. Entity name: {1}",
                    RealtimeEngineUtils::ModifiableStatusToString(modifiable), GetName())
                    .c_str());
        }

        return false;
    }

    return m_statePatcher != nullptr ? m_statePatcher->RemoveDirtyComponent(key, *GetComponents()) : RemoveComponentDirect(key, true);
}

void SpaceEntity::RemoveAsChildFromParent()
{
    // Pretty naff way to do this, entity heirarchy must be fragile. Wouldn't be hard to redesign and make solid.
    if (m_parent != nullptr)
    {
        m_parent->m_childEntities.RemoveItem(this);
        m_parent = nullptr;
    }
}

void SpaceEntity::RemoveParentFromChildEntity(size_t index)
{
    if (index < m_childEntities.Size())
    {
        m_childEntities[index]->RemoveParentEntity();
        m_childEntities[index]->m_parent = nullptr;
    }
}

void SpaceEntity::RemoveParentId() { m_parentId = nullptr; }

void SpaceEntity::ApplyLocalPatch(bool invokeUpdateCallback, bool allowSelfMessaging)
{
    if (m_statePatcher == nullptr)
    {
        if (m_logSystem)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                "Attempting to apply a patch to a SpaceEntity without a StatePatcher. This is an operation that should only be performed in "
                "an online context. Taking no action.");
        }
        return;
    }

    /// If we're sending patches to ourselves, don't apply local patches, as we'll be directly deserialising the data instead.
    if (!allowSelfMessaging)
    {
        auto [UpdateFlags, ComponentUpdates] = m_statePatcher->ApplyLocalPatch();

        if (invokeUpdateCallback && m_entityUpdateCallback != nullptr)
        {
            m_entityUpdateCallback(this, UpdateFlags, ComponentUpdates);
        }
    }
}

SpaceEntity::UpdateCallback SpaceEntity::GetEntityUpdateCallback() { return m_entityUpdateCallback; }

SpaceEntity::DestroyCallback SpaceEntity::GetEntityDestroyCallback() { return m_entityDestroyCallback; }

SpaceEntity* SpaceEntity::GetParent() { return m_parent; }

void SpaceEntity::SetParent(SpaceEntity* inParent) { m_parent = inParent; }

uint16_t SpaceEntity::GenerateComponentId()
{
    auto nextId = m_nextComponentId;

    // We want to also account for dirty components. If we're in a context that has them (online, patching), account for them, otherwise we can just
    // use an empty container (ie, ignore it)
    const auto& dirtyComponents
        = m_statePatcher != nullptr ? m_statePatcher->GetDirtyComponents() : std::unordered_map<uint16_t, SpaceEntityStatePatcher::DirtyComponent> {};

    for (;;)
    {
        if (!m_components.HasKey(nextId) && !(dirtyComponents.count(nextId) > 0))
        {
            m_nextComponentId = nextId + 1;

            return nextId;
        }

        ++nextId;

        if (nextId == COMPONENT_KEY_END_COMPONENTS)
        {
            nextId = COMPONENT_KEY_START_COMPONENTS;
        }
    }
}

ComponentBase* SpaceEntity::InstantiateComponent(uint16_t instantiateId, ComponentType instantiateType)
{
    ComponentBase* component;

    switch (instantiateType)
    {
    case ComponentType::StaticModel:
        component = new StaticModelSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::AnimatedModel:
        component = new AnimatedModelSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::VideoPlayer:
        component = new VideoPlayerSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Image:
        component = new ImageSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::ExternalLink:
        component = new ExternalLinkSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::AvatarData:
        component = new AvatarSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Light:
        component = new LightSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::ScriptData:
        component = new ScriptSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Button:
        component = new ButtonSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Custom:
        component = new CustomSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Portal:
        component = new PortalSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Conversation:
        component = new ConversationSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Audio:
        component = new AudioSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Spline:
        component = new SplineSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Collision:
        component = new CollisionSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Reflection:
        component = new ReflectionSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Fog:
        component = new FogSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::ECommerce:
        component = new ECommerceSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::CinematicCamera:
        component = new CinematicCameraSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::FiducialMarker:
        component = new FiducialMarkerSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::GaussianSplat:
        component = new GaussianSplatSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Text:
        component = new TextSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::Hotspot:
        component = new HotspotSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::ScreenSharing:
        component = new ScreenSharingSpaceComponent(m_logSystem, this);
        break;
    case ComponentType::AIChatbot:
        component = new AIChatbotSpaceComponent(m_logSystem, this);
        break;
    default:
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(
                csp::common::LogLevel::Warning, fmt::format("Unknown Component type of value: {}", static_cast<uint32_t>(instantiateType)).c_str());
        }
        return nullptr;
    }
    }

    component->m_id = instantiateId;

    return component;
}

EntityScript& SpaceEntity::GetScript() { return m_script; }

bool SpaceEntity::IsSelected() const { return m_selectedId != 0; }

uint64_t SpaceEntity::GetSelectingClientID() const { return m_selectedId; }

bool SpaceEntity::Select()
{
    std::scoped_lock entitiesLocker(m_entityMutexLock);
    return InternalSetSelectionStateOfEntity(true);
}

bool SpaceEntity::Deselect()
{
    std::scoped_lock entitiesLocker(m_entityMutexLock);
    return InternalSetSelectionStateOfEntity(false);
}

ModifiableStatus SpaceEntity::IsModifiable() const
{
    if (m_entitySystem == nullptr)
    {
        // Return true here so entities that arent attached to the entity system can be modified.
        // This is currently used for testing.
        return ModifiableStatus::Modifiable;
    }

    return m_entitySystem->IsEntityModifiable(this);
}

bool SpaceEntity::Lock()
{
    if (IsLocked())
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "Entity is already locked.");
        }
        return false;
    }

    return SetProperty(
        *this, m_entityLock, static_cast<int64_t>(LockType::UserAgnostic), SpaceEntityComponentKey::LockType, UPDATE_FLAGS_LOCK_TYPE, m_logSystem);
}

bool SpaceEntity::Unlock()
{
    if (IsLocked() == false)
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "Entity is not currently locked.");
        }
        return false;
    }

    // We don't call "SetProperty" here, because the internal IsModifiable check will always fail due to the entity being locked.
    if (m_statePatcher)
    {
        return m_statePatcher->SetDirtyProperty(SpaceEntityComponentKey::LockType, m_entityLock, static_cast<int64_t>(LockType::None));
    }
    else
    {
        SetPropertyDirect(m_entityLock, LockType::None, UPDATE_FLAGS_LOCK_TYPE, true);
        return true;
    }
}

csp::multiplayer::LockType SpaceEntity::GetLockType() const { return m_entityLock; }

bool SpaceEntity::IsLocked() const { return m_entityLock != LockType::None; }

void SpaceEntity::QueueUpdate()
{
    if (m_entitySystem)
    {
        if (m_entitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        {
            static_cast<csp::multiplayer::OnlineRealtimeEngine*>(m_entitySystem)->QueueEntityUpdate(this);
        }
    }
}

bool SpaceEntity::InternalSetSelectionStateOfEntity(const bool selectedState)
{
    uint64_t localClientId = (m_entitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        ? static_cast<csp::multiplayer::OnlineRealtimeEngine*>(m_entitySystem)->GetMultiplayerConnectionInstance()->GetClientId()
        : csp::multiplayer::OfflineRealtimeEngine::LocalClientId();

    if (selectedState)
    {
        if (!IsSelected())
        {
            // Set a pending selection property
            if (m_statePatcher != nullptr)
            {
                // Weird! Needs to be an int rather than a uint.
                m_statePatcher->SetDirtyProperty(SpaceEntityComponentKey::SelectedClientId, m_selectedId, static_cast<int64_t>(localClientId));
            }

            bool added = m_entitySystem->AddEntityToSelectedEntities(this);
            if (added)
            {
                SetPropertyDirect(m_selectedId, localClientId, UPDATE_FLAGS_SELECTION_ID, true);
                return true;
            }
        }
        return false;
    }

    if (localClientId == GetSelectingClientID())
    {
        if (IsSelected())
        {
            // Set a pending selection property (deselection I'm guessing, being zero)
            if (m_statePatcher != nullptr)
            {
                // Weird! Needs to be an int rather than a uint.
                m_statePatcher->SetDirtyProperty(SpaceEntityComponentKey::SelectedClientId, m_selectedId, static_cast<int64_t>(0));
            }

            bool removed = m_entitySystem->RemoveEntityFromSelectedEntities(this);
            if (removed)
            {
                SetPropertyDirect(m_selectedId, 0, UPDATE_FLAGS_SELECTION_ID, true);
                return true;
            }
        }

        return false;
    }

    return false;
}

std::chrono::milliseconds SpaceEntity::GetTimeOfLastPatch() { return m_statePatcher != nullptr ? m_statePatcher->GetTimeOfLastPatch() : 0ms; }

// Not dirtyable because it is a mandatory type in ObjectMessage. It's sent no matter what.
// It may still be nicer to make this in-pattern and do loopbacks like the dirtyable props, but no formal need.
void SpaceEntity::SetOwnerId(uint64_t inOwnerId) { m_ownerId = inOwnerId; }

void SpaceEntity::SetParentIdDirect(csp::common::Optional<uint64_t> value, bool callNotifyingCallback)
{
    m_parentId = value;
    m_entitySystem->ResolveEntityHierarchy(this);

    if (m_entityUpdateCallback && callNotifyingCallback)
    {
        csp::common::Array<ComponentUpdateInfo> empty;
        m_entityUpdateCallback(this, UPDATE_FLAGS_PARENT, empty);
    }
}

bool SpaceEntity::AddComponentDirect(uint16_t componentKey, ComponentBase* component, bool callNotifyingCallback)
{
    std::scoped_lock componentsLocker(m_componentsLock);
    m_components[componentKey] = component;

    if (callNotifyingCallback && m_entityUpdateCallback)
    {
        csp::common::Array<ComponentUpdateInfo> updateInfo(1);
        updateInfo[0] = ComponentUpdateInfo { componentKey, ComponentUpdateType::Add };
        m_entityUpdateCallback(this, UPDATE_FLAGS_COMPONENTS, updateInfo);
    }

    return true;
}

bool SpaceEntity::UpdateComponentDirect(uint16_t componentKey, ComponentBase* component, bool callNotifyingCallback)
{
    std::scoped_lock componentsLocker(m_componentsLock); // Still lock, friendly to the pattern, and to scripting updates

    // You're probably wondering why this sets no data? It's a bit of an unfortunate break from the pattern this one. Think of this more as a
    // "NotifySomethingAboutComponentHasChanged", as the actual property updates happen on the component itself.
    if (callNotifyingCallback)
    {
        if (m_entityUpdateCallback)
        {
            csp::common::Array<ComponentUpdateInfo> updateInfo(1);
            updateInfo[0] = ComponentUpdateInfo { componentKey, ComponentUpdateType::Update };
            m_entityUpdateCallback(this, UPDATE_FLAGS_COMPONENTS, updateInfo);
        }

        // Todo, rename so it is known this is about script properties
        OnPropertyChanged(component, componentKey);
    }

    return true;
}

bool SpaceEntity::RemoveComponentDirect(uint16_t componentKey, bool callNotifyingCallback)
{
    std::scoped_lock componentsLocker(m_componentsLock);

    if (m_components.HasKey(componentKey))
    {
        m_components[componentKey]->OnRemove();
        m_components.Remove(componentKey);

        if (callNotifyingCallback && m_entityUpdateCallback)
        {
            csp::common::Array<ComponentUpdateInfo> updateInfo(1);
            updateInfo[0] = ComponentUpdateInfo { componentKey, ComponentUpdateType::Delete };
            m_entityUpdateCallback(this, UPDATE_FLAGS_COMPONENTS, updateInfo);
        }
        return true;
    }
    else
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "DestroyComponent: Key Does Not Exist");
        }
        return false;
    }
}

ComponentBase* SpaceEntity::FindFirstComponentOfType(ComponentType findType) const
{
    ComponentBase* locatedComponent = nullptr;

    for (const std::pair<const uint16_t, ComponentBase*>& component : m_components.GetUnderlying())
    {

        if (component.second->GetComponentType() == findType)
        {
            locatedComponent = component.second;
            break;
        }
    }

    // If we're in an online context, we also want to check any pending (dirty) components
    if (locatedComponent == nullptr && m_statePatcher != nullptr)
    {
        // Not interested in deletes, as they're going away anyhow.
        locatedComponent = m_statePatcher->GetFirstPendingComponentOfType(findType, { ComponentUpdateType::Add, ComponentUpdateType::Update });
    }

    return locatedComponent;
}

void SpaceEntity::AddChildEntity(SpaceEntity* childEntity) { m_childEntities.Append(childEntity); }

void SpaceEntity::ResolveParentChildRelationship()
{
    // Entity has been re-parented
    if (m_parentId.HasValue())
    {
        // Entity was previously parented to another object, so cleanup
        if (m_parent != nullptr)
        {
            m_parent->m_childEntities.RemoveItem(this);
        }

        // Set our new parent
        m_parent = m_entitySystem->FindSpaceEntityById(*m_parentId);

        if (m_entitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        {
            if (m_parent == nullptr)
            {
                // For the OnlineRealtimeSystem, it's possible for parents to exist within the PendingAdds array,
                // so we also need to check here.
                auto* onlineRealtimeEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(m_entitySystem);

                // Parent may not have been added yet
                // so check pending entities
                for (const auto& pendingParent : *onlineRealtimeEngine->GetPendingAdds())
                {
                    if (pendingParent->m_id == *m_parentId)
                    {
                        m_parent = pendingParent;
                        break;
                    }
                }
            }
        }

        if (m_parent != nullptr)
        {
            m_parent->m_childEntities.Append(this);
        }
        else
        {
            if (m_logSystem != nullptr)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Error,
                    fmt::format("SpaceEntity unable to find parent for entity: {}. Please report if this issue is encountered.", GetId()).c_str());
            }
            return;
        }
    }
    else
    {
        if (m_parent != nullptr)
        {
            m_parent->m_childEntities.RemoveItem(this);
            m_parent = nullptr;
        }
    }
}

const std::unique_ptr<SpaceEntityStatePatcher>& SpaceEntity::GetStatePatcher() const { return m_statePatcher; }

std::unique_ptr<SpaceEntityStatePatcher>& SpaceEntity::GetStatePatcher() { return m_statePatcher; }

void SpaceEntity::AddComponentFromItemComponentData(uint16_t componentId, const mcs::ItemComponentData& componentData)
{
    auto componentDataMap = std::get<std::map<uint16_t, mcs::ItemComponentData>>(componentData.GetValue());
    ComponentType messageComponentType = static_cast<ComponentType>(std::get<uint64_t>(componentDataMap[COMPONENT_KEY_COMPONENTTYPE].GetValue()));

    if (messageComponentType != ComponentType::Invalid)
    {
        auto* component = InstantiateComponent(componentId, messageComponentType);

        // if Component == nullptr component has not been instantiated, so is skipped.
        if (component != nullptr)
        {
            for (const auto& patchComponentPair : componentDataMap)
            {
                if (patchComponentPair.first == COMPONENT_KEY_COMPONENTTYPE)
                {
                    // We don't store the type inside our component properties
                    continue;
                }

                csp::common::ReplicatedValue property = ToReplicatedValue(patchComponentPair.second);

                component->m_properties[patchComponentPair.first] = property;
                component->OnCreated();
            }
            std::scoped_lock componentsLocker(m_componentsLock);
            m_components[componentId] = component;
        }
    }
}

ComponentUpdateInfo SpaceEntity::AddComponentFromItemComponentDataPatch(uint16_t componentId, const mcs::ItemComponentData& componentData)
{
    auto componentDataMap = std::get<std::map<uint16_t, mcs::ItemComponentData>>(componentData.GetValue());
    ComponentType patchComponentType = static_cast<ComponentType>(std::get<uint64_t>(componentDataMap[COMPONENT_KEY_COMPONENTTYPE].GetValue()));

    auto updateType = ComponentUpdateType::Update;

    if (!m_components.HasKey(componentId))
    {
        updateType = ComponentUpdateType::Add;
    }
    else if (m_components[componentId]->GetComponentType() != patchComponentType)
    {
        updateType = ComponentUpdateType::Delete;
    }

    std::scoped_lock componentsLocker(m_componentsLock);

    switch (updateType)
    {
    case ComponentUpdateType::Update:
    {
        auto* component = m_components[componentId];

        for (const auto& patchComponentPair : componentDataMap)
        {
            if (patchComponentPair.first == COMPONENT_KEY_COMPONENTTYPE)
            {
                // We don't store the type inside our component properties
                continue;
            }

            csp::common::ReplicatedValue property = ToReplicatedValue(patchComponentPair.second);

            // UpdateComponentDirect(false);
            component->SetPropertyFromPatch(patchComponentPair.first, property);
        }

        break;
    }
    case ComponentUpdateType::Add:
    {
        auto* component = InstantiateComponent(componentId, patchComponentType);
        // if Component != nullptr component has not been Instantiate, so is skipped.
        if (component != nullptr)
        {
            for (const auto& patchComponentPair : componentDataMap)
            {
                if (patchComponentPair.first == COMPONENT_KEY_COMPONENTTYPE)
                {
                    // We don't store the type inside our component properties
                    continue;
                }

                csp::common::ReplicatedValue property = ToReplicatedValue(patchComponentPair.second);

                // UpdateComponentDirect(false);?
                component->SetPropertyFromPatch(patchComponentPair.first, property);
            }

            AddComponentDirect(componentId, component, false);
            component->OnCreated();
        }
        break;
    }
    case ComponentUpdateType::Delete:
        RemoveComponentDirect(componentId);
        break;
    default:
        assert(false && "Unknown component update type!");
        break;
    }

    ComponentUpdateInfo updateInfo;
    updateInfo.ComponentId = componentId;
    updateInfo.UpdateType = updateType;
    return updateInfo;
}

csp::common::Array<EntityProperty> SpaceEntity::CreateReplicatedProperties()
{
    /* clang-format off */
    return { 
        { 
            SpaceEntityComponentKey::Name, UPDATE_FLAGS_NAME, 
            [&name = m_name]() { return csp::common::ReplicatedValue { name }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_name, value.GetString(), UPDATE_FLAGS_NAME); } 
        },
        {
            SpaceEntityComponentKey::Position, UPDATE_FLAGS_POSITION,
            [&position = m_transform.Position]() { return csp::common::ReplicatedValue { position }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_transform.Position, value.GetVector3(), UPDATE_FLAGS_POSITION); }
        },
        { 
            SpaceEntityComponentKey::Rotation, UPDATE_FLAGS_ROTATION,
            [&rotation = m_transform.Rotation]() { return csp::common::ReplicatedValue { rotation }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_transform.Rotation, value.GetVector4(), UPDATE_FLAGS_ROTATION); }
        },
        {
            SpaceEntityComponentKey::Scale, UPDATE_FLAGS_SCALE,
            [&scale = m_transform.Scale]() { return csp::common::ReplicatedValue { scale }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_transform.Scale, value.GetVector3(), UPDATE_FLAGS_SCALE); }
        },
        {
            SpaceEntityComponentKey::SelectedClientId, UPDATE_FLAGS_SELECTION_ID,
            [&selectedId = m_selectedId]() { return csp::common::ReplicatedValue { static_cast<int64_t>(selectedId) }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_selectedId, value.GetInt(), UPDATE_FLAGS_SELECTION_ID); }
        },
        {
            SpaceEntityComponentKey::ThirdPartyRef, UPDATE_FLAGS_THIRD_PARTY_REF,
            [&thirdPartyRef = m_thirdPartyRef]() { return csp::common::ReplicatedValue { thirdPartyRef }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_thirdPartyRef, value.GetString(), UPDATE_FLAGS_THIRD_PARTY_REF); }
        },
        {
            SpaceEntityComponentKey::LockType, UPDATE_FLAGS_LOCK_TYPE,
            [&entityLock = m_entityLock]() { return csp::common::ReplicatedValue { static_cast<int64_t>(entityLock) }; },
            [this](const csp::common::ReplicatedValue& value) { SetPropertyDirect(m_entityLock, static_cast<LockType>(value.GetInt()), UPDATE_FLAGS_LOCK_TYPE); }
        }

    };
    /* clang-format on */
}

csp::multiplayer::EntityScriptInterface* SpaceEntity::GetScriptInterface() { return m_scriptInterface.get(); }

void SpaceEntity::ClaimScriptOwnership()
{
    if (m_entitySystem->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
    {
        auto* onlineRealtimeEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(m_entitySystem);
        onlineRealtimeEngine->ClaimScriptOwnership(this);
    }
}

void SpaceEntity::OnPropertyChanged(ComponentBase* dirtyComponent, int32_t propertyKey)
{
    m_script.OnPropertyChanged(dirtyComponent->GetId(), propertyKey);
}

} // namespace csp::multiplayer
