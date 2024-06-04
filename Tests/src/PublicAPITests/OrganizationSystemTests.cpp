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
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <PublicAPITests/AssetSystemTestHelpers.h>


using namespace csp::common;
using namespace csp::systems;


namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

} // namespace


csp::common::Map<csp::common::String, csp::common::String> GetOrganizationAccountDetails()
{
	if (!std::filesystem::exists("organization_test_account_creds.txt"))
	{
		LogFatal("organization_test_account_creds.txt must be in the following format:\nOrgAdminLoginEmail "
				 "<OrgAdminLoginEmail>\nOrgAdminLoginPassword <OrgAdminLoginPassword>");
	}

	csp::common::Map<csp::common::String, csp::common::String> OutMap;

	std::ifstream CredsFile;
	CredsFile.open("organization_test_account_creds.txt");
	std::string Key, Value;
	while (CredsFile >> Key >> Value)
	{
		OutMap[Key.c_str()] = Value.c_str();
	}

	return OutMap;
}


void GetUsersRoles(::OrganizationSystem* OrganizationSystem,
				   const csp::common::Optional<csp::common::String>& OrganizationId,
				   const csp::common::Array<csp::common::String>& UserIds,
				   csp::common::Array<OrganizationRoleInfo>& OutOrganizationRoleInfo)
{
	auto [Result] = AWAIT_PRE(OrganizationSystem, GetUserRolesInOrganization, RequestPredicate, OrganizationId, UserIds);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	OutOrganizationRoleInfo = Result.GetOrganizationRoleInfo();
}

