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
#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include <iostream>

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

void WaitForCallback(bool& CallbackCalled)
{
	// Wait for message
	auto Start		 = std::chrono::steady_clock::now();
	auto Current	 = std::chrono::steady_clock::now();
	int64_t TestTime = 0;

	while (CallbackCalled == false && TestTime < 20)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
	}
}

} // namespace

void CreateHotspotgroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
						const csp::common::String& GroupName,
						const csp::common::Array<csp::common::String>& Items,
						csp::systems::HotspotGroup& OutSequence,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::HotspotSequenceSystem::CreateHotspotGroup, HotspotSequenceSystem, GroupName, Items).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::HotspotGroup group = Result.GetHotspotGroup();

		EXPECT_EQ(group.Name, GroupName);
		EXPECT_EQ(group.Items.Size(), Items.Size());

		for (int i = 0; i < group.Items.Size(); ++i)
		{
			EXPECT_EQ(group.Items[i], Items[i]);
		}

		OutSequence = group;
	}
}

void DeleteHotspotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
						const csp::common::String& GroupName,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::DeleteHotspotGroup, HotspotSequenceSystem, GroupName).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
}

void GetHotpotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
					const csp::common::String& GroupName,
					csp::systems::HotspotGroup& Group,
					csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
					csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::GetHotspotGroup, HotspotSequenceSystem, GroupName).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
	csp::systems::HotspotGroup group = Result.GetHotspotGroup();
	if (Result.GetResultCode() == csp::systems::EResultCode::Success)
	{
		Group = group;
	}
}

void UpdateHotspotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
						const csp::common::String& GroupName,
						const csp::common::Array<csp::common::String>& Items,
						csp::systems::HotspotGroup& HotspotGroup,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::HotspotSequenceSystem::UpdateHotspotGroup, HotspotSequenceSystem, GroupName, Items).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::HotspotGroup group = Result.GetHotspotGroup();

		EXPECT_EQ(group.Name, GroupName);
		EXPECT_EQ(group.Items.Size(), Items.Size());

		for (int i = 0; i < group.Items.Size(); ++i)
		{
			EXPECT_EQ(group.Items[i], Items[i]);
		}

		HotspotGroup = group;
	}
}

void RenameHotspotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
						const csp::common::String& GroupName,
						const csp::common::String& NewGroupName,
						csp::systems::HotspotGroup& HotspotGroup,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::HotspotSequenceSystem::RenameHotspotGroup, HotspotSequenceSystem, GroupName, NewGroupName).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::HotspotGroup group = Result.GetHotspotGroup();
		EXPECT_EQ(group.Name, NewGroupName);

		HotspotGroup = group;
	}
}


void GetHotspotGroups(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
					  const csp::common::Array<csp::common::String>& GroupNames,
					  csp::common::Array<csp::systems::HotspotGroup>& Groups,
					  csp::systems::EResultCode ExpectedResultCode					= csp::systems::EResultCode::Success,
					  csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::GetHotspotGroups, HotspotSequenceSystem).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	csp::common::Array<csp::systems::HotspotGroup> HotspotGroups = Result.GetHotspotGroups();
	Groups														 = HotspotGroups;
}

void CompareGroups(const csp::systems::HotspotGroup& S1, const csp::systems::HotspotGroup& S2)
{
	EXPECT_EQ(S1.Name, S2.Name);
	EXPECT_EQ(S1.Items.Size(), S2.Items.Size());
	if (S1.Items.Size() == S2.Items.Size())
	{
		for (int i = 0; i < S1.Items.Size(); ++i)
		{
			EXPECT_EQ(S1.Items[i], S2.Items[i]);
		}
	}
}

static constexpr const char* TestSpaceName		  = "CSP-UNITTEST-SPACE-MAG";
static constexpr const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_CREATE_HOTSPOTGROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, CreateHotspotGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE-MAG";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	char UniqueSpaceName[256];

	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	// Create hot spot group
	csp::common::Array<csp::common::String> GroupItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

	csp::systems::HotspotGroup HotspotGroup;
	CreateHotspotgroup(HotspotSystem, TestGroupName, GroupItems, HotspotGroup);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, TestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif


