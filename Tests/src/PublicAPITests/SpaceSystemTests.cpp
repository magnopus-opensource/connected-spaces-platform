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

#include "gtest/gtest.h"
#include <algorithm>
#include <filesystem>


using namespace csp::common;
using namespace csp::systems;


// TODO: Clean up these tests


namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

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

void CreateSpace(::SpaceSystem* SpaceSystem,
				 const String& Name,
				 const String& Description,
				 SpaceAttributes Attributes,
				 const Optional<Map<String, String>>& Metadata,
				 const Optional<InviteUserRoleInfoCollection>& InviteUsers,
				 const Optional<FileAssetDataSource>& Thumbnail,
				 Space& OutSpace)
{
	Map<String, String> TestMetadata = Metadata.HasValue() ? (*Metadata) : Map<String, String>({{"site", "Void"}});

	// TODO: Add tests for public spaces
	auto [Result] = AWAIT_PRE(SpaceSystem, CreateSpace, RequestPredicate, Name, Description, Attributes, InviteUsers, TestMetadata, Thumbnail);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	OutSpace = Result.GetSpace();
}

void CreateSpaceWithBuffer(::SpaceSystem* SpaceSystem,
						   const String& Name,
						   const String& Description,
						   SpaceAttributes Attributes,
						   const Optional<Map<String, String>>& Metadata,
						   const Optional<InviteUserRoleInfoCollection>& InviteUsers,
						   BufferAssetDataSource& Thumbnail,
						   Space& OutSpace)
{
	Map<String, String> TestMetadata = Metadata.HasValue() ? (*Metadata) : Map<String, String>({{"site", "Void"}});

	auto [Result]
		= AWAIT_PRE(SpaceSystem, CreateSpaceWithBuffer, RequestPredicate, Name, Description, Attributes, InviteUsers, TestMetadata, Thumbnail);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	OutSpace = Result.GetSpace();
}

void GetSpace(::SpaceSystem* SpaceSystem, const String& SpaceId, Space& OutSpace)
{
	auto [Result] = AWAIT_PRE(SpaceSystem, GetSpace, RequestPredicate, SpaceId);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	OutSpace = Result.GetSpace();
}

Array<BasicSpace> GetSpacesByAttributes(::SpaceSystem* SpaceSystem,
										const Optional<bool>& IsDiscoverable,
										const Optional<bool>& IsArchived,
										const Optional<bool>& RequiresInvite,
										const Optional<int>& ResultsSkipNo,
										const Optional<int>& ResultsMaxNo)
{
	auto [Result]
		= AWAIT_PRE(SpaceSystem, GetSpacesByAttributes, RequestPredicate, IsDiscoverable, IsArchived, RequiresInvite, ResultsSkipNo, ResultsMaxNo);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	const auto SpacesTotalCount = Result.GetTotalCount();
	const auto Spaces			= Result.GetSpaces();

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

void UpdateSpace(::SpaceSystem* SpaceSystem,
				 const String& SpaceId,
				 const Optional<String>& NewName,
				 const Optional<String>& NewDescription,
				 const Optional<SpaceAttributes>& NewAttributes,
				 BasicSpace& OutSpace)
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
	SiteInfo.Name	  = SiteName;
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
	OutSites				= Array<Site>(ResultSites.Size());

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
	Array<String> Ids = {UserId};
	auto [Result]	  = AWAIT_PRE(SpaceSystem, GetUsersRoles, RequestPredicate, SpaceId, Ids);

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
	OutUsersRoles				  = Array<UserRoleInfo>(ReturnedRolesInfo.Size());

	for (int idx = 0; idx < ReturnedRolesInfo.Size(); ++idx)
	{
		OutUsersRoles[idx] = ReturnedRolesInfo[idx];
	}
}

void UpdateSpaceMetadata(::SpaceSystem* SpaceSystem, const String& SpaceId, const Optional<Map<String, String>>& NewMetadata)
{
	Map<String, String> Metadata = NewMetadata.HasValue() ? *NewMetadata : Map<String, String>();

	auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceMetadata, RequestPredicate, SpaceId, Metadata);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	std::cerr << "Space metadata has been updated successfully" << std::endl;
}

