#pragma once

#include "Olympus/Common/Optional.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/SpaceEntitySystem.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/Assets/Asset.h"
#include "Olympus/Systems/Assets/AssetCollection.h"
#include "Olympus/Systems/Spaces/Site.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/Spaces/UserRoles.h"
#include "Olympus/Systems/SystemBase.h"

namespace oly_services
{

class ApiBase;

}

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

/// @ingroup Space System
/// @brief Public facing system that allows interfacing with CHS's concept of a Group.
/// Offers methods for creating, deleting and joining spaces.
class OLY_API OLY_NO_DISPOSE SpaceSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~SpaceSystem();

    /** @name Helper Functions
     *
     *   @{ */

    /// @brief Enter a space.
    /// @param Space Space : space to enter into
    /// @param Callback EnterSpaceResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void EnterSpace(const oly_common::String& SpaceId, bool AutoConnect, EnterSpaceResultCallback Callback);

    /// @brief Send exit current space event to EventSystem.
    void ExitSpace();

    /// @brief Send exit current space event to EventSystem.
    /// @param Connection oly_multiplayer::MultiplayerConnection : optional Connection to disconnect MultiplayerConnection
    /// @param Callback BoolCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void ExitSpaceAndDisconnect(oly_multiplayer::MultiplayerConnection* Connection, BoolCallback Callback);

    /// @brief Get whether user is currently in a space.
    /// @return Result of whether they are in a Space.
    bool IsInSpace();

    /// @brief Get the user's current space.
    /// @return The space data object the user is currently in
    const Space& GetCurrentSpace() const;

    /**
     * @brief Sets a callback to be executed when an entity is remotely created.
     * Only one callback may be registered, calling this function again will override whatever was previously set.
     * If this is not set, some patch functions may fail.
     * @param Callback EntityCreatedCallback : the callback to execute.
     */
    OLY_EVENT void SetEntityCreatedCallback(oly_multiplayer::SpaceEntitySystem::EntityCreatedCallback Callback);

    /*
     * @brief Sets a callback to be executed when all existing entities have been retrieved after entering a space.
     * @param Callback CallbackHandler : the callback to execute.
     */
    OLY_EVENT void SetInitialEntitiesRetrievedCallback(oly_multiplayer::SpaceEntitySystem::CallbackHandler Callback);

    /*
     * @brief Sets a callback to be executed when the script system is ready to run scripts.
     * @param Callback CallbackHandler : the callback to execute.
     */
    OLY_EVENT void SetScriptSystemReadyCallback(oly_multiplayer::SpaceEntitySystem::CallbackHandler Callback);

    /** @} */

    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */

    /// @brief Creates a new space.
    /// @param Name oly_common::String : name for the new space
    /// @param Description oly_common::String : description for the new space
    /// @param Type oly_systems::SpaceType : type of the new space
    /// @param InviteUsers oly_common::Array<oly_systems::InviteUserRoleInfo> : optional array of users to be invited into the space
    /// @param Metadata oly_common::String : metadata information for the new space
    /// @param FileThumbnail oly_systems::FileAssetDataSource : optional thumbnail image for the new space
    /// @param Callback oly_systems::SpaceResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateSpace(const oly_common::String& Name, const oly_common::String& Description, SpaceType Type,
        const oly_common::Optional<oly_common::Array<oly_systems::InviteUserRoleInfo>>& InviteUsers,
        const oly_common::Map<oly_common::String, oly_common::String>& Metadata,
        const oly_common::Optional<oly_systems::FileAssetDataSource>& FileThumbnail, SpaceResultCallback Callback);

    /// @brief Creates a new space Using BufferAssetDataSource.
    /// @param Name oly_common::String : name for the new space
    /// @param Description oly_common::String : description for the new space
    /// @param Type oly_systems::SpaceType : type of the new space
    /// @param InviteUsers oly_common::Array<oly_systems::InviteUserRoleInfo> : optional array of users to be invited into the space
    /// @param Metadata oly_common::String : metadata information for the new space
    /// @param Thumbnail oly_systems::BufferAssetDataSource : thumbnail image buffer for the new space
    /// @param Callback oly_systems::SpaceResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateSpaceWithBuffer(const oly_common::String& Name, const oly_common::String& Description, SpaceType Type,
        const oly_common::Optional<oly_common::Array<oly_systems::InviteUserRoleInfo>>& InviteUsers,
        const oly_common::Map<oly_common::String, oly_common::String>& Metadata, const oly_systems::BufferAssetDataSource& Thumbnail,
        SpaceResultCallback Callback);

    /// @brief Updates the name and/or the description of a Space
    /// @param Space Space : the Space to update
    /// @param Name oly_common::Optional<oly_common::String> : if a new name is provided it will be used to update the Space name
    /// @param Description oly_common::Optional<oly_common::String> : if a new description is provided it will be used to update the Space description
    /// @param Type oly_common::Optional<oly_systems::SpaceType> : if a new type is provided it will be used to update the Space type
    /// @param Callback BasicSpaceResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateSpace(const oly_common::String& SpaceId, const oly_common::Optional<oly_common::String>& Name,
        const oly_common::Optional<oly_common::String>& Description, const oly_common::Optional<oly_systems::SpaceType>& Type,
        BasicSpaceResultCallback Callback);

    /// @brief Deletes a given space and the corresponding UserService group.
    /// @param Space Space : space to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteSpace(const oly_common::String& SpaceId, NullResultCallback Callback);

    /// @brief Retrieves all spaces corresponding to the currently logged in user.
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpaces(SpacesResultCallback Callback);

    /// @brief Retrieves basic space details for the requested space types available to the logged in user. Results pagination is supported through
    /// the use of ResultsSkipNumber and ResultsMaxNumber.
    /// @param RequestedSpaceTypes oly_common::Array<SpaceType: array of space types for which the basic space details will be retrieved.
    /// @param ResultsSkipNumber oly_common::Optional<int> : number of result entries that will be skipped from the result. For no skip pass nullptr.
    /// @param ResultsMaxNumber oly_common::Optional<int> : maximum number of result entries to be retrieved. For all available result entries pass
    /// nullptr.
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpacesByTypes(const oly_common::Array<SpaceType>& RequestedSpaceTypes,
        const oly_common::Optional<int>& ResultsSkipNumber, const oly_common::Optional<int>& ResultsMaxNumber, BasicSpacesResultCallback Callback);

    /// @brief Retrieves space details corresponding to the provided Space IDs
    /// @param RequestedSpaceIDs oly_common::Array<oly_common::String> : array of Space IDs for which the space details will be retrieved
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpacesByIds(const oly_common::Array<oly_common::String>& RequestedSpaceIDs, SpacesResultCallback Callback);

    /// @brief Retrieves all spaces corresponding to the provided user ID.
    /// @param UserId oly_common::String : unique ID of user
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpacesForUserId(const oly_common::String& UserId, SpacesResultCallback Callback);

    /// @brief Retrieves a space by its unique ID.
    /// @param SpaceId oly_common::String : unique ID of Space
    /// @param Callback SpaceResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpace(const oly_common::String& SpaceId, SpaceResultCallback Callback);

    /// @brief Invites a given email to a specific space.
    /// @param Space Space : space to invite to
    /// @param Email oly_common:String : email to invite to space
    /// @param IsModeratorRole oly_common::Optional<bool> : if present and true sets the user's role in the space to "Moderator", pass false or
    /// nullptr to leave role as default
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void InviteToSpace(const oly_common::String& SpaceId, const oly_common::String& Email,
        const oly_common::Optional<bool>& IsModeratorRole, NullResultCallback Callback);

    /// @brief Invites all the given emails to a specific space.
    /// @param Space Space : space to invite to
    /// @param InviteUsers oly_common::Array<InviteUserRoleInfo> : Array of users to invite with their email and role
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void BulkInviteToSpace(
        const oly_common::String& SpaceId, const oly_common::Array<InviteUserRoleInfo>& InviteUsers, NullResultCallback Callback);

    /// @brief Returns an array of obfuscated email addresses, addresses of users that have not yet accepted the space invite
    /// @param Space Space : Space for which the invites where sent
    /// @param Callback PendingInvitesResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetPendingUserInvites(const oly_common::String& SpaceId, PendingInvitesResultCallback Callback);

    /// @brief Removes a user from a space by the user's unique ID.
    /// @param Space Space : space to remove user from
    /// @param UserId oly_common::String : unique ID of user
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void RemoveUserFromSpace(const oly_common::String& SpaceId, const oly_common::String& UserId, NullResultCallback Callback);

    /// @brief Adds a user to a space by the user's unique ID.
    /// @param Space Space : space to add user to
    /// @param UserId oly_common::String : unique ID of user
    /// @param Callback SpaceResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddUserToSpace(const oly_common::String& SpaceId, const oly_common::String& UserId, SpaceResultCallback Callback);

    /// @brief Creates new Site information and associates it with the Space.
    /// @param Space Space : Space to associate the Site information with
    /// @param SiteInfo Site : Site information to be added
    /// @param Callback SiteResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddSiteInfo(const oly_common::String& SpaceId, Site& SiteInfo, SiteResultCallback Callback);

    /// @brief Removes the Site information from the Space.
    /// @param Space Space : Space for which to remove the associated Site information
    /// @param SiteInfo Site : Site information to be removed
    /// @param Callback NullResultCallback : callback when asynchronous task
    OLY_ASYNC_RESULT void RemoveSiteInfo(const oly_common::String& SpaceId, Site& SiteInfo, NullResultCallback Callback);

    /// @brief Retrieves the Sites information associated with a Space.
    /// @param Space Space : Space to be queried for Site information
    /// @param Callback SitesCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSitesInfo(const oly_common::String& SpaceId, SitesCollectionResultCallback Callback);

    /// @brief Updates the space role for a particular user
    /// @param Space Space : The space that the requested user is part of
    /// @param NewUserRoleInfo UserRoleInfo : New user role information containing the new role for the specified user
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateUserRole(const oly_common::String& SpaceId, const UserRoleInfo& NewUserRoleInfo, NullResultCallback Callback);

    /// @brief Retrieves the User role information for the User Ids that have been passed in
    /// @param Space Space : Space for which the User Roles will be retrieved
    /// @param RequestedUserIds oly_common::Array<oly_common::String> : Array of User Ids for which the User Roles will be retrieved
    /// @param Callback UserRoleCollectionCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetUsersRoles(
        const oly_common::String& SpaceId, const oly_common::Array<oly_common::String>& RequestedUserIds, UserRoleCollectionCallback Callback);

    /// @brief Updates the Space metadata information with the new one provided
    /// @param SpaceId oly_common::String : ID of Space for which the metadata will be updated
    /// @param NewMetadata oly_common::String : New metadata information that will replace the previous one
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateSpaceMetadata(
        const oly_common::String& SpaceId, const oly_common::Map<oly_common::String, oly_common::String>& NewMetadata, NullResultCallback Callback);

    /// @brief Retrieves Spaces metadata information
    /// @param Spaces oly_common::Array<Space> : Spaces for which metadata will be retrieved
    /// @param Callback SpacesMetadataResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpacesMetadata(const oly_common::Array<oly_common::String>& Spaces, SpacesMetadataResultCallback Callback);

    /// @brief Retrieves the Space metadata information
    /// @param Space Space : Space for which the metadata will be retrieved
    /// @param Callback SpaceMetadataResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpaceMetadata(const oly_common::String& SpaceId, SpaceMetadataResultCallback Callback);

    /// @brief Updates the Space thumbnail image or adds one if it didn't have it previously using FileAssetDataSource
    /// @param Space Space : Space for which the thumbnail will be updated
    /// @param NewThumbnail oly_systems::FileAssetDataSource : New thumbnail information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateSpaceThumbnail(
        const oly_common::String& SpaceId, const oly_systems::FileAssetDataSource& NewThumbnail, NullResultCallback Callback);

    /// @brief Updates the Space thumbnail image or adds one if it didn't have it previously using BufferAssetDataSource
    /// @param Space Space : Space for which the thumbnail will be updated
    /// @param NewThumbnail oly_systems::BufferAssetDataSource : New thumbnail information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateSpaceThumbnailWithBuffer(
        const oly_common::String& SpaceId, const oly_systems::BufferAssetDataSource& NewThumbnail, NullResultCallback Callback);

    /// @brief Retrieves the space thumbnail information associated with the space
    /// If the space does not have a thumbnail associated with it the result callback will be successful, the HTTP res code will be ResponseNotFound
    /// and the Uri field inside the UriResult will be empty
    /// @param Space Space : Space for which the thumbnail information will be retrieved
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpaceThumbnail(const oly_common::String& SpaceId, UriResultCallback Callback);

    /// @brief Adds user to group banned list. Banned list can be retrieved from the space
    /// @param Space Space : Space for which the ban will be issued on
    /// @param RequestedUserId oly_common::String : User id to be banned from the space
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddUserToSpaceBanList(
        const oly_common::String& SpaceId, const oly_common::String& RequestedUserId, NullResultCallback Callback);

    /// @brief Deletes user from group banned list. Banned list can be retrieved from the space
    /// @param Space Space : Space for which the Space for which the ban will be removed on
    /// @param RequestedUserId oly_common::String : User id to have ban removed from the space
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteUserFromSpaceBanList(
        const oly_common::String& SpaceId, const oly_common::String& RequestedUserId, NullResultCallback Callback);

    /// @brief Add or update a GeoLocation for the space
    /// @param SpaceId oly_common::String : Id of the space to udpate
    /// @param Location oly_common::Optional<GeoLocation> : The latitude and longitude of the geo location
    /// @param Orientation oly_common::Optional<double> : The compass direction the space points. Must be between 0 (north) and 360 (inclusive)
    /// @param GeoFence oly_common::Optional<oly_common::Array<GeoLocation>> : Array of points that creates a geo fence for the space.
    ///                                                                        Must be in clockwise order and start and end with the same value.
    /// @param Callback SpaceGeoLocationResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateSpaceGeoLocation(const oly_common::String& SpaceId, const oly_common::Optional<GeoLocation>& Location,
        const oly_common::Optional<float>& Orientation, const oly_common::Optional<oly_common::Array<GeoLocation>>& GeoFence,
        SpaceGeoLocationResultCallback Callback);

    /// @brief Get the geo location details for the given space id
    /// @param SpaceId oly_common::String : Id of the space
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetSpaceGeoLocation(const oly_common::String& SpaceId, SpaceGeoLocationResultCallback Callback);

    /// @brief Delete the geo location information of the space
    /// @param SpaceId oly_common::String : Id of the space to be udpated
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteSpaceGeoLocation(const oly_common::String& SpaceId, NullResultCallback Callback);

    ///@}

