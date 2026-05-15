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

#include "CSP/Systems/Settings/SettingsSystem.h"

#include "CSP/Common/Array.h"
#include "CSP/Common/List.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/ResultHelpers.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"

#include <iostream>
#include <rapidjson/rapidjson.h>
#include <rapidjson/error/en.h>
#include <sstream>

constexpr int MAX_RECENT_SPACES = 50;
constexpr const char* AVATAR_PORTRAIT_ASSET_NAME = "AVATAR_PORTRAIT_ASSET_";
constexpr const char* AVATAR_PORTRAIT_ASSET_COLLECTION_NAME = "AVATAR_PORTRAIT_ASSET_COLLECTION_";

using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace csp::systems
{

SettingsSystem::SettingsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_settingsApi(nullptr)
{
}

SettingsSystem::SettingsSystem(web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_settingsApi = new chs::SettingsApi(inWebClient);
}

SettingsSystem::~SettingsSystem() { delete (m_settingsApi); }

void SettingsSystem::SetSettingValue(const String& inContext, const String& inKey, const String& inValue, NullResultCallback callback) const
{
    auto& systemsManager = SystemsManager::Get();
    const auto* userSystem = systemsManager.GetUserSystem();

    const auto& userId = userSystem->GetLoginState().UserId;

    auto inSettings = std::make_shared<chs::SettingsDto>();
    std::map<String, String> newSettings;
    newSettings.clear();
    newSettings.insert(std::make_pair(inKey, inValue));
    inSettings->SetSettings(newSettings);

    SettingsResultCallback internalCallback = [inKey, callback](const SettingsCollectionResult& result)
    {
        if (result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        NullResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    services::ResponseHandlerPtr settingsResponseHandler
        = m_settingsApi->CreateHandler<SettingsResultCallback, SettingsCollectionResult, void, chs::SettingsDto>(
            internalCallback, nullptr, web::EResponseCodes::ResponseOK);

    static_cast<chs::SettingsApi*>(m_settingsApi)->usersUserIdSettingsContextPut({ userId, inContext, inSettings }, settingsResponseHandler);
}

void SettingsSystem::GetSettingValue(const String& inContext, const String& inKey, StringResultCallback callback) const
{
    auto& systemsManager = SystemsManager::Get();
    const auto* userSystem = systemsManager.GetUserSystem();
    std::vector<String> myKey = { inKey };

    const auto& userId = userSystem->GetLoginState().UserId;

    SettingsResultCallback internalCallback = [inKey, callback](const SettingsCollectionResult& result)
    {
        StringResult stringResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == EResultCode::Success)
        {
            // Only attempt to read settings if result is valid

            auto& settings = result.GetSettingsCollection().Settings;

            if (settings.HasKey(inKey))
            {
                stringResult.SetValue(settings[inKey]);
            }
            else
            {
                stringResult.SetValue("");
            }
        }
        else
        {
            // Handle failure state
            stringResult.SetValue("");
        }

        INVOKE_IF_NOT_NULL(callback, stringResult);
    };

    services::ResponseHandlerPtr settingsResponseHandler
        = m_settingsApi->CreateHandler<SettingsResultCallback, SettingsCollectionResult, void, chs::SettingsDto>(
            internalCallback, nullptr, web::EResponseCodes::ResponseOK);

    static_cast<chs::SettingsApi*>(m_settingsApi)->usersUserIdSettingsContextGet({ userId, inContext, myKey }, settingsResponseHandler);
}

void SettingsSystem::SetNDAStatus(bool inValue, NullResultCallback callback)
{
    const String ndaStatus = inValue ? "true" : "false";

    SetSettingValue("UserSettings", "NDAStatus", ndaStatus, callback);
}

void SettingsSystem::GetNDAStatus(BooleanResultCallback callback)
{
    StringResultCallback getSettingCallback = [=](const StringResult& result)
    {
        BooleanResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
        internalResult.SetValue(result.GetValue() == "true");
        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetSettingValue("UserSettings", "NDAStatus", getSettingCallback);
}

void SettingsSystem::SetNewsletterStatus(bool inValue, NullResultCallback callback)
{
    const String newsletterStatus = inValue ? "true" : "false";

    SetSettingValue("UserSettings", "Newsletter", newsletterStatus, callback);
}

void SettingsSystem::GetNewsletterStatus(BooleanResultCallback callback)
{
    StringResultCallback getSettingCallback = [=](const StringResult& result)
    {
        BooleanResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
        internalResult.SetValue(result.GetValue() == "true");

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetSettingValue("UserSettings", "Newsletter", getSettingCallback);
}

void SettingsSystem::AddRecentlyVisitedSpace(const String inSpaceId, NullResultCallback callback)
{
    StringArrayResultCallback getRecentSpacesCallback = [=](StringArrayResult result)
    {
        if (result.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& recentSpacesArray = result.GetValue();

        auto recentSpaces = recentSpacesArray.ToList();
        recentSpaces.Insert(0, inSpaceId);

        // Remove duplicate entry
        for (size_t i = recentSpaces.Size() - 1; i > 0; --i)
        {
            if (recentSpaces[i] == inSpaceId)
            {
                recentSpaces.Remove(i);

                // Only a single duplicate entry (if any) should be found
                break;
            }
        }

        // Remove last entry if max number of spaces reached
        if (recentSpaces.Size() > MAX_RECENT_SPACES)
        {
            recentSpaces.Remove(MAX_RECENT_SPACES);
        }

        auto recentSpacesString = String::Join(recentSpaces, ',');

        SetSettingValue("UserSettings", "RecentSpaces", recentSpacesString, callback);
    };

    GetRecentlyVisitedSpaces(getRecentSpacesCallback);
}

void SettingsSystem::GetRecentlyVisitedSpaces(StringArrayResultCallback callback)
{
    StringResultCallback getSettingCallback = [=](const StringResult& result)
    {
        StringArrayResult internalResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Success)
        {
            auto& recentSpacesString = result.GetValue();

            if (recentSpacesString.IsEmpty())
            {
                internalResult.SetValue({});
            }
            else
            {
                auto recentSpacesList = recentSpacesString.Split(',');
                auto recentSpaces = recentSpacesList.ToArray();
                internalResult.SetValue(recentSpaces);
            }
        }

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetSettingValue("UserSettings", "RecentSpaces", getSettingCallback);
}

void SettingsSystem::ClearRecentlyVisitedSpaces(NullResultCallback callback) { SetSettingValue("UserSettings", "RecentSpaces", "", callback); }

void SettingsSystem::AddBlockedSpace(const String inSpaceId, NullResultCallback callback)
{
    StringArrayResultCallback getBlockedSpacesCallback = [=](StringArrayResult result)
    {
        if (result.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& blockedSpacesArray = result.GetValue();

        // Ignore if space already blocked
        for (size_t i = 0; i < blockedSpacesArray.Size(); ++i)
        {
            if (blockedSpacesArray[i] == inSpaceId)
            {
                NullResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(callback, internalResult);

                return;
            }
        }

        auto blockedSpaces = blockedSpacesArray.ToList();
        blockedSpaces.Insert(0, inSpaceId);

        auto blockedSpacesString = String::Join(blockedSpaces, ',');

        SetSettingValue("UserSettings", "BlockedSpaces", blockedSpacesString, callback);
    };

    GetBlockedSpaces(getBlockedSpacesCallback);
}

void SettingsSystem::RemoveBlockedSpace(const String inSpaceId, NullResultCallback callback)
{
    StringArrayResultCallback getBlockedSpacesCallback = [=](StringArrayResult result)
    {
        if (result.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& blockedSpacesArray = result.GetValue();
        auto foundSpace = false;

        // Ignore if space not blocked
        for (size_t i = 0; i < blockedSpacesArray.Size(); ++i)
        {
            if (blockedSpacesArray[i] == inSpaceId)
            {
                foundSpace = true;

                break;
            }
        }

        if (!foundSpace)
        {
            NullResult internalResult(result.GetResultCode(), result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        auto blockedSpaces = blockedSpacesArray.ToList();
        blockedSpaces.RemoveItem(inSpaceId);

        auto blockedSpacesString = String::Join(blockedSpaces, ',');

        SetSettingValue("UserSettings", "BlockedSpaces", blockedSpacesString, callback);
    };

    GetBlockedSpaces(getBlockedSpacesCallback);
}

void SettingsSystem::GetBlockedSpaces(StringArrayResultCallback callback)
{
    StringResultCallback getSettingCallback = [=](const StringResult& result)
    {
        StringArrayResult internalResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Success)
        {
            const auto& blockedSpacesString = result.GetValue();

            if (blockedSpacesString.IsEmpty())
            {
                internalResult.SetValue({});
            }
            else
            {
                auto blockedSpacesList = blockedSpacesString.Split(',');
                auto blockedSpaces = blockedSpacesList.ToArray();
                internalResult.SetValue(blockedSpaces);
            }
        }

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetSettingValue("UserSettings", "BlockedSpaces", getSettingCallback);
}

void SettingsSystem::ClearBlockedSpaces(NullResultCallback callback) { SetSettingValue("UserSettings", "BlockedSpaces", "", callback); }

void SettingsSystem::UpdateAvatarPortrait(const FileAssetDataSource& newAvatarPortrait, NullResultCallback callback)
{
    AssetCollectionsResultCallback avatarPortraitAssetCollCallback = [=](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& assetCollections = assetCollResult.GetAssetCollections();

            if (assetCollections.IsEmpty())
            {
                // space without a thumbnail
                AddAvatarPortrait(newAvatarPortrait, callback);
            }
            else
            {
                const auto& avatarPortraitAssetCollection = assetCollections[0];

                AssetsResultCallback avatarPortraitAssetCallback = [=](const AssetsResult& portraitResult)
                {
                    if (portraitResult.GetResultCode() == EResultCode::Success)
                    {
                        UriResultCallback uploadCallback = [=](const UriResult& uploadResult)
                        {
                            if (uploadResult.GetResultCode() == EResultCode::Failed)
                            {
                                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                                    static_cast<int>(uploadResult.GetResultCode()), uploadResult.GetHttpResultCode());
                            }

                            const NullResult internalResult(uploadResult);
                            INVOKE_IF_NOT_NULL(callback, internalResult);
                        };

                        auto& avatarPortraitAsset = ((AssetsResult&)portraitResult).GetAssets()[0];
                        avatarPortraitAsset.MimeType = newAvatarPortrait.GetMimeType();
                        const auto assetSystem = SystemsManager::Get().GetAssetSystem();

                        assetSystem->UploadAssetData(avatarPortraitAssetCollection, avatarPortraitAsset, newAvatarPortrait, uploadCallback);
                    }
                    else
                    {
                        NullResult internalResult(portraitResult);
                        INVOKE_IF_NOT_NULL(callback, internalResult);
                    }
                };

                GetAvatarPortraitAsset(avatarPortraitAssetCollection, avatarPortraitAssetCallback);
            }
        }
        else
        {
            const NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        }
    };

    auto* userSystem = SystemsManager::Get().GetUserSystem();
    const auto& userId = userSystem->GetLoginState().UserId;
    GetAvatarPortraitAssetCollection(userId, avatarPortraitAssetCollCallback);
}

void SettingsSystem::GetAvatarPortrait(const csp::common::String inUserId, UriResultCallback callback)
{
    AssetCollectionsResultCallback avatarPortraitAssetCollCallback = [=](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& assetCollections = assetCollResult.GetAssetCollections();

            if (assetCollections.IsEmpty())
            {
                // User Doesn't have a Avatar Portrait
                const UriResult internalResult(EResultCode::Success, static_cast<uint16_t>(web::EResponseCodes::ResponseNotFound));
                INVOKE_IF_NOT_NULL(callback, internalResult);
            }
            else
            {
                const auto& avatarPortraitAssetCollection = assetCollections[0];

                AssetsResultCallback avatarPortraitAssetCallback = [=](const AssetsResult& assetsResult)
                {
                    if (assetsResult.GetResultCode() == EResultCode::Success)
                    {
                        const auto& assets = assetsResult.GetAssets();

                        if (assets.Size() > 0)
                        {
                            const UriResult internalResult(assetsResult.GetAssets()[0].Uri);
                            INVOKE_IF_NOT_NULL(callback, internalResult);
                        }
                        else
                        {
                            UriResult internalResult(EResultCode::Failed, 200);
                            internalResult.SetResponseBody(
                                "Invalid avatar portrait AssetCollection. AssetCollection should contain an Asset but does not!");
                            internalResult.m_uri = "";

                            INVOKE_IF_NOT_NULL(callback, internalResult);
                        }
                    }
                    else
                    {
                        const UriResult internalResult(assetsResult.GetResultCode(), assetsResult.GetHttpResultCode());
                        INVOKE_IF_NOT_NULL(callback, internalResult);
                    }
                };

                GetAvatarPortraitAsset(avatarPortraitAssetCollection, avatarPortraitAssetCallback);
            }
        }
        else
        {
            const UriResult internalResult(assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, internalResult);
        }
    };

    GetAvatarPortraitAssetCollection(inUserId, avatarPortraitAssetCollCallback);
}

void SettingsSystem::UpdateAvatarPortraitWithBuffer(const BufferAssetDataSource& newAvatarPortrait, NullResultCallback callback)
{
    AssetCollectionsResultCallback thumbnailAssetCollCallback = [=](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& assetCollections = assetCollResult.GetAssetCollections();

            if (assetCollections.IsEmpty())
            {
                // space without a thumbnail
                AddAvatarPortraitWithBuffer(newAvatarPortrait, callback);
            }
            else
            {
                const auto& thumbnailAssetCollection = assetCollections[0];

                AssetsResultCallback thumbnailAssetCallback = [=](const AssetsResult& thumbnailResult)
                {
                    if (thumbnailResult.GetResultCode() == EResultCode::Success)
                    {
                        UriResultCallback uploadCallback = [=](const UriResult& uploadResult)
                        {
                            if (uploadResult.GetResultCode() == EResultCode::Failed)
                            {
                                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                                    (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                            }

                            const NullResult internalResult(uploadResult);
                            INVOKE_IF_NOT_NULL(callback, internalResult);
                        };

                        auto& thumbnailAsset = ((AssetsResult&)thumbnailResult).GetAssets()[0];
                        thumbnailAsset.FileName = SpaceSystemHelpers::GetUniqueAvatarThumbnailAssetName(
                            SpaceSystemHelpers::GetAssetFileExtension(newAvatarPortrait.GetMimeType()));
                        thumbnailAsset.MimeType = newAvatarPortrait.GetMimeType();
                        const auto assetSystem = SystemsManager::Get().GetAssetSystem();
                        assetSystem->UploadAssetData(thumbnailAssetCollection, thumbnailAsset, newAvatarPortrait, uploadCallback);
                    }
                    else
                    {
                        NullResult internalResult(thumbnailResult);
                        INVOKE_IF_NOT_NULL(callback, internalResult);
                    }
                };

                GetAvatarPortraitAsset(thumbnailAssetCollection, thumbnailAssetCallback);
            }
        }
        else
        {
            const NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        }
    };

    auto* userSystem = SystemsManager::Get().GetUserSystem();
    const auto& userId = userSystem->GetLoginState().UserId;
    GetAvatarPortraitAssetCollection(userId, thumbnailAssetCollCallback);
}

void SettingsSystem::AddAvatarPortrait(const FileAssetDataSource& imageDataSource, NullResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* assetSystem = systemsManager.GetAssetSystem();
    auto* userSystem = systemsManager.GetUserSystem();

    const auto& userId = userSystem->GetLoginState().UserId;

    AssetCollectionResultCallback createAssetCollCallback = [=](const AssetCollectionResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& portraitAssetColl = assetCollResult.GetAssetCollection();

            AssetResultCallback createAssetCallback = [=](const AssetResult& createAssetResult)
            {
                if (createAssetResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                if (createAssetResult.GetResultCode() == EResultCode::Success)
                {
                    UriResultCallback uploadCallback = [=](const UriResult& uploadResult)
                    {
                        if (uploadResult.GetResultCode() == EResultCode::Failed)
                        {
                            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait upload data has failed. ResCode: %d, HttpResCode: %d",
                                (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                        }

                        const NullResult internalResult(uploadResult);
                        INVOKE_IF_NOT_NULL(callback, internalResult);
                    };

                    assetSystem->UploadAssetData(portraitAssetColl, createAssetResult.GetAsset(), imageDataSource, uploadCallback);
                }
                else
                {
                    CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset creation was not successful. ResCode: %d, HttpResCode: %d",
                        (int)createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode());

                    const NullResult internalResult(createAssetResult);
                    INVOKE_IF_NOT_NULL(callback, internalResult);
                }
            };

            const auto uniqueAssetName = AVATAR_PORTRAIT_ASSET_NAME + userId;
            assetSystem->CreateAsset(portraitAssetColl, uniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, createAssetCallback);
        }
        else
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());

            const NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        }
    };

    const auto avatarPortraitAssetCollectionName = AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + userId;

    assetSystem->CreateAssetCollection(
        nullptr, nullptr, avatarPortraitAssetCollectionName, nullptr, EAssetCollectionType::DEFAULT, nullptr, createAssetCollCallback);
}

void SettingsSystem::AddAvatarPortraitWithBuffer(const BufferAssetDataSource& imageDataSource, NullResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* assetSystem = systemsManager.GetAssetSystem();
    auto* userSystem = systemsManager.GetUserSystem();

    const auto& userId = userSystem->GetLoginState().UserId;

    AssetCollectionResultCallback createAssetCollCallback = [=](const AssetCollectionResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& portraitAvatarAssetColl = assetCollResult.GetAssetCollection();

            AssetResultCallback createAssetCallback = [=](const AssetResult& createAssetResult)
            {
                if (createAssetResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                if (createAssetResult.GetResultCode() == EResultCode::Success)
                {
                    UriResultCallback uploadCallback = [=](const UriResult& uploadResult)
                    {
                        if (uploadResult.GetResultCode() == EResultCode::Failed)
                        {
                            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait upload data has failed. ResCode: %d, HttpResCode: %d",
                                (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                        }

                        const NullResult internalResult(uploadResult);
                        INVOKE_IF_NOT_NULL(callback, internalResult);
                    };

                    Asset avatarPortraitAsset = createAssetResult.GetAsset();

                    avatarPortraitAsset.FileName
                        = AVATAR_PORTRAIT_ASSET_NAME + userId + SpaceSystemHelpers::GetAssetFileExtension(imageDataSource.GetMimeType());
                    avatarPortraitAsset.MimeType = imageDataSource.GetMimeType();
                    assetSystem->UploadAssetData(portraitAvatarAssetColl, avatarPortraitAsset, imageDataSource, uploadCallback);
                }
                else
                {
                    CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset creation was not successful. ResCode: %d, HttpResCode: %d",
                        (int)createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode());

                    const NullResult internalResult(createAssetResult);
                    INVOKE_IF_NOT_NULL(callback, internalResult);
                }
            };

            const auto uniqueAssetName = AVATAR_PORTRAIT_ASSET_NAME + userId;
            assetSystem->CreateAsset(portraitAvatarAssetColl, uniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, createAssetCallback);
        }
        else
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());

            const NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        }
    };

    const String spaceThumbnailAssetCollectionName = AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + userId;

    // don't associate this asset collection with a particular space so that it can be retrieved by guest users that have not joined this space
    assetSystem->CreateAssetCollection(
        nullptr, nullptr, spaceThumbnailAssetCollectionName, nullptr, EAssetCollectionType::DEFAULT, nullptr, createAssetCollCallback);
}

void SettingsSystem::GetAvatarPortraitAssetCollection(const csp::common::String inUserId, AssetCollectionsResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* assetSystem = systemsManager.GetAssetSystem();

    AssetCollectionsResultCallback getAssetCollCallback = [=](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());
        }

        INVOKE_IF_NOT_NULL(callback, assetCollResult);
    };

    Array<String> avatarPortraitAssetCollectionName = { AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + inUserId };

    Array<String> prototypeNames = { avatarPortraitAssetCollectionName };
    Array<EAssetCollectionType> prototypeTypes = { EAssetCollectionType::DEFAULT };

    assetSystem->FindAssetCollections(nullptr, nullptr, prototypeNames, prototypeTypes, nullptr, nullptr, nullptr, nullptr, getAssetCollCallback);
}

