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
#include "CSP/Systems/Organizations/OrganizationSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "Debug/Logging.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <PublicAPITests/AssetSystemTestHelpers.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace csp::common;
using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void GetUsersRoles(::OrganizationSystem* OrganizationSystem, const csp::common::Optional<csp::common::String>& OrganizationId,
    const csp::common::Array<csp::common::String>& UserIds, csp::common::Array<OrganizationRoleInfo>& OutOrganizationRoleInfo)
{
    auto [Result] = AWAIT_PRE(OrganizationSystem, GetUserRolesInOrganization, RequestPredicate, OrganizationId, UserIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutOrganizationRoleInfo = Result.GetOrganizationRoleInfo();
}

InviteOrganizationRoleCollection CreateOrganizationInvites(const String& EmailUser1, const String& EmailUser2)
{
    InviteOrganizationRoleInfo InviteUser1;
    InviteUser1.UserEmail = EmailUser1;
    InviteUser1.OrganizationRoles = { EOrganizationRole::Member };

    InviteOrganizationRoleInfo InviteUser2;
    InviteUser2.UserEmail = EmailUser2;
    InviteUser2.OrganizationRoles = { EOrganizationRole::Member };

    InviteOrganizationRoleCollection OrganizationInvites;
    OrganizationInvites.EmailLinkUrl = "https://dev.magnoverse.space";
    OrganizationInvites.SignupUrl = "https://dev.magnoverse.space";
    OrganizationInvites.InvitedUserRoles = { InviteUser1, InviteUser2 };

    return OrganizationInvites;
}

void CleanupTestUser(const String& UserId)
{
    // Only the super user has the required privileges to remove users.
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Delete the test user
    auto [DeleteDefaultUserResult] = AWAIT_PRE(UserSystem, DeleteUser, RequestPredicate, UserId);
    EXPECT_EQ(DeleteDefaultUserResult.GetResultCode(), csp::systems::EResultCode::Success);
}

Organization CreateTestOrganization(const String& OrgOwnerId)
{
    auto& SystemsManager = ::SystemsManager::Get();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    const char* TestOrganizationName = "CSP-TEST-ORG";

    char UniqueOrgName[256];
    SPRINTF(UniqueOrgName, "%s-%s", TestOrganizationName, GetUniqueString().c_str());

    // Create new organization with the created user as the Organization Owner
    auto [CreateOrgResult] = AWAIT_PRE(OrganizationSystem, CreateOrganization, RequestPredicate, OrgOwnerId, UniqueOrgName);
    EXPECT_EQ(CreateOrgResult.GetResultCode(), csp::systems::EResultCode::Success);

    return CreateOrgResult.GetOrganization();
}

void CleanupTestOrgization(const String& OrganizationId)
{
    auto& SystemsManager = ::SystemsManager::Get();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    // Delete the Organization
    auto [DeleteOrgResult] = AWAIT_PRE(OrganizationSystem, DeactivateOrganization, RequestPredicate, OrganizationId);
    EXPECT_EQ(DeleteOrgResult.GetResultCode(), csp::systems::EResultCode::Success);
}

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    csp::common::Array<EOrganizationRole> AltUserRoles { EOrganizationRole::Member, EOrganizationRole::Administrator };
    csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
    csp::common::String SignupUrl = "https://dev.magnoverse.space";

    // Invite non-member user to the Organization
    auto [InviteResult] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, TestOrganization.Id, TestDefaultUserProfile.Email,
        AltUserRoles, EmailLinkUrl, SignupUrl);
    EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Confirm that non-member user now has the correct roles in Organization
    csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
    GetUsersRoles(OrganizationSystem, nullptr, { TestDefaultUserProfile.UserId }, UserOrganizationRoleInfo);

    EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

    // remove user from organization
    auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, TestDefaultUserProfile.UserId);
    EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

    CleanupTestOrgization(TestOrganization.Id);

    // Log out - Admin Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_WITHOUT_MEMBER_ROLE_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationWithoutMemberRoleTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // Member role has intentionally been omitted. All users must have member role and so CSP it will automatically add it
    csp::common::Array<EOrganizationRole> AltUserRoles { EOrganizationRole::Administrator };
    csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
    csp::common::String SignupUrl = "https://dev.magnoverse.space";

    // Invite non-member user to the Organization
    auto [InviteResult] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, TestOrganization.Id, TestDefaultUserProfile.Email,
        AltUserRoles, EmailLinkUrl, SignupUrl);
    EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Confirm that non-member user now has the correct roles in Organization
    csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
    GetUsersRoles(OrganizationSystem, nullptr, { TestDefaultUserProfile.UserId }, UserOrganizationRoleInfo);

    EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

    // remove user from organization
    auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, TestDefaultUserProfile.UserId);
    EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

    CleanupTestOrgization(TestOrganization.Id);

    // Log out - Admin Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_WITHOUT_PERMISSIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationWithoutPermissionsTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();
    Profile TestAltUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // invite default with member role.
    csp::common::Array<EOrganizationRole> DefaultUserRoles { EOrganizationRole::Member };
    csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
    csp::common::String SignupUrl = "https://dev.magnoverse.space";

    // Invite non-member user to the Organization
    auto [InviteResult] = AWAIT_PRE(
        OrganizationSystem, InviteToOrganization, RequestPredicate, nullptr, TestDefaultUserProfile.Email, DefaultUserRoles, EmailLinkUrl, SignupUrl);
    EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out admin
    LogOut(UserSystem);

    // Log in default test user who is only a member role in the Organization
    String DefaultLoginId;
    LogIn(UserSystem, DefaultLoginId, TestDefaultUserProfile.Email, GeneratedTestAccountPassword);

    // Define organization roles for the new user.
    csp::common::Array<EOrganizationRole> AltUserRoles { EOrganizationRole::Member, EOrganizationRole::Administrator };

    // A user with only the member role should not be able to invite other users to the Organization.
    auto [Result] = AWAIT_PRE(
        OrganizationSystem, InviteToOrganization, RequestPredicate, nullptr, TestAltUserProfile.Email, AltUserRoles, EmailLinkUrl, SignupUrl);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    // remove user from organization
    auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, TestDefaultUserProfile.UserId);
    EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the alt test user
    CleanupTestUser(TestAltUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_INVALID_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToInvalidOrganizationTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    const char* Invalid_Organization_Id = "invalid";

    csp::common::Array<EOrganizationRole> AltUserRoles { EOrganizationRole::Member, EOrganizationRole::Administrator };
    csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
    csp::common::String SignupUrl = "https://dev.magnoverse.space";

    // Invite non-member user to the Organization
    auto [Result] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, Invalid_Organization_Id, TestDefaultUserProfile.Email,
        AltUserRoles, EmailLinkUrl, SignupUrl);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    CleanupTestOrgization(TestOrganization.Id);

    // Log out - Admin Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_BULK_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, BulkInviteToOrganizationTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();
    Profile TestAltUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    auto OrganizationInvites = CreateOrganizationInvites(TestDefaultUserProfile.Email, TestAltUserProfile.Email);

    // Invite non-member users to the Organization
    auto [Result] = AWAIT_PRE(OrganizationSystem, BulkInviteToOrganization, RequestPredicate, nullptr, OrganizationInvites);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Confirm that non-member users now have the correct roles in Organization
    ::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
    GetUsersRoles(OrganizationSystem, nullptr, { TestDefaultUserProfile.UserId, TestAltUserProfile.UserId }, UserOrganizationRoleInfo);

    EXPECT_EQ(UserOrganizationRoleInfo.Size(), 2);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 1);
    EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
    EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles.Size(), 1);
    EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles[0], EOrganizationRole::Member);

    // remove user1 from organization
    auto [RemoveUser1Result] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, TestDefaultUserProfile.UserId);
    EXPECT_EQ(RemoveUser1Result.GetResultCode(), csp::systems::EResultCode::Success);

    // remove user2 from organization
    auto [RemoveUser2Result] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, TestAltUserProfile.UserId);
    EXPECT_EQ(RemoveUser2Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the alt test user
    CleanupTestUser(TestAltUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_CREATE_ORGANISATION_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, CreateOrganisationSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    Profile TestAdminUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Get the Id of the Organization the user is authenticated against. Users can currently only
    // belong to a single Organization so we just use the first one.
    auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
    EXPECT_EQ(OrganizationIds.Size(), 1);

    auto& Oko_Tests_OrganizationId = OrganizationIds[0];

    EXPECT_EQ(Space.OrganizationId, Oko_Tests_OrganizationId);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_ORGANIZATIONID_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, OrganizationIdTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
    const char* TestAssetName = "OLY-UNITTEST-ASSET-REWIND";
    const char* TestThirdPartyReferenceId = "OLY-UNITTEST-ASSET-THIRDPARTY";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    Profile TestAdminUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    csp::common::String UserId;

    // Log in
    LogIn(UserSystem, UserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // Get the Id of the Organization the user is authenticated against. Users can currently only
    // belong to a single Organization so we just use the first one.
    auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
    EXPECT_EQ(OrganizationIds.Size(), 1);

    auto& Oko_Tests_OrganizationId = OrganizationIds[0];

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create asset collection
    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

    EXPECT_EQ(AssetCollection.OrganizationId, Oko_Tests_OrganizationId);

    // Delete asset collection
    DeleteAssetCollection(AssetSystem, AssetCollection);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_WITH_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationWithIdTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // Get the Id of the Organization the user is authenticated against. Users can currently only
    // belong to a single Organization so we just use the first one.
    auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
    EXPECT_EQ(OrganizationIds.Size(), 1);

    auto& Oko_Tests_OrganizationId = OrganizationIds[0];

    // Get the specified Organization.
    auto [Result] = AWAIT_PRE(OrganizationSystem, GetOrganization, RequestPredicate, Oko_Tests_OrganizationId);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& Organization = Result.GetOrganization();

    // Confirm Organization has the correct Id
    EXPECT_EQ(Organization.Id, Oko_Tests_OrganizationId);
    // If an admin retrieves the Organization object it should contain info on all members
    EXPECT_TRUE(Organization.Members.Size() > 0);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_WITH_NO_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationWithNoIdTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // Get the Organization - by specifying no Id, we should retrieve the active organization.
    auto [Result] = AWAIT_PRE(OrganizationSystem, GetOrganization, RequestPredicate, nullptr);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& Organization = Result.GetOrganization();

    // Get the Id of the Organization the user is authenticated against. Users can currently only
    // belong to a single Organization so we just use the first one.
    auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
    EXPECT_EQ(OrganizationIds.Size(), 1);

    auto& Oko_Tests_OrganizationId = OrganizationIds[0];

    // Confirm Organization has the correct Id
    EXPECT_EQ(Organization.Id, Oko_Tests_OrganizationId);
    // If a member retrieves the Organization object it should contain info on all members
    EXPECT_TRUE(Organization.Members.Size() > 0);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_AS_MEMBER_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationAsMemberTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // invite default with member role.
    csp::common::Array<EOrganizationRole> DefaultUserRoles { EOrganizationRole::Member };
    csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
    csp::common::String SignupUrl = "https://dev.magnoverse.space";

    // Invite non-member user to the Organization
    auto [InviteResult] = AWAIT_PRE(
        OrganizationSystem, InviteToOrganization, RequestPredicate, nullptr, TestDefaultUserProfile.Email, DefaultUserRoles, EmailLinkUrl, SignupUrl);
    EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out admin
    LogOut(UserSystem);

    // Log in default user who is only a member role in the Organization
    String DefaultLoginId;
    LogIn(UserSystem, DefaultLoginId, TestDefaultUserProfile.Email, GeneratedTestAccountPassword);

    // Get the Id of the Organization the user is authenticated against. Users can currently only
    // belong to a single Organization so we just use the first one.
    auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
    EXPECT_EQ(OrganizationIds.Size(), 1);

    auto& Oko_Tests_OrganizationId = OrganizationIds[0];

    // Get the specified Organization.
    auto [GetOrgResult] = AWAIT_PRE(OrganizationSystem, GetOrganization, RequestPredicate, Oko_Tests_OrganizationId);
    EXPECT_EQ(GetOrgResult.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& Organization = GetOrgResult.GetOrganization();

    // Confirm Organization has the correct Id
    EXPECT_EQ(Organization.Id, Oko_Tests_OrganizationId);
    // If a member retrieves the Organization object it should contain info on only 1 member (them)
    EXPECT_EQ(Organization.Members.Size(), 2);

    // remove default user from organization
    auto [RemoveResult]
        = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, Oko_Tests_OrganizationId, TestDefaultUserProfile.UserId);
    EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_ORGANIZATION_CRUD_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, OrganizationCRUDTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

    const char* TestOrganizationName = "CSP-TEST-ORG";

    char UpdatedOrgName[256];
    SPRINTF(UpdatedOrgName, "%s-%s", TestOrganizationName, GetUniqueString().c_str());

    Profile TestAdminUserProfile = CreateTestUser();
    Profile TestDefaultUserProfile = CreateTestUser();

    // log in as super user - The super user has the required privileges to create an organization.
    String SuperUserId;
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    Organization TestOrganization = CreateTestOrganization(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);

    // log in as test user - The test user has the required privileges to invite people to an organization.
    String TestUserId;
    LogIn(UserSystem, TestUserId, TestAdminUserProfile.Email, GeneratedTestAccountPassword);

    // Update Organization with a new name
    auto [UpdateResult] = AWAIT_PRE(OrganizationSystem, UpdateOrganization, RequestPredicate, TestOrganization.Id, UpdatedOrgName);
    EXPECT_EQ(UpdateResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Confirm that the updated Organization name is correct
    const auto& UpdatedOrganization = UpdateResult.GetOrganization();
    EXPECT_EQ(UpdatedOrganization.Name, UpdatedOrgName);

    // Log out - Default Test user
    LogOut(UserSystem);

    // log in as super user - The super user has the required privileges to remove users.
    LogIn(UserSystem, SuperUserId, SuperUserLoginEmail, SuperUserLoginPassword);

    CleanupTestOrgization(TestOrganization.Id);

    // Delete the default test user
    CleanupTestUser(TestDefaultUserProfile.UserId);

    // Delete the admin test user
    CleanupTestUser(TestAdminUserProfile.UserId);

    // Log out - SuperUser
    LogOut(UserSystem);
}
#endif
