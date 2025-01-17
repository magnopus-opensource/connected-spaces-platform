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
#include <sstream>

constexpr int MAX_RECENT_SPACES = 50;
constexpr const char* AVATAR_PORTRAIT_ASSET_NAME = "AVATAR_PORTRAIT_ASSET_";
constexpr const char* AVATAR_PORTRAIT_ASSET_COLLECTION_NAME = "AVATAR_PORTRAIT_ASSET_COLLECTION_";

using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace csp::systems
{

SettingsSystem::SettingsSystem()
    : SystemBase(nullptr, nullptr)
    , SettingsAPI(nullptr)
{
}

SettingsSystem::SettingsSystem(web::WebClient* InWebClient)
    : SystemBase(InWebClient, nullptr)
{
    SettingsAPI = CSP_NEW chs::SettingsApi(InWebClient);
}

SettingsSystem::~SettingsSystem() { CSP_DELETE(SettingsAPI); }

void SettingsSystem::SetSettingValue(const String& InContext, const String& InKey, const String& InValue, NullResultCallback Callback) const
{
    auto& SystemsManager = SystemsManager::Get();
    const auto* UserSystem = SystemsManager.GetUserSystem();

    const auto& UserId = UserSystem->GetLoginState().UserId;

    auto InSettings = std::make_shared<chs::SettingsDto>();
    std::map<String, String> NewSettings;
    NewSettings.clear();
    NewSettings.insert(std::make_pair(InKey, InValue));
    InSettings->SetSettings(NewSettings);

    SettingsResultCallback InternalCallback = [InKey, Callback](const SettingsCollectionResult& Result)
    {
        if (Result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    services::ResponseHandlerPtr SettingsResponseHandler
        = SettingsAPI->CreateHandler<SettingsResultCallback, SettingsCollectionResult, void, chs::SettingsDto>(
            InternalCallback, nullptr, web::EResponseCodes::ResponseOK);

    static_cast<chs::SettingsApi*>(SettingsAPI)->apiV1UsersUserIdSettingsContextPut(UserId, InContext, InSettings, SettingsResponseHandler);
}

void SettingsSystem::GetSettingValue(const String& InContext, const String& InKey, StringResultCallback Callback) const
{
    auto& SystemsManager = SystemsManager::Get();
    const auto* UserSystem = SystemsManager.GetUserSystem();
    std::vector<String> MyKey = { InKey };

    const auto& UserId = UserSystem->GetLoginState().UserId;

    SettingsResultCallback InternalCallback = [InKey, Callback](const SettingsCollectionResult& Result)
    {
        StringResult StringResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == EResultCode::Success)
        {
            // Only attempt to read settings if result is valid

            auto& Settings = Result.GetSettingsCollection().Settings;

            if (Settings.HasKey(InKey))
            {
                StringResult.SetValue(Settings[InKey]);
            }
            else
            {
                StringResult.SetValue("");
            }
        }
        else
        {
            // Handle failure state
            StringResult.SetValue("");
        }

        INVOKE_IF_NOT_NULL(Callback, StringResult);
    };

    services::ResponseHandlerPtr SettingsResponseHandler
        = SettingsAPI->CreateHandler<SettingsResultCallback, SettingsCollectionResult, void, chs::SettingsDto>(
            InternalCallback, nullptr, web::EResponseCodes::ResponseOK);

    static_cast<chs::SettingsApi*>(SettingsAPI)->apiV1UsersUserIdSettingsContextGet(UserId, InContext, MyKey, SettingsResponseHandler);
}

void SettingsSystem::SetNDAStatus(bool InValue, NullResultCallback Callback)
{
    const String NDAStatus = InValue ? "true" : "false";

    SetSettingValue("UserSettings", "NDAStatus", NDAStatus, Callback);
}

void SettingsSystem::GetNDAStatus(BooleanResultCallback Callback)
{
    StringResultCallback GetSettingCallback = [=](const StringResult& Result)
    {
        BooleanResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
        InternalResult.SetValue(Result.GetValue() == "true");
        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetSettingValue("UserSettings", "NDAStatus", GetSettingCallback);
}

void SettingsSystem::SetNewsletterStatus(bool InValue, NullResultCallback Callback)
{
    const String NewsletterStatus = InValue ? "true" : "false";

    SetSettingValue("UserSettings", "Newsletter", NewsletterStatus, Callback);
}

void SettingsSystem::GetNewsletterStatus(BooleanResultCallback Callback)
{
    StringResultCallback GetSettingCallback = [=](const StringResult& Result)
    {
        BooleanResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
        InternalResult.SetValue(Result.GetValue() == "true");

        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetSettingValue("UserSettings", "Newsletter", GetSettingCallback);
}

void SettingsSystem::AddRecentlyVisitedSpace(const String InSpaceID, NullResultCallback Callback)
{
    StringArrayResultCallback GetRecentSpacesCallback = [=](StringArrayResult Result)
    {
        if (Result.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& RecentSpacesArray = Result.GetValue();

        auto RecentSpaces = RecentSpacesArray.ToList();
        RecentSpaces.Insert(0, InSpaceID);

        // Remove duplicate entry
        for (size_t i = RecentSpaces.Size() - 1; i > 0; --i)
        {
            if (RecentSpaces[i] == InSpaceID)
            {
                RecentSpaces.Remove(i);

                // Only a single duplicate entry (if any) should be found
                break;
            }
        }

        // Remove last entry if max number of spaces reached
        if (RecentSpaces.Size() > MAX_RECENT_SPACES)
        {
            RecentSpaces.Remove(MAX_RECENT_SPACES);
        }

        auto RecentSpacesString = String::Join(RecentSpaces, ',');

        SetSettingValue("UserSettings", "RecentSpaces", RecentSpacesString, Callback);
    };

    GetRecentlyVisitedSpaces(GetRecentSpacesCallback);
}

void SettingsSystem::GetRecentlyVisitedSpaces(StringArrayResultCallback Callback)
{
    StringResultCallback GetSettingCallback = [=](const StringResult& Result)
    {
        StringArrayResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == EResultCode::Success)
        {
            auto& RecentSpacesString = Result.GetValue();

            if (RecentSpacesString.IsEmpty())
            {
                InternalResult.SetValue({});
            }
            else
            {
                auto RecentSpacesList = RecentSpacesString.Split(',');
                auto RecentSpaces = RecentSpacesList.ToArray();
                InternalResult.SetValue(RecentSpaces);
            }
        }

        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetSettingValue("UserSettings", "RecentSpaces", GetSettingCallback);
}

void SettingsSystem::ClearRecentlyVisitedSpaces(NullResultCallback Callback) { SetSettingValue("UserSettings", "RecentSpaces", "", Callback); }

void SettingsSystem::AddBlockedSpace(const String InSpaceID, NullResultCallback Callback)
{
    StringArrayResultCallback GetBlockedSpacesCallback = [=](StringArrayResult Result)
    {
        if (Result.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& BlockedSpacesArray = Result.GetValue();

        // Ignore if space already blocked
        for (int i = 0; i < BlockedSpacesArray.Size(); ++i)
        {
            if (BlockedSpacesArray[i] == InSpaceID)
            {
                NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }
        }

        auto BlockedSpaces = BlockedSpacesArray.ToList();
        BlockedSpaces.Insert(0, InSpaceID);

        auto BlockedSpacesString = String::Join(BlockedSpaces, ',');

        SetSettingValue("UserSettings", "BlockedSpaces", BlockedSpacesString, Callback);
    };

    GetBlockedSpaces(GetBlockedSpacesCallback);
}

void SettingsSystem::RemoveBlockedSpace(const String InSpaceID, NullResultCallback Callback)
{
    StringArrayResultCallback GetBlockedSpacesCallback = [=](StringArrayResult Result)
    {
        if (Result.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& BlockedSpacesArray = Result.GetValue();
        auto FoundSpace = false;

        // Ignore if space not blocked
        for (int i = 0; i < BlockedSpacesArray.Size(); ++i)
        {
            if (BlockedSpacesArray[i] == InSpaceID)
            {
                FoundSpace = true;

                break;
            }
        }

        if (!FoundSpace)
        {
            NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        auto BlockedSpaces = BlockedSpacesArray.ToList();
        BlockedSpaces.RemoveItem(InSpaceID);

        auto BlockedSpacesString = String::Join(BlockedSpaces, ',');

        SetSettingValue("UserSettings", "BlockedSpaces", BlockedSpacesString, Callback);
    };

    GetBlockedSpaces(GetBlockedSpacesCallback);
}

void SettingsSystem::GetBlockedSpaces(StringArrayResultCallback Callback)
{
    StringResultCallback GetSettingCallback = [=](const StringResult& Result)
    {
        StringArrayResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == EResultCode::Success)
        {
            const auto& BlockedSpacesString = Result.GetValue();

            if (BlockedSpacesString.IsEmpty())
            {
                InternalResult.SetValue({});
            }
            else
            {
                auto BlockedSpacesList = BlockedSpacesString.Split(',');
                auto BlockedSpaces = BlockedSpacesList.ToArray();
                InternalResult.SetValue(BlockedSpaces);
            }
        }

        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetSettingValue("UserSettings", "BlockedSpaces", GetSettingCallback);
}

void SettingsSystem::ClearBlockedSpaces(NullResultCallback Callback) { SetSettingValue("UserSettings", "BlockedSpaces", "", Callback); }

void SettingsSystem::UpdateAvatarPortrait(const FileAssetDataSource& NewAvatarPortrait, NullResultCallback Callback)
{
    AssetCollectionsResultCallback AvatarPortraitAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& AssetCollections = AssetCollResult.GetAssetCollections();

            if (AssetCollections.IsEmpty())
            {
                // space without a thumbnail
                AddAvatarPortrait(NewAvatarPortrait, Callback);
            }
            else
            {
                const auto& AvatarPortraitAssetCollection = AssetCollections[0];

                AssetsResultCallback AvatarPortraitAssetCallback = [=](const AssetsResult& PortraitResult)
                {
                    if (PortraitResult.GetResultCode() == EResultCode::Success)
                    {
                        UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
                        {
                            if (UploadResult.GetResultCode() == EResultCode::Failed)
                            {
                                CSP_LOG_FORMAT(LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                                    static_cast<int>(UploadResult.GetResultCode()), UploadResult.GetHttpResultCode());
                            }

                            const NullResult InternalResult(UploadResult);
                            INVOKE_IF_NOT_NULL(Callback, InternalResult);
                        };

                        auto& AvatarPortraitAsset = ((AssetsResult&)PortraitResult).GetAssets()[0];
                        AvatarPortraitAsset.MimeType = NewAvatarPortrait.GetMimeType();
                        const auto AssetSystem = SystemsManager::Get().GetAssetSystem();

                        AssetSystem->UploadAssetData(AvatarPortraitAssetCollection, AvatarPortraitAsset, NewAvatarPortrait, UploadCallback);
                    }
                    else
                    {
                        NullResult InternalResult(PortraitResult);
                        INVOKE_IF_NOT_NULL(Callback, InternalResult);
                    }
                };

                GetAvatarPortraitAsset(AvatarPortraitAssetCollection, AvatarPortraitAssetCallback);
            }
        }
        else
        {
            const NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    auto* UserSystem = SystemsManager::Get().GetUserSystem();
    const auto& UserId = UserSystem->GetLoginState().UserId;
    GetAvatarPortraitAssetCollection(UserId, AvatarPortraitAssetCollCallback);
}

void SettingsSystem::GetAvatarPortrait(const csp::common::String InUserID, UriResultCallback Callback)
{
    AssetCollectionsResultCallback AvatarPortraitAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& AssetCollections = AssetCollResult.GetAssetCollections();

            if (AssetCollections.IsEmpty())
            {
                // User Doesn't have a Avatar Portrait
                const UriResult InternalResult(EResultCode::Success, static_cast<uint16_t>(web::EResponseCodes::ResponseNotFound));
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            }
            else
            {
                const auto& AvatarPortraitAssetCollection = AssetCollections[0];

                AssetsResultCallback AvatarPortraitAssetCallback = [=](const AssetsResult& AssetsResult)
                {
                    if (AssetsResult.GetResultCode() == EResultCode::Success)
                    {
                        const auto& Assets = AssetsResult.GetAssets();

                        if (Assets.Size() > 0)
                        {
                            const UriResult InternalResult(AssetsResult.GetAssets()[0].Uri);
                            INVOKE_IF_NOT_NULL(Callback, InternalResult);
                        }
                        else
                        {
                            UriResult InternalResult(EResultCode::Failed, 200);
                            InternalResult.SetResponseBody(
                                "Invalid avatar portrait AssetCollection. AssetCollection should contain an Asset but does not!");
                            InternalResult.Uri = "";

                            INVOKE_IF_NOT_NULL(Callback, InternalResult);
                        }
                    }
                    else
                    {
                        const UriResult InternalResult(AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());
                        INVOKE_IF_NOT_NULL(Callback, InternalResult);
                    }
                };

                GetAvatarPortraitAsset(AvatarPortraitAssetCollection, AvatarPortraitAssetCallback);
            }
        }
        else
        {
            const UriResult InternalResult(AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    GetAvatarPortraitAssetCollection(InUserID, AvatarPortraitAssetCollCallback);
}

void SettingsSystem::UpdateAvatarPortraitWithBuffer(const BufferAssetDataSource& NewAvatarPortrait, NullResultCallback Callback)
{
    AssetCollectionsResultCallback ThumbnailAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& AssetCollections = AssetCollResult.GetAssetCollections();

            if (AssetCollections.IsEmpty())
            {
                // space without a thumbnail
                AddAvatarPortraitWithBuffer(NewAvatarPortrait, Callback);
            }
            else
            {
                const auto& ThumbnailAssetCollection = AssetCollections[0];

                AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& ThumbnailResult)
                {
                    if (ThumbnailResult.GetResultCode() == EResultCode::Success)
                    {
                        UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
                        {
                            if (UploadResult.GetResultCode() == EResultCode::Failed)
                            {
                                CSP_LOG_FORMAT(LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                                    (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                            }

                            const NullResult InternalResult(UploadResult);
                            INVOKE_IF_NOT_NULL(Callback, InternalResult);
                        };

                        auto& ThumbnailAsset = ((AssetsResult&)ThumbnailResult).GetAssets()[0];
                        ThumbnailAsset.FileName = SpaceSystemHelpers::GetUniqueAvatarThumbnailAssetName(
                            SpaceSystemHelpers::GetAssetFileExtension(NewAvatarPortrait.GetMimeType()));
                        ThumbnailAsset.MimeType = NewAvatarPortrait.GetMimeType();
                        const auto AssetSystem = SystemsManager::Get().GetAssetSystem();
                        AssetSystem->UploadAssetData(ThumbnailAssetCollection, ThumbnailAsset, NewAvatarPortrait, UploadCallback);
                    }
                    else
                    {
                        NullResult InternalResult(ThumbnailResult);
                        INVOKE_IF_NOT_NULL(Callback, InternalResult);
                    }
                };

                GetAvatarPortraitAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
            }
        }
        else
        {
            const NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    auto* UserSystem = SystemsManager::Get().GetUserSystem();
    const auto& UserId = UserSystem->GetLoginState().UserId;
    GetAvatarPortraitAssetCollection(UserId, ThumbnailAssetCollCallback);
}

void SettingsSystem::AddAvatarPortrait(const FileAssetDataSource& ImageDataSource, NullResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* UserSystem = SystemsManager.GetUserSystem();

    const auto& UserId = UserSystem->GetLoginState().UserId;

    AssetCollectionResultCallback CreateAssetCollCallback = [=](const AssetCollectionResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& PortraitAssetColl = AssetCollResult.GetAssetCollection();

            AssetResultCallback CreateAssetCallback = [=](const AssetResult& CreateAssetResult)
            {
                if (CreateAssetResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                if (CreateAssetResult.GetResultCode() == EResultCode::Success)
                {
                    UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
                    {
                        if (UploadResult.GetResultCode() == EResultCode::Failed)
                        {
                            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait upload data has failed. ResCode: %d, HttpResCode: %d",
                                (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                        }

                        const NullResult InternalResult(UploadResult);
                        INVOKE_IF_NOT_NULL(Callback, InternalResult);
                    };

                    AssetSystem->UploadAssetData(PortraitAssetColl, CreateAssetResult.GetAsset(), ImageDataSource, UploadCallback);
                }
                else
                {
                    CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset creation was not successful. ResCode: %d, HttpResCode: %d",
                        (int)CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode());

                    const NullResult InternalResult(CreateAssetResult);
                    INVOKE_IF_NOT_NULL(Callback, InternalResult);
                }
            };

            const auto UniqueAssetName = AVATAR_PORTRAIT_ASSET_NAME + UserId;
            AssetSystem->CreateAsset(PortraitAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
        }
        else
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());

            const NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    const auto AvatarPortraitAssetCollectionName = AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + UserId;

    AssetSystem->CreateAssetCollection(
        nullptr, nullptr, AvatarPortraitAssetCollectionName, nullptr, EAssetCollectionType::DEFAULT, nullptr, CreateAssetCollCallback);
}

void SettingsSystem::AddAvatarPortraitWithBuffer(const BufferAssetDataSource& ImageDataSource, NullResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* UserSystem = SystemsManager.GetUserSystem();

    const auto& UserId = UserSystem->GetLoginState().UserId;

    AssetCollectionResultCallback CreateAssetCollCallback = [=](const AssetCollectionResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& PortraitAvatarAssetColl = AssetCollResult.GetAssetCollection();

            AssetResultCallback CreateAssetCallback = [=](const AssetResult& CreateAssetResult)
            {
                if (CreateAssetResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                if (CreateAssetResult.GetResultCode() == EResultCode::Success)
                {
                    UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
                    {
                        if (UploadResult.GetResultCode() == EResultCode::Failed)
                        {
                            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait upload data has failed. ResCode: %d, HttpResCode: %d",
                                (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                        }

                        const NullResult InternalResult(UploadResult);
                        INVOKE_IF_NOT_NULL(Callback, InternalResult);
                    };

                    Asset AvatarPortraitAsset = CreateAssetResult.GetAsset();

                    AvatarPortraitAsset.FileName
                        = AVATAR_PORTRAIT_ASSET_NAME + UserId + SpaceSystemHelpers::GetAssetFileExtension(ImageDataSource.GetMimeType());
                    AvatarPortraitAsset.MimeType = ImageDataSource.GetMimeType();
                    AssetSystem->UploadAssetData(PortraitAvatarAssetColl, AvatarPortraitAsset, ImageDataSource, UploadCallback);
                }
                else
                {
                    CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset creation was not successful. ResCode: %d, HttpResCode: %d",
                        (int)CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode());

                    const NullResult InternalResult(CreateAssetResult);
                    INVOKE_IF_NOT_NULL(Callback, InternalResult);
                }
            };

            const auto UniqueAssetName = AVATAR_PORTRAIT_ASSET_NAME + UserId;
            AssetSystem->CreateAsset(PortraitAvatarAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
        }
        else
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());

            const NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    const String SpaceThumbnailAssetCollectionName = AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + UserId;

    // don't associate this asset collection with a particular space so that it can be retrieved by guest users that have not joined this space
    AssetSystem->CreateAssetCollection(
        nullptr, nullptr, SpaceThumbnailAssetCollectionName, nullptr, EAssetCollectionType::DEFAULT, nullptr, CreateAssetCollCallback);
}

void SettingsSystem::GetAvatarPortraitAssetCollection(const csp::common::String InUserID, AssetCollectionsResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    AssetCollectionsResultCallback GetAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());
        }

        INVOKE_IF_NOT_NULL(Callback, AssetCollResult);
    };

    Array<String> AvatarPortraitAssetCollectionName = { AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + InUserID };

    Array<String> PrototypeNames = { AvatarPortraitAssetCollectionName };
    Array<EAssetCollectionType> PrototypeTypes = { EAssetCollectionType::DEFAULT };

    AssetSystem->FindAssetCollections(nullptr, nullptr, PrototypeNames, PrototypeTypes, nullptr, nullptr, nullptr, nullptr, GetAssetCollCallback);
}

void SettingsSystem::GetAvatarPortraitAsset(const AssetCollection& AvatarPortraitAssetCollection, AssetsResultCallback Callback)
{
    AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& AssetsResult)
    {
        if (AssetsResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The Avatar Portrait asset retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());
        }

        INVOKE_IF_NOT_NULL(Callback, AssetsResult);
    };

    const auto AssetSystem = SystemsManager::Get().GetAssetSystem();
    AssetSystem->GetAssetsInCollection(AvatarPortraitAssetCollection, ThumbnailAssetCallback);
}

void SettingsSystem::RemoveAvatarPortrait(NullResultCallback Callback)
{
    const auto AssetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionsResultCallback PortraitAvatarAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Success)
        {
            const auto& AssetCollections = AssetCollResult.GetAssetCollections();

            if (AssetCollections.IsEmpty())
            {
                // user doesnt have a avatar portrait
                const NullResult InternalResult(AssetCollResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            }
            else
            {
                const auto& PortraitAvatarAssetCollection = AssetCollections[0];

                AssetsResultCallback PortraitAvatarAssetCallback = [=](const AssetsResult& PortraitResult)
                {
                    if (PortraitResult.GetResultCode() == EResultCode::InProgress)
                    {
                        return;
                    }

                    if (PortraitResult.GetResultCode() == EResultCode::Success)
                    {
                        NullResultCallback DeleteAssetCallback = [=](const NullResult& DeleteAssetResult)
                        {
                            if (DeleteAssetResult.GetResultCode() == EResultCode::InProgress)
                            {
                                return;
                            }

                            if (DeleteAssetResult.GetResultCode() == EResultCode::Success)
                            {
                                NullResultCallback DeleteAssetCollCallback = [=](const NullResult& DeleteAssetCollResult)
                                {
                                    if (DeleteAssetCollResult.GetResultCode() == EResultCode::Failed)
                                    {
                                        CSP_LOG_FORMAT(LogLevel::Error,
                                            "The Portrait Avatar asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
                                            (int)DeleteAssetResult.GetResultCode(), DeleteAssetResult.GetHttpResultCode());
                                    }

                                    NullResult InternalResult(DeleteAssetCollResult);
                                    INVOKE_IF_NOT_NULL(Callback, DeleteAssetCollResult);
                                };

                                AssetSystem->DeleteAssetCollection(PortraitAvatarAssetCollection, DeleteAssetCollCallback);
                            }
                            else
                            {
                                CSP_LOG_FORMAT(LogLevel::Error, "The Portrait Avatar asset deletion was not successful. ResCode: %d, HttpResCode: %d",
                                    (int)DeleteAssetResult.GetResultCode(), DeleteAssetResult.GetHttpResultCode());

                                NullResult InternalResult(DeleteAssetResult);
                                INVOKE_IF_NOT_NULL(Callback, DeleteAssetResult);
                            }
                        };

                        const auto& PortraitAvatarAsset = PortraitResult.GetAssets()[0];
                        AssetSystem->DeleteAsset(PortraitAvatarAssetCollection, PortraitAvatarAsset, DeleteAssetCallback);
                    }
                    else
                    {
                        const NullResult InternalResult(PortraitResult);
                        INVOKE_IF_NOT_NULL(Callback, InternalResult);
                    }
                };

                GetAvatarPortraitAsset(PortraitAvatarAssetCollection, PortraitAvatarAssetCallback);
            }
        }
        else
        {
            const NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        }
    };

    auto* UserSystem = SystemsManager::Get().GetUserSystem();
    const auto& UserId = UserSystem->GetLoginState().UserId;
    GetAvatarPortraitAssetCollection(UserId, PortraitAvatarAssetCollCallback);
}

void SettingsSystem::SetAvatarInfo(AvatarType InType, const String& InIdentifier, NullResultCallback Callback)
{
    rapidjson::Document Json;
    Json.SetObject();
    Json.AddMember("type", static_cast<int>(InType), Json.GetAllocator());
    Json.AddMember("identifier", rapidjson::Value(InIdentifier.c_str(), InIdentifier.Length()), Json.GetAllocator());
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    Json.Accept(Writer);

    SetSettingValue("UserSettings", "AvatarInfo", Buffer.GetString(), Callback);
}

void SettingsSystem::GetAvatarInfo(AvatarInfoResultCallback Callback)
{
    if (!Callback)
    {
        CSP_LOG_ERROR_MSG("SettingsSystem::GetAvatarInfo(): Callback must not my empty!");

        return;
    }

    StringResultCallback GetSettingCallback = [=](const StringResult& Result)
    {
        AvatarInfoResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_ERROR_MSG("Failed to retrieve avatar info.");

            Callback(InternalResult);

            return;
        }

        const auto& Value = Result.GetValue();

        if (Value.IsEmpty())
        {
            Callback(InternalResult);

            return;
        }

        rapidjson::Document Json;
        Json.Parse(Value.c_str());

        if (!Json.HasMember("type") || !Json.HasMember("identifier"))
        {
            CSP_LOG_ERROR_MSG("Invalid avatar info!");

            Callback(InternalResult);

            return;
        }

        const auto& type = Json["type"];

        if (!type.IsInt())
        {
            CSP_LOG_ERROR_MSG("Invalid avatar info!");

            Callback(InternalResult);

            return;
        }

        InternalResult.SetAvatarType(static_cast<AvatarType>(type.GetInt()));

        if (Json.HasMember("identifierType") && static_cast<VariantType>(Json["identifierType"].GetInt()) == VariantType::Integer)
        {
            // Integer type is no longer supported -- convert to string
            InternalResult.SetAvatarIdentifier(std::to_string(Json["identifier"].GetInt()).c_str());
            Callback(InternalResult);
        }
        else if (!Json.HasMember("identifierType") || (static_cast<VariantType>(Json["identifierType"].GetInt()) == VariantType::String))
        {
            InternalResult.SetAvatarIdentifier(Json["identifier"].GetString());
            Callback(InternalResult);
        }
        else
        {
            CSP_LOG_ERROR_MSG("Invalid identifier type!");

            Callback(InternalResult);

            return;
        }
    };

    GetSettingValue("UserSettings", "AvatarInfo", GetSettingCallback);
}

} // namespace csp::systems