#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_GET_HOTSPOT_GROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	// Create hotspot group
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

	csp::systems::HotspotGroup HotspotGroup;
	CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotspotGroup);

	// Get the group we just created
	csp::systems::HotspotGroup RetrievedHotspotGroup;
	GetHotpotGroup(HotspotSystem, TestGroupName, RetrievedHotspotGroup);

	CompareGroups(HotspotGroup, RetrievedHotspotGroup);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, TestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_UPDATE_HOTSPOT_GROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, UpdateHotspotGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE-MAG";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	char UniqueSpaceName[256];

	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);


	// Create hotspot group
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2"};
	csp::common::Array<csp::common::String> NewItems {"Hotspot3"};
	csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

	const char* TestSequenceReferenceID = "CSP-UNITTEST-ReferenceID-MAG";

	csp::systems::HotspotGroup HotspotGroup1;
	csp::systems::HotspotGroup HotspotGroup2;

	CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotspotGroup1);

	csp::systems::HotspotGroup expected;
	expected.Name  = HotspotGroup1.Name;
	expected.Items = {"Hotspot3"};

	UpdateHotspotGroup(HotspotSystem, TestGroupName, NewItems, HotspotGroup2);
	CompareGroups(HotspotGroup2, expected);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, TestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_RENAME_HOTSPOT_GROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameHotspotGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE-MAG";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	char UniqueSpaceName[256];

	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);


	// Create hotspot group
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2"};
	csp::common::String OldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
	csp::common::String NewTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

	const char* TestSequenceReferenceID = "CSP-UNITTEST-ReferenceID-MAG";

	csp::systems::HotspotGroup HotspotGroup;

	CreateHotspotgroup(HotspotSystem, OldTestGroupName, SequenceItems, HotspotGroup);
	EXPECT_EQ(HotspotGroup.Name, OldTestGroupName);
	RenameHotspotGroup(HotspotSystem, OldTestGroupName, NewTestGroupName, HotspotGroup);
	EXPECT_EQ(HotspotGroup.Name, NewTestGroupName);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, NewTestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_RENAME_FAIL_HOTSPOT_GROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameFailHotspotGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE-MAG";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	char UniqueSpaceName[256];

	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);


	// Create hotspot group
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2"};
	csp::common::String OldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
	csp::common::String NewTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

	const char* TestSequenceReferenceID = "CSP-UNITTEST-ReferenceID-MAG";

	csp::systems::HotspotGroup HotspotGroup;

	csp::common::String expectedName = SpaceSystem->GetCurrentSpace().Id + ":" + NewTestGroupName;

	RenameHotspotGroup(HotspotSystem, OldTestGroupName, NewTestGroupName, HotspotGroup, csp::systems::EResultCode::Failed);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, NewTestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_GET_HOTSPOT_NO_GROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotNoGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	// Create hotspot group
	csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

	csp::systems::HotspotGroup HotspotGroup;

	// Get the sequence we know does not exist
	GetHotpotGroup(HotspotSystem, TestGroupName, HotspotGroup, csp::systems::EResultCode::Failed);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, TestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_GET_HOTSPOT_GROUPS_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotsGroupsTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result]				= AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	csp::common::String spaceID = {UniqueSpaceName};
	// Create hotspot group
	csp::common::Array<csp::common::String> SequenceItems1 {"Hotspot1"};
	csp::common::Array<csp::common::String> SequenceItems2 {"Hotspot1", "Hotspot2"};
	csp::common::Array<csp::common::String> SequenceItems3 {"Hotspot1", "Hotspot2", "Hotspot3"};
	csp::common::String TestGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG-1";
	csp::common::String TestGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG-2";
	csp::common::String TestGroupName3 = "CSP-UNITTEST-SEQUENCE-MAG-3";


	csp::systems::HotspotGroup HotspotGroup1;
	csp::systems::HotspotGroup HotspotGroup2;
	csp::systems::HotspotGroup HotspotGroup3;

	CreateHotspotgroup(HotspotSystem, TestGroupName1, SequenceItems1, HotspotGroup1);
	CreateHotspotgroup(HotspotSystem, TestGroupName2, SequenceItems2, HotspotGroup2);
	CreateHotspotgroup(HotspotSystem, TestGroupName3, SequenceItems3, HotspotGroup3);

	csp::common::Array<csp::systems::HotspotGroup> ExpectedGroups = {HotspotGroup1, HotspotGroup2, HotspotGroup3};
	csp::common::Array<csp::systems::HotspotGroup> RetrievedGroups;
	csp::common::Array<csp::common::String> SearchGroupNames = {TestGroupName1, TestGroupName2, TestGroupName3};
	csp::common::Array<csp::common::String> ExpectedGroupNames
		= {spaceID + ":" + TestGroupName1, spaceID + ":" + TestGroupName2, spaceID + ":" + TestGroupName3};
	// Get the sequence we just created
	GetHotspotGroups(HotspotSystem, ExpectedGroupNames, RetrievedGroups);
	EXPECT_EQ(RetrievedGroups.Size(), RetrievedGroups.Size());
	for (size_t i = 0; i < ExpectedGroups.Size(); i++)
	{
		CompareGroups(RetrievedGroups[i], ExpectedGroups[i]);
	}

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, HotspotGroup1.Name);
	DeleteHotspotGroup(HotspotSystem, HotspotGroup2.Name);
	DeleteHotspotGroup(HotspotSystem, HotspotGroup3.Name);
	// Delete space
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_DELETE_HOTSPOT_NO_GROUP_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, DeleteHotspotNoGroupTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	// Create hotspot group
	csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, TestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_HOTSPOTSEQUENCESYSTEM_TESTS || RUN_GENERATE_SEQUENCE_KEY_TEST
CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GenerateSequenceKeyTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* HotspotSystem	 = SystemsManager.GetHotspotSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
	csp::common::String spaceID		 = {UniqueSpaceName};
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);
	auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	// Create hotspot group
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

	csp::systems::HotspotGroup HotspotGroup;
	CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotspotGroup);

	EXPECT_EQ(TestGroupName, HotspotGroup.Name);

	// Delete sequence
	DeleteHotspotGroup(HotspotSystem, TestGroupName);
	SpaceSystem->ExitSpace(
		[](const csp::systems::NullResult& Result)
		{
		});
	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif