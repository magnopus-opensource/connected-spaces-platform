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
#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/PatchTypes.h"
#include "MCS/MCSTypes.h"
#include "Multiplayer/MCSComponentPacker.h"
#include "Multiplayer/SpaceEntityKeys.h"

#include <cstdint>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>

namespace csp::common
{
class IJSScriptRunner;
class IRealtimeEngine;
}

namespace csp::multiplayer
{

class SpaceEntity;
class ComponentBase;

/*
    Object used to define information needed for a property to be replicated.
    This creates a single-point property registration to remove the need to update replication logic in multiple locations.
    When creating a new entity variable that should be replicated:
        - The ToReplicatedValue function should constuct the given ReplicatedValue using the variable.
        - The FromReplicatedValue function should set the variable using the ReplicatedValue.
    This allows external code to agnostically set and get these variables through ReplicatedValues.
    *** Note *** Ensure FromReplicatedValue is thread-safe, as this may be called on a different thread.
*/
class EntityProperty
{
public:
    EntityProperty() = default;

    EntityProperty(SpaceEntityComponentKey Key, SpaceEntityUpdateFlags UpdateFlag, std::function<csp::common::ReplicatedValue()> ToReplicatedValue,
        std::function<void(const csp::common::ReplicatedValue&)> FromReplicatedValue)
        : Key { Key }
        , UpdateFlag { UpdateFlag }
        , ToReplicatedValue { ToReplicatedValue }
        , FromReplicatedValue { FromReplicatedValue }
    {
    }

    // Sets this entity property to the given value.
    // This internally calls the specified FromReplicatedValue function.
    void Set(const csp::common::ReplicatedValue& RepValue);

    // Gets this entity property as a ReplicatedValue.
    // This internally calls the specified ToReplicatedValue.
    csp::common::ReplicatedValue Get() const;

    // Returns the unique identifier defined for this property.
    // This allows us to keep track of the property when it is replicated.
    // These keys are currently defined in SpaceEntityKeys.h
    SpaceEntityComponentKey GetKey() const { return Key; }

    // Returns the enum used for specifying which entity property has been updated to callers.
    // These are passed to callers through the SpaceEntity::UpdateCallbacks.
    SpaceEntityUpdateFlags GetUpdateFlag() const { return UpdateFlag; }

private:
    SpaceEntityComponentKey Key;
    SpaceEntityUpdateFlags UpdateFlag;
    std::function<csp::common::ReplicatedValue()> ToReplicatedValue;
    std::function<void(const csp::common::ReplicatedValue&)> FromReplicatedValue;
};

/*
Object to manage patch based dirty property state management on SpaceEntity
A compositional component on SpaceEntity, we  defer to this behaviour if this type exists, otherwise, we do more basic synchronous sets.
Put another way, this deals with updating SpaceEntities in a way that is compatible with receiving async online messages.
This is a bit of a compromise in order to keep debt isolated, the "true" solution to this is to make SpaceEntity a pure data-transfer class, and have
the complex logic isolated to OnlineRealtimeEngine, which is actually responsible for all this nonsense.

The way this works is,
 - on `SpaceEntity`, you will call a `SetX` method, say `SetPosition`
 - If the entity has a patcher (this object, it will have one if the engine is online)
    - Add a dirty position property to the patcher
    - These properties are used to construct patches, that are sent over the network
    - When the patches are being applied locally (ApplyLocalPatch), `SetPositionDirect(false)` will be called for the property
    - Note the `false` argument, this does not call the callback. ApplyLocalPatch gathers all the changes via a bitset, calls the callback only once.
    - Dirty properties are then cleared, rinse and repeat.
 - Else, directly call SetPositionDirect(true) which sets the position and calls the callback. This is Synchronous, as opposed to the above which
   happens in response to SignalR events)

The state of the patcher is more-or-less exactly representative of a "Patch".
*/

class SpaceEntityStatePatcher
{
public:
    // Success/fail callback for when a patch is sent
    typedef std::function<void(bool)> PatchSentCallback;

    SpaceEntityStatePatcher(csp::common::LogSystem* LogSystem, csp::multiplayer::SpaceEntity& SpaceEntity);

    class DirtyComponent
    {
    public:
        ComponentBase* Component;
        ComponentUpdateType UpdateType;
    };

    bool SetDirtyComponent(uint16_t ComponentKey, DirtyComponent DirtyComponent);
    bool RemoveDirtyComponent(uint16_t ComponentKey, const csp::common::Map<uint16_t, ComponentBase*>& CurrentComponents);

