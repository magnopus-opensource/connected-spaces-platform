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
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

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

void CreateSequence(csp::systems::SequenceSystem* SequenceSystem,
					const csp::common::String& SequenceKey,
					const csp::common::String& ReferenceType,
					const csp::common::String& ReferenceId,
					const csp::common::Array<csp::common::String>& Items,
					csp::systems::Sequence& OutSequence,
					csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
					csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::SequenceSystem::CreateSequence, SequenceSystem, SequenceKey, ReferenceType, ReferenceId, Items)
						.Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::Sequence Sequence = Result.GetSequence();

		EXPECT_EQ(Sequence.Key, SequenceKey);
		EXPECT_EQ(Sequence.ReferenceType, ReferenceType);
		EXPECT_EQ(Sequence.ReferenceId, ReferenceId);
		EXPECT_EQ(Sequence.Items.Size(), Items.Size());

		for (int i = 0; i < Sequence.Items.Size(); ++i)
		{
			EXPECT_EQ(Sequence.Items[i], Items[i]);
		}

		OutSequence = Sequence;
	}
}

void DeleteSequences(csp::systems::SequenceSystem* SequenceSystem,
					 const csp::common::Array<csp::common::String>& SequenceKeys,
					 csp::systems::EResultCode ExpectedResultCode				   = csp::systems::EResultCode::Success,
					 csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::SequenceSystem::DeleteSequences, SequenceSystem, SequenceKeys).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
}

void GetSequence(csp::systems::SequenceSystem* SequenceSystem,
				 const csp::common::String& SequenceKey,
				 csp::systems::Sequence& OutSequence,
				 csp::systems::EResultCode ExpectedResultCode				   = csp::systems::EResultCode::Success,
				 csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::SequenceSystem::GetSequence, SequenceSystem, SequenceKey).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	csp::systems::Sequence Sequence = Result.GetSequence();

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		EXPECT_EQ(Sequence.Key, SequenceKey);

		OutSequence = Sequence;
	}
}

void UpdateSequence(csp::systems::SequenceSystem* SequenceSystem,
					const csp::common::String& SequenceKey,
					const csp::common::String& ReferenceType,
					const csp::common::String& ReferenceId,
					const csp::common::Array<csp::common::String>& Items,
					csp::systems::Sequence& OutSequence,
					csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
					csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::SequenceSystem::UpdateSequence, SequenceSystem, SequenceKey, ReferenceType, ReferenceId, Items)
						.Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::Sequence Sequence = Result.GetSequence();

		EXPECT_EQ(Sequence.Key, SequenceKey);
		EXPECT_EQ(Sequence.ReferenceType, ReferenceType);
		EXPECT_EQ(Sequence.ReferenceId, ReferenceId);
		EXPECT_EQ(Sequence.Items.Size(), Items.Size());

		for (int i = 0; i < Sequence.Items.Size(); ++i)
		{
			EXPECT_EQ(Sequence.Items[i], Items[i]);
		}

		OutSequence = Sequence;
	}
}

void RenameSequence(csp::systems::SequenceSystem* SequenceSystem,
					const csp::common::String& OldSequenceKey,
					const csp::common::String& NewSequenceKey,
					csp::systems::Sequence& OutSequence,
					csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
					csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::systems::SequenceSystem::RenameSequence, SequenceSystem, OldSequenceKey, NewSequenceKey).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		csp::systems::Sequence Sequence = Result.GetSequence();

		EXPECT_EQ(Sequence.Key, NewSequenceKey);

		OutSequence = Sequence;
	}
}

void GetSequencesByCriteria(csp::systems::SequenceSystem* SequenceSystem,
							const csp::common::Array<csp::common::String>& SequenceKeys,
							const csp::common::Optional<csp::common::String>& KeyRegex,
							const csp::common::Optional<csp::common::String>& ReferenceType,
							const csp::common::Array<csp::common::String>& ReferenceIds,
							csp::common::Array<csp::systems::Sequence>& OutSequences,
							csp::systems::EResultCode ExpectedResultCode				  = csp::systems::EResultCode::Success,
							csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::systems::SequenceSystem::GetSequencesByCriteria, SequenceSystem, SequenceKeys, KeyRegex, ReferenceType, ReferenceIds)
			  .Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	csp::common::Array<csp::systems::Sequence> Sequences = Result.GetSequences();
	OutSequences										 = Sequences;
}

