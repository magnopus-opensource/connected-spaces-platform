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
#include "CSP/Systems/HotSpotSequence/HotSpotSequenceSystem.h"
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

void CreateHotspotgroup(csp::systems::HotSpotSequenceSystem* HotspotSequenceSystem,
						const csp::common::String& GroupName,
						const csp::common::Array<csp::common::String>& Items,
						csp::systems::HotSpotGroup& OutSequence,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::HotSpotSequenceSystem::CreateHotspotGroup, HotspotSequenceSystem, GroupName, Items).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::HotSpotGroup group = Result.GetHotSpotGroup();

		EXPECT_EQ(group.Items.Size(), Items.Size());

		for (int i = 0; i < group.Items.Size(); ++i)
		{
			EXPECT_EQ(group.Items[i], Items[i]);
		}

		OutSequence = group;
	}
}

void DeleteHotspotGroup(csp::systems::HotSpotSequenceSystem* HotSpotSequenceSystem,
						const csp::common::String& GroupName,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::HotSpotSequenceSystem::DeleteHotspotGroup, HotSpotSequenceSystem, GroupName).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
}

void GetHotpotGroup(csp::systems::HotSpotSequenceSystem* HotSpotSequenceSystem,
					const csp::common::String& GroupName,
					csp::systems::HotSpotGroup& Group,
					csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
					csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::HotSpotSequenceSystem::GetHotspotGroup, HotSpotSequenceSystem, GroupName).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
	csp::systems::HotSpotGroup group = Result.GetHotSpotGroup();
	if (Result.GetResultCode() == csp::systems::EResultCode::Success)
	{
		Group = group;
	}
}

void UpdateHotspotGroup(csp::systems::HotSpotSequenceSystem* HotSpotSequenceSystem,
						const csp::common::String& GroupName,
						const csp::common::Array<csp::common::String>& Items,
						csp::systems::HotSpotGroup& HotSpotGroup,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::HotSpotSequenceSystem::UpdateHotspotGroup, HotSpotSequenceSystem, GroupName, Items).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::HotSpotGroup group = Result.GetHotSpotGroup();

		EXPECT_EQ(group.Items.Size(), Items.Size());

		for (int i = 0; i < group.Items.Size(); ++i)
		{
			EXPECT_EQ(group.Items[i], Items[i]);
		}

		HotSpotGroup = group;
	}
}

void RenameHotspotGroup(csp::systems::HotSpotSequenceSystem* HotSpotSequenceSystem,
						const csp::common::String& GroupName,
						const csp::common::String& NewGroupName,
						csp::systems::HotSpotGroup& HotSpotGroup,
						csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
						csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::HotSpotSequenceSystem::RenameHotspotGroup, HotSpotSequenceSystem, GroupName, NewGroupName).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::HotSpotGroup group = Result.GetHotSpotGroup();
		HotSpotGroup					 = group;
	}
}


void GetHotspotGroups(csp::systems::HotSpotSequenceSystem* HotSpotSequenceSystem,
					  const csp::common::Array<csp::common::String>& GroupNames,
					  csp::common::Array<csp::systems::HotSpotGroup>& Groups,
					  csp::systems::EResultCode ExpectedResultCode					= csp::systems::EResultCode::Success,
					  csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::HotSpotSequenceSystem::GetHotspotGroups, HotSpotSequenceSystem).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	csp::common::Array<csp::systems::HotSpotGroup> HotSpotGroups = Result.GetHotSpotGroups();
	Groups														 = HotSpotGroups;
}

void CompareGroups(const csp::systems::HotSpotGroup& S1, const csp::systems::HotSpotGroup& S2)
{
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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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

	csp::systems::HotSpotGroup HotSpotGroup;
	CreateHotspotgroup(HotspotSystem, TestGroupName, GroupItems, HotSpotGroup);

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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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

	csp::systems::HotSpotGroup HotSpotGroup;
	CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotSpotGroup);

	// Get the group we just created
	csp::systems::HotSpotGroup RetrievedHotSpotGroup;
	GetHotpotGroup(HotspotSystem, TestGroupName, RetrievedHotSpotGroup);

	CompareGroups(HotSpotGroup, RetrievedHotSpotGroup);

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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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

	csp::systems::HotSpotGroup HotSpotGroup1;
	csp::systems::HotSpotGroup HotSpotGroup2;

	CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotSpotGroup1);

	csp::systems::HotSpotGroup expected;
	expected.Name  = HotSpotGroup1.Name;
	expected.Items = {"Hotspot3"};

	UpdateHotspotGroup(HotspotSystem, TestGroupName, NewItems, HotSpotGroup2);
	CompareGroups(HotSpotGroup2, expected);

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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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

	csp::systems::HotSpotGroup HotSpotGroup;

	CreateHotspotgroup(HotspotSystem, OldTestGroupName, SequenceItems, HotSpotGroup);
	csp::common::String expectedName = SpaceSystem->GetCurrentSpace().Id + ":" + NewTestGroupName;

	RenameHotspotGroup(HotspotSystem, OldTestGroupName, NewTestGroupName, HotSpotGroup);
	EXPECT_EQ(HotSpotGroup.Name, expectedName);

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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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

	csp::systems::HotSpotGroup HotSpotGroup;

	// Get the sequence we know does not exist
	GetHotpotGroup(HotspotSystem, TestGroupName, HotSpotGroup, csp::systems::EResultCode::Failed);

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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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


	csp::systems::HotSpotGroup HotSpotGroup1;
	csp::systems::HotSpotGroup HotSpotGroup2;
	csp::systems::HotSpotGroup HotSpotGroup3;

	CreateHotspotgroup(HotspotSystem, TestGroupName1, SequenceItems1, HotSpotGroup1);
	CreateHotspotgroup(HotspotSystem, TestGroupName2, SequenceItems2, HotSpotGroup2);
	CreateHotspotgroup(HotspotSystem, TestGroupName3, SequenceItems3, HotSpotGroup3);

	csp::common::Array<csp::systems::HotSpotGroup> ExpectedGroups = {HotSpotGroup1, HotSpotGroup2, HotSpotGroup3};
	csp::common::Array<csp::systems::HotSpotGroup> RetrievedGroups;
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
	DeleteHotspotGroup(HotspotSystem, HotSpotGroup1.Name);
	DeleteHotspotGroup(HotspotSystem, HotSpotGroup2.Name);
	DeleteHotspotGroup(HotspotSystem, HotSpotGroup3.Name);
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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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
	auto* HotspotSystem	 = SystemsManager.GetHotSpotSequenceSystem();

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

	csp::systems::HotSpotGroup HotSpotGroup;
	CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotSpotGroup);
	csp::common::String expectedName = SpaceSystem->GetCurrentSpace().Id + ":" + TestGroupName;

	EXPECT_EQ(expectedName, HotSpotGroup.Name);

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