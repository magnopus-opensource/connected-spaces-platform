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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateSequence(csp::systems::SequenceSystem* sequenceSystem, const csp::common::String& sequenceKey, const csp::common::String& referenceType,
    const csp::common::String& referenceId, const csp::common::Array<csp::common::String>& items,
    csp::common::Map<csp::common::String, csp::common::String> metaData, csp::systems::Sequence& outSequence,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::CreateSequence, sequenceSystem, sequenceKey, referenceType, referenceId, items, metaData)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);
    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::Sequence sequence = Result.GetSequence();

        EXPECT_EQ(sequence.Key, sequenceKey);
        EXPECT_EQ(sequence.ReferenceType, referenceType);
        EXPECT_EQ(sequence.ReferenceId, referenceId);
        EXPECT_EQ(sequence.Items.Size(), items.Size());
        auto keys = sequence.MetaData.Keys();
        for (size_t i = 0; i < keys->Size(); i++)
        {
            EXPECT_EQ(sequence.MetaData[(*keys)[i]], metaData[(*keys)[i]]);
        }

        for (size_t i = 0; i < sequence.Items.Size(); ++i)
        {
            EXPECT_EQ(sequence.Items[i], items[i]);
        }

        outSequence = sequence;
    }
}

void DeleteSequences(csp::systems::SequenceSystem* sequenceSystem, const csp::common::Array<csp::common::String>& sequenceKeys,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::DeleteSequences, sequenceSystem, sequenceKeys).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);
}

void GetSequence(csp::systems::SequenceSystem* sequenceSystem, const csp::common::String& sequenceKey, csp::systems::Sequence& outSequence,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::GetSequence, sequenceSystem, sequenceKey).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);

    csp::systems::Sequence sequence = Result.GetSequence();

    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        EXPECT_EQ(sequence.Key, sequenceKey);

        outSequence = sequence;
    }
}

void UpdateSequence(csp::systems::SequenceSystem* sequenceSystem, const csp::common::String& sequenceKey, const csp::common::String& referenceType,
    const csp::common::String& referenceId, const csp::common::Array<csp::common::String>& items,
    csp::common::Map<csp::common::String, csp::common::String> metaData, csp::systems::Sequence& outSequence,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::UpdateSequence, sequenceSystem, sequenceKey, referenceType, referenceId, items, metaData)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);

    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::Sequence sequence = Result.GetSequence();

        EXPECT_EQ(sequence.Key, sequenceKey);
        EXPECT_EQ(sequence.ReferenceType, referenceType);
        EXPECT_EQ(sequence.ReferenceId, referenceId);
        EXPECT_EQ(sequence.Items.Size(), items.Size());

        for (size_t i = 0; i < sequence.Items.Size(); ++i)
        {
            EXPECT_EQ(sequence.Items[i], items[i]);
        }
        auto keys = sequence.MetaData.Keys();
        for (size_t i = 0; i < keys->Size(); i++)
        {
            EXPECT_EQ(sequence.MetaData[(*keys)[i]], metaData[(*keys)[i]]);
        }
        outSequence = sequence;
    }
}

void RenameSequence(csp::systems::SequenceSystem* sequenceSystem, const csp::common::String& oldSequenceKey,
    const csp::common::String& newSequenceKey, csp::systems::Sequence& outSequence,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::RenameSequence, sequenceSystem, oldSequenceKey, newSequenceKey).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);

    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::Sequence sequence = Result.GetSequence();

        EXPECT_EQ(sequence.Key, newSequenceKey);

        outSequence = sequence;
    }
}

void GetSequencesByCriteria(csp::systems::SequenceSystem* sequenceSystem, const csp::common::Array<csp::common::String>& sequenceKeys,
    const csp::common::Optional<csp::common::String>& keyRegex, const csp::common::Optional<csp::common::String>& referenceType,
    const csp::common::Array<csp::common::String>& referenceIds, csp::common::Array<csp::systems::Sequence>& outSequences,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    csp::common::Map<csp::common::String, csp::common::String> metaData;
    auto [Result] = Awaitable(
        &csp::systems::SequenceSystem::GetSequencesByCriteria, sequenceSystem, sequenceKeys, keyRegex, referenceType, referenceIds, metaData)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);

    csp::common::Array<csp::systems::Sequence> sequences = Result.GetSequences();
    outSequences = sequences;
}