    // Sometimes, we need to use different types than internal storage ... non-ideal. (Motivating example was selection id's needing to be int64's in
    // patches but are stored as uint64s)
    template <typename T, typename U> bool SetDirtyProperty(SpaceEntityComponentKey PropertyKey, const T& PriorValue, const U& NewValue)
    {
        std::scoped_lock<std::mutex> PropertiesLocker(DirtyPropertiesLock);

        // We're not 100% sure, but this erase was likely put here for a very specific case where:
        // A value was changed, but before a patch is sent,
        // the value is set back to its original value.
        // This will prevent a redundant patch from being sent.
        DirtyProperties.erase(PropertyKey);

        if (NewValue != static_cast<U>(PriorValue))
        {
            DirtyProperties[PropertyKey] = csp::common::ReplicatedValue(NewValue);
            return true;
        }
        else
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::VeryVerbose, "Attempting to set dirty property to identical value, ignoring.");
            }
            return false;
        }
    }

    //@return First: Flags of the components that got updated by the patch (as represented by the dirty properties on this type)
    //        Second: Record of all component updates made in the patch application
    [[nodiscard]] std::pair<SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>> ApplyLocalPatch();

    std::unordered_map<SpaceEntityComponentKey, csp::common::ReplicatedValue> GetDirtyProperties() const;
    std::unordered_map<uint16_t, DirtyComponent> GetDirtyComponents() const;

    std::chrono::milliseconds GetTimeOfLastPatch() const;
    void SetTimeOfLastPatch(std::chrono::milliseconds NewTimeOfLastPatch);

    csp::common::Optional<csp::common::Optional<uint64_t>> GetNewParentId() const;
    void SetNewParentId(csp::common::Optional<uint64_t> NewParentId);

    bool HasPendingPatch() const;

    csp::multiplayer::ComponentBase* GetFirstPendingComponentOfType(csp::multiplayer::ComponentType Type,
        std::set<ComponentUpdateType> InterestingUpdateTypes
        = { ComponentUpdateType::Add, ComponentUpdateType::Update, ComponentUpdateType::Delete }) const;

    [[nodiscard]] mcs::ObjectMessage CreateObjectMessage() const;
    [[nodiscard]] mcs::ObjectPatch CreateObjectPatch() const;

    [[nodiscard]] static std::unique_ptr<csp::multiplayer::SpaceEntity> NewFromObjectMessage(const mcs::ObjectMessage& Message,
        csp::common::IRealtimeEngine& RealtimeEngine, csp::common::IJSScriptRunner& ScriptRunner, csp::common::LogSystem& LogSystem);

    // Apply the data inside the object patch to the space entity this patcher relates to.
    void ApplyPatchFromObjectPatch(const mcs::ObjectPatch& Patch);

    // Patch sent callback, invoked from OnlineRealtimeEngine
    void SetPatchSentCallback(PatchSentCallback Callback);
    SpaceEntityStatePatcher::PatchSentCallback GetEntityPatchSentCallback();
    void CallEntityPatchSentCallback(bool Success);

    // These add entity properties to the patchers map to be able to set and get replicated entity variables for patches,
    // without having to know about individual entity variables.
    void RegisterProperty(const EntityProperty& Property);
    void RegisterProperties(const csp::common::Array<EntityProperty>& Properties);

private:
    CSP_START_IGNORE
    mutable std::mutex DirtyPropertiesLock;
    mutable std::mutex DirtyComponentsLock;
    CSP_END_IGNORE

    std::unordered_map<SpaceEntityComponentKey, csp::common::ReplicatedValue> DirtyProperties;
    std::unordered_map<uint16_t, DirtyComponent> DirtyComponents;
    csp::common::List<uint16_t> TransientDeletionComponentIds;
    std::chrono::milliseconds TimeOfLastPatch;

    // Container of EntityProperties, which are proxy types that allow us to get and set specific replicatable
    // values on a SpaceEntity. Populated via RegisterProperty/RegisterProperties.
    std::unordered_map<SpaceEntityComponentKey, EntityProperty> RegisteredProperties;

    // Weird eh?
    // The deal here is that we need to know :
    // 1. If there is a "new" parent ID, ie, is there a change
    // 2. Is that "new" ID an actual parent ID, or empty to mean "no parent"
    // That's why there's these nested optionals
    csp::common::Optional<csp::common::Optional<uint64_t>> NewParentId = nullptr;

    csp::common::LogSystem* LogSystem = nullptr; // May be null
    csp::multiplayer::SpaceEntity& SpaceEntity;
    PatchSentCallback EntityPatchSentCallback;
};

} // namespace csp::multiplayer
