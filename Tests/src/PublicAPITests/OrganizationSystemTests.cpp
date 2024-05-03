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
#include "Debug/Logging.h"

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

	OutOrganizationRoleInfo = Result.GetOrganizationRoleInfo();
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

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, InviteToOrganizationTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Get User Id of non-member user
	String AltUser1NonMemberId;
	LogIn(UserSystem, AltUser1NonMemberId, AltUser1NonMemberEmail, AltUser1NonMemberPassword);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, AltUser1AdminEmail, AltUser1AdminPassword);

	csp::common::Array<EOrganizationRole> AltUserRoles{EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl		= "https://dev.magnoverse.space";
	csp::common::String SignupUrl			= "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [InviteResult] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, AltUser1NonMemberEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member user now has the correct roles in Organization
	csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, {AltUser1NonMemberId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// remove user from organization
	auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, AltUser1NonMemberId);
	EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

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

	// Get User Id of non-member user
	String AltUser1NonMemberId;
	LogIn(UserSystem, AltUser1NonMemberId, AltUser1NonMemberEmail, AltUser1NonMemberPassword);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, AltUser1AdminEmail, AltUser1AdminPassword);

	// Member role has intentionally been omitted. All users must have member role and so CSP it will automatically add it
	csp::common::Array<EOrganizationRole> AltUserRoles{EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl = "https://dev.magnoverse.space";

	// Invite non-member user to the Organization
	auto [InviteResult] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, AltUser1NonMemberEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(InviteResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member user now has the correct roles in Organization, including member role
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, {AltUser1NonMemberId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[1], EOrganizationRole::Administrator);

	// remove user from organization
	auto [RemoveResult] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, AltUser1NonMemberId);
	EXPECT_EQ(RemoveResult.GetResultCode(), csp::systems::EResultCode::Success);

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

	// Log in as user with only a member role in the Organization
	String MemberOnlyUser1Id;
	LogIn(UserSystem, MemberOnlyUser1Id, AltUser1MemberEmail, AltUser1MemberPassword);

	// Define organization roles for the new user.
	csp::common::Array<EOrganizationRole> AltUserRoles{EOrganizationRole::Member, EOrganizationRole::Administrator};
	csp::common::String EmailLinkUrl = "https://dev.magnoverse.space";
	csp::common::String SignupUrl = "https://dev.magnoverse.space";

	// A user with only the member role should not be able to invite other users to the Organization.
	auto [Result] = AWAIT_PRE(OrganizationSystem, InviteToOrganization, RequestPredicate, AltUser1NonMemberEmail, AltUserRoles, EmailLinkUrl, SignupUrl);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS || RUN_ORGANIZATIONSYSTEM_BULK_INVITE_TO_ORGANIZATION_TEST
CSP_PUBLIC_TEST(CSPEngine, OrganizationSystemTests, BulkInviteToOrganizationTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* OrganizationSystem	 = SystemsManager.GetOrganizationSystem();

	// Get User Id of non-member user 1
	String AltUser1NonMemberId;
	LogIn(UserSystem, AltUser1NonMemberId, AltUser1NonMemberEmail, AltUser1NonMemberPassword);
	LogOut(UserSystem);

	// Get User Id of non-member user 2
	String AltUser2NonMemberId;
	LogIn(UserSystem, AltUser2NonMemberId, AltUser2NonMemberEmail, AltUser2NonMemberPassword);
	LogOut(UserSystem);

	// Log in - user needs to be an admin member of the organization
	String AdminUserId;
	LogIn(UserSystem, AdminUserId, AltUser1AdminEmail, AltUser1AdminPassword);

	auto OrganizationInvites = CreateOrganizationInvites();

	// Invite non-member users to the Organization
	auto [Result] = AWAIT_PRE(OrganizationSystem, BulkInviteToOrganization, RequestPredicate, OrganizationInvites);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Confirm that non-member users now have the correct roles in Organization
	::csp::common::Array<OrganizationRoleInfo> UserOrganizationRoleInfo;
	GetUsersRoles(OrganizationSystem, {AltUser1NonMemberId, AltUser2NonMemberId}, UserOrganizationRoleInfo);

	EXPECT_EQ(UserOrganizationRoleInfo.Size(), 2);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[0].OrganizationRoles[0], EOrganizationRole::Member);
	EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles.Size(), 1);
	EXPECT_EQ(UserOrganizationRoleInfo[1].OrganizationRoles[0], EOrganizationRole::Member);

	// remove user1 from organization
	auto [RemoveUser1Result] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, AltUser1NonMemberId);
	EXPECT_EQ(RemoveUser1Result.GetResultCode(), csp::systems::EResultCode::Success);

	// remove user2 from organization
	auto [RemoveUser2Result] = AWAIT_PRE(OrganizationSystem, RemoveUserFromOrganization, RequestPredicate, AltUser2NonMemberId);
	EXPECT_EQ(RemoveUser2Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Log out
	LogOut(UserSystem);
}
#endif