void GetSpaceMetadata(::SpaceSystem* SpaceSystem, const String& SpaceId, Map<String, String>& OutMetadata)
{
	auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceMetadata, RequestPredicate, SpaceId);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	OutMetadata = Result.GetMetadata();
}

void GetSpacesMetadata(::SpaceSystem* SpaceSystem, const Array<String>& SpaceIds, Map<String, Map<String, String>>& OutMetadata)
{
	auto [Result] = AWAIT_PRE(SpaceSystem, GetSpacesMetadata, RequestPredicate, SpaceIds);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	OutMetadata = Result.GetMetadata();
}

bool IsUriValid(const std::string& Uri, const std::string& FileName)
{
	// check that Uri starts with something valid
	if (Uri.find("https://world-streaming.magnoboard.com/", 0) != 0)
	{
		return false;
	}

	// check that the correct filename is present in the Uri
	const auto PosLastSlash = Uri.rfind('/');
	const auto UriFileName	= Uri.substr(PosLastSlash + 1, FileName.size());

	return (FileName == UriFileName);
}


InviteUserRoleInfoCollection CreateInviteUsers()
{
	// Create normal users
	const auto TestUser1Email = String("testnopus.pokemon+1@magnopus.com");
	const auto TestUser2Email = String("testnopus.pokemon+2@magnopus.com");

	InviteUserRoleInfo InviteUser1;
	InviteUser1.UserEmail = TestUser1Email;
	InviteUser1.UserRole  = SpaceUserRole::User;

	InviteUserRoleInfo InviteUser2;
	InviteUser2.UserEmail = TestUser2Email;
	InviteUser2.UserRole  = SpaceUserRole::User;

	// Create moderator users
	const auto TestModInviteUser1Email = String("testnopus.pokemon+mod1@magnopus.com");
	const auto TestModInviteUser2Email = String("testnopus.pokemon+mod2@magnopus.com");

	InviteUserRoleInfo ModInviteUser1;
	ModInviteUser1.UserEmail = TestModInviteUser1Email;
	ModInviteUser1.UserRole	 = SpaceUserRole::Moderator;

	InviteUserRoleInfo ModInviteUser2;
	ModInviteUser2.UserEmail = TestModInviteUser2Email;
	ModInviteUser2.UserRole	 = SpaceUserRole::Moderator;

	InviteUserRoleInfoCollection InviteUsers;
	InviteUsers.InviteUserRoleInfos = {InviteUser1, InviteUser2, ModInviteUser1, ModInviteUser2};
	InviteUsers.EmailLinkUrl		= "https://dev.magnoverse.space";
	InviteUsers.SignupUrl			= "https://dev.magnoverse.space";


	return InviteUsers;
}

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, CreateSpaceTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	auto InviteUsers = CreateInviteUsers();

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
	FILE* UploadFile	= nullptr;
	fopen_s(&UploadFile, UploadFilePath.string().c_str(), "rb");

	uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
	auto* UploadFileData	 = new unsigned char[UploadFileSize];
	fread(UploadFileData, UploadFileSize, 1, UploadFile);
	fclose(UploadFile);

	BufferAssetDataSource BufferSource;
	BufferSource.Buffer		  = UploadFileData;
	BufferSource.BufferLength = UploadFileSize;

	BufferSource.SetMimeType("image/png");

	// Create space
	::Space Space;
	CreateSpaceWithBuffer(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, BufferSource, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	auto InviteUsers = CreateInviteUsers();

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
	FILE* UploadFile	= nullptr;
	fopen_s(&UploadFile, UploadFilePath.string().c_str(), "rb");

	uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
	auto* UploadFileData	 = new unsigned char[UploadFileSize];
	fread(UploadFileData, UploadFileSize, 1, UploadFile);
	fclose(UploadFile);

	BufferAssetDataSource BufferSource;
	BufferSource.Buffer		  = UploadFileData;
	BufferSource.BufferLength = UploadFileSize;

	BufferSource.SetMimeType("image/png");

	// Create space
	::Space Space;
	CreateSpaceWithBuffer(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, BufferSource, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniquePublicSpaceName[256];
	SPRINTF(UniquePublicSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniquePrivateSpaceName[256];
	SPRINTF(UniquePrivateSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	::Space PublicSpace;
	CreateSpace(SpaceSystem, UniquePublicSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, PublicSpace);

	::Space PrivateSpace;
	CreateSpace(SpaceSystem, UniquePrivateSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, PrivateSpace);

	Array<String> SpacesIds = {PublicSpace.Id, PrivateSpace.Id};

	Array<::Space> ResultSpaces = GetSpacesByIds(SpaceSystem, SpacesIds);

	EXPECT_EQ(ResultSpaces.Size(), SpacesIds.Size());

	bool PrivateSpaceFound = false;
	bool PublicSpaceFound  = false;

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	constexpr size_t SPACE_COUNT = 3;

	String UserId;

	// Log in using default test account to create spaces
	LogIn(UserSystem, UserId);

	// Create test spaces
	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	String SpaceId[SPACE_COUNT];

	for (int i = 0; i < SPACE_COUNT; ++i)
	{
		char UniqueSpaceName[256];
		SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

		::Space Space;

		CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, Space);

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

		EXPECT_TRUE((bool) (Space.Attributes & SpaceAttributes::IsDiscoverable));
		EXPECT_FALSE((bool) (Space.Attributes & SpaceAttributes::RequiresInvite));
	}

	// Log out as guest
	LogOut(UserSystem);

	// Clean up
	LogIn(UserSystem, UserId);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	constexpr size_t SPACE_COUNT = 3;

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create test spaces
	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	String SpaceId[SPACE_COUNT];

	for (int i = 0; i < SPACE_COUNT; ++i)
	{
		char UniqueSpaceName[256];
		SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

		::Space Space;

		CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, Space);

		SpaceId[i] = Space.Id;
	}

	// Get only the public spaces
	Array<BasicSpace> ResultSpaces = GetSpacesByAttributes(SpaceSystem, true, false, false, 0, static_cast<int>(SPACE_COUNT));

	EXPECT_GE(ResultSpaces.Size(), SPACE_COUNT);

	// Make sure that all returned spaces are public
	for (int i = 0; i < ResultSpaces.Size(); ++i)
	{
		const auto& Space = ResultSpaces[i];

		EXPECT_TRUE((bool) (Space.Attributes & SpaceAttributes::IsDiscoverable));
		EXPECT_FALSE((bool) (Space.Attributes & SpaceAttributes::RequiresInvite));
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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	constexpr size_t SPACE_COUNT = 3;

	String UserId;

	// Log in using default test account to create spaces
	LogIn(UserSystem, UserId);

	// Create test spaces
	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	String SpaceId[SPACE_COUNT];

	for (int i = 0; i < SPACE_COUNT; ++i)
	{
		char UniqueSpaceName[256];
		SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

		::Space Space;

		CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

		SpaceId[i] = Space.Id;
	}

	// Get only the public spaces
	Array<BasicSpace> ResultSpaces = GetSpacesByAttributes(SpaceSystem, false, false, true, 0, static_cast<int>(SPACE_COUNT));

	EXPECT_GE(ResultSpaces.Size(), SPACE_COUNT);

	// Make sure that all returned spaces are public
	for (int i = 0; i < ResultSpaces.Size(); ++i)
	{
		const auto& Space = ResultSpaces[i];

		EXPECT_FALSE((bool) (Space.Attributes & SpaceAttributes::IsDiscoverable));
		EXPECT_TRUE((bool) (Space.Attributes & SpaceAttributes::RequiresInvite));
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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	constexpr size_t SPACE_COUNT = 6;

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create test spaces
	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	String SpaceId[SPACE_COUNT];

	for (int i = 0; i < SPACE_COUNT; ++i)
	{
		char UniqueSpaceName[256];
		SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

		::Space Space;

		CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

		SpaceId[i] = Space.Id;
	}

	// Get private spaces paginated
	{
		auto [Result] = AWAIT_PRE(SpaceSystem, GetSpacesByAttributes, RequestPredicate, false, false, true, 0, static_cast<int>(SPACE_COUNT / 2));

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		const auto SpacesTotalCount = Result.GetTotalCount();
		const auto Spaces			= Result.GetSpaces();

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Login as an admin user in order to be able to create the test space
	String SpaceOwnerUserId;
	LogIn(UserSystem, SpaceOwnerUserId);

	::Space PublicSpace;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, PublicSpace);

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
	LogIn(UserSystem, SpaceOwnerUserId);
	DeleteSpace(SpaceSystem, PublicSpace.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ADD_SITE_INFO_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, AddSiteInfoTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	LogIn(UserSystem, UserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	LogIn(UserSystem, UserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();

	// Get alt account user ID
	String AltUserId;


	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String DefaultUserId;

	// Log in
	LogIn(UserSystem, DefaultUserId);

	// Create test space
	InviteUserRoleInfo InviteUser;
	InviteUser.UserEmail = AlternativeLoginEmail;
	InviteUser.UserRole	 = SpaceUserRole::User;
	InviteUserRoleInfoCollection InviteUsers;
	InviteUsers.InviteUserRoleInfos = {InviteUser};
	InviteUsers.EmailLinkUrl		= "dev.magnoverse.space";

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, nullptr, Space);

	// Log out
	LogOut(UserSystem);

	// Log in using alt test account
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Ensure alt test account can join space
	{
		auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

		ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

		SpaceSystem->ExitSpace(
			[](const csp::systems::NullResult& Result)
			{
			});
	}

	// Log out and log in again using default test account
	LogOut(UserSystem);
	LogIn(UserSystem, DefaultUserId);

	// Update test account user roles for space
	GetSpace(SpaceSystem, Space.Id, Space);

	::UserRoleInfo UpdatedDefaultUserRole	 = {DefaultUserId, SpaceUserRole::Moderator};
	::UserRoleInfo UpdatedSecondTestUserRole = {AltUserId, SpaceUserRole::Owner};

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Login as an admin user in order to be able to create the test space
	String SpaceOwnerUserId;
	LogIn(UserSystem, SpaceOwnerUserId);

	::Space PublicSpace;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, PublicSpace);

	LogOut(UserSystem);

	// Log in as a guest user
	String GuestUserId;
	LogInAsGuest(UserSystem, GuestUserId);

	auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, PublicSpace.Id, GuestUserId);
	EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

	LogOut(UserSystem);

	// login as an admin user
	LogIn(UserSystem, SpaceOwnerUserId);

	::UserRoleInfo UpdatedUserRoleInfo = {GuestUserId, SpaceUserRole::Moderator};
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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	// Get alt account user ID
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String DefaultUserId;

	// Log in
	LogIn(UserSystem, DefaultUserId);

	// create a space with no other user Ids invited
	::Space Space;
	// CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Invite second test account as a Moderator Role user
	auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeLoginEmail, true, "", "");
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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;
	LogIn(UserSystem, UserId);

	Map<String, String> TestSpaceMetadata = {{"site", "Void"}};

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, Space);

	Map<String, String> RetrievedSpaceMetadata;
	GetSpaceMetadata(SpaceSystem, Space.Id, RetrievedSpaceMetadata);

	EXPECT_EQ(RetrievedSpaceMetadata.Size(), TestSpaceMetadata.Size());
	EXPECT_EQ(RetrievedSpaceMetadata["site"], "Void");

	TestSpaceMetadata["site"] = "MagOffice";

	UpdateSpaceMetadata(SpaceSystem, Space.Id, TestSpaceMetadata);

	GetSpaceMetadata(SpaceSystem, Space.Id, RetrievedSpaceMetadata);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;
	LogIn(UserSystem, UserId);

	Map<String, String> TestSpaceMetadata = {{"site", "Void"}};

	::Space Space1, Space2;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, Space1);
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, TestSpaceMetadata, nullptr, nullptr, Space2);

	Array<String> Spaces = {Space1.Id, Space2.Id};
	Map<String, Map<String, String>> RetrievedSpacesMetadata;
	GetSpacesMetadata(SpaceSystem, Spaces, RetrievedSpacesMetadata);

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

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACE_THUMBNAIL_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, UpdateSpaceThumbnailTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	LogIn(UserSystem, UserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Create space without a thumbnail
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	{
		auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
		EXPECT_TRUE(Result.GetUri().IsEmpty());
	}


	{ /// Bad file path test
		FileAssetDataSource SpaceThumbnail;
		const std::string LocalFileName = "OKO.png";
		const auto FilePath				= std::filesystem::absolute("assets/badpath/" + LocalFileName);
		SpaceThumbnail.FilePath			= FilePath.u8string().c_str();
		SpaceThumbnail.SetMimeType("image/png");

		auto [Result] = AWAIT_PRE(SpaceSystem, UpdateSpaceThumbnail, RequestPredicate, Space.Id, SpaceThumbnail);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
	}

	{
		FileAssetDataSource SpaceThumbnail;
		const std::string LocalFileName = "OKO.png";
		const auto FilePath				= std::filesystem::absolute("assets/" + LocalFileName);
		SpaceThumbnail.FilePath			= FilePath.u8string().c_str();
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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	LogIn(UserSystem, UserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Create space without a thumbnail
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	{
		auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
		EXPECT_TRUE(Result.GetUri().IsEmpty());
	}

	auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
	FILE* UploadFile	= nullptr;
	fopen_s(&UploadFile, UploadFilePath.string().c_str(), "rb");

	uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
	auto* UploadFileData	 = new unsigned char[UploadFileSize];
	fread(UploadFileData, UploadFileSize, 1, UploadFile);
	fclose(UploadFile);

	BufferAssetDataSource SpaceThumbnail;
	SpaceThumbnail.Buffer		= UploadFileData;
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
	Asset.Uri	   = uri_Result.GetUri();
	// Get data
	auto [Download_Result] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicateWithProgress, Asset);

	EXPECT_EQ(Download_Result.GetResultCode(), csp::systems::EResultCode::Success);

	size_t DownloadedAssetDataSize = Download_Result.GetDataLength();
	auto DownloadedAssetData	   = new uint8_t[DownloadedAssetDataSize];
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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;
	LogIn(UserSystem, UserId);

	::Space Space;
	Map<String, String> Metadata;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, Metadata, nullptr, nullptr, Space);

	Map<String, String> RetrievedSpaceMetadata;
	GetSpaceMetadata(SpaceSystem, Space.Id, RetrievedSpaceMetadata);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;
	LogIn(UserSystem, UserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	UpdateSpaceMetadata(SpaceSystem, Space.Id, nullptr);

	Map<String, String> RetrievedSpaceMetadata;
	GetSpaceMetadata(SpaceSystem, Space.Id, RetrievedSpaceMetadata);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	const char* TestUserEmail	 = "testnopus.pokemon@magnopus.com";
	const char* TestEmailLinkUrl = "https://dev.magnoverse.space/";
	const char* TestSignupUrl	 = "https://dev.magnoverse.space/";

	String UserId;
	LogIn(UserSystem, UserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, TestUserEmail, nullptr, TestEmailLinkUrl, TestSignupUrl);

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

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

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_BULK_INVITE_TO_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BulkInvitetoSpaceTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	auto InviteUsers = CreateInviteUsers();

	String UserId;
	LogIn(UserSystem, UserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();

	const char* TestSpaceName			  = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	  = "OLY-UNITTEST-SPACEDESC-REWIND";
	Map<String, String> TestSpaceMetadata = {{"site", "Void"}};

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in with default user
	LogIn(UserSystem, UserId);

	// Create public space
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, TestSpaceMetadata, nullptr, nullptr, Space);

	// Log out with default user and in with alt user
	LogOut(UserSystem);
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	ASSERT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	// Get metadata for public space
	Map<String, String> RetrievedMetadata;
	GetSpaceMetadata(SpaceSystem, Space.Id, RetrievedMetadata);

	ASSERT_EQ(RetrievedMetadata.Size(), TestSpaceMetadata.Size());
	ASSERT_TRUE(RetrievedMetadata.HasKey("site"));
	ASSERT_EQ(RetrievedMetadata["site"], TestSpaceMetadata["site"]);

	// Exit and re-enter space to verify its OK to always add self to public space
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	{
		auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		SpaceSystem->ExitSpace(
			[](const csp::systems::NullResult& Result)
			{
			});
	}

	// Log back in with default user so space can be deleted
	LogOut(UserSystem);
	LogIn(UserSystem, UserId);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String PrimaryUserId;

	LogIn(UserSystem, PrimaryUserId);

	::Space Space;
	FileAssetDataSource SpaceThumbnail;
	const std::string LocalFileName = "test.json";
	const auto FilePath				= std::filesystem::absolute("assets/" + LocalFileName);
	SpaceThumbnail.FilePath			= FilePath.u8string().c_str();
	SpaceThumbnail.SetMimeType("application/json");

	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, SpaceThumbnail, Space);

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
	LogIn(UserSystem, SecondaryUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	{
		auto [Result] = AWAIT_PRE(SpaceSystem, GetSpaceThumbnail, RequestPredicate, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(InitialSpaceThumbnailUri, Result.GetUri());
	}

	LogOut(UserSystem);

	LogIn(UserSystem, PrimaryUserId);
	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_THUMBNAIL_WITH_GUEST_USER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GetSpaceThumbnailWithGuestUserTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String PrimaryUserId;

	LogIn(UserSystem, PrimaryUserId);

	::Space Space;
	FileAssetDataSource SpaceThumbnail;
	const std::string LocalFileName = "test.json";
	auto FilePath					= std::filesystem::absolute("assets/" + LocalFileName);
	SpaceThumbnail.FilePath			= FilePath.u8string().c_str();
	SpaceThumbnail.SetMimeType("application/json");

	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, SpaceThumbnail, Space);

	LogOut(UserSystem);

	String GuestId;
	LogInAsGuest(UserSystem, GuestId);

	FileAssetDataSource UpdatedSpaceThumbnail;
	FilePath					   = std::filesystem::absolute("assets/Fox.glb");
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

	LogIn(UserSystem, PrimaryUserId);
	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_BAN_GUESTUSER_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, BanGuestUserTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Login with first user to create space
	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, Space);

	LogOut(UserSystem);

	// Login with second user and join space
	String GuestId;
	LogInAsGuest(UserSystem, GuestId);

	auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, Space.Id, GuestId);

	EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

	LogOut(UserSystem);

	// Login again with first user to ban second user
	LogIn(UserSystem, PrimaryUserId);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Login with first user to create space
	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, Space);

	LogOut(UserSystem);

	// Login with second user and join space
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	auto [AddUserResult] = AWAIT_PRE(SpaceSystem, AddUserToSpace, RequestPredicate, Space.Id, AltUserId);

	EXPECT_EQ(AddUserResult.GetResultCode(), csp::systems::EResultCode::Success);

	LogOut(UserSystem);

	// Login again with first user to ban second user
	LogIn(UserSystem, PrimaryUserId);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	{
		EXPECT_FALSE(SpaceSystem->IsInSpace());

		auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		EXPECT_TRUE(SpaceSystem->IsInSpace());

		SpaceSystem->ExitSpace(
			[](const csp::systems::NullResult& Result)
			{
			});

		EXPECT_FALSE(SpaceSystem->IsInSpace());
	}

	LogOut(UserSystem);

	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	{
		auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
	}

	LogOut(UserSystem);

	LogIn(UserSystem, PrimaryUserId);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_ASNONMODERATOR_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceAsNonModeratorTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	LogOut(UserSystem);

	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	{
		auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
	}

	LogOut(UserSystem);

	LogIn(UserSystem, PrimaryUserId);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_ASMODERATOR_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, EnterSpaceAsModeratorTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);
	LogOut(UserSystem);

	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);
	::Space Space;
	InviteUserRoleInfo InviteUser;
	InviteUser.UserEmail = AlternativeLoginEmail;
	InviteUser.UserRole	 = SpaceUserRole::User;
	InviteUserRoleInfoCollection InviteUsers;
	InviteUsers.InviteUserRoleInfos = {InviteUser};
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteUsers, nullptr, Space);

	::UserRoleInfo NewUserRoleInfo;
	NewUserRoleInfo.UserId	 = AltUserId;
	NewUserRoleInfo.UserRole = SpaceUserRole::Moderator;

	UpdateUserRole(SpaceSystem, Space.Id, NewUserRoleInfo);

	LogOut(UserSystem);

	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Note the space is now out of date and does not have the new user in it's lists
	{
		auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		SpaceSystem->ExitSpace(
			[](const csp::systems::NullResult& Result)
			{
			});
	}

	LogOut(UserSystem);

	LogIn(UserSystem, PrimaryUserId);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceSystemTests, GeoLocationTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	GeoLocation InitialGeoLocation;
	InitialGeoLocation.Latitude	 = 1.1;
	InitialGeoLocation.Longitude = 2.2;

	float InitialOrientation = 90.0f;

	csp::common::Array<GeoLocation> InitialGeoFence(4);

	GeoLocation GeoFence0;
	GeoFence0.Latitude	= 5.5;
	GeoFence0.Longitude = 6.6;
	InitialGeoFence[0]	= GeoFence0;
	InitialGeoFence[3]	= GeoFence0;

	GeoLocation GeoFence1;
	GeoFence1.Latitude	= 7.7;
	GeoFence1.Longitude = 8.8;
	InitialGeoFence[1]	= GeoFence1;

	GeoLocation GeoFence2;
	GeoFence2.Latitude	= 9.9;
	GeoFence2.Longitude = 10.0;
	InitialGeoFence[2]	= GeoFence2;

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
	SecondGeoLocation.Latitude	= 3.3;
	SecondGeoLocation.Longitude = 4.4;

	float SecondOrientation = 270.0f;

	csp::common::Array<GeoLocation> SecondGeoFence(4);
	GeoFence0.Latitude	= 11.1;
	GeoFence0.Longitude = 12.2;
	SecondGeoFence[0]	= GeoFence0;
	SecondGeoFence[3]	= GeoFence0;
	GeoFence1.Latitude	= 13.3;
	GeoFence1.Longitude = 14.4;
	SecondGeoFence[1]	= GeoFence1;
	GeoFence2.Latitude	= 15.5;
	GeoFence2.Longitude = 16.6;
	SecondGeoFence[2]	= GeoFence2;

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	GeoLocation ValidGeoLocation;
	ValidGeoLocation.Latitude  = 1.1;
	ValidGeoLocation.Longitude = 2.2;

	GeoLocation InvalidGeoLocation;
	InvalidGeoLocation.Latitude	 = 500.0;
	InvalidGeoLocation.Longitude = 2.2;

	float ValidOrientation	 = 90.0f;
	float InvalidOrientation = 500.0f;

	csp::common::Array<GeoLocation> ValidGeoFence(4);
	csp::common::Array<GeoLocation> ShortGeoFence(2);
	csp::common::Array<GeoLocation> InvalidGeoFence(4);
	csp::common::Array<GeoLocation> InvalidGeoLocationGeoFence(4);
	GeoLocation GeoFence0;
	GeoFence0.Latitude	= 5.5;
	GeoFence0.Longitude = 6.6;
	GeoLocation GeoFence1;
	GeoFence1.Latitude	= 7.7;
	GeoFence1.Longitude = 8.8;
	GeoLocation GeoFence2;
	GeoFence2.Latitude	= 9.9;
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
		auto [AddGeoResult] = AWAIT_PRE(SpaceSystem,
										UpdateSpaceGeoLocation,
										RequestPredicate,
										Space.Id,
										ValidGeoLocation,
										ValidOrientation,
										InvalidGeoLocationGeoFence);

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
		auto [AddGeoResult] = AWAIT_PRE(SpaceSystem,
										UpdateSpaceGeoLocation,
										RequestPredicate,
										Space.Id,
										ValidGeoLocation,
										ValidOrientation,
										InvalidGeoLocationGeoFence);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Create a space as the primary user
	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Switch to the alt user to try and update the geo location
	LogOut(UserSystem);
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	GeoLocation InitialGeoLocation;
	InitialGeoLocation.Latitude	 = 1.1;
	InitialGeoLocation.Longitude = 2.2;

	float InitialOrientation = 90.0f;

	auto [AddGeoResultAsAlt]
		= AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

	EXPECT_EQ(AddGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
	EXPECT_EQ(AddGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

	// Switch back to the primary user to actually create the geo location
	LogOut(UserSystem);
	LogIn(UserSystem, PrimaryUserId);


	auto [AddGeoResultAsPrimary]
		= AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

	EXPECT_EQ(AddGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

	// Switch back to the alt user again
	LogOut(UserSystem);
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Test they cannot get the space geo location details since the space is private
	auto [GetGeoResultAsAlt] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

	EXPECT_EQ(GetGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
	EXPECT_EQ(GetGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

	// Test they cannot update the geolocation
	GeoLocation SecondGeoLocation;
	SecondGeoLocation.Latitude	= 3.3;
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
	LogIn(UserSystem, PrimaryUserId);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Create a space as the primary user
	String PrimaryUserId;
	LogIn(UserSystem, PrimaryUserId);
	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Public, nullptr, nullptr, nullptr, Space);

	// Switch to the alt user to try and update the geo location
	LogOut(UserSystem);
	String AltUserId;
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	GeoLocation InitialGeoLocation;
	InitialGeoLocation.Latitude	 = 1.1;
	InitialGeoLocation.Longitude = 2.2;

	float InitialOrientation = 90.0f;

	auto [AddGeoResultAsAlt]
		= AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

	EXPECT_EQ(AddGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Failed);
	EXPECT_EQ(AddGeoResultAsAlt.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));

	// Switch back to the primary user to actually create the geo location
	LogOut(UserSystem);
	LogIn(UserSystem, PrimaryUserId);

	auto [AddGeoResultAsPrimary]
		= AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, InitialGeoLocation, InitialOrientation, nullptr);

	EXPECT_EQ(AddGeoResultAsPrimary.GetResultCode(), csp::systems::EResultCode::Success);

	// Switch back to the alt user again
	LogOut(UserSystem);
	LogIn(UserSystem, AltUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Test they can get the space geo location details since the space is public
	auto [GetGeoResultAsAlt] = AWAIT_PRE(SpaceSystem, GetSpaceGeoLocation, RequestPredicate, Space.Id);

	EXPECT_EQ(GetGeoResultAsAlt.GetResultCode(), csp::systems::EResultCode::Success);
	EXPECT_TRUE(GetGeoResultAsAlt.HasSpaceGeoLocation());
	EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Location.Latitude, InitialGeoLocation.Latitude);
	EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Location.Longitude, InitialGeoLocation.Longitude);
	EXPECT_DOUBLE_EQ(GetGeoResultAsAlt.GetSpaceGeoLocation().Orientation, InitialOrientation);

	// Test they cannot update the geolocation
	GeoLocation SecondGeoLocation;
	SecondGeoLocation.Latitude	= 3.3;
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
	LogIn(UserSystem, PrimaryUserId);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "CSP-TEST-SPACE";
	const char* TestSpaceDescription = "CSP-TEST-SPACEDESC";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	Array<InviteUserRoleInfo> UserRoles(1);
	UserRoles[0].UserEmail = AlternativeLoginEmail;
	UserRoles[0].UserRole  = SpaceUserRole::User;
	InviteUserRoleInfoCollection InviteInfo;
	InviteInfo.InviteUserRoleInfos = UserRoles;

	::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, InviteInfo, nullptr, Space);

	// Log out and log in as alt user
	LogOut(UserSystem);
	LogIn(UserSystem, UserId, AlternativeLoginEmail, AlternativeLoginPassword);

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
	LogIn(UserSystem, UserId);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif
