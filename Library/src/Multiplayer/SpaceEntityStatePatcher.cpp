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
#include "Multiplayer/MCSComponentPacker.h"
#include "Multiplayer/SpaceEntityKeys.h"

#include <algorithm>
#include <fmt/format.h>

namespace csp::multiplayer
{

SpaceEntityStatePatcher::SpaceEntityStatePatcher(csp::common::LogSystem* LogSystem, csp::multiplayer::SpaceEntity& SpaceEntity)
    : TimeOfLastPatch(0)
    , LogSystem(LogSystem)
    , SpaceEntity(SpaceEntity)
{
}

bool SpaceEntityStatePatcher::SetDirtyComponent(uint16_t ComponentKey, DirtyComponent DirtyComponent)
{
    std::scoped_lock ComponentsLocker(DirtyComponentsLock);

    if (DirtyComponents.count(ComponentKey) > 0)
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Verbose,
                fmt::format(
                    "SpaceEntityStatePatcher::SetDirtyComponent. Dirty components map already contains key : {}. Performing no action", ComponentKey)
                    .c_str());
        }
        return false;
    }

    DirtyComponents[ComponentKey] = DirtyComponent;
    return true;
}

bool SpaceEntityStatePatcher::RemoveDirtyComponent(uint16_t ComponentKey, const csp::common::Map<uint16_t, ComponentBase*>& CurrentComponents)
{
    std::scoped_lock ComponentsLocker(DirtyComponentsLock);

    if (!TransientDeletionComponentIds.Contains(ComponentKey) || CurrentComponents.HasKey(ComponentKey))
    {

        DirtyComponents.erase(ComponentKey);
        TransientDeletionComponentIds.Append(ComponentKey);
        return true;
    }
    else
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "RemoveComponent: No Component with the specified key found!");
        }
        return false;
    }
}

