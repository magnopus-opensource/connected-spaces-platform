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
#include "CSP/Common/Array.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/MultiplayerHubMethods.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest-param-test.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <future>
#include <tuple>

using namespace csp::common;
using namespace csp::systems;

// TODO: Clean up these tests

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

bool RequestPredicateWithProgress(const csp::systems::ResultBase& result)
{
    if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        PrintProgress(result.GetRequestProgress());

        return false;
    }

    return true;
}

void CreateSpace(::SpaceSystem* spaceSystem, const String& name, const String& description, SpaceAttributes attributes,
    const Optional<Map<String, String>>& metadata, const Optional<InviteUserRoleInfoCollection>& inviteUsers,
    const Optional<FileAssetDataSource>& thumbnail, const Optional<Array<String>>& tags, Space& outSpace)
{
    Map<String, String> testMetadata = metadata.HasValue() ? (*metadata) : Map<String, String>({ { "site", "Void" } });

    // TODO: Add tests for public spaces
    auto [Result] = AWAIT_PRE(spaceSystem, CreateSpace, RequestPredicate, name, description, attributes, inviteUsers, testMetadata, thumbnail, tags);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outSpace = Result.GetSpace();
}

void CreateSpaceWithBuffer(::SpaceSystem* spaceSystem, const String& name, const String& description, SpaceAttributes attributes,
    const Optional<Map<String, String>>& metadata, const Optional<InviteUserRoleInfoCollection>& inviteUsers, BufferAssetDataSource& thumbnail,
    const Optional<Array<String>>& tags, Space& outSpace)
{
    Map<String, String> testMetadata = metadata.HasValue() ? (*metadata) : Map<String, String>({ { "site", "Void" } });

    auto [Result]
        = AWAIT_PRE(spaceSystem, CreateSpaceWithBuffer, RequestPredicate, name, description, attributes, inviteUsers, testMetadata, thumbnail, tags);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outSpace = Result.GetSpace();
}

void CreateDefaultTestSpace(SpaceSystem* spaceSystem, Space& outSpace, csp::systems::SpaceAttributes attributes)
{
    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, attributes, nullptr, nullptr, nullptr, nullptr, outSpace);
}

void GetSpace(::SpaceSystem* spaceSystem, const String& spaceId, Space& outSpace)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetSpace, RequestPredicate, spaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outSpace = Result.GetSpace();
}

Array<BasicSpace> GetSpacesByAttributes(::SpaceSystem* spaceSystem, const Optional<bool>& isDiscoverable, const Optional<bool>& isArchived,
    const Optional<bool>& requiresInvite, const Optional<int>& resultsSkipNo, const Optional<int>& resultsMaxNo)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetSpacesByAttributes, RequestPredicate, isDiscoverable, isArchived, requiresInvite, resultsSkipNo,
        resultsMaxNo, nullptr, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto spacesTotalCount = Result.GetTotalCount();
    const auto spaces = Result.GetSpaces();

    if (spaces.Size() > 0)
    {
        EXPECT_GT(spacesTotalCount, 0);
    }

    return spaces;
}

Array<Space> GetSpacesByIds(::SpaceSystem* spaceSystem, const Array<String>& spaceIDs)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetSpacesByIds, RequestPredicate, spaceIDs);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetSpaces();
}

void UpdateSpace(::SpaceSystem* spaceSystem, const String& spaceId, const Optional<String>& newName, const Optional<String>& newDescription,
    const Optional<SpaceAttributes>& newAttributes, const Optional<Array<String>>& newTags, BasicSpace& outSpace)
{
    auto [Result] = AWAIT_PRE(spaceSystem, UpdateSpace, RequestPredicate, spaceId, newName, newDescription, newAttributes, newTags);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outSpace = Result.GetSpace();
}

void AddSiteInfo(::SpaceSystem* spaceSystem, const char* name, const String& spaceId, Site& outSite)
{
    const char* siteName = (name) ? name : "CSP-UNITTEST-SITE-NAME";

    GeoLocation siteLocation(175.0, 85.0);
    OlyRotation siteRotation(200.0, 200.0, 200.0, 200.0);

    Site siteInfo;
    siteInfo.Name = siteName;
    siteInfo.Location = siteLocation;
    siteInfo.Rotation = siteRotation;

    auto [Result] = AWAIT_PRE(spaceSystem, AddSiteInfo, RequestPredicate, spaceId, siteInfo);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outSite = Result.GetSite();
    std::cerr << "Site Created: Name=" << outSite.Name << " Id=" << outSite.Id << std::endl;
}

void DeleteSpace(::SpaceSystem* spaceSystem, const String& spaceId)
{
    auto [Result] = AWAIT_PRE(spaceSystem, DeleteSpace, RequestPredicate, spaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void RemoveSiteInfo(::SpaceSystem* spaceSystem, const String& spaceId, ::Site& site)
{
    auto [Result] = AWAIT_PRE(spaceSystem, RemoveSiteInfo, RequestPredicate, spaceId, site);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    std::cerr << "Site Deleted: Name=" << site.Name << " Id=" << site.Id << std::endl;
}

void GetSpaceSites(::SpaceSystem* spaceSystem, const String& spaceId, Array<Site>& outSites)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetSitesInfo, RequestPredicate, spaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& resultSites = Result.GetSites();
    outSites = Array<Site>(resultSites.Size());

    for (size_t idx = 0; idx < resultSites.Size(); ++idx)
    {
        outSites[idx] = resultSites[idx];
    }
}

void UpdateUserRole(::SpaceSystem* spaceSystem, const String& spaceId, UserRoleInfo& newUserRoleInfo)
{
    auto [Result] = AWAIT_PRE(spaceSystem, UpdateUserRole, RequestPredicate, spaceId, newUserRoleInfo);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cerr << "The user role for UserId: " << newUserRoleInfo.UserId << " has been updated successfully" << std::endl;
    }
}

void GetRoleForSpecificUser(::SpaceSystem* spaceSystem, const String& spaceId, const String& userId, UserRoleInfo& outUserRoleInfo)
{
    Array<String> ids = { userId };
    auto [Result] = AWAIT_PRE(spaceSystem, GetUsersRoles, RequestPredicate, spaceId, ids);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& returnedRolesInfo = Result.GetUsersRoles();

    EXPECT_EQ(returnedRolesInfo.Size(), 1);

    outUserRoleInfo = returnedRolesInfo[0];
}

void GetUsersRoles(::SpaceSystem* spaceSystem, const String& spaceId, const Array<String>& requestedUserIds, Array<UserRoleInfo>& outUsersRoles)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetUsersRoles, RequestPredicate, spaceId, requestedUserIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& returnedRolesInfo = Result.GetUsersRoles();
    outUsersRoles = Array<UserRoleInfo>(returnedRolesInfo.Size());

    for (size_t idx = 0; idx < returnedRolesInfo.Size(); ++idx)
    {
        outUsersRoles[idx] = returnedRolesInfo[idx];
    }
}

void UpdateAndAssertSpaceMetadata(::SpaceSystem* spaceSystem, const String& spaceId, const Optional<Map<String, String>>& newMetadata)
{
    Map<String, String> metadata = newMetadata.HasValue() ? *newMetadata : Map<String, String>();

    auto [Result] = AWAIT_PRE(spaceSystem, UpdateSpaceMetadata, RequestPredicate, spaceId, metadata);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    std::cerr << "Space metadata has been updated successfully" << std::endl;
}

Map<String, String> GetAndAssertSpaceMetadata(::SpaceSystem* spaceSystem, const String& spaceId)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceMetadata, RequestPredicate, spaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetMetadata();
}

