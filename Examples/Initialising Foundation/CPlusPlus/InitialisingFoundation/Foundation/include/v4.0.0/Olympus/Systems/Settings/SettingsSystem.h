#pragma once

#include "Olympus/Systems/Assets/Asset.h"
#include "Olympus/Systems/Assets/AssetCollection.h"
#include "Olympus/Systems/Settings/SettingsCollection.h"
#include "Olympus/Systems/SystemBase.h"
#include "Olympus/Systems/SystemsResult.h"

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

/// @ingroup Settings System
/// @brief Public facing system that allows interfacing with CHS's settings service.
/// Offers methods for storing and retrieving client settings.
class OLY_API OLY_NO_DISPOSE SettingsSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~SettingsSystem();

    /// @brief Set a boolean indicating whether the current user has completed a non-disclosure agreement.
    /// NullResultCallback. Returns status of the update task, no payload expected.
    /// @param InValue bool : boolean reflecting desired state to store in CHS.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes.
    OLY_ASYNC_RESULT void SetNDAStatus(const oly_common::String& InUserId, bool InValue, NullResultCallback Callback);

    /// @brief Get a boolean indicating whether the current user has completed a non-disclosure agreement.
    /// @param Callback SettingsBoolCallback : callback to call when a response is received.
    OLY_ASYNC_RESULT void GetNDAStatus(const oly_common::String& InUserId, BooleanResultCallback Callback);

    /// @brief Opt in or out to receiving a newsletter for the current user.
    /// NullResultCallback. Returns status of the update task, no payload expected.
    /// @param InValue bool : boolean reflecting desired state to store in CHS
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void SetNewsletterStatus(const oly_common::String& InUserId, bool InValue, NullResultCallback Callback);

    /// @brief Get a boolean indicating whether the current user has opted into receiving a newsletter.
    /// @param Callback SettingsBoolCallback : callback to call when a response is received.
    OLY_ASYNC_RESULT void GetNewsletterStatus(const oly_common::String& InUserId, BooleanResultCallback Callback);

    /// @brief Add a Space to the current user's list of recently visited Spaces
    /// Supplying a SpaceID will store as the most recent space, manages the list order and storing to CHS.
    /// NullResultCallback. Returns status of the update task, no payload expected.
    /// @param InSpaceID oly_common::String : SpaceID of most recent space entered
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddRecentlyVisitedSpace(
        const oly_common::String& InUserId, const oly_common::String InSpaceID, NullResultCallback Callback);

    /// @brief Get an array of the most recently visited Spaces for the current user.
    /// Returns an oly_common::Array of oly_common::Strings ordered from most to least recent spaces up to a maximum of 10 entries.
    /// @param Callback SettingsArrayCallback : callback to call when a response is received.
    OLY_ASYNC_RESULT void GetRecentlyVisitedSpaces(const oly_common::String& InUserId, StringArrayResultCallback Callback);

    /// @brief Clear the list of recently-visited spaces for the current user.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes.
    OLY_ASYNC_RESULT void ClearRecentlyVisitedSpaces(const oly_common::String& InUserId, NullResultCallback Callback);

    /// @brief Block a space for the current user.
    /// The client is expected to implement the actual space filtering functionality as this function only adds the provided space to a list and will
    /// not affect the spaces you get back from `SpaceSystem::GetSpaces()`.
    /// @param InSpaceID oly_common::String : SpaceID of most space to block
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void AddBlockedSpace(const oly_common::String& InUserId, const oly_common::String InSpaceID, NullResultCallback Callback);

    /// @brief Unblock a space for the current user.
    /// @param InSpaceID oly_common::String : SpaceID of most space to block
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void RemoveBlockedSpace(const oly_common::String& InUserId, const oly_common::String InSpaceID, NullResultCallback Callback);

    /// @brief Get a list of Spaces that were blocked by the current user.
    /// Returns an oly_common::Array of oly_common::Strings ordered from most to least recent blocked spaces.
    /// @param Callback SettingsArrayCallback : callback to call when a response is received.
    OLY_ASYNC_RESULT void GetBlockedSpaces(const oly_common::String& InUserId, StringArrayResultCallback Callback);

    /// @brief Clear the list of blocked Spaces for the current user.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes.
    OLY_ASYNC_RESULT void ClearBlockedSpaces(const oly_common::String& InUserId, NullResultCallback Callback);

    /// @brief Updates the Portrait Avatar image or adds one if it didn't have it previously using FileAssetDataSource
    /// @param UserId oly_common::String : UserId of Avatar Portrait
    /// @param NewAvatarPortrait oly_systems::FileAssetDataSource : New Portrait Avatar information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateAvatarPortrait(
        const oly_common::String& UserId, const oly_systems::FileAssetDataSource& NewAvatarPortrait, NullResultCallback Callback);

    /// @brief Retrieves the Avatar Portrait information associated with the space
    /// If the user of the Avatar portrait associated with it the result callback will be successful, the HTTP res code will be ResponseNotFound
    /// and the Uri field inside the UriResult will be empty
    /// @param UserId oly_common::String : UserId of Avatar Portrait
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAvatarPortrait(const oly_common::String& UserId, UriResultCallback Callback);

    /// @brief Updates the Avatar Portrait image or adds one if it didn't have it previously using BufferAssetDataSource
    /// @param UserId oly_common::String : UserId of Avatar Portrait
    /// @param NewAvatarPortrait oly_systems::BufferAssetDataSource : New Avatar Portrait information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateAvatarPortraitWithBuffer(
        const oly_common::String& UserId, const oly_systems::BufferAssetDataSource& NewAvatarPortrait, NullResultCallback Callback);

private:
    SettingsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    OLY_NO_EXPORT SettingsSystem(oly_web::WebClient* InWebClient);

    void SetSettingValue(const oly_common::String& InUserId, const oly_common::String& InContext, const oly_common::String& InKey,
        const oly_common::String& InValue, NullResultCallback Callback) const;
    void GetSettingValue(const oly_common::String& InUserId, const oly_common::String& InContext, const oly_common::String& InKey,
        StringResultCallback Callback) const;

    oly_services::ApiBase* SettingsAPI;

    void AddAvatarPortrait(const oly_common::String& UserId, const oly_systems::FileAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void AddAvatarPortraitWithBuffer(
        const oly_common::String& UserId, const oly_systems::BufferAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void GetAvatarPortraitAssetCollection(const oly_common::String& UserId, AssetCollectionsResultCallback Callback);
    void GetAvatarPortraitAsset(const AssetCollection& AvatarPortraitAssetCollection, AssetsResultCallback Callback);
    void RemoveAvatarPortrait(const oly_common::String& UserId, NullResultCallback Callback);
};

} // namespace oly_systems
