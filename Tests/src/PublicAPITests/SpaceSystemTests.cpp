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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest-param-test.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <filesystem>
#include <tuple>
#include <uuid_v4.h>

using namespace csp::common;
using namespace csp::systems;

// TODO: Clean up these tests

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

bool RequestPredicateWithProgress(const csp::systems::ResultBase& Result)
{
    if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        PrintProgress(Result.GetRequestProgress());

        return false;
    }

    return true;
}

void CreateSpace(::SpaceSystem* SpaceSystem, const String& Name, const String& Description, SpaceAttributes Attributes,
    const Optional<Map<String, String>>& Metadata, const Optional<InviteUserRoleInfoCollection>& InviteUsers,
    const Optional<FileAssetDataSource>& Thumbnail, const Optional<Array<String>>& Tags, Space& OutSpace)
{
    Map<String, String> TestMetadata = Metadata.HasValue() ? (*Metadata) : Map<String, String>({ { "site", "Void" } });

    // TODO: Add tests for public spaces
    auto [Result] = AWAIT_PRE(SpaceSystem, CreateSpace, RequestPredicate, Name, Description, Attributes, InviteUsers, TestMetadata, Thumbnail, Tags);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutSpace = Result.GetSpace();
}

void CreateSpaceWithBuffer(::SpaceSystem* SpaceSystem, const String& Name, const String& Description, SpaceAttributes Attributes,
    const Optional<Map<String, String>>& Metadata, const Optional<InviteUserRoleInfoCollection>& InviteUsers, BufferAssetDataSource& Thumbnail,
    const Optional<Array<String>>& Tags, Space& OutSpace)
{
    Map<String, String> TestMetadata = Metadata.HasValue() ? (*Metadata) : Map<String, String>({ { "site", "Void" } });

    auto [Result]
        = AWAIT_PRE(SpaceSystem, CreateSpaceWithBuffer, RequestPredicate, Name, Description, Attributes, InviteUsers, TestMetadata, Thumbnail, Tags);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutSpace = Result.GetSpace();
}

void CreateDefaultTestSpace(SpaceSystem* SpaceSystem, Space& OutSpace)
{
    // Create space
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, OutSpace);
}

void GetSpace(::SpaceSystem* SpaceSystem, const String& SpaceId, Space& OutSpace)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpace, RequestPredicate, SpaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutSpace = Result.GetSpace();
}

Array<BasicSpace> GetSpacesByAttributes(::SpaceSystem* SpaceSystem, const Optional<bool>& IsDiscoverable, const Optional<bool>& IsArchived,
    const Optional<bool>& RequiresInvite, const Optional<int>& ResultsSkipNo, const Optional<int>& ResultsMaxNo)
{
    auto [Result]
        = AWAIT_PRE(SpaceSystem, GetSpacesByAttributes, RequestPredicate, IsDiscoverable, IsArchived, RequiresInvite, ResultsSkipNo, ResultsMaxNo);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto SpacesTotalCount = Result.GetTotalCount();
    const auto Spaces = Result.GetSpaces();

    if (Spaces.Size() > 0)
    {
        EXPECT_GT(SpacesTotalCount, 0);
    }

    return Spaces;
}

Array<Space> GetSpacesByIds(::SpaceSystem* SpaceSystem, const Array<String>& SpaceIDs)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpacesByIds, RequestPredicate, SpaceIDs);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetSpaces();
}

void UpdateSpace(::SpaceSystem* SpaceSystem, const String& SpaceId, const Optional<String>& NewName, const Optional<String>& NewDescription,
    const Optional<SpaceAttributes>& NewAttributes, BasicSpace& OutSpace)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpace, RequestPredicate, SpaceId, NewName, NewDescription, NewAttributes);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutSpace = Result.GetSpace();
}

void AddSiteInfo(::SpaceSystem* SpaceSystem, const char* Name, const String& SpaceId, Site& OutSite)
{
    const char* SiteName = (Name) ? Name : "OLY-UNITTEST-SITE-NAME";

    GeoLocation SiteLocation(175.0, 85.0);
    OlyRotation SiteRotation(200.0, 200.0, 200.0, 200.0);

    Site SiteInfo;
    SiteInfo.Name = SiteName;
    SiteInfo.Location = SiteLocation;
    SiteInfo.Rotation = SiteRotation;

    auto [Result] = AWAIT_PRE(SpaceSystem, AddSiteInfo, RequestPredicate, SpaceId, SiteInfo);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutSite = Result.GetSite();
    std::cerr << "Site Created: Name=" << OutSite.Name << " Id=" << OutSite.Id << std::endl;
}

void DeleteSpace(::SpaceSystem* SpaceSystem, const String& SpaceId)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, DeleteSpace, RequestPredicate, SpaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void RemoveSiteInfo(::SpaceSystem* SpaceSystem, const String& SpaceId, ::Site& Site)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, RemoveSiteInfo, RequestPredicate, SpaceId, Site);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    std::cerr << "Site Deleted: Name=" << Site.Name << " Id=" << Site.Id << std::endl;
}

void GetSpaceSites(::SpaceSystem* SpaceSystem, const String& SpaceId, Array<Site>& OutSites)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSitesInfo, RequestPredicate, SpaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& ResultSites = Result.GetSites();
    OutSites = Array<Site>(ResultSites.Size());

    for (int idx = 0; idx < ResultSites.Size(); ++idx)
    {
        OutSites[idx] = ResultSites[idx];
    }
}

void UpdateUserRole(::SpaceSystem* SpaceSystem, const String& SpaceId, UserRoleInfo& NewUserRoleInfo)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, UpdateUserRole, RequestPredicate, SpaceId, NewUserRoleInfo);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cerr << "The user role for UserId: " << NewUserRoleInfo.UserId << " has been updated successfully" << std::endl;
    }
}

void GetRoleForSpecificUser(::SpaceSystem* SpaceSystem, const String& SpaceId, const String& UserId, UserRoleInfo& OutUserRoleInfo)
{
    Array<String> Ids = { UserId };
    auto [Result] = AWAIT_PRE(SpaceSystem, GetUsersRoles, RequestPredicate, SpaceId, Ids);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& ReturnedRolesInfo = Result.GetUsersRoles();

    EXPECT_EQ(ReturnedRolesInfo.Size(), 1);

    OutUserRoleInfo = ReturnedRolesInfo[0];
}

void GetUsersRoles(::SpaceSystem* SpaceSystem, const String& SpaceId, const Array<String>& RequestedUserIds, Array<UserRoleInfo>& OutUsersRoles)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetUsersRoles, RequestPredicate, SpaceId, RequestedUserIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& ReturnedRolesInfo = Result.GetUsersRoles();
    OutUsersRoles = Array<UserRoleInfo>(ReturnedRolesInfo.Size());

    for (int idx = 0; idx < ReturnedRolesInfo.Size(); ++idx)
    {
        OutUsersRoles[idx] = ReturnedRolesInfo[idx];
    }
}

void UpdateAndAssertSpaceMetadata(
    ::SpaceSystem* SpaceSystem, const String& SpaceId, const Optional<Map<String, String>>& NewMetadata, const Optional<Array<String>>& Tags)
{
    Map<String, String> Metadata = NewMetadata.HasValue() ? *NewMetadata : Map<String, String>();

    auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceMetadata, RequestPredicate, SpaceId, Metadata, Tags);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    std::cerr << "Space metadata has been updated successfully" << std::endl;
}

Map<String, String> GetAndAssertSpaceMetadata(::SpaceSystem* SpaceSystem, const String& SpaceId)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceMetadata, RequestPredicate, SpaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetMetadata();
}

Map<String, Map<String, String>> GetAndAssertSpacesMetadata(::SpaceSystem* SpaceSystem, const Array<String>& SpaceIds)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpacesMetadata, RequestPredicate, SpaceIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetMetadata();
}

Array<String> GetAndAssertSpaceTags(::SpaceSystem* SpaceSystem, const String& SpaceId)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceMetadata, RequestPredicate, SpaceId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetTags();
}

