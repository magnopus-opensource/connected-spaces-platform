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

#include "CSP/Common/List.h"
#include "CSP/Common/String.h"

CSP_START_IGNORE
namespace csp::multiplayer
{
enum class AvatarPlayMode : int;
enum class AvatarState : int;
class SpaceTransform;
class SpaceEntity;
class EventBus;
}
CSP_END_IGNORE

namespace csp::multiplayer
{
/*
 * Namespacing for interface types can be weird, as they by design live in places that arn't
 * where their concretions are. These callbacks are "multiplayer" (name to be changed), despite
 * being defined here for use in the interface
 */

// Callback that provides a success/fail type of response.
typedef std::function<void(bool)> CallbackHandler;

// Callback that provides a non-owning pointer to a SpaceEntity object.
typedef std::function<void(csp::multiplayer::SpaceEntity*)> EntityCreatedCallback;
}

namespace csp::common
{

// This, frustratingly, cannot be an in-class type due to the code generator.
/// @brief Enum of concrete types of RealtimeEngines.
enum class RealtimeEngineType : int
{
    OnlineMultiUser = 0,
    OnlineSingleUser
};

/**
 * @class IRealtimeEngine
 * @brief Interface abstraction of a CSP Realtime Engine.
 *
 * A Realtime engine is the CSP component responsible for driving the realtime behaviour
 * of a connected space, primarily by responding to user input and managing updates
 * to entities within the space.
 *
 * Most users will use the OnlineMultiUserRealtimeEngine implementation to support online
 * experiences. However, other implementations exist for alternate use cases, such as
 * OnlineSingleUserRealtimeEngine for single-user flows inside an online space.
 *
 * A RealtimeEngine should be created before CSP initialization using one of the provided
 * factory functions, and then passed to CSPFoundation::Initialize.
 *
 * @note
 * Terminology:
 * - @b Entity: All items in a space are entities.
 * - @b Avatar: A specialization of Entity representing an avatar. Defined by whether the entity contains an AvatarSpaceComponent.
 * - @b Object: An entity that is not an avatar. Defined by that entity not containing an AvatarSpaceComponent.
 */
class CSP_API IRealtimeEngine
{
public:
    /// @brief Virtual destructor.
    virtual ~IRealtimeEngine() = default;

    /// @brief Returns the concrete type of the instantiation of the abstract IRealtimeEngine.
    virtual RealtimeEngineType GetRealtimeEngineType() const = 0;

    /***** ENTITY MANAGEMENT *************************************************/

    /// @brief Create and add a SpaceEntity with type Avatar, and relevant components and default states as specified.
    /// @param Name csp::common::String : The entity name of the newly created avatar entity.
    /// @param SpaceTransform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param State csp::multiplayer::AvatarState : The initial Avatar State to set.
    /// @param AvatarId csp::common::String : The ID to be set on the AvatarSpaceComponent
    /// @param AvatarPlayMode csp::multiplayer::AvatarPlayMode : The Initial AvatarPlayMode to set.
    /// @param Callback csp::multiplayer::EntityCreatedCallback A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    /// @pre Entity creation callback is non null. This can be set via SetEntityCreatedCallback.
    CSP_ASYNC_RESULT virtual void CreateAvatar(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& SpaceTransform,
        csp::multiplayer::AvatarState State, const csp::common::String& AvatarId, csp::multiplayer::AvatarPlayMode AvatarPlayMode,
        csp::multiplayer::EntityCreatedCallback Callback)
        = 0;

    /// @brief Create and add a SpaceEntity, with relevant default values.
    /// @param Name csp::common::String : The name of the newly created SpaceEntity.
    /// @param SpaceTransform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param Callback csp::multiplayer::EntityCreatedCallback : A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    /// @pre Entity creation callback is non null. This can be set via SetEntityCreatedCallback.
    CSP_ASYNC_RESULT virtual void CreateEntity(
        const csp::common::String& Name, const csp::multiplayer::SpaceTransform& SpaceTransform, csp::multiplayer::EntityCreatedCallback Callback)
        = 0;

    /// @brief Destroy the specified entity.
    /// @param Entity csp::multiplayer::SpaceEntity : A non-owning pointer to the entity to be destroyed.
    /// @param Callback csp::multiplayer::CallbackHandler : A callback that executes when the entity destruction is complete.
    CSP_ASYNC_RESULT virtual void DestroyEntity(csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::CallbackHandler Callback) = 0;

    /// @brief Sets a callback to be executed when an entity is fully created.
    ///
    /// Only one EntityCreatedCallback may be registered, calling this function again will override whatever was previously set.
    /// This must be set prior to creating an Entity.
    ///
    /// @param Callback csp::multiplayer::EntityCreatedCallback : the callback to execute.
    CSP_EVENT virtual void SetEntityCreatedCallback(csp::multiplayer::EntityCreatedCallback Callback) = 0;

    /***** ENTITY ACCESS *****************************************************/

    /// @brief Finds the first found SpaceEntity of a matching Name.
    /// @param Name csp::common::String : The name to search.
    /// @return A non-owning pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceEntity(const csp::common::String& Name) = 0;

    /// @brief Finds the first found SpaceEntity that has the ID EntityId.
    /// @param EntityId uint64_t : The Id to look for.
    /// @return A non-owning pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceEntityById(uint64_t EntityId) = 0;

    /// @brief Finds the first found SpaceEntity of a matching Name. The found SpaceEntity will contain an AvatarSpaceComponent.
    /// @param Name The name to search for.
    /// @return A pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceAvatar(const csp::common::String& Name) = 0;