InviteOrganizationRoleCollection CreateOrganizationInvites()
{
	InviteOrganizationRoleInfo InviteUser1;
	InviteUser1.UserEmail		  = DefaultLoginEmail;
	InviteUser1.OrganizationRoles = {EOrganizationRole::Member};

	InviteOrganizationRoleInfo InviteUser2;
	InviteUser2.UserEmail		  = AlternativeLoginEmail;
	InviteUser2.OrganizationRoles = {EOrganizationRole::Member};

	InviteOrganizationRoleCollection OrganizationInvites;
	OrganizationInvites.EmailLinkUrl	 = "https://dev.magnoverse.space";
	OrganizationInvites.SignupUrl		 = "https://dev.magnoverse.space";
	OrganizationInvites.InvitedUserRoles = {InviteUser1, InviteUser2};

	return OrganizationInvites;
}

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

	// Get User Id of non-member user
	String DefaultLoginId;
	LogIn(UserSystem, DefaultLoginId);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, OrgAdminUserEmail, OrgAdminUserPassword);

	csp::common::Array<EOrganizationRole> AltUserRoles {EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl	 = "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [InviteResult]
		= AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, nullptr, DefaultLoginEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member user now has the correct roles in Organization
	csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, nullptr, {DefaultLoginId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// remove user from organization
	auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, DefaultLoginId);
	EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_WITHOUT_MEMBER_ROLE_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationWithoutMemberRoleTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

	// Get User Id of non-member user
	String DefaultLoginId;
	LogIn(UserSystem, DefaultLoginId);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, OrgAdminUserEmail, OrgAdminUserPassword);

	// Member role has intentionally been omitted. All users must have member role and so CSP it will automatically add it
	csp::common::Array<EOrganizationRole> AltUserRoles {EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl	 = "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [InviteResult]
		= AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, nullptr, DefaultLoginEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member user now has the correct roles in Organization, including member role
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, nullptr, {DefaultLoginId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// remove user from organization
	auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, DefaultLoginId);
	EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_WITHOUT_PERMISSIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationWithoutPermissionsTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, OrgAdminUserEmail, OrgAdminUserPassword);

	// invite default with member role.
	csp::common::Array<EOrganizationRole> DefaultUserRoles {EOrganizationRole::Member};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl	 = "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [InviteResult] = AWAIT_PRE(OrganizationSystem,
									InviteToOrganization,
									RequestPredicate,
									nullptr,
									DefaultLoginEmail,
									DefaultUserRoles,
									EmailLinkUrl,
									SignupUrl);
	EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out admin
	LogOut(UserSystem);

	// Log in default user who is only a member role in the Organization
	String DefaultLoginId;
	LogIn(UserSystem, DefaultLoginId);

	// Define organization roles for the new user.
	csp::common::Array<EOrganizationRole> AltUserRoles {EOrganizationRole::Member, EOrganizationRole::Administrator};

	// A user with only the member role should not be able to invite other users to the Organization.
	auto [Result] = AWAIT_PRE(OrganizationSystem,
							  InviteToOrganization,
							  RequestPredicate,
							  nullptr,
							  AlternativeLoginEmail,
							  AltUserRoles,
							  EmailLinkUrl,
							  SignupUrl);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

	// remove user from organization
	auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, DefaultLoginId);
	EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out default user
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_SPECIFIED_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToSpecifiedOrganizationTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

	// Get User Id of non-member user
	String DefaultLoginId;
	LogIn(UserSystem, DefaultLoginId);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, OrgAdminUserEmail, OrgAdminUserPassword);

	// Get the Id of the Organization the user is authenticated against. Users can currently only
	// belong to a single Organization so we just use the first one.
	auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
	EXPECT_EQ(OrganizationIds.Size(), 1);

	auto& Oko_Tests_OrganizationId = OrganizationIds[0];

	csp::common::Array<EOrganizationRole> AltUserRoles {EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl	 = "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [InviteResult] = AWAIT_PRE(OrganizationSystem,
									InviteToOrganization,
									RequestPredicate,
									Oko_Tests_OrganizationId,
									DefaultLoginEmail,
									AltUserRoles,
									EmailLinkUrl,
									SignupUrl);
	EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member user now has the correct roles in Organization
	csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, Oko_Tests_OrganizationId, {DefaultLoginId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// remove user from organization
	auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, Oko_Tests_OrganizationId, DefaultLoginId);
	EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_INVALID_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToInvalidOrganizationTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

	const char* Invalid_Organization_Id = "invalid";

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, OrgAdminUserEmail, OrgAdminUserPassword);

	csp::common::Array<EOrganizationRole> AltUserRoles {EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl	 = "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [Result] = AWAIT_PRE(OrganizationSystem,
							  InviteToOrganization,
							  RequestPredicate,
							  Invalid_Organization_Id,
							  DefaultLoginEmail,
							  AltUserRoles,
							  EmailLinkUrl,
							  SignupUrl);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_BULK_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, BulkInviteToOrganizationTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();

	// Get User Id of non-member user 1
	String DefaultLoginId;
	LogIn(UserSystem, DefaultLoginId);
	LogOut(UserSystem);

	// Get User Id of non-member user 2
	String AltLoginId;
	LogIn(UserSystem, AltLoginId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, OrgAdminUserEmail, OrgAdminUserPassword);

	auto OrganizationInvites = CreateOrganizationInvites();

	// Invite non-member users to the Organization
	auto [Result] = AWAIT_PRE(OrganizationSystem, BulkInviteToOrganization, RequestPredicate, nullptr, OrganizationInvites);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member users now have the correct roles in Organization
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, nullptr, {DefaultLoginId, AltLoginId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles[0], EOrganizationRole::Member);

	// remove user1 from organization
	auto [RemoveUser1Result] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, DefaultLoginId);
	EXPECT_EQ(RemoveUser1Result.GetResultCode(), csp::systems::EResultCode::Success);

	// remove user2 from organization
	auto [RemoveUser2Result] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, nullptr, AltLoginId);
	EXPECT_EQ(RemoveUser2Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_CREATE_ORGANISATION_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, CreateOrganisationSpaceTest)
{
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager	 = ::SystemsManager::Get();
	auto* UserSystem		 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem = SystemsManager.GetOrganizationSystem();
	auto* SpaceSystem		 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Log in
	String UserId;
	LogIn(UserSystem, UserId, OrgAdminUserEmail, OrgAdminUserPassword);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto OrgAccountDetails	  = GetOrganizationAccountDetails();
	bool HasOrgAccountDetails = OrgAccountDetails.HasKey("OrgAdminLoginEmail") && OrgAccountDetails.HasKey("OrgAdminLoginPassword");

	EXPECT_TRUE(HasOrgAccountDetails);

	auto OrgAdminUserEmail	  = OrgAccountDetails["OrgAdminLoginEmail"];
	auto OrgAdminUserPassword = OrgAccountDetails["OrgAdminLoginPassword"];

	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	const char* TestSpaceName			  = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	  = "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName	  = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
	const char* TestAssetName			  = "OLY-UNITTEST-ASSET-REWIND";
	const char* TestThirdPartyReferenceId = "OLY-UNITTEST-ASSET-THIRDPARTY";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId, OrgAdminUserEmail, OrgAdminUserPassword);

	// Get the Id of the Organization the user is authenticated against. Users can currently only
	// belong to a single Organization so we just use the first one.
	auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
	EXPECT_EQ(OrganizationIds.Size(), 1);

	auto& Oko_Tests_OrganizationId = OrganizationIds[0];

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Create asset collection
	csp::systems::AssetCollection AssetCollection;
	CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

	EXPECT_EQ(AssetCollection.OrganizationId, Oko_Tests_OrganizationId);

	// Delete asset collection
	DeleteAssetCollection(AssetSystem, AssetCollection);
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif