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
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include <iostream>

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void CreateHotspotgroup(csp::systems::HotspotSequenceSystem* hotspotSequenceSystem, const csp::common::String& groupName,
    const csp::common::Array<csp::common::String>& items, csp::systems::HotspotGroup& outSequence,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result]
        = Awaitable(&csp::systems::HotspotSequenceSystem::CreateHotspotGroup, hotspotSequenceSystem, groupName, items).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);

    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::HotspotGroup group = Result.GetHotspotGroup();

        EXPECT_EQ(group.Name, groupName);
        EXPECT_EQ(group.Items.Size(), items.Size());

        for (size_t i = 0; i < group.Items.Size(); ++i)
        {
            EXPECT_EQ(group.Items[i], items[i]);
        }

        outSequence = group;
    }
}

void DeleteHotspotGroup(csp::systems::HotspotSequenceSystem* hotspotSequenceSystem, const csp::common::String& groupName,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::DeleteHotspotGroup, hotspotSequenceSystem, groupName).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
}

void GetHotpotGroup(csp::systems::HotspotSequenceSystem* hotspotSequenceSystem, const csp::common::String& groupName,
    csp::systems::HotspotGroup& group, csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::GetHotspotGroup, hotspotSequenceSystem, groupName).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);
    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        group = Result.GetHotspotGroup();
    }
}

void UpdateHotspotGroup(csp::systems::HotspotSequenceSystem* hotspotSequenceSystem, const csp::common::String& groupName,
    const csp::common::Array<csp::common::String>& items, csp::systems::HotspotGroup& hotspotGroup,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result]
        = Awaitable(&csp::systems::HotspotSequenceSystem::UpdateHotspotGroup, hotspotSequenceSystem, groupName, items).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);

    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::HotspotGroup group = Result.GetHotspotGroup();

        EXPECT_EQ(group.Name, groupName);
        EXPECT_EQ(group.Items.Size(), items.Size());

        for (size_t i = 0; i < group.Items.Size(); ++i)
        {
            EXPECT_EQ(group.Items[i], items[i]);
        }

        hotspotGroup = group;
    }
}

void RenameHotspotGroup(csp::systems::HotspotSequenceSystem* hotspotSequenceSystem, const csp::common::String& groupName,
    const csp::common::String& newGroupName, csp::systems::HotspotGroup& hotspotGroup,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result]
        = Awaitable(&csp::systems::HotspotSequenceSystem::RenameHotspotGroup, hotspotSequenceSystem, groupName, newGroupName).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);

    if (expectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::HotspotGroup group = Result.GetHotspotGroup();
        hotspotGroup = group;
    }
}

void CompareGroups(const csp::systems::HotspotGroup& s1, const csp::systems::HotspotGroup& s2)
{
    EXPECT_EQ(s1.Name, s2.Name);
    EXPECT_EQ(s1.Items.Size(), s2.Items.Size());
    if (s1.Items.Size() == s2.Items.Size())
    {
        for (size_t i = 0; i < s1.Items.Size(); ++i)
        {
            EXPECT_EQ(s1.Items[i], s2.Items[i]);
        }
    }
}

