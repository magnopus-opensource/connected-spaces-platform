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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

bool RequestPredicateWithProgress(const csp::systems::ResultBase& result)
{
    if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        PrintProgress(result.GetRequestProgress());

        return false;
    }

    return true;
}

bool IsUriValid(const std::string& uri, const std::string& fileName)
{
    // check that the correct filename is present in the Uri
    const auto posLastSlash = uri.rfind('/');
    const auto uriFileName = uri.substr(posLastSlash + 1, fileName.size());

    return (fileName == uriFileName);
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, NDAStatusTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    auto [SetNDATrue] = AWAIT(settingsSystem, SetNDAStatus, true);
    EXPECT_EQ(SetNDATrue.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetNDAResult] = AWAIT(settingsSystem, GetNDAStatus);
    EXPECT_EQ(GetNDAResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetNDAResult.GetValue(), true);

    auto [SetNDAFalse] = AWAIT(settingsSystem, SetNDAStatus, false);
    EXPECT_EQ(SetNDAFalse.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, NewsletterStatusTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    auto [SetNewsletterTrue] = AWAIT(settingsSystem, SetNewsletterStatus, true);
    EXPECT_EQ(SetNewsletterTrue.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetNewsletterResult] = AWAIT(settingsSystem, GetNewsletterStatus);
    EXPECT_EQ(GetNewsletterResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetNewsletterResult.GetValue(), true);

    auto [SetNewsletterFalse] = AWAIT(settingsSystem, SetNewsletterStatus, false);
    EXPECT_EQ(SetNewsletterFalse.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, RecentSpacesTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    auto [SetRecentSpaces] = AWAIT(settingsSystem, AddRecentlyVisitedSpace, "RecentSpace");
    EXPECT_EQ(SetRecentSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetRecentSpaces] = AWAIT(settingsSystem, GetRecentlyVisitedSpaces);
    EXPECT_EQ(GetRecentSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    auto returnedStringArray = GetRecentSpaces.GetValue();
    EXPECT_EQ(returnedStringArray.Size(), 1);
    EXPECT_EQ(returnedStringArray[0].c_str(), std::string("RecentSpace"));

    auto [ClearRecentSpaces] = AWAIT(settingsSystem, ClearRecentlyVisitedSpaces);
    EXPECT_EQ(ClearRecentSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, BlockedSpacesTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Clear at start in case another test left something in the list
    auto [PreClearBlockedSpaces] = AWAIT(settingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(PreClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    auto [SetBlockedSpaces] = AWAIT(settingsSystem, AddBlockedSpace, "BlockedSpace");
    EXPECT_EQ(SetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetBlockedSpaces] = AWAIT(settingsSystem, GetBlockedSpaces);
    EXPECT_EQ(GetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    auto returnedStringArray = GetBlockedSpaces.GetValue();
    EXPECT_EQ(returnedStringArray.Size(), 1);
    EXPECT_EQ(returnedStringArray[0].c_str(), std::string("BlockedSpace"));

    auto [ClearBlockedSpaces] = AWAIT(settingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(ClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, RemoveBlockedSpaceTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);
    // Clear at start in case another test left something in the list
    {
        auto [Result] = AWAIT(settingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    csp::common::String blockedSpace1Name = "BlockedSpace1";
    csp::common::String blockedSpace2Name = "BlockedSpace2";

    // check block spaces is empty
    {
        auto [Result] = AWAIT(settingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& blockedSpaces = Result.GetValue();

        EXPECT_EQ(blockedSpaces.Size(), 0);
    }

    // Add 1 blocked spaces
    {

        auto [Result2] = AWAIT(settingsSystem, AddBlockedSpace, blockedSpace2Name);

        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(settingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& blockedSpaces = Result.GetValue();

        EXPECT_EQ(blockedSpaces.Size(), 1);
    }

    // Remove 1 blocked space
    {
        auto [Result] = AWAIT(settingsSystem, RemoveBlockedSpace, blockedSpace2Name);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(settingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& blockedSpaces = Result.GetValue();

        EXPECT_EQ(blockedSpaces.Size(), 0);
    }

    // Clear all blocked spaces
    {
        auto [Result] = AWAIT(settingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Clear at start in case another test left something in the list
    {
        auto [Result] = AWAIT(settingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Add 2 blocked spaces
    {
        auto [Result1] = AWAIT(settingsSystem, AddBlockedSpace, blockedSpace1Name);

        EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

        auto [Result2] = AWAIT(settingsSystem, AddBlockedSpace, blockedSpace2Name);

        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(settingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& blockedSpaces = Result.GetValue();

        EXPECT_EQ(blockedSpaces.Size(), 2);
    }

    // Remove 1 blocked space
    {
        auto [Result] = AWAIT(settingsSystem, RemoveBlockedSpace, blockedSpace2Name);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get blocked spaces
    {
        auto [Result] = AWAIT(settingsSystem, GetBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto& blockedSpaces = Result.GetValue();

        EXPECT_EQ(blockedSpaces.Size(), 1);
        EXPECT_EQ(blockedSpaces[0], blockedSpace1Name);
    }

    // Clear all blocked spaces
    {
        auto [Result] = AWAIT(settingsSystem, ClearBlockedSpaces);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, MultiBlockedSpacesTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Clear at start in case another test left something in the list
    auto [PreClearBlockedSpaces] = AWAIT(settingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(PreClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    constexpr int numBlockedSpaces = 10;

    for (int spaceIndex = 0; spaceIndex < numBlockedSpaces; ++spaceIndex)
    {
        char blockedSpaceName[256];
        SPRINTF(blockedSpaceName, "BlockSpace%d", spaceIndex);

        auto [SetBlockedSpaces] = AWAIT(settingsSystem, AddBlockedSpace, blockedSpaceName);
        EXPECT_EQ(SetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    }

    auto [GetBlockedSpaces] = AWAIT(settingsSystem, GetBlockedSpaces);
    EXPECT_EQ(GetBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);
    auto returnedStringArray = GetBlockedSpaces.GetValue();

    EXPECT_EQ(returnedStringArray.Size(), numBlockedSpaces);

    for (int spaceIndex = 0; spaceIndex < numBlockedSpaces; ++spaceIndex)
    {
        char blockedSpaceName[256];
        SPRINTF(blockedSpaceName, "BlockSpace%d", spaceIndex);

        // Note that spaces come back in reverse order
        EXPECT_EQ(returnedStringArray[numBlockedSpaces - spaceIndex - 1].c_str(), std::string(blockedSpaceName));
    }

    auto [ClearBlockedSpaces] = AWAIT(settingsSystem, ClearBlockedSpaces);
    EXPECT_EQ(ClearBlockedSpaces.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, UpdateAvatarPortraitTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    {
        csp::systems::FileAssetDataSource avatarPortrait;
        const std::string localFileName = "OKO.png";
        const auto filePath = std::filesystem::absolute("assets/" + localFileName);
        avatarPortrait.FilePath = filePath.u8string().c_str();
        avatarPortrait.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(settingsSystem, UpdateAvatarPortrait, RequestPredicate, avatarPortrait);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [GetAvatarPortraitResult] = AWAIT_PRE(settingsSystem, GetAvatarPortrait, RequestPredicate, userId);
        EXPECT_EQ(GetAvatarPortraitResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(GetAvatarPortraitResult.GetUri().c_str(), localFileName));
    }

    {
        csp::systems::FileAssetDataSource avatarPortrait;
        const std::string localFileName = "OKO.png";
        const auto filePath = std::filesystem::absolute("assets/" + localFileName);
        avatarPortrait.FilePath = filePath.u8string().c_str();
        avatarPortrait.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(settingsSystem, UpdateAvatarPortrait, RequestPredicate, avatarPortrait);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [GetAvatarPortraitResult] = AWAIT_PRE(settingsSystem, GetAvatarPortrait, RequestPredicate, userId);
        EXPECT_EQ(GetAvatarPortraitResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(GetAvatarPortraitResult.GetUri().c_str(), localFileName));
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, UpdateAvatarPortraitWithBufferTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);

    csp::systems::BufferAssetDataSource avatarPortraitThumbnail;
    avatarPortraitThumbnail.Buffer = uploadFileData->data();
    avatarPortraitThumbnail.BufferLength = uploadFileData->size();

    avatarPortraitThumbnail.SetMimeType("image/png");

    auto [Result] = AWAIT_PRE(settingsSystem, UpdateAvatarPortraitWithBuffer, RequestPredicate, avatarPortraitThumbnail);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    // get asset uri
    auto [GetAvatarPortraitResult] = AWAIT_PRE(settingsSystem, GetAvatarPortrait, RequestPredicate, userId);
    csp::systems::Asset asset;
    asset.FileName = "OKO.png";
    asset.Uri = GetAvatarPortraitResult.GetUri().c_str();
    printf("Downloading asset data...\n");
    // Get data
    auto [Download_Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, asset);

    EXPECT_EQ(Download_Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t downloadedAssetDataSize = Download_Result.GetDataLength();
    auto downloadedAssetData = new uint8_t[downloadedAssetDataSize];
    memcpy(downloadedAssetData, Download_Result.GetData(), downloadedAssetDataSize);

    EXPECT_EQ(downloadedAssetDataSize, uploadFileData->size());
    EXPECT_EQ(memcmp(downloadedAssetData, uploadFileData->data(), uploadFileData->size()), 0);

    delete[] downloadedAssetData;

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SettingsSystemTests, DefaultAvatarInfoTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Get default avatar info (without the user ever having set it)
    {
        auto [Result] = AWAIT(settingsSystem, GetAvatarInfo);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAvatarType(), csp::systems::AvatarType::None);
        EXPECT_EQ(Result.GetAvatarVisible(), true);
    }

    // Log out
    LogOut(userSystem);
}

namespace CSPEngine
{

/*
 * For convenience, we test various permutations of setting avatar info with a matrix-based approach.
 * In all cases, we're assume the response is a success. The tests are concerned with validating that the data
 * we set is what we get back.
 */

class SetAvatarInfo : public PublicTestBaseWithParam<std::tuple<csp::systems::AvatarType, csp::common::String, bool>>
{
};

TEST_P(SetAvatarInfo, SetAvatarInfoTest)
{
    const csp::systems::AvatarType avatarType = std::get<0>(GetParam());
    const csp::common::String avatarIdentifier = std::get<1>(GetParam());
    const bool avatarVisible = std::get<2>(GetParam());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* settingsSystem = systemsManager.GetSettingsSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Set Avatar info
    {
        auto [Result] = AWAIT(settingsSystem, SetAvatarInfo, avatarType, avatarIdentifier, avatarVisible);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Get Avatar info
    {
        auto [Result] = AWAIT(settingsSystem, GetAvatarInfo);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAvatarType(), avatarType);
        EXPECT_EQ(Result.GetAvatarIdentifier(), avatarIdentifier);
        EXPECT_EQ(Result.GetAvatarVisible(), avatarVisible);
    }

    // Log out
    LogOut(userSystem);
}

INSTANTIATE_TEST_SUITE_P(SettingsSystemTests, SetAvatarInfo,
    testing::Values(std::make_tuple(csp::systems::AvatarType::Custom, "https://notarealweb.site/my_avatar.glb", true),
        std::make_tuple(csp::systems::AvatarType::Custom, "https://notarealweb.site/my_avatar.glb", false),
        std::make_tuple(csp::systems::AvatarType::Premade, "1", false), std::make_tuple(csp::systems::AvatarType::None, "", true)));

} // namespace CSPEngine