void GetAllSequencesContainingItems(csp::systems::SequenceSystem* sequenceSystem, const csp::common::Array<csp::common::String>& inItems,
    const csp::common::Optional<csp::common::String>& inReferenceType, const csp::common::Array<csp::common::String>& inReferenceIds,
    csp::common::Array<csp::systems::Sequence>& outSequences, csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None,
    uint16_t expectedHttpResponseCode = 200)
{
    auto [Result] = Awaitable(&csp::systems::SequenceSystem::GetAllSequencesContainingItems, sequenceSystem, inItems, inReferenceType, inReferenceIds)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    EXPECT_EQ(Result.GetHttpResultCode(), expectedHttpResponseCode);

    csp::common::Array<csp::systems::Sequence> sequences = Result.GetSequences();
    outSequences = sequences;

    // Search all sequences
    for (size_t i = 0; i < outSequences.Size(); ++i)
    {
        const csp::systems::Sequence& sequence = outSequences[i];
        bool found = false;

        // Search all items in sequence
        for (size_t j = 0; j < sequence.Items.Size(); ++j)
        {
            const csp::common::String foundItem = sequence.Items[j];

            // Search all searched items to find a match
            for (size_t k = 0; k < inItems.Size(); ++k)
            {
                const csp::common::String searchedItem = inItems[k];

                if (foundItem == searchedItem)
                {
                    found = true;
                }
            }
        }

        EXPECT_TRUE(found);
    }
}