    /// @brief Finds the first found SpaceEntity of a matching Name. The found SpaceEntity will not contain an AvatarSpaceComponent.
    /// @param Name The name to search for.
    /// @return A pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceObject(const csp::common::String& Name) = 0;

    /// @brief Get an Entity by its index.
    ///
    /// @param EntityIndex size_t : The index of the entity to get.
    /// @return A non-owning pointer to the entity at the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetEntityByIndex(size_t EntityIndex) = 0;

    /// @brief Get an Avatar by its index. The returned pointer will be an entity that contains an AvatarSpaceComponent.
    ///
    /// @param AvatarIndex size_t : The index of the avatar entity to get.
    /// @return A non-owning pointer to the avatar entity with the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetAvatarByIndex(size_t AvatarIndex) = 0;

    /// @brief Get an Object by its index. The returned pointer will be an entity that does not contain an AvatarSpaceComponent.
    ///
    /// @param ObjectIndex size_t : The index of the object entity to get.
    /// @return A non-owning pointer to the object entity with the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetObjectByIndex(size_t ObjectIndex) = 0;

    /// @brief Get the number of total entities in the system.
    /// @return The total number of entities.
    virtual size_t GetNumEntities() const = 0;

    /// @brief Get the number of total Avatars in the system. Avatars are entities that contain AvatarSpaceComponents.
    /// @return The total number of Avatar entities.
    virtual size_t GetNumAvatars() const = 0;

    /// @brief Get the number of total Objects in the system. Objects are entities that do not contain AvatarSpaceComponents.
    /// @return The total number of object entities.
    virtual size_t GetNumObjects() const = 0;

    /// @brief Retrieves all entities that exist at the root level (do not have a parent entity).
    /// @return A list of root entities containing non-owning pointers to entities.
    [[nodiscard]] virtual const csp::common::List<csp::multiplayer::SpaceEntity*>* GetRootHierarchyEntities() const = 0;

    /// @brief Sets a callback to be executed when all existing entities have been retrieved after entering a space.
    /// @param Callback CallbackHandler : A callback that executes when all existing entities have been retrieved.
    CSP_EVENT virtual void SetInitialEntitiesRetrievedCallback(csp::multiplayer::CallbackHandler Callback) = 0;

    /***** ENTITY PROCESSING *************************************************/

    /// @brief Adds an entity to a list of entities to be updated when ProcessPendingEntityOperations is called.
    /// From a client perspective, ProcessPendingEntityOperations is normally called via the CSPFoundation::Tick method.
    /// @param Entity SpaceEntity : A non-owning pointer to the entity to be marked.
    virtual void MarkEntityForUpdate(csp::multiplayer::SpaceEntity* Entity) = 0;

    /// @brief Applies any pending changes to entities that have been marked for update.
    virtual void ProcessPendingEntityOperations() = 0;
};
}

// The following doc is temporary for reference during the module migration effort.

/* As this interface is more or less a conversion from `SpaceEntitySystem`, what follows are
 * the methods that were public, but are no longer going to be public as far as the interface
 * is concerned. I've checked via searching the Magnopus org for uses of these, and initial findings
 * show these can be hidden, although some client validation wouldn't go amiss.
 * In reality, SpaceEntitySystem isn't going anywhere and will continue to exists behind the interface, so these
 * methods are not being deleted, they're just not being exposed to clients or CSP-core.
 * They'll still be called by methods internal to the module.
 *
 *
 * SpaceEntitySystem::AddEntity (We think only Ichabod uses this, we think incorrectly. _Definitely_ need to check this one. Potential full deletion
 * candidate)
 * SpaceEntitySystem::LocalDestroyEntity SpaceEntitySystem::LockEntityUpdate SpaceEntitySystem::UnlockEntityUpdate
 * SpaceEntitySystem::SetConnection
 * SpaceEntitySystem::RegisterEntityScriptAsModule (Potential full deletion candidate)
 * SpaceEntitySystem::BindNewEntityToScript (Potential full deletion candidate)
 * SpaceEntitySystem::RetrieveAllEntities
 * SpaceEntitySystem::LocalDestroyAllEntities
 * SpaceEntitySystem::SetSelectionStateOfEntity (Covered by SpaceEntity Select/Deselect)
 * SpaceEntitySystem::CreateObjectInternal
 * SpaceEntitySystem::ResolveEntityHierarchy
 * SpaceEntitySystem::Initialise
 * SpaceEntitySystem::Shutdown
 * SpaceEntitySystem::GetPendingAdds
 * SpaceEntitySystem::CheckIfWeShouldRunScriptsLocally
 * SpaceEntitySystem::RunScriptRemotely (Public interface to this is EntityScript::RunScript)
 */

/* These are the methods that have been removed from the base interface, but will exist publicly on the Multi User concretion.
 *
 * SpaceEntitySystem::GetEntityPatchRateLimitEnabled
 * SpaceEntitySystem::SetEntityPatchRateLimitEnabled
 * SpaceEntitySystem::EnableLeaderElection
 * SpaceEntitySystem::DisableLeaderElection
 * SpaceEntitySystem::IsLeaderElectionEnabled
 * SpaceEntitySystem::GetLeaderId
 * SpaceEntitySystem::ClaimScriptOwnership
 * SpaceEntitySystem::SetScriptSystemReadyCallback (Should rename to SetScriptLeaderReadyCallback)
 * SpaceEntitySystem::GetMultiplayerConnectionInstance
 * SystemsManager::GetEventBus (Rename EventBus -> NetworkEventBus)
 */
