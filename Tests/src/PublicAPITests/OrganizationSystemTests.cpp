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
#include "CSP/Systems/Organizations/OrganizationSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <algorithm>
#include <filesystem>


using namespace csp::common;
using namespace csp::systems;


namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

} // namespace



void GetUsersRoles(::OrganizationSystem* OrganizationSystem, const csp::common::Array<csp::common::String>& UserIds, csp::common::Array<OrganizationRoleInfo>& OutOrganizationRoleInfo)
{
	auto [Result] = AWAIT_PRE(OrganizationSystem, GetUserRolesInOrganization, RequestPredicate, UserIds);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	const auto& OrganizationRoleInfo = Result.GetOrganizationRoleInfo();

	OutOrganizationRoleInfo = OrganizationRoleInfo;
}

InviteOrganizationRoleCollection CreateOrganizationInvites()
{
	InviteOrganizationRoleInfo InviteUser1;
	InviteUser1.UserEmail = AltUser1NonMemberEmail;
	InviteUser1.OrganizationRoles  = {EOrganizationRole::Member};

	InviteOrganizationRoleInfo InviteUser2;
	InviteUser2.UserEmail = AltUser2NonMemberEmail;
	InviteUser2.OrganizationRoles  = {EOrganizationRole::Member};

	InviteOrganizationRoleCollection OrganizationInvites;
	OrganizationInvites.EmailLinkUrl = "https://dev.magnoverse.space";
	OrganizationInvites.SignupUrl = "https://dev.magnoverse.space";
	OrganizationInvites.InvitedUserRoles = {InviteUser1, InviteUser2};

	return OrganizationInvites;
}

/*
 *Test:
 *Invite user and confirm roles correct
 *Invite user without Member role and confirm they end up with the role
 *Invite user with only member role and confirm that this fails.
 *Bulk invite users and confirm roles correct
 *Bulk invite users without Member role and confirm they end up with the role
*/

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Get alt account user ID
	// Todo: Will need to add use test credentials of new user without a role
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	// Log in
	// Todo: Will need to add new test credentials
	String DefaultUserId;
	LogIn(UserSystem, DefaultUserId);

	csp::common::Array<EOrganizationRole> AltUserRoles{EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl		= "https://dev.magnoverse.space";
	csp::common::String SignupUrl			= "https://dev.magnoverse.space";

	// Invite alt User to the Organization
	// Todo: Will need to use new Org test credentials with Owner or Admin(?) role.
	auto [Result] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, AlternativeLoginEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that alt user has correct roles in Organization
	// Todo: Will need to use new Org test credentials
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, {AltUserId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_WITHOUR_MEMBER_ROLE_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationWithoutMemberRoleTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Get alt account user ID
	// Todo: Will need to add new test credentials
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	// Log in
	// Todo: Will need to add use test credentials of new user without a role
	String DefaultUserId;
	LogIn(UserSystem, DefaultUserId);

	// Member role has been omitted. All users must have member role so it will be automatically added.
	csp::common::Array<EOrganizationRole> AltUserRoles{EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl = "https://dev.magnoverse.space";

	// Invite alt User to the Organization
	// Todo: Will need to use new Org test credentials with Owner or Admin(?) role.
	auto [Result] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, AlternativeLoginEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that alt user has correct roles in Organization
	// Todo: Will need to use new Org test credentials
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, {AltUserId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	// Confirm that they have the Member role even though it was not specified.
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_WITHOUT_PERMISSIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationWithoutPermissionsTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Get alt account user ID
	// Todo: Will need to add use test credentials of new user without a role
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	// Log in
	// Todo: Will need to use test credentials of user who only has member role
	String MemberOnlyUserId;
	LogIn(UserSystem, MemberOnlyUserId);

	// User who only has member role is trying to invite a user with Administrator role. This is not allowed.
	csp::common::Array<EOrganizationRole> AltUserRoles{EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl = "https://dev.magnoverse.space";

	// A user with only the member role should not be able to invite other users with elevated permissions to the Organization.
	// Todo: Will need to use new Org test credentials with Owner or Admin(?) role.
	auto [Result] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, AlternativeLoginEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

	// Log out
	LogOut(UserSystem);
}
#endif

//#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_BULK_INVITE_TO_ORGANIZATION_TEST
#if 1
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, BulkInviteToOrganizationTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Log in
	// Todo: Will need to add new test credentials
	String DefaultUserId;
	LogIn(UserSystem, DefaultUserId);

	auto OrganizationInvites = CreateOrganizationInvites();

	// Invite alt User to the Organization
	// Todo: Will need to use new Org test credentials with Owner or Admin(?) role.
	auto [Result] = AWAIT_PRE(OrganizationSystem, BulkInviteToOrganization, RequestPredicate, OrganizationInvites);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Get alt account user ID
	// Todo: Will need to add new test credentials
	String AltUser1NonMemberId;
	LogIn(UserSystem, AltUser1NonMemberId, AltUser1NonMemberEmail, AltUser1NonMemberPassword);
	LogOut(UserSystem);

	String AltUser2NonMemberId;
	LogIn(UserSystem, AltUser2NonMemberId, AltUser2NonMemberEmail, AltUser2NonMemberPassword);
	LogOut(UserSystem);

	// Confirm that alt user has correct roles in Organization
	// Todo: Will need to use new Org test credentials
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, {AltUser1NonMemberId, AltUser2NonMemberId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles[0], EOrganizationRole::Member);

	// Log out
	LogOut(UserSystem);
}
#endif