void CompareSequences(const csp::systems::Sequence& s1, const csp::systems::Sequence& s2)
{
    EXPECT_EQ(s1.Key, s2.Key);
    EXPECT_EQ(s1.ReferenceType, s2.ReferenceType);
    EXPECT_EQ(s1.ReferenceId, s2.ReferenceId);
    EXPECT_EQ(s1.Items.Size(), s2.Items.Size());

    for (size_t i = 0; i < s1.Items.Size(); ++i)
    {
        EXPECT_EQ(s1.Items[i], s2.Items[i]);
    }
}
} // namespace

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    // Create sequence with reserved characters in the sequenceID (which is allowed).
    const char* testReservedCharsSequenceKey = "CSP UNITTEST SEQUENCE MAG";
    char uniqueReservedCharsSequenceName[256];
    SPRINTF(uniqueReservedCharsSequenceName, "%s-%s", testReservedCharsSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence reservedCharsSequence;
    CreateSequence(sequenceSystem, uniqueReservedCharsSequenceName, "GroupId", space.Id, sequenceItems, {}, reservedCharsSequence,
        csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 200);

    // Delete sequences
    DeleteSequences(sequenceSystem, { sequence.Key, reservedCharsSequence.Key }, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Any attempt to get a sequence with key containing an / or % will result in a failure.
    // Create sequence with / character
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* testSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Create sequence with % in the name
    const char* testSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char uniqueSequenceNameMod[256];
    SPRINTF(uniqueSequenceNameMod, "%s-%s", testSequenceKeyMod, GetUniqueString().c_str());
    CreateSequence(sequenceSystem, uniqueSequenceNameMod, "GroupId", space.Id, sequenceItems, {}, sequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceNoItemsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems;
    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence);

    // Delete sequence
    DeleteSequences(sequenceSystem, { sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, CreateSequenceNoSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    const char* testSequenceReferenceId = "CSP-UNITTEST-ReferenceID-MAG";

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "TesId", testSequenceReferenceId, sequenceItems, {}, sequence);

    // Delete sequence
    DeleteSequences(sequenceSystem, { sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequenceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    // Note that the sequence key uses reserved characters.
    // We expect CSP to correctly handle the encoding and decoding of these characters for us.
    const char* testSequenceKey = "**CSP UNITTEST SEQUENCE MAG**";

    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence);

    // Get the sequence we just created
    csp::systems::Sequence retrievedSequence;
    GetSequence(sequenceSystem, uniqueSequenceName, retrievedSequence);

    CompareSequences(sequence, retrievedSequence);

    // Delete sequence
    DeleteSequences(sequenceSystem, { sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    // Any attempt to get a sequence with key containing an / or % will result in a failure.

    // Get sequence with invalid / key
    const char* testSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char uniqueSequenceName[256];
    std::string unique = GetUniqueString();
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, unique.c_str());

    csp::systems::Sequence retrievedSequence;
    GetSequence(sequenceSystem, uniqueSequenceName, retrievedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // get sequence with invalid % key
    const char* testSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char uniqueSequenceNameMod[256];
    SPRINTF(uniqueSequenceNameMod, "%s-%s", testSequenceKeyMod, unique.c_str());

    GetSequence(sequenceSystem, uniqueSequenceNameMod, retrievedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, UpdateSequenceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    csp::common::Map<csp::common::String, csp::common::String> metaData;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, metaData, sequence);

    // Update sequence
    csp::common::Array<csp::common::String> updatedSequenceItems { "Hotspot4", "Hotspot5" };

    csp::systems::Sequence updatedSequence;
    metaData["Foo"] = "Bar";
    UpdateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, updatedSequenceItems, metaData, updatedSequence);

    // Delete sequence
    DeleteSequences(sequenceSystem, { updatedSequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, UpdateSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    const char* testSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());
    const char* testSequenceKeySpace = "CSP-UNITTEST SEQUENCE-MAG";
    char uniqueSequenceNameSpace[256];
    SPRINTF(uniqueSequenceNameSpace, "%s-%s", testSequenceKeySpace, GetUniqueString().c_str());
    const char* testSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char uniqueSequenceNameMod[256];
    SPRINTF(uniqueSequenceNameMod, "%s-%s", testSequenceKeyMod, GetUniqueString().c_str());

    csp::common::Map<csp::common::String, csp::common::String> metaData;

    // Update sequence
    csp::common::Array<csp::common::String> updatedSequenceItems { "Hotspot4", "Hotspot5" };

    // Any attempt to get a sequence with key containing an / or % will result in a failure
    csp::systems::Sequence updatedSequence;
    metaData["Foo"] = "Bar";

    // Verify cannot update sequence with a key that contains /
    UpdateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, updatedSequenceItems, metaData, updatedSequence,
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Verify cannot update sequence with a key that contains %
    UpdateSequence(sequenceSystem, uniqueSequenceNameMod, "GroupId", space.Id, updatedSequenceItems, metaData, updatedSequence,
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, RenameSequenceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence);

    // Rename sequence
    const char* testUpdatedSequenceKey = "*CSP UNITTEST SEQUENCE MAG*-UPDATED";
    char uniqueUpdatedSequenceName[256];
    SPRINTF(uniqueUpdatedSequenceName, "%s-%s", testUpdatedSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence updatedSequence;
    RenameSequence(sequenceSystem, uniqueSequenceName, uniqueUpdatedSequenceName, updatedSequence);

    // Delete sequence
    DeleteSequences(sequenceSystem, { updatedSequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, RenameSequenceInvalidKeyTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence);
    std::string uniqueString = GetUniqueString();

    // Any attempt to get a sequence with key containing an / or % will result in a failure
    // Rename sequence
    const char* testUpdatedSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG-UPDATED";
    char uniqueUpdatedSequenceName[256];
    SPRINTF(uniqueUpdatedSequenceName, "%s-%s", testUpdatedSequenceKey, uniqueString.c_str());

    const char* testUpdatedSequenceKeySpace = "CSP-UNITTEST SEQUENCE-MAG-UPDATED";
    char uniqueUpdatedSequenceNameSpace[256];
    SPRINTF(uniqueUpdatedSequenceNameSpace, "%s-%s", testUpdatedSequenceKeySpace, uniqueString.c_str());

    const char* testUpdatedSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG-UPDATED";
    char uniqueUpdatedSequenceNameMod[256];
    SPRINTF(uniqueUpdatedSequenceNameMod, "%s-%s", testUpdatedSequenceKeyMod, uniqueString.c_str());

    csp::systems::Sequence updatedSequence;

    // sequence name with a / fails
    RenameSequence(sequenceSystem, uniqueSequenceName, uniqueUpdatedSequenceName, updatedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // sequence name with a % fails
    RenameSequence(sequenceSystem, uniqueSequenceName, uniqueUpdatedSequenceNameMod, updatedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Delete sequence
    DeleteSequences(sequenceSystem, { uniqueSequenceName }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequencesByCriteriaTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequences
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "Group1", space.Id, sequenceItems, {}, sequence);

    csp::common::Array<csp::common::String> sequenceItems2 { "Hotspot4", "Hotspot5", "Hotspot6" };
    const char* testSequenceKey2 = "*CSP UNITTEST SEQUENCE MAG*2";
    char uniqueSequenceName2[256];
    SPRINTF(uniqueSequenceName2, "%s-%s", testSequenceKey2, GetUniqueString().c_str());

    csp::systems::Sequence sequence2;
    CreateSequence(sequenceSystem, uniqueSequenceName2, "Group2", space.Id, sequenceItems2, {}, sequence2);

    // Test searches
    csp::common::Array<csp::systems::Sequence> retrievedSequences;

    // Test Sequence key search

    // Get the first sequence
    GetSequencesByCriteria(sequenceSystem, { sequence.Key }, nullptr, nullptr, {}, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 1);
    CompareSequences(retrievedSequences[0], sequence);

    // Get the second sequence
    GetSequencesByCriteria(sequenceSystem, { sequence2.Key }, nullptr, nullptr, {}, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 1);
    CompareSequences(retrievedSequences[0], sequence2);

    // Try and get an invalid sequence
    GetSequencesByCriteria(sequenceSystem, { "Unknown_Key" }, nullptr, nullptr, {}, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 0);

    // Test Regex search
    GetSequencesByCriteria(sequenceSystem, {}, uniqueSequenceName2, nullptr, {}, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 1);
    CompareSequences(retrievedSequences[0], sequence2);

    // Test reference type and id search

    // Get the first sequence
    GetSequencesByCriteria(sequenceSystem, {}, nullptr, "Group1", { { space.Id } }, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 1);
    CompareSequences(retrievedSequences[0], sequence);

    // Get the second sequence
    GetSequencesByCriteria(sequenceSystem, {}, nullptr, "Group2", { { space.Id } }, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 1);
    CompareSequences(retrievedSequences[0], sequence2);

    // Try and get an invalid sequence
    GetSequencesByCriteria(sequenceSystem, {}, nullptr, "Group3", { { space.Id } }, retrievedSequences);
    EXPECT_EQ(retrievedSequences.Size(), 0);

    // Delete sequence
    DeleteSequences(
        sequenceSystem, { sequence.Key, sequence2.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetSequencesByCriteriaInvalidKeyTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Test searches
    csp::common::Array<csp::systems::Sequence> retrievedSequences;

    // Test Sequence key search
    const char* testSequenceKey = "CSP-UNITTEST/SEQUENCE-MAG";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());
    const char* testSequenceKeySpace = "CSP-UNITTEST SEQUENCE-MAG";
    char uniqueSequenceNameSpace[256];
    SPRINTF(uniqueSequenceNameSpace, "%s-%s", testSequenceKeySpace, GetUniqueString().c_str());
    const char* testSequenceKeyMod = "CSP-UNITTEST%SEQUENCE-MAG";
    char uniqueSequenceNameMod[256];
    SPRINTF(uniqueSequenceNameMod, "%s-%s", testSequenceKeyMod, GetUniqueString().c_str());

    // Any attempt to get a sequence with key containing an / or % will result in a failure.
    // verify get fails when using a key name with a / character
    GetSequencesByCriteria(sequenceSystem, { uniqueSequenceName }, nullptr, nullptr, {}, retrievedSequences, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // verify get fails when using a key name with a % character
    GetSequencesByCriteria(sequenceSystem, { uniqueSequenceNameMod }, nullptr, nullptr, {}, retrievedSequences, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::InvalidSequenceKey, 0);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, RegisterSequenceUpdatedTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    bool callbackCalled = false;

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    auto createCallback = [&callbackCalled, &uniqueSequenceName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
    {
        EXPECT_EQ(networkEventData.Key, uniqueSequenceName);
        EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Create);
        EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Default);

        callbackCalled = true;
    };

    sequenceSystem->SetSequenceChangedCallback(createCallback);

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence);

    WaitForCallback(callbackCalled);
    EXPECT_TRUE(callbackCalled);

    // Rename sequence
    const char* testUpdatedSequenceKey = "*CSP UNITTEST SEQUENCE MAG*-UPDATED";
    char uniqueUpdatedSequenceName[256];
    SPRINTF(uniqueUpdatedSequenceName, "%s-%s", testUpdatedSequenceKey, GetUniqueString().c_str());

    auto updateCallback
        = [&callbackCalled, &sequence, &uniqueUpdatedSequenceName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
    {
        EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Update);
        EXPECT_EQ(networkEventData.NewKey, std::string(uniqueUpdatedSequenceName).c_str());
        EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Default);

        callbackCalled = true;
    };

    sequenceSystem->SetSequenceChangedCallback(updateCallback);
    callbackCalled = false;

    csp::systems::Sequence updatedSequence;
    RenameSequence(sequenceSystem, uniqueSequenceName, uniqueUpdatedSequenceName, updatedSequence);

    WaitForCallback(callbackCalled);
    EXPECT_TRUE(callbackCalled);

    // Delete sequence
    auto deleteCallback = [&callbackCalled, &uniqueUpdatedSequenceName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
    {
        EXPECT_EQ(networkEventData.Key, uniqueUpdatedSequenceName);
        EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Delete);
        EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Default);

        callbackCalled = true;
    };

    sequenceSystem->SetSequenceChangedCallback(deleteCallback);
    callbackCalled = false;

    DeleteSequences(sequenceSystem, { updatedSequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    WaitForCallback(callbackCalled);
    EXPECT_TRUE(callbackCalled);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, SequencePermissionsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    csp::systems::Profile defaultUser = CreateTestUser();
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequence
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* testSequenceKey = "*CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName[256];
    SPRINTF(uniqueSequenceName, "%s-%s", testSequenceKey, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, sequenceItems, {}, sequence);

    // Log out the user which created the sequence
    LogOut(userSystem);

    // Login with another user
    LogInAsNewTestUser(userSystem, userId, true, true);

    // Ensure we can still get the sequence from a space we are not an editor of
    csp::systems::Sequence retrievedSequence;
    GetSequence(sequenceSystem, uniqueSequenceName, retrievedSequence);

    // Try and edit the sequence from a space we are not an editor of

    // Update sequence
    csp::common::Array<csp::common::String> updatedSequenceItems { "Hotspot4", "Hotspot5" };

    csp::systems::Sequence updatedSequence;
    UpdateSequence(sequenceSystem, uniqueSequenceName, "GroupId", space.Id, updatedSequenceItems, {}, updatedSequence,
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::None, 403);

    // Rename sequence
    const char* testUpdatedSequenceKey = "*CSP UNITTEST SEQUENCE MAG*-UPDATED";
    char uniqueUpdatedSequenceName[256];
    SPRINTF(uniqueUpdatedSequenceName, "%s-%s", testUpdatedSequenceKey, GetUniqueString().c_str());

    RenameSequence(sequenceSystem, uniqueSequenceName, uniqueUpdatedSequenceName, updatedSequence, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::None, 403);

    // Delete sequence
    DeleteSequences(sequenceSystem, { updatedSequence.Key }, csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::None, 400);

    // Log out
    LogOut(userSystem);

    // Login again with the original user the cleanup
    LogIn(userSystem, userId, defaultUser.Email, GeneratedTestAccountPassword);

    // Delete sequence
    DeleteSequences(sequenceSystem, { sequence.Key }, csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceSystemTests, GetAllSequencesContainingItemsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* sequenceSystem = systemsManager.GetSequenceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];

    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create sequences
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };

    const char* testSequenceKey1 = "CSP UNITTEST SEQUENCE MAG*";
    char uniqueSequenceName1[256];
    SPRINTF(uniqueSequenceName1, "%s-%s", testSequenceKey1, GetUniqueString().c_str());

    csp::systems::Sequence sequence;
    CreateSequence(sequenceSystem, uniqueSequenceName1, "GroupId", space.Id, sequenceItems, {}, sequence, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    const char* testSequenceKey2 = "CSP UNITTEST SEQUENCE MAG2";
    char uniqueSequenceName2[256];
    SPRINTF(uniqueSequenceName2, "%s-%s", testSequenceKey2, GetUniqueString().c_str());

    csp::systems::Sequence sequence2;
    CreateSequence(sequenceSystem, uniqueSequenceName2, "GroupId", space.Id, sequenceItems, {}, sequence2, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    csp::common::Array<csp::common::String> sequenceItems3 { "Hotspot1", "Hotspot2" };

    const char* testSequenceKey3 = "CSP UNITTEST SEQUENCE MAG3";
    char uniqueSequenceName3[256];
    SPRINTF(uniqueSequenceName3, "%s-%s", testSequenceKey3, GetUniqueString().c_str());

    csp::systems::Sequence sequence3;
    CreateSequence(sequenceSystem, uniqueSequenceName3, "GroupId", space.Id, sequenceItems3, {}, sequence3, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 200);

    csp::common::Array<csp::systems::Sequence> foundSequences;
    GetAllSequencesContainingItems(sequenceSystem, { "Hotspot3" }, "GroupId", { space.Id }, foundSequences);

    EXPECT_EQ(foundSequences.Size(), 2);

    bool foundSequence1 = false;
    bool foundSequence2 = false;

    for (size_t i = 0; i < foundSequences.Size(); ++i)
    {
        if (foundSequences[i].Key == uniqueSequenceName1)
        {
            foundSequence1 = true;
        }
        else if (foundSequences[i].Key == uniqueSequenceName2)
        {
            foundSequence2 = true;
        }
    }

    EXPECT_TRUE(foundSequence1);
    EXPECT_TRUE(foundSequence2);

    // Delete sequences
    DeleteSequences(sequenceSystem, { sequence.Key, sequence2.Key, sequence3.Key }, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None, 204);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}