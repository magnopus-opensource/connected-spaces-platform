#pragma once

#include "Olympus/Common/List.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/Components/AvatarSpaceComponent.h"
#include "Olympus/OlympusCommon.h"

#include <deque>
#include <functional>
#include <list>
#include <mutex>
#include <set>

namespace signalr
{
class value;
}

namespace oly_multiplayer
{
class ClientElectionManager;

class SignalRConnection;

} // namespace oly_multiplayer

namespace oly_multiplayer
{

class MultiplayerConnection;
class SpaceTransform;
class SpaceEntity;

class OLY_API OLY_NO_DISPOSE SpaceEntitySystem
{
    /** @cond DO_NOT_DOCUMENT */
    friend class MultiplayerConnection;
    friend class SpaceEntityEventHandler;
    friend class ClientElectionManager;
    friend class EntityScript;
    /** @endcond */

public:
    ~SpaceEntitySystem();

    typedef std::function<void(bool)> CallbackHandler;
    typedef std::function<void(SpaceEntity*)> EntityCreatedCallback;

    /**
     * @brief Creates a SpaceEntity with type Avatar, and relevant components and default states as specified.
     * @param InName The name to give the new SpaceEntity.
     * @param InSpaceTransform The initial transform to set the SpaceEntity to.
     * @param InState The initial Avatar State to set.
     * @param InAvatarId The Initial AvatarID to set.
     * @param InAvatarPlayMode The Initial AvatarPlayMode to set.
     * @param Callback A callback that executes when the creation is complete,
     * which contains a pointer to the new SpaceEntity so that it can be used on the local client.
     */
    OLY_ASYNC_RESULT void CreateAvatar(const oly_common::String& InName, const SpaceTransform& InSpaceTransform, AvatarState InState,
        const oly_common::String& InAvatarId, AvatarPlayMode InAvatarPlayMode, EntityCreatedCallback Callback);

    /**
     * @brief Creates a SpaceEntity of type Object, and relevant default values.
     * @param InName The name to give the new SpaceEntity.
     * @param InSpaceTransform The initial transform to set the SpaceEntity to.
     * @param Callback A callback that executes when the creation is complete,
     * which contains a pointer to the new SpaceEntity so that it can be used on the local client.
     */
    OLY_ASYNC_RESULT void CreateObject(const oly_common::String& InName, const SpaceTransform& InSpaceTransform, EntityCreatedCallback Callback);

    /**
     * @brief Destroys the both the remote view and the local view of the specified entity
     */
    OLY_ASYNC_RESULT void DestroyEntity(SpaceEntity* Entity, CallbackHandler Callback);

    /**
     * @brief Destroys the local client's view of the specified entity
     */
    void LocalDestroyEntity(SpaceEntity* Entity);

    /**
     * @brief Finds the first SpaceEntity that matches InName.
     * @param InName The name to search.
     * @return A pointer to the first found match SpaceEntity.
     */
    SpaceEntity* FindSpaceEntity(const oly_common::String& InName);
    /**
     * @brief Finds the first SpaceEntity that has the ID EntityId.
     * @param EntityId The Id to look for.
     * @return A pointer to the first found match SpaceEntity.
     */
    SpaceEntity* FindSpaceEntityById(uint64_t EntityId);
    /**
     * @brief Finds the first SpaceEntity of type Avatar that matches InName.
     * @param InName The name to search.
     * @return A pointer to the first found match SpaceEntity.
     */
    SpaceEntity* FindSpaceAvatar(const oly_common::String& InName);
    /**
     * @brief Finds the first SpaceEntity of type Object that matches InName.
     * @param InName The name to search.
     * @return A pointer to the first found match SpaceEntity.
     */
    SpaceEntity* FindSpaceObject(const oly_common::String& InName);

    /**
     * @brief Locks the entity mutex.
     */
    void LockEntityUpdate() const;
    /**
     * @brief Unlocks the entity mutex.
     */
    void UnlockEntityUpdate() const;

    /**
     * @brief Get the number of total entities in the system (both Avatars and Objects)
     */
    [[nodiscard]] size_t GetNumEntities() const;
    /**
     * @brief Get the number of total Avatars in the system
     */
    [[nodiscard]] size_t GetNumAvatars() const;
    /**
     * @brief Get the number of total Objects in the system
     */
    [[nodiscard]] size_t GetNumObjects() const;

    /**
     * @brief Get an Entity (Avatar or Object) by its index
     * Note this is not currently thread safe and should only be called from the main thread
     */
    SpaceEntity* GetEntityByIndex(const size_t EntityIndex);
    /**
     * @brief Get an Avatar by its index
     * Note this is not currently thread safe and should only be called from the main thread
     */
    SpaceEntity* GetAvatarByIndex(const size_t AvatarIndex);
    /**
     * @brief Get an Object by its index
     * Note this is not currently thread safe and should only be called from the main thread
     */
    SpaceEntity* GetObjectByIndex(const size_t ObjectIndex);

    /**
     * @brief Get MultiplayerConnection Object
     */
    MultiplayerConnection* GetMultiplayerConnection();

    /**
     * @brief Add a new entity to the system.
     * This can be called at any time from any thread and internally add the entity to a pending
     * list which is then updated in a thread safe manner when ProcessPendingEntityOperations
     * is called from the main thread.
     */
    void AddEntity(SpaceEntity* EntityToAdd);

