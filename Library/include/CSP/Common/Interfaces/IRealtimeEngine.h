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
#include "CSP/Common/Interfaces/InvalidInterfaceUserError.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"

namespace csp::multiplayer
{
enum class AvatarPlayMode;
enum class AvatarState;
class SpaceTransform;
class SpaceEntity;
}

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
// Note: It's not great that we require std (function) dependencies in interop types, seems to defeat the whole point.

/**
 * This callback should be fired during IRealtimeEngine::FetchAllEntitiesAndPopulateBuffers when the system is ready for
 * csp::systems::SpaceSystem::EnterSpace to yield control back to calling clients. This may be done prior to actually completing fetching all
 * entities, as that could be a long operation and the specific RealtimeEngine implementation may not wish to block clients entering a space.
 */
typedef std::function<void()> EntityFetchStartedCallback;

/**
 * This callback should be fired once all the entities have been fetched upon initial space setup, invoked via
 * IRealtimeEngine::FetchAllEntitiesAndPopulateBuffers. This serves as a public notification to clients that the RealtimeEngine is in a valid state
 * and entity inspection and mutation may begin.
 *
 * Provides the number of entities fetched as an argument
 */
typedef std::function<void(uint32_t)> EntityFetchCompleteCallback;

class LoginState;

// This, frustratingly, cannot be an in-class type due to the code generator.
/// @brief Enum of concrete types of RealtimeEngines.
enum class RealtimeEngineType
{
    Online = 0,
    Offline
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
 *
 * @important This type is not a true interface, instead throwing default implementations.
 *            This is due to wrapper generator constraints, hopefully temporary ones.
 */
class CSP_API IRealtimeEngine
{
public:
    /// @brief Virtual destructor.
    virtual ~IRealtimeEngine() = default;

    /// @brief Returns the concrete type of the instantiation of the abstract IRealtimeEngine.
    virtual RealtimeEngineType GetRealtimeEngineType() const { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /***** ENTITY MANAGEMENT *************************************************/

    /// @brief Create and add a SpaceEntity with type Avatar, and relevant components and default states as specified.
    /// @param Name csp::common::String : The entity name of the newly created avatar entity.
    /// @param LoginState const csp::common::Optional<csp::common::LoginState>& : Login state tokens for authenticating on the remote server. Only
    /// required if backend data store requires authentication.
    /// @param SpaceTransform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param IsVisible bool : Whether the avatar defaults to being visible.
    /// @param AvatarState csp::multiplayer::AvatarState : The initial Avatar State to set.
    /// @param AvatarId csp::common::String : The ID to be set on the AvatarSpaceComponent
    /// @param AvatarPlayMode csp::multiplayer::AvatarPlayMode : The Initial AvatarPlayMode to set.
    /// @param Callback csp::multiplayer::EntityCreatedCallback A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT virtual void CreateAvatar(const csp::common::String& Name, const csp::common::Optional<csp::common::LoginState> LoginState,
        const csp::multiplayer::SpaceTransform& SpaceTransform, bool IsVisible, const csp::multiplayer::AvatarState& AvatarState,
        const csp::common::String& AvatarId, const csp::multiplayer::AvatarPlayMode& AvatarPlayMode, csp::multiplayer::EntityCreatedCallback Callback)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Create and add a SpaceEntity, with relevant default values.
    /// @param Name csp::common::String : The name of the newly created SpaceEntity.
    /// @param SpaceTransform csp::multiplayer::SpaceTransform : The initial transform to set the SpaceEntity to.
    /// @param ParentID csp::common::Optional<uint64_t> : ID of another entity in the space that this entity should be created as a child to. If
    /// empty, entity is created as a root entity.
    /// @param Callback csp::multiplayer::EntityCreatedCallback : A callback that executes when the creation is complete,
    /// which will provide a non-owning pointer to the new SpaceEntity so that it can be used on the local client.
    CSP_ASYNC_RESULT virtual void CreateEntity(const csp::common::String& Name, const csp::multiplayer::SpaceTransform& SpaceTransform,
        const csp::common::Optional<uint64_t>& ParentID, csp::multiplayer::EntityCreatedCallback Callback)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    virtual void AddEntity(csp::multiplayer::SpaceEntity* EntityToAdd) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /// @brief Destroy the specified entity.
    /// @param Entity csp::multiplayer::SpaceEntity : A non-owning pointer to the entity to be destroyed.
    /// @param Callback csp::multiplayer::CallbackHandler : A callback that executes when the entity destruction is complete.
    CSP_ASYNC_RESULT virtual void DestroyEntity(csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::CallbackHandler Callback)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Sets a callback to be executed when an entity is fully created.
    ///
    /// Only one EntityCreatedCallback may be registered, calling this function again will override whatever was previously set.
    /// The better way to set this and avoid initialisation race conditions is via passing this in the constructor, only use this if you wish to
    /// override or remove this callback.
    ///
    /// @param Callback csp::multiplayer::EntityCreatedCallback : the callback to execute.
    CSP_EVENT virtual void SetEntityCreatedCallback(csp::multiplayer::EntityCreatedCallback Callback)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    virtual bool AddEntityToSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }
    virtual bool RemoveEntityFromSelectedEntities(csp::multiplayer::SpaceEntity* Entity)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /**
     * @brief Fetch space entities from the RealtimeEngine data source and perform initial setup to populate internal buffers
     *
     * This method is called during csp::systems::SpaceSystem::Enterspace.
     *
     * @param SpaceId const csp::common::String& : The identifier of the space to fetch entities from. Format depends on how your specific backend
     * store indexes spaces.
     * @param FetchStartedCallback EntityFetchStartedCallback : Callback notifying for when csp::systems::SpaceSystem::EnterSpace can yield control
     * back to clients
     * @post @param FetchStartedCallback will have been called, and the EntityFetchCompleteCallback the engine was constructed with will either have
     * been called, or an async job started with an intent to call it once fetch is complete.
     */
    CSP_NO_EXPORT virtual void FetchAllEntitiesAndPopulateBuffers(const csp::common::String& SpaceId, EntityFetchStartedCallback FetchStartedCallback)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /***** ENTITY ACCESS *****************************************************/