private:
    SpaceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    OLY_NO_EXPORT SpaceSystem(oly_web::WebClient* InWebClient);

    // Space Metadata
    void GetMetadataAssetCollection(const oly_common::String& SpaceId, AssetCollectionResultCallback Callback);
    void GetMetadataAssetCollections(const oly_common::Array<oly_common::String>& Spaces, AssetCollectionsResultCallback Callback);
    void AddMetadata(
        const oly_common::String& SpaceId, const oly_common::Map<oly_common::String, oly_common::String>& Metadata, NullResultCallback Callback);
    void RemoveMetadata(const oly_common::String& SpaceId, NullResultCallback Callback);

    // Space Thumbnail
    void AddSpaceThumbnail(const oly_common::String& SpaceId, const oly_systems::FileAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void AddSpaceThumbnailWithBuffer(
        const oly_common::String& SpaceId, const oly_systems::BufferAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void GetSpaceThumbnailAssetCollection(const oly_common::String& SpaceId, AssetCollectionsResultCallback Callback);
    void GetSpaceThumbnailAsset(const AssetCollection& ThumbnailAssetCollection, AssetsResultCallback Callback);
    void RemoveSpaceThumbnail(const oly_common::String& SpaceId, NullResultCallback Callback);

    void GetSpaceGeoLocationInternal(const oly_common::String& SpaceId, SpaceGeoLocationResultCallback Callback);

    void SetConnectionCallbacks(oly_multiplayer::MultiplayerConnection* Connection);

    oly_services::ApiBase* GroupAPI;
    Space CurrentSpace;

    oly_multiplayer::SpaceEntitySystem::EntityCreatedCallback EntityCreatedCallback;
    oly_multiplayer::SpaceEntitySystem::CallbackHandler InitialEntitiesRetrievedCallback;
    oly_multiplayer::SpaceEntitySystem::CallbackHandler ScriptSystemReadyCallback;
};

} // namespace oly_systems