std::pair<SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>> SpaceEntityStatePatcher::ApplyLocalPatch()
{
    std::scoped_lock<std::mutex> PropertiesLocker(DirtyPropertiesLock);
    std::scoped_lock<std::mutex> ComponentsLocker(DirtyComponentsLock);

    auto UpdateFlags = static_cast<SpaceEntityUpdateFlags>(0);

    // Allocate a ComponentUpdates array (to pass update info to the client), with
    // sufficient size for all dirty components and scheduled deletions.
    csp::common::Array<ComponentUpdateInfo> ComponentUpdates(DirtyComponents.size() + TransientDeletionComponentIds.Size());

    if (DirtyComponents.size() > 0)
    {
        UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_COMPONENTS);

        size_t Index = 0;
        for (const auto& DirtyComponent : DirtyComponents)
        {
            uint16_t ComponentKey = DirtyComponent.first;

            switch (DirtyComponents[ComponentKey].UpdateType)
            {
            case ComponentUpdateType::Add:
                SpaceEntity.AddComponentDirect(ComponentKey, DirtyComponents[ComponentKey].Component, false);
                // Components[ComponentKey] = DirtyComponents[ComponentKey].Component;
                ComponentUpdates[Index].ComponentId = DirtyComponents[ComponentKey].Component->GetId();
                ComponentUpdates[Index].UpdateType = ComponentUpdateType::Add;
                break;
            case ComponentUpdateType::Delete:
                SpaceEntity.RemoveComponentDirect(ComponentKey, false);
                ComponentUpdates[Index].ComponentId = ComponentKey;
                ComponentUpdates[Index].UpdateType = ComponentUpdateType::Delete;
                break;
            case ComponentUpdateType::Update:
            {
                // This is weird ... we wern't doing any actual data update here before ... just notification stuff. I guess because all the actual
                // data was set by this point as we change the properties in the components themselves. Odd pattern break.
                SpaceEntity.UpdateComponentDirect(ComponentKey, DirtyComponents[ComponentKey].Component, false);

                ComponentUpdates[Index].ComponentId = DirtyComponents[ComponentKey].Component->GetId();
                ComponentUpdates[Index].UpdateType = ComponentUpdateType::Update;

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

            Index++;
        }

        DirtyComponents.clear();
    }

    if (DirtyProperties.size() > 0)
    {

        for (const auto& DirtyProperty : DirtyProperties)
        {
            uint16_t PropertyKey = DirtyProperty.first;
            switch (PropertyKey)
            {
            case COMPONENT_KEY_VIEW_ENTITYNAME:
                SpaceEntity.SetNameDirect(DirtyProperties[PropertyKey].GetString());
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_NAME);
                break;
            case COMPONENT_KEY_VIEW_POSITION:
                SpaceEntity.SetPositionDirect(DirtyProperties[PropertyKey].GetVector3());
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_POSITION);
                break;
            case COMPONENT_KEY_VIEW_ROTATION:
                SpaceEntity.SetRotationDirect(DirtyProperties[PropertyKey].GetVector4());
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_ROTATION);
                break;
            case COMPONENT_KEY_VIEW_SCALE:
                SpaceEntity.SetScaleDirect(DirtyProperties[PropertyKey].GetVector3());
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_SCALE);
                break;
            case COMPONENT_KEY_VIEW_SELECTEDCLIENTID:
                SpaceEntity.SetSelectedIdDirect(DirtyProperties[PropertyKey].GetInt());
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_SELECTION_ID);
                break;
            case COMPONENT_KEY_VIEW_THIRDPARTYREF:
                SpaceEntity.SetThirdPartyRefDirect(DirtyProperties[PropertyKey].GetString());
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_REF);
                break;
            case COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM:
                SpaceEntity.SetThirdPartyPlatformDirect(static_cast<csp::systems::EThirdPartyPlatform>(DirtyProperties[PropertyKey].GetInt()));
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_PLATFORM);
                break;
            case COMPONENT_KEY_VIEW_LOCKTYPE:
                SpaceEntity.SetEntityLockDirect(static_cast<LockType>(DirtyProperties[PropertyKey].GetInt()));
                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_LOCK_TYPE);
                break;
            default:
                break;
            }
        }

        DirtyProperties.clear();
    }

    // Parent ID, (this would be a dirty property as above, but the wrapper generator stops us expressing optional values. Very not nice.)
    if (NewParentId.HasValue()) // If the outer optional is set, then we want to change the parent
    {
        SpaceEntity.SetParentIdDirect(*NewParentId, false);
        UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_PARENT);
        NewParentId = csp::common::Optional<csp::common::Optional<uint64_t>> {}; // Reset, as we've just set it
    }

    // Component Deletes
    if (TransientDeletionComponentIds.Size() > 0)
    {
        for (size_t i = 0; i < TransientDeletionComponentIds.Size(); ++i)
        {
            if (SpaceEntity.GetComponents()->HasKey(TransientDeletionComponentIds[i]))
            {
                ComponentBase* Component = SpaceEntity.GetComponent(TransientDeletionComponentIds[i]);
                Component->OnLocalDelete();

                SpaceEntity.RemoveComponentDirect(TransientDeletionComponentIds[i]);

                // Start indexing from the end of the section reserved for DirtyComponents.
                // We start adding DirtyComponents to ComponentUpdates first, so here we need to respect that
                // and start at an offset to add our deletion updates.
                ComponentUpdates[DirtyComponents.size() + i].ComponentId = TransientDeletionComponentIds[i];
                ComponentUpdates[DirtyComponents.size() + i].UpdateType = ComponentUpdateType::Delete;

                UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_COMPONENTS);
            }
        }

        TransientDeletionComponentIds.Clear();
    }

    return std::pair<SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>>(UpdateFlags, ComponentUpdates);
}

std::unordered_map<uint16_t, csp::common::ReplicatedValue> SpaceEntityStatePatcher::GetDirtyProperties() const { return DirtyProperties; }

std::unordered_map<uint16_t, SpaceEntityStatePatcher::DirtyComponent> SpaceEntityStatePatcher::GetDirtyComponents() const { return DirtyComponents; }