Map<String, Map<String, String>> GetAndAssertSpacesMetadata(::SpaceSystem* spaceSystem, const Array<String>& spaceIds)
{
    auto [Result] = AWAIT_PRE(spaceSystem, GetSpacesMetadata, RequestPredicate, spaceIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetMetadata();
}

bool IsUriValid(const std::string& uri, const std::string& fileName)
{
    // check that the correct filename is present in the Uri
    const auto posLastSlash = uri.rfind('/');
    const auto uriFileName = uri.substr(posLastSlash + 1, fileName.size());

    return (fileName == uriFileName);
}

InviteUserRoleInfoCollection CreateInviteUsers()
{
    // Create normal users
    const auto testUser1Email = String("testnopus.pokemon+1@magnopus.com");
    const auto testUser2Email = String("testnopus.pokemon+2@magnopus.com");

    InviteUserRoleInfo inviteUser1;
    inviteUser1.UserEmail = testUser1Email;
    inviteUser1.UserRole = SpaceUserRole::User;

    InviteUserRoleInfo inviteUser2;
    inviteUser2.UserEmail = testUser2Email;
    inviteUser2.UserRole = SpaceUserRole::User;

    // Create moderator users
    const auto testModInviteUser1Email = String("testnopus.pokemon+mod1@magnopus.com");
    const auto testModInviteUser2Email = String("testnopus.pokemon+mod2@magnopus.com");

    InviteUserRoleInfo modInviteUser1;
    modInviteUser1.UserEmail = testModInviteUser1Email;
    modInviteUser1.UserRole = SpaceUserRole::Moderator;

    InviteUserRoleInfo modInviteUser2;
    modInviteUser2.UserEmail = testModInviteUser2Email;
    modInviteUser2.UserRole = SpaceUserRole::Moderator;

    InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser1, inviteUser2, modInviteUser1, modInviteUser2 };
    inviteUsers.EmailLinkUrl = "https://dev.magnoverse.space";
    inviteUsers.SignupUrl = "https://dev.magnoverse.space";

    return inviteUsers;
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    FileAssetDataSource spaceThumbnail;
    const std::string localFileName = "OKO.png";
    const auto filePath = std::filesystem::absolute("assets/" + localFileName);
    spaceThumbnail.FilePath = filePath.u8string().c_str();
    spaceThumbnail.SetMimeType("image/png");

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, spaceThumbnail, nullptr, space);

    auto [GetThumbnailResult] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);

    EXPECT_EQ(GetThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(IsUriValid(GetThumbnailResult.GetUri().c_str(), localFileName));

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithInvalidThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    FileAssetDataSource spaceThumbnail;
    const std::string localFileName = "OKO.png";
    const auto filePath = std::filesystem::absolute("assets/badpath/" + localFileName);
    spaceThumbnail.FilePath = filePath.u8string().c_str();
    spaceThumbnail.SetMimeType("image/png");

    auto [Result] = AWAIT_PRE(spaceSystem, CreateSpace, RequestPredicate, testSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr,
        Map<String, String>({ { "site", "Void" } }), spaceThumbnail, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    bool spaceHasBeenDeleted = false;
    while (!spaceHasBeenDeleted)
    {
        // Validate that the in progress invalid space creation has been resolved and space has been removed.
        auto [SpacesResult] = AWAIT_PRE(spaceSystem, GetSpaces, RequestPredicate);
        spaceHasBeenDeleted = SpacesResult.GetSpaces().Size() == 0;

        // Wait to allow for the space deltion to be processed.
        //
        // This is required as shutting down will cause a memory access violation,
        // due to spawned process not being resolved while systems are being destroyed.
        std::this_thread::sleep_for(500ms);
    }

    EXPECT_EQ(spaceHasBeenDeleted, true);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithTagsTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    Array<String> tags = { "tag-test" };

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, tags, space);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBulkInviteTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    auto inviteUsers = CreateInviteUsers();

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteUsers, nullptr, nullptr, space);

    auto [GetInvitesResult] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& pendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(pendingInvites.Size(), inviteUsers.InviteUserRoleInfos.Size());

    for (size_t idx = 0; idx < pendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << pendingInvites[idx] << std::endl;
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithEmptyBulkInviteTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    InviteUserRoleInfoCollection inviteUsers;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteUsers, nullptr, nullptr, space);

    auto [GetInvitesResult] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& pendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(pendingInvites.Size(), inviteUsers.InviteUserRoleInfos.Size());

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBufferTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);

    BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData->data();
    bufferSource.BufferLength = uploadFileData->size();

    bufferSource.SetMimeType("image/png");

    // Create space
    ::Space space;
    CreateSpaceWithBuffer(
        spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, bufferSource, nullptr, space);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBufferWithThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    LogInAsNewTestUser(userSystem, userId);

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);
    
    BufferAssetDataSource spaceThumbnail;
    spaceThumbnail.Buffer = uploadFileData->data();
    spaceThumbnail.BufferLength = uploadFileData->size();

    spaceThumbnail.SetMimeType("image/png");

    ::Space space;
    CreateSpaceWithBuffer(
        spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, spaceThumbnail, nullptr, space);

    auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
    EXPECT_FALSE(Result.GetUri().IsEmpty());

    auto [GetThumbnailResult] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);
    EXPECT_EQ(GetThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);
    printf("Downloading asset data...\n");

    // Get asset uri
    auto [uri_Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);
    ::Asset asset;
    asset.FileName = "test.json";
    asset.Uri = uri_Result.GetUri();
    // Get data
    auto [Download_Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, asset);

    EXPECT_EQ(Download_Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t downloadedAssetDataSize = Download_Result.GetDataLength();
    auto downloadedAssetData = std::make_unique<uint8_t[]>(downloadedAssetDataSize);
    memcpy(downloadedAssetData.get(), Download_Result.GetData(), downloadedAssetDataSize);

    EXPECT_EQ(downloadedAssetDataSize, uploadFileData->size());
    EXPECT_EQ(memcmp(downloadedAssetData.get(), uploadFileData->data(), uploadFileData->size()), 0);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithInvalidBufferWithThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    auto inviteUsers = CreateInviteUsers();

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);
    
    BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData->data();
    bufferSource.BufferLength = uploadFileData->size();

    bufferSource.SetMimeType("image/json");

    auto [Result] = AWAIT_PRE(spaceSystem, CreateSpaceWithBuffer, RequestPredicate, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private,
        inviteUsers, Map<String, String>({ { "site", "Void" } }), bufferSource, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    // required as the GetSpaces will return out dated result before the request has been processed.
    std::this_thread::sleep_for(700ms);

    bool spaceHasBeenDeleted = false;
    while (!spaceHasBeenDeleted)
    {
        // Validate that the in progress invalid space creation has been resolved and space has been removed.
        auto [SpacesResult] = AWAIT_PRE(spaceSystem, GetSpaces, RequestPredicate);
        spaceHasBeenDeleted = SpacesResult.GetSpaces().Size() == 0;

        // Wait to allow for the space deltion to be processed.
        //
        // This is required as shutting down will cause a memory access violation,
        // due to spawned process not being resolved while systems are being destroyed.
        std::this_thread::sleep_for(500ms);
    }

    EXPECT_EQ(spaceHasBeenDeleted, true);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBufferWithBulkInviteTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    auto inviteUsers = CreateInviteUsers();

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);
    
    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);
    
    BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData->data();
    bufferSource.BufferLength = uploadFileData->size();

    bufferSource.SetMimeType("image/png");

    // Create space
    ::Space space;
    CreateSpaceWithBuffer(
        spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteUsers, bufferSource, nullptr, space);

    auto [GetInvitesResult] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& pendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(pendingInvites.Size(), inviteUsers.InviteUserRoleInfos.Size());

    for (size_t idx = 0; idx < pendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << pendingInvites[idx] << std::endl;
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBufferWithEmptyBulkInviteTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);

    BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData->data();
    bufferSource.BufferLength = uploadFileData->size();

    bufferSource.SetMimeType("image/png");

    // Create space
    ::Space space;
    InviteUserRoleInfoCollection inviteUsers;
    CreateSpaceWithBuffer(
        spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteUsers, bufferSource, nullptr, space);

    auto [GetInvitesResult] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& pendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(pendingInvites.Size(), inviteUsers.InviteUserRoleInfos.Size());

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceDescriptionTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Update space description
    char updatedDescription[256];
    SPRINTF(updatedDescription, "%s-Updated", testSpaceDescription);

    BasicSpace updatedBasicSpace;
    UpdateSpace(spaceSystem, space.Id, nullptr, updatedDescription, nullptr, nullptr, updatedBasicSpace);

    EXPECT_EQ(updatedBasicSpace.Name, space.Name);
    EXPECT_EQ(updatedBasicSpace.Description, updatedDescription);
    EXPECT_EQ(updatedBasicSpace.Attributes, space.Attributes);

    ::Space updatedSpace;
    GetSpace(spaceSystem, space.Id, updatedSpace);

    EXPECT_EQ(updatedSpace.Name, space.Name);
    EXPECT_EQ(updatedSpace.Description, updatedDescription);
    EXPECT_EQ(updatedSpace.Attributes, space.Attributes);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceTypeTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Update space type
    auto updatedAttributes = SpaceAttributes::Public;

    BasicSpace updatedBasicSpace;
    UpdateSpace(spaceSystem, space.Id, nullptr, nullptr, updatedAttributes, nullptr, updatedBasicSpace);

    EXPECT_EQ(updatedBasicSpace.Name, space.Name);
    EXPECT_EQ(updatedBasicSpace.Description, ""); // This should be empty because we elected to not give one when we invoked `UpdateSpace`.
    EXPECT_EQ(updatedBasicSpace.Attributes, updatedAttributes);

    ::Space updatedSpace;
    GetSpace(spaceSystem, space.Id, updatedSpace);

    EXPECT_EQ(updatedSpace.Name, space.Name);
    EXPECT_EQ(updatedSpace.Description,
        ""); // This should remain cleared since not specifying a description in `UpdateSpace` is equivalent to clearing it.
    EXPECT_EQ(updatedSpace.Attributes, updatedAttributes);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpacesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Get spaces
    auto [Result] = AWAIT_PRE(spaceSystem, GetSpaces, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto& resultSpaces = Result.GetSpaces();

    EXPECT_GT(resultSpaces.Size(), 0);

    bool spaceFound = false;

    for (size_t i = 0; i < resultSpaces.Size(); ++i)
    {
        if (resultSpaces[i].Name == uniqueSpaceName)
        {
            spaceFound = true;

            break;
        }
    }

    EXPECT_TRUE(spaceFound);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    ::Space resultSpace;
    GetSpace(spaceSystem, space.Id, resultSpace);

    EXPECT_EQ(resultSpace.Name, space.Name);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpacesByIdsTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniquePublicSpaceName[256];
    SPRINTF(uniquePublicSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniquePrivateSpaceName[256];
    SPRINTF(uniquePrivateSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    ::Space publicSpace;
    CreateSpace(spaceSystem, uniquePublicSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, publicSpace);

    ::Space privateSpace;
    CreateSpace(
        spaceSystem, uniquePrivateSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, privateSpace);

    Array<String> spacesIds = { publicSpace.Id, privateSpace.Id };

    Array<::Space> resultSpaces = GetSpacesByIds(spaceSystem, spacesIds);

    EXPECT_EQ(resultSpaces.Size(), spacesIds.Size());

    bool privateSpaceFound = false;
    bool publicSpaceFound = false;

    for (size_t i = 0; i < resultSpaces.Size(); ++i)
    {
        if (resultSpaces[i].Name == uniquePrivateSpaceName)
        {
            privateSpaceFound = true;
            continue;
        }
        else if (resultSpaces[i].Name == uniquePublicSpaceName)
        {
            publicSpaceFound = true;
            continue;
        }
    }

    EXPECT_TRUE(privateSpaceFound);
    EXPECT_TRUE(publicSpaceFound);

    DeleteSpace(spaceSystem, publicSpace.Id);
    DeleteSpace(spaceSystem, privateSpace.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPublicSpacesAsGuestTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    constexpr size_t spaceCount = 3;

    String userId;

    // Log in using default test account to create spaces
    csp::systems::Profile spaceCreatorUser = CreateTestUser();
    LogIn(userSystem, userId, spaceCreatorUser.Email, GeneratedTestAccountPassword);

    // Create test spaces
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    String spaceId[spaceCount];

    for (size_t i = 0; i < spaceCount; ++i)
    {
        char uniqueSpaceName[256];
        SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

        ::Space space;

        CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

        spaceId[i] = space.Id;
    }

    // Log out
    LogOut(userSystem);

    // Log in as guest
    LogInAsGuest(userSystem, userId);

    // Get public spaces
    Array<BasicSpace> resultSpaces = GetSpacesByAttributes(spaceSystem, true, false, false, 0, static_cast<int>(spaceCount));

    EXPECT_GE(resultSpaces.Size(), spaceCount);

    // Make sure that all returned spaces are public
    for (size_t i = 0; i < resultSpaces.Size(); ++i)
    {
        const auto& space = resultSpaces[i];

        EXPECT_TRUE((bool)(space.Attributes & SpaceAttributes::IsDiscoverable));
        EXPECT_FALSE((bool)(space.Attributes & SpaceAttributes::RequiresInvite));
    }

    // Log out as guest
    LogOut(userSystem);

    // Clean up
    LogIn(userSystem, userId, spaceCreatorUser.Email, GeneratedTestAccountPassword);

    for (size_t i = 0; i < spaceCount; ++i)
    {
        DeleteSpace(spaceSystem, spaceId[i]);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPublicSpacesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    constexpr size_t spaceCount = 3;

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create test spaces
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    String spaceId[spaceCount];

    for (size_t i = 0; i < spaceCount; ++i)
    {
        char uniqueSpaceName[256];
        SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

        ::Space space;

        CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

        spaceId[i] = space.Id;
    }

    // Get only the public spaces
    Array<BasicSpace> resultSpaces = GetSpacesByAttributes(spaceSystem, true, false, false, 0, static_cast<int>(spaceCount));

    EXPECT_GE(resultSpaces.Size(), spaceCount);

    // Make sure that all returned spaces are public
    for (size_t i = 0; i < resultSpaces.Size(); ++i)
    {
        const auto& space = resultSpaces[i];

        EXPECT_TRUE((bool)(space.Attributes & SpaceAttributes::IsDiscoverable));
        EXPECT_FALSE((bool)(space.Attributes & SpaceAttributes::RequiresInvite));
    }

    // Clean up
    for (size_t i = 0; i < spaceCount; ++i)
    {
        DeleteSpace(spaceSystem, spaceId[i]);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPrivateSpacesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    constexpr size_t spaceCount = 3;

    String userId;

    // Log in using default test account to create spaces
    LogInAsNewTestUser(userSystem, userId);

    // Create test spaces
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    String spaceId[spaceCount];

    for (size_t i = 0; i < spaceCount; ++i)
    {
        char uniqueSpaceName[256];
        SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

        ::Space space;

        CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

        spaceId[i] = space.Id;
    }

    // Get only the public spaces
    Array<BasicSpace> resultSpaces = GetSpacesByAttributes(spaceSystem, false, false, true, 0, static_cast<int>(spaceCount));

    EXPECT_GE(resultSpaces.Size(), spaceCount);

    // Make sure that all returned spaces are public
    for (size_t i = 0; i < resultSpaces.Size(); ++i)
    {
        const auto& space = resultSpaces[i];

        EXPECT_FALSE((bool)(space.Attributes & SpaceAttributes::IsDiscoverable));
        EXPECT_TRUE((bool)(space.Attributes & SpaceAttributes::RequiresInvite));
    }

    // Clean up
    for (size_t i = 0; i < spaceCount; ++i)
    {
        DeleteSpace(spaceSystem, spaceId[i]);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPaginatedPrivateSpacesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    constexpr size_t spaceCount = 6;

    String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create test spaces
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    String spaceId[spaceCount];

    for (size_t i = 0; i < spaceCount; ++i)
    {
        char uniqueSpaceName[256];
        SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

        ::Space space;

        CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

        spaceId[i] = space.Id;
    }

    // Get private spaces paginated
    {
        auto [Result] = AWAIT_PRE(spaceSystem, GetSpacesByAttributes, RequestPredicate, false, false, true, 0, static_cast<int>(spaceCount / 2),
            nullptr, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto spacesTotalCount = Result.GetTotalCount();
        const auto spaces = Result.GetSpaces();

        EXPECT_EQ(spaces.Size(), spaceCount / 2);
        EXPECT_GE(spacesTotalCount, spaceCount);
    }

    // Clean up
    for (size_t i = 0; i < spaceCount; ++i)
    {
        DeleteSpace(spaceSystem, spaceId[i]);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, JoinPublicSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Login as an admin user in order to be able to create the test space
    String spaceOwnerUserId;
    csp::systems::Profile spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);

    ::Space publicSpace;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, publicSpace);

    LogOut(userSystem);

    // Log in as a guest user
    String guestUserId;
    LogInAsGuest(userSystem, guestUserId);

    auto [AddUserResult] = AWAIT_PRE(spaceSystem, AddUserToSpace, RequestPredicate, publicSpace.Id, guestUserId);

    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    std::cerr << "User added to space" << std::endl;

    ::Space updatedPublicSpace;
    GetSpace(spaceSystem, publicSpace.Id, updatedPublicSpace);

    Array<::UserRoleInfo> retrievedUserRoles;
    GetUsersRoles(spaceSystem, updatedPublicSpace.Id, updatedPublicSpace.UserIds, retrievedUserRoles);

    EXPECT_EQ(retrievedUserRoles.Size(), 2);

    for (size_t idx = 0; idx < retrievedUserRoles.Size(); ++idx)
    {
        if (retrievedUserRoles[idx].UserId == spaceOwnerUserId)
        {
            EXPECT_EQ(retrievedUserRoles[idx].UserRole, SpaceUserRole::Owner);
        }
        else if (retrievedUserRoles[idx].UserId == guestUserId)
        {
            EXPECT_EQ(retrievedUserRoles[idx].UserRole, SpaceUserRole::User);
        }
        else
        {
            ASSERT_TRUE(false && "Encountered unexpected space user");
        }
    }

    // Log out
    LogOut(userSystem);

    // Login as an admin user in order to be able to delete the test space
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, publicSpace.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, AddSiteInfoTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    Site siteInfo;
    AddSiteInfo(spaceSystem, nullptr, space.Id, siteInfo);

    RemoveSiteInfo(spaceSystem, space.Id, siteInfo);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSiteInfoTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    Site siteInfo1, siteInfo2;
    AddSiteInfo(spaceSystem, "Site1", space.Id, siteInfo1);
    AddSiteInfo(spaceSystem, "Site2", space.Id, siteInfo2);

    Array<Site> spaceSites;
    GetSpaceSites(spaceSystem, space.Id, spaceSites);

    EXPECT_EQ(spaceSites.Size(), 2);

    bool site1Found = false;
    bool site2Found = false;

    for (size_t idx = 0; idx < spaceSites.Size(); ++idx)
    {
        if (spaceSites[idx].Name == siteInfo1.Name)
        {
            site1Found = true;
            continue;
        }
        else if (spaceSites[idx].Name == siteInfo2.Name)
        {
            site2Found = true;
            continue;
        }
    }

    EXPECT_TRUE(site1Found && site2Found);

    RemoveSiteInfo(spaceSystem, space.Id, siteInfo1);
    RemoveSiteInfo(spaceSystem, space.Id, siteInfo2);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateUserRolesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Get alt account user ID
    String altUserId;

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String defaultUserId;

    // Create default and alternative users
    csp::systems::Profile defaultUser = CreateTestUser();
    csp::systems::Profile alternativeUser = CreateTestUser();

    // Log in
    LogIn(userSystem, defaultUserId, defaultUser.Email, GeneratedTestAccountPassword);

    // Create test space
    InviteUserRoleInfo inviteUser;
    inviteUser.UserEmail = alternativeUser.Email;
    inviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser };
    inviteUsers.EmailLinkUrl = "dev.magnoverse.space";

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteUsers, nullptr, nullptr, space);

    // Log out
    LogOut(userSystem);

    // Log in using alt test account
    LogIn(userSystem, altUserId, alternativeUser.Email, GeneratedTestAccountPassword);

    // Ensure alt test account can join space
    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Log out and log in again using default test account
    LogOut(userSystem);
    LogIn(userSystem, defaultUserId, defaultUser.Email, GeneratedTestAccountPassword);

    // Update test account user roles for space
    GetSpace(spaceSystem, space.Id, space);

    ::UserRoleInfo updatedDefaultUserRole = { defaultUserId, SpaceUserRole::Moderator };
    ::UserRoleInfo updatedSecondTestUserRole = { altUserId, SpaceUserRole::Owner };

    // User Roles should not be changed after update as a owner cannot be modified
    // This also means a owner cannot be turned into a moderator
    auto [DefaultResult] = AWAIT_PRE(spaceSystem, UpdateUserRole, RequestPredicate, space.Id, updatedDefaultUserRole);

    // Update first account role should fail
    EXPECT_EQ(DefaultResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [SecondResult] = AWAIT_PRE(spaceSystem, UpdateUserRole, RequestPredicate, space.Id, updatedSecondTestUserRole);

    // Update second account role should fail
    EXPECT_EQ(SecondResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // Verify updated user roles
    Array<::UserRoleInfo> retrievedUserRoles;
    GetUsersRoles(spaceSystem, space.Id, space.UserIds, retrievedUserRoles);

    EXPECT_EQ(retrievedUserRoles.Size(), 2);

    for (size_t idx = 0; idx < retrievedUserRoles.Size(); ++idx)
    {
        if (retrievedUserRoles[idx].UserId == defaultUserId)
        {
            EXPECT_EQ(retrievedUserRoles[idx].UserRole, SpaceUserRole::Owner);
        }
        else if (retrievedUserRoles[idx].UserId == altUserId)
        {
            EXPECT_EQ(retrievedUserRoles[idx].UserRole, SpaceUserRole::User);
        }
        else
        {
            ASSERT_TRUE(false && "Encountered unexpected space user");
        }
    }

    GetSpace(spaceSystem, space.Id, space);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateGuestUserRoleTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Login as an admin user in order to be able to create the test space
    String spaceOwnerUserId;
    csp::systems::Profile adminUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, adminUser.Email, GeneratedTestAccountPassword);

    ::Space publicSpace;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, publicSpace);

    LogOut(userSystem);

    // Log in as a guest user
    String guestUserId;
    LogInAsGuest(userSystem, guestUserId);

    auto [AddUserResult] = AWAIT_PRE(spaceSystem, AddUserToSpace, RequestPredicate, publicSpace.Id, guestUserId);
    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(userSystem);

    // login as an admin user
    LogIn(userSystem, spaceOwnerUserId, adminUser.Email, GeneratedTestAccountPassword);

    ::UserRoleInfo updatedUserRoleInfo = { guestUserId, SpaceUserRole::Moderator };
    UpdateUserRole(spaceSystem, publicSpace.Id, updatedUserRoleInfo);

    ::UserRoleInfo retrievedUserRoles;
    GetRoleForSpecificUser(spaceSystem, publicSpace.Id, guestUserId, retrievedUserRoles);
    EXPECT_EQ(retrievedUserRoles.UserRole, SpaceUserRole::Moderator);

    DeleteSpace(spaceSystem, publicSpace.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, SetUserRoleOnInviteTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Get alt account user ID
    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);
    LogOut(userSystem);

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String defaultUserId;
    csp::systems::Profile defaultUser = CreateTestUser();

    // Log in
    LogIn(userSystem, defaultUserId, defaultUser.Email, GeneratedTestAccountPassword);

    // create a space with no other user Ids invited
    ::Space space;
    // CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Invite second test account as a Moderator Role user
    auto [Result] = AWAIT_PRE(spaceSystem, InviteToSpace, RequestPredicate, space.Id, altUser.Email, true, "", "");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    ::UserRoleInfo userRoleInfo;
    GetRoleForSpecificUser(spaceSystem, space.Id, altUserId, userRoleInfo);
    EXPECT_EQ(userRoleInfo.UserRole, SpaceUserRole::Moderator);

    // As the default test user has the "internal-service" global role he can delete the space no matter the space role it holds.
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    Map<String, String> testSpaceMetadata = { { "site", "Void" } };

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, testSpaceMetadata, nullptr, nullptr, nullptr, space);

    Map<String, String> retrievedSpaceMetadata = GetAndAssertSpaceMetadata(spaceSystem, space.Id);

    EXPECT_EQ(retrievedSpaceMetadata.Size(), testSpaceMetadata.Size());
    EXPECT_EQ(retrievedSpaceMetadata["site"], "Void");

    testSpaceMetadata["site"] = "MagOffice";

    UpdateAndAssertSpaceMetadata(spaceSystem, space.Id, testSpaceMetadata);

    retrievedSpaceMetadata = GetAndAssertSpaceMetadata(spaceSystem, space.Id);

    EXPECT_EQ(retrievedSpaceMetadata.Size(), testSpaceMetadata.Size());
    EXPECT_EQ(retrievedSpaceMetadata["site"], "MagOffice");

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpacesMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    Map<String, String> testSpaceMetadata = { { "site", "Void" } };

    ::Space space1, space2;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, testSpaceMetadata, nullptr, nullptr, nullptr, space1);
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, testSpaceMetadata, nullptr, nullptr, nullptr, space2);

    Map<String, Map<String, String>> retrievedSpaceMetadata = GetAndAssertSpacesMetadata(spaceSystem, { space1.Id, space2.Id });

    EXPECT_EQ(retrievedSpaceMetadata[space1.Id].Size(), testSpaceMetadata.Size());
    EXPECT_EQ(retrievedSpaceMetadata[space2.Id].Size(), testSpaceMetadata.Size());
    EXPECT_EQ(retrievedSpaceMetadata[space1.Id]["site"], "Void");
    EXPECT_EQ(retrievedSpaceMetadata[space2.Id]["site"], "Void");

    testSpaceMetadata["site"] = "MagOffice";

    // OB-3939 fix: passing tags as nullptr should leave them unchanged
    UpdateAndAssertSpaceMetadata(spaceSystem, space1.Id, testSpaceMetadata);
    UpdateAndAssertSpaceMetadata(spaceSystem, space2.Id, testSpaceMetadata);

    retrievedSpaceMetadata = GetAndAssertSpacesMetadata(spaceSystem, { space1.Id, space2.Id });

    EXPECT_EQ(retrievedSpaceMetadata[space1.Id].Size(), testSpaceMetadata.Size());
    EXPECT_EQ(retrievedSpaceMetadata[space2.Id].Size(), testSpaceMetadata.Size());
    EXPECT_EQ(retrievedSpaceMetadata[space1.Id]["site"], "MagOffice");
    EXPECT_EQ(retrievedSpaceMetadata[space2.Id]["site"], "MagOffice");

    DeleteSpace(spaceSystem, space1.Id);
    DeleteSpace(spaceSystem, space2.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceTagsMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    Map<String, String> testSpaceMetadata = { { "site", "Void" } };

    ::Space space1, space2;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, testSpaceMetadata, nullptr, nullptr, nullptr, space1);
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, testSpaceMetadata, nullptr, nullptr, nullptr, space2);

    Array<String> spaces = { space1.Id, space2.Id };
    Map<String, Map<String, String>> retrievedSpacesMetadata = GetAndAssertSpacesMetadata(spaceSystem, spaces);

    EXPECT_EQ(retrievedSpacesMetadata.Size(), 2);

    const auto& metadata1 = retrievedSpacesMetadata[space1.Id];

    EXPECT_EQ(metadata1.Size(), testSpaceMetadata.Size());
    EXPECT_EQ(metadata1["site"], "Void");

    const auto& metadata2 = retrievedSpacesMetadata[space2.Id];

    EXPECT_EQ(metadata2.Size(), testSpaceMetadata.Size());
    EXPECT_EQ(metadata2["site"], "Void");

    DeleteSpace(spaceSystem, spaces[0]);
    DeleteSpace(spaceSystem, spaces[1]);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpacesTagsMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Array<String> { "tag-test" }, space);

    EXPECT_EQ(space.Tags.Size(), 1);
    EXPECT_EQ(space.Tags[0], "tag-test");

    // Update tags
    ::Space updatedSpace;
    UpdateSpace(spaceSystem, space.Id, nullptr, nullptr, nullptr, Array<String> { "new-tag-test", "new-tag-test-2" }, updatedSpace);

    EXPECT_EQ(updatedSpace.Tags.Size(), 2);
    EXPECT_EQ(updatedSpace.Tags[0], "new-tag-test");
    EXPECT_EQ(updatedSpace.Tags[1], "new-tag-test-2");

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    LogInAsNewTestUser(userSystem, userId);

    // Create space without a thumbnail
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    {
        auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
        EXPECT_TRUE(Result.GetUri().IsEmpty());
    }

    { /// Bad file path test
        FileAssetDataSource spaceThumbnail;
        const std::string localFileName = "OKO.png";
        const auto filePath = std::filesystem::absolute("assets/badpath/" + localFileName);
        spaceThumbnail.FilePath = filePath.u8string().c_str();
        spaceThumbnail.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(spaceSystem, UpdateSpaceThumbnail, RequestPredicate, space.Id, spaceThumbnail);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        FileAssetDataSource spaceThumbnail;
        const std::string localFileName = "OKO.png";
        const auto filePath = std::filesystem::absolute("assets/" + localFileName);
        spaceThumbnail.FilePath = filePath.u8string().c_str();
        spaceThumbnail.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(spaceSystem, UpdateSpaceThumbnail, RequestPredicate, space.Id, spaceThumbnail);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [GetThumbnailResult] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);

        EXPECT_EQ(GetThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(GetThumbnailResult.GetUri().c_str(), localFileName));
    }

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceThumbnailWithBufferTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    LogInAsNewTestUser(userSystem, userId);

    // Create space without a thumbnail
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    {
        auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
        EXPECT_TRUE(Result.GetUri().IsEmpty());
    }

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_NE(uploadFileData, std::nullopt);

    BufferAssetDataSource spaceThumbnail;
    spaceThumbnail.Buffer = uploadFileData->data();
    spaceThumbnail.BufferLength = uploadFileData->size();

    spaceThumbnail.SetMimeType("image/png");

    auto [Result] = AWAIT_PRE(spaceSystem, UpdateSpaceThumbnailWithBuffer, RequestPredicate, space.Id, spaceThumbnail);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetThumbnailResult] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);
    EXPECT_EQ(GetThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);
    printf("Downloading asset data...\n");

    // Get asset uri
    auto [uri_Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);
    ::Asset asset;
    asset.FileName = "test.json";
    asset.Uri = uri_Result.GetUri();
    // Get data
    auto [Download_Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, asset);

    EXPECT_EQ(Download_Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t downloadedAssetDataSize = Download_Result.GetDataLength();
    auto downloadedAssetData = std::make_unique<uint8_t[]>(downloadedAssetDataSize);
    memcpy(downloadedAssetData.get(), Download_Result.GetData(), downloadedAssetDataSize);

    EXPECT_EQ(downloadedAssetDataSize, uploadFileData->size());
    EXPECT_EQ(memcmp(downloadedAssetData.get(), uploadFileData->data(), uploadFileData->size()), 0);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithEmptyMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    Map<String, String> metadata;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, metadata, nullptr, nullptr, nullptr, space);

    Map<String, String> retrievedSpaceMetadata = GetAndAssertSpaceMetadata(spaceSystem, space.Id);

    EXPECT_EQ(retrievedSpaceMetadata.Size(), 0UL);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceWithEmptyMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    UpdateAndAssertSpaceMetadata(spaceSystem, space.Id, nullptr);

    Map<String, String> retrievedSpaceMetadata = GetAndAssertSpaceMetadata(spaceSystem, space.Id);

    EXPECT_EQ(retrievedSpaceMetadata.Size(), 0UL);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

// - TODO - JQ - Rename this test to InviteUserToSpaceTest?
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPendingUserInvitesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // It is not possible to record pending invites and see them decrement as they are accepted,
    // because the invites are sent by email and have to be accepted by clicking a link in them.
    // The test suite does not have the capability to open emails and click links in them.
    // The workaround is to test each separately.
    // Using an email that is not associated to any existing account, only the pending invites counter increases (the accepted invites counter remains
    // at 0).

    // This test only works if the below email is not associated to any existing account.
    const char* testUserEmail = "non-existing.account@magnopus.com";
    const char* testEmailLinkUrl = "https://dev.magnoverse.space/";
    const char* testSignupUrl = "https://dev.magnoverse.space/";

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Check that there are no pending invites before inviting a user
    auto [GetInvitesResultBefore] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetInvitesResultBefore.GetResultCode(), csp::systems::EResultCode::Success);
    auto& pendingInvitesBefore = GetInvitesResultBefore.GetPendingInvitesEmails();
    EXPECT_EQ(pendingInvitesBefore.Size(), 0);

    // Invite a user to the space
    auto [Result] = AWAIT_PRE(spaceSystem, InviteToSpace, RequestPredicate, space.Id, testUserEmail, nullptr, testEmailLinkUrl, testSignupUrl);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Check that there is one pending invite after inviting a user
    auto [GetInvitesResult] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto& pendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(pendingInvites.Size(), 1);
    for (size_t idx = 0; idx < pendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << pendingInvites[idx] << std::endl;
    }

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetAcceptedUserInvitesTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    const char* testEmailLinkUrl = "https://dev.magnoverse.space/";
    const char* testSignupUrl = "https://dev.magnoverse.space/";

    // Create users
    String spaceCreatorUserId;
    csp::systems::Profile spaceCreatorUser = CreateTestUser();

    String user1Id;
    csp::systems::Profile user1 = CreateTestUser();
    csp::systems::Profile user2 = CreateTestUser();

    // It is not possible to record pending invites and see them decrement as they are accepted,
    // because the invites are sent by email and have to be accepted by clicking a link in them.
    // The test suite does not have the capability to open emails and click links in them.
    // The workaround is to test each separately.
    // Using an account that already exists, only the accepted invites counter increases (the pending invites counter remains at 0).
    // Note that all invites are accepted at once on the test tenant.

    // Log in as Space Creator and create space
    LogIn(userSystem, spaceCreatorUserId, spaceCreatorUser.Email, GeneratedTestAccountPassword);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Check that there are no accepted invites before inviting users
    auto [GetAcceptedInvitesResultBefore] = AWAIT_PRE(spaceSystem, GetAcceptedUserInvites, RequestPredicate, space.Id);
    EXPECT_EQ(GetAcceptedInvitesResultBefore.GetResultCode(), csp::systems::EResultCode::Success);
    auto& acceptedInvitesBefore = GetAcceptedInvitesResultBefore.GetAcceptedInvitesUserIds();
    EXPECT_EQ(acceptedInvitesBefore.Size(), 0);

    // Invite User1 and User2 to the space
    auto [ResultInviteUser1]
        = AWAIT_PRE(spaceSystem, InviteToSpace, RequestPredicate, space.Id, user1.Email, nullptr, testEmailLinkUrl, testSignupUrl);
    EXPECT_EQ(ResultInviteUser1.GetResultCode(), csp::systems::EResultCode::Success);
    auto [ResultInviteUser2]
        = AWAIT_PRE(spaceSystem, InviteToSpace, RequestPredicate, space.Id, user2.Email, nullptr, testEmailLinkUrl, testSignupUrl);
    EXPECT_EQ(ResultInviteUser2.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out as Space Creator
    LogOut(userSystem);

    // Log in as User1 and enter the space, which triggers invite acceptance on the test tenant (for all users, so including User2)
    LogIn(userSystem, user1Id, user1.Email, GeneratedTestAccountPassword);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterSpaceResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    ASSERT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Log back in as Space Creator to check the accepted invites
    LogOut(userSystem);
    LogIn(userSystem, spaceCreatorUserId, spaceCreatorUser.Email, GeneratedTestAccountPassword);

    // Check the accepted invites are recorded correctly
    auto [GetAcceptedInvitesResult] = AWAIT_PRE(spaceSystem, GetAcceptedUserInvites, RequestPredicate, space.Id);

    EXPECT_EQ(GetAcceptedInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto& acceptedInvites = GetAcceptedInvitesResult.GetAcceptedInvitesUserIds();
    EXPECT_EQ(acceptedInvites.Size(), 2);
    for (size_t idx = 0; idx < acceptedInvites.Size(); ++idx)
    {
        std::cerr << "Accepted space invite for user id: " << acceptedInvites[idx] << std::endl;
    }

    // Clean up
    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BulkInvitetoSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    auto inviteUsers = CreateInviteUsers();

    String userId;
    LogInAsNewTestUser(userSystem, userId);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [Result] = AWAIT_PRE(spaceSystem, BulkInviteToSpace, RequestPredicate, space.Id, inviteUsers);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetInvitesResult] = AWAIT_PRE(spaceSystem, GetPendingUserInvites, RequestPredicate, space.Id);

    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& pendingInvites = GetInvitesResult.GetPendingInvitesEmails();

    EXPECT_EQ(pendingInvites.Size(), 4);

    for (size_t idx = 0; idx < pendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << pendingInvites[idx] << std::endl;
    }

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPublicSpaceMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    Map<String, String> testSpaceMetadata = { { "site", "Void" } };

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Log in with default user
    csp::systems::Profile defaultUser = CreateTestUser();
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Create public space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, testSpaceMetadata, nullptr, nullptr, nullptr, space);

    // Log out with default user and in with alt user
    LogOut(userSystem);
    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    ASSERT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Get metadata for public space
    Map<String, String> retrievedMetadata = GetAndAssertSpaceMetadata(spaceSystem, space.Id);

    ASSERT_EQ(retrievedMetadata.Size(), testSpaceMetadata.Size());
    ASSERT_TRUE(retrievedMetadata.HasKey("site"));
    ASSERT_EQ(retrievedMetadata["site"], testSpaceMetadata["site"]);

    // Exit and re-enter space to verify its OK to always add self to public space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    {
        auto [Result2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult2] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Log back in with default user so space can be deleted
    LogOut(userSystem);
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String primaryUserId;

    csp::systems::Profile primaryTestUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryTestUser.Email, GeneratedTestAccountPassword);

    ::Space space;
    FileAssetDataSource spaceThumbnail;
    const std::string localFileName = "test.json";
    const auto filePath = std::filesystem::absolute("assets/" + localFileName);
    spaceThumbnail.FilePath = filePath.u8string().c_str();
    spaceThumbnail.SetMimeType("application/json");

    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, spaceThumbnail, nullptr, space);

    String initialSpaceThumbnailUri;
    {
        auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        initialSpaceThumbnailUri = Result.GetUri();

        EXPECT_TRUE(IsUriValid(initialSpaceThumbnailUri.c_str(), localFileName));
    }

    LogOut(userSystem);

    // check that a user that doesn't belong to the space can retrieve the thumbnail
    String secondaryUserId;
    csp::systems::Profile secondaryTestUser = CreateTestUser();

    LogIn(userSystem, secondaryUserId, secondaryTestUser.Email, GeneratedTestAccountPassword);

    {
        auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(initialSpaceThumbnailUri, Result.GetUri());
    }

    LogOut(userSystem);

    LogIn(userSystem, primaryUserId, primaryTestUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceThumbnailWithGuestUserTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    ::Space space;
    FileAssetDataSource spaceThumbnail;
    const std::string localFileName = "test.json";
    auto filePath = std::filesystem::absolute("assets/" + localFileName);
    spaceThumbnail.FilePath = filePath.u8string().c_str();
    spaceThumbnail.SetMimeType("application/json");

    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, spaceThumbnail, nullptr, space);

    LogOut(userSystem);

    String guestId;
    LogInAsGuest(userSystem, guestId);

    FileAssetDataSource updatedSpaceThumbnail;
    filePath = std::filesystem::absolute("assets/Fox.glb");
    updatedSpaceThumbnail.FilePath = filePath.u8string().c_str();
    updatedSpaceThumbnail.SetMimeType("model/gltf-binary");

    {
        // A guest shouldn't be able to update the space thumbnail
        auto [Result] = AWAIT_PRE(spaceSystem, UpdateSpaceThumbnail, RequestPredicate, space.Id, updatedSpaceThumbnail);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        // But it should be able to retrieve it
        auto [Result] = AWAIT_PRE(spaceSystem, GetSpaceThumbnail, RequestPredicate, space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(Result.GetUri().c_str(), localFileName));
    }

    LogOut(userSystem);

    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BanGuestUserTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Login with first user to create space
    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

    LogOut(userSystem);

    // Login with second user and join space
    String guestId;
    LogInAsGuest(userSystem, guestId);

    auto [AddUserResult] = AWAIT_PRE(spaceSystem, AddUserToSpace, RequestPredicate, space.Id, guestId);

    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(userSystem);

    // Login again with first user to ban second user
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    GetSpace(spaceSystem, space.Id, space);

    {
        auto [Result] = AWAIT_PRE(spaceSystem, AddUserToSpaceBanList, RequestPredicate, space.Id, guestId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(spaceSystem, space.Id, space);

        EXPECT_FALSE(space.BannedUserIds.IsEmpty());
        EXPECT_EQ(space.BannedUserIds[0], guestId);
    }

    {
        auto [Result] = AWAIT_PRE(spaceSystem, DeleteUserFromSpaceBanList, RequestPredicate, space.Id, guestId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(spaceSystem, space.Id, space);

        EXPECT_TRUE(space.BannedUserIds.IsEmpty());
    }

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BanUserTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Login with first user to create space
    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

    LogOut(userSystem);

    // Login with second user and join space
    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    auto [AddUserResult] = AWAIT_PRE(spaceSystem, AddUserToSpace, RequestPredicate, space.Id, altUserId);

    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(userSystem);

    // Login again with first user to ban second user
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    GetSpace(spaceSystem, space.Id, space);

    {
        auto [Result] = AWAIT_PRE(spaceSystem, AddUserToSpaceBanList, RequestPredicate, space.Id, altUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(spaceSystem, space.Id, space);

        EXPECT_FALSE(space.BannedUserIds.IsEmpty());
        EXPECT_EQ(space.BannedUserIds[0], altUserId);
    }
    {
        auto [Result] = AWAIT_PRE(spaceSystem, DeleteUserFromSpaceBanList, RequestPredicate, space.Id, altUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(spaceSystem, space.Id, space);

        EXPECT_TRUE(space.BannedUserIds.IsEmpty());
    }

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    {
        EXPECT_FALSE(spaceSystem->IsInSpace());

        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [Result] = AWAIT(spaceSystem, EnterSpace, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_TRUE(spaceSystem->IsInSpace());

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        EXPECT_FALSE(spaceSystem->IsInSpace());
    }

    LogOut(userSystem);

    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [Result] = AWAIT(spaceSystem, EnterSpace, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(userSystem);

    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceAsNonModeratorTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);
    LogOut(userSystem);

    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);
    LogOut(userSystem);

    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [Result] = AWAIT(spaceSystem, EnterSpace, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(userSystem);

    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceAsModeratorTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);
    LogOut(userSystem);

    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);
    ::Space space;
    InviteUserRoleInfo inviteUser;
    inviteUser.UserEmail = altUser.Email;
    inviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser };
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteUsers, nullptr, nullptr, space);

    ::UserRoleInfo newUserRoleInfo;
    newUserRoleInfo.UserId = altUserId;
    newUserRoleInfo.UserRole = SpaceUserRole::Moderator;

    UpdateUserRole(spaceSystem, space.Id, newUserRoleInfo);

    LogOut(userSystem);

    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    // Note the space is now out of date and does not have the new user in its lists
    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [Result] = AWAIT(spaceSystem, EnterSpace, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    LogOut(userSystem);

    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String primaryUserId;
    LogInAsNewTestUser(userSystem, primaryUserId);
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    GeoLocation initialGeoLocation;
    initialGeoLocation.Latitude = 1.1;
    initialGeoLocation.Longitude = 2.2;

    float initialOrientation = 90.0f;

    csp::common::Array<GeoLocation> initialGeoFence(4);

    GeoLocation geoFence0;
    geoFence0.Latitude = 5.5;
    geoFence0.Longitude = 6.6;
    initialGeoFence[0] = geoFence0;
    initialGeoFence[3] = geoFence0;

    GeoLocation geoFence1;
    geoFence1.Latitude = 7.7;
    geoFence1.Longitude = 8.8;
    initialGeoFence[1] = geoFence1;

    GeoLocation geoFence2;
    geoFence2.Latitude = 9.9;
    geoFence2.Longitude = 10.0;
    initialGeoFence[2] = geoFence2;

    auto [AddGeoResult]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, initialGeoLocation, initialOrientation, initialGeoFence);

    EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(AddGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().Location.Latitude, initialGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().Location.Longitude, initialGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().Orientation, initialOrientation);

    for (size_t i = 0; i < AddGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
    {
        EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, initialGeoFence[i].Latitude);
        EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, initialGeoFence[i].Longitude);
    }

    auto [GetGeoResult] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(GetGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(GetGeoResult.GetSpaceGeoLocation().Location.Latitude, initialGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(GetGeoResult.GetSpaceGeoLocation().Location.Longitude, initialGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(GetGeoResult.GetSpaceGeoLocation().Orientation, initialOrientation);

    GeoLocation secondGeoLocation;
    secondGeoLocation.Latitude = 3.3;
    secondGeoLocation.Longitude = 4.4;

    float secondOrientation = 270.0f;

    csp::common::Array<GeoLocation> secondGeoFence(4);
    geoFence0.Latitude = 11.1;
    geoFence0.Longitude = 12.2;
    secondGeoFence[0] = geoFence0;
    secondGeoFence[3] = geoFence0;
    geoFence1.Latitude = 13.3;
    geoFence1.Longitude = 14.4;
    secondGeoFence[1] = geoFence1;
    geoFence2.Latitude = 15.5;
    geoFence2.Longitude = 16.6;
    secondGeoFence[2] = geoFence2;

    auto [UpdateGeoResult]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, secondGeoLocation, secondOrientation, secondGeoFence);

    EXPECT_EQ(UpdateGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(UpdateGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().Location.Latitude, secondGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().Location.Longitude, secondGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().Orientation, secondOrientation);

    for (size_t i = 0; i < UpdateGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
    {
        EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, secondGeoFence[i].Latitude);
        EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, secondGeoFence[i].Longitude);
    }

    auto [GetUpdatedGeoResult] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetUpdatedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(GetUpdatedGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().Location.Latitude, secondGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().Location.Longitude, secondGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().Orientation, secondOrientation);

    for (size_t i = 0; i < GetUpdatedGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
    {
        EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, secondGeoFence[i].Latitude);
        EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, secondGeoFence[i].Longitude);
    }

    auto [DeleteGeoResult] = AWAIT_PRE(spaceSystem, DeleteSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(DeleteGeoResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetDeletedGeoResult] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetDeletedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(GetDeletedGeoResult.HasSpaceGeoLocation());

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationValidationTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String primaryUserId;
    LogInAsNewTestUser(userSystem, primaryUserId);
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    GeoLocation validGeoLocation;
    validGeoLocation.Latitude = 1.1;
    validGeoLocation.Longitude = 2.2;

    GeoLocation invalidGeoLocation;
    invalidGeoLocation.Latitude = 500.0;
    invalidGeoLocation.Longitude = 2.2;

    float validOrientation = 90.0f;
    float invalidOrientation = 500.0f;

    csp::common::Array<GeoLocation> validGeoFence(4);
    csp::common::Array<GeoLocation> shortGeoFence(2);
    csp::common::Array<GeoLocation> invalidGeoFence(4);
    csp::common::Array<GeoLocation> invalidGeoLocationGeoFence(4);
    GeoLocation geoFence0;
    geoFence0.Latitude = 5.5;
    geoFence0.Longitude = 6.6;
    GeoLocation geoFence1;
    geoFence1.Latitude = 7.7;
    geoFence1.Longitude = 8.8;
    GeoLocation geoFence2;
    geoFence2.Latitude = 9.9;
    geoFence2.Longitude = 10.0;

    validGeoFence[0] = geoFence0;
    validGeoFence[1] = geoFence1;
    validGeoFence[2] = geoFence2;
    validGeoFence[3] = geoFence0;

    shortGeoFence[0] = geoFence0;
    shortGeoFence[1] = geoFence2;

    invalidGeoFence[0] = geoFence0;
    invalidGeoFence[1] = geoFence1;
    invalidGeoFence[2] = geoFence2;
    invalidGeoFence[3] = geoFence2;

    invalidGeoLocationGeoFence[0] = geoFence0;
    invalidGeoLocationGeoFence[1] = geoFence1;
    invalidGeoLocationGeoFence[2] = invalidGeoLocation;
    invalidGeoLocationGeoFence[3] = geoFence0;

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, invalidGeoLocation, validOrientation, validGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, invalidOrientation, validGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, shortGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, invalidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult] = AWAIT_PRE(
            spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, invalidGeoLocationGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Actually add a geo location and test again since a different code path is followed when one exists
    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, validGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, invalidGeoLocation, validOrientation, validGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, invalidOrientation, validGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, shortGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, invalidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult] = AWAIT_PRE(
            spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, validGeoLocation, validOrientation, invalidGeoLocationGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [DeleteGeoResult] = AWAIT_PRE(spaceSystem, DeleteSpaceGeoLocation, RequestPredicate, space.Id);

        EXPECT_EQ(DeleteGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationWithoutPermissionTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Create a space as the primary user
    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Switch to the alt user to try and update the geo location
    LogOut(userSystem);
    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    GeoLocation initialGeoLocation;
    initialGeoLocation.Latitude = 1.1;
    initialGeoLocation.Longitude = 2.2;

    float initialOrientation = 90.0f;

    auto [AddGeoResultAsAlt]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, initialGeoLocation, initialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(AddGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Switch back to the primary user to actually create the geo location
    LogOut(userSystem);
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    auto [AddGeoResultAsPrimary]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, initialGeoLocation, initialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    // Switch back to the alt user again
    LogOut(userSystem);
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    // Test they cannot get the space geo location details since the space is private
    auto [GetGeoResultAsAlt] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Test they cannot update the geolocation
    GeoLocation secondGeoLocation;
    secondGeoLocation.Latitude = 3.3;
    secondGeoLocation.Longitude = 4.4;

    float secondOrientation = 270.0f;

    auto [UpdateGeoResultAsAlt]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, secondGeoLocation, secondOrientation, nullptr);

    EXPECT_EQ(UpdateGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdateGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Test they cannot delete the geo location
    auto [DeleteGeoResultAsAlt] = AWAIT_PRE(spaceSystem, DeleteSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(DeleteGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(DeleteGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Log back in as primary to clean up
    LogOut(userSystem);
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    auto [DeleteGeoResultAsPrimary] = AWAIT_PRE(spaceSystem, DeleteSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(DeleteGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetDeletedGeoResult] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetDeletedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(GetDeletedGeoResult.HasSpaceGeoLocation());

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationWithoutPermissionPublicSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Create a space as the primary user
    String primaryUserId;
    csp::systems::Profile primaryUser = CreateTestUser();
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

    // Switch to the alt user to try and update the geo location
    LogOut(userSystem);
    String altUserId;
    csp::systems::Profile altUser = CreateTestUser();
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    GeoLocation initialGeoLocation;
    initialGeoLocation.Latitude = 1.1;
    initialGeoLocation.Longitude = 2.2;

    float initialOrientation = 90.0f;

    auto [AddGeoResultAsAlt]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, initialGeoLocation, initialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(AddGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Switch back to the primary user to actually create the geo location
    LogOut(userSystem);
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    auto [AddGeoResultAsPrimary]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, initialGeoLocation, initialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    // Switch back to the alt user again
    LogOut(userSystem);
    LogIn(userSystem, altUserId, altUser.Email, GeneratedTestAccountPassword);

    // Test they can get the space geo location details since the space is public
    auto [GetGeoResultAsAlt] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(GetGeoResultAsAlt.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Location.Latitude, initialGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Location.Longitude, initialGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Orientation, initialOrientation);

    // Test they cannot update the geolocation
    GeoLocation secondGeoLocation;
    secondGeoLocation.Latitude = 3.3;
    secondGeoLocation.Longitude = 4.4;

    float secondOrientation = 270.0f;

    auto [UpdateGeoResultAsAlt]
        = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, secondGeoLocation, secondOrientation, nullptr);

    EXPECT_EQ(UpdateGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdateGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Test they cannot delete the geo location
    auto [DeleteGeoResultAsAlt] = AWAIT_PRE(spaceSystem, DeleteSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(DeleteGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(DeleteGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Log back in as primary to clean up
    LogOut(userSystem);
    LogIn(userSystem, primaryUserId, primaryUser.Email, GeneratedTestAccountPassword);

    auto [DeleteGeoResultAsPrimary] = AWAIT_PRE(spaceSystem, DeleteSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(DeleteGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetDeletedGeoResult] = AWAIT_PRE(spaceSystem, GetSpaceGeoLocation, RequestPredicate, space.Id);

    EXPECT_EQ(GetDeletedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(GetDeletedGeoResult.HasSpaceGeoLocation());

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, DuplicateSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-TEST-SPACE";
    const char* testSpaceDescription = "CSP-TEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Create default and alt users
    csp::systems::Profile defaultUser = CreateTestUser();
    csp::systems::Profile alternativeUser = CreateTestUser();

    // Log in
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Create space
    Array<InviteUserRoleInfo> userRoles(1);
    userRoles[0].UserEmail = alternativeUser.Email;
    userRoles[0].UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection inviteInfo;
    inviteInfo.InviteUserRoleInfos = userRoles;

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteInfo, nullptr, nullptr, space);

    // Log out and log in as alt user
    LogOut(userSystem);
    LogIn(userSystem, userId, alternativeUser.Email, GeneratedTestAccountPassword);

    // Attempt to duplicate space
    {
        SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

        auto [Result] = AWAIT_PRE(spaceSystem, DuplicateSpace, RequestPredicate, space.Id, uniqueSpaceName, SpaceAttributes::Private, nullptr, true);

        ASSERT_EQ(Result.GetResultCode(), EResultCode::Success);

        const auto& newSpace = Result.GetSpace();

        ASSERT_NE(newSpace.Id, space.Id);
        ASSERT_EQ(newSpace.Name, uniqueSpaceName);
        ASSERT_EQ(newSpace.Description, space.Description);
        ASSERT_EQ(newSpace.Attributes, SpaceAttributes::Private);
        ASSERT_EQ(newSpace.OwnerId, userId);
        ASSERT_NE(space.OwnerId, userId);

        // Ensure we can enter the newly duplicated Space
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, newSpace.Id, realtimeEngine.get());
        ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        ASSERT_EQ(ExitSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Delete duplicated space
        DeleteSpace(spaceSystem, newSpace.Id);
    }

    // Log out and log in as default user to clean up original space
    LogOut(userSystem);
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, DuplicateSpaceAsyncTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-TEST-SPACE";
    const char* testSpaceDescription = "CSP-TEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;

    // Create default and alt users
    csp::systems::Profile defaultUser = CreateTestUser();
    csp::systems::Profile alternativeUser = CreateTestUser();

    // Log in
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Create space
    Array<InviteUserRoleInfo> userRoles(1);
    userRoles[0].UserEmail = alternativeUser.Email;
    userRoles[0].UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection inviteInfo;
    inviteInfo.InviteUserRoleInfos = userRoles;

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, inviteInfo, nullptr, nullptr, space);

    // Log out and log in as alt user
    LogOut(userSystem);
    LogIn(userSystem, userId, alternativeUser.Email, GeneratedTestAccountPassword);

    String newSpaceId;

    // Attempt to duplicate space asynchrously
    {
        std::promise<std::tuple<String, String, String>> asyncCallCompletedPromise;
        std::future<std::tuple<String, String, String>> asyncCallCompletedFuture = asyncCallCompletedPromise.get_future();

        auto asyncCallCompletedCallback = [&](const csp::common::AsyncCallCompletedEventData& networkEventData) {
            asyncCallCompletedPromise.set_value({ networkEventData.OperationName, networkEventData.ReferenceId, networkEventData.ReferenceType });
        };

        spaceSystem->SetAsyncCallCompletedCallback(asyncCallCompletedCallback);

        SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

        auto [Result]
            = AWAIT_PRE(spaceSystem, DuplicateSpaceAsync, RequestPredicate, space.Id, uniqueSpaceName, SpaceAttributes::Private, nullptr, true);

        ASSERT_EQ(Result.GetResultCode(), EResultCode::Success);

        // It is possible for this test to take an extended period of time to complete if the backend services are under load.
        // We are therefore setting a timeout on waiting for the async callback to be received.
        const std::chrono::seconds timeoutDuration(30);

        std::future_status status = asyncCallCompletedFuture.wait_for(timeoutDuration);

        ASSERT_NE(status, std::future_status::timeout) << "The DuplicateSpaceAsync operation timed out after 30 seconds.";

        std::tuple<String, String, String> asyncCallResult = asyncCallCompletedFuture.get();

        String operationName = std::get<0>(asyncCallResult);
        String referenceType = std::get<2>(asyncCallResult);
        newSpaceId = std::get<1>(asyncCallResult);

        ASSERT_TRUE(operationName == "DuplicateSpaceAsync");
        ASSERT_TRUE(referenceType == "GroupId");
    }

    // Ensure we can enter the newly duplicated Space
    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, newSpaceId, realtimeEngine.get());
        ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        ASSERT_EQ(ExitSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Delete duplicated space
    DeleteSpace(spaceSystem, newSpaceId);

    // Log out and log in as default user to clean up original space
    LogOut(userSystem);
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
namespace CSPEngine
{

class ExitSpaceRealtimeEngine : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

TEST_P(ExitSpaceRealtimeEngine, ExitSpaceRealtimeEngineTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* multiplayerConnection = systemsManager.GetMultiplayerConnection();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    String userId;
    csp::systems::Profile user = CreateTestUser();
    LogIn(userSystem, userId, user.Email, GeneratedTestAccountPassword);

    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    const csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    ASSERT_FALSE(spaceSystem->IsInSpace());

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        ASSERT_TRUE(multiplayerConnection->GetOnlineRealtimeEngine() == nullptr);
    }

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [Result] = AWAIT(spaceSystem, EnterSpace, space.Id, realtimeEngine.get());

    ASSERT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    ASSERT_TRUE(spaceSystem->IsInSpace());

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        ASSERT_TRUE(multiplayerConnection->GetOnlineRealtimeEngine() == realtimeEngine.get());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    ASSERT_FALSE(spaceSystem->IsInSpace());

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        ASSERT_TRUE(multiplayerConnection->GetOnlineRealtimeEngine() == nullptr);
    }

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

INSTANTIATE_TEST_SUITE_P(
    SpaceSystemTests, ExitSpaceRealtimeEngine, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

/*
 * Test all the permutations of EnterSpace, concerning space visibility and invite permissions.
 * Ensure that the method returns the correct success/failure's
 * First: The Attributes the space should be built with (gated, requiresinvite, etc)
 * Second: The expected result code from attempting to enter the space
 * Third: A string that is expected to be contained in stdout, (ie, what error message do we expect)
 */

class EnterSpaceWhenGuest : public PublicTestBaseWithParam<std::tuple<SpaceAttributes, csp::systems::EResultCode, std::string>>
{
};

class EnterSpaceWhenUninvited : public PublicTestBaseWithParam<std::tuple<SpaceAttributes, csp::systems::EResultCode, std::string>>
{
};

class EnterSpaceWhenInvited : public PublicTestBaseWithParam<std::tuple<SpaceAttributes, csp::systems::EResultCode, std::string>>
{
};

class EnterSpaceWhenCreator : public PublicTestBaseWithParam<std::tuple<SpaceAttributes, csp::systems::EResultCode, std::string>>
{
};

class EnterSpaceWhenBanned : public PublicTestBaseWithParam<std::tuple<SpaceAttributes, csp::systems::EResultCode, std::string>>
{
};

TEST_P(EnterSpaceWhenGuest, EnterSpaceWhenGuestTest)
{
    // Conditions & Expectations
    const SpaceAttributes spacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode joinSpaceResultExpected = std::get<1>(GetParam());
    const std::string expectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    std::string uniqueSpaceName = testSpaceName + std::string("-") + GetUniqueString();

    // Create a space according to param attribute
    String spaceOwnerUserId;
    csp::systems::Profile spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space createdSpace;
    CreateSpace(spaceSystem, uniqueSpaceName.c_str(), testSpaceDescription, spacePermission, nullptr, nullptr, nullptr, nullptr, createdSpace);
    LogOut(userSystem);

    // Log in as guest
    String guestUserId;
    LogInAsGuest(userSystem, guestUserId);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(RealtimeEngineType::Online) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), joinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string outStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(outStdErr.find(expectedMsg), std::string::npos);

    // Log out
    LogOut(userSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, createdSpace.Id);
    LogOut(userSystem);
}

TEST_P(EnterSpaceWhenUninvited, EnterSpaceWhenUninvitedTest)
{
    // Conditions & Expectations
    const SpaceAttributes spacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode joinSpaceResultExpected = std::get<1>(GetParam());
    const std::string expectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    std::string uniqueSpaceName = testSpaceName + std::string("-") + GetUniqueString();

    // Create a space according to param attribute
    String spaceOwnerUserId;
    csp::systems::Profile spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space createdSpace;
    CreateSpace(spaceSystem, uniqueSpaceName.c_str(), testSpaceDescription, spacePermission, nullptr, nullptr, nullptr, nullptr, createdSpace);
    LogOut(userSystem);

    // Log in as another user who isn't invited
    String uninvitedUserId;
    csp::systems::Profile uninvitedUser = CreateTestUser();
    LogIn(userSystem, uninvitedUserId, uninvitedUser.Email, GeneratedTestAccountPassword);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(RealtimeEngineType::Online) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), joinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string outStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(outStdErr.find(expectedMsg), std::string::npos);

    // Log out
    LogOut(userSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, createdSpace.Id);
    LogOut(userSystem);
}

TEST_P(EnterSpaceWhenInvited, EnterSpaceWhenInvitedTest)
{
    // Conditions & Expectations
    const SpaceAttributes spacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode joinSpaceResultExpected = std::get<1>(GetParam());
    const std::string expectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    std::string uniqueSpaceName = testSpaceName + std::string("-") + GetUniqueString();

    // Create a space according to param attribute, and invite a user
    csp::systems::Profile invitedUser = CreateTestUser();

    InviteUserRoleInfo inviteUser;
    inviteUser.UserEmail = invitedUser.Email;
    inviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser };

    String spaceOwnerUserId;
    csp::systems::Profile spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space createdSpace;
    CreateSpace(spaceSystem, uniqueSpaceName.c_str(), testSpaceDescription, spacePermission, nullptr, inviteUsers, nullptr, nullptr, createdSpace);
    LogOut(userSystem);

    // Log in as invited user
    String invitedUserId;
    LogIn(userSystem, invitedUserId, invitedUser.Email, GeneratedTestAccountPassword);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(RealtimeEngineType::Online) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), joinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string outStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(outStdErr.find(expectedMsg), std::string::npos);

    // Log out
    LogOut(userSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, createdSpace.Id);
    LogOut(userSystem);
}

TEST_P(EnterSpaceWhenCreator, EnterSpaceWhenCreatorTest)
{
    // Conditions & Expectations
    const SpaceAttributes spacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode joinSpaceResultExpected = std::get<1>(GetParam());
    const std::string expectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    std::string uniqueSpaceName = testSpaceName + std::string("-") + GetUniqueString();

    // Create a space according to param attribute
    String spaceOwnerUserId;
    csp::systems::Profile spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space createdSpace;
    CreateSpace(spaceSystem, uniqueSpaceName.c_str(), testSpaceDescription, spacePermission, nullptr, nullptr, nullptr, nullptr, createdSpace);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(RealtimeEngineType::Online) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), joinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string outStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(outStdErr.find(expectedMsg), std::string::npos);

    // Delete test space
    DeleteSpace(spaceSystem, createdSpace.Id);
    LogOut(userSystem);
}

TEST_P(EnterSpaceWhenBanned, EnterSpaceWhenBannedTest)
{
    // Conditions & Expectations
    const SpaceAttributes spacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode joinSpaceResultExpected = std::get<1>(GetParam());
    const std::string expectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    std::string uniqueSpaceName = testSpaceName + std::string("-") + GetUniqueString();

    // Create a space according to param attribute, and ban a user
    csp::systems::Profile bannedUser = CreateTestUser();

    // Invite the banned user, to make sure that bans apply even if invited.
    InviteUserRoleInfo inviteUser;
    inviteUser.UserEmail = bannedUser.Email;
    inviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser };

    String spaceOwnerUserId;
    csp::systems::Profile spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space createdSpace;
    CreateSpace(spaceSystem, uniqueSpaceName.c_str(), testSpaceDescription, spacePermission, nullptr, inviteUsers, nullptr, nullptr, createdSpace);
    LogOut(userSystem);

    // Log in as banned user
    String bannedUserId;
    LogIn(userSystem, bannedUserId, bannedUser.Email, GeneratedTestAccountPassword);

    {
        std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(RealtimeEngineType::Online) };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        // In order to ban the user, they have to have entered the space. (This seems like an underthought limitation)
        auto [EnterSpaceResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
        ASSERT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        ASSERT_EQ(ExitSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
        LogOut(userSystem);

        // Log back in as owner and ban the user
        LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
        auto [Result] = AWAIT_PRE(spaceSystem, AddUserToSpaceBanList, RequestPredicate, createdSpace.Id, bannedUser.UserId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        LogOut(userSystem);
    }
    {
        // Login as the banned user and attempt to enter the space and check the expected result
        LogIn(userSystem, bannedUserId, bannedUser.Email, GeneratedTestAccountPassword);

        std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(RealtimeEngineType::Online) };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        testing::internal::CaptureStderr();
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
        ASSERT_EQ(EnterResult.GetResultCode(), joinSpaceResultExpected);

        // Verify that Stderr contains expected message.
        std::string outStdErr = testing::internal::GetCapturedStderr();
        EXPECT_NE(outStdErr.find(expectedMsg), std::string::npos);

        // Log out
        LogOut(userSystem);
    }

    // Login as owner user in order to be able to delete the test space
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, createdSpace.Id);
    LogOut(userSystem);
}

// Before adding another matrix to this giant case, look into `testing::Combine` to see if we can get away with being terser
INSTANTIATE_TEST_SUITE_P(SpaceSystemTests, EnterSpaceWhenGuest,
    testing::Values(std::make_tuple(SpaceAttributes::Gated, csp::systems::EResultCode::Failed,
                        "Logged in user does not have permission to join this space. Failed to add to space."),
        std::make_tuple(SpaceAttributes::IsDiscoverable, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::None, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Private, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::Public, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::RequiresInvite, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."), // RequiresInvite == Private, although the
                                                                                                       // name dosen't really convey it. :(
        std::make_tuple(SpaceAttributes::Unlisted, csp::systems::EResultCode::Success, "Successfully entered space.")));

INSTANTIATE_TEST_SUITE_P(SpaceSystemTests, EnterSpaceWhenUninvited,
    testing::Values(std::make_tuple(SpaceAttributes::Gated, csp::systems::EResultCode::Failed,
                        "Logged in user does not have permission to join this space. Failed to add to space."),
        std::make_tuple(SpaceAttributes::IsDiscoverable, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::None, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Private, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::Public, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::RequiresInvite, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::Unlisted, csp::systems::EResultCode::Success, "Successfully entered space.")));

INSTANTIATE_TEST_SUITE_P(SpaceSystemTests, EnterSpaceWhenInvited,
    testing::Values(std::make_tuple(SpaceAttributes::Gated, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::IsDiscoverable, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::None, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Private, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Public, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::RequiresInvite, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Unlisted, csp::systems::EResultCode::Success, "Successfully entered space.")));

INSTANTIATE_TEST_SUITE_P(SpaceSystemTests, EnterSpaceWhenCreator,
    testing::Values(std::make_tuple(SpaceAttributes::Gated, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::IsDiscoverable, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::None, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Private, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Public, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::RequiresInvite, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(SpaceAttributes::Unlisted, csp::systems::EResultCode::Success, "Successfully entered space.")));

INSTANTIATE_TEST_SUITE_P(SpaceSystemTests, EnterSpaceWhenBanned,
    testing::Values(std::make_tuple(SpaceAttributes::Gated, csp::systems::EResultCode::Failed,
                        "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::IsDiscoverable, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::None, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::Private, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::Public, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::RequiresInvite, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space."),
        std::make_tuple(SpaceAttributes::Unlisted, csp::systems::EResultCode::Failed,
            "Logged in user does not have permission to discover this space. Failed to enter space.")));

/*
 * Test all the permutations of EnterSpace, concerning realtime engine type and whether or not the multiplayer connection is established.
 * Tested seperately to the access control tests above, as the logic is isolated. Access control is not performed on offline engines.
 * First: The realtime engine type to create
 * Second: Whether or not to create a multiplayer connection on login
 * Third: The expected result code from attempting to enter the space
 * Fourth: A string that is expected to be contained in stdout, (ie, what error message do we expect)
 */

class EnterSpaceOnlineOffline
    : public PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool, csp::systems::EResultCode, std::string>>
{
};

TEST_P(EnterSpaceOnlineOffline, EnterSpaceOnlineOfflineTest)
{
    // Conditions & Expectations
    const csp::common::RealtimeEngineType realtimeEngineType = std::get<0>(GetParam());
    const bool loginWithMultiplayerConnection = std::get<1>(GetParam());
    const csp::systems::EResultCode joinSpaceResultExpected = std::get<2>(GetParam());
    const std::string expectedMsg = std::get<3>(GetParam());

    const bool isOnline = (realtimeEngineType == csp::common::RealtimeEngineType::Online);

    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    Space createdSpace;
    csp::systems::Profile spaceOwnerUser;
    String spaceOwnerUserId;
    // Make this space even if using an offline engine, to allow us to check that we've not added a user to it in offline mode later
    std::string uniqueSpaceName = testSpaceName + std::string("-") + GetUniqueString();

    spaceOwnerUser = CreateTestUser();
    LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
    CreateSpace(
        spaceSystem, uniqueSpaceName.c_str(), testSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, createdSpace);
    LogOut(userSystem);

    // Log in as guest (best to check offline engine logins, as it's the expected case ... although it shouldn't matter access permissions don't apply
    // to offline)
    String guestUserId;
    LogInAsGuest(userSystem, guestUserId, loginWithMultiplayerConnection);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();

    const auto createdSpaceId = isOnline ? createdSpace.Id : csp::common::String { "Offline Space" };

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, createdSpace.Id, realtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), joinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string outStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(outStdErr.find(expectedMsg), std::string::npos);

    // If we're offline, check that we havn't actually entered the space as far as CHS is concerned
    // This is a bit of an encapsulation break, as this depends on knowing that the local user ID and the logged in one are different.
    // With network mocks, we could check that we never actually call the endpoints, which would be better
    if (!isOnline)
    {
        ::Space queriedSpace;
        GetSpace(spaceSystem, createdSpace.Id, queriedSpace);
        EXPECT_FALSE(std::any_of(queriedSpace.UserIds.cbegin(), queriedSpace.UserIds.cend(),
            [&EnterResult](const csp::common::String& userId) { return userId == EnterResult.GetSpace().CreatedBy; }));
    }

    LogOut(userSystem);

    if (isOnline)
    {
        // Login as owner user in order to be able to delete the test space
        LogIn(userSystem, spaceOwnerUserId, spaceOwnerUser.Email, GeneratedTestAccountPassword);
        DeleteSpace(spaceSystem, createdSpace.Id);
        LogOut(userSystem);
    }
}

INSTANTIATE_TEST_SUITE_P(SpaceSystemTests, EnterSpaceOnlineOffline,
    testing::Values(std::make_tuple(RealtimeEngineType::Online, true, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(RealtimeEngineType::Online, false, csp::systems::EResultCode::Failed,
            "Cannot enter an online space without an established multiplayer connection. Did you create one when logging in?"),
        std::make_tuple(RealtimeEngineType::Offline, true, csp::systems::EResultCode::Success, "Successfully entered space."),
        std::make_tuple(RealtimeEngineType::Offline, false, csp::systems::EResultCode::Success, "Successfully entered space.")));

}
