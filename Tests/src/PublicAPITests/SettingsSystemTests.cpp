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

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Settings/SettingsSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

bool RequestPredicateWithProgress(const csp::systems::ResultBase& Result)
{
    if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        PrintProgress(Result.GetRequestProgress());

        return false;
    }

    return true;
}

bool IsUriValid(const std::string& Uri, const std::string& FileName)
{
    // check that Uri starts with something valid
    if (Uri.find("https://world-streaming.magnopus-dev.cloud/", 0) != 0)
    {
        return false;
    }

    // check that the correct filename is present in the Uri
    const auto PosLastSlash = Uri.rfind('/');
    const auto UriFileName = Uri.substr(PosLastSlash + 1, FileName.size());

    return (FileName == UriFileName);
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_NDASTATUS_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, NDAStatusTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    auto [SetNDATrue] = AWAIT(SettingsSystem, SetNDAStatus, true);
    EXPECT_EQ(SetNDATrue.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetNDAResult] = AWAIT(SettingsSystem, GetNDAStatus);
    EXPECT_EQ(GetNDAResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetNDAResult.GetValue(), true);

    auto [SetNDAFalse] = AWAIT(SettingsSystem, SetNDAStatus, false);
    EXPECT_EQ(SetNDAFalse.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_NEWSLETTERSTATUS_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, NewsletterStatusTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    auto [SetNewsletterTrue] = AWAIT(SettingsSystem, SetNewsletterStatus, true);
    EXPECT_EQ(SetNewsletterTrue.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetNewsletterResult] = AWAIT(SettingsSystem, GetNewsletterStatus);
    EXPECT_EQ(GetNewsletterResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetNewsletterResult.GetValue(), true);

    auto [SetNewsletterFalse] = AWAIT(SettingsSystem, SetNewsletterStatus, false);
    EXPECT_EQ(SetNewsletterFalse.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_RECENTSPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, RecentSpacesTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    auto [SetRecentSpaces] = AWAIT(SettingsSystem, AddRecentlyVisitedSpace, "RecentSpace");
    EXPECT_EQ(SetRecentSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetRecentSpaces] = AWAIT(SettingsSystem, GetRecentlyVisitedSpaces);
    EXPECT_EQ(GetRecentSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    auto ReturnedStringArray = GetRecentSpaces.GetValue();
    EXPECT_EQ(ReturnedStringArray.Size(), 1);
    EXPECT_EQ(ReturnedStringArray[0].c_str(), std::string("RecentSpace"));

    auto [ClearRecentSpaces] = AWAIT(SettingsSystem, ClearRecentlyVisitedSpaces);
    EXPECT_EQ(ClearRecentSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_BLOCKEDSPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, BlockedSpacesTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Clear at start in case another test left something in the list
    auto [PreClearBlockedSpaces] = AWAIT(SettingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(PreClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    auto [SetBlockedSpaces] = AWAIT(SettingsSystem, AddBlockedSpace, "BlockedSpace");
    EXPECT_EQ(SetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetBlockedSpaces] = AWAIT(SettingsSystem, GetBlockedSpaces);
    EXPECT_EQ(GetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    auto ReturnedStringArray = GetBlockedSpaces.GetValue();
    EXPECT_EQ(ReturnedStringArray.Size(), 1);
    EXPECT_EQ(ReturnedStringArray[0].c_str(), std::string("BlockedSpace"));

    auto [ClearBlockedSpaces] = AWAIT(SettingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(ClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_REMOVEBLOCKEDSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, RemoveBlockedSpaceTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);
    // Clear at start in case another test left something in the list
    {
        auto [Result] = AWAIT(SettingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    csp::common::String BlockedSpace1Name = "BlockedSpace1";
    csp::common::String BlockedSpace2Name = "BlockedSpace2";

    // check block spaces is empty
    {
        auto [Result] = AWAIT(SettingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& BlockedSpaces = Result.GetValue();

        EXPECT_EQ(BlockedSpaces.Size(), 0);
    }

    // Add 1 blocked spaces
    {

        auto [Result2] = AWAIT(SettingsSystem, AddBlockedSpace, BlockedSpace2Name);

        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(SettingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& BlockedSpaces = Result.GetValue();

        EXPECT_EQ(BlockedSpaces.Size(), 1);
    }

    // Remove 1 blocked space
    {
        auto [Result] = AWAIT(SettingsSystem, RemoveBlockedSpace, BlockedSpace2Name);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(SettingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& BlockedSpaces = Result.GetValue();

        EXPECT_EQ(BlockedSpaces.Size(), 0);
    }

    // Clear all blocked spaces
    {
        auto [Result] = AWAIT(SettingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Clear at start in case another test left something in the list
    {
        auto [Result] = AWAIT(SettingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Add 2 blocked spaces
    {
        auto [Result1] = AWAIT(SettingsSystem, AddBlockedSpace, BlockedSpace1Name);

        EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

        auto [Result2] = AWAIT(SettingsSystem, AddBlockedSpace, BlockedSpace2Name);

        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(SettingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& BlockedSpaces = Result.GetValue();

        EXPECT_EQ(BlockedSpaces.Size(), 2);
    }

    // Remove 1 blocked space
    {
        auto [Result] = AWAIT(SettingsSystem, RemoveBlockedSpace, BlockedSpace2Name);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(SettingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& BlockedSpaces = Result.GetValue();

        EXPECT_EQ(BlockedSpaces.Size(), 1);
        EXPECT_EQ(BlockedSpaces[0], BlockedSpace1Name);
    }

    // Clear all blocked spaces
    {
        auto [Result] = AWAIT(SettingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_MULIBLOCKEDSPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, MultiBlockedSpacesTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Clear at start in case another test left something in the list
    auto [PreClearBlockedSpaces] = AWAIT(SettingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(PreClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    constexpr int NUM_BLOCKED_SPACES = 10;

    for (int SpaceIndex = 0; SpaceIndex < NUM_BLOCKED_SPACES; ++SpaceIndex)
    {
        char BlockedSpaceName[256];
        SPRINTF(BlockedSpaceName, "BlockSpace%d", SpaceIndex);

        auto [SetBlockedSpaces] = AWAIT(SettingsSystem, AddBlockedSpace, BlockedSpaceName);
        EXPECT_EQ(SetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    }

    auto [GetBlockedSpaces] = AWAIT(SettingsSystem, GetBlockedSpaces);
    EXPECT_EQ(GetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    auto ReturnedStringArray = GetBlockedSpaces.GetValue();

    EXPECT_EQ(ReturnedStringArray.Size(), NUM_BLOCKED_SPACES);

    for (int SpaceIndex = 0; SpaceIndex < NUM_BLOCKED_SPACES; ++SpaceIndex)
    {
        char BlockedSpaceName[256];
        SPRINTF(BlockedSpaceName, "BlockSpace%d", SpaceIndex);

        // Note that spaces come back in reverse order
        EXPECT_EQ(ReturnedStringArray[NUM_BLOCKED_SPACES - SpaceIndex - 1].c_str(), std::string(BlockedSpaceName));
    }

    auto [ClearBlockedSpaces] = AWAIT(SettingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(ClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_AVATARPORTRAIT_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, UpdateAvatarPortraitTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    {
        csp::systems::FileAssetDataSource AvatarPortrait;
        const std::string LocalFileName = "OKO.png";
        const auto FilePath = std::filesystem::absolute("assets/" + LocalFileName);
        AvatarPortrait.FilePath = FilePath.u8string().c_str();
        AvatarPortrait.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(SettingsSystem, UpdateAvatarPortrait, RequestPredicate, AvatarPortrait);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [GetAvatarPortraitResult] = AWAIT_PRE(SettingsSystem, GetAvatarPortrait, RequestPredicate, UserId);
        EXPECT_EQ(GetAvatarPortraitResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(GetAvatarPortraitResult.GetUri().c_str(), LocalFileName));
    }

    {
        csp::systems::FileAssetDataSource AvatarPortrait;
        const std::string LocalFileName = "OKO.png";
        const auto FilePath = std::filesystem::absolute("assets/" + LocalFileName);
        AvatarPortrait.FilePath = FilePath.u8string().c_str();
        AvatarPortrait.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(SettingsSystem, UpdateAvatarPortrait, RequestPredicate, AvatarPortrait);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [GetAvatarPortraitResult] = AWAIT_PRE(SettingsSystem, GetAvatarPortrait, RequestPredicate, UserId);
        EXPECT_EQ(GetAvatarPortraitResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(GetAvatarPortraitResult.GetUri().c_str(), LocalFileName));
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SPACESYSTEM_UPDATEAVATAR_PORTRAIT_WITH_BUFFER_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, UpdateAvatarPortraitWithBufferTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
    FILE* UploadFile = fopen(UploadFilePath.string().c_str(), "rb");
    uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
    auto* UploadFileData = new unsigned char[UploadFileSize];
    fread(UploadFileData, UploadFileSize, 1, UploadFile);
    fclose(UploadFile);

    csp::systems::BufferAssetDataSource AvatarPortraitThumbnail;
    AvatarPortraitThumbnail.Buffer = UploadFileData;
    AvatarPortraitThumbnail.BufferLength = UploadFileSize;

    AvatarPortraitThumbnail.SetMimeType("image/png");

    auto [Result] = AWAIT_PRE(SettingsSystem, UpdateAvatarPortraitWithBuffer, RequestPredicate, AvatarPortraitThumbnail);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    // get asset uri
    auto [GetAvatarPortraitResult] = AWAIT_PRE(SettingsSystem, GetAvatarPortrait, RequestPredicate, UserId);
    csp::systems::Asset Asset;
    Asset.FileName = "OKO.png";
    Asset.Uri = GetAvatarPortraitResult.GetUri().c_str();
    printf("Downloading asset data...\n");
    // Get data
    auto [Download_Result] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicateWithProgress, Asset);

    EXPECT_EQ(Download_Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t DownloadedAssetDataSize = Download_Result.GetDataLength();
    auto DownloadedAssetData = new uint8_t[DownloadedAssetDataSize];
    memcpy(DownloadedAssetData, Download_Result.GetData(), DownloadedAssetDataSize);

    EXPECT_EQ(DownloadedAssetDataSize, UploadFileSize);
    EXPECT_EQ(memcmp(DownloadedAssetData, UploadFileData, UploadFileSize), 0);

    delete[] UploadFileData;
    delete[] DownloadedAssetData;

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_AVATARINFOSTRING_TEST
CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, AvatarInfoTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    auto Type = csp::systems::AvatarType::Custom;
    csp::common::String Identifier = "https://notarealweb.site/my_cool_avatar.glb";

    // Set Avatar info
    {
        auto [Result] = AWAIT(SettingsSystem, SetAvatarInfo, Type, Identifier);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get Avatar info
    {
        auto [Result] = AWAIT(SettingsSystem, GetAvatarInfo);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAvatarType(), Type);
    }

    // Log out
    LogOut(UserSystem);
}
#endif