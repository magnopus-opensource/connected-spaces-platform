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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateSequence(csp::systems::SequenceSystem* SequenceSystem, const csp::common::String& SequenceKey, const csp::common::String& ReferenceType,
    const csp::common::String& ReferenceId, const csp::common::Array<csp::common::String>& Items,
    csp::common::Map<csp::common::String, csp::common::String> MetaData, csp::systems::Sequence& OutSequence,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::CreateSequence, SequenceSystem, SequenceKey, ReferenceType, ReferenceId, Items, MetaData)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);
    if (ExpectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::Sequence Sequence = Result.GetSequence();

        EXPECT_EQ(Sequence.Key, SequenceKey);
        EXPECT_EQ(Sequence.ReferenceType, ReferenceType);
        EXPECT_EQ(Sequence.ReferenceId, ReferenceId);
        EXPECT_EQ(Sequence.Items.Size(), Items.Size());
        auto Keys = Sequence.MetaData.Keys();
        for (size_t i = 0; i < Keys->Size(); i++)
        {
            EXPECT_EQ(Sequence.MetaData[(*Keys)[i]], MetaData[(*Keys)[i]]);
        }

        for (int i = 0; i < Sequence.Items.Size(); ++i)
        {
            EXPECT_EQ(Sequence.Items[i], Items[i]);
        }

        OutSequence = Sequence;
    }
}

void DeleteSequences(csp::systems::SequenceSystem* SequenceSystem, const csp::common::Array<csp::common::String>& SequenceKeys,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::DeleteSequences, SequenceSystem, SequenceKeys).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);
}

void GetSequence(csp::systems::SequenceSystem* SequenceSystem, const csp::common::String& SequenceKey, csp::systems::Sequence& OutSequence,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::GetSequence, SequenceSystem, SequenceKey).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);

    csp::systems::Sequence Sequence = Result.GetSequence();

    if (ExpectedResultCode == csp::systems::EResultCode::Success)
    {
        EXPECT_EQ(Sequence.Key, SequenceKey);

        OutSequence = Sequence;
    }
}

void UpdateSequence(csp::systems::SequenceSystem* SequenceSystem, const csp::common::String& SequenceKey, const csp::common::String& ReferenceType,
    const csp::common::String& ReferenceId, const csp::common::Array<csp::common::String>& Items,
    csp::common::Map<csp::common::String, csp::common::String> MetaData, csp::systems::Sequence& OutSequence,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::UpdateSequence, SequenceSystem, SequenceKey, ReferenceType, ReferenceId, Items, MetaData)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);

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
        auto Keys = Sequence.MetaData.Keys();
        for (size_t i = 0; i < Keys->Size(); i++)
        {
            EXPECT_EQ(Sequence.MetaData[(*Keys)[i]], MetaData[(*Keys)[i]]);
        }
        OutSequence = Sequence;
    }
}

void RenameSequence(csp::systems::SequenceSystem* SequenceSystem, const csp::common::String& OldSequenceKey,
    const csp::common::String& NewSequenceKey, csp::systems::Sequence& OutSequence,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::RenameSequence, SequenceSystem, OldSequenceKey, NewSequenceKey).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);

    if (ExpectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::Sequence Sequence = Result.GetSequence();

        EXPECT_EQ(Sequence.Key, NewSequenceKey);

        OutSequence = Sequence;
    }
}

void GetSequencesByCriteria(csp::systems::SequenceSystem* SequenceSystem, const csp::common::Array<csp::common::String>& SequenceKeys,
    const csp::common::Optional<csp::common::String>& KeyRegex, const csp::common::Optional<csp::common::String>& ReferenceType,
    const csp::common::Array<csp::common::String>& ReferenceIds, csp::common::Array<csp::systems::Sequence>& OutSequences,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    csp::common::Map<csp::common::String, csp::common::String> MetaData;
    auto [Result] = Awaitable(
        &csp::systems::SequenceSystem::GetSequencesByCriteria, SequenceSystem, SequenceKeys, KeyRegex, ReferenceType, ReferenceIds, MetaData)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);

    csp::common::Array<csp::systems::Sequence> Sequences = Result.GetSequences();
    OutSequences = Sequences;
}