std::chrono::milliseconds SpaceEntityStatePatcher::GetTimeOfLastPatch() const { return TimeOfLastPatch; }

void SpaceEntityStatePatcher::SetTimeOfLastPatch(std::chrono::milliseconds NewTimeOfLastPatch) { this->TimeOfLastPatch = NewTimeOfLastPatch; }

csp::common::Optional<csp::common::Optional<uint64_t>> SpaceEntityStatePatcher::GetNewParentId() const { return NewParentId; }

void SpaceEntityStatePatcher::SetNewParentId(csp::common::Optional<uint64_t> Value) { NewParentId = Value; }

bool SpaceEntityStatePatcher::HasPendingPatch() const
{
    return !(DirtyComponents.size() == 0 && DirtyProperties.size() == 0 && TransientDeletionComponentIds.Size() == 0
        && GetNewParentId().HasValue() == false);
}

csp::multiplayer::ComponentBase* SpaceEntityStatePatcher::GetFirstPendingComponentOfType(
    ComponentType Type, std::set<ComponentUpdateType> InterestingUpdateTypes) const
{
    std::scoped_lock ComponentsLocker(DirtyComponentsLock);

    for (const std::pair<const uint16_t, DirtyComponent>& DirtyComp : DirtyComponents)
    {
        // If any of our dirty components are :
        //  - Of the type requested AND
        //  - Of update types of interest
        const auto UpdateType = DirtyComp.second.UpdateType;
        if ((DirtyComp.second.Component->GetComponentType() == Type) && (InterestingUpdateTypes.find(UpdateType) != InterestingUpdateTypes.end()))
        {
            return DirtyComp.second.Component;
        }
    }
    return nullptr;
}

mcs::ObjectMessage SpaceEntityStatePatcher::CreateObjectMessage() const
{
    // 1. Convert all of our view components to mcs compatible types.
    MCSComponentPacker ComponentPacker;

    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_ENTITYNAME, SpaceEntity.GetName());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_POSITION, SpaceEntity.GetPosition());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_ROTATION, SpaceEntity.GetRotation());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_SCALE, SpaceEntity.GetScale());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, SpaceEntity.GetSelectingClientID());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, SpaceEntity.GetThirdPartyPlatformType());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_THIRDPARTYREF, SpaceEntity.GetThirdPartyRef());
    ComponentPacker.WriteValue(COMPONENT_KEY_VIEW_LOCKTYPE, SpaceEntity.GetLockType());

    std::scoped_lock<std::mutex> ComponentsLocker(DirtyComponentsLock);

    for (const std::pair<const uint16_t, SpaceEntityStatePatcher::DirtyComponent>& Component : DirtyComponents)
    {
        assert(Component.second.Component != nullptr && "DirtyComponent given a null component!");

        if (Component.second.Component != nullptr)
        {
            auto* RealComponent = Component.second.Component;
            ComponentPacker.WriteValue(Component.first, RealComponent);
        }
    }

    // 3. Create the object message using the reqired properties and our created components.
    return mcs::ObjectMessage { SpaceEntity.GetId(), static_cast<uint64_t>(SpaceEntity.GetEntityType()), SpaceEntity.GetIsTransferable(),
        SpaceEntity.GetIsPersistent(), SpaceEntity.GetOwnerId(), Convert(SpaceEntity.GetParentId()), ComponentPacker.GetComponents() };
}