void CompareSequences(const csp::systems::Sequence& S1, const csp::systems::Sequence& S2)
{
	EXPECT_EQ(S1.Key, S2.Key);
	EXPECT_EQ(S1.ReferenceType, S2.ReferenceType);
	EXPECT_EQ(S1.ReferenceId, S2.ReferenceId);
	EXPECT_EQ(S1.Items.Size(), S2.Items.Size());

	for (int i = 0; i < S1.Items.Size(); ++i)
	{
		EXPECT_EQ(S1.Items[i], S2.Items[i]);
	}
}

static constexpr const char* TestSpaceName		  = "CSP-UNITTEST-SPACE-MAG";
static constexpr const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_CREATESEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

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

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	// Delete sequence
	DeleteSequences(SequenceSystem, {Sequence.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_CREATESEQUENCENOITEMS_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceNoItemsTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

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

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems;
	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	// Delete sequence
	DeleteSequences(SequenceSystem, {Sequence.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_CREATESEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceNoSpaceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	const char* TestSequenceReferenceID = "CSP-UNITTEST-ReferenceID-MAG";

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "TesId", TestSequenceReferenceID, SequenceItems, Sequence);

	// Delete sequence
	DeleteSequences(SequenceSystem, {Sequence.Key});

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_GETSEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequenceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	// Get the sequence we just created
	csp::systems::Sequence RetrievedSequence;
	GetSequence(SequenceSystem, UniqueSequenceName, RetrievedSequence);

	CompareSequences(Sequence, RetrievedSequence);

	// Delete sequence
	DeleteSequences(SequenceSystem, {Sequence.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_UPDATESEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, UpdateSequenceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

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

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	// Update sequence
	csp::common::Array<csp::common::String> UpdatedSequenceItems {"Hotspot4", "Hotspot5"};

	csp::systems::Sequence UpdatedSequence;
	UpdateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, UpdatedSequenceItems, UpdatedSequence);

	// Delete sequence
	DeleteSequences(SequenceSystem, {UpdatedSequence.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_RENAMESEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, RenameSequenceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

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

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};

	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	// Rename sequence
	const char* TestUpdatedSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG-UPDATED";
	char UniqueUpdatedSequenceName[256];
	SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence UpdatedSequence;
	RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceName, UpdatedSequence);

	// Delete sequence
	DeleteSequences(SequenceSystem, {UpdatedSequence.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_GETSEQUENCEBYCRITERIA_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequencesByCriteriaTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

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

	// Create sequences
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};
	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "Group1", Space.Id, SequenceItems, Sequence);

	csp::common::Array<csp::common::String> SequenceItems2 {"Hotspot4", "Hotspot5", "Hotspot6"};
	const char* TestSequenceKey2 = "CSP-UNITTEST-SEQUENCE-MAG2";
	char UniqueSequenceName2[256];
	SPRINTF(UniqueSequenceName2, "%s-%s", TestSequenceKey2, GetUniqueString().c_str());

	csp::systems::Sequence Sequence2;
	CreateSequence(SequenceSystem, UniqueSequenceName2, "Group2", Space.Id, SequenceItems2, Sequence2);

	// Test searches
	csp::common::Array<csp::systems::Sequence> RetrievedSequences;

	// Test Sequence key search

	// Get the first sequence
	GetSequencesByCriteria(SequenceSystem, {Sequence.Key}, nullptr, nullptr, {}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 1);
	CompareSequences(RetrievedSequences[0], Sequence);

	// Get the second sequence
	GetSequencesByCriteria(SequenceSystem, {Sequence2.Key}, nullptr, nullptr, {}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 1);
	CompareSequences(RetrievedSequences[0], Sequence2);

	// Try and get an invalid sequence
	GetSequencesByCriteria(SequenceSystem, {"Unknown Key"}, nullptr, nullptr, {}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 0);

	// Test Regex search
	GetSequencesByCriteria(SequenceSystem, {}, UniqueSequenceName2, nullptr, {}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 1);
	CompareSequences(RetrievedSequences[0], Sequence2);

	// Test reference type and id search

	// Get the first sequence
	GetSequencesByCriteria(SequenceSystem, {}, nullptr, "Group1", {{Space.Id}}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 1);
	CompareSequences(RetrievedSequences[0], Sequence);

	// Get the second sequence
	GetSequencesByCriteria(SequenceSystem, {}, nullptr, "Group2", {{Space.Id}}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 1);
	CompareSequences(RetrievedSequences[0], Sequence2);

	// Try and get an invalid sequence
	GetSequencesByCriteria(SequenceSystem, {}, nullptr, "Group3", {{Space.Id}}, RetrievedSequences);
	EXPECT_EQ(RetrievedSequences.Size(), 0);

	// Delete sequence
	DeleteSequences(SequenceSystem, {Sequence.Key, Sequence2.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_REGISTERSEQUENCEUPDATED_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, RegisterSequenceUpdatedTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	bool CallbackCalled = false;

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};

	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	auto CreateCallback = [&CallbackCalled, &UniqueSequenceName](const csp::multiplayer::SequenceChangedParams& Params)
	{
		EXPECT_EQ(Params.Key, UniqueSequenceName);
		EXPECT_EQ(Params.UpdateType, csp::multiplayer::ESequenceUpdateType::Create);

		CallbackCalled = true;
	};

	Connection->SetSequenceChangedCallback(CreateCallback);

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	WaitForCallback(CallbackCalled);
	EXPECT_TRUE(CallbackCalled);

	// Rename sequence
	const char* TestUpdatedSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG-UPDATED";
	char UniqueUpdatedSequenceName[256];
	SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, GetUniqueString().c_str());

	auto UpdateCallback = [&CallbackCalled, &Sequence, &UniqueUpdatedSequenceName](const csp::multiplayer::SequenceChangedParams& Params)
	{
		EXPECT_EQ(Params.Key, Sequence.Key);
		EXPECT_EQ(Params.UpdateType, csp::multiplayer::ESequenceUpdateType::Update);
		EXPECT_EQ(Params.NewKey, std::string(UniqueUpdatedSequenceName).c_str());

		CallbackCalled = true;
	};

	Connection->SetSequenceChangedCallback(UpdateCallback);
	CallbackCalled = false;

	csp::systems::Sequence UpdatedSequence;
	RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceName, UpdatedSequence);

	WaitForCallback(CallbackCalled);
	EXPECT_TRUE(CallbackCalled);

	// Delete sequence
	auto DeleteCallback = [&CallbackCalled, &UniqueUpdatedSequenceName](const csp::multiplayer::SequenceChangedParams& Params)
	{
		EXPECT_EQ(Params.Key, UniqueUpdatedSequenceName);
		EXPECT_EQ(Params.UpdateType, csp::multiplayer::ESequenceUpdateType::Delete);

		CallbackCalled = true;
	};

	Connection->SetSequenceChangedCallback(DeleteCallback);
	CallbackCalled = false;

	DeleteSequences(SequenceSystem, {UpdatedSequence.Key});

	WaitForCallback(CallbackCalled);
	EXPECT_TRUE(CallbackCalled);

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

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_SEQUENCE_PERMISSIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, SequencePermissionsTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* SequenceSystem = SystemsManager.GetSequenceSystem();

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

	// Create sequence
	csp::common::Array<csp::common::String> SequenceItems {"Hotspot1", "Hotspot2", "Hotspot3"};

	const char* TestSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG";
	char UniqueSequenceName[256];
	SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

	csp::systems::Sequence Sequence;
	CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, Sequence);

	// Log out the user which created the sequence
	LogOut(UserSystem);

	// Login with another user
	LogIn(UserSystem, UserId, AlternativeLoginEmail, AlternativeLoginPassword, true);

	// Ensure we can still get the sequence from a space we are not an editor of
	csp::systems::Sequence RetrievedSequence;
	GetSequence(SequenceSystem, UniqueSequenceName, RetrievedSequence);

	// Try and edit the sequence from a space we are not an editor of

	// Update sequence
	csp::common::Array<csp::common::String> UpdatedSequenceItems {"Hotspot4", "Hotspot5"};

	csp::systems::Sequence UpdatedSequence;
	UpdateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, UpdatedSequenceItems, UpdatedSequence, csp::systems::EResultCode::Failed);

	// Rename sequence
	const char* TestUpdatedSequenceKey = "CSP-UNITTEST-SEQUENCE-MAG-UPDATED";
	char UniqueUpdatedSequenceName[256];
	SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, GetUniqueString().c_str());

	RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceName, UpdatedSequence, csp::systems::EResultCode::Failed);

	// Delete sequence
	DeleteSequences(SequenceSystem, {UpdatedSequence.Key}, csp::systems::EResultCode::Failed);

	// Log out
	LogOut(UserSystem);

	// Login again with the original user the cleanup
	LogIn(UserSystem, UserId);

	// Delete sequence
	DeleteSequences(SequenceSystem, {Sequence.Key});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif