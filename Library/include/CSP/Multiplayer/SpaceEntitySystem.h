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
#include "CSP/Common/List.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/EventParameters.h"

#include <chrono>
#include <deque>
#include <functional>
#include <list>
#include <mutex>
#include <set>

namespace signalr
{

class value;

} // namespace signalr

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

class SpaceSystem;
class SystemsManager;
class SequenceSystem;

} // namespace csp::systems

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{

class ClientElectionManager;
class MultiplayerConnection;
class SignalRConnection;
class SpaceEntity;
class SpaceTransform;

/// @brief Class for creating and managing multiplayer objects known as space entities.
///
/// This provides functions to create and manage multiple player avatars and other objects.
/// It manages things like queueing updated entities and triggering tick events. Callbacks
/// can be registered for certain events that occur within the entity system so clients can
/// react appropriately.
class CSP_API SpaceEntitySystem
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::systems::SystemsManager;
    friend class csp::systems::SpaceSystem;
    friend class MultiplayerConnection;
    friend class SpaceEntityEventHandler;
    friend class ClientElectionManager;
    friend class EntityScript;
    friend class SpaceEntity;
    friend void csp::memory::Delete<SpaceEntitySystem>(SpaceEntitySystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    // Callback used to provide a success/fail type of response.
    typedef std::function<void(bool)> CallbackHandler;

    // Callback that will provide a pointer to a SpaceEntity object.
    typedef std::function<void(SpaceEntity*)> EntityCreatedCallback;

    /// @brief Creates a SpaceEntity with type Avatar, and relevant components and default states as specified.
    /// @param InName csp::common::String : The name to give the new SpaceEntity.
    /// @param InSpaceTransform SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param InState AvatarState : The initial Avatar State to set.
    /// @param InAvatarId csp::common::String : The Initial AvatarID to set.
    /// @param InAvatarPlayMode AvatarPlayMode : The Initial AvatarPlayMode to set.
    /// @param Callback EntityCreatedCallback A callback that executes when the creation is complete,
    /// which contains a pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT void CreateAvatar(const csp::common::String& InName, const SpaceTransform& InSpaceTransform, AvatarState InState,
        const csp::common::String& InAvatarId, AvatarPlayMode InAvatarPlayMode, EntityCreatedCallback Callback);

    /// @brief Creates a SpaceEntity of type Object, and relevant default values.
    /// @param InName csp::common::String : The name to give the new SpaceEntity.
    /// @param InSpaceTransform SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param Callback EntityCreatedCallback : A callback that executes when the creation is complete,
    /// which contains a pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT void CreateObject(const csp::common::String& InName, const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback);

    /// @brief Destroys both the remote view and the local view of the specified entity.
    /// @param Entity SpaceEntity : The entity to be destroyed.
    /// @param Callback CallbackHandler : the callback to execute.
    CSP_ASYNC_RESULT void DestroyEntity(SpaceEntity* Entity, CallbackHandler Callback);

    /// @brief Destroys the local client's view of the specified entity.
    /// @param Entity SpaceEntity : The entity to be destroyed locally.
    void LocalDestroyEntity(SpaceEntity* Entity);

    /// @brief Finds the first SpaceEntity that matches InName.
    /// @param InName csp::common::String : The name to search.
    /// @return A pointer to the first found match SpaceEntity.
    SpaceEntity* FindSpaceEntity(const csp::common::String& InName);

    /// @brief Finds the first SpaceEntity that has the ID EntityId.
    /// @param EntityId uint64_t : The Id to look for.
    /// @return A pointer to the first found match SpaceEntity.
    SpaceEntity* FindSpaceEntityById(uint64_t EntityId);

    /// @brief Finds the first SpaceEntity of type Avatar that matches InName.
    /// @param InName The name to search.
    /// @return A pointer to the first found match SpaceEntity.
    SpaceEntity* FindSpaceAvatar(const csp::common::String& InName);

    /// @brief Finds the first SpaceEntity of type Object that matches InName.
    /// @param InName The name to search.
    /// @return A pointer to the first found match SpaceEntity.
    SpaceEntity* FindSpaceObject(const csp::common::String& InName);

    /// @brief Locks the entity mutex.
    void LockEntityUpdate() const;

    /// @brief Unlocks the entity mutex.
    void UnlockEntityUpdate() const;

    /// @brief Get the number of total entities in the system (both Avatars and Objects).
    /// @return The total number of entities.
    [[nodiscard]] size_t GetNumEntities() const;

    /// @brief Get the number of total Avatars in the system.
    /// @return The total number of Avatar entities.
    [[nodiscard]] size_t GetNumAvatars() const;

    /// @brief Get the number of total Objects in the system.
    /// @return The total number of object entities.
    [[nodiscard]] size_t GetNumObjects() const;

    /// @brief Get an Entity (Avatar or Object) by its index.
    ///
    /// Note this is not currently thread safe and should only be called from the main thread.
    ///
    /// @param EntityIndex size_t : The index of the entity to get.
    /// @return A pointer to the entity with the given index.
    SpaceEntity* GetEntityByIndex(const size_t EntityIndex);

    /// @brief Get an Avatar by its index.
    ///
    /// Note this is not currently thread safe and should only be called from the main thread.
    ///
    /// @param AvatarIndex size_t : The index of the avatar entity to get.
    /// @return A pointer to the avatar entity with the given index.
    SpaceEntity* GetAvatarByIndex(const size_t AvatarIndex);

    /// @brief Get an Object by its index.
    ///
    /// Note this is not currently thread safe and should only be called from the main thread.
    ///
    /// @param ObjectIndex size_t : The index of the object entity to get.
    /// @return A pointer to the object entity with the given index.
    SpaceEntity* GetObjectByIndex(const size_t ObjectIndex);

    /// @brief Add a new entity to the system.
    ///
    /// This can be called at any time from any thread and internally add the entity to a pending
    /// list which is then updated in a thread safe manner when ProcessPendingEntityOperations
    /// is called from the main thread.
    ///
    /// @param EntityToAdd SpaceEntity : Pointer to the entity to be added.
    void AddEntity(SpaceEntity* EntityToAdd);

    /// @brief Sets a callback to be executed when an entity is remotely created.
    ///
    /// Only one callback may be registered, calling this function again will override whatever was previously set.
    /// If this is not set, some patch functions may fail.
    ///
    /// @param Callback EntityCreatedCallback : the callback to execute.
    CSP_EVENT void SetEntityCreatedCallback(EntityCreatedCallback Callback);

    /// @brief Sets a local pointer to the connection for communication with the endpoints, this should be called as early as possible.
    ///
    /// Note that this is already called in MultiplayerConnection::Connect, so this shouldn't need to be called anywhere else.
    /// This should not be called by client code directly, marked as No Export.
    ///
    /// @param InConnection csp::multiplayer::SignalRConnection : A pointer to the connection object to be used by the system.
    CSP_NO_EXPORT void SetConnection(csp::multiplayer::SignalRConnection* InConnection);

    /// @brief Sets a callback to be executed when all existing entities have been retrieved after entering a space.
    /// @param Callback CallbackHandler : the callback to execute.
    CSP_EVENT void SetInitialEntitiesRetrievedCallback(CallbackHandler Callback);

    /// @brief Sets a callback to be executed when the script system is ready to run scripts.
    /// @param Callback CallbackHandler : the callback to execute.
    CSP_EVENT void SetScriptSystemReadyCallback(CallbackHandler Callback);

    /// @brief Triggers queuing of the SpaceEntities updated components and replicated data.
    ///
    /// Causes the replication of a SpaceEntities data on next Tick() or ProcessPendingEntityOperations(). However, this is bound by an
    /// entities rate limit and will only be replicated if there has been sufficient time since the last time the entity sent a message.
    ///
    /// @param EntityToUpdate SpaceEntity : A pointer to the SpaceEntity to update.
    void QueueEntityUpdate(SpaceEntity* EntityToUpdate);

    /// @brief Processes pending entity operations and then calls tick on scripts if necessary.
    void TickEntities();

    // TODO: OF-1005 This should not be a part of the public API
    void RegisterEntityScriptAsModule(SpaceEntity* NewEntity);

    // TODO: OF-1005 This should not be a part of the public API
    void BindNewEntityToScript(SpaceEntity* NewEntity);

    /// @brief Sets the script owner for the given entity to the current client
    /// @param Entity SpaceEntity : A pointer to the entity
    void ClaimScriptOwnership(SpaceEntity* Entity) const;

    /// @brief Adds the entity to a list of entities to be updated on tick
    /// @param Entity SpaceEntity : A pointer to the entity to be added
    void MarkEntityForUpdate(SpaceEntity* Entity);

    /// @brief Process pending entity adds/removes and Patch message send and receives.
    ///
    /// Note this should only be called from main thread
    void ProcessPendingEntityOperations();

    // TODO: Should this be NO_EXPORT?
    /// @brief Retrieves all entities from the endpoint, calls "GetAllScopedObjects" currently.
    ///
    /// Note this will generate new entity objects for every entity in the current scopes.
    /// If this is called by a client manually without first deleting all existing tracked entities, it is possible there will be duplicates.
    /// It is highly advised not to call this function unless you know what you are doing.
    void RetrieveAllEntities();

    /// @brief Destroys the client's local view of all currently known entities.
    ///
    /// They still reside on the server, however they will not be accessible in the client application.
    void LocalDestroyAllEntities();

    /// @brief Sets the selected state of an entity, if the operation is acceptable.
    ///
    /// Criteria:
    /// For Selection:
    ///   - Entity must be deselected currently
    /// For Deselection:
    ///   - Entity must be selected currently
    ///   - Entity must be selected by the client attempting the deselection (SpaceEntity::GetSelectingClientID will return this information)
    ///
    /// @param SelectedState bool : The state to set the entity to, Selected = True, Deselected = false.
    /// @param Entity SpaceEntity : A pointer to the entity to modify selection state on.
    /// @return True if a selection state change has occurred, false if no change was made (due to one of the above criteria not being met).
    bool SetSelectionStateOfEntity(const bool SelectedState, SpaceEntity* Entity);

    /// @brief Enable Leader Election feature.
    void EnableLeaderElection();

    /// @brief Disable Leader Election feature.
    void DisableLeaderElection();

    /// @brief Check if the Leader Election feature is enabled.
    /// @return true if enabled, false otherwise.
    bool IsLeaderElectionEnabled() const;

    /// @brief Debug helper to get the id of the currently elected script leader.
    /// @return The id of the leader.
    uint64_t GetLeaderId() const;

    /// @brief Finds a component by the given id.
    ///
    /// Searchs through all components of all entites so should be used sparingly.
    ///
    /// @param Id The id of the component to find.
    /// @return A pointer to the found component which can be nullptr if the component is not found.
    ComponentBase* FindComponentById(uint16_t Id);

    /// @brief Retrieve the state of the patch rate limiter. If true, patches are limited for each individual entity to a fixed rate.
    /// @return True if enabled, false otherwise.
    const bool GetEntityPatchRateLimitEnabled() const;

    /// @brief Set the state of the patch rate limiter. If true, patches are limited for each individual entity to a fixed rate.
    ///
    /// This feature is enabled by default and should only be disabled if you are encountering issues.
    ///
    /// @param Enabled : sets if the feature should be enabled or not.
    /// \rst
    ///.. note::
    ///   If disabling this feature, more requests will be made to Magnopus Connected Services,
    ///   and consequntly more patch merges may occur on the server as a result.
    /// \endrst
    void SetEntityPatchRateLimitEnabled(bool Enabled);

    /// @brief Retrieves all entites that exist at the root level (do not have a parent entity).
    /// @return A list of root entities.
    const csp::common::List<SpaceEntity*>* GetRootHierarchyEntities() const;

protected:
    using SpaceEntityList = csp::common::List<SpaceEntity*>;

    SpaceEntityList Entities;
    SpaceEntityList Avatars;
    SpaceEntityList Objects;
    SpaceEntityList SelectedEntities;
    SpaceEntityList RootHierarchyEntities;

    std::recursive_mutex* EntitiesLock;

private:
    SpaceEntitySystem(MultiplayerConnection* InMultiplayerConnection);
    ~SpaceEntitySystem();

    MultiplayerConnection* MultiplayerConnectionInst;
    csp::multiplayer::SignalRConnection* Connection;

    using SpaceEntityQueue = std::deque<SpaceEntity*>;
    using PatchMessageQueue = std::deque<signalr::value*>;
    using SpaceEntitySet = std::set<SpaceEntity*>;

    EntityCreatedCallback SpaceEntityCreatedCallback;
    CallbackHandler InitialEntitiesRetrievedCallback;
    CallbackHandler ScriptSystemReadyCallback;

    void Initialise();
    void Shutdown();

    void BindOnObjectMessage();
    void BindOnObjectPatch();
    void BindOnRequestToSendObject();
    void BindOnRequestToDisconnect() const;

    void GetEntitiesPaged(int Skip, int Limit, const std::function<void(const signalr::value&, std::exception_ptr)>& Callback);
    std::function<void(const signalr::value&, std::exception_ptr)> CreateRetrieveAllEntitiesCallback(int Skip);

    void RemoveEntity(SpaceEntity* EntityToRemove);

    void AddPendingEntity(SpaceEntity* EntityToAdd);
    void RemovePendingEntity(SpaceEntity* EntityToRemove);
    void ApplyIncomingPatch(const signalr::value*);
    void HandleException(const std::exception_ptr& Except, const std::string& ExceptionDescription);

    void OnAllEntitiesCreated();
    void DetermineScriptOwners();

    void ResolveParentChildForDeletion(SpaceEntity* Deletion);
    void ResolveEntityHierarchy(SpaceEntity* Entity);
    bool EntityIsInRootHierarchy(SpaceEntity* Entity);

    void ClaimScriptOwnershipFromClient(uint64_t ClientId);
    bool CheckIfWeShouldRunScriptsLocally() const;
    void RunScriptRemotely(int64_t ContextId, const csp::common::String& ScriptText);
    void TickEntityScripts();

    void OnAvatarAdd(const SpaceEntity* Avatar, const SpaceEntityList& Avatars);
    void OnAvatarRemove(const SpaceEntity* Avatar, const SpaceEntityList& Avatars);
    void OnObjectAdd(const SpaceEntity* Object, const SpaceEntityList& Entities);
    void OnObjectRemove(const SpaceEntity* Object, const SpaceEntityList& Entities);

    void CreateObjectInternal(const csp::common::String& InName, csp::common::Optional<uint64_t> InParent, const SpaceTransform& InSpaceTransform,
        EntityCreatedCallback Callback);

    class EntityScriptBinding* ScriptBinding;
    class SpaceEntityEventHandler* EventHandler;
    class ClientElectionManager* ElectionManager;

    std::mutex* TickEntitiesLock;

    SpaceEntityQueue* PendingAdds;
    SpaceEntityQueue* PendingRemoves;
    SpaceEntitySet* PendingOutgoingUpdateUniqueSet;
    PatchMessageQueue* PendingIncomingUpdates;

    bool EnableEntityTick;
    std::list<SpaceEntity*> TickUpdateEntities;

    std::chrono::system_clock::time_point LastTickTime;
    std::chrono::milliseconds EntityPatchRate;

    bool EntityPatchRateLimitEnabled = true;

    bool IsInitialised = false;
};

} // namespace csp::multiplayer
