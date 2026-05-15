/*
 * Copyright 2025 Magnopus LLC

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

#include "SpaceEntityStatePatcher.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Common/Convert.h"

#include <algorithm>
#include <fmt/format.h>

namespace csp::multiplayer
{

SpaceEntityStatePatcher::SpaceEntityStatePatcher(csp::common::LogSystem* logSystem, csp::multiplayer::SpaceEntity& spaceEntity)
    : m_timeOfLastPatch(0)
    , m_logSystem(logSystem)
    , m_spaceEntity(spaceEntity)
{
}

bool SpaceEntityStatePatcher::SetDirtyComponent(uint16_t componentKey, DirtyComponent dirtyComponent)
{
    std::scoped_lock componentsLocker(m_dirtyComponentsLock);

    if (m_dirtyComponents.count(componentKey) > 0)
    {
        if (m_logSystem)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
                fmt::format(
                    "SpaceEntityStatePatcher::SetDirtyComponent. Dirty components map already contains key : {}. Performing no action", componentKey)
                    .c_str());
        }
        return false;
    }

    m_dirtyComponents[componentKey] = dirtyComponent;
    return true;
}

bool SpaceEntityStatePatcher::RemoveDirtyComponent(uint16_t componentKey, const csp::common::Map<uint16_t, ComponentBase*>& currentComponents)
{
    std::scoped_lock componentsLocker(m_dirtyComponentsLock);

    if (!m_transientDeletionComponentIds.Contains(componentKey) || currentComponents.HasKey(componentKey))
    {

        m_dirtyComponents.erase(componentKey);
        m_transientDeletionComponentIds.Append(componentKey);
        return true;
    }
    else
    {
        if (m_logSystem)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "RemoveComponent: No Component with the specified key found!");
        }
        return false;
    }
}

std::pair<SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>> SpaceEntityStatePatcher::ApplyLocalPatch()
{
    std::scoped_lock<std::mutex> propertiesLocker(m_dirtyPropertiesLock);
    std::scoped_lock<std::mutex> componentsLocker(m_dirtyComponentsLock);

    auto updateFlags = static_cast<SpaceEntityUpdateFlags>(0);

    for (const auto& dirtyProperty : m_dirtyProperties)
    {
        // Find our entity property using the dirty property id.
        const SpaceEntityComponentKey propertyKey = dirtyProperty.first;
        auto propertyIt = m_registeredProperties.find(propertyKey);

        if (propertyIt != m_registeredProperties.end())
        {
            // Set our entity property using the dirty property value.
            EntityProperty& property = propertyIt->second;
            updateFlags = static_cast<SpaceEntityUpdateFlags>(updateFlags | property.GetUpdateFlag());
            property.Set(dirtyProperty.second);
        }
        else
        {
            if (m_logSystem)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Error, "ApplyLocalPatch: No Property with the specified key found!");
            }
        }
    }

    m_dirtyProperties.clear();

    // Allocate a ComponentUpdates array (to pass update info to the client), with
    // sufficient size for all dirty components and scheduled deletions.
    csp::common::Array<ComponentUpdateInfo> componentUpdates(m_dirtyComponents.size() + m_transientDeletionComponentIds.Size());

    if (m_dirtyComponents.size() > 0)
    {
        updateFlags = static_cast<SpaceEntityUpdateFlags>(updateFlags | UPDATE_FLAGS_COMPONENTS);

        size_t index = 0;
        for (const auto& dirtyComponent : m_dirtyComponents)
        {
            uint16_t componentKey = dirtyComponent.first;

            switch (m_dirtyComponents[componentKey].UpdateType)
            {
            case ComponentUpdateType::Add:
                m_spaceEntity.AddComponentDirect(componentKey, m_dirtyComponents[componentKey].Component, false);
                // Components[ComponentKey] = DirtyComponents[ComponentKey].Component;
                componentUpdates[index].ComponentId = m_dirtyComponents[componentKey].Component->GetId();
                componentUpdates[index].UpdateType = ComponentUpdateType::Add;
                break;
            case ComponentUpdateType::Delete:
                m_spaceEntity.RemoveComponentDirect(componentKey, false);
                componentUpdates[index].ComponentId = componentKey;
                componentUpdates[index].UpdateType = ComponentUpdateType::Delete;
                break;
            case ComponentUpdateType::Update:
            {
                // You may expect a `SpaceEntity.UpdateComponentDirect`, but component property updates
                // are still out-of-pattern and set immediately rather than looping back. Should change.

                componentUpdates[index].ComponentId = m_dirtyComponents[componentKey].Component->GetId();
                componentUpdates[index].UpdateType = ComponentUpdateType::Update;

                // TODO: For the moment, we update all properties on a dirty component, in future we need to change this to per property
                // replication. Components[DirtyComponents[i].Component->GetId()]->Properties = DirtyComponents[i].Component->DirtyProperties;

                /*const csp::common::Map<uint32_t, csp::common::ReplicatedValue> DirtyComponentProperties =
                DirtyComponents[i].Component->DirtyProperties; const csp::common::Array<uint32_t>* DirtyComponentPropertyKeys
                = DirtyComponentProperties.Keys();

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

            index++;
        }

        m_dirtyComponents.clear();
    }

    // Parent ID, (this would be a dirty property as above, but the wrapper generator stops us expressing optional values. Very not nice.)
    if (m_newParentId.HasValue()) // If the outer optional is set, then we want to change the parent
    {
        m_spaceEntity.SetParentIdDirect(*m_newParentId, false);
        updateFlags = static_cast<SpaceEntityUpdateFlags>(updateFlags | UPDATE_FLAGS_PARENT);
        m_newParentId = csp::common::Optional<csp::common::Optional<uint64_t>> {}; // Reset, as we've just set it
    }

    // Component Deletes
    if (m_transientDeletionComponentIds.Size() > 0)
    {
        for (size_t i = 0; i < m_transientDeletionComponentIds.Size(); ++i)
        {
            if (m_spaceEntity.GetComponents()->HasKey(m_transientDeletionComponentIds[i]))
            {
                ComponentBase* component = m_spaceEntity.GetComponent(m_transientDeletionComponentIds[i]);
                component->OnLocalDelete();

                m_spaceEntity.RemoveComponentDirect(m_transientDeletionComponentIds[i]);

                // Start indexing from the end of the section reserved for DirtyComponents.
                // We start adding DirtyComponents to ComponentUpdates first, so here we need to respect that
                // and start at an offset to add our deletion updates.
                componentUpdates[m_dirtyComponents.size() + i].ComponentId = m_transientDeletionComponentIds[i];
                componentUpdates[m_dirtyComponents.size() + i].UpdateType = ComponentUpdateType::Delete;

                updateFlags = static_cast<SpaceEntityUpdateFlags>(updateFlags | UPDATE_FLAGS_COMPONENTS);
            }
        }

        m_transientDeletionComponentIds.Clear();
    }

    return std::pair<SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>>(updateFlags, componentUpdates);
}

std::unordered_map<SpaceEntityComponentKey, csp::common::ReplicatedValue> SpaceEntityStatePatcher::GetDirtyProperties() const
{
    return m_dirtyProperties;
}

std::unordered_map<uint16_t, SpaceEntityStatePatcher::DirtyComponent> SpaceEntityStatePatcher::GetDirtyComponents() const { return m_dirtyComponents; }

std::chrono::milliseconds SpaceEntityStatePatcher::GetTimeOfLastPatch() const { return m_timeOfLastPatch; }

void SpaceEntityStatePatcher::SetTimeOfLastPatch(std::chrono::milliseconds newTimeOfLastPatch) { this->m_timeOfLastPatch = newTimeOfLastPatch; }

csp::common::Optional<csp::common::Optional<uint64_t>> SpaceEntityStatePatcher::GetNewParentId() const { return m_newParentId; }

void SpaceEntityStatePatcher::SetNewParentId(csp::common::Optional<uint64_t> value) { m_newParentId = value; }

bool SpaceEntityStatePatcher::HasPendingPatch() const
{
    return !(m_dirtyComponents.size() == 0 && m_dirtyProperties.size() == 0 && m_transientDeletionComponentIds.Size() == 0
        && GetNewParentId().HasValue() == false);
}

csp::multiplayer::ComponentBase* SpaceEntityStatePatcher::GetFirstPendingComponentOfType(
    ComponentType type, std::set<ComponentUpdateType> interestingUpdateTypes) const
{
    std::scoped_lock componentsLocker(m_dirtyComponentsLock);

    for (const std::pair<const uint16_t, DirtyComponent>& dirtyComp : m_dirtyComponents)
    {
        // If any of our dirty components are :
        //  - Of the type requested AND
        //  - Of update types of interest
        const auto updateType = dirtyComp.second.UpdateType;
        if ((dirtyComp.second.Component->GetComponentType() == type) && (interestingUpdateTypes.find(updateType) != interestingUpdateTypes.end()))
        {
            return dirtyComp.second.Component;
        }
    }
    return nullptr;
}

mcs::ObjectMessage SpaceEntityStatePatcher::CreateObjectMessage() const
{
    // 1. Convert all of our view components to mcs compatible types.
    MCSComponentPacker componentPacker;

    for (const auto& prop : m_registeredProperties)
    {
        csp::common::ReplicatedValue replicatedValue = prop.second.Get();
        componentPacker.WriteValue(prop.second.GetKey(), replicatedValue);
    }

    std::scoped_lock<std::mutex> componentsLocker(m_dirtyComponentsLock);

    for (const std::pair<const uint16_t, SpaceEntityStatePatcher::DirtyComponent>& component : m_dirtyComponents)
    {
        assert(component.second.Component != nullptr && "DirtyComponent given a null component!");

        if (component.second.Component != nullptr)
        {
            auto* realComponent = component.second.Component;
            componentPacker.WriteValue(component.first, realComponent);
        }
    }

    // 3. Create the object message using the reqired properties and our created components.
    return mcs::ObjectMessage { m_spaceEntity.GetId(), static_cast<uint64_t>(m_spaceEntity.GetEntityType()), m_spaceEntity.GetIsTransferable(),
        m_spaceEntity.GetIsPersistent(), m_spaceEntity.GetOwnerId(), Convert(m_spaceEntity.GetParentId()), componentPacker.GetComponents() };
}

mcs::ObjectPatch SpaceEntityStatePatcher::CreateObjectPatch() const
{
    MCSComponentPacker componentPacker;

    // 1. Convert our modified view components to mcs compatible types.
    {
        // Loop through modfied view components and convert to ItemComponentData.
        for (const std::pair<const SpaceEntityComponentKey, csp::common::ReplicatedValue>& dirtyProp : m_dirtyProperties)
        {
            componentPacker.WriteValue(dirtyProp.first, dirtyProp.second);
        }
    }

    // 2. Convert all of our runtime components to mcs compatible types.
    {
        std::scoped_lock componentsLocker(m_dirtyComponentsLock);

        // Loop through all components and convert to ItemComponentData.
        for (const std::pair<const uint16_t, SpaceEntityStatePatcher::DirtyComponent>& component : m_dirtyComponents)
        {
            assert(component.second.Component != nullptr && "DirtyComponent given a null component!");

            if (component.second.Component != nullptr)
            {
                auto* realComponent = component.second.Component;
                componentPacker.WriteValue(component.first, realComponent);
            }
        }
    }

    // 3. Handle any component deletions (The fact this has to take a non-const pointer to SpaceEntity is deceptive, ruins the function signature
    // here.)
    ComponentBase deletionComponent(ComponentType::Delete, m_logSystem, &m_spaceEntity);

    for (size_t i = 0; i < m_transientDeletionComponentIds.Size(); ++i)
    {
        deletionComponent.SetId(m_transientDeletionComponentIds[i]);
        componentPacker.WriteValue(deletionComponent.GetId(), &deletionComponent);
    }

    // 4. Create the object patch using the required properties and our created components.
    // Seems like a bit of a mixed bag here, Components + Parent updates are disconnected state, but pulling Id's from SpaceEntity feels like it
    // leaves us vulnerable to sequencing bugs. Fine if ID + OwnerID never change, but dubious about that for OwnerId.
    const bool hasBeenParentUpdate = m_newParentId.HasValue();
    return mcs::ObjectPatch { m_spaceEntity.GetId(), m_spaceEntity.GetOwnerId(), false, hasBeenParentUpdate,
        hasBeenParentUpdate ? Convert(*m_newParentId) : Convert(m_spaceEntity.GetParentId()), componentPacker.GetComponents() };
}

std::unique_ptr<csp::multiplayer::SpaceEntity> SpaceEntityStatePatcher::NewFromObjectMessage(const mcs::ObjectMessage& message,
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::IJSScriptRunner& scriptRunner, csp::common::LogSystem& logSystem)
{
    const auto id = message.GetId();
    const auto type = static_cast<SpaceEntityType>(message.GetType());
    const auto isTransferable = message.GetIsTransferable();
    const auto isPersistent = message.GetIsPersistent();
    const auto ownerId = message.GetOwnerId();
    const auto parentId = common::Convert(message.GetParentId());

    const std::optional<std::map<uint16_t, mcs::ItemComponentData>>& messageComponents = message.GetComponents();

    std::unique_ptr<csp::multiplayer::SpaceEntity> entity(new csp::multiplayer::SpaceEntity(
        &realtimeEngine, scriptRunner, &logSystem, type, id, "", SpaceTransform {}, ownerId, parentId, isTransferable, isPersistent));

    if (messageComponents.has_value())
    {
        // Get view components
        MCSComponentUnpacker componentUnpacker { *messageComponents };

        // It's unfortunate we have to break the usual pattern of getting the registered properties from the state patcher here,
        // but we can't assume that this will be called in an online context, due to this function being used for deserializing entities
        // with the SceneDescription file.
        auto properties = entity->CreateReplicatedProperties();

        for (const auto& componentDataPair : *messageComponents)
        {
            // All component keys less than COMPONENT_KEY_END_COMPONENTS are our CSP runtime components
            if (componentDataPair.first < COMPONENT_KEY_END_COMPONENTS)
            {
                // Convert the mcs component to a csp component
                entity->AddComponentFromItemComponentData(componentDataPair.first, componentDataPair.second);
            }
            else
            {
                // Anything after COMPONENT_KEY_END_COMPONENTS are our CSP entity properties

                // Find the property using our property key.
                EntityProperty* property = std::find_if(properties.begin(), properties.end(),
                    [key = componentDataPair.first](const EntityProperty& prop) { return static_cast<uint16_t>(prop.GetKey()) == key; });

                if (property != properties.end())
                {
                    // Set our property from the component value.
                    csp::common::ReplicatedValue value;
                    componentUnpacker.TryReadValue(componentDataPair.first, value);
                    property->Set(value);
                }
                else
                {
                    logSystem.LogMsg(csp::common::LogLevel::Error, "NewFromObjectMessage: No Property with the specified key found!");
                }
            }
        }
    }

    // Would much rather return this as a value, requires simplifying SpaceEntity such that it can have copy/move operators.
    return entity;
}

void SpaceEntityStatePatcher::ApplyPatchFromObjectPatch(const mcs::ObjectPatch& patch)
{
    SpaceEntityUpdateFlags updateFlags = SpaceEntityUpdateFlags(0);
    csp::common::Array<ComponentUpdateInfo> componentUpdates(0);

    auto patchComponents = patch.GetComponents();

    if (patchComponents.has_value())
    {
        MCSComponentUnpacker componentUnpacker { *patch.GetComponents() };
        uint64_t componentCount = componentUnpacker.GetRuntimeComponentsCount();

        if (componentCount > 0)
        {
            updateFlags = static_cast<SpaceEntityUpdateFlags>(updateFlags | UPDATE_FLAGS_COMPONENTS);
        }

        componentUpdates = csp::common::Array<ComponentUpdateInfo>(componentCount);
        size_t componentIndex = 0;

        for (const auto& componentDataPair : *patchComponents)
        {
            // All component keys less than COMPONENT_KEY_END_COMPONENTS are our CSP runtime components
            if (componentDataPair.first < COMPONENT_KEY_END_COMPONENTS)
            {
                // Add the component to our entity
                ComponentUpdateInfo updateInfo
                    = m_spaceEntity.AddComponentFromItemComponentDataPatch(componentDataPair.first, componentDataPair.second);
                componentUpdates[componentIndex] = updateInfo;
                componentIndex++;
            }
            else
            {
                // Anything after COMPONENT_KEY_END_COMPONENTS are our CSP entity properties

                // Find the property using our property key.
                auto propertyIt = m_registeredProperties.find(static_cast<SpaceEntityComponentKey>(componentDataPair.first));

                if (propertyIt != m_registeredProperties.end())
                {

                    // Set our property from the component value.
                    EntityProperty& property = propertyIt->second;
                    updateFlags = SpaceEntityUpdateFlags(updateFlags | property.GetUpdateFlag());

                    csp::common::ReplicatedValue value;
                    componentUnpacker.TryReadValue(componentDataPair.first, value);
                    property.Set(value);
                }
                else
                {
                    if (m_logSystem)
                    {
                        m_logSystem->LogMsg(csp::common::LogLevel::Error, "ApplyPatchFromObjectPatch: No Property with the specified key found!");
                    }
                }
            }
        }
    }

    m_spaceEntity.SetOwnerId(patch.GetOwnerId());
    const auto parentId = common::Convert(patch.GetParentId());

    m_spaceEntity.SetParentIdDirect(parentId, false);
    if (patch.GetShouldUpdateParent())
    {
        updateFlags = static_cast<SpaceEntityUpdateFlags>(updateFlags | UPDATE_FLAGS_PARENT);
    }

    if (updateFlags != 0 && m_spaceEntity.GetEntityUpdateCallback() != nullptr)
    {
        m_spaceEntity.GetEntityUpdateCallback()(&m_spaceEntity, updateFlags, componentUpdates);
    }
}

void SpaceEntityStatePatcher::SetPatchSentCallback(PatchSentCallback callback) { m_entityPatchSentCallback = callback; }

SpaceEntityStatePatcher::PatchSentCallback SpaceEntityStatePatcher::GetEntityPatchSentCallback() { return m_entityPatchSentCallback; }

void SpaceEntityStatePatcher::CallEntityPatchSentCallback(bool success) { m_entityPatchSentCallback(success); }

void SpaceEntityStatePatcher::RegisterProperty(const EntityProperty& property) { m_registeredProperties[property.GetKey()] = property; }

void SpaceEntityStatePatcher::RegisterProperties(const csp::common::Array<EntityProperty>& properties)
{
    for (const auto& prop : properties)
    {
        RegisterProperty(prop);
    }
}

void EntityProperty::Set(const csp::common::ReplicatedValue& repValue) { m_fromReplicatedValue(repValue); }

csp::common::ReplicatedValue EntityProperty::Get() const { return m_toReplicatedValue(); }

} // namespace csp::multiplayer