mcs::ObjectPatch SpaceEntityStatePatcher::CreateObjectPatch() const
{
    MCSComponentPacker ComponentPacker;

    // 1. Convert our modified view components to mcs compatible types.
    {
        // Loop through modfied view components and convert to ItemComponentData.
        for (const std::pair<const uint16_t, csp::common::ReplicatedValue>& DirtyProp : DirtyProperties)
        {
            ComponentPacker.WriteValue(DirtyProp.first, DirtyProp.second);
        }
    }

    // 2. Convert all of our runtime components to mcs compatible types.
    {
        std::scoped_lock ComponentsLocker(DirtyComponentsLock);

        // Loop through all components and convert to ItemComponentData.
        for (const std::pair<const uint16_t, SpaceEntityStatePatcher::DirtyComponent>& Component : DirtyComponents)
        {
            assert(Component.second.Component != nullptr && "DirtyComponent given a null component!");

            if (Component.second.Component != nullptr)
            {
                auto* RealComponent = Component.second.Component;
                ComponentPacker.WriteValue(Component.first, RealComponent);
            }
        }
    }

    // 3. Handle any component deletions (The fact this has to take a non-const pointer to SpaceEntity is deceptive, ruins the function signature
    // here.)
    ComponentBase DeletionComponent(ComponentType::Delete, LogSystem, &SpaceEntity);

    for (size_t i = 0; i < TransientDeletionComponentIds.Size(); ++i)
    {
        DeletionComponent.SetId(TransientDeletionComponentIds[i]);
        ComponentPacker.WriteValue(DeletionComponent.GetId(), &DeletionComponent);
    }

    // 4. Create the object patch using the required properties and our created components.
    // Seems like a bit of a mixed bag here, Components + Parent updates are disconnected state, but pulling Id's from SpaceEntity feels like it
    // leaves us vulnerable to sequencing bugs. Fine if ID + OwnerID never change, but dubious about that for OwnerId.
    const bool HasBeenParentUpdate = NewParentId.HasValue();
    return mcs::ObjectPatch { SpaceEntity.GetId(), SpaceEntity.GetOwnerId(), false, HasBeenParentUpdate,
        HasBeenParentUpdate ? Convert(*NewParentId) : Convert(SpaceEntity.GetParentId()), ComponentPacker.GetComponents() };
}

SpaceEntity* SpaceEntityStatePatcher::NewFromObjectMessage(const mcs::ObjectMessage& Message, csp::common::IRealtimeEngine& RealtimeEngine,
    csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem& LogSystem)
{
    const auto Id = Message.GetId();
    const auto Type = static_cast<SpaceEntityType>(Message.GetType());
    const auto IsTransferable = Message.GetIsTransferable();
    const auto IsPersistent = Message.GetIsPersistent();
    const auto OwnerId = Message.GetOwnerId();
    const auto ParentId = common::Convert(Message.GetParentId());

    // This stuff read from the component unpacker, I guess the above are first-class pieces of data.
    csp::common::String Name = "";
    SpaceTransform Transform {};
    uint64_t SelectedId = 0;
    csp::systems::EThirdPartyPlatform ThirdPartyPlatform = csp::systems::EThirdPartyPlatform::NONE;
    csp::common::String ThirdPartyRef = "";
    LockType EntityLock = LockType::None;

    auto MessageComponents = Message.GetComponents();

    std::vector<std::pair<uint16_t, mcs::ItemComponentData>> ComponentsToAdd;

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

            ComponentsToAdd.push_back(ComponentDataPair);
        }
    }

    csp::multiplayer::SpaceEntity* NewEntity = new csp::multiplayer::SpaceEntity(
        &RealtimeEngine, ScriptRunner, &LogSystem, Type, Id, Name, Transform, OwnerId, ParentId, IsTransferable, IsPersistent);

    // We need a consistent strategy for ensuring fully constructed SpaceEntities. Either they're simple enough data objects that they can just have
    // public data, or they have preconditions and should have guarded construction ... this mix is dangerous.
    NewEntity->SetSelectedIdDirect(SelectedId);
    NewEntity->SetThirdPartyPlatformDirect(ThirdPartyPlatform);
    NewEntity->SetThirdPartyRefDirect(ThirdPartyRef);
    NewEntity->SetEntityLockDirect(EntityLock);

    for (const auto& Comp : ComponentsToAdd)
    {
        NewEntity->AddComponentFromItemComponentData(Comp.first, Comp.second);
    }

    // Would much rather return this as a value, requires simplifying SpaceEntity such that it can have copy/move operators.
    return NewEntity;
}