Map<String, Array<String>> GetAndAssertSpacesTags(::SpaceSystem* SpaceSystem, const Array<String>& SpaceIds)
{
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpacesMetadata, RequestPredicate, SpaceIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    return Result.GetTags();
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

InviteUserRoleInfoCollection CreateInviteUsers()
{
    // Create normal users
    const auto TestUser1Email = String("testnopus.pokemon+1@magnopus.com");
    const auto TestUser2Email = String("testnopus.pokemon+2@magnopus.com");

    InviteUserRoleInfo InviteUser1;
    InviteUser1.UserEmail = TestUser1Email;
    InviteUser1.UserRole = SpaceUserRole::User;

    InviteUserRoleInfo InviteUser2;
    InviteUser2.UserEmail = TestUser2Email;
    InviteUser2.UserRole = SpaceUserRole::User;

    // Create moderator users
    const auto TestModInviteUser1Email = String("testnopus.pokemon+mod1@magnopus.com");
    const auto TestModInviteUser2Email = String("testnopus.pokemon+mod2@magnopus.com");

    InviteUserRoleInfo ModInviteUser1;
    ModInviteUser1.UserEmail = TestModInviteUser1Email;
    ModInviteUser1.UserRole = SpaceUserRole::Moderator;

    InviteUserRoleInfo ModInviteUser2;
    ModInviteUser2.UserEmail = TestModInviteUser2Email;
    ModInviteUser2.UserRole = SpaceUserRole::Moderator;

    InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser1, InviteUser2, ModInviteUser1, ModInviteUser2 };
    InviteUsers.EmailLinkUrl = "https://dev.magnoverse.space";
    InviteUsers.SignupUrl = "https://dev.magnoverse.space";

    return InviteUsers;
}

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACE_WITH_TAGS_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithTagsTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    Array<String> Tags = { "tag-test" };

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Tags, Space);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACE_WITH_BULK_INVITE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBulkInviteTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    auto InviteUsers = CreateInviteUsers();

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, nullptr, nullptr, Space);

    auto [GetInvitesResult] = AWAIT_PRE(SpaceSystem, GetPendingUserInvites, RequestPredicate, Space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& PendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(PendingInvites.Size(), InviteUsers.InviteUserRoleInfos.Size());

    for (auto idx = 0; idx < PendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << PendingInvites[idx] << std::endl;
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACEWITHBUFFER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBufferTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
    FILE* UploadFile = nullptr;
    fopen_s(&UploadFile, UploadFilePath.string().c_str(), "rb");

    uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
    auto* UploadFileData = new unsigned char[UploadFileSize];
    fread(UploadFileData, UploadFileSize, 1, UploadFile);
    fclose(UploadFile);

    BufferAssetDataSource BufferSource;
    BufferSource.Buffer = UploadFileData;
    BufferSource.BufferLength = UploadFileSize;

    BufferSource.SetMimeType("image/png");

    // Create space
    ::Space Space;
    CreateSpaceWithBuffer(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, BufferSource, nullptr, Space);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACEWITHBUFFER_WITH_BULK_INVITE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithBufferWithBulkInviteTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    auto InviteUsers = CreateInviteUsers();

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
    FILE* UploadFile = nullptr;
    fopen_s(&UploadFile, UploadFilePath.string().c_str(), "rb");

    uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
    auto* UploadFileData = new unsigned char[UploadFileSize];
    fread(UploadFileData, UploadFileSize, 1, UploadFile);
    fclose(UploadFile);

    BufferAssetDataSource BufferSource;
    BufferSource.Buffer = UploadFileData;
    BufferSource.BufferLength = UploadFileSize;

    BufferSource.SetMimeType("image/png");

    // Create space
    ::Space Space;
    CreateSpaceWithBuffer(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, BufferSource, nullptr, Space);

    auto [GetInvitesResult] = AWAIT_PRE(SpaceSystem, GetPendingUserInvites, RequestPredicate, Space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& PendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(PendingInvites.Size(), InviteUsers.InviteUserRoleInfos.Size());

    for (auto idx = 0; idx < PendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << PendingInvites[idx] << std::endl;
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACEDESCRIPTION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceDescriptionTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Update space description
    char UpdatedDescription[256];
    SPRINTF(UpdatedDescription, "%s-Updated", TestSpaceDescription);

    BasicSpace UpdatedBasicSpace;
    UpdateSpace(SpaceSystem, Space.Id, nullptr, UpdatedDescription, nullptr, UpdatedBasicSpace);

    EXPECT_EQ(UpdatedBasicSpace.Name, Space.Name);
    EXPECT_EQ(UpdatedBasicSpace.Description, UpdatedDescription);
    EXPECT_EQ(UpdatedBasicSpace.Attributes, Space.Attributes);

    ::Space UpdatedSpace;
    GetSpace(SpaceSystem, Space.Id, UpdatedSpace);

    EXPECT_EQ(UpdatedSpace.Name, Space.Name);
    EXPECT_EQ(UpdatedSpace.Description, UpdatedDescription);
    EXPECT_EQ(UpdatedSpace.Attributes, Space.Attributes);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACETYPE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceTypeTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Update space type
    auto UpdatedAttributes = SpaceAttributes::Public;

    BasicSpace UpdatedBasicSpace;
    UpdateSpace(SpaceSystem, Space.Id, nullptr, nullptr, UpdatedAttributes, UpdatedBasicSpace);

    EXPECT_EQ(UpdatedBasicSpace.Name, Space.Name);
    EXPECT_EQ(UpdatedBasicSpace.Description, ""); // This should be empty because we elected to not give one when we invoked `UpdateSpace`.
    EXPECT_EQ(UpdatedBasicSpace.Attributes, UpdatedAttributes);

    ::Space UpdatedSpace;
    GetSpace(SpaceSystem, Space.Id, UpdatedSpace);

    EXPECT_EQ(UpdatedSpace.Name, Space.Name);
    EXPECT_EQ(UpdatedSpace.Description,
        ""); // This should remain cleared since not specifying a description in `UpdateSpace` is equivalent to clearing it.
    EXPECT_EQ(UpdatedSpace.Attributes, UpdatedAttributes);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpacesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Get spaces
    auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaces, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto& ResultSpaces = Result.GetSpaces();

    EXPECT_GT(ResultSpaces.Size(), 0);

    bool SpaceFound = false;

    for (int i = 0; i < ResultSpaces.Size(); ++i)
    {
        if (ResultSpaces[i].Name == UniqueSpaceName)
        {
            SpaceFound = true;

            break;
        }
    }

    EXPECT_TRUE(SpaceFound);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    ::Space ResultSpace;
    GetSpace(SpaceSystem, Space.Id, ResultSpace);

    EXPECT_EQ(ResultSpace.Name, Space.Name);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACESBYIDS_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpacesByIdsTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniquePublicSpaceName[256];
    SPRINTF(UniquePublicSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniquePrivateSpaceName[256];
    SPRINTF(UniquePrivateSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    ::Space PublicSpace;
    CreateSpace(SpaceSystem, UniquePublicSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, PublicSpace);

    ::Space PrivateSpace;
    CreateSpace(
        SpaceSystem, UniquePrivateSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, PrivateSpace);

    Array<String> SpacesIds = { PublicSpace.Id, PrivateSpace.Id };

    Array<::Space> ResultSpaces = GetSpacesByIds(SpaceSystem, SpacesIds);

    EXPECT_EQ(ResultSpaces.Size(), SpacesIds.Size());

    bool PrivateSpaceFound = false;
    bool PublicSpaceFound = false;

    for (int i = 0; i < ResultSpaces.Size(); ++i)
    {
        if (ResultSpaces[i].Name == UniquePrivateSpaceName)
        {
            PrivateSpaceFound = true;
            continue;
        }
        else if (ResultSpaces[i].Name == UniquePublicSpaceName)
        {
            PublicSpaceFound = true;
            continue;
        }
    }

    EXPECT_TRUE(PrivateSpaceFound);
    EXPECT_TRUE(PublicSpaceFound);

    DeleteSpace(SpaceSystem, PublicSpace.Id);
    DeleteSpace(SpaceSystem, PrivateSpace.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPUBLICSPACESASGUEST_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPublicSpacesAsGuestTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    constexpr size_t SPACE_COUNT = 3;

    String UserId;

    // Log in using default test account to create spaces
    csp::systems::Profile SpaceCreatorUser = CreateTestUser();
    LogIn(UserSystem, UserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);

    // Create test spaces
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    String SpaceId[SPACE_COUNT];

    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        ::Space Space;

        CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

        SpaceId[i] = Space.Id;
    }

    // Log out
    LogOut(UserSystem);

    // Log in as guest
    LogInAsGuest(UserSystem, UserId);

    // Get public spaces
    Array<BasicSpace> ResultSpaces = GetSpacesByAttributes(SpaceSystem, true, false, false, 0, static_cast<int>(SPACE_COUNT));

    EXPECT_GE(ResultSpaces.Size(), SPACE_COUNT);

    // Make sure that all returned spaces are public
    for (int i = 0; i < ResultSpaces.Size(); ++i)
    {
        const auto& Space = ResultSpaces[i];

        EXPECT_TRUE((bool)(Space.Attributes & SpaceAttributes::IsDiscoverable));
        EXPECT_FALSE((bool)(Space.Attributes & SpaceAttributes::RequiresInvite));
    }

    // Log out as guest
    LogOut(UserSystem);

    // Clean up
    LogIn(UserSystem, UserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);

    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        DeleteSpace(SpaceSystem, SpaceId[i]);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPUBLICSPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPublicSpacesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    constexpr size_t SPACE_COUNT = 3;

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create test spaces
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    String SpaceId[SPACE_COUNT];

    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        ::Space Space;

        CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

        SpaceId[i] = Space.Id;
    }

    // Get only the public spaces
    Array<BasicSpace> ResultSpaces = GetSpacesByAttributes(SpaceSystem, true, false, false, 0, static_cast<int>(SPACE_COUNT));

    EXPECT_GE(ResultSpaces.Size(), SPACE_COUNT);

    // Make sure that all returned spaces are public
    for (int i = 0; i < ResultSpaces.Size(); ++i)
    {
        const auto& Space = ResultSpaces[i];

        EXPECT_TRUE((bool)(Space.Attributes & SpaceAttributes::IsDiscoverable));
        EXPECT_FALSE((bool)(Space.Attributes & SpaceAttributes::RequiresInvite));
    }

    // Clean up
    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        DeleteSpace(SpaceSystem, SpaceId[i]);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPRIVATESPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPrivateSpacesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    constexpr size_t SPACE_COUNT = 3;

    String UserId;

    // Log in using default test account to create spaces
    LogInAsNewTestUser(UserSystem, UserId);

    // Create test spaces
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    String SpaceId[SPACE_COUNT];

    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        ::Space Space;

        CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

        SpaceId[i] = Space.Id;
    }

    // Get only the public spaces
    Array<BasicSpace> ResultSpaces = GetSpacesByAttributes(SpaceSystem, false, false, true, 0, static_cast<int>(SPACE_COUNT));

    EXPECT_GE(ResultSpaces.Size(), SPACE_COUNT);

    // Make sure that all returned spaces are public
    for (int i = 0; i < ResultSpaces.Size(); ++i)
    {
        const auto& Space = ResultSpaces[i];

        EXPECT_FALSE((bool)(Space.Attributes & SpaceAttributes::IsDiscoverable));
        EXPECT_TRUE((bool)(Space.Attributes & SpaceAttributes::RequiresInvite));
    }

    // Clean up
    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        DeleteSpace(SpaceSystem, SpaceId[i]);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPAGINATEDPRIVATESPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPaginatedPrivateSpacesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    constexpr size_t SPACE_COUNT = 6;

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create test spaces
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    String SpaceId[SPACE_COUNT];

    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        ::Space Space;

        CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

        SpaceId[i] = Space.Id;
    }

    // Get private spaces paginated
    {
        auto [Result] = AWAIT_PRE(SpaceSystem, GetSpacesByAttributes, RequestPredicate, false, false, true, 0, static_cast<int>(SPACE_COUNT / 2));

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto SpacesTotalCount = Result.GetTotalCount();
        const auto Spaces = Result.GetSpaces();

        EXPECT_EQ(Spaces.Size(), SPACE_COUNT / 2);
        EXPECT_GE(SpacesTotalCount, SPACE_COUNT);
    }

    // Clean up
    for (int i = 0; i < SPACE_COUNT; ++i)
    {
        DeleteSpace(SpaceSystem, SpaceId[i]);
    }

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_JOINPUBLICSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, JoinPublicSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Login as an admin user in order to be able to create the test space
    String SpaceOwnerUserId;
    csp::systems::Profile SpaceOwnerUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);

    ::Space PublicSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, PublicSpace);

    LogOut(UserSystem);

    // Log in as a guest user
    String GuestUserId;
    LogInAsGuest(UserSystem, GuestUserId);

    auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, PublicSpace.Id, GuestUserId);

    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    std::cerr << "User added to space" << std::endl;

    ::Space UpdatedPublicSpace;
    GetSpace(SpaceSystem, PublicSpace.Id, UpdatedPublicSpace);

    Array<::UserRoleInfo> RetrievedUserRoles;
    GetUsersRoles(SpaceSystem, UpdatedPublicSpace.Id, UpdatedPublicSpace.UserIds, RetrievedUserRoles);

    EXPECT_EQ(RetrievedUserRoles.Size(), 2);

    for (auto idx = 0; idx < RetrievedUserRoles.Size(); ++idx)
    {
        if (RetrievedUserRoles[idx].UserId == SpaceOwnerUserId)
        {
            EXPECT_EQ(RetrievedUserRoles[idx].UserRole, SpaceUserRole::Owner);
        }
        else if (RetrievedUserRoles[idx].UserId == GuestUserId)
        {
            EXPECT_EQ(RetrievedUserRoles[idx].UserRole, SpaceUserRole::User);
        }
        else
        {
            ASSERT_TRUE(false && "Encountered unexpected space user");
        }
    }

    // Log out
    LogOut(UserSystem);

    // Login as an admin user in order to be able to delete the test space
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, PublicSpace.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ADD_SITE_INFO_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, AddSiteInfoTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    Site SiteInfo;
    AddSiteInfo(SpaceSystem, nullptr, Space.Id, SiteInfo);

    RemoveSiteInfo(SpaceSystem, Space.Id, SiteInfo);

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_SITE_INFO_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSiteInfoTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    Site SiteInfo1, SiteInfo2;
    AddSiteInfo(SpaceSystem, "Site1", Space.Id, SiteInfo1);
    AddSiteInfo(SpaceSystem, "Site2", Space.Id, SiteInfo2);

    Array<Site> SpaceSites;
    GetSpaceSites(SpaceSystem, Space.Id, SpaceSites);

    EXPECT_EQ(SpaceSites.Size(), 2);

    bool Site1Found = false;
    bool Site2Found = false;

    for (int idx = 0; idx < SpaceSites.Size(); ++idx)
    {
        if (SpaceSites[idx].Name == SiteInfo1.Name)
        {
            Site1Found = true;
            continue;
        }
        else if (SpaceSites[idx].Name == SiteInfo2.Name)
        {
            Site2Found = true;
            continue;
        }
    }

    EXPECT_TRUE(Site1Found && Site2Found);

    RemoveSiteInfo(SpaceSystem, Space.Id, SiteInfo1);
    RemoveSiteInfo(SpaceSystem, Space.Id, SiteInfo2);

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_USER_ROLES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateUserRolesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Get alt account user ID
    String AltUserId;

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String DefaultUserId;

    // Create default and alternative users
    csp::systems::Profile DefaultUser = CreateTestUser();
    csp::systems::Profile AlternativeUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, DefaultUserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Create test space
    InviteUserRoleInfo InviteUser;
    InviteUser.UserEmail = AlternativeUser.Email;
    InviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser };
    InviteUsers.EmailLinkUrl = "dev.magnoverse.space";

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, nullptr, nullptr, Space);

    // Log out
    LogOut(UserSystem);

    // Log in using alt test account
    LogIn(UserSystem, AltUserId, AlternativeUser.Email, GeneratedTestAccountPassword);

    // Ensure alt test account can join space
    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Log out and log in again using default test account
    LogOut(UserSystem);
    LogIn(UserSystem, DefaultUserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Update test account user roles for space
    GetSpace(SpaceSystem, Space.Id, Space);

    ::UserRoleInfo UpdatedDefaultUserRole = { DefaultUserId, SpaceUserRole::Moderator };
    ::UserRoleInfo UpdatedSecondTestUserRole = { AltUserId, SpaceUserRole::Owner };

    // User Roles should not be changed after update as a owner cannot be modified
    // This also means a owner cannot be turned into a moderator
    auto [DefaultResult] = AWAIT_PRE(SpaceSystem, UpdateUserRole, RequestPredicate, Space.Id, UpdatedDefaultUserRole);

    // Update first account role should fail
    EXPECT_EQ(DefaultResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [SecondResult] = AWAIT_PRE(SpaceSystem, UpdateUserRole, RequestPredicate, Space.Id, UpdatedSecondTestUserRole);

    // Update second account role should fail
    EXPECT_EQ(SecondResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // Verify updated user roles
    Array<::UserRoleInfo> RetrievedUserRoles;
    GetUsersRoles(SpaceSystem, Space.Id, Space.UserIds, RetrievedUserRoles);

    EXPECT_EQ(RetrievedUserRoles.Size(), 2);

    for (auto idx = 0; idx < RetrievedUserRoles.Size(); ++idx)
    {
        if (RetrievedUserRoles[idx].UserId == DefaultUserId)
        {
            EXPECT_EQ(RetrievedUserRoles[idx].UserRole, SpaceUserRole::Owner);
        }
        else if (RetrievedUserRoles[idx].UserId == AltUserId)
        {
            EXPECT_EQ(RetrievedUserRoles[idx].UserRole, SpaceUserRole::User);
        }
        else
        {
            ASSERT_TRUE(false && "Encountered unexpected space user");
        }
    }

    GetSpace(SpaceSystem, Space.Id, Space);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_GUEST_USER_ROLE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateGuestUserRoleTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Login as an admin user in order to be able to create the test space
    String SpaceOwnerUserId;
    csp::systems::Profile AdminUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, AdminUser.Email, GeneratedTestAccountPassword);

    ::Space PublicSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, PublicSpace);

    LogOut(UserSystem);

    // Log in as a guest user
    String GuestUserId;
    LogInAsGuest(UserSystem, GuestUserId);

    auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, PublicSpace.Id, GuestUserId);
    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(UserSystem);

    // login as an admin user
    LogIn(UserSystem, SpaceOwnerUserId, AdminUser.Email, GeneratedTestAccountPassword);

    ::UserRoleInfo UpdatedUserRoleInfo = { GuestUserId, SpaceUserRole::Moderator };
    UpdateUserRole(SpaceSystem, PublicSpace.Id, UpdatedUserRoleInfo);

    ::UserRoleInfo RetrievedUserRoles;
    GetRoleForSpecificUser(SpaceSystem, PublicSpace.Id, GuestUserId, RetrievedUserRoles);
    EXPECT_EQ(RetrievedUserRoles.UserRole, SpaceUserRole::Moderator);

    DeleteSpace(SpaceSystem, PublicSpace.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_SET_USER_ROLE_ON_INVITE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, SetUserRoleOnInviteTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Get alt account user ID
    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);
    LogOut(UserSystem);

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String DefaultUserId;
    csp::systems::Profile DefaultUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, DefaultUserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // create a space with no other user Ids invited
    ::Space Space;
    // CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Invite second test account as a Moderator Role user
    auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AltUser.Email, true, "", "");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    ::UserRoleInfo UserRoleInfo;
    GetRoleForSpecificUser(SpaceSystem, Space.Id, AltUserId, UserRoleInfo);
    EXPECT_EQ(UserRoleInfo.UserRole, SpaceUserRole::Moderator);

    // As the default test user has the "internal-service" global role he can delete the space no matter the space role it holds.
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACE_METADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    Map<String, String> TestSpaceMetadata = { { "site", "Void" } };
    Array<String> Tags = { "tag-test" };

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, nullptr, Space);

    Map<String, String> RetrievedSpaceMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);

    EXPECT_EQ(RetrievedSpaceMetadata.Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata["site"], "Void");

    TestSpaceMetadata["site"] = "MagOffice";

    UpdateAndAssertSpaceMetadata(SpaceSystem, Space.Id, TestSpaceMetadata, Tags);

    RetrievedSpaceMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);

    EXPECT_EQ(RetrievedSpaceMetadata.Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata["site"], "MagOffice");

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_SPACES_METADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpacesMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    Map<String, String> TestSpaceMetadata = { { "site", "Void" } };

    ::Space Space1, Space2;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, nullptr, Space1);
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, nullptr, Space2);

    Array<String> Spaces = { Space1.Id, Space2.Id };
    Map<String, Map<String, String>> RetrievedSpacesMetadata = GetAndAssertSpacesMetadata(SpaceSystem, Spaces);

    EXPECT_EQ(RetrievedSpacesMetadata.Size(), 2);

    const auto& Metadata1 = RetrievedSpacesMetadata[Space1.Id];

    EXPECT_EQ(Metadata1.Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(Metadata1["site"], "Void");

    const auto& Metadata2 = RetrievedSpacesMetadata[Space2.Id];

    EXPECT_EQ(Metadata2.Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(Metadata2["site"], "Void");

    DeleteSpace(SpaceSystem, Spaces[0]);
    DeleteSpace(SpaceSystem, Spaces[1]);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACETAGS_METADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceTagsMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    Map<String, String> TestSpaceMetadata = { { "site", "Void" } };
    Array<String> Tags = { "tag-test" };

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, Tags, Space);

    Map<String, String> RetrievedSpaceMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);
    Array<String> RetrievedTags = GetAndAssertSpaceTags(SpaceSystem, Space.Id);

    EXPECT_EQ(RetrievedSpaceMetadata.Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata["site"], "Void");
    EXPECT_EQ(RetrievedTags.Size(), Tags.Size());
    EXPECT_EQ(RetrievedTags[0], "tag-test");

    TestSpaceMetadata["site"] = "MagOffice";

    // OB-3939 fix: passing tags as nullptr should leave them unchanged
    UpdateAndAssertSpaceMetadata(SpaceSystem, Space.Id, TestSpaceMetadata, nullptr);

    RetrievedSpaceMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);
    RetrievedTags = GetAndAssertSpaceTags(SpaceSystem, Space.Id);

    EXPECT_EQ(RetrievedSpaceMetadata.Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata["site"], "MagOffice");
    EXPECT_EQ(RetrievedTags.Size(), Tags.Size());
    EXPECT_EQ(RetrievedTags[0], "tag-test");

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACESTAGS_METADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpacesTagsMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    Map<String, String> TestSpaceMetadata = { { "site", "Void" } };
    Array<String> Tags = { "tag-test" };

    ::Space Space1, Space2;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, Tags, Space1);
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, Tags, Space2);

    Map<String, Map<String, String>> RetrievedSpaceMetadata = GetAndAssertSpacesMetadata(SpaceSystem, { Space1.Id, Space2.Id });
    Map<String, Array<String>> RetrievedTags = GetAndAssertSpacesTags(SpaceSystem, { Space1.Id, Space2.Id });

    EXPECT_EQ(RetrievedSpaceMetadata[Space1.Id].Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata[Space2.Id].Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata[Space1.Id]["site"], "Void");
    EXPECT_EQ(RetrievedSpaceMetadata[Space2.Id]["site"], "Void");
    EXPECT_EQ(RetrievedTags[Space1.Id].Size(), Tags.Size());
    EXPECT_EQ(RetrievedTags[Space2.Id].Size(), Tags.Size());
    EXPECT_EQ(RetrievedTags[Space1.Id][0], "tag-test");
    EXPECT_EQ(RetrievedTags[Space2.Id][0], "tag-test");

    TestSpaceMetadata["site"] = "MagOffice";

    // OB-3939 fix: passing tags as nullptr should leave them unchanged
    UpdateAndAssertSpaceMetadata(SpaceSystem, Space1.Id, TestSpaceMetadata, nullptr);
    UpdateAndAssertSpaceMetadata(SpaceSystem, Space2.Id, TestSpaceMetadata, nullptr);

    RetrievedSpaceMetadata = GetAndAssertSpacesMetadata(SpaceSystem, { Space1.Id, Space2.Id });
    RetrievedTags = GetAndAssertSpacesTags(SpaceSystem, { Space1.Id, Space2.Id });

    EXPECT_EQ(RetrievedSpaceMetadata[Space1.Id].Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata[Space2.Id].Size(), TestSpaceMetadata.Size());
    EXPECT_EQ(RetrievedSpaceMetadata[Space1.Id]["site"], "MagOffice");
    EXPECT_EQ(RetrievedSpaceMetadata[Space2.Id]["site"], "MagOffice");
    EXPECT_EQ(RetrievedTags[Space1.Id].Size(), Tags.Size());
    EXPECT_EQ(RetrievedTags[Space2.Id].Size(), Tags.Size());
    EXPECT_EQ(RetrievedTags[Space1.Id][0], "tag-test");
    EXPECT_EQ(RetrievedTags[Space2.Id][0], "tag-test");

    DeleteSpace(SpaceSystem, Space1.Id);
    DeleteSpace(SpaceSystem, Space2.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACE_THUMBNAIL_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceThumbnailTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    // Create space without a thumbnail
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
        EXPECT_TRUE(Result.GetUri().IsEmpty());
    }

    { /// Bad file path test
        FileAssetDataSource SpaceThumbnail;
        const std::string LocalFileName = "OKO.png";
        const auto FilePath = std::filesystem::absolute("assets/badpath/" + LocalFileName);
        SpaceThumbnail.FilePath = FilePath.u8string().c_str();
        SpaceThumbnail.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceThumbnail, RequestPredicate, Space.Id, SpaceThumbnail);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        FileAssetDataSource SpaceThumbnail;
        const std::string LocalFileName = "OKO.png";
        const auto FilePath = std::filesystem::absolute("assets/" + LocalFileName);
        SpaceThumbnail.FilePath = FilePath.u8string().c_str();
        SpaceThumbnail.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceThumbnail, RequestPredicate, Space.Id, SpaceThumbnail);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [GetThumbnailResult] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

        EXPECT_EQ(GetThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(GetThumbnailResult.GetUri().c_str(), LocalFileName));
    }

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACE_THUMBNAIL_WITH_BUFFER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceThumbnailWithBufferTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    // Create space without a thumbnail
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
        EXPECT_TRUE(Result.GetUri().IsEmpty());
    }

    auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
    FILE* UploadFile = nullptr;
    fopen_s(&UploadFile, UploadFilePath.string().c_str(), "rb");

    uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
    auto* UploadFileData = new unsigned char[UploadFileSize];
    fread(UploadFileData, UploadFileSize, 1, UploadFile);
    fclose(UploadFile);

    BufferAssetDataSource SpaceThumbnail;
    SpaceThumbnail.Buffer = UploadFileData;
    SpaceThumbnail.BufferLength = UploadFileSize;

    SpaceThumbnail.SetMimeType("image/png");

    auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceThumbnailWithBuffer, RequestPredicate, Space.Id, SpaceThumbnail);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetThumbnailResult] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);
    EXPECT_EQ(GetThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);
    printf("Downloading asset data...\n");

    // Get asset uri
    auto [uri_Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);
    ::Asset Asset;
    Asset.FileName = "test.json";
    Asset.Uri = uri_Result.GetUri();
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

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATE_SPACE_EMPTY_METADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceWithEmptyMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    ::Space Space;
    Map<String, String> Metadata;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, Metadata, nullptr, nullptr, nullptr, Space);

    Map<String, String> RetrievedSpaceMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);

    EXPECT_EQ(RetrievedSpaceMetadata.Size(), 0UL);

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACE_EMPTY_METADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceWithEmptyMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    UpdateAndAssertSpaceMetadata(SpaceSystem, Space.Id, nullptr, nullptr);

    Map<String, String> RetrievedSpaceMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);

    EXPECT_EQ(RetrievedSpaceMetadata.Size(), 0UL);

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