void GetHotspotGroups(csp::systems::HotspotSequenceSystem* hotspotSequenceSystem,
    const csp::common::Optional<csp::common::Array<csp::systems::HotspotGroup>>& expectedGroups,
    csp::common::Array<csp::systems::HotspotGroup>& groups, csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::GetHotspotGroups, hotspotSequenceSystem).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);

    csp::common::Array<csp::systems::HotspotGroup> hotspotGroups = Result.GetHotspotGroups();

    if (Result.GetResultCode() == csp::systems::EResultCode::Success && expectedGroups.HasValue())
    {
        EXPECT_EQ(hotspotGroups.Size(), (*expectedGroups).Size());
        for (size_t i = 0; i < (*expectedGroups).Size(); i++)
        {
            CompareGroups(hotspotGroups[i], (*expectedGroups)[i]);
        }
    }

    groups = hotspotGroups;
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, CreateHotspotGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    // Create hot spot group
    csp::common::Array<csp::common::String> groupItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String testGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    // Validate sequence creation events.
    bool callbackCalled = false;
    hotspotSystem->SetHotspotSequenceChangedCallback(
        [&callbackCalled, &space, &testGroupName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
        {
            EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Create);
            EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
            EXPECT_EQ(networkEventData.SpaceId, space.Id);
            EXPECT_EQ(networkEventData.Key, testGroupName);
            callbackCalled = true;
        });

    csp::systems::HotspotGroup hotspotGroup;
    CreateHotspotgroup(hotspotSystem, testGroupName, groupItems, hotspotGroup);

    WaitForCallback(callbackCalled);
    callbackCalled = false;

    // Validate sequence deletion events.
    hotspotSystem->SetHotspotSequenceChangedCallback(
        [&callbackCalled, &space, &testGroupName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
        {
            EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Delete);
            EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
            EXPECT_EQ(networkEventData.SpaceId, space.Id);
            EXPECT_EQ(networkEventData.Key, testGroupName);
            callbackCalled = true;
        });

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, testGroupName);

    // Clear out the callback as we have validated what we came here for.
    WaitForCallback(callbackCalled);
    hotspotSystem->SetHotspotSequenceChangedCallback(nullptr);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String testGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup hotspotGroup;
    CreateHotspotgroup(hotspotSystem, testGroupName, sequenceItems, hotspotGroup);

    // Get the group we just created
    csp::systems::HotspotGroup retrievedHotspotGroup;
    GetHotpotGroup(hotspotSystem, testGroupName, retrievedHotspotGroup);

    CompareGroups(hotspotGroup, retrievedHotspotGroup);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, testGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, UpdateHotspotGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::Array<csp::common::String> newItems { "Hotspot3" };
    csp::common::String testGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup hotspotGroup1;
    csp::systems::HotspotGroup hotspotGroup2;

    CreateHotspotgroup(hotspotSystem, testGroupName, sequenceItems, hotspotGroup1);

    csp::systems::HotspotGroup expected;
    expected.Name = hotspotGroup1.Name;
    expected.Items = { "Hotspot3" };

    UpdateHotspotGroup(hotspotSystem, testGroupName, newItems, hotspotGroup2);
    CompareGroups(hotspotGroup2, expected);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, testGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameHotspotGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::String oldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String newTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

    csp::systems::HotspotGroup hotspotGroup;
    CreateHotspotgroup(hotspotSystem, oldTestGroupName, sequenceItems, hotspotGroup);

    EXPECT_EQ(hotspotGroup.Name, oldTestGroupName);

    hotspotSystem->SetHotspotSequenceChangedCallback(
        [&space, &oldTestGroupName, &newTestGroupName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
        {
            // callback will be triggered when calling RenameHotspotGroup with event type update
            if (networkEventData.UpdateType == csp::common::ESequenceUpdateType::Update)
            {
                EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Update);
                EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
                EXPECT_EQ(networkEventData.Key, oldTestGroupName);
                EXPECT_EQ(networkEventData.NewKey, newTestGroupName);
                EXPECT_EQ(networkEventData.SpaceId, space.Id);
            }
        });

    RenameHotspotGroup(hotspotSystem, oldTestGroupName, newTestGroupName, hotspotGroup);

    EXPECT_EQ(hotspotGroup.Name, newTestGroupName);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, newTestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameHotspotGroupPersistantTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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

    // Create hotspot group
    csp::systems::HotspotGroup hotspotGroup;
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::String oldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String newTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

        auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        CreateHotspotgroup(hotspotSystem, oldTestGroupName, sequenceItems, hotspotGroup);

        EXPECT_EQ(hotspotGroup.Name, oldTestGroupName);

        hotspotSystem->SetHotspotSequenceChangedCallback(
            [&space, &oldTestGroupName, &newTestGroupName](const csp::common::SequenceChangedNetworkEventData& networkEventData)
            {
                // callback will be triggered when calling RenameHotspotGroup with event type update
                if (networkEventData.UpdateType == csp::common::ESequenceUpdateType::Update)
                {
                    EXPECT_EQ(networkEventData.UpdateType, csp::common::ESequenceUpdateType::Update);
                    EXPECT_EQ(networkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
                    EXPECT_EQ(networkEventData.Key, oldTestGroupName);
                    EXPECT_EQ(networkEventData.NewKey, newTestGroupName);
                    EXPECT_EQ(networkEventData.SpaceId, space.Id);
                }
            });

        RenameHotspotGroup(hotspotSystem, oldTestGroupName, newTestGroupName, hotspotGroup);

        EXPECT_EQ(hotspotGroup.Name, newTestGroupName);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Renter the Space and get all hotspot groups to ensure the change to the HotspotGroup name persists
    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

        auto [Result] = AWAIT(spaceSystem, EnterSpace, space.Id, realtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        csp::common::Array<csp::systems::HotspotGroup> retrievedGroups;
        csp::common::Array<csp::systems::HotspotGroup> expectedGroups = { hotspotGroup };

        // Get all hotspot sequences in the Space
        GetHotspotGroups(hotspotSystem, expectedGroups, retrievedGroups);

        // Ensure the group we previously created has the correct (new) name
        EXPECT_EQ(retrievedGroups[0].Name, newTestGroupName);

        // Delete sequence
        DeleteHotspotGroup(hotspotSystem, newTestGroupName);
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        EXPECT_FALSE(spaceSystem->IsInSpace());
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameFailHotspotGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::String oldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String newTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

    csp::systems::HotspotGroup hotspotGroup;

    csp::common::String expectedName = spaceSystem->GetCurrentSpace().Id + ":" + newTestGroupName;

    RenameHotspotGroup(hotspotSystem, oldTestGroupName, newTestGroupName, hotspotGroup, csp::systems::EResultCode::Failed);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, newTestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotNoGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    // Create hotspot group
    csp::common::String testGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup hotspotGroup;

    // Get the sequence we know does not exist
    GetHotpotGroup(hotspotSystem, testGroupName, hotspotGroup, csp::systems::EResultCode::Failed);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, testGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotsGroupsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    csp::common::String spaceID = { uniqueSpaceName };
    // Create hotspot group
    csp::common::Array<csp::common::String> sequenceItems1 { "Hotspot1" };
    csp::common::Array<csp::common::String> sequenceItems2 { "Hotspot1", "Hotspot2" };
    csp::common::Array<csp::common::String> sequenceItems3 { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String testGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG-1";
    csp::common::String testGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG-2";
    csp::common::String testGroupName3 = "CSP-UNITTEST-SEQUENCE-MAG-3";

    csp::systems::HotspotGroup hotspotGroup1;
    csp::systems::HotspotGroup hotspotGroup2;
    csp::systems::HotspotGroup hotspotGroup3;

    CreateHotspotgroup(hotspotSystem, testGroupName1, sequenceItems1, hotspotGroup1);
    CreateHotspotgroup(hotspotSystem, testGroupName2, sequenceItems2, hotspotGroup2);
    CreateHotspotgroup(hotspotSystem, testGroupName3, sequenceItems3, hotspotGroup3);

    csp::common::Array<csp::systems::HotspotGroup> expectedGroups = { hotspotGroup1, hotspotGroup2, hotspotGroup3 };
    csp::common::Array<csp::systems::HotspotGroup> retrievedGroups;
    csp::common::Array<csp::common::String> searchGroupNames = { testGroupName1, testGroupName2, testGroupName3 };

    // Get the sequence we just created
    GetHotspotGroups(hotspotSystem, expectedGroups, retrievedGroups);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, hotspotGroup1.Name);
    DeleteHotspotGroup(hotspotSystem, hotspotGroup2.Name);
    DeleteHotspotGroup(hotspotSystem, hotspotGroup3.Name);
    // Delete space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, DeleteHotspotNoGroupTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

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
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    // Create hotspot group
    csp::common::String testGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, testGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GenerateSequenceKeyTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    csp::common::String spaceID = { uniqueSpaceName };
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    // Create hotspot group
    csp::common::Array<csp::common::String> sequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String testGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup hotspotGroup;
    CreateHotspotgroup(hotspotSystem, testGroupName, sequenceItems, hotspotGroup);

    EXPECT_EQ(testGroupName, hotspotGroup.Name);

    // Delete sequence
    DeleteHotspotGroup(hotspotSystem, testGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, DeleteHotspotComponentTest)
{
    // Tests the deletion of corresponding sequences when the HotspotComponent is deleted.
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    // Create object to represent the hotspot
    csp::common::String objectName = "Object 1";
    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> { });

    bool componentAdded = false;

    CreatedObject->SetUpdateCallback(
        [&componentAdded, objectName](csp::multiplayer::SpaceEntity* entity, csp::multiplayer::SpaceEntityUpdateFlags /*Flags*/,
            csp::common::Array<csp::multiplayer::ComponentUpdateInfo>& updateInfo)
        {
            if (entity->GetName() == objectName)
            {
                for (size_t i = 0; i < updateInfo.Size(); ++i)
                {
                    if (updateInfo[i].UpdateType == csp::multiplayer::ComponentUpdateType::Add)
                    {
                        componentAdded = true;
                    }
                }
            }
        });

    // Create hotspot component
    auto* hotspotComponent
        = static_cast<csp::multiplayer::HotspotSpaceComponent*>(CreatedObject->AddComponent(csp::multiplayer::ComponentType::Hotspot));

    CreatedObject->QueueUpdate();
    WaitForCallbackWithUpdate(componentAdded, realtimeEngine.get());

    EXPECT_TRUE(componentAdded);

    // Create Hotspot groups
    csp::common::String testGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String testGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG2";
    csp::common::String testItemName = "AnotherItem";

    csp::systems::HotspotGroup hotspotGroup1;
    csp::systems::HotspotGroup hotspotGroup2;

    {
        // Create 2 groups that contains the component

        // Create one with only a single item to test deletion functionality
        CreateHotspotgroup(hotspotSystem, testGroupName1, { hotspotComponent->GetUniqueComponentId() }, hotspotGroup1);

        // Create one with an additional item to test update functionality
        CreateHotspotgroup(hotspotSystem, testGroupName2, { hotspotComponent->GetUniqueComponentId(), testItemName }, hotspotGroup2);

        // Ensure the 2 groups are created correctly
        csp::common::Array<csp::systems::HotspotGroup> foundGroups;
        csp::common::Array<csp::systems::HotspotGroup> expectedGroups = { hotspotGroup1, hotspotGroup2 };

        GetHotspotGroups(hotspotSystem, expectedGroups, foundGroups);
    }

    // Remove component
    {
        bool sequenceDeleted = false;
        bool sequenceUpdate = false;
        bool sequencesUpdated = false;

        auto cb = [testGroupName1, testGroupName2, &sequenceDeleted, &sequenceUpdate, &sequencesUpdated](
                      const csp::common::SequenceChangedNetworkEventData& networkEventData)
        {
            if (networkEventData.Key == testGroupName1 && networkEventData.UpdateType == csp::common::ESequenceUpdateType::Delete)
            {
                // Ensure we delete the group which only has one item
                sequenceDeleted = true;
            }
            else if (networkEventData.Key == testGroupName2 && networkEventData.UpdateType == csp::common::ESequenceUpdateType::Update)
            {
                // Ensure we update the sequence that has multiple items
                sequenceUpdate = true;
            }

            sequencesUpdated = sequenceDeleted && sequenceUpdate;
        };

        hotspotSystem->SetHotspotSequenceChangedCallback(cb);

        CreatedObject->RemoveComponent(hotspotComponent->GetId());

        // Delete the hotspot from the sequence, has to be done explicitly
        hotspotSystem->RemoveItemFromGroups(hotspotComponent->GetUniqueComponentId(), [](const csp::systems::NullResult /*Result*/) { });

        CreatedObject->QueueUpdate();

        WaitForCallbackWithUpdate(sequencesUpdated, realtimeEngine.get());

        EXPECT_TRUE(sequencesUpdated);
    }

    // 1 group should be deleted, and one should have its key removed
    {
        csp::common::Array<csp::systems::HotspotGroup> remainingGroups;
        GetHotspotGroups(hotspotSystem, nullptr, remainingGroups);

        EXPECT_EQ(remainingGroups.Size(), 1);
        EXPECT_EQ(remainingGroups[0].Items.Size(), 1);
        EXPECT_EQ(remainingGroups[0].Items[0], testItemName);
    }

    // Delete remaining group
    DeleteHotspotGroup(hotspotSystem, testGroupName2);

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, SequencePersistenceTest)
{
    // Ensures hotspot sequences still exist when re-entering a space
    // This tests that the ComponentBase::OnLocalDelete is only called when actually deleting a component
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* hotspotSystem = systemsManager.GetHotspotSequenceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    // Create object to represent the hotspot
    csp::common::String objectName = "Object 1";
    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> { });

    bool componentAdded = false;

    CreatedObject->SetUpdateCallback(
        [&componentAdded, objectName](csp::multiplayer::SpaceEntity* entity, csp::multiplayer::SpaceEntityUpdateFlags /*Flags*/,
            csp::common::Array<csp::multiplayer::ComponentUpdateInfo>& updateInfo)
        {
            if (entity->GetName() == objectName)
            {
                for (size_t i = 0; i < updateInfo.Size(); ++i)
                {
                    if (updateInfo[i].UpdateType == csp::multiplayer::ComponentUpdateType::Add)
                    {
                        componentAdded = true;
                    }
                }
            }
        });

    // Create hotspot component
    auto* hotspotComponent
        = static_cast<csp::multiplayer::HotspotSpaceComponent*>(CreatedObject->AddComponent(csp::multiplayer::ComponentType::Hotspot));

    CreatedObject->QueueUpdate();
    WaitForCallbackWithUpdate(componentAdded, realtimeEngine.get());

    EXPECT_TRUE(componentAdded);

    // Create Hotspot groups
    csp::common::String testGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String testGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG2";
    csp::common::String testGroupName3 = "CSP-UNITTEST-SEQUENCE-MAG3";

    csp::systems::HotspotGroup hotspotGroup1;
    csp::systems::HotspotGroup hotspotGroup2;
    csp::systems::HotspotGroup hotspotGroup3;

    {
        // Create 2 groups that contains the component
        CreateHotspotgroup(hotspotSystem, testGroupName1, { hotspotComponent->GetUniqueComponentId() }, hotspotGroup1);
        CreateHotspotgroup(hotspotSystem, testGroupName2, { hotspotComponent->GetUniqueComponentId() }, hotspotGroup2);

        // Create another group that doesnt contain the component
        CreateHotspotgroup(hotspotSystem, testGroupName3, { "TestName" }, hotspotGroup3);

        // Ensure the 3 groups are created correctly
        csp::common::Array<csp::systems::HotspotGroup> foundGroups;
        csp::common::Array<csp::systems::HotspotGroup> expectedGroups = { hotspotGroup1, hotspotGroup2, hotspotGroup3 };

        GetHotspotGroups(hotspotSystem, expectedGroups, foundGroups);
    }

    // Exit the space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Ensure data has been written to database by chs before entering the space again
    // This is due to an enforced 2 second chs database write delay
    std::this_thread::sleep_for(7s);

    // Reenter the space
    auto [ReEnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    // Ensure the 3 groups still exist
    csp::common::Array<csp::systems::HotspotGroup> foundGroups;
    csp::common::Array<csp::systems::HotspotGroup> expectedGroups = { hotspotGroup1, hotspotGroup2, hotspotGroup3 };

    GetHotspotGroups(hotspotSystem, expectedGroups, foundGroups);

    EXPECT_EQ(foundGroups.Size(), 3);

    // Exit space
    auto [ExitSpaceResult2] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