void SettingsSystem::GetAvatarPortraitAsset(const AssetCollection& avatarPortraitAssetCollection, AssetsResultCallback callback)
{
    AssetsResultCallback thumbnailAssetCallback = [=](const AssetsResult& assetsResult)
    {
        if (assetsResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)assetsResult.GetResultCode(), assetsResult.GetHttpResultCode());
        }

        INVOKE_IF_NOT_NULL(callback, assetsResult);
    };

    const auto assetSystem = SystemsManager::Get().GetAssetSystem();
    assetSystem->GetAssetsInCollection(avatarPortraitAssetCollection, thumbnailAssetCallback);
}

void SettingsSystem::RemoveAvatarPortrait(NullResultCallback callback)
{
    const auto assetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionsResultCallback portraitAvatarAssetCollCallback = [=](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& assetCollections = assetCollResult.GetAssetCollections();

            if (assetCollections.IsEmpty())
            {
                // user doesnt have a avatar portrait
                const NullResult internalResult(assetCollResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);
            }
            else
            {
                const auto& portraitAvatarAssetCollection = assetCollections[0];

                AssetsResultCallback portraitAvatarAssetCallback = [=](const AssetsResult& portraitResult)
                {
                    if (portraitResult.GetResultCode() == EResultCode::InProgress)
                    {
                        return;
                    }

                    if (portraitResult.GetResultCode() == EResultCode::Success)
                    {
                        NullResultCallback deleteAssetCallback = [=](const NullResult& deleteAssetResult)
                        {
                            if (deleteAssetResult.GetResultCode() == EResultCode::InProgress)
                            {
                                return;
                            }

                            if (deleteAssetResult.GetResultCode() == EResultCode::Success)
                            {
                                NullResultCallback deleteAssetCollCallback = [=](const NullResult& deleteAssetCollResult)
                                {
                                    if (deleteAssetCollResult.GetResultCode() == EResultCode::Failed)
                                    {
                                        CSP_LOG_FORMAT(LogLevel::Error,
                                            "The Portrait Avatar asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
                                            (int)deleteAssetResult.GetResultCode(), deleteAssetResult.GetHttpResultCode());
                                    }

                                    NullResult internalResult(deleteAssetCollResult);
                                    INVOKE_IF_NOT_NULL(callback, deleteAssetCollResult);
                                };

                                assetSystem->DeleteAssetCollection(portraitAvatarAssetCollection, deleteAssetCollCallback);
                            }
                            else
                            {
                                CSP_LOG_FORMAT(LogLevel::Error, "The Portrait Avatar asset deletion was not successful. ResCode: %d, HttpResCode: %d",
                                    (int)deleteAssetResult.GetResultCode(), deleteAssetResult.GetHttpResultCode());

                                NullResult internalResult(deleteAssetResult);
                                INVOKE_IF_NOT_NULL(callback, deleteAssetResult);
                            }
                        };

                        const auto& portraitAvatarAsset = portraitResult.GetAssets()[0];
                        assetSystem->DeleteAsset(portraitAvatarAssetCollection, portraitAvatarAsset, deleteAssetCallback);
                    }
                    else
                    {
                        const NullResult internalResult(portraitResult);
                        INVOKE_IF_NOT_NULL(callback, internalResult);
                    }
                };

                GetAvatarPortraitAsset(portraitAvatarAssetCollection, portraitAvatarAssetCallback);
            }
        }
        else
        {
            const NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        }
    };

    auto* userSystem = SystemsManager::Get().GetUserSystem();
    const auto& userId = userSystem->GetLoginState().UserId;
    GetAvatarPortraitAssetCollection(userId, portraitAvatarAssetCollCallback);
}