void GetAllSequencesContainingItems(csp::systems::SequenceSystem* SequenceSystem, const csp::common::Array<csp::common::String>& InItems,
    const csp::common::Optional<csp::common::String>& InReferenceType, const csp::common::Array<csp::common::String>& InReferenceIds,
    csp::common::Array<csp::systems::Sequence>& OutSequences, csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t ExpectedHTTPResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::GetAllSequencesContainingItems, SequenceSystem, InItems, InReferenceType, InReferenceIds)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), ExpectedHTTPResponseCode);

    csp::common::Array<csp::systems::Sequence> Sequences = Result.GetSequences();
    OutSequences = Sequences;

    // Search all sequences
    for (size_t i = 0; i < OutSequences.Size(); ++i)
    {
        const csp::systems::Sequence& Sequence = OutSequences[i];
        bool Found = false;

        // Search all items in sequence
        for (size_t j = 0; j < Sequence.Items.Size(); ++j)
        {
            const csp::common::String FoundItem = Sequence.Items[j];

            // Search all searched items to find a match
            for (size_t k = 0; k < InItems.Size(); ++k)
            {
                const csp::common::String SearchedItem = InItems[k];

                if (FoundItem == SearchedItem)
                {
                    Found = true;
                }
            }
        }

        EXPECT_TRUE(Found);
    }
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
} // namespace

