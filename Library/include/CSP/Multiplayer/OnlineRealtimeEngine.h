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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"

#include "CSP/CSPCommon.h"
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/List.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"

#include <chrono>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <set>

namespace async
{
CSP_START_IGNORE
template <typename T> class task;
template <typename T> class shared_task;
template <typename T> class event_task;
CSP_END_IGNORE
}

namespace signalr
{
class value;
} // namespace signalr

class CSPEngine_OnlineRealtimeEngineTests_TestErrorInRemoteGenerateNewAvatarId_Test;
class CSPEngine_OnlineRealtimeEngineTests_TestSuccessInRemoteGenerateNewAvatarId_Test;
class CSPEngine_OnlineRealtimeEngineTests_TestErrorInSendNewAvatarObjectMessage_Test;
class CSPEngine_OnlineRealtimeEngineTests_TestSuccessInSendNewAvatarObjectMessage_Test;
class CSPEngine_OnlineRealtimeEngineTests_TestSuccessInCreateNewLocalAvatar_Test;
class CSPEngine_MultiplayerTests_ManyEntitiesTest_Test;

namespace csp::common
{
class LogSystem;
class LoginState;
}

namespace csp::common::events
{
class Event;
}

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{

class ClientElectionManager;
class MultiplayerConnection;
class ISignalRConnection;
class NetworkEventBus;

/// @brief Class for creating and managing multiplayer objects known as space entities.
///
/// This provides functions to create and manage multiple player avatars and other objects.
/// It manages things like queueing updated entities and triggering tick events. Callbacks
/// can be registered for certain events that occur within the entity system so clients can
/// react appropriately.
class CSP_API OnlineRealtimeEngine : public csp::common::IRealtimeEngine
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class CSPEngine_OnlineRealtimeEngineTests_TestErrorInRemoteGenerateNewAvatarId_Test;
    friend class CSPEngine_OnlineRealtimeEngineTests_TestSuccessInRemoteGenerateNewAvatarId_Test;
    friend class CSPEngine_OnlineRealtimeEngineTests_TestErrorInSendNewAvatarObjectMessage_Test;
    friend class CSPEngine_OnlineRealtimeEngineTests_TestSuccessInSendNewAvatarObjectMessage_Test;
    friend class CSPEngine_OnlineRealtimeEngineTests_TestSuccessInCreateNewLocalAvatar_Test;
    friend class CSPEngine_MultiplayerTests_ManyEntitiesTest_Test;
    /** @endcond */
    CSP_END_IGNORE

public:
    // Callback that will provide a pointer to a SpaceEntity object.
    typedef std::function<void(SpaceEntity*)> EntityCreatedCallback;

    /// @brief OnlineRealtimeEngine constructor
    /// @param InMultiplayerConnection MultiplayerConnection* : the multiplayer connection to construct the OnlineRealtimeEngine with
    /// @param LogSystem csp::common::LogSystem : Logger such that this system can print status and debug output
    /// @param NetworkEventBus csp::multiplayer::NetworkEventBus& : Reference the the network event bus, used for leadership election messaging.
    /// @param RemoteScriptRunner csp::common::IJSScriptRunner& : Object capable of running a script. Called to execute scripts when the leader
    /// election system
    OnlineRealtimeEngine(MultiplayerConnection& InMultiplayerConnection, csp::common::LogSystem& LogSystem,
        csp::multiplayer::NetworkEventBus& NetworkEventBus, csp::common::IJSScriptRunner& RemoteScriptRunner);

    /// @brief OnlineRealtimeEngine destructor
    CSP_NO_EXPORT ~OnlineRealtimeEngine();

    /********************** REALTIME ENGINE INTERFACE ************************/
    /*************************************************************************/

    /// @brief Returns the concrete type of the instantiation of the abstract IRealtimeEngine.
    virtual csp::common::RealtimeEngineType GetRealtimeEngineType() const override;

    /***** ENTITY MANAGEMENT *************************************************/

    /// @brief Create and add a SpaceEntity with type Avatar, and relevant components and default states as specified.
    /// @param Name csp::common::String : The entity name of the newly created avatar entity.
    /// @param UserId csp::common::String : The Id of the user creating the avatar. This can be fetched from csp::systems::UserSystem::GetLoginState
    /// @param SpaceTransform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param State csp::multiplayer::AvatarState : The initial Avatar State to set.
    /// @param AvatarId csp::common::String : The ID to be set on the AvatarSpaceComponent
    /// @param AvatarPlayMode csp::multiplayer::AvatarPlayMode : The Initial AvatarPlayMode to set.
    /// @param Callback csp::multiplayer::EntityCreatedCallback A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT virtual void CreateAvatar(const csp::common::String& Name, const csp::common::String& UserId,
        const csp::multiplayer::SpaceTransform& SpaceTransform, bool IsVisible, csp::multiplayer::AvatarState State,
        const csp::common::String& AvatarId, csp::multiplayer::AvatarPlayMode AvatarPlayMode,
        csp::multiplayer::EntityCreatedCallback Callback) override;

    /// @brief Create and add a SpaceEntity, with relevant default values.
    /// @param Name csp::common::String : The name of the newly created SpaceEntity.
    /// @param SpaceTransform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param ParentID csp::common::Optional<int64_t> : ID of another entity in the space that this entity should be created as a child to. If empty,
    /// entity is created as a root entity.
    /// @param Callback csp::multiplayer::EntityCreatedCallback : A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT virtual void CreateEntity(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& SpaceTransform,
        const csp::common::Optional<uint64_t>& ParentID, csp::multiplayer::EntityCreatedCallback Callback) override;

    /// @brief Add a new entity to the system.
    ///
    /// This can be called at any time from any thread and internally add the entity to a pending
    /// list which is then updated in a thread safe manner when ProcessPendingEntityOperations
    /// is called from the main thread.
    ///
    /// @param EntityToAdd SpaceEntity : Pointer to the entity to be added.
    void AddEntity(SpaceEntity* EntityToAdd) override;

    /// @brief Destroy the specified entity.
    /// @param Entity csp::multiplayer::SpaceEntity : A non-owning pointer to the entity to be destroyed.
    /// @param Callback csp::multiplayer::CallbackHandler : A callback that executes when the entity destruction is complete.
    CSP_ASYNC_RESULT virtual void DestroyEntity(csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::CallbackHandler Callback) override;

    /// @brief Adds an entity to the set of selected entities
    /// @param Entity csp::multiplayer::SpaceEntity* Entity to set as selected
    /// @return True if the entity was succesfully added, false if the entity already existed in the selection and thus could not be added.
    CSP_NO_EXPORT virtual bool AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* Entity) override;

    /// @brief Removes an entity to the set of selected entities
    /// @param Entity csp::multiplayer::SpaceEntity* Entity to set as selected
    /// @return True if the entity was succesfully removed, false if the entity did not exist in the selection and thus could not be removed.
    CSP_NO_EXPORT virtual bool RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* Entity) override;

    /***** ENTITY ACCESS *****************************************************/

    /// @brief Finds the first found SpaceEntity of a matching Name.
    /// @param Name csp::common::String : The name to search.
    /// @return A non-owning pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceEntity(const csp::common::String& Name) override;

    /// @brief Finds the first found SpaceEntity that has the ID EntityId.
    /// @param EntityId uint64_t : The Id to look for.
    /// @return A non-owning pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceEntityById(uint64_t EntityId) override;

    /// @brief Finds the first found SpaceEntity of a matching Name. The found SpaceEntity will contain an AvatarSpaceComponent.
    /// @param Name The name to search for.
    /// @return A pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceAvatar(const csp::common::String& Name) override;

    /// @brief Finds the first found SpaceEntity of a matching Name. The found SpaceEntity will not contain an AvatarSpaceComponent.
    /// @param Name The name to search for.
    /// @return A pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceObject(const csp::common::String& Name) override;

    /// @brief Get an Entity by its index.
    ///
    /// @param EntityIndex size_t : The index of the entity to get.
    /// @return A non-owning pointer to the entity at the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetEntityByIndex(size_t EntityIndex) override;

    /// @brief Get an Avatar by its index. The returned pointer will be an entity that contains an AvatarSpaceComponent.
    ///
    /// @param AvatarIndex size_t : The index of the avatar entity to get.
    /// @return A non-owning pointer to the avatar entity with the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetAvatarByIndex(size_t AvatarIndex) override;

    /// @brief Get an Object by its index. The returned pointer will be an entity that does not contain an AvatarSpaceComponent.
    ///
    /// @param ObjectIndex size_t : The index of the object entity to get.
    /// @return A non-owning pointer to the object entity with the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetObjectByIndex(size_t ObjectIndex) override;

    /// @brief Get the number of total entities in the system.
    /// @return The total number of entities.
    virtual size_t GetNumEntities() const override;

    /// @brief Get the number of total Avatars in the system. Avatars are entities that contain AvatarSpaceComponents.
    /// @return The total number of Avatar entities.
    virtual size_t GetNumAvatars() const override;

    /// @brief Get the number of total Objects in the system. Objects are entities that do not contain AvatarSpaceComponents.
    /// @return The total number of object entities.
    virtual size_t GetNumObjects() const override;

    /// @brief Return all the entities currently known to the realtime engine.
    /// @warning This list may be extremely large.
    /// @return A non-owning pointer to a List of non-owning pointers to all entities.
    virtual const csp::common::List<csp::multiplayer::SpaceEntity*>* GetAllEntities() const override;

    /// @brief Retrieves all entities that exist at the root level (do not have a parent entity).
    /// @return A list of root entities containing non-owning pointers to entities.
    [[nodiscard]] virtual const csp::common::List<csp::multiplayer::SpaceEntity*>* GetRootHierarchyEntities() const override;

    /// @brief "Resolves" the entity heirarchy for the given entity, setting all internal parent/child buffers correctly.
    /// This method is called whenever parent/child relationships are changed for a given entity, including when one is first created.
    /// @param Entity csp::multiplayer::SpaceEntity* : The Entity to resolve
    CSP_NO_EXPORT virtual void ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* Entity) override;

    /***** ENTITY PROCESSING *************************************************/

    /**
     * @brief Fetch all entities in the space from MCS
     *
     * Uses SignalR to fetch all the entities from MCS and populate the entities buffer.
     * Also refreshes the scopes in the space, (a potentially redundant action that we are trying to remove),
     * this coincidentally restarts the multiplayer connection, although this should be a purely internal implementation detail.
     *
     * @param SpaceId const csp::common::String& : MCS formatted SpaceId
     * @param FetchStartedCallback EntityFetchStartedCallback Callback called once scopes are reset and entity fetch has begun.
     *
     * @pre Space represented by SpaceId must exist on the configured server endpoint. See csp::systems::SpaceSystem::CreateSpace
     * @post FetchStartedCallback will be called. The csp::common::EntityFetchCompleteCallback passed in the constructor will be called async
     * once all the entities are fetched.
     */
    CSP_NO_EXPORT void FetchAllEntitiesAndPopulateBuffers(
        const csp::common::String& SpaceId, csp::common::EntityFetchStartedCallback FetchStartedCallback) override;

    /// @brief Lock a mutex that guards against any changes to the entity list.
    /// If the mutex is already locked, will wait until it is able to acquire the lock. May cause deadlocks.
    CSP_NO_EXPORT virtual void LockEntityUpdate() override;

    /// @brief Lock a mutex that guards against any changes to the entity list.
    /// @return Whether the mutex successfully locked. The mutex will fail to lock if already locked in order to avoid deadlocks.
    CSP_NO_EXPORT virtual bool TryLockEntityUpdate() override;

    /// @brief Unlock a mutex that guards against any changes to the entity list.
    CSP_NO_EXPORT virtual void UnlockEntityUpdate() override;

    /***** IREALTIMEENGINE INTERFACE IMPLEMENTAITON END *************************************************/

    /// @brief Adds an entity to a list of entities to be updated when ProcessPendingEntityOperations is called.
    /// From a client perspective, ProcessPendingEntityOperations is normally called via the CSPFoundation::Tick method.
    /// @param Entity SpaceEntity : A non-owning pointer to the entity to be marked.
    void QueueEntityUpdate(csp::multiplayer::SpaceEntity* Entity);

    /// @brief Applies any pending changes to entities that have been marked for update.
    void ProcessPendingEntityOperations();

    /// @brief Sets a callback to be executed when a remote entity is created.
    /// To wait for local entities to be created, await the callback provided in the CreateObject/CreateAvatar methods.
    ///
    /// Only one EntityCreatedCallback may be registered, calling this function again will override whatever was previously set.
    ///
    /// @param Callback csp::multiplayer::EntityCreatedCallback : the callback to execute.
    CSP_EVENT void SetRemoteEntityCreatedCallback(csp::multiplayer::EntityCreatedCallback Callback);

    /// @brief Sets a callback to be executed when the script system is ready to run scripts.
    /// @param Callback CallbackHandler : the callback to execute.
    CSP_EVENT void SetScriptLeaderReadyCallback(CallbackHandler Callback);

    /// @brief Sets the script owner for the given entity to the current client
    /// @param Entity SpaceEntity : A pointer to the entity
    void ClaimScriptOwnership(SpaceEntity* Entity) const;

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

    /// @brief Retrieve the state of the patch rate limiter. If true, patches are limited for each individual entity to a fixed rate.
    /// @return True if enabled, false otherwise.
    bool GetEntityPatchRateLimitEnabled() const;

    /// @brief Set the state of the patch rate limiter. If true, patches are limited for each individual entity to a fixed rate.
    ///
    /// This feature is enabled by default and should only be disabled if you are encountering issues.
    ///
    /// @param Enabled : sets if the feature should be enabled or not.
    /// \rst
    ///.. note::
    ///   If disabling this feature, more requests will be made to Magnopus Connected Services,
    ///   and consequently more patch merges may occur on the server as a result.
    /// \endrst
    void SetEntityPatchRateLimitEnabled(bool Enabled);

    /// @brief "Refreshes" (ie, turns on an off again), the multiplayer connection, in order to refresh scopes.
    /// This shouldn't be neccesary, we should devote some effort to checking if it still is at some point
    /// @param SpaceId csp::Common:String& : The Id of the space to refresh
    /// @param RefreshMultiplayerContinuationEvent : std::shared_ptr<async::event_task<std::optional<csp::multiplayer::ErrorCode>>> Continuation event
    /// that populates an optional error code on failure. Error is empty on success.
    CSP_NO_EXPORT void RefreshMultiplayerConnectionToEnactScopeChange(csp::common::String SpaceId,
        std::shared_ptr<async::event_task<std::optional<csp::multiplayer::ErrorCode>>> RefreshMultiplayerContinuationEvent);

    /// @brief Checks whether we should run scripts locally
    /// @return bool
    CSP_NO_EXPORT bool CheckIfWeShouldRunScriptsLocally() const;

    /// @brief Runs the provided script remotely
    /// @param ContextId int64_t : the ID of the context on which to run the script
    /// @param ScriptText csp::common::String& : the text of the script to run
    CSP_NO_EXPORT void RunScriptRemotely(int64_t ContextId, const csp::common::String& ScriptText);

    /// @brief Getter for the pending adds
    /// @return: SpaceEntityQueue*
    CSP_NO_EXPORT std::deque<csp::multiplayer::SpaceEntity*>* GetPendingAdds();

    /// @brief Getter for the multiplayer connection instance
    /// @return: MultiplayerConnection*
    CSP_NO_EXPORT MultiplayerConnection* GetMultiplayerConnectionInstance() const;

    // @brief Ticks all entities and scripts, processing any pending local and remote updates
    // Will only tick scrips if EnableEntityTick is enabled, which it should be if entity fetch has completed.
    CSP_NO_EXPORT void TickEntities();

    /*
     * Called when MultiplayerConnection recieved signalR events.
     */
    CSP_NO_EXPORT void OnObjectMessage(const signalr::value& Params);
    CSP_NO_EXPORT void OnObjectPatch(const signalr::value& Params);
    CSP_NO_EXPORT void OnRequestToSendObject(const signalr::value& Params);

