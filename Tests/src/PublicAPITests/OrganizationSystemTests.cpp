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
//


#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_WITH_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationWithIdTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, AltUser1AdminEmail, AltUser1AdminPassword);

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
	// If a member retrieves the Organization object it should contain info on all members
	EXPECT_TRUE(Organization.Members.Size() > 0);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_WITH_NO_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationWithNoIdTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, AltUser1AdminEmail, AltUser1AdminPassword);

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

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_WITH_INCORRECT_ROLE_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationWithIncorrectRoleTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Log in with an Organization member - The Organization members collection should only contain info on this user.
	String AltUser1MemberId;
	LogIn(UserSystem, AltUser1MemberId, AltUser1MemberEmail, AltUser1MemberPassword);

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
	// If a member retrieves the Organization object it should contain info on only 1 member (them)
	EXPECT_EQ(Organization.Members.Size(), 1);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_GET_ORGANIZATION_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, GetOrganizationIdTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Log in as a member of an organization
	String AltUser1MemberId;
	LogIn(UserSystem, AltUser1MemberId, AltUser1MemberEmail, AltUser1MemberPassword);

	const auto RetrievedOrganizationId = OrganizationSystem->GetCurrentOrganizationId();

	// Get the Id of the Organization the user is authenticated against. Users can currently only
	// belong to a single Organization so we just use the first one.
	auto OrganizationIds = UserSystem->GetLoginState().OrganizationIds;
	EXPECT_EQ(OrganizationIds.Size(), 1);

	auto& CurrentOrganizationId = OrganizationIds[0];

	// The returned Organization Id should match the one returned by the User System
	EXPECT_EQ(RetrievedOrganizationId, CurrentOrganizationId);

	// Log out
	LogOut(UserSystem);
}
#endif
