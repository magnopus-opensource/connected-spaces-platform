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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void CreateHotspotgroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem, const csp::common::String& GroupName,
    const csp::common::Array<csp::common::String>& Items, csp::systems::HotspotGroup& OutSequence,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
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

        for (size_t i = 0; i < group.Items.Size(); ++i)
        {
            EXPECT_EQ(group.Items[i], Items[i]);
        }

        OutSequence = group;
    }
}

void DeleteHotspotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem, const csp::common::String& GroupName,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::DeleteHotspotGroup, HotspotSequenceSystem, GroupName).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
}

void GetHotpotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem, const csp::common::String& GroupName,
    csp::systems::HotspotGroup& Group, csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
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

void UpdateHotspotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem, const csp::common::String& GroupName,
    const csp::common::Array<csp::common::String>& Items, csp::systems::HotspotGroup& HotspotGroup,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
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

        for (size_t i = 0; i < group.Items.Size(); ++i)
        {
            EXPECT_EQ(group.Items[i], Items[i]);
        }

        HotspotGroup = group;
    }
}

void RenameHotspotGroup(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem, const csp::common::String& GroupName,
    const csp::common::String& NewGroupName, csp::systems::HotspotGroup& HotspotGroup,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result]
        = Awaitable(&csp::systems::HotspotSequenceSystem::RenameHotspotGroup, HotspotSequenceSystem, GroupName, NewGroupName).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

    if (ExpectedResultCode == csp::systems::EResultCode::Success)
    {
        csp::systems::HotspotGroup group = Result.GetHotspotGroup();
        HotspotGroup = group;
    }
}

void CompareGroups(const csp::systems::HotspotGroup& S1, const csp::systems::HotspotGroup& S2)
{
    EXPECT_EQ(S1.Name, S2.Name);
    EXPECT_EQ(S1.Items.Size(), S2.Items.Size());
    if (S1.Items.Size() == S2.Items.Size())
    {
        for (size_t i = 0; i < S1.Items.Size(); ++i)
        {
            EXPECT_EQ(S1.Items[i], S2.Items[i]);
        }
    }
}

void GetHotspotGroups(csp::systems::HotspotSequenceSystem* HotspotSequenceSystem,
    const csp::common::Optional<csp::common::Array<csp::systems::HotspotGroup>>& ExpectedGroups,
    csp::common::Array<csp::systems::HotspotGroup>& Groups, csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = Awaitable(&csp::systems::HotspotSequenceSystem::GetHotspotGroups, HotspotSequenceSystem).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

    csp::common::Array<csp::systems::HotspotGroup> HotspotGroups = Result.GetHotspotGroups();

    if (Result.GetResultCode() == csp::systems::EResultCode::Success && ExpectedGroups.HasValue())
    {
        EXPECT_EQ(HotspotGroups.Size(), (*ExpectedGroups).Size());
        for (size_t i = 0; i < (*ExpectedGroups).Size(); i++)
        {
            CompareGroups(HotspotGroups[i], (*ExpectedGroups)[i]);
        }
    }

    Groups = HotspotGroups;
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, CreateHotspotGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    // Create hot spot group
    csp::common::Array<csp::common::String> GroupItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    // Validate sequence creation events.
    bool CallbackCalled = false;
    HotspotSystem->SetHotspotSequenceChangedCallback(
        [&CallbackCalled, &Space, &TestGroupName](const csp::common::SequenceChangedNetworkEventData& NetworkEventData)
        {
            EXPECT_EQ(NetworkEventData.UpdateType, csp::common::ESequenceUpdateType::Create);
            EXPECT_EQ(NetworkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
            EXPECT_EQ(NetworkEventData.SpaceId, Space.Id);
            EXPECT_EQ(NetworkEventData.Key, TestGroupName);
            CallbackCalled = true;
        });

    csp::systems::HotspotGroup HotspotGroup;
    CreateHotspotgroup(HotspotSystem, TestGroupName, GroupItems, HotspotGroup);

    WaitForCallback(CallbackCalled);
    CallbackCalled = false;

    // Validate sequence deletion events.
    HotspotSystem->SetHotspotSequenceChangedCallback(
        [&CallbackCalled, &Space, &TestGroupName](const csp::common::SequenceChangedNetworkEventData& NetworkEventData)
        {
            EXPECT_EQ(NetworkEventData.UpdateType, csp::common::ESequenceUpdateType::Delete);
            EXPECT_EQ(NetworkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
            EXPECT_EQ(NetworkEventData.SpaceId, Space.Id);
            EXPECT_EQ(NetworkEventData.Key, TestGroupName);
            CallbackCalled = true;
        });

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, TestGroupName);

    // Clear out the callback as we have validated what we came here for.
    WaitForCallback(CallbackCalled);
    HotspotSystem->SetHotspotSequenceChangedCallback(nullptr);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup HotspotGroup;
    CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotspotGroup);

    // Get the group we just created
    csp::systems::HotspotGroup RetrievedHotspotGroup;
    GetHotpotGroup(HotspotSystem, TestGroupName, RetrievedHotspotGroup);

    CompareGroups(HotspotGroup, RetrievedHotspotGroup);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, TestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, UpdateHotspotGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::Array<csp::common::String> NewItems { "Hotspot3" };
    csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup HotspotGroup1;
    csp::systems::HotspotGroup HotspotGroup2;

    CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotspotGroup1);

    csp::systems::HotspotGroup Expected;
    Expected.Name = HotspotGroup1.Name;
    Expected.Items = { "Hotspot3" };

    UpdateHotspotGroup(HotspotSystem, TestGroupName, NewItems, HotspotGroup2);
    CompareGroups(HotspotGroup2, Expected);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, TestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameHotspotGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::String OldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String NewTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

    csp::systems::HotspotGroup HotspotGroup;
    CreateHotspotgroup(HotspotSystem, OldTestGroupName, SequenceItems, HotspotGroup);

    EXPECT_EQ(HotspotGroup.Name, OldTestGroupName);

    HotspotSystem->SetHotspotSequenceChangedCallback(
        [&Space, &OldTestGroupName, &NewTestGroupName](const csp::common::SequenceChangedNetworkEventData& NetworkEventData)
        {
            // callback will be triggered when calling RenameHotspotGroup with event type update
            if (NetworkEventData.UpdateType == csp::common::ESequenceUpdateType::Update)
            {
                EXPECT_EQ(NetworkEventData.UpdateType, csp::common::ESequenceUpdateType::Update);
                EXPECT_EQ(NetworkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
                EXPECT_EQ(NetworkEventData.Key, OldTestGroupName);
                EXPECT_EQ(NetworkEventData.NewKey, NewTestGroupName);
                EXPECT_EQ(NetworkEventData.SpaceId, Space.Id);
            }
        });

    RenameHotspotGroup(HotspotSystem, OldTestGroupName, NewTestGroupName, HotspotGroup);

    EXPECT_EQ(HotspotGroup.Name, NewTestGroupName);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, NewTestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameHotspotGroupPersistantTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

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

    // Create hotspot group
    csp::systems::HotspotGroup HotspotGroup;
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::String OldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String NewTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

        auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        CreateHotspotgroup(HotspotSystem, OldTestGroupName, SequenceItems, HotspotGroup);

        EXPECT_EQ(HotspotGroup.Name, OldTestGroupName);

        HotspotSystem->SetHotspotSequenceChangedCallback(
            [&Space, &OldTestGroupName, &NewTestGroupName](const csp::common::SequenceChangedNetworkEventData& NetworkEventData)
            {
                // callback will be triggered when calling RenameHotspotGroup with event type update
                if (NetworkEventData.UpdateType == csp::common::ESequenceUpdateType::Update)
                {
                    EXPECT_EQ(NetworkEventData.UpdateType, csp::common::ESequenceUpdateType::Update);
                    EXPECT_EQ(NetworkEventData.SequenceType, csp::common::ESequenceType::Hotspot);
                    EXPECT_EQ(NetworkEventData.Key, OldTestGroupName);
                    EXPECT_EQ(NetworkEventData.NewKey, NewTestGroupName);
                    EXPECT_EQ(NetworkEventData.SpaceId, Space.Id);
                }
            });

        RenameHotspotGroup(HotspotSystem, OldTestGroupName, NewTestGroupName, HotspotGroup);

        EXPECT_EQ(HotspotGroup.Name, NewTestGroupName);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Renter the Space and get all hotspot groups to ensure the change to the HotspotGroup name persists
    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

        auto [Result] = AWAIT(SpaceSystem, EnterSpace, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        csp::common::Array<csp::systems::HotspotGroup> RetrievedGroups;
        csp::common::Array<csp::systems::HotspotGroup> ExpectedGroups = { HotspotGroup };

        // Get all hotspot sequences in the Space
        GetHotspotGroups(HotspotSystem, ExpectedGroups, RetrievedGroups);

        // Ensure the group we previously created has the correct (new) name
        EXPECT_EQ(RetrievedGroups[0].Name, NewTestGroupName);

        // Delete sequence
        DeleteHotspotGroup(HotspotSystem, NewTestGroupName);
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        EXPECT_FALSE(SpaceSystem->IsInSpace());
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, RenameFailHotspotGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    // Create hotspot group
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2" };
    csp::common::String OldTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String NewTestGroupName = "CSP-UNITTEST-SEQUENCE-MAG2";

    csp::systems::HotspotGroup HotspotGroup;

    csp::common::String expectedName = SpaceSystem->GetCurrentSpace().Id + ":" + NewTestGroupName;

    RenameHotspotGroup(HotspotSystem, OldTestGroupName, NewTestGroupName, HotspotGroup, csp::systems::EResultCode::Failed);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, NewTestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotNoGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    // Create hotspot group
    csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup HotspotGroup;

    // Get the sequence we know does not exist
    GetHotpotGroup(HotspotSystem, TestGroupName, HotspotGroup, csp::systems::EResultCode::Failed);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, TestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GetHotspotsGroupsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    csp::common::String spaceID = { UniqueSpaceName };
    // Create hotspot group
    csp::common::Array<csp::common::String> SequenceItems1 { "Hotspot1" };
    csp::common::Array<csp::common::String> SequenceItems2 { "Hotspot1", "Hotspot2" };
    csp::common::Array<csp::common::String> SequenceItems3 { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String TestGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG-1";
    csp::common::String TestGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG-2";
    csp::common::String TestGroupName3 = "CSP-UNITTEST-SEQUENCE-MAG-3";

    csp::systems::HotspotGroup HotspotGroup1;
    csp::systems::HotspotGroup HotspotGroup2;
    csp::systems::HotspotGroup HotspotGroup3;

    CreateHotspotgroup(HotspotSystem, TestGroupName1, SequenceItems1, HotspotGroup1);
    CreateHotspotgroup(HotspotSystem, TestGroupName2, SequenceItems2, HotspotGroup2);
    CreateHotspotgroup(HotspotSystem, TestGroupName3, SequenceItems3, HotspotGroup3);

    csp::common::Array<csp::systems::HotspotGroup> ExpectedGroups = { HotspotGroup1, HotspotGroup2, HotspotGroup3 };
    csp::common::Array<csp::systems::HotspotGroup> RetrievedGroups;
    csp::common::Array<csp::common::String> SearchGroupNames = { TestGroupName1, TestGroupName2, TestGroupName3 };

    // Get the sequence we just created
    GetHotspotGroups(HotspotSystem, ExpectedGroups, RetrievedGroups);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, HotspotGroup1.Name);
    DeleteHotspotGroup(HotspotSystem, HotspotGroup2.Name);
    DeleteHotspotGroup(HotspotSystem, HotspotGroup3.Name);
    // Delete space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, DeleteHotspotNoGroupTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    // Create hotspot group
    csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, TestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, GenerateSequenceKeyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    csp::common::String spaceID = { UniqueSpaceName };
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    // Create hotspot group
    csp::common::Array<csp::common::String> SequenceItems { "Hotspot1", "Hotspot2", "Hotspot3" };
    csp::common::String TestGroupName = "CSP-UNITTEST-SEQUENCE-MAG";

    csp::systems::HotspotGroup HotspotGroup;
    CreateHotspotgroup(HotspotSystem, TestGroupName, SequenceItems, HotspotGroup);

    EXPECT_EQ(TestGroupName, HotspotGroup.Name);

    // Delete sequence
    DeleteHotspotGroup(HotspotSystem, TestGroupName);
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, DeleteHotspotComponentTest)
{
    // Tests the deletion of corresponding sequences when the HotspotComponent is deleted.
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    // Create object to represent the hotspot
    csp::common::String ObjectName = "Object 1";
    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    bool ComponentAdded = false;

    CreatedObject->SetUpdateCallback(
        [&ComponentAdded, ObjectName](csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::SpaceEntityUpdateFlags /*Flags*/,
            csp::common::Array<csp::multiplayer::ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ObjectName)
            {
                for (size_t i = 0; i < UpdateInfo.Size(); ++i)
                {
                    if (UpdateInfo[i].UpdateType == csp::multiplayer::ComponentUpdateType::Add)
                    {
                        ComponentAdded = true;
                    }
                }
            }
        });

    // Create hotspot component
    auto* HotspotComponent
        = static_cast<csp::multiplayer::HotspotSpaceComponent*>(CreatedObject->AddComponent(csp::multiplayer::ComponentType::Hotspot));

    CreatedObject->QueueUpdate();
    WaitForCallbackWithUpdate(ComponentAdded, RealtimeEngine.get());

    EXPECT_TRUE(ComponentAdded);

    // Create Hotspot groups
    csp::common::String TestGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String TestGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG2";
    csp::common::String TestItemName = "AnotherItem";

    csp::systems::HotspotGroup HotspotGroup1;
    csp::systems::HotspotGroup HotspotGroup2;

    {
        // Create 2 groups that contains the component

        // Create one with only a single item to test deletion functionality
        CreateHotspotgroup(HotspotSystem, TestGroupName1, { HotspotComponent->GetUniqueComponentId() }, HotspotGroup1);

        // Create one with an additional item to test update functionality
        CreateHotspotgroup(HotspotSystem, TestGroupName2, { HotspotComponent->GetUniqueComponentId(), TestItemName }, HotspotGroup2);

        // Ensure the 2 groups are created correctly
        csp::common::Array<csp::systems::HotspotGroup> FoundGroups;
        csp::common::Array<csp::systems::HotspotGroup> ExpectedGroups = { HotspotGroup1, HotspotGroup2 };

        GetHotspotGroups(HotspotSystem, ExpectedGroups, FoundGroups);
    }

    // Remove component
    {
        bool SequenceDeleted = false;
        bool SequenceUpdate = false;
        bool SequencesUpdated = false;

        auto CB = [TestGroupName1, TestGroupName2, &SequenceDeleted, &SequenceUpdate, &SequencesUpdated](
                      const csp::common::SequenceChangedNetworkEventData& NetworkEventData)
        {
            if (NetworkEventData.Key == TestGroupName1 && NetworkEventData.UpdateType == csp::common::ESequenceUpdateType::Delete)
            {
                // Ensure we delete the group which only has one item
                SequenceDeleted = true;
            }
            else if (NetworkEventData.Key == TestGroupName2 && NetworkEventData.UpdateType == csp::common::ESequenceUpdateType::Update)
            {
                // Ensure we update the sequence that has multiple items
                SequenceUpdate = true;
            }

            SequencesUpdated = SequenceDeleted && SequenceUpdate;
        };

        HotspotSystem->SetHotspotSequenceChangedCallback(CB);

        CreatedObject->RemoveComponent(HotspotComponent->GetId());

        // Delete the hotspot from the sequence, has to be done explicitly
        HotspotSystem->RemoveItemFromGroups(HotspotComponent->GetUniqueComponentId(), [](const csp::systems::NullResult /*Result*/) { });

        CreatedObject->QueueUpdate();

        WaitForCallbackWithUpdate(SequencesUpdated, RealtimeEngine.get());

        EXPECT_TRUE(SequencesUpdated);
    }

    // 1 group should be deleted, and one should have its key removed
    {
        csp::common::Array<csp::systems::HotspotGroup> RemainingGroups;
        GetHotspotGroups(HotspotSystem, nullptr, RemainingGroups);

        EXPECT_EQ(RemainingGroups.Size(), 1);
        EXPECT_EQ(RemainingGroups[0].Items.Size(), 1);
        EXPECT_EQ(RemainingGroups[0].Items[0], TestItemName);
    }

    // Delete remaining group
    DeleteHotspotGroup(HotspotSystem, TestGroupName2);

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, HotspotSequenceTests, SequencePersistenceTest)
{
    // Ensures hotspot sequences still exist when re-entering a space
    // This tests that the ComponentBase::OnLocalDelete is only called when actually deleting a component
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* HotspotSystem = SystemsManager.GetHotspotSequenceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    // Create object to represent the hotspot
    csp::common::String ObjectName = "Object 1";
    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    bool ComponentAdded = false;

    CreatedObject->SetUpdateCallback(
        [&ComponentAdded, ObjectName](csp::multiplayer::SpaceEntity* Entity, csp::multiplayer::SpaceEntityUpdateFlags /*Flags*/,
            csp::common::Array<csp::multiplayer::ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ObjectName)
            {
                for (size_t i = 0; i < UpdateInfo.Size(); ++i)
                {
                    if (UpdateInfo[i].UpdateType == csp::multiplayer::ComponentUpdateType::Add)
                    {
                        ComponentAdded = true;
                    }
                }
            }
        });

    // Create hotspot component
    auto* HotspotComponent
        = static_cast<csp::multiplayer::HotspotSpaceComponent*>(CreatedObject->AddComponent(csp::multiplayer::ComponentType::Hotspot));

    CreatedObject->QueueUpdate();
    WaitForCallbackWithUpdate(ComponentAdded, RealtimeEngine.get());

    EXPECT_TRUE(ComponentAdded);

    // Create Hotspot groups
    csp::common::String TestGroupName1 = "CSP-UNITTEST-SEQUENCE-MAG1";
    csp::common::String TestGroupName2 = "CSP-UNITTEST-SEQUENCE-MAG2";
    csp::common::String TestGroupName3 = "CSP-UNITTEST-SEQUENCE-MAG3";

    csp::systems::HotspotGroup HotspotGroup1;
    csp::systems::HotspotGroup HotspotGroup2;
    csp::systems::HotspotGroup HotspotGroup3;

    {
        // Create 2 groups that contains the component
        CreateHotspotgroup(HotspotSystem, TestGroupName1, { HotspotComponent->GetUniqueComponentId() }, HotspotGroup1);
        CreateHotspotgroup(HotspotSystem, TestGroupName2, { HotspotComponent->GetUniqueComponentId() }, HotspotGroup2);

        // Create another group that doesnt contain the component
        CreateHotspotgroup(HotspotSystem, TestGroupName3, { "TestName" }, HotspotGroup3);

        // Ensure the 3 groups are created correctly
        csp::common::Array<csp::systems::HotspotGroup> FoundGroups;
        csp::common::Array<csp::systems::HotspotGroup> ExpectedGroups = { HotspotGroup1, HotspotGroup2, HotspotGroup3 };

        GetHotspotGroups(HotspotSystem, ExpectedGroups, FoundGroups);
    }

    // Exit the space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Ensure data has been written to database by chs before entering the space again
    // This is due to an enforced 2 second chs database write delay
    std::this_thread::sleep_for(7s);

    // Reenter the space
    auto [ReEnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    // Ensure the 3 groups still exist
    csp::common::Array<csp::systems::HotspotGroup> FoundGroups;
    csp::common::Array<csp::systems::HotspotGroup> ExpectedGroups = { HotspotGroup1, HotspotGroup2, HotspotGroup3 };

    GetHotspotGroups(HotspotSystem, ExpectedGroups, FoundGroups);

    EXPECT_EQ(FoundGroups.Size(), 3);

    // Exit space
    auto [ExitSpaceResult2] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