static constexpr const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
static constexpr const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_CREATESEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    // Create sequence with reserved characters in the sequenceID (which is allowed).
    const char* TestReservedCharsSequenceKey = "CSP UNITTEST SEQUENCE MAG";
    char UniqueReservedCharsSequenceName[256];
    SPRINTF(UniqueReservedCharsSequenceName, "%s-%s", TestReservedCharsSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence ReservedCharsSequence;
    CreateSequence(SequenceSystem, UniqueReservedCharsSequenceName, "GroupId", Space.Id, SequenceItems, {}, ReservedCharsSequence,
        csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 200);

    // Delete sequences
    DeleteSequences(SequenceSystem, { Sequence.Key, ReservedCharsSequence.Key }, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_CREATESEQUENCE_INVALIDKEY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Any attempt to get a sequence with key containing an / or % will result in a failure.
    // Create sequence with / character
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* TestSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Create sequence with % in the name
    const char* TestSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char UniqueSequenceNameMod[256];
    SPRINTF(UniqueSequenceNameMod, "%s-%s", TestSequenceKeyMod, GetUniqueString().c_str());
    CreateSequence(SequenceSystem, UniqueSequenceNameMod, "GroupId", Space.Id, SequenceItems, {}, Sequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems;
    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence);

    // Delete sequence
    DeleteSequences(SequenceSystem, { Sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    const char* TestSequenceReferenceID = "CSP-UNITTEST-ReferenceID-MAG";

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "TesId", TestSequenceReferenceID, SequenceItems, {}, Sequence);

    // Delete sequence
    DeleteSequences(SequenceSystem, { Sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_GETSEQUENCE_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequenceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    // Note that the sequence key uses reserved characters.
    // We expect CSP to correctly handle the encoding and decoding of these characters for us.
    const char* TestSequenceKey = "**CSP UNITTEST SEQUENCE MAG**";

    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence);

    // Get the sequence we just created
    csp::systems::Sequence RetrievedSequence;
    GetSequence(SequenceSystem, UniqueSequenceName, RetrievedSequence);

    CompareSequences(Sequence, RetrievedSequence);

    // Delete sequence
    DeleteSequences(SequenceSystem, { Sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_GETSEQUENCE_INVALIDKEY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    // Any attempt to get a sequence with key containing an / or % will result in a failure.

    // Get sequence with invalid / key
    const char* TestSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char UniqueSequenceName[256];
    std::string Unique = GetUniqueString();
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, Unique.c_str());

    csp::systems::Sequence RetrievedSequence;
    GetSequence(SequenceSystem, UniqueSequenceName, RetrievedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // get sequence with invalid % key
    const char* TestSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char UniqueSequenceNameMod[256];
    SPRINTF(UniqueSequenceNameMod, "%s-%s", TestSequenceKeyMod, Unique.c_str());

    GetSequence(SequenceSystem, UniqueSequenceNameMod, RetrievedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);
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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    csp::common::Map<csp::common::String, csp::common::String> MetaData;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, MetaData, Sequence);

    // Update sequence
    csp::common::Array<csp::common::String> UpdatedSequenceItems { "Hotspot4", "Hotspot5" };

    csp::systems::Sequence UpdatedSequence;
    MetaData["Foo"] = "Bar";
    UpdateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, UpdatedSequenceItems, MetaData, UpdatedSequence);

    // Delete sequence
    DeleteSequences(SequenceSystem, { UpdatedSequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_UPDATESEQUENCE_INVALIDKEY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, UpdateSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    const char* TestSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());
    const char* TestSequenceKeySpace = "CSP-UNITTEST SEQUENCE-MAG";
    char UniqueSequenceNameSpace[256];
    SPRINTF(UniqueSequenceNameSpace, "%s-%s", TestSequenceKeySpace, GetUniqueString().c_str());
    const char* TestSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char UniqueSequenceNameMod[256];
    SPRINTF(UniqueSequenceNameMod, "%s-%s", TestSequenceKeyMod, GetUniqueString().c_str());

    csp::common::Map<csp::common::String, csp::common::String> MetaData;

    // Update sequence
    csp::common::Array<csp::common::String> UpdatedSequenceItems { "Hotspot4", "Hotspot5" };

    // Any attempt to get a sequence with key containing an / or % will result in a failure
    csp::systems::Sequence UpdatedSequence;
    MetaData["Foo"] = "Bar";

    // Verify cannot update sequence with a key that contains /
    UpdateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, UpdatedSequenceItems, MetaData, UpdatedSequence,
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Verify cannot update sequence with a key that contains %
    UpdateSequence(SequenceSystem, UniqueSequenceNameMod, "GroupId", Space.Id, UpdatedSequenceItems, MetaData, UpdatedSequence,
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence);

    // Rename sequence
    const char* TestUpdatedSequenceKey = "*CSP UNITTEST SEQUENCE MAG*-UPDATED";
    char UniqueUpdatedSequenceName[256];
    SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence UpdatedSequence;
    RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceName, UpdatedSequence);

    // Delete sequence
    DeleteSequences(SequenceSystem, { UpdatedSequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_RENAMESEQUENCE_INVALIDKEY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, RenameSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence);
    std::string UniqueString = GetUniqueString();

    // Any attempt to get a sequence with key containing an / or % will result in a failure
    // Rename sequence
    const char* TestUpdatedSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG-UPDATED";
    char UniqueUpdatedSequenceName[256];
    SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, UniqueString.c_str());

    const char* TestUpdatedSequenceKeySpace = "CSP-UNITTEST SEQUENCE-MAG-UPDATED";
    char UniqueUpdatedSequenceNameSpace[256];
    SPRINTF(UniqueUpdatedSequenceNameSpace, "%s-%s", TestUpdatedSequenceKeySpace, UniqueString.c_str());

    const char* TestUpdatedSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG-UPDATED";
    char UniqueUpdatedSequenceNameMod[256];
    SPRINTF(UniqueUpdatedSequenceNameMod, "%s-%s", TestUpdatedSequenceKeyMod, UniqueString.c_str());

    csp::systems::Sequence UpdatedSequence;

    // sequence name with a / fails
    RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceName, UpdatedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // sequence name with a % fails
    RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceNameMod, UpdatedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Delete sequence
    DeleteSequences(SequenceSystem, { UniqueSequenceName }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequences
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "Group1", Space.Id, SequenceItems, {}, Sequence);

    csp::common::Array<csp::common::String> SequenceItems2 { "Hotspot4", "Hotspot5", "Hotspot6" };
    const char* TestSequenceKey2 = "*CSP UNITTEST SEQUENCE MAG*2";
    char UniqueSequenceName2[256];
    SPRINTF(UniqueSequenceName2, "%s-%s", TestSequenceKey2, GetUniqueString().c_str());

    csp::systems::Sequence Sequence2;
    CreateSequence(SequenceSystem, UniqueSequenceName2, "Group2", Space.Id, SequenceItems2, {}, Sequence2);

    // Test searches
    csp::common::Array<csp::systems::Sequence> RetrievedSequences;

    // Test Sequence key search

    // Get the first sequence
    GetSequencesByCriteria(SequenceSystem, { Sequence.Key }, nullptr, nullptr, {}, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 1);
    CompareSequences(RetrievedSequences[0], Sequence);

    // Get the second sequence
    GetSequencesByCriteria(SequenceSystem, { Sequence2.Key }, nullptr, nullptr, {}, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 1);
    CompareSequences(RetrievedSequences[0], Sequence2);

    // Try and get an invalid sequence
    GetSequencesByCriteria(SequenceSystem, { "Unknown_Key" }, nullptr, nullptr, {}, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 0);

    // Test Regex search
    GetSequencesByCriteria(SequenceSystem, {}, UniqueSequenceName2, nullptr, {}, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 1);
    CompareSequences(RetrievedSequences[0], Sequence2);

    // Test reference type and id search

    // Get the first sequence
    GetSequencesByCriteria(SequenceSystem, {}, nullptr, "Group1", { { Space.Id } }, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 1);
    CompareSequences(RetrievedSequences[0], Sequence);

    // Get the second sequence
    GetSequencesByCriteria(SequenceSystem, {}, nullptr, "Group2", { { Space.Id } }, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 1);
    CompareSequences(RetrievedSequences[0], Sequence2);

    // Try and get an invalid sequence
    GetSequencesByCriteria(SequenceSystem, {}, nullptr, "Group3", { { Space.Id } }, RetrievedSequences);
    EXPECT_EQ(RetrievedSequences.Size(), 0);

    // Delete sequence
    DeleteSequences(
        SequenceSystem, { Sequence.Key, Sequence2.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_GETSEQUENCEBYCRITERIA_INVALIDKEY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequencesByCriteriaInvalidKeyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Test searches
    csp::common::Array<csp::systems::Sequence> RetrievedSequences;

    // Test Sequence key search
    const char* TestSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());
    const char* TestSequenceKeySpace = "CSP-UNITTEST SEQUENCE-MAG";
    char UniqueSequenceNameSpace[256];
    SPRINTF(UniqueSequenceNameSpace, "%s-%s", TestSequenceKeySpace, GetUniqueString().c_str());
    const char* TestSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char UniqueSequenceNameMod[256];
    SPRINTF(UniqueSequenceNameMod, "%s-%s", TestSequenceKeyMod, GetUniqueString().c_str());

    // Any attempt to get a sequence with key containing an / or % will result in a failure.
    // verify get fails when using a key name with a / character
    GetSequencesByCriteria(SequenceSystem, { UniqueSequenceName }, nullptr, nullptr, {}, RetrievedSequences, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // verify get fails when using a key name with a % character
    GetSequencesByCriteria(SequenceSystem, { UniqueSequenceNameMod }, nullptr, nullptr, {}, RetrievedSequences, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    bool CallbackCalled = false;

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    auto CreateCallback = [&CallbackCalled, &UniqueSequenceName](const csp::multiplayer::SequenceChangedParams& Params)
    {
        EXPECT_EQ(Params.Key, UniqueSequenceName);
        EXPECT_EQ(Params.UpdateType, csp::multiplayer::ESequenceUpdateType::Create);

        CallbackCalled = true;
    };

    SequenceSystem->SetSequenceChangedCallback(CreateCallback);

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence);

    WaitForCallback(CallbackCalled);
    EXPECT_TRUE(CallbackCalled);

    // Rename sequence
    const char* TestUpdatedSequenceKey = "*CSP UNITTEST SEQUENCE MAG*-UPDATED";
    char UniqueUpdatedSequenceName[256];
    SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, GetUniqueString().c_str());

    auto UpdateCallback = [&CallbackCalled, &Sequence, &UniqueUpdatedSequenceName](const csp::multiplayer::SequenceChangedParams& Params)
    {
        EXPECT_EQ(Params.UpdateType, csp::multiplayer::ESequenceUpdateType::Update);
        EXPECT_EQ(Params.Key, std::string(UniqueUpdatedSequenceName).c_str());

        CallbackCalled = true;
    };

    SequenceSystem->SetSequenceChangedCallback(UpdateCallback);
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

    SequenceSystem->SetSequenceChangedCallback(DeleteCallback);
    CallbackCalled = false;

    DeleteSequences(SequenceSystem, { UpdatedSequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    WaitForCallback(CallbackCalled);
    EXPECT_TRUE(CallbackCalled);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    csp::systems::Profile DefaultUser = CreateTestUser();
    LogIn(UserSystem, UserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequence
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* TestSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName[256];
    SPRINTF(UniqueSequenceName, "%s-%s", TestSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, SequenceItems, {}, Sequence);

    // Log out the user which created the sequence
    LogOut(UserSystem);

    // Login with another user
    LogInAsNewTestUser(UserSystem, UserId, true);

    // Ensure we can still get the sequence from a space we are not an editor of
    csp::systems::Sequence RetrievedSequence;
    GetSequence(SequenceSystem, UniqueSequenceName, RetrievedSequence);

    // Try and edit the sequence from a space we are not an editor of

    // Update sequence
    csp::common::Array<csp::common::String> UpdatedSequenceItems { "Hotspot4", "Hotspot5" };

    csp::systems::Sequence UpdatedSequence;
    UpdateSequence(SequenceSystem, UniqueSequenceName, "GroupId", Space.Id, UpdatedSequenceItems, {}, UpdatedSequence,
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::None, 403);

    // Rename sequence
    const char* TestUpdatedSequenceKey = "*CSP UNITTEST SEQUENCE MAG*-UPDATED";
    char UniqueUpdatedSequenceName[256];
    SPRINTF(UniqueUpdatedSequenceName, "%s-%s", TestUpdatedSequenceKey, GetUniqueString().c_str());

    RenameSequence(SequenceSystem, UniqueSequenceName, UniqueUpdatedSequenceName, UpdatedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::None, 403);

    // Delete sequence
    DeleteSequences(SequenceSystem, { UpdatedSequence.Key }, csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::None, 400);

    // Log out
    LogOut(UserSystem);

    // Login again with the original user the cleanup
    LogIn(UserSystem, UserId, DefaultUser.Email, GeneratedTestAccountPassword);

    // Delete sequence
    DeleteSequences(SequenceSystem, { Sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCESYSTEM_TESTS || RUN_SEQUENCESYSTEM_GETALLSEQUENCESCONTAINING_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetAllSequencesContainingItemsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* SequenceSystem = SystemsManager.GetSequenceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];

    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create sequences
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* TestSequenceKey1 = "CSP UNITTEST SEQUENCE MAG*";
    char UniqueSequenceName1[256];
    SPRINTF(UniqueSequenceName1, "%s-%s", TestSequenceKey1, GetUniqueString().c_str());

    csp::systems::Sequence Sequence;
    CreateSequence(SequenceSystem, UniqueSequenceName1, "GroupId", Space.Id, SequenceItems, {}, Sequence, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    const char* TestSequenceKey2 = "CSP UNITTEST SEQUENCE MAG2";
    char UniqueSequenceName2[256];
    SPRINTF(UniqueSequenceName2, "%s-%s", TestSequenceKey2, GetUniqueString().c_str());

    csp::systems::Sequence Sequence2;
    CreateSequence(SequenceSystem, UniqueSequenceName2, "GroupId", Space.Id, SequenceItems, {}, Sequence2, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    csp::common::Array<csp::common::String> SequenceItems3 { "Hotspot1", "Hotspot2" };

    const char* TestSequenceKey3 = "CSP UNITTEST SEQUENCE MAG3";
    char UniqueSequenceName3[256];
    SPRINTF(UniqueSequenceName3, "%s-%s", TestSequenceKey3, GetUniqueString().c_str());

    csp::systems::Sequence Sequence3;
    CreateSequence(SequenceSystem, UniqueSequenceName3, "GroupId", Space.Id, SequenceItems3, {}, Sequence3, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    csp::common::Array<csp::systems::Sequence> FoundSequences;
    GetAllSequencesContainingItems(SequenceSystem, { "Hotspot3" }, "GroupId", { Space.Id }, FoundSequences);

    EXPECT_EQ(FoundSequences.Size(), 2);

    bool FoundSequence1 = false;
    bool FoundSequence2 = false;

    for (size_t i = 0; i < FoundSequences.Size(); ++i)
    {
        if (FoundSequences[i].Key == UniqueSequenceName1)
        {
            FoundSequence1 = true;
        }
        else if (FoundSequences[i].Key == UniqueSequenceName2)
        {
            FoundSequence2 = true;
        }
    }

    EXPECT_TRUE(FoundSequence1);
    EXPECT_TRUE(FoundSequence2);

    // Delete sequences
    DeleteSequences(SequenceSystem, { Sequence.Key, Sequence2.Key, Sequence3.Key }, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif