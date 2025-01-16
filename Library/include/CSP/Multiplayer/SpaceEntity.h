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
#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/IEntitySerialiser.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include "CSP/ThirdPartyPlatforms.h"
#include "SpaceEntitySystem.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

CSP_START_IGNORE
#ifdef CSP_TESTS
class CSPEngine_SerialisationTests_SpaceEntityUserSignalRSerialisationTest_Test;
class CSPEngine_SerialisationTests_SpaceEntityUserSignalRSerialisationTest_Test;
class CSPEngine_SerialisationTests_SpaceEntityObjectSignalRDeserialisationTest_Test;
class CSPEngine_SerialisationTests_SpaceEntityObjectSignalRDeserialisationTest_Test;
class CSPEngine_SerialisationTests_MapDeserialisationTest_Test;
#endif
CSP_END_IGNORE

namespace csp::multiplayer
{
class SpaceEntitySystem;
class EntityScript;
class EntityScriptInterface;

/// @brief Enum used to specify the the type of a space entity
///
/// Note that this specifically starts from 1 as 0 is reserved for internal purposes.
/// Any additions should not use 0.
enum class SpaceEntityType
{
    Avatar = 1,
    Object,
};

/// @brief This Enum should be used to determine what kind of operation the component update represents.
/// Update means properties on the component have updated, all need to be checked as we do not provide reference of specific property updates.
/// Add means the component is newly added, clients should ensure that this triggers appropriate instantiation of wrapping objects.
/// All properties for the component should be included.
/// Delete means the component has been marked for deletion. It is likely that some other clients will not have the component at the point this is
/// recieved. Any wrapping data objects should be deleted when this is recieved, and clients should cease updating this component as any call would
/// fail. The CSP representation of the component has been removed at this point.
enum class ComponentUpdateType
{
    Update,
    Add,
    Delete,
};

/// @brief Info class that specifies a type of update and the ID of a component the update is applied to.
class CSP_API ComponentUpdateInfo
{
public:
    uint16_t ComponentId;
    ComponentUpdateType UpdateType;
};

/// @brief Enum used to specify what part of a SpaceEntity was updated when deserialising.
/// Use this to determine which parts of an entity to copy values from when an update occurs.
/// It is a bitwise flag enum, so values are additive, the value may represent several flags.
enum SpaceEntityUpdateFlags
{
    UPDATE_FLAGS_NAME = 1,
    UPDATE_FLAGS_POSITION = 2,
    UPDATE_FLAGS_ROTATION = 4,
    UPDATE_FLAGS_SCALE = 8,
    UPDATE_FLAGS_COMPONENTS = 16,
    UPDATE_FLAGS_SELECTION_ID = 32,
    UPDATE_FLAGS_THIRD_PARTY_REF = 64,
    UPDATE_FLAGS_THIRD_PARTY_PLATFORM = 128,
    UPDATE_FLAGS_PARENT = 256,
};

/// @brief Primary multiplayer object that can have associated scripts and many multiplayer components created within it.
class CSP_API SpaceEntity
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceEntitySystem;
    friend class EntityScript;
    friend class EntityScriptInterface;
    friend class EntitySystemScriptInterface;
    friend class ComponentBase;
    friend class ComponentScriptInterface;
#ifdef CSP_TESTS
    friend class ::CSPEngine_SerialisationTests_SpaceEntityUserSignalRSerialisationTest_Test;
    friend class ::CSPEngine_SerialisationTests_SpaceEntityUserSignalRDeserialisationTest_Test;
    friend class ::CSPEngine_SerialisationTests_SpaceEntityObjectSignalRSerialisationTest_Test;
    friend class ::CSPEngine_SerialisationTests_SpaceEntityObjectSignalRDeserialisationTest_Test;
    friend class ::CSPEngine_SerialisationTests_MapDeserialisationTest_Test;
#endif
    /** @endcond */
    CSP_END_IGNORE

public:
    // Callback used when patch messages are received.
    typedef std::function<void(SpaceEntity*, SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>&)> UpdateCallback;

    // Callback used when entity is destroyed.
    typedef std::function<void(bool)> DestroyCallback;

    // General callback providing success/fail boolean.
    typedef std::function<void(bool)> CallbackHandler;