void SettingsSystem::SetAvatarInfo(AvatarType inType, const String& inIdentifier, bool inAvatarVisible, NullResultCallback callback)
{
    rapidjson::Document json;
    json.SetObject();
    json.AddMember("type", static_cast<int>(inType), json.GetAllocator());
    json.AddMember(
        "identifier", rapidjson::Value(inIdentifier.c_str(), static_cast<rapidjson::SizeType>(inIdentifier.Length())), json.GetAllocator());
    json.AddMember("avatarVisible", inAvatarVisible, json.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);

    SetSettingValue("UserSettings", "AvatarInfo", buffer.GetString(), callback);
}

void SettingsSystem::GetAvatarInfo(AvatarInfoResultCallback callback)
{
    if (!callback)
    {
        CSP_LOG_ERROR_MSG("SettingsSystem::GetAvatarInfo(): Callback must not my empty!");

        return;
    }

    StringResultCallback getSettingCallback = [=](const StringResult& result)
    {
        AvatarInfoResult internalResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_ERROR_MSG("Failed to retrieve avatar info.");

            callback(internalResult);
            return;
        }

        const auto& value = result.GetValue();

        if (value.IsEmpty())
        {
            CSP_LOG_MSG(csp::common::LogLevel::Verbose, "Whilst avatar info was successfully returned, no avatar settings data was found.");
            callback(internalResult);
            return;
        }

        rapidjson::Document json;
        rapidjson::ParseResult ok = json.Parse(value.c_str());
        if (!ok)
        {
            CSP_LOG_ERROR_FORMAT("Failed to parse avatar info JSON data. Error: %s, Offset: %zu", rapidjson::GetParseError_En(ok.Code()), ok.Offset());
            callback(internalResult);
            return;
        }

        // Avatar type
        if (json.HasMember("type") && json["type"].IsInt())
        {
            internalResult.SetAvatarType(static_cast<AvatarType>(json["type"].GetInt()));
        }
        else
        {
            CSP_LOG_ERROR_MSG("Whilst avatar info was successfully returned with data, no avatar type was included.");
        }

        // Avatar identifier type (used to identify or locate the actual avatar asset)
        if (json.HasMember("identifier"))
        {
            if (json.HasMember("identifierType") && static_cast<VariantType>(json["identifierType"].GetInt()) == VariantType::Integer)
            {
                // Integer type is no longer supported -- convert to string
                internalResult.SetAvatarIdentifier(std::to_string(json["identifier"].GetInt()).c_str());
            }
            else if (!json.HasMember("identifierType") || (static_cast<VariantType>(json["identifierType"].GetInt()) == VariantType::String))
            {
                internalResult.SetAvatarIdentifier(json["identifier"].GetString());
            }
            else
            {
                CSP_LOG_ERROR_MSG("Invalid avatar identifier type!");
            }
        }
        else
        {
            // No identifier field was present in the json
            CSP_LOG_ERROR_MSG("Whilst avatar info was successfully returned with data, no avatar identifier was included.");
        }

        // Avatar visibility
        if (json.HasMember("avatarVisible") && json["avatarVisible"].IsBool())
        {
            internalResult.SetAvatarVisible(json["avatarVisible"].GetBool());
        }

        callback(internalResult);
    };

    GetSettingValue("UserSettings", "AvatarInfo", getSettingCallback);
}

} // namespace csp::systems
