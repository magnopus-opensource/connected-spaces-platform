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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"

#include "CSP/CSPCommon.h"
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/List.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/String.h"

#include <memory>
#include <mutex>
#include <set>

class CSPEngine_OfflineRealtimeEngineTests_SelectEntity_Test;

namespace csp::common
{
class LoginState;
class LogSystem;
}

namespace csp::multiplayer
{
class CSPSceneDescription;

/// @brief Class for creating and managing objects in an offline context.
///
/// This provides functionality to create and manage a player avatar and other objects while being offline.
/// The callbacks that are injected into functions are all synchronous, meaning they are called before the funciton ends.
class CSP_API OfflineRealtimeEngine : public csp::common::IRealtimeEngine
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class ::CSPEngine_OfflineRealtimeEngineTests_SelectEntity_Test;
    /** @endcond */
    CSP_END_IGNORE

public:
    // Callback that will provide a pointer to a SpaceEntity object.
    typedef std::function<void(SpaceEntity*)> EntityCreatedCallback;

    /********************** REALTIME ENGINE INTERFACE ************************/
    /*************************************************************************/

    /// @brief Returns the concrete type of the instantiation of the abstract IRealtimeEngine.
    virtual csp::common::RealtimeEngineType GetRealtimeEngineType() const override;

    /***** ENTITY MANAGEMENT *************************************************/

    /// @brief Create and add a SpaceEntity with type Avatar, and relevant components and default states as specified.
    /// @param Name csp::common::String : The entity name of the newly created avatar entity.
    /// @param UserId csp::common::String : The Id of the user creating the avatar. This can be fetched from csp::systems::UserSystem::GetLoginState
    /// @param Transform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param State csp::multiplayer::AvatarState : The initial Avatar State to set.
    /// @param AvatarId csp::common::String : The ID to be set on the AvatarSpaceComponent
    /// @param AvatarPlayMode csp::multiplayer::AvatarPlayMode : The Initial AvatarPlayMode to set.
    /// @param Callback csp::multiplayer::EntityCreatedCallback A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT virtual void CreateAvatar(const csp::common::String& Name, const csp::common::String& UserId,
        const csp::multiplayer::SpaceTransform& Transform, bool IsVisible, csp::multiplayer::AvatarState State, const csp::common::String& AvatarId,
        csp::multiplayer::AvatarPlayMode AvatarPlayMode, csp::multiplayer::EntityCreatedCallback Callback) override;

    /// @brief Create and add a SpaceEntity, with relevant default values.
    /// @param Name csp::common::String : The name of the newly created SpaceEntity.
    /// @param Transform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param ParentID csp::common::Optional<int64_t> : ID of another entity in the space that this entity should be created as a child to. If empty,
    /// entity is created as a root entity.
    /// @param Callback csp::multiplayer::EntityCreatedCallback : A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT virtual void CreateEntity(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& Transform,
        const csp::common::Optional<uint64_t>& ParentID, csp::multiplayer::EntityCreatedCallback Callback) override;

    /// @brief Add a new entity to the system.
    /// This can be called at any time from any thread and internally add the entity to the entities list.
    ///
    /// @param EntityToAdd SpaceEntity : Pointer to the entity to be added.
    void AddEntity(SpaceEntity* EntityToAdd) override;

    /// @brief Destroy the specified entity.
    /// @param Entity csp::multiplayer::SpaceEntity : A non-owning pointer to the entity to be destroyed.
    /// @param Callback csp::multiplayer::CallbackHandler : A callback that executes when the entity destruction is complete.
    CSP_ASYNC_RESULT virtual void DestroyEntity(csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::CallbackHandler Callback) override;

    /// @brief Sets a callback to be executed when an entity is fully created.
    ///
    /// Only one EntityCreatedCallback may be registered, calling this function again will override whatever was previously set.
    ///
    /// @param Callback csp::multiplayer::EntityCreatedCallback : the callback to execute.
    CSP_EVENT virtual void SetEntityCreatedCallback(csp::multiplayer::EntityCreatedCallback Callback) override;

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

    /// @brief Retrieves all entities that exist at the root level (do not have a parent entity).
    /// @return A list of root entities containing non-owning pointers to entities.
    [[nodiscard]] virtual const csp::common::List<csp::multiplayer::SpaceEntity*>* GetRootHierarchyEntities() const override;

    /// @brief Adds the given entity to the hierarchy by updating entity children and root hierarchy.
    /// @param Entity csp::multiplayer::SpaceEntity* : The Entity to add to the hierarchy.
    CSP_NO_EXPORT virtual void ResolveEntityHierarchy(csp::multiplayer::SpaceEntity* Entity) override;

    /***** ENTITY PROCESSING *************************************************/

    /// @brief Adds an entity to a list of entities to be updated when ProcessPendingEntityOperations is called.
    /// From a client perspective, ProcessPendingEntityOperations is normally called via the CSPFoundation::Tick method.
    /// @param Entity SpaceEntity : A non-owning pointer to the entity to be marked.
    virtual void QueueEntityUpdate(csp::multiplayer::SpaceEntity* Entity) override;

    /// @brief Applies any pending changes to entities that have been marked for update.
    /// This only processes changes to existing entities, such as properties or components. All entity creations and deletions are handled instantly.
    virtual void ProcessPendingEntityOperations() override;

    /**
     * @brief TODO: I'm not sure what this function will do yet.
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

    /// @brief OnlineRealtimeEngine constructor
    /// @param SceneDescription CSPSceneDescription : The scene description containing entities within the scene.
    /// These entities will be populated in the RealtimeEngine.
    /// @param LogSystem csp::common::LogSystem : Logger such that this system can print status and debug output
    /// @param RemoteScriptRunner csp::common::IJSScriptRunner& : Object capable of running a script.
    OfflineRealtimeEngine(
        const CSPSceneDescription& SceneDescription, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner);

    using SpaceEntityList = csp::common::List<SpaceEntity*>;

private:
    using SpaceEntitySet = std::set<SpaceEntity*>;

    // Should not be null
    csp::common::LogSystem* LogSystem;

    // May not be null
    csp::common::IJSScriptRunner* ScriptRunner;

    SpaceEntityList Entities;
    SpaceEntityList Avatars;
    SpaceEntityList Objects;
    SpaceEntityList SelectedEntities;
    SpaceEntityList RootHierarchyEntities;

    std::unique_ptr<SpaceEntitySet> EntitiesToUpdate;

    std::unique_ptr<std::recursive_mutex> EntitiesLock;

    EntityCreatedCallback SpaceEntityCreatedCallback;

    void AddPendingEntity(SpaceEntity* EntityToAdd);
    void RemovePendingEntity(SpaceEntity* EntityToRemove);
};
}