    // Callback that will provide a pointer to a SpaceEntity object.
    typedef std::function<void(SpaceEntity*)> EntityCreatedCallback;

    /// @brief Creates a default instance of a SpaceEntity.
    SpaceEntity();

    /// @brief Creates a SpaceEntity instance using the space entity system provided.
    SpaceEntity(SpaceEntitySystem* InEntitySystem);

    /// @brief Destroys the SpaceEntity instance.
    ~SpaceEntity();

    /// @brief Get the ID of this SpaceEntity, this is generated by the endpoints and should be unique to each Entity.
    /// @return The uint64_t ID of the SpaceEntity.
    uint64_t GetId() const;

    /// @brief Get the ClientID of the owner of the SpaceEntity.
    ///
    /// This starts as the user that creates the Entity but can change if another user patches the Entity.
    ///
    /// @return the uint64_t ClientID of the owner of the SpaceEntity.
    uint64_t GetOwnerId() const;

    /// @brief Get the name set for this SpaceEntity.
    /// @return Name.
    const csp::common::String& GetName() const;

    /// @brief Set the name of the SpaceEntity.
    /// @param Value csp::common::String : The name to set.
    void SetName(const csp::common::String& Value);

    /// @brief Get the SpaceTransform of the SpaceEntity.
    /// @return SpaceTransform.
    const SpaceTransform& GetTransform() const;

    /// @brief Get the Global SpaceTransform of the SpaceEntity, derived from it's parent.
    /// @return SpaceTransform.
    SpaceTransform GetGlobalTransform() const;

    /// @brief Get the position of the SpaceEntity, in world space.
    /// @return Position.
    const csp::common::Vector3& GetPosition() const;

    /// @brief Get the Global position of the SpaceEntity, in world space, derived from it's parent.
    /// @return Position.
    csp::common::Vector3 GetGlobalPosition() const;

    /// @brief Set the position of the SpaceEntity, in world space.
    /// @param Value csp::common::Vector3 : The position to set.
    void SetPosition(const csp::common::Vector3& Value);

    /// @brief Get the rotation of the SpaceEntity.
    /// @return Rotation.
    const csp::common::Vector4& GetRotation() const;

    /// @brief Get the Global rotation of the SpaceEntity, derived from it's parent.
    /// @return Rotation.
    csp::common::Vector4 GetGlobalRotation() const;

    /// @brief Set the rotation of the SpaceEntity.
    /// @param Value csp::common::Vector4 : The rotation to set.
    void SetRotation(const csp::common::Vector4& Value);

    /// @brief Get the scale of the SpaceEntity.
    /// @return Scale.
    const csp::common::Vector3& GetScale() const;

    /// @brief Get the Global scale of the SpaceEntity, derived from it's parent.
    /// @return Scale.
    csp::common::Vector3 GetGlobalScale() const;

    /// @brief Set the scale of the SpaceEntity.
    /// @param Value csp::common::Vector3 : The scale to set.
    void SetScale(const csp::common::Vector3& Value);

    /// @brief Get whether the space is transient or persistant.
    /// @return returns True if the space is transient and false if it is marked as persistant.
    bool GetIsTransient() const;

    /// @brief Get the third party reference of this entity.
    /// @return A string representing the third party reference set for this entity.
    const csp::common::String& GetThirdPartyRef() const;

    /// @brief Set the third party reference for this entity
    /// @param InThirdPartyRef csp::common::String : The third party reference to set.
    void SetThirdPartyRef(const csp::common::String& InThirdPartyRef);

    /// @brief Get the third party platform type of this entity.
    /// @return A string representing third party platform type set for this entity.
    const csp::systems::EThirdPartyPlatform GetThirdPartyPlatformType() const;

    /// @brief Set third party platform type for this entity.
    /// @param InThirdPartyPlatformType csp::systems::EThirdPartyPlatform : The third party platform type to set.
    void SetThirdPartyPlatformType(const csp::systems::EThirdPartyPlatform InThirdPartyPlatformType);

    /// @brief Get the type of the Entity.
    /// @return The SpaceEntityType enum value.
    SpaceEntityType GetEntityType() const;

    /// @brief Get SpaceEntitySystem Object
    /// @return SpaceEntitySystem
    SpaceEntitySystem* GetSpaceEntitySystem();