protected:
    csp::common::List<SpaceEntity*> Entities;
    csp::common::List<SpaceEntity*> Avatars;
    csp::common::List<SpaceEntity*> Objects;
    csp::common::List<SpaceEntity*> SelectedEntities;
    csp::common::List<SpaceEntity*> RootHierarchyEntities;

    std::recursive_mutex* EntitiesLock;

private:
    OnlineRealtimeEngine(); // needed for the wrapper generator

    // Should not be null
    MultiplayerConnection* MultiplayerConnectionInst;

    // Should not be null
    csp::common::LogSystem* LogSystem;

    /// @brief Destroys the local client's view of the specified entity.
    /// @param Entity SpaceEntity : The entity to be destroyed locally.
    void LocalDestroyEntity(SpaceEntity* Entity);

    using PatchMessageQueue = std::deque<signalr::value*>;
    using SpaceEntitySet = std::set<SpaceEntity*>;

    EntityCreatedCallback RemoteSpaceEntityCreatedCallback;
    CallbackHandler ScriptSystemReadyCallback;

    void GetEntitiesPaged(int Skip, int Limit, const std::function<void(const signalr::value&, std::exception_ptr)>& Callback);
    std::function<void(const signalr::value&, std::exception_ptr)> CreateRetrieveAllEntitiesCallback(
        int Skip, csp::common::EntityFetchCompleteCallback FetchCompleteCallback);

    // Calls GetEntitiesPaged to start off a paged recursive fetch of all the entities in the space
    void RetrieveAllEntities(csp::common::EntityFetchCompleteCallback FetchCompleteCallback);

    /// Destroy all the entities locally, only used in destruction
    void LocalDestroyAllEntities();

    void RemoveEntity(SpaceEntity* EntityToRemove);

    void AddPendingEntity(SpaceEntity* EntityToAdd);
    void RemovePendingEntity(SpaceEntity* EntityToRemove);
    void ApplyIncomingPatch(const signalr::value*);
    void HandleException(const std::exception_ptr& Except, const std::string& ExceptionDescription);

    bool EntityIsInRootHierarchy(SpaceEntity* Entity);

    void ClaimScriptOwnershipFromClient(uint64_t ClientId);

    void OnAvatarAdd(const SpaceEntity* Avatar, const csp::common::List<SpaceEntity*>& Avatars);
    void OnAvatarRemove(const SpaceEntity* Avatar, const csp::common::List<SpaceEntity*>& Avatars);
    void OnObjectAdd(const SpaceEntity* Object, const csp::common::List<SpaceEntity*>& Entities);
    void OnObjectRemove(const SpaceEntity* Object, const csp::common::List<SpaceEntity*>& Entities);

    void SendPatches(const csp::common::List<SpaceEntity*> PendingEntities);

    // Used in OnObjectMessage as well as in the initial entity fetch. Uses CreateEntity to make entities when instructed to from the server, via
    // signalR message.
    SpaceEntity* CreateRemotelyRetrievedEntity(const signalr::value& EntityMessage);

    // CreateAvatar Continuations
    CSP_START_IGNORE
    async::shared_task<uint64_t> RemoteGenerateNewAvatarId();
    std::function<async::task<std::tuple<signalr::value, std::exception_ptr>>(uint64_t)> SendNewAvatarObjectMessage(const csp::common::String& Name,
        const csp::common::String& UserId, const SpaceTransform& Transform, bool IsVisible, const csp::common::String& AvatarId,
        AvatarState AvatarState, AvatarPlayMode AvatarPlayMode);
    std::function<void(std::tuple<async::shared_task<uint64_t>, async::task<void>>)> CreateNewLocalAvatar(const csp::common::String& Name,
        const csp::common::String& UserId, const SpaceTransform& Transform, bool IsVisible, const csp::common::String& AvatarId,
        AvatarState AvatarState, AvatarPlayMode AvatarPlayMode, EntityCreatedCallback Callback);
    CSP_END_IGNORE

    class EntityScriptBinding* ScriptBinding;
    class SpaceEntityEventHandler* EventHandler;
    class ClientElectionManager* ElectionManager;

    std::recursive_mutex* TickEntitiesLock;

    std::deque<csp::multiplayer::SpaceEntity*>* PendingAdds;
    std::deque<csp::multiplayer::SpaceEntity*>* PendingRemoves;
    std::set<csp::multiplayer::SpaceEntity*>* PendingOutgoingUpdateUniqueSet;
    PatchMessageQueue* PendingIncomingUpdates;

    bool EnableEntityTick;
    std::list<SpaceEntity*> TickUpdateEntities;

    std::chrono::system_clock::time_point LastTickTime;
    std::chrono::milliseconds EntityPatchRate;

    bool EntityPatchRateLimitEnabled = true;

    // May not be null
    csp::common::IJSScriptRunner* ScriptRunner;
    // May not be null
    csp::multiplayer::NetworkEventBus* NetworkEventBus;
};

} // namespace csp::multiplayer