    /// @brief Finds the first found SpaceEntity of a matching Name.
    /// @param Name csp::common::String : The name to search.
    /// @return A non-owning pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceEntity(const csp::common::String& Name)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Finds the first found SpaceEntity that has the ID EntityId.
    /// @param EntityId uint64_t : The Id to look for.
    /// @return A non-owning pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceEntityById(uint64_t EntityId)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Finds the first found SpaceEntity of a matching Name. The found SpaceEntity will contain an AvatarSpaceComponent.
    /// @param Name The name to search for.
    /// @return A pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceAvatar(const csp::common::String& Name)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Finds the first found SpaceEntity of a matching Name. The found SpaceEntity will not contain an AvatarSpaceComponent.
    /// @param Name The name to search for.
    /// @return A pointer to the first found matching SpaceEntity.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* FindSpaceObject(const csp::common::String& Name)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Get an Entity by its index.
    ///
    /// @param EntityIndex size_t : The index of the entity to get.
    /// @return A non-owning pointer to the entity at the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetEntityByIndex(size_t EntityIndex)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Get an Avatar by its index. The returned pointer will be an entity that contains an AvatarSpaceComponent.
    ///
    /// @param AvatarIndex size_t : The index of the avatar entity to get.
    /// @return A non-owning pointer to the avatar entity with the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetAvatarByIndex(size_t AvatarIndex)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Get an Object by its index. The returned pointer will be an entity that does not contain an AvatarSpaceComponent.
    ///
    /// @param ObjectIndex size_t : The index of the object entity to get.
    /// @return A non-owning pointer to the object entity with the given index.
    [[nodiscard]] virtual csp::multiplayer::SpaceEntity* GetObjectByIndex(size_t ObjectIndex)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Get the number of total entities in the system.
    /// @return The total number of entities.
    virtual size_t GetNumEntities() const { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /// @brief Get the number of total Avatars in the system. Avatars are entities that contain AvatarSpaceComponents.
    /// @return The total number of Avatar entities.
    virtual size_t GetNumAvatars() const { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /// @brief Get the number of total Objects in the system. Objects are entities that do not contain AvatarSpaceComponents.
    /// @return The total number of object entities.
    virtual size_t GetNumObjects() const { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /// @brief Retrieves all entities that exist at the root level (do not have a parent entity).
    /// @return A list of root entities containing non-owning pointers to entities.
    [[nodiscard]] virtual const csp::common::List<csp::multiplayer::SpaceEntity*>* GetRootHierarchyEntities() const
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /// @brief Set Callback that notifies when the OnlineRealtimeEngine is in a valid state
    /// after entering a space, and entity mutation can begin. Users should not mutate entities before receiving this callback.
    /// This callback should be emitted in response to FetchAllEntitiesAndPopulateBuffers completing, either syncronously or asyncronously.
    /// This callback must be set prior to entering a space.
    /// @param FetchCompleteCallback csp::common::EntityFetchCompleteCallback : Callback that notifies when the RealtimeEngine has completed initial
    /// entity fetch
    void SetEntityFetchCompleteCallback(EntityFetchCompleteCallback Callback) { EntityFetchCompleteCallback = Callback; }

    // Wrapper generator limitation, would rather expose this as it'd be handy to know :(
    CSP_NO_EXPORT EntityFetchCompleteCallback GetEntityFetchCompleteCallback() const { return EntityFetchCompleteCallback; }

    /***** ENTITY PROCESSING *************************************************/

    /// @brief Adds an entity to a list of entities to be updated when ProcessPendingEntityOperations is called.
    /// From a client perspective, ProcessPendingEntityOperations is normally called via the CSPFoundation::Tick method.
    /// @param Entity SpaceEntity : A non-owning pointer to the entity to be marked.
    virtual void MarkEntityForUpdate(csp::multiplayer::SpaceEntity* Entity) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /// @brief Applies any pending changes to entities that have been marked for update.
    virtual void ProcessPendingEntityOperations() { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

protected:
    // We want copies and moves and such to be possible for derived types, but they need to be sure to explicitly implement the behaviour.
    // Protect these operations by default to prevent accidental slicing.
    IRealtimeEngine() = default; // Wrapper gen required ... :(
    IRealtimeEngine(const IRealtimeEngine&) = delete;
    IRealtimeEngine& operator=(const IRealtimeEngine&) = delete;
    IRealtimeEngine(IRealtimeEngine&&) = delete;
    IRealtimeEngine& operator=(IRealtimeEngine&&) = delete;

    EntityFetchCompleteCallback EntityFetchCompleteCallback = nullptr;
};
}

// The following doc is temporary for reference during the module migration effort.

/* As this interface is more or less a conversion from `OnlineRealtimeEngine`, what follows are
 * the methods that were public, but are no longer going to be public as far as the interface
 * is concerned. I've checked via searching the Magnopus org for uses of these, and initial findings
 * show these can be hidden, although some client validation wouldn't go amiss.
 * In reality, OnlineRealtimeEngine isn't going anywhere and will continue to exists behind the interface, so these
 * methods are not being deleted, they're just not being exposed to clients or CSP-core.
 * They'll still be called by methods internal to the module.
 *
 *
 * OnlineRealtimeEngine::AddEntity (We think only Ichabod uses this, we think incorrectly. _Definitely_ need to check this one. Potential full deletion
 * candidate)
 * OnlineRealtimeEngine::LocalDestroyEntity OnlineRealtimeEngine::LockEntityUpdate OnlineRealtimeEngine::UnlockEntityUpdate
 * OnlineRealtimeEngine::SetConnection
 * OnlineRealtimeEngine::RegisterEntityScriptAsModule (Potential full deletion candidate)
 * OnlineRealtimeEngine::BindNewEntityToScript (Potential full deletion candidate)
 * OnlineRealtimeEngine::RetrieveAllEntities
 * OnlineRealtimeEngine::LocalDestroyAllEntities
 * OnlineRealtimeEngine::SetSelectionStateOfEntity (Covered by SpaceEntity Select/Deselect)
 * OnlineRealtimeEngine::CreateObjectInternal
 * OnlineRealtimeEngine::ResolveEntityHierarchy
 * OnlineRealtimeEngine::Initialise
 * OnlineRealtimeEngine::Shutdown
 * OnlineRealtimeEngine::GetPendingAdds
 * OnlineRealtimeEngine::CheckIfWeShouldRunScriptsLocally
 * OnlineRealtimeEngine::RunScriptRemotely (Public interface to this is EntityScript::RunScript)
 */

/* These are the methods that have been removed from the base interface, but will exist publicly on the Multi User concretion.
 *
 * OnlineRealtimeEngine::GetEntityPatchRateLimitEnabled
 * OnlineRealtimeEngine::SetEntityPatchRateLimitEnabled
 * OnlineRealtimeEngine::EnableLeaderElection
 * OnlineRealtimeEngine::DisableLeaderElection
 * OnlineRealtimeEngine::IsLeaderElectionEnabled
 * OnlineRealtimeEngine::GetLeaderId
 * OnlineRealtimeEngine::ClaimScriptOwnership
 * OnlineRealtimeEngine::SetScriptSystemReadyCallback (Should rename to SetScriptLeaderReadyCallback)
 * OnlineRealtimeEngine::GetMultiplayerConnectionInstance
 * SystemsManager::GetEventBus (Rename EventBus -> NetworkEventBus)
 * OnlineRealtimeEngine::RefreshMultiplayerConnection
 */
