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
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Spaces/Site.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemBase.h"

#include <optional>

namespace csp::services
{

class ApiBase;

} // namespace csp::services

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace async
{
CSP_START_IGNORE
template <typename T> class event_task;
template <typename T> class task;
CSP_END_IGNORE
}

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup Space System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' concept of a Group.
/// Offers methods for creating, deleting and joining spaces.
class CSP_API CSP_NO_DISPOSE SpaceSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<SpaceSystem>(SpaceSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /** @name Helper Functions
     *
     *   @{ */

    /// @brief Enter a space if you have permission to, based on the Space settings.
    /// This includes setting scopes (and toggling event listening in order to set the scope).
    /// It also retrieves all entities in the space. Ensure Connect is called prior to this.
    /// If user does not have permission to discover or enter the space, callback will be called with EResultCode::Failed and
    /// ERequestFailureReason::UserSpaceAccessDenied
    /// @param Space Space : space to enter into
    /// @param Callback EnterSpaceResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void EnterSpace(const csp::common::String& SpaceId, NullResultCallback Callback);

    /// @brief Exits the space and deregisters from the space scope.
    CSP_ASYNC_RESULT void ExitSpace(NullResultCallback Callback);

    /// @brief Get whether user is currently in a space.
    /// @return Result of whether they are in a Space.
    bool IsInSpace();

    /// @brief Get the user's current space.
    /// @return The space data object the user is currently in
    const Space& GetCurrentSpace() const;

    /** @} */

    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */

    /// @brief Creates a new space.
    /// @param Name csp::common::String : name for the new space
    /// @param Description csp::common::String : description for the new space
    /// @param Type csp::systems::SpaceType : type of the new space
    /// @param InviteUsers csp::common::Optional<InviteUserRoleInfoCollection> : Collection containing the email link URL and the users to invite with
    /// their emails and roles
    /// @param Metadata csp::common::String : metadata information for the new space
    /// @param FileThumbnail csp::systems::FileAssetDataSource : optional thumbnail image for the new space
    /// @param Tags csp::common::Array<csp::common::String : optional array of strings to set the metadata tags for the new space
    /// @param Callback csp::systems::SpaceResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateSpace(const csp::common::String& Name, const csp::common::String& Description, SpaceAttributes Attributes,
        const csp::common::Optional<InviteUserRoleInfoCollection>& InviteUsers,
        const csp::common::Map<csp::common::String, csp::common::String>& Metadata,
        const csp::common::Optional<csp::systems::FileAssetDataSource>& FileThumbnail,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, SpaceResultCallback Callback);

    /// @brief Creates a new space Using BufferAssetDataSource.
    /// @param Name csp::common::String : name for the new space
    /// @param Description csp::common::String : description for the new space
    /// @param Type csp::systems::SpaceType : type of the new space
    /// @param InviteUsers csp::common::Optional<InviteUserRoleInfoCollection> : Collection containing the email link URL and the users to invite with
    /// their emails and roles
    /// @param Metadata csp::common::String : metadata information for the new space
    /// @param Thumbnail csp::systems::BufferAssetDataSource : thumbnail image buffer for the new space
    /// @param Tags csp::common::Array<csp::common::String : optional array of strings to set the metadata tags for the new space
    /// @param Callback csp::systems::SpaceResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateSpaceWithBuffer(const csp::common::String& Name, const csp::common::String& Description, SpaceAttributes Attributes,
        const csp::common::Optional<InviteUserRoleInfoCollection>& InviteUsers,
        const csp::common::Map<csp::common::String, csp::common::String>& Metadata, const csp::systems::BufferAssetDataSource& Thumbnail,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, SpaceResultCallback Callback);

    /// @brief Updates the name and/or the description of a Space
    /// @param Space Space : the Space to update
    /// @param Name csp::common::Optional<csp::common::String> : if a new name is provided it will be used to update the Space name
    /// @param Description csp::common::Optional<csp::common::String> : if a new description is provided it will be used to update the Space
    /// description
    /// @param Type csp::common::Optional<csp::systems::SpaceType> : if a new type is provided it will be used to update the Space type
    /// @param Tags csp::common::Optional<csp::common::Array<csp::common::String>> : If new tags are provided they will be used to update the Space
    /// @param Callback BasicSpaceResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateSpace(const csp::common::String& SpaceId, const csp::common::Optional<csp::common::String>& Name,
        const csp::common::Optional<csp::common::String>& Description, const csp::common::Optional<SpaceAttributes>& Type,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, BasicSpaceResultCallback Callback);

    /// @brief Deletes a given space and the corresponding UserService group.
    /// @param Space Space : space to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteSpace(const csp::common::String& SpaceId, NullResultCallback Callback);

    /// @brief Retrieves all spaces corresponding to the currently logged in user.
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpaces(SpacesResultCallback Callback);

    /// @brief Retrieves basic space details for the spaces with the given attributes available to the logged in user.
    /// Results pagination is supported through the use of ResultsSkip and ResultsMax.
    /// @param IsDiscoverable csp::common::Optional<bool> : `true` or `false` to filter by IsDiscoverable attribute value.
    /// @param IsArchived csp::common::Optional<bool> : `true` or `false` to filter by IsArchived attribute value.
    /// @param RequiresInvite csp::common::Optional<bool> : `true` or `false` to filter by RequiresInvite attribute value.
    /// @param ResultsSkip csp::common::Optional<int> : number of result entries that will be skipped from the result. For no skip pass nullptr.
    /// @param ResultsMax csp::common::Optional<int> : maximum number of result entries to be retrieved. For all available result entries pass
    /// nullptr.
    /// @param MustContainTags csp::common::Optional<csp::common::Array<csp::common::String>> : Array of tags that must be present in retrieved
    /// spaces. For no mandatory tags pass nullptr.
    /// @param MustExcludeTags csp::common::Optional<csp::common::Array<csp::common::String>> : Array of tags that must not be present in retrieved
    /// spaces. For no excluded tags pass nullptr.
    /// @param MustIncludeAllTags csp::common::Optional<bool> : Whether all tags in @param MustContainTags must be present in retrieved spaces.
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpacesByAttributes(const csp::common::Optional<bool>& IsDiscoverable, const csp::common::Optional<bool>& IsArchived,
        const csp::common::Optional<bool>& RequiresInvite, const csp::common::Optional<int>& ResultsSkip,
        const csp::common::Optional<int>& ResultsMax, const csp::common::Optional<csp::common::Array<csp::common::String>>& MustContainTags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& MustExcludeTags, const csp::common::Optional<bool>& MustIncludeAllTags,
        BasicSpacesResultCallback Callback);

    /// @brief Retrieves space details corresponding to the provided Space IDs
    /// @param RequestedSpaceIDs csp::common::Array<csp::common::String> : array of Space IDs for which the space details will be retrieved
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpacesByIds(const csp::common::Array<csp::common::String>& RequestedSpaceIDs, SpacesResultCallback Callback);

    /// @brief Retrieves all spaces corresponding to the provided user ID.
    /// @param UserId csp::common::String : unique ID of user
    /// @param Callback SpacesResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpacesForUserId(const csp::common::String& UserId, SpacesResultCallback Callback);

    /// @brief Retrieves a space by its unique ID.
    /// @param SpaceId csp::common::String : unique ID of Space
    /// @param Callback SpaceResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpace(const csp::common::String& SpaceId, SpaceResultCallback Callback);
    CSP_NO_EXPORT async::task<SpaceResult> GetSpace(const csp::common::String& SpaceId);

    /// @brief Invites a given email to a specific space.
    /// @param Space Space : space to invite to
    /// @param Email csp::common::String : email to invite to space
    /// @param IsModeratorRole csp::common::Optional<bool> : if present and true sets the user's role in the space to "Moderator", pass false or
    /// nullptr to leave role as default
    /// @param EmailLinkUrl csp::common::Optional<csp::common::String> : link that will be provided in the invite email
    /// @param SignupUrl csp::common::Optional<csp::common::String> : destination link that will be provided in the invite email
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void InviteToSpace(const csp::common::String& SpaceId, const csp::common::String& Email,
        const csp::common::Optional<bool>& IsModeratorRole, const csp::common::Optional<csp::common::String>& EmailLinkUrl,
        const csp::common::Optional<csp::common::String>& SignupUrl, NullResultCallback Callback);

    /// @brief Invites all the given emails to a specific space.
    /// @param Space Space : space to invite to
    /// @param InviteUsers InviteUserRoleInfoCollection : Collection containing the email link URL and the users to invite with their emails and roles
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void BulkInviteToSpace(
        const csp::common::String& SpaceId, const InviteUserRoleInfoCollection& InviteUsers, NullResultCallback Callback);

    /// @brief Returns an array of obfuscated email addresses, addresses of users that have not yet accepted the space invite
    /// @param Space Space : Space for which the invites where sent
    /// @param Callback PendingInvitesResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetPendingUserInvites(const csp::common::String& SpaceId, PendingInvitesResultCallback Callback);

    /// @brief Returns an array of ids of users that accepted the space invite
    /// @param Space Space : Space for which the invites where sent
    /// @param Callback AcceptedInvitesResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAcceptedUserInvites(const csp::common::String& SpaceId, AcceptedInvitesResultCallback Callback);

    /// @brief Removes a user from a space by the user's unique ID.
    /// @param Space Space : space to remove user from
    /// @param UserId csp::common::String : unique ID of user
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void RemoveUserFromSpace(const csp::common::String& SpaceId, const csp::common::String& UserId, NullResultCallback Callback);

    /// @brief Adds a user to a space by the user's unique ID.
    /// @param Space Space : space to add user to
    /// @param UserId csp::common::String : unique ID of user
    /// @param Callback SpaceResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void AddUserToSpace(const csp::common::String& SpaceId, const csp::common::String& UserId, SpaceResultCallback Callback);
    CSP_NO_EXPORT async::task<SpaceResult> AddUserToSpace(const csp::common::String& SpaceId, const csp::common::String& UserId);

    /// @brief Creates new Site information and associates it with the Space.
    /// @param Space Space : Space to associate the Site information with
    /// @param SiteInfo Site : Site information to be added
    /// @param Callback SiteResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void AddSiteInfo(const csp::common::String& SpaceId, Site& SiteInfo, SiteResultCallback Callback);

    /// @brief Removes the Site information from the Space.
    /// @param Space Space : Space for which to remove the associated Site information
    /// @param SiteInfo Site : Site information to be removed
    /// @param Callback NullResultCallback : callback when asynchronous task
    CSP_ASYNC_RESULT void RemoveSiteInfo(const csp::common::String& SpaceId, Site& SiteInfo, NullResultCallback Callback);

    /// @brief Retrieves the Sites information associated with a Space.
    /// @param Space Space : Space to be queried for Site information
    /// @param Callback SitesCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSitesInfo(const csp::common::String& SpaceId, SitesCollectionResultCallback Callback);

    /// @brief Updates the space role for a particular user
    /// @param Space Space : The space that the requested user is part of
    /// @param NewUserRoleInfo UserRoleInfo : New user role information containing the new role for the specified user
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateUserRole(const csp::common::String& SpaceId, const UserRoleInfo& NewUserRoleInfo, NullResultCallback Callback);

    /// @brief Retrieves the User role information for the User Ids that have been passed in
    /// @param Space Space : Space for which the User Roles will be retrieved
    /// @param RequestedUserIds csp::common::Array<csp::common::String> : Array of User Ids for which the User Roles will be retrieved
    /// @param Callback UserRoleCollectionCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetUsersRoles(
        const csp::common::String& SpaceId, const csp::common::Array<csp::common::String>& RequestedUserIds, UserRoleCollectionCallback Callback);

    /// @brief Updates the Space metadata information with the new one provided
    /// @param SpaceId csp::common::String : ID of Space for which the metadata will be updated
    /// @param NewMetadata csp::common::String : New metadata information that will replace the previous one
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateSpaceMetadata(const csp::common::String& SpaceId,
        const csp::common::Map<csp::common::String, csp::common::String>& NewMetadata, NullResultCallback Callback);

    /// @brief Retrieves Spaces metadata information
    /// @param Spaces csp::common::Array<Space> : Spaces for which metadata will be retrieved
    /// @param Callback SpacesMetadataResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpacesMetadata(const csp::common::Array<csp::common::String>& Spaces, SpacesMetadataResultCallback Callback);

    /// @brief Retrieves the Space metadata information
    /// @param Space Space : Space for which the metadata will be retrieved
    /// @param Callback SpaceMetadataResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpaceMetadata(const csp::common::String& SpaceId, SpaceMetadataResultCallback Callback);

    /// @brief Updates the Space thumbnail image or adds one if it didn't have it previously using FileAssetDataSource
    /// @param Space Space : Space for which the thumbnail will be updated
    /// @param NewThumbnail csp::systems::FileAssetDataSource : New thumbnail information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateSpaceThumbnail(
        const csp::common::String& SpaceId, const csp::systems::FileAssetDataSource& NewThumbnail, NullResultCallback Callback);

    /// @brief Updates the Space thumbnail image or adds one if it didn't have it previously using BufferAssetDataSource
    /// @param Space Space : Space for which the thumbnail will be updated
    /// @param NewThumbnail csp::systems::BufferAssetDataSource : New thumbnail information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateSpaceThumbnailWithBuffer(
        const csp::common::String& SpaceId, const csp::systems::BufferAssetDataSource& NewThumbnail, NullResultCallback Callback);

    /// @brief Retrieves the space thumbnail information associated with the space
    /// If the space does not have a thumbnail associated with it the result callback will be successful, the HTTP res code will be ResponseNotFound
    /// and the Uri field inside the UriResult will be empty
    /// @param Space Space : Space for which the thumbnail information will be retrieved
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpaceThumbnail(const csp::common::String& SpaceId, UriResultCallback Callback);

    /// @brief Adds user to group banned list. Banned list can be retrieved from the space
    /// @param Space Space : Space for which the ban will be issued on
    /// @param RequestedUserId csp::common::String : User id to be banned from the space
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void AddUserToSpaceBanList(
        const csp::common::String& SpaceId, const csp::common::String& RequestedUserId, NullResultCallback Callback);

    /// @brief Deletes user from group banned list. Banned list can be retrieved from the space
    /// @param Space Space : Space for which the Space for which the ban will be removed on
    /// @param RequestedUserId csp::common::String : User id to have ban removed from the space
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteUserFromSpaceBanList(
        const csp::common::String& SpaceId, const csp::common::String& RequestedUserId, NullResultCallback Callback);

    /// @brief Add or update a GeoLocation for the space
    /// @param SpaceId csp::common::String : Id of the space to udpate
    /// @param Location csp::common::Optional<GeoLocation> : The latitude and longitude of the geo location
    /// @param Orientation csp::common::Optional<double> : The compass direction the space points. Must be between 0 (north) and 360 (inclusive)
    /// @param GeoFence csp::common::Optional<csp::common::Array<GeoLocation>> : Array of points that creates a geo fence for the space.
    ///                                                                        Must be in clockwise order and start and end with the same value.
    /// @param Callback SpaceGeoLocationResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::Optional<GeoLocation>& Location,
        const csp::common::Optional<float>& Orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence,
        SpaceGeoLocationResultCallback Callback);

    /// @brief Get the geo location details for the given space id
    /// @param SpaceId csp::common::String : Id of the space
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetSpaceGeoLocation(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback);

    /// @brief Delete the geo location information of the space
    /// @param SpaceId csp::common::String : Id of the space to be udpated
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteSpaceGeoLocation(const csp::common::String& SpaceId, NullResultCallback Callback);

    /// @brief Duplicate an existing space and assign it to the current user
    /// @param SpaceId csp::common::String : Id of the space to duplicate.
    /// @param NewName csp::common::String : A unique name for the duplicated space.
    /// @param NewAttributes csp::systems::SpaceAttributes : Attributes to apply to the duplicated space.
    /// @param MemberGroupIds csp::common::Array<csp::common::String> : An optional array of group (space) IDs to copy users from.
    /// @param ShallowCopy bool : If true, the duplicated space will reference the assets of the original space. Otherwise, all assets will be
    /// duplicated.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DuplicateSpace(const csp::common::String& SpaceId, const csp::common::String& NewName, SpaceAttributes NewAttributes,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& MemberGroupIds, bool ShallowCopy, SpaceResultCallback Callback);

    ///@}