void SpaceEntityStatePatcher::ApplyPatchFromObjectPatch(const mcs::ObjectPatch& Patch)
{

    SpaceEntityUpdateFlags UpdateFlags = SpaceEntityUpdateFlags(0);
    csp::common::Array<ComponentUpdateInfo> ComponentUpdates(0);

    auto PatchComponents = Patch.GetComponents();

    if (PatchComponents.has_value())
    {
        csp::common::String Name;
        SpaceTransform Transform;
        uint64_t SelectedId;
        csp::systems::EThirdPartyPlatform ThirdPartyPlatform;
        csp::common::String ThirdPartyRef;
        LockType EntityLock;

        MCSComponentUnpacker ComponentUnpacker { *Patch.GetComponents() };

        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_ENTITYNAME, Name))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_NAME);
            SpaceEntity.SetNameDirect(Name);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_POSITION, Transform.Position))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_POSITION);
            SpaceEntity.SetPositionDirect(Transform.Position);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_ROTATION, Transform.Rotation))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_ROTATION);
            SpaceEntity.SetRotationDirect(Transform.Rotation);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_SCALE, Transform.Scale))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_SCALE);
            SpaceEntity.SetScaleDirect(Transform.Scale);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_SELECTEDCLIENTID, SelectedId))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_SELECTION_ID);
            SpaceEntity.SetSelectedIdDirect(SelectedId);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM, ThirdPartyPlatform))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_PLATFORM);
            SpaceEntity.SetThirdPartyPlatformDirect(ThirdPartyPlatform);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_THIRDPARTYREF, ThirdPartyRef))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_THIRD_PARTY_REF);
            SpaceEntity.SetThirdPartyRefDirect(ThirdPartyRef);
        }
        if (ComponentUnpacker.TryReadValue(COMPONENT_KEY_VIEW_LOCKTYPE, EntityLock))
        {
            UpdateFlags = SpaceEntityUpdateFlags(UpdateFlags | UPDATE_FLAGS_LOCK_TYPE);
            SpaceEntity.SetEntityLockDirect(EntityLock);
        }

        uint64_t ComponentCount = ComponentUnpacker.GetRuntimeComponentsCount();

        if (ComponentCount > 0)
        {
            UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_COMPONENTS);

            ComponentUpdates = csp::common::Array<ComponentUpdateInfo>(ComponentCount);
            size_t ComponentIndex = 0;

            for (const auto& ComponentDataPair : *PatchComponents)
            {
                if (ComponentDataPair.first >= COMPONENT_KEY_END_COMPONENTS)
                {
                    // This is the end of our components
                    break;
                }

                ComponentUpdateInfo UpdateInfo
                    = SpaceEntity.AddComponentFromItemComponentDataPatch(ComponentDataPair.first, ComponentDataPair.second);
                ComponentUpdates[ComponentIndex] = UpdateInfo;
                ComponentIndex++;
            }
        }
    }

    SpaceEntity.SetOwnerId(Patch.GetOwnerId());
    const auto ParentId = common::Convert(Patch.GetParentId());

    SpaceEntity.SetParentIdDirect(ParentId, false);
    if (Patch.GetShouldUpdateParent())
    {
        UpdateFlags = static_cast<SpaceEntityUpdateFlags>(UpdateFlags | UPDATE_FLAGS_PARENT);
    }

    if (UpdateFlags != 0 && SpaceEntity.GetEntityUpdateCallback() != nullptr)
    {
        SpaceEntity.GetEntityUpdateCallback()(&SpaceEntity, UpdateFlags, ComponentUpdates);
    }
}

void SpaceEntityStatePatcher::SetPatchSentCallback(PatchSentCallback Callback) { EntityPatchSentCallback = Callback; }

SpaceEntityStatePatcher::PatchSentCallback SpaceEntityStatePatcher::GetEntityPatchSentCallback() { return EntityPatchSentCallback; }

void SpaceEntityStatePatcher::CallEntityPatchSentCallback(bool Success) { EntityPatchSentCallback(Success); }

} // namespace csp::multiplayer