    /// @brief Sets the parent for this entity
    /// QueueUpdate() should be called afterwards to enable changes to the parent.
    /// @param ParentId uint64_t The new parent id of this entity.
    void SetParentId(uint64_t ParentId);

    /// @brief Removes the parent entity
    /// QueueUpdate() should be called afterwards to enable changes to the parent.
    void RemoveParentEntity();

    /// @brief Gets the parent of this entity
    /// @return SpaceEntity
    SpaceEntity* GetParentEntity() const;

    /// @brief Create a new entity with this entity as it's parent
    /// @param InName csp::common::String : The name to give the new SpaceEntity.
    /// @param InSpaceTransform SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param Callback EntityCreatedCallback : A callback that executes when the creation is complete,
    /// which contains a pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT void CreateChildEntity(
        const csp::common::String& InName, const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback);

    /// @brief Gets the children of this entity
    /// @return csp::common::List<SpaceEntity>
    const csp::common::List<SpaceEntity*>* GetChildEntities() const;

    /// @brief Queues an update which will be executed on next Tick() or ProcessPendingEntityOperations(). Not a blocking or async function.
    void QueueUpdate();

    /// @brief Sends a patch message with a flag to destroy the entity.
    ///
    /// Will remove the entity from endpoints and signal remote clients to delete the entity.
    /// Note this will trigger local deletion of the SpaceEntity immediately, without considering if remotes were able to also delete.
    /// If the endpoint fails to process this message, the client that called this function will be out of sync.
    /// It is advised to handle this situation by dropping the client out of a space if the callback comes back as failed.
    ///
    /// @param Callback CallbackHandler : The callback triggers when the patch message completes, either successfully or unsuccessfully.
    CSP_ASYNC_RESULT void Destroy(CallbackHandler Callback);

    /// @brief Set a callback to be executed when a patch message is received for this Entity. Only one callback can be set.
    /// @param Callback UpdateCallback : Contains the SpaceEntity that updated, a set of flags to tell which parts updated
    /// and an array of information to tell which components updated.
    /// When this callback is recieved, the flags and arrays should be used to determine which properties have updated data.
    CSP_EVENT void SetUpdateCallback(UpdateCallback Callback);

    /// @brief Set a callback to be executed when a patch message with a destroy flag is received for this Entity. Only one callback can be set.
    /// @param Callback DestroyCallback : Contains a bool that is true if the Entity is being deleted.
    CSP_EVENT void SetDestroyCallback(DestroyCallback Callback);

    /// @brief Set a callback to be executed when a patch message queued for the entity is sent. Only one callback can be set.
    /// @param Callback CallbackHandler : Contains a bool that is true when the patch message is sent.
    CSP_EVENT void SetPatchSentCallback(CallbackHandler Callback);

    /// @brief Get the map of components on this SpaceEntity.
    /// @return A map of components indexed with the component ID.
    const csp::common::Map<uint16_t, ComponentBase*>* GetComponents() const;

    /// @brief Get a component on this SpaceEntity by the specified key.
    /// @param Key uint16_t : The component ID for the desired component.
    /// @return The component if found or nullptr if not found.
    ComponentBase* GetComponent(uint16_t Key);

    /// @brief Add a component of the given type.
    /// @param Type ComponentType : The type of component to add.
    /// @return The newly created component.
    ComponentBase* AddComponent(ComponentType Type);

    /// @brief Remove a component of the given key.
    ///
    /// Note that the component cannot currently truly be removed from the server data,
    /// the best we can do is add a blank component in its place, which clients decide to
    /// ignore when retrieving data.
    ///
    /// @param Key uint16_t : The component ID of the component to remove.
    void RemoveComponent(uint16_t Key);

    /// @brief Serialise local changes into patch message format into the given serialiser. Does not send a patch.
    /// @param Serialiser IEntitySerialiser : The serialiser to use.
    void SerialisePatch(IEntitySerialiser& Serialiser) const;

    /// @brief Serialise the entire SpaceEntity into object message format into the given serialiser. Does not send a message.
    /// @param Serialiser IEntitySerialiser : The serialiser to use.
    void Serialise(IEntitySerialiser& Serialiser);

    /// @brief Serialises a given component into a consistent format for the given serialiser.
    /// @param Serialiser IEntitySerialiser : The serialiser to use.
    /// @param Component ComponentBase : The component to be serialised.
    void SerialiseComponent(IEntitySerialiser& Serialiser, ComponentBase* Component) const;

    /// @brief Using the given deserialiser, populate the SpaceEntity with the data in the deserialiser.
    /// @param Deserialiser IEntityDeserialiser : The deserialiser to use.
    void Deserialise(IEntityDeserialiser& Deserialiser);

    /// @brief Gets the script associated with the space entity.
    /// @return The EntityScript instance set on the entity.
    csp::multiplayer::EntityScript* GetScript();

    /// @brief Returns the selection state of the entity.
    /// @return Selection state of the entity, Selected = True, Deselected = False.
    [[nodiscard]] bool IsSelected() const;

    /// @brief Retrieve the ClientID for the Selecting Client.
    /// @return The client ID of the selecting client. Deselected Entity = 0.
    [[nodiscard]] uint64_t GetSelectingClientID() const;

    /// @brief Select the Entity. Only works if the Entity is currently deselected.
    /// @return True if selection occurred. False if not.
    bool Select();

    /// @brief Deselect the Entity.
    ///
    /// Only works if:
    /// - The Entity is currently selected
    /// - The Client attempting to deselect has the same ClientID as the one who selected it
    ///
    /// @return True if deselection occurred. False if not.
    bool Deselect();

    /// @brief Checks if the entity can be modified.
    /// Specifically whether the local client already owns the entity or can take ownership of the entity.
    /// @return True if the entity can be modified, False if not.
    bool IsModifiable();