    /**
     * @brief Sets a callback to be executed when an entity is remotely created.
     * Only one callback may be registered, calling this function again will override whatever was previously set.
     * If this is not set, some patch functions may fail.
     * @param Callback EntityCreatedCallback : the callback to execute.
     */
    OLY_EVENT void SetEntityCreatedCallback(EntityCreatedCallback Callback);

    /**
     * @brief Sets a local pointer to the connection for communication with the endpoints, this should be called as early as possible.
     * Note that this is already called in MultiplayerConnection::Connect, so this shouldn't need to be called anywhere else.
     * This should not be called by client code directly, marked as No Export.
     */
    OLY_NO_EXPORT void SetConnection(oly_multiplayer::SignalRConnection* InConnection);

    /*
     * @brief Sets a callback to be executed when all existing entities have been retrieved after entering a space.
     * @param Callback CallbackHandler : the callback to execute.
     */
    OLY_EVENT void SetInitialEntitiesRetrievedCallback(CallbackHandler Callback);

    /*
     * @brief Sets a callback to be executed when the script system is ready to run scripts.
     * @param Callback CallbackHandler : the callback to execute.
     */
    OLY_EVENT void SetScriptSystemReadyCallback(CallbackHandler Callback);

    /**
     * @brief Triggers queuing of the SpaceEntities updated components and replicated data. Causes the replication of a SpaceEntities data on next
     * Tick() or ProcessPendingEntityOperations().
     * @param EntityToUpdate A pointer to the SpaceEntity to update.
     */
    void QueueEntityUpdate(SpaceEntity* EntityToUpdate);

    // TODO: Add documentation comment
    void TickEntities();
    // TODO: Add documentation comment
    void RegisterEntityScriptAsModule(SpaceEntity* NewEntity);
    // TODO: Add documentation comment
    void BindNewEntityToScript(SpaceEntity* NewEntity);
    // TODO: Add documentation comment
    void ClaimScriptOwnership(SpaceEntity* Entity) const;
    // TODO: Add documentation comment
    void MarkEntityForUpdate(SpaceEntity* Entity);

    /**
     * @brief Process pending entity adds/removes and Patch message send and receives.
     * Note this should only be called from main thread
     */
    void ProcessPendingEntityOperations();

    // TODO: Should this be NO_EXPORT?
    /**
     * @brief Retrieves all entities from the endpoint, calls "GetAllScopedObjects" currently.
     * Note this will generate new entity objects for every entity in the current scopes.
     * If this is called by a client manually without first deleting all existing tracked entities, it is possible there will be duplicates.
     * It is highly advised not to call this function unless you know what you are doing.
     */
    void RetrieveAllEntities();

    /**
     * @brief Sets the selected state of an entity, if the operation is acceptable. Criteria:
     * For Selection:
     * - Entity must be deselected currently
     * For Deselection:
     * - Entity must be selected currently
     * - Entity must be selected by the client attempting the deselection (SpaceEntity::GetSelectingClientID will return this information)
     * @param SelectedState The state to set the entity to, Selected = True, Deselected = false.
     * @param Entity The entity to modify selection state on.
     * @return Returns true if a selection state change has occurred, false if no change was made (due to one of the above criteria not being met).
     */
    bool SetSelectionStateOfEntity(const bool SelectedState, SpaceEntity* Entity);

    /**
     * Enable Leader Election feature
     */
    void EnableLeaderElection();
    void DisableLeaderElection();
    bool IsLeaderElectionEnabled() const;

    /**
            Debug helper to get the id of the currently elected script leader
    */
    uint64_t GetLeaderId() const;

    /*
     * @brief Finds a component by the given id
     * Searchs through all components of all entites so should be used sparingly
     * @param Id The id of the component to find
     */
    ComponentBase* FindComponentById(uint16_t Id);

protected:
    using SpaceEntityList = oly_common::List<SpaceEntity*>;

    SpaceEntityList Entities;
    std::recursive_mutex* EntitiesLock;

private:
    SpaceEntitySystem(MultiplayerConnection* InMultiplayerConnection);

    MultiplayerConnection* MultiplayerConnectionInst;
    oly_multiplayer::SignalRConnection* Connection;

    using SpaceEntityQueue = std::deque<SpaceEntity*>;
    using PatchMessageQueue = std::deque<signalr::value*>;
    using SpaceEntitySet = std::set<SpaceEntity*>;

    SpaceEntityList Avatars;
    SpaceEntityList Objects;
    SpaceEntityList SelectedEntities;

    EntityCreatedCallback SpaceEntityCreatedCallback;
    CallbackHandler InitialEntitiesRetrievedCallback;
    CallbackHandler ScriptSystemReadyCallback;

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

    void ClaimScriptOwnershipFromClient(uint64_t ClientId);
    bool CheckIfWeShouldRunScriptsLocally() const;
    void RunScriptRemotely(int64_t ContextId, const oly_common::String& ScriptText);
    void TickEntityScripts();

    void OnAvatarAdd(const SpaceEntity* Avatar, const SpaceEntityList& Avatars);
    void OnAvatarRemove(const SpaceEntity* Avatar, const SpaceEntityList& Avatars);
    void OnObjectAdd(const SpaceEntity* Object, const SpaceEntityList& Entities);
    void OnObjectRemove(const SpaceEntity* Object, const SpaceEntityList& Entities);

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
};

} // namespace oly_multiplayer