// - TODO - JQ - Rename this test to InviteUserToSpaceTest?
#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_PENDING_INVITES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPendingUserInvitesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // It is not possible to record pending invites and see them decrement as they are accepted,
    // because the invites are sent by email and have to be accepted by clicking a link in them.
    // The test suite does not have the capability to open emails and click links in them.
    // The workaround is to test each separately.
    // Using an email that is not associated to any existing account, only the pending invites counter increases (the accepted invites counter remains
    // at 0).

    // This test only works if the below email is not associated to any existing account.
    const char* TestUserEmail = "non-existing.account@magnopus.com";
    const char* TestEmailLinkUrl = "https://dev.magnoverse.space/";
    const char* TestSignupUrl = "https://dev.magnoverse.space/";

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Check that there are no pending invites before inviting a user
    auto [GetInvitesResultBefore] = AWAIT_PRE(SpaceSystem, GetPendingUserInvites, RequestPredicate, Space.Id);
    EXPECT_EQ(GetInvitesResultBefore.GetResultCode(), csp::systems::EResultCode::Success);
    auto& PendingInvitesBefore = GetInvitesResultBefore.GetPendingInvitesEmails();
    EXPECT_EQ(PendingInvitesBefore.Size(), 0);

    // Invite a user to the space
    auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, TestUserEmail, nullptr, TestEmailLinkUrl, TestSignupUrl);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Check that there is one pending invite after inviting a user
    auto [GetInvitesResult] = AWAIT_PRE(SpaceSystem, GetPendingUserInvites, RequestPredicate, Space.Id);
    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto& PendingInvites = GetInvitesResult.GetPendingInvitesEmails();
    EXPECT_EQ(PendingInvites.Size(), 1);
    for (auto idx = 0; idx < PendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << PendingInvites[idx] << std::endl;
    }

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_ACCEPTED_INVITES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetAcceptedUserInvitesTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    const char* TestEmailLinkUrl = "https://dev.magnoverse.space/";
    const char* TestSignupUrl = "https://dev.magnoverse.space/";

    // Create users
    String SpaceCreatorUserId;
    csp::systems::Profile SpaceCreatorUser = CreateTestUser();

    String User1Id;
    csp::systems::Profile User1 = CreateTestUser();
    csp::systems::Profile User2 = CreateTestUser();

    // It is not possible to record pending invites and see them decrement as they are accepted,
    // because the invites are sent by email and have to be accepted by clicking a link in them.
    // The test suite does not have the capability to open emails and click links in them.
    // The workaround is to test each separately.
    // Using an account that already exists, only the accepted invites counter increases (the pending invites counter remains at 0).
    // Note that all invites are accepted at once on the test tenant.

    // Log in as Space Creator and create space
    LogIn(UserSystem, SpaceCreatorUserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Check that there are no accepted invites before inviting users
    auto [GetAcceptedInvitesResultBefore] = AWAIT_PRE(SpaceSystem, GetAcceptedUserInvites, RequestPredicate, Space.Id);
    EXPECT_EQ(GetAcceptedInvitesResultBefore.GetResultCode(), csp::systems::EResultCode::Success);
    auto& AcceptedInvitesBefore = GetAcceptedInvitesResultBefore.GetAcceptedInvitesUserIds();
    EXPECT_EQ(AcceptedInvitesBefore.Size(), 0);

    // Invite User1 and User2 to the space
    auto [ResultInviteUser1]
        = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, User1.Email, nullptr, TestEmailLinkUrl, TestSignupUrl);
    EXPECT_EQ(ResultInviteUser1.GetResultCode(), csp::systems::EResultCode::Success);
    auto [ResultInviteUser2]
        = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, User2.Email, nullptr, TestEmailLinkUrl, TestSignupUrl);
    EXPECT_EQ(ResultInviteUser2.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out as Space Creator
    LogOut(UserSystem);

    // Log in as User1 and enter the space, which triggers invite acceptance on the test tenant (for all users, so including User2)
    LogIn(UserSystem, User1Id, User1.Email, GeneratedTestAccountPassword);

    auto [EnterSpaceResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    ASSERT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Log back in as Space Creator to check the accepted invites
    LogOut(UserSystem);
    LogIn(UserSystem, SpaceCreatorUserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);

    // Check the accepted invites are recorded correctly
    auto [GetAcceptedInvitesResult] = AWAIT_PRE(SpaceSystem, GetAcceptedUserInvites, RequestPredicate, Space.Id);

    EXPECT_EQ(GetAcceptedInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto& AcceptedInvites = GetAcceptedInvitesResult.GetAcceptedInvitesUserIds();
    EXPECT_EQ(AcceptedInvites.Size(), 2);
    for (auto idx = 0; idx < AcceptedInvites.Size(); ++idx)
    {
        std::cerr << "Accepted space invite for user id: " << AcceptedInvites[idx] << std::endl;
    }

    // Clean up
    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_BULK_INVITE_TO_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BulkInvitetoSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    auto InviteUsers = CreateInviteUsers();

    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [Result] = AWAIT_PRE(SpaceSystem, BulkInviteToSpace, RequestPredicate, Space.Id, InviteUsers);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetInvitesResult] = AWAIT_PRE(SpaceSystem, GetPendingUserInvites, RequestPredicate, Space.Id);

    EXPECT_EQ(GetInvitesResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto& PendingInvites = GetInvitesResult.GetPendingInvitesEmails();

    EXPECT_EQ(PendingInvites.Size(), 4);

    for (auto idx = 0; idx < PendingInvites.Size(); ++idx)
    {
        std::cerr << "Pending space invite for email: " << PendingInvites[idx] << std::endl;
    }

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPUBLICSPACEMETADATA_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetPublicSpaceMetadataTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    Map<String, String> TestSpaceMetadata = { { "site", "Void" } };

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Log in with default user
    csp::systems::Profile DefaultUser = CreateTestUser();
    LogIn(UserSystem, UserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Create public space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, TestSpaceMetadata, nullptr, nullptr, nullptr, Space);

    // Log out with default user and in with alt user
    LogOut(UserSystem);
    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    ASSERT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Get metadata for public space
    Map<String, String> RetrievedMetadata = GetAndAssertSpaceMetadata(SpaceSystem, Space.Id);

    ASSERT_EQ(RetrievedMetadata.Size(), TestSpaceMetadata.Size());
    ASSERT_TRUE(RetrievedMetadata.HasKey("site"));
    ASSERT_EQ(RetrievedMetadata["site"], TestSpaceMetadata["site"]);

    // Exit and re-enter space to verify its OK to always add self to public space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    {
        auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Log back in with default user so space can be deleted
    LogOut(UserSystem);
    LogIn(UserSystem, UserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_THUMBNAIL_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceThumbnailTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String PrimaryUserId;

    csp::systems::Profile PrimaryTestUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryTestUser.Email, GeneratedTestAccountPassword);

    ::Space Space;
    FileAssetDataSource SpaceThumbnail;
    const std::string LocalFileName = "test.json";
    const auto FilePath = std::filesystem::absolute("assets/" + LocalFileName);
    SpaceThumbnail.FilePath = FilePath.u8string().c_str();
    SpaceThumbnail.SetMimeType("application/json");

    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, SpaceThumbnail, nullptr, Space);

    String InitialSpaceThumbnailUri;
    {
        auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        InitialSpaceThumbnailUri = Result.GetUri();

        EXPECT_TRUE(IsUriValid(InitialSpaceThumbnailUri.c_str(), LocalFileName));
    }

    LogOut(UserSystem);

    // check that a user that doesn't belong to the space can retrieve the thumbnail
    String SecondaryUserId;
    csp::systems::Profile SecondaryTestUser = CreateTestUser();

    LogIn(UserSystem, SecondaryUserId, SecondaryTestUser.Email, GeneratedTestAccountPassword);

    {
        auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(InitialSpaceThumbnailUri, Result.GetUri());
    }

    LogOut(UserSystem);

    LogIn(UserSystem, PrimaryUserId, PrimaryTestUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_THUMBNAIL_WITH_GUEST_USER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceThumbnailWithGuestUserTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    ::Space Space;
    FileAssetDataSource SpaceThumbnail;
    const std::string LocalFileName = "test.json";
    auto FilePath = std::filesystem::absolute("assets/" + LocalFileName);
    SpaceThumbnail.FilePath = FilePath.u8string().c_str();
    SpaceThumbnail.SetMimeType("application/json");

    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, SpaceThumbnail, nullptr, Space);

    LogOut(UserSystem);

    String GuestId;
    LogInAsGuest(UserSystem, GuestId);

    FileAssetDataSource UpdatedSpaceThumbnail;
    FilePath = std::filesystem::absolute("assets/Fox.glb");
    UpdatedSpaceThumbnail.FilePath = FilePath.u8string().c_str();
    UpdatedSpaceThumbnail.SetMimeType("model/gltf-binary");

    {
        // A guest shouldn't be able to update the space thumbnail
        auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceThumbnail, RequestPredicate, Space.Id, UpdatedSpaceThumbnail);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        // But it should be able to retrieve it
        auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(IsUriValid(Result.GetUri().c_str(), LocalFileName));
    }

    LogOut(UserSystem);

    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_BAN_GUESTUSER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BanGuestUserTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Login with first user to create space
    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

    LogOut(UserSystem);

    // Login with second user and join space
    String GuestId;
    LogInAsGuest(UserSystem, GuestId);

    auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, Space.Id, GuestId);

    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(UserSystem);

    // Login again with first user to ban second user
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    GetSpace(SpaceSystem, Space.Id, Space);

    {
        auto [Result] = AWAIT_PRE(SpaceSystem, AddUserToSpaceBanList, RequestPredicate, Space.Id, GuestId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(SpaceSystem, Space.Id, Space);

        EXPECT_FALSE(Space.BannedUserIds.IsEmpty());
        EXPECT_EQ(Space.BannedUserIds[0], GuestId);
    }

    {
        auto [Result] = AWAIT_PRE(SpaceSystem, DeleteUserFromSpaceBanList, RequestPredicate, Space.Id, GuestId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(SpaceSystem, Space.Id, Space);

        EXPECT_TRUE(Space.BannedUserIds.IsEmpty());
    }

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_BAN_USER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BanUserTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Login with first user to create space
    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

    LogOut(UserSystem);

    // Login with second user and join space
    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, Space.Id, AltUserId);

    EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(UserSystem);

    // Login again with first user to ban second user
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    GetSpace(SpaceSystem, Space.Id, Space);

    {
        auto [Result] = AWAIT_PRE(SpaceSystem, AddUserToSpaceBanList, RequestPredicate, Space.Id, AltUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(SpaceSystem, Space.Id, Space);

        EXPECT_FALSE(Space.BannedUserIds.IsEmpty());
        EXPECT_EQ(Space.BannedUserIds[0], AltUserId);
    }
    {
        auto [Result] = AWAIT_PRE(SpaceSystem, DeleteUserFromSpaceBanList, RequestPredicate, Space.Id, AltUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        GetSpace(SpaceSystem, Space.Id, Space);

        EXPECT_TRUE(Space.BannedUserIds.IsEmpty());
    }

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        EXPECT_FALSE(SpaceSystem->IsInSpace());

        auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_TRUE(SpaceSystem->IsInSpace());

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        EXPECT_FALSE(SpaceSystem->IsInSpace());
    }

    LogOut(UserSystem);

    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    {
        auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(UserSystem);

    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_ASNONMODERATOR_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceAsNonModeratorTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);
    LogOut(UserSystem);

    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);
    LogOut(UserSystem);

    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    {
        auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(UserSystem);

    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_ASMODERATOR_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceAsModeratorTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);
    LogOut(UserSystem);

    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);
    ::Space Space;
    InviteUserRoleInfo InviteUser;
    InviteUser.UserEmail = AltUser.Email;
    InviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser };
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, nullptr, nullptr, Space);

    ::UserRoleInfo NewUserRoleInfo;
    NewUserRoleInfo.UserId = AltUserId;
    NewUserRoleInfo.UserRole = SpaceUserRole::Moderator;

    UpdateUserRole(SpaceSystem, Space.Id, NewUserRoleInfo);

    LogOut(UserSystem);

    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    // Note the space is now out of date and does not have the new user in its lists
    {
        auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    LogOut(UserSystem);

    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String PrimaryUserId;
    LogInAsNewTestUser(UserSystem, PrimaryUserId);
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    GeoLocation InitialGeoLocation;
    InitialGeoLocation.Latitude = 1.1;
    InitialGeoLocation.Longitude = 2.2;

    float InitialOrientation = 90.0f;

    csp::common::Array<GeoLocation> InitialGeoFence(4);

    GeoLocation GeoFence0;
    GeoFence0.Latitude = 5.5;
    GeoFence0.Longitude = 6.6;
    InitialGeoFence[0] = GeoFence0;
    InitialGeoFence[3] = GeoFence0;

    GeoLocation GeoFence1;
    GeoFence1.Latitude = 7.7;
    GeoFence1.Longitude = 8.8;
    InitialGeoFence[1] = GeoFence1;

    GeoLocation GeoFence2;
    GeoFence2.Latitude = 9.9;
    GeoFence2.Longitude = 10.0;
    InitialGeoFence[2] = GeoFence2;

    auto [AddGeoResult]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, InitialGeoFence);

    EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(AddGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().Location.Latitude, InitialGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().Location.Longitude, InitialGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().Orientation, InitialOrientation);

    for (auto i = 0; i < AddGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
    {
        EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, InitialGeoFence[i].Latitude);
        EXPECT_DOUBLE_EQ(AddGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, InitialGeoFence[i].Longitude);
    }

    auto [GetGeoResult] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(GetGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(GetGeoResult.GetSpaceGeoLocation().Location.Latitude, InitialGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(GetGeoResult.GetSpaceGeoLocation().Location.Longitude, InitialGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(GetGeoResult.GetSpaceGeoLocation().Orientation, InitialOrientation);

    GeoLocation SecondGeoLocation;
    SecondGeoLocation.Latitude = 3.3;
    SecondGeoLocation.Longitude = 4.4;

    float SecondOrientation = 270.0f;

    csp::common::Array<GeoLocation> SecondGeoFence(4);
    GeoFence0.Latitude = 11.1;
    GeoFence0.Longitude = 12.2;
    SecondGeoFence[0] = GeoFence0;
    SecondGeoFence[3] = GeoFence0;
    GeoFence1.Latitude = 13.3;
    GeoFence1.Longitude = 14.4;
    SecondGeoFence[1] = GeoFence1;
    GeoFence2.Latitude = 15.5;
    GeoFence2.Longitude = 16.6;
    SecondGeoFence[2] = GeoFence2;

    auto [UpdateGeoResult]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, SecondGeoLocation, SecondOrientation, SecondGeoFence);

    EXPECT_EQ(UpdateGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(UpdateGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().Location.Latitude, SecondGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().Location.Longitude, SecondGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().Orientation, SecondOrientation);

    for (auto i = 0; i < UpdateGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
    {
        EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, SecondGeoFence[i].Latitude);
        EXPECT_DOUBLE_EQ(UpdateGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, SecondGeoFence[i].Longitude);
    }

    auto [GetUpdatedGeoResult] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetUpdatedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(GetUpdatedGeoResult.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().Location.Latitude, SecondGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().Location.Longitude, SecondGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().Orientation, SecondOrientation);

    for (auto i = 0; i < GetUpdatedGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
    {
        EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, SecondGeoFence[i].Latitude);
        EXPECT_DOUBLE_EQ(GetUpdatedGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, SecondGeoFence[i].Longitude);
    }

    auto [DeleteGeoResult] = AWAIT_PRE(SpaceSystem, DeleteSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(DeleteGeoResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetDeletedGeoResult] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetDeletedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(GetDeletedGeoResult.HasSpaceGeoLocation());

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_VALIDATION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationValidationTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String PrimaryUserId;
    LogInAsNewTestUser(UserSystem, PrimaryUserId);
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    GeoLocation ValidGeoLocation;
    ValidGeoLocation.Latitude = 1.1;
    ValidGeoLocation.Longitude = 2.2;

    GeoLocation InvalidGeoLocation;
    InvalidGeoLocation.Latitude = 500.0;
    InvalidGeoLocation.Longitude = 2.2;

    float ValidOrientation = 90.0f;
    float InvalidOrientation = 500.0f;

    csp::common::Array<GeoLocation> ValidGeoFence(4);
    csp::common::Array<GeoLocation> ShortGeoFence(2);
    csp::common::Array<GeoLocation> InvalidGeoFence(4);
    csp::common::Array<GeoLocation> InvalidGeoLocationGeoFence(4);
    GeoLocation GeoFence0;
    GeoFence0.Latitude = 5.5;
    GeoFence0.Longitude = 6.6;
    GeoLocation GeoFence1;
    GeoFence1.Latitude = 7.7;
    GeoFence1.Longitude = 8.8;
    GeoLocation GeoFence2;
    GeoFence2.Latitude = 9.9;
    GeoFence2.Longitude = 10.0;

    ValidGeoFence[0] = GeoFence0;
    ValidGeoFence[1] = GeoFence1;
    ValidGeoFence[2] = GeoFence2;
    ValidGeoFence[3] = GeoFence0;

    ShortGeoFence[0] = GeoFence0;
    ShortGeoFence[1] = GeoFence2;

    InvalidGeoFence[0] = GeoFence0;
    InvalidGeoFence[1] = GeoFence1;
    InvalidGeoFence[2] = GeoFence2;
    InvalidGeoFence[3] = GeoFence2;

    InvalidGeoLocationGeoFence[0] = GeoFence0;
    InvalidGeoLocationGeoFence[1] = GeoFence1;
    InvalidGeoLocationGeoFence[2] = InvalidGeoLocation;
    InvalidGeoLocationGeoFence[3] = GeoFence0;

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InvalidGeoLocation, ValidOrientation, ValidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, InvalidOrientation, ValidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, ShortGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, InvalidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult] = AWAIT_PRE(
            SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, InvalidGeoLocationGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Actually add a geo location and test again since a different code path is followed when one exists
    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, ValidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InvalidGeoLocation, ValidOrientation, ValidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, InvalidOrientation, ValidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, ShortGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, InvalidGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [AddGeoResult] = AWAIT_PRE(
            SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, ValidGeoLocation, ValidOrientation, InvalidGeoLocationGeoFence);

        EXPECT_EQ(AddGeoResult.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    {
        auto [DeleteGeoResult] = AWAIT_PRE(SpaceSystem, DeleteSpaceGeoLocation, RequestPredicate, Space.Id);

        EXPECT_EQ(DeleteGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_WITHOUT_PERMISSION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationWithoutPermissionTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Create a space as the primary user
    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Switch to the alt user to try and update the geo location
    LogOut(UserSystem);
    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    GeoLocation InitialGeoLocation;
    InitialGeoLocation.Latitude = 1.1;
    InitialGeoLocation.Longitude = 2.2;

    float InitialOrientation = 90.0f;

    auto [AddGeoResultAsAlt]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(AddGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Switch back to the primary user to actually create the geo location
    LogOut(UserSystem);
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    auto [AddGeoResultAsPrimary]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    // Switch back to the alt user again
    LogOut(UserSystem);
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    // Test they cannot get the space geo location details since the space is private
    auto [GetGeoResultAsAlt] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(GetGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Test they cannot update the geolocation
    GeoLocation SecondGeoLocation;
    SecondGeoLocation.Latitude = 3.3;
    SecondGeoLocation.Longitude = 4.4;

    float SecondOrientation = 270.0f;

    auto [UpdateGeoResultAsAlt]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, SecondGeoLocation, SecondOrientation, nullptr);

    EXPECT_EQ(UpdateGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdateGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Test they cannot delete the geo location
    auto [DeleteGeoResultAsAlt] = AWAIT_PRE(SpaceSystem, DeleteSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(DeleteGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(DeleteGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Log back in as primary to clean up
    LogOut(UserSystem);
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    auto [DeleteGeoResultAsPrimary] = AWAIT_PRE(SpaceSystem, DeleteSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(DeleteGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetDeletedGeoResult] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetDeletedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(GetDeletedGeoResult.HasSpaceGeoLocation());

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_WITHOUT_PERMISSION_PUBLIC_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationWithoutPermissionPublicSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Create a space as the primary user
    String PrimaryUserId;
    csp::systems::Profile PrimaryUser = CreateTestUser();
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

    // Switch to the alt user to try and update the geo location
    LogOut(UserSystem);
    String AltUserId;
    csp::systems::Profile AltUser = CreateTestUser();
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    GeoLocation InitialGeoLocation;
    InitialGeoLocation.Latitude = 1.1;
    InitialGeoLocation.Longitude = 2.2;

    float InitialOrientation = 90.0f;

    auto [AddGeoResultAsAlt]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(AddGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Switch back to the primary user to actually create the geo location
    LogOut(UserSystem);
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    auto [AddGeoResultAsPrimary]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

    EXPECT_EQ(AddGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    // Switch back to the alt user again
    LogOut(UserSystem);
    LogIn(UserSystem, AltUserId, AltUser.Email, GeneratedTestAccountPassword);

    // Test they can get the space geo location details since the space is public
    auto [GetGeoResultAsAlt] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(GetGeoResultAsAlt.HasSpaceGeoLocation());
    EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Location.Latitude, InitialGeoLocation.Latitude);
    EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Location.Longitude, InitialGeoLocation.Longitude);
    EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Orientation, InitialOrientation);

    // Test they cannot update the geolocation
    GeoLocation SecondGeoLocation;
    SecondGeoLocation.Latitude = 3.3;
    SecondGeoLocation.Longitude = 4.4;

    float SecondOrientation = 270.0f;

    auto [UpdateGeoResultAsAlt]
        = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, SecondGeoLocation, SecondOrientation, nullptr);

    EXPECT_EQ(UpdateGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdateGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Test they cannot delete the geo location
    auto [DeleteGeoResultAsAlt] = AWAIT_PRE(SpaceSystem, DeleteSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(DeleteGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(DeleteGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

    // Log back in as primary to clean up
    LogOut(UserSystem);
    LogIn(UserSystem, PrimaryUserId, PrimaryUser.Email, GeneratedTestAccountPassword);

    auto [DeleteGeoResultAsPrimary] = AWAIT_PRE(SpaceSystem, DeleteSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(DeleteGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

    auto [GetDeletedGeoResult] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

    EXPECT_EQ(GetDeletedGeoResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(GetDeletedGeoResult.HasSpaceGeoLocation());

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_DUPLICATESPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, DuplicateSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-TEST-SPACE";
    const char* TestSpaceDescription = "CSP-TEST-SPACEDESC";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    String UserId;

    // Create default and alt users
    csp::systems::Profile DefaultUser = CreateTestUser();
    csp::systems::Profile AlternativeUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, UserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Create space
    Array<InviteUserRoleInfo> UserRoles(1);
    UserRoles[0].UserEmail = AlternativeUser.Email;
    UserRoles[0].UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection InviteInfo;
    InviteInfo.InviteUserRoleInfos = UserRoles;

    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteInfo, nullptr, nullptr, Space);

    // Log out and log in as alt user
    LogOut(UserSystem);
    LogIn(UserSystem, UserId, AlternativeUser.Email, GeneratedTestAccountPassword);

    // Attempt to duplicate space
    {
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        auto [Result] = AWAIT_PRE(SpaceSystem, DuplicateSpace, RequestPredicate, Space.Id, UniqueSpaceName, SpaceAttributes::Private, nullptr, true);

        EXPECT_EQ(Result.GetResultCode(), EResultCode::Success);

        const auto& NewSpace = Result.GetSpace();

        EXPECT_NE(NewSpace.Id, Space.Id);
        EXPECT_EQ(NewSpace.Name, UniqueSpaceName);
        EXPECT_EQ(NewSpace.Description, Space.Description);
        EXPECT_EQ(NewSpace.Attributes, SpaceAttributes::Private);
        EXPECT_EQ(NewSpace.OwnerId, UserId);
        EXPECT_NE(Space.OwnerId, UserId);

        // Delete duplicated space
        DeleteSpace(SpaceSystem, NewSpace.Id);
    }

    // Log out and log in as default user to clean up original space
    LogOut(UserSystem);
    LogIn(UserSystem, UserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_PERMISSIONS_MATRIX_TESTS
namespace CSPEngine
{
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
    const SpaceAttributes SpacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode JoinSpaceResultExpected = std::get<1>(GetParam());
    const std::string ExpectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    std::string UniqueSpaceName = TestSpaceName + std::string("-") + UUIDv4::UUIDGenerator<std::mt19937_64>().getUUID().str();

    // Create a space according to param attribute
    String SpaceOwnerUserId;
    csp::systems::Profile SpaceOwnerUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space CreatedSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName.c_str(), TestSpaceDescription, SpacePermission, nullptr, nullptr, nullptr, nullptr, CreatedSpace);
    LogOut(UserSystem);

    // Log in as guest
    String GuestUserId;
    LogInAsGuest(UserSystem, GuestUserId);

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, CreatedSpace.Id);
    ASSERT_EQ(EnterResult.GetResultCode(), JoinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string OutStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(OutStdErr.find(ExpectedMsg), std::string::npos);

    // Log out
    LogOut(UserSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, CreatedSpace.Id);
    LogOut(UserSystem);
}

TEST_P(EnterSpaceWhenUninvited, EnterSpaceWhenUninvitedTest)
{
    // Conditions & Expectations
    const SpaceAttributes SpacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode JoinSpaceResultExpected = std::get<1>(GetParam());
    const std::string ExpectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    std::string UniqueSpaceName = TestSpaceName + std::string("-") + UUIDv4::UUIDGenerator<std::mt19937_64>().getUUID().str();

    // Create a space according to param attribute
    String SpaceOwnerUserId;
    csp::systems::Profile SpaceOwnerUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space CreatedSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName.c_str(), TestSpaceDescription, SpacePermission, nullptr, nullptr, nullptr, nullptr, CreatedSpace);
    LogOut(UserSystem);

    // Log in as another user who isn't invited
    String UninvitedUserId;
    csp::systems::Profile UninvitedUser = CreateTestUser();
    LogIn(UserSystem, UninvitedUserId, UninvitedUser.Email, GeneratedTestAccountPassword);

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, CreatedSpace.Id);
    ASSERT_EQ(EnterResult.GetResultCode(), JoinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string OutStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(OutStdErr.find(ExpectedMsg), std::string::npos);

    // Log out
    LogOut(UserSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, CreatedSpace.Id);
    LogOut(UserSystem);
}

TEST_P(EnterSpaceWhenInvited, EnterSpaceWhenInvitedTest)
{
    // Conditions & Expectations
    const SpaceAttributes SpacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode JoinSpaceResultExpected = std::get<1>(GetParam());
    const std::string ExpectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    std::string UniqueSpaceName = TestSpaceName + std::string("-") + UUIDv4::UUIDGenerator<std::mt19937_64>().getUUID().str();

    // Create a space according to param attribute, and invite a user
    csp::systems::Profile InvitedUser = CreateTestUser();

    InviteUserRoleInfo InviteUser;
    InviteUser.UserEmail = InvitedUser.Email;
    InviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser };

    String SpaceOwnerUserId;
    csp::systems::Profile SpaceOwnerUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space CreatedSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName.c_str(), TestSpaceDescription, SpacePermission, nullptr, InviteUsers, nullptr, nullptr, CreatedSpace);
    LogOut(UserSystem);

    // Log in as invited user
    String InvitedUserId;
    LogIn(UserSystem, InvitedUserId, InvitedUser.Email, GeneratedTestAccountPassword);

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, CreatedSpace.Id);
    ASSERT_EQ(EnterResult.GetResultCode(), JoinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string OutStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(OutStdErr.find(ExpectedMsg), std::string::npos);

    // Log out
    LogOut(UserSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, CreatedSpace.Id);
    LogOut(UserSystem);
}

TEST_P(EnterSpaceWhenCreator, EnterSpaceWhenCreatorTest)
{
    // Conditions & Expectations
    const SpaceAttributes SpacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode JoinSpaceResultExpected = std::get<1>(GetParam());
    const std::string ExpectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    std::string UniqueSpaceName = TestSpaceName + std::string("-") + UUIDv4::UUIDGenerator<std::mt19937_64>().getUUID().str();

    // Create a space according to param attribute
    String SpaceOwnerUserId;
    csp::systems::Profile SpaceOwnerUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space CreatedSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName.c_str(), TestSpaceDescription, SpacePermission, nullptr, nullptr, nullptr, nullptr, CreatedSpace);

    // Attempt to enter the space and check the expected result
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, CreatedSpace.Id);
    ASSERT_EQ(EnterResult.GetResultCode(), JoinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string OutStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(OutStdErr.find(ExpectedMsg), std::string::npos);

    // Delete test space
    DeleteSpace(SpaceSystem, CreatedSpace.Id);
    LogOut(UserSystem);
}

TEST_P(EnterSpaceWhenBanned, EnterSpaceWhenBannedTest)
{
    // Conditions & Expectations
    const SpaceAttributes SpacePermission = std::get<0>(GetParam());
    const csp::systems::EResultCode JoinSpaceResultExpected = std::get<1>(GetParam());
    const std::string ExpectedMsg = std::get<2>(GetParam());

    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    std::string UniqueSpaceName = TestSpaceName + std::string("-") + UUIDv4::UUIDGenerator<std::mt19937_64>().getUUID().str();

    // Create a space according to param attribute, and ban a user
    csp::systems::Profile BannedUser = CreateTestUser();

    // Invite the banned user, to make sure that bans apply even if invited.
    InviteUserRoleInfo InviteUser;
    InviteUser.UserEmail = BannedUser.Email;
    InviteUser.UserRole = SpaceUserRole::User;
    InviteUserRoleInfoCollection InviteUsers;
    InviteUsers.InviteUserRoleInfos = { InviteUser };

    String SpaceOwnerUserId;
    csp::systems::Profile SpaceOwnerUser = CreateTestUser();
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    Space CreatedSpace;
    CreateSpace(SpaceSystem, UniqueSpaceName.c_str(), TestSpaceDescription, SpacePermission, nullptr, InviteUsers, nullptr, nullptr, CreatedSpace);
    LogOut(UserSystem);

    // Log in as banned user
    String BannedUserId;
    LogIn(UserSystem, BannedUserId, BannedUser.Email, GeneratedTestAccountPassword);
    // In order to ban the user, they have to have entered the space. (This seems like an underthought limitation)
    auto [EnterSpaceResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, CreatedSpace.Id);
    ASSERT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    ASSERT_EQ(ExitSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    LogOut(UserSystem);

    // Log back in as owner and ban the user
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    auto [Result] = AWAIT_PRE(SpaceSystem, AddUserToSpaceBanList, RequestPredicate, CreatedSpace.Id, BannedUser.UserId);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    LogOut(UserSystem);

    // Login as the banned user and attempt to enter the space and check the expected result
    LogIn(UserSystem, BannedUserId, BannedUser.Email, GeneratedTestAccountPassword);
    testing::internal::CaptureStderr();
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, CreatedSpace.Id);
    ASSERT_EQ(EnterResult.GetResultCode(), JoinSpaceResultExpected);

    // Verify that Stderr contains expected message.
    std::string OutStdErr = testing::internal::GetCapturedStderr();
    EXPECT_NE(OutStdErr.find(ExpectedMsg), std::string::npos);

    // Log out
    LogOut(UserSystem);

    // Login as owner user in order to be able to delete the test space
    LogIn(UserSystem, SpaceOwnerUserId, SpaceOwnerUser.Email, GeneratedTestAccountPassword);
    DeleteSpace(SpaceSystem, CreatedSpace.Id);
    LogOut(UserSystem);
}

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
}
#endif