private:
    class DirtyComponent
    {
    public:
        ComponentBase* Component;
        ComponentUpdateType UpdateType;
    };

    void DeserialiseFromPatch(IEntityDeserialiser& Deserialiser);
    void ApplyLocalPatch(bool InvokeUpdateCallback = true);
    uint16_t GenerateComponentId();
    ComponentBase* InstantiateComponent(uint16_t Id, ComponentType Type);
    void AddDirtyComponent(ComponentBase* DirtyComponent);

    void AddRef();
    void RemoveRef();
    std::atomic_int* GetRefCount();

    void OnPropertyChanged(ComponentBase* DirtyComponent, int32_t PropertyKey);
    csp::multiplayer::EntityScriptInterface* GetScriptInterface();

    void ClaimScriptOwnership();
    void MarkForUpdate();

    // Do NOT call directly, always call either Select() Deselect() or SpaceEntitySystem::InternalSetSelectionStateOfEntity()
    bool InternalSetSelectionStateOfEntity(const bool SelectedState, uint64_t ClientID);

    void DestroyComponent(uint16_t Key);

    ComponentBase* FindFirstComponentOfType(ComponentType Type, bool SearchDirtyComponents = false) const;

    void AddChildEntitiy(SpaceEntity* ChildEntity);

    void ResolveParentChildRelationship();

    SpaceEntitySystem* EntitySystem;

    SpaceEntityType Type;
    uint64_t Id;
    bool IsTransferable;
    bool IsPersistant;
    uint64_t OwnerId;
    csp::common::Optional<uint64_t> ParentId;
    bool ShouldUpdateParent;

    csp::common::String Name;
    SpaceTransform Transform;
    csp::systems::EThirdPartyPlatform ThirdPartyPlatform;
    csp::common::String ThirdPartyRef;
    uint64_t SelectedId;

    SpaceEntity* Parent;
    csp::common::List<SpaceEntity*> ChildEntities;

    UpdateCallback EntityUpdateCallback;
    DestroyCallback EntityDestroyCallback;
    CallbackHandler EntityPatchSentCallback;

    csp::common::Map<uint16_t, ComponentBase*> Components;
    csp::common::Map<uint16_t, ReplicatedValue> DirtyProperties;
    csp::common::Map<uint16_t, DirtyComponent> DirtyComponents;
    uint16_t NextComponentId;

    csp::multiplayer::EntityScript* Script;
    csp::multiplayer::EntityScriptInterface* ScriptInterface;

    std::mutex* EntityLock;
    std::mutex* ComponentsLock;
    std::mutex* PropertiesLock;

    std::atomic_int* RefCount;

    csp::common::List<uint16_t> TransientDeletionComponentIds;

    std::chrono::milliseconds TimeOfLastPatch;
};

} // namespace csp::multiplayer
