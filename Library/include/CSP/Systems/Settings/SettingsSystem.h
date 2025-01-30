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

#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Settings/SettingsCollection.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

CSP_START_IGNORE
#ifdef CSP_TESTS
class CSPEngine_SettingsSystemTests_AvatarInfoIntTest_Test;
#endif
CSP_END_IGNORE

namespace csp::services
{

class ApiBase;

} // namespace csp::services

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup Settings System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' settings service.
/// Offers methods for storing and retrieving client settings.
class CSP_API SettingsSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<SettingsSystem>(SettingsSystem* Ptr);
#ifdef CSP_TESTS
    friend class ::CSPEngine_SettingsSystemTests_AvatarInfoIntTest_Test;
#endif
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Set a boolean indicating whether the current user has completed a non-disclosure agreement.
    /// NullResultCallback. Returns status of the update task, no payload expected.
    /// @param InValue bool : boolean reflecting desired state to store in Magnopus Connected Services.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void SetNDAStatus(bool InValue, NullResultCallback Callback);

    /// @brief Get a boolean indicating whether the current user has completed a non-disclosure agreement.
    /// @param Callback SettingsBoolCallback : callback to call when a response is received.
    CSP_ASYNC_RESULT void GetNDAStatus(BooleanResultCallback Callback);

    /// @brief Opt in or out to receiving a newsletter for the current user.
    /// NullResultCallback. Returns status of the update task, no payload expected.
    /// @param InValue bool : boolean reflecting desired state to store in Magnopus Connected Services
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void SetNewsletterStatus(bool InValue, NullResultCallback Callback);

    /// @brief Get a boolean indicating whether the current user has opted into receiving a newsletter.
    /// @param Callback SettingsBoolCallback : callback to call when a response is received.
    CSP_ASYNC_RESULT void GetNewsletterStatus(BooleanResultCallback Callback);

    /// @brief Add a Space to the current user's list of recently visited Spaces
    /// Supplying a SpaceID will store as the most recent space, manages the list order and storing to Magnopus Connected Services.
    /// NullResultCallback. Returns status of the update task, no payload expected.
    /// @param InSpaceID csp::common::String : SpaceID of most recent space entered
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void AddRecentlyVisitedSpace(const csp::common::String InSpaceID, NullResultCallback Callback);

    /// @brief Get an array of the most recently visited Spaces for the current user.
    /// Returns an csp::common::Array of csp::common::Strings ordered from most to least recent spaces up to a maximum of 10 entries.
    /// @param Callback SettingsArrayCallback : callback to call when a response is received.
    CSP_ASYNC_RESULT void GetRecentlyVisitedSpaces(StringArrayResultCallback Callback);

    /// @brief Clear the list of recently-visited spaces for the current user.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void ClearRecentlyVisitedSpaces(NullResultCallback Callback);

    /// @brief Block a space for the current user.
    /// The client is expected to implement the actual space filtering functionality as this function only adds the provided space to a list and will
    /// not affect the spaces you get back from `SpaceSystem::GetSpaces()`.
    /// @param InSpaceID csp::common::String : SpaceID of most space to block
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void AddBlockedSpace(const csp::common::String InSpaceID, NullResultCallback Callback);

    /// @brief Unblock a space for the current user.
    /// @param InSpaceID csp::common::String : SpaceID of most space to block
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void RemoveBlockedSpace(const csp::common::String InSpaceID, NullResultCallback Callback);

    /// @brief Get a list of Spaces that were blocked by the current user.
    /// Returns an csp::common::Array of csp::common::Strings ordered from most to least recent blocked spaces.
    /// @param Callback SettingsArrayCallback : callback to call when a response is received.
    CSP_ASYNC_RESULT void GetBlockedSpaces(StringArrayResultCallback Callback);

    /// @brief Clear the list of blocked Spaces for the current user.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void ClearBlockedSpaces(NullResultCallback Callback);

    /// @brief Updates the Portrait Avatar image or adds one if it didn't have it previously using FileAssetDataSource
    /// @param NewAvatarPortrait csp::systems::FileAssetDataSource : New Portrait Avatar information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAvatarPortrait(const csp::systems::FileAssetDataSource& NewAvatarPortrait, NullResultCallback Callback);

    /// @brief Retrieves the Avatar Portrait information associated with the space
    /// If the user of the Avatar portrait associated with it the result callback will be successful, the HTTP res code will be ResponseNotFound
    /// and the Uri field inside the UriResult will be empty
    /// @param InUserID const csp::common::String : The id of the user we are seeking to retrieve the portrait for
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAvatarPortrait(const csp::common::String InUserID, UriResultCallback Callback);

    /// @brief Updates the Avatar Portrait image or adds one if it didn't have it previously using BufferAssetDataSource
    /// @param NewAvatarPortrait csp::systems::BufferAssetDataSource : New Avatar Portrait information
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAvatarPortraitWithBuffer(const csp::systems::BufferAssetDataSource& NewAvatarPortrait, NullResultCallback Callback);

    /// @brief Sets the avatar type and identifier for a user.
    /// @param InType csp::systems::AvatarType : The type of avatar (predefined, Ready Player Me, or custom).
    /// @param InIdentifier csp::common::String : A string used to identify or locate the avatar.
    /// @param Callback NullResultCallback : Callback to call when task finishes.
    CSP_ASYNC_RESULT void SetAvatarInfo(AvatarType InType, const csp::common::String& InIdentifier, NullResultCallback Callback);

    /// @brief Retrieves the avatar type and identifier for a user.
    /// @param Callback NullResultCallback : Callback to call when task finishes.
    CSP_ASYNC_RESULT void GetAvatarInfo(AvatarInfoResultCallback Callback);

private:
    SettingsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT SettingsSystem(csp::web::WebClient* InWebClient);
    ~SettingsSystem();

    void SetSettingValue(const csp::common::String& InContext, const csp::common::String& InKey, const csp::common::String& InValue,
        NullResultCallback Callback) const;
    void GetSettingValue(const csp::common::String& InContext, const csp::common::String& InKey, StringResultCallback Callback) const;

    csp::services::ApiBase* SettingsAPI;

    void AddAvatarPortrait(const csp::systems::FileAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void AddAvatarPortraitWithBuffer(const csp::systems::BufferAssetDataSource& ImageDataSource, NullResultCallback Callback);
    void GetAvatarPortraitAssetCollection(const csp::common::String InUserID, AssetCollectionsResultCallback Callback);
    void GetAvatarPortraitAsset(const AssetCollection& AvatarPortraitAssetCollection, AssetsResultCallback Callback);
    void RemoveAvatarPortrait(NullResultCallback Callback);
};

} // namespace csp::systems