private:
    SpaceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    SpaceSystem(csp::web::WebClient* InWebClient);
    ~SpaceSystem();

    // Space Metadata
    void GetMetadataAssetCollection(const csp::common::String& SpaceId, AssetCollectionResultCallback Callback);
    void GetMetadataAssetCollections(const csp::common::Array<csp::common::String>& Spaces, AssetCollectionsResultCallback Callback);
    void AddMetadata(
        const csp::common::String& SpaceId, const csp::common::Map<csp::common::String, csp::common::String>& Metadata, NullResultCallback Callback);
    void RemoveMetadata(const csp::common::String& SpaceId, NullResultCallback Callback);

    // Space Thumbnail
    void AddSpaceThumbnail(const csp::common::String& SpaceId, const csp::systems::FileAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void AddSpaceThumbnailWithBuffer(
        const csp::common::String& SpaceId, const csp::systems::BufferAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void GetSpaceThumbnailAssetCollection(const csp::common::String& SpaceId, AssetCollectionsResultCallback Callback);
    void GetSpaceThumbnailAsset(const AssetCollection& ThumbnailAssetCollection, AssetsResultCallback Callback);
    void RemoveSpaceThumbnail(const csp::common::String& SpaceId, NullResultCallback Callback);

    void GetSpaceGeoLocationInternal(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback);

    // EnterSpace Continuations
    auto AddUserToSpaceIfNecessary(NullResultCallback Callback, SpaceSystem& SpaceSystem);
    auto FireEnterSpaceEvent(Space& OutCurrentSpace);
    // Wraps RefreshMultiplayerConnectionToEnactScopeChange as a continuation.
    auto RefreshMultiplayerScopes();

    /* Stop the multiplayer connetion, change scope, start listening again
       Not ideal, we'd rather not have to go to all this effort.
       Used in EnterSpace. */
    void RefreshMultiplayerConnectionToEnactScopeChange(csp::common::String SpaceId,
        std::shared_ptr<async::event_task<std::optional<csp::multiplayer::ErrorCode>>> RefreshMultiplayerContinuationEvent);

    csp::services::ApiBase* GroupAPI;
    csp::services::ApiBase* SpaceAPI;
    Space CurrentSpace;
};

} // namespace csp::systems
