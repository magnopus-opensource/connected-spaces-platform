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
#include "AssetSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "MultiplayerTestRunnerProcess.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "filesystem.hpp"
#include "signalrclient/signalr_value.h"

#include "gtest/gtest.h"
#include <CSP/Multiplayer/Components/ImageSpaceComponent.h>
#include <CSP/Multiplayer/Components/LightSpaceComponent.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

void InitialiseTestingConnection();
void OnConnect();
void OnDisconnect(bool ok);
void OnUserCreated(SpaceEntity* InUser, SpaceEntitySystem* EntitySystem);

std::atomic_bool IsTestComplete;
std::atomic_bool IsDisconnected;
std::atomic_bool IsReadyForUpdate;
SpaceEntity* TestUser;

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit = 20000;
const int NumberOfEntityUpdateTicks = 5;
int ReceivedEntityUpdatesCount;

bool EventSent = false;
bool EventReceived = false;

ReplicatedValue ObjectFloatProperty;
ReplicatedValue ObjectBoolProperty;
ReplicatedValue ObjectIntProperty;
ReplicatedValue ObjectStringProperty;

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void InitialiseTestingConnection()
{
    IsTestComplete = false;
    IsDisconnected = false;
    IsReadyForUpdate = false;
    TestUser = nullptr;

    WaitForTestTimeoutCountMs = 0;
    ReceivedEntityUpdatesCount = 0;

    EventSent = false;
    EventReceived = false;

    ObjectFloatProperty = ReplicatedValue(2.3f);
    ObjectBoolProperty = ReplicatedValue(true);
    ObjectIntProperty = ReplicatedValue(static_cast<int64_t>(42));
    ObjectStringProperty = "My replicated string";
}

void SetRandomProperties(SpaceEntity* User, SpaceEntitySystem* EntitySystem)
{
    if (User == nullptr)
    {
        return;
    }

    IsReadyForUpdate = false;

    char NameBuffer[10];
    SPRINTF(NameBuffer, "MyName%i", rand() % 100);
    User->SetName(NameBuffer);

    csp::common::Vector3 Position = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    User->SetPosition(Position);

    csp::common::Vector4 Rotation
        = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    User->SetRotation(Rotation);

    AvatarSpaceComponent* AvatarComponent = static_cast<AvatarSpaceComponent*>(User->GetComponent(0));
    AvatarComponent->SetState(static_cast<AvatarState>(rand() % 6));

    EntitySystem->QueueEntityUpdate(User);
}

void OnConnect(SpaceEntitySystem* EntitySystem)
{
    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    EntitySystem->CreateAvatar(UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode,
        [EntitySystem](SpaceEntity* NewAvatar)
        {
            EXPECT_NE(NewAvatar, nullptr);

            std::cerr << "CreateAvatar Local Callback" << std::endl;

            EXPECT_EQ(NewAvatar->GetEntityType(), SpaceEntityType::Avatar);

            if (NewAvatar->GetEntityType() == SpaceEntityType::Avatar)
            {
                OnUserCreated(NewAvatar, EntitySystem);
            }
        });
}

void OnDisconnect(bool ok)
{
    EXPECT_TRUE(ok);

    std::cerr << "OnDisconnect" << std::endl;

    IsDisconnected = true;
}

void OnUserCreated(SpaceEntity* InUser, SpaceEntitySystem* EntitySystem)
{
    EXPECT_EQ(InUser->GetComponents()->Size(), 1);

    auto* AvatarComponent = InUser->GetComponent(0);

    EXPECT_EQ(AvatarComponent->GetComponentType(), ComponentType::AvatarData);

    TestUser = InUser;
    TestUser->SetUpdateCallback(
        [InUser](SpaceEntity* UpdatedUser, SpaceEntityUpdateFlags InUpdateFlags, csp::common::Array<ComponentUpdateInfo> InComponentUpdateInfoArray)
        {
            if (InUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_NAME)
            {
                std::cerr << "Name Updated: " << UpdatedUser->GetName() << std::endl;
            }

            if (InUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
            {
                std::cerr << "Position Updated: X:" << UpdatedUser->GetPosition().X << " Y:" << UpdatedUser->GetPosition().Y
                          << " Z:" << UpdatedUser->GetPosition().Z << std::endl;
            }

            if (InUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_ROTATION)
            {
                std::cerr << "Rotation Updated: X:" << UpdatedUser->GetRotation().X << " Y:" << UpdatedUser->GetRotation().Y
                          << " Z:" << UpdatedUser->GetRotation().Z << " W:" << UpdatedUser->GetRotation().W << std::endl;
            }

            if (InUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS)
            {
                for (int i = 0; i < InComponentUpdateInfoArray.Size(); ++i)
                {
                    uint16_t ComponentID = InComponentUpdateInfoArray[i].ComponentId;

                    if (ComponentID < csp::multiplayer::COMPONENT_KEYS_START_VIEWS)
                    {
                        std::cerr << "Component Updated: ID: " << ComponentID << std::endl;

                        const csp::common::Map<uint32_t, ReplicatedValue>& Properties = *UpdatedUser->GetComponent(ComponentID)->GetProperties();
                        const csp::common::Array<uint32_t>* PropertyKeys = Properties.Keys();

                        for (int j = 0; j < PropertyKeys->Size(); ++j)
                        {
                            if (j >= 3) // We only randomise the first 3 properties, so we don't really need to print any more
                            {
                                break;
                            }

                            uint32_t PropertyID = PropertyKeys->operator[](j);
                            std::cerr << "\tProperty ID: " << PropertyID;

                            const ReplicatedValue& Property = Properties[PropertyID];

                            switch (Property.GetReplicatedValueType())
                            {
                            case ReplicatedValueType::Integer:
                                std::cerr << "\tValue: " << Property.GetInt() << std::endl;
                                break;
                            case ReplicatedValueType::String:
                                std::cerr << "\tValue: " << Property.GetString() << std::endl;
                                break;
                            case ReplicatedValueType::Float:
                                std::cerr << "\tValue: " << Property.GetFloat() << std::endl;
                                break;
                            case ReplicatedValueType::Boolean:
                                std::cerr << "\tValue: " << Property.GetBool() << std::endl;
                                break;
                            case ReplicatedValueType::Vector3:
                                std::cerr << "\tValue: {" << Property.GetVector3().X << ", " << Property.GetVector3().Y << ", "
                                          << Property.GetVector3().Z << "}" << std::endl;
                                break;
                            case ReplicatedValueType::Vector4:
                                std::cerr << "\tValue: {" << Property.GetVector4().X << ", " << Property.GetVector4().Y << ", "
                                          << Property.GetVector4().Z << ", " << Property.GetVector4().W << "}" << std::endl;
                                break;
                            default:
                                break;
                            }
                        }

                        CSP_DELETE(PropertyKeys);
                    }
                }
            }

            if (InUser == TestUser)
            {
                ReceivedEntityUpdatesCount++;
                IsReadyForUpdate = true;
            }
        });

    TestUser->SetDestroyCallback(
        [](bool Ok)
        {
            if (Ok)
            {
                std::cerr << "Destroy Callback Complete!" << std::endl;
            }
        });

    std::cerr << "OnUserCreated" << std::endl;

    SetRandomProperties(InUser, EntitySystem);
}
} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_MANUAL_SIGNALRCONNECTION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ManualConnectionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterSpaceResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    EXPECT_EQ(CreatedObject->GetName(), ObjectName);
    EXPECT_EQ(CreatedObject->GetPosition(), ObjectTransform.Position);
    EXPECT_EQ(CreatedObject->GetRotation(), ObjectTransform.Rotation);
    EXPECT_EQ(CreatedObject->GetScale(), ObjectTransform.Scale);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_SIGNALRCONNECTION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SignalRConnectionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_KEEPALIVE_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SignalRKeepAliveTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    WaitForTestTimeoutCountMs = 0;
    int KeepAliveInterval = 200;

    while (WaitForTestTimeoutCountMs < KeepAliveInterval)
    {
        std::this_thread::sleep_for(20ms);
        WaitForTestTimeoutCountMs += 20;
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_ENTITYREPLICATION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityReplicationTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    OnConnect(EntitySystem);

    WaitForTestTimeoutCountMs = 0;

    while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        EntitySystem->ProcessPendingEntityOperations();

        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;

        if (ReceivedEntityUpdatesCount < NumberOfEntityUpdateTicks)
        {
            if (IsReadyForUpdate)
            {
                SetRandomProperties(TestUser, EntitySystem);
            }
        }
        else if (ReceivedEntityUpdatesCount == NumberOfEntityUpdateTicks && IsReadyForUpdate) // Send a final update that doesn't change the data
        {
            IsReadyForUpdate = false;
            EntitySystem->QueueEntityUpdate(TestUser);
        }
        else
        {
            IsTestComplete = true;
        }
    }

    EXPECT_TRUE(IsTestComplete);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_SELF_REPLICATION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SelfReplicationTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    if (FlagSetResult == ErrorCode::None)
    {
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform
            = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

        EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        EXPECT_EQ(CreatedObject->GetName(), ObjectName);
        EXPECT_EQ(CreatedObject->GetPosition(), ObjectTransform.Position);
        EXPECT_EQ(CreatedObject->GetRotation(), ObjectTransform.Rotation);
        EXPECT_EQ(CreatedObject->GetScale(), ObjectTransform.Scale);

        auto ModelComponent = dynamic_cast<StaticModelSpaceComponent*>(CreatedObject->AddComponent(ComponentType::StaticModel));
        ModelComponent->SetExternalResourceAssetId("SomethingElse");
        ModelComponent->SetExternalResourceAssetCollectionId("Something");

        bool EntityUpdated = false;

        CreatedObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == "Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_SCALE)
                    {
                        std::cerr << "Scale Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });
        CreatedObject->SetScale(csp::common::Vector3 { 3.0f, 3.0f, 3.0f });
        CreatedObject->QueueUpdate();

        while (!EntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
        {
            EntitySystem->ProcessPendingEntityOperations();
            std::this_thread::sleep_for(50ms);
            WaitForTestTimeoutCountMs += 50;
        }

        EXPECT_LE(WaitForTestTimeoutCountMs, WaitForTestTimeoutLimit);

        EXPECT_EQ(CreatedObject->GetScale().X, 3.0f);
        EXPECT_EQ(CreatedObject->GetScale().Y, 3.0f);
        EXPECT_EQ(CreatedObject->GetScale().Z, 3.0f);
    }

    auto [FlagSetResult2] = AWAIT(Connection, SetAllowSelfMessagingFlag, false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CREATE_AVATAR_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, CreateAvatarTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;
    LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);
    EXPECT_EQ(Avatar->GetPosition(), UserTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), UserTransform.Rotation);

    auto& Components = *Avatar->GetComponents();

    EXPECT_EQ(Components.Size(), 1);

    auto* Component = Components[0];

    EXPECT_EQ(Component->GetComponentType(), ComponentType::AvatarData);

    // Verify the values of UserAvatarState and UserAvatarPlayMode
    auto* AvatarComponent = dynamic_cast<AvatarSpaceComponent*>(Component);

    EXPECT_NE(AvatarComponent, nullptr);
    EXPECT_EQ(AvatarComponent->GetState(), UserAvatarState);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), UserAvatarPlayMode);
    EXPECT_EQ(AvatarComponent->GetLocomotionModel(), UserAvatarLocomotionModel);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CREATE_CREATOR_AVATAR_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, CreateCreatorAvatarTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    const csp::common::String& UserName = "Creator 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolCreatorAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Creator;
    LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);
    EXPECT_EQ(Avatar->GetPosition(), UserTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), UserTransform.Rotation);

    auto& Components = *Avatar->GetComponents();

    EXPECT_EQ(Components.Size(), 1);

    auto* Component = Components[0];

    EXPECT_EQ(Component->GetComponentType(), ComponentType::AvatarData);

    // Verify the values of UserAvatarState and UserAvatarPlayMode
    AvatarSpaceComponent* AvatarComponent = dynamic_cast<AvatarSpaceComponent*>(Component);
    EXPECT_NE(AvatarComponent, nullptr);
    EXPECT_EQ(AvatarComponent->GetState(), UserAvatarState);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), UserAvatarPlayMode);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), AvatarPlayMode::Creator);
    EXPECT_EQ(AvatarComponent->GetLocomotionModel(), UserAvatarLocomotionModel);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CREATE_MANY_AVATAR_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, CreateManyAvatarTest)
{
    /*
     * At time of writing (2025) this may seem a bit out of place.
     * There is no special need to test avatar creation in this multiprocess way.
     * It's only that creating avatars was used as the most basic example to
     * develop the multiplayer test runner, hence this test being here, just
     * as an exerciser.
     */
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    auto ThisProcessTestUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, UserId, ThisProcessTestUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Unlisted, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto TestUser1 = CreateTestUser();
    auto TestUser2 = CreateTestUser();

    MultiplayerTestRunnerProcess CreateAvatarRunner
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR)
              .SetSpaceId(Space.Id.c_str())
              .SetLoginEmail(TestUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60);

    MultiplayerTestRunnerProcess CreateAvatarRunner2
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR)
              .SetSpaceId(Space.Id.c_str())
              .SetLoginEmail(TestUser2.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60);

    std::array<MultiplayerTestRunnerProcess, 2> Runners = { CreateAvatarRunner, CreateAvatarRunner2 };
    std::array<std::future<void>, 2> ReadyForAssertionsFutures = { Runners[0].ReadyForAssertionsFuture(), Runners[1].ReadyForAssertionsFuture() };

    // Start all the MultiplayerTestRunners
    for (auto& Runner : Runners)
    {
        Runner.StartProcess();
    }

    // Wait until the processes have reached the point where we're ready to assert
    for (auto& Future : ReadyForAssertionsFutures)
    {
        // Just being safe here, so we dont hang forever in case of catastrophe.
        auto Status = Future.wait_for(std::chrono::seconds(60));

        if (Status == std::future_status::timeout)
        {
            FAIL("CreateAvatar process timed out before it was ready for assertions.");
        }
    }

    // We must tick the entities or our local CSP instance wont know about the changes the other processes have made.
    EntitySystem->TickEntities();

    // Check there are 2 avatars in the space.
    // (The two external processes added one each, our process here in the test project just joined the room, didnt add an avatar)
    EXPECT_EQ(EntitySystem->GetNumAvatars(), 2);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_AVATAR_MOVEMENT_DIRECTION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, AvatarMovementDirectionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    auto& Components = *Avatar->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    auto* Component = Components[0];
    EXPECT_EQ(Component->GetComponentType(), ComponentType::AvatarData);

    AvatarSpaceComponent* AvatarComponent = dynamic_cast<AvatarSpaceComponent*>(Component);
    EXPECT_NE(AvatarComponent, nullptr);

    // test setting and getting movement direction
    AvatarComponent->SetMovementDirection(csp::common::Vector3::One());

    Avatar->QueueUpdate();

    EXPECT_EQ(AvatarComponent->GetMovementDirection(), csp::common::Vector3::One());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_OBJECT_CREATE_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ObjectCreateTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    EXPECT_EQ(CreatedObject->GetName(), ObjectName);
    EXPECT_EQ(CreatedObject->GetPosition(), ObjectTransform.Position);
    EXPECT_EQ(CreatedObject->GetRotation(), ObjectTransform.Rotation);
    EXPECT_EQ(CreatedObject->GetScale(), ObjectTransform.Scale);
    EXPECT_EQ(CreatedObject->GetThirdPartyRef(), "");
    EXPECT_EQ(CreatedObject->GetThirdPartyPlatformType(), csp::systems::EThirdPartyPlatform::NONE);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_OBJECT_ADDCOMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ObjectAddComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    bool PatchPending = true;
    Object->SetPatchSentCallback([&PatchPending](bool ok) { PatchPending = false; });

    const csp::common::String ModelAssetId = "NotARealId";

    auto* StaticModelComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    auto StaticModelComponentKey = StaticModelComponent->GetId();
    StaticModelComponent->SetExternalResourceAssetId(ModelAssetId);
    Object->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    PatchPending = true;

    auto& Components = *Object->GetComponents();

    EXPECT_EQ(Components.Size(), 1);
    EXPECT_TRUE(Components.HasKey(StaticModelComponentKey));

    auto* _StaticModelComponent = Object->GetComponent(StaticModelComponentKey);

    EXPECT_EQ(_StaticModelComponent->GetComponentType(), ComponentType::StaticModel);
    auto* RealStaticModelComponent = (StaticModelSpaceComponent*)_StaticModelComponent;

    EXPECT_EQ(RealStaticModelComponent->GetExternalResourceAssetId(), ModelAssetId);

    const csp::common::String ImageAssetId = "AlsoNotARealId";

    auto* ImageComponent = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
    auto ImageModelComponentKey = ImageComponent->GetId();
    ImageComponent->SetImageAssetId(ImageAssetId);
    Object->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_EQ(Object->GetComponents()->Size(), 2);
    EXPECT_TRUE(Components.HasKey(StaticModelComponentKey));
    EXPECT_TRUE(Components.HasKey(ImageModelComponentKey));

    auto* _ImageComponent = Object->GetComponent(ImageModelComponentKey);

    EXPECT_EQ(_ImageComponent->GetComponentType(), ComponentType::Image);
    auto* RealImageComponent = (ImageSpaceComponent*)_ImageComponent;

    EXPECT_EQ(RealImageComponent->GetImageAssetId(), ImageAssetId);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_OBJECT_REMOVECOMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ObjectRemoveComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    bool PatchPending = true;
    Object->SetPatchSentCallback([&PatchPending](bool ok) { PatchPending = false; });

    const csp::common::String ModelAssetId = "NotARealId";

    auto* StaticModelComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    auto StaticModelComponentKey = StaticModelComponent->GetId();
    StaticModelComponent->SetExternalResourceAssetId(ModelAssetId);
    auto* ImageComponent = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
    auto ImageComponentKey = ImageComponent->GetId();
    ImageComponent->SetImageAssetId("TestID");
    Object->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    PatchPending = true;

    auto& Components = *Object->GetComponents();

    EXPECT_EQ(Components.Size(), 2);
    EXPECT_TRUE(Components.HasKey(StaticModelComponentKey));
    EXPECT_TRUE(Components.HasKey(ImageComponentKey));

    auto* _StaticModelComponent = Object->GetComponent(StaticModelComponentKey);

    EXPECT_EQ(_StaticModelComponent->GetComponentType(), ComponentType::StaticModel);
    auto* RealStaticModelComponent = (StaticModelSpaceComponent*)_StaticModelComponent;

    EXPECT_EQ(RealStaticModelComponent->GetExternalResourceAssetId(), ModelAssetId);

    Object->RemoveComponent(StaticModelComponentKey);
    Object->RemoveComponent(ImageComponentKey);

    Object->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    auto& RealComponents = *Object->GetComponents();

    EXPECT_EQ(RealComponents.Size(), 0);
    EXPECT_FALSE(RealComponents.HasKey(StaticModelComponentKey));
    EXPECT_FALSE(RealComponents.HasKey(ImageComponentKey));

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

// This test currently requires manual steps and will be reviewed as part of OF-1535.
#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CONNECTION_INTERRUPT_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, MultiplayerTests, ConnectionInterruptTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    bool Interrupted = false;
    bool Disconnected = false;

    Connection->SetNetworkInterruptionCallback([&Interrupted](csp::common::String Message) { Interrupted = true; });

    Connection->SetDisconnectionCallback([&Disconnected](csp::common::String Message) { Disconnected = true; });

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [Avatar]
        = Awaitable(&SpaceEntitySystem::CreateAvatar, EntitySystem, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode)
              .Await();

    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    float TestTime = 0;

    // Interrupt connection here
    while (!Interrupted && TestTime < 60)
    {
        std::this_thread::sleep_for(50ms);

        SetRandomProperties(TestUser, EntitySystem);

        Current = std::chrono::steady_clock::now();
        TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();

        csp::CSPFoundation::Tick();
    }

    EXPECT_TRUE(Interrupted);

    // Delete space
    Awaitable(&csp::systems::SpaceSystem::DeleteSpace, SpaceSystem, Space.Id).Await();

    // Log out
    Awaitable(&csp::systems::UserSystem::Logout, UserSystem).Await();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_DELETE_MULTIPLE_ENTITIES_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, DeleteMultipleEntitiesTest)
{
    // Test for OB-1046
    // If the rate limiter hasn't processed all PendingOutgoingUpdates after SpaceEntity deletion it will crash when trying to process them

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Create 3 seperate objects to ensure there is too many updates for the rate limiter to process in one tick

    // Create object
    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);
    auto* ImageComponent = (ImageSpaceComponent*)CreatedObject->AddComponent(ComponentType::Image);
    CreatedObject->QueueUpdate();

    // Create object 2
    auto [CreatedObject2] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);
    auto* ImageComponent2 = (ImageSpaceComponent*)CreatedObject2->AddComponent(ComponentType::Image);
    CreatedObject2->QueueUpdate();

    // Create object 3
    auto [CreatedObject3] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);
    auto* ImageComponent3 = (ImageSpaceComponent*)CreatedObject3->AddComponent(ComponentType::Image);
    CreatedObject3->QueueUpdate();

    // Destroy Entites
    EntitySystem->DestroyEntity(CreatedObject, [](bool) {});
    EntitySystem->DestroyEntity(CreatedObject2, [](bool) {});
    EntitySystem->DestroyEntity(CreatedObject3, [](bool) {});

    csp::CSPFoundation::Tick();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_ENTITY_SELECTION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntitySelectionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    CreatedObject->Select();

    EXPECT_TRUE(CreatedObject->IsSelected());

    CreatedObject->Deselect();

    EXPECT_FALSE(CreatedObject->IsSelected());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

// Derived type that allows us to access protected members of SpaceEntitySystem
class InternalSpaceEntitySystem : public csp::multiplayer::SpaceEntitySystem
{
    ~InternalSpaceEntitySystem();

public:
    void ClearEntities()
    {
        std::scoped_lock<std::recursive_mutex> EntitiesLocker(*EntitiesLock);

        Entities.Clear();
        Objects.Clear();
        Avatars.Clear();
    }
};

// Disabled by default as it can be slow
#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_MANYENTITIES_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ManyEntitiesTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
    const char* TestAssetName = "OLY-UNITTEST-ASSET-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](auto) {});

    EXPECT_EQ(EntitySystem->GetNumEntities(), 0);
    EXPECT_EQ(EntitySystem->GetNumObjects(), 0);

    // Create a bunch of entities
    constexpr size_t NUM_ENTITIES_TO_CREATE = 15;
    constexpr char ENTITY_NAME_PREFIX[] = "Object_";

    SpaceTransform Transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    for (size_t i = 0; i < NUM_ENTITIES_TO_CREATE; ++i)
    {
        csp::common::String Name = ENTITY_NAME_PREFIX;
        Name.Append(std::to_string(i).c_str());

        auto [Object] = AWAIT(EntitySystem, CreateObject, Name, Transform);

        EXPECT_NE(Object, nullptr);
    }

    EXPECT_EQ(EntitySystem->GetNumEntities(), NUM_ENTITIES_TO_CREATE);
    EXPECT_EQ(EntitySystem->GetNumObjects(), NUM_ENTITIES_TO_CREATE);

    EntitySystem->ProcessPendingEntityOperations();

    // Clear all entities locally
    auto InternalEntitySystem = static_cast<InternalSpaceEntitySystem*>(EntitySystem);
    InternalEntitySystem->ClearEntities();

    EXPECT_EQ(EntitySystem->GetNumEntities(), 0);
    EXPECT_EQ(EntitySystem->GetNumObjects(), 0);

    // Retrieve all entities and verify count
    auto GotAllEntities = false;

    EntitySystem->SetInitialEntitiesRetrievedCallback([&](bool) { GotAllEntities = true; });

    EntitySystem->RetrieveAllEntities();

    while (!GotAllEntities)
    {
        std::this_thread::sleep_for(100ms);
    }

    EXPECT_EQ(EntitySystem->GetNumEntities(), NUM_ENTITIES_TO_CREATE);
    // We created objects exclusively, so this should also be true.
    EXPECT_EQ(EntitySystem->GetNumEntities(), EntitySystem->GetNumObjects());

    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    EXPECT_EQ(ExitResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Validate that leaving a space flushes CSP's view of all currently known entities.
    EXPECT_EQ(EntitySystem->GetNumEntities(), 0);
    EXPECT_EQ(EntitySystem->GetNumObjects(), 0);
    EXPECT_EQ(EntitySystem->GetNumAvatars(), 0);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_INVALID_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, InvalidComponentFieldsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
    const char* TestAssetName = "OLY-UNITTEST-ASSET-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    bool AssetDetailBlobChangedCallbackCalled = false;
    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    const csp::common::String ModelAssetId = "NotARealId";

    auto* LightSpaceComponentInstance = (LightSpaceComponent*)Object->AddComponent(ComponentType::Invalid);

    // Process component creation
    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_FIND_COMPONENT_BY_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, FindComponentByIdTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Create space object
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [SpaceEntity] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create second space object
    csp::common::String ObjectName2 = "Object 2";
    auto [SpaceEntity2] = AWAIT(EntitySystem, CreateObject, ObjectName2, ObjectTransform);

    auto Component1 = SpaceEntity->AddComponent(ComponentType::AnimatedModel);
    auto Component2 = SpaceEntity2->AddComponent(ComponentType::AnimatedModel);

    SpaceEntity->QueueUpdate();
    SpaceEntity2->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    auto FoundComponent = EntitySystem->FindComponentById(Component1->GetId());

    EXPECT_TRUE(FoundComponent != nullptr);
    EXPECT_EQ(Component1->GetId(), FoundComponent->GetId());

    FoundComponent = EntitySystem->FindComponentById(Component2->GetId());

    EXPECT_TRUE(FoundComponent != nullptr);
    EXPECT_EQ(Component2->GetId(), FoundComponent->GetId());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

void RunParentEntityReplicationTest(bool Local)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // If local is false, test DeserialiseFromPatch functionality
    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, !Local);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName1 = "ChildEntity1";
    csp::common::String ChildEntityName2 = "ChildEntity2";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);
    auto [CreatedChildEntity1] = AWAIT(EntitySystem, CreateObject, ChildEntityName1, ObjectTransform);
    auto [CreatedChildEntity2] = AWAIT(EntitySystem, CreateObject, ChildEntityName2, ObjectTransform);

    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

    EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 3);

    // Test setting the parent for the first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName1 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity1->SetParentId(CreatedParentEntity->GetId());

        // Parents shouldn't be set until after replication
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 3);

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);

        EXPECT_TRUE(ChildEntityUpdated);

        // Check entity1 is parented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity1);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);
    }

    // Test setting the parent for the second child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity2->SetParentId(CreatedParentEntity->GetId());

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);

        EXPECT_TRUE(ChildEntityUpdated);

        // Check all entities are parented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 2);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[1], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 1);
    }

    // Remove parent from first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName1 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity1->RemoveParentEntity();

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);

        // Check entity is  unparented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);
    }

    // Remove parent from second child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity2->RemoveParentEntity();

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);

        // Check entity is  unparented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 0);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 3);
    }

    if (!Local)
    {
        auto [FlagSetResult2] = AWAIT(Connection, SetAllowSelfMessagingFlag, false);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_LOCAL_PARENT_ENTITY_REPLICATION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentEntityLocalReplicationTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::ApplyLocalPatch functionality
    // for ParentId and ChildEntities
    RunParentEntityReplicationTest(true);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_ENTITY_REPLICATION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentEntityReplicationTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::DeserializeFromPatch functionality
    // for ParentId and ChildEntities
    RunParentEntityReplicationTest(false);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_GLOBAL_POSITION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityGlobalPositionTest)
{
    // Tests the SpaceEntitySystem::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    // create a parent child entity, where the parent is positioned at the position [1,1,1], and the child is position [1,0,0] relative to the parent
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 1, 1, 1 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformExpected
        = { csp::common::Vector3 { 2, 1, 1 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransformParent);
    auto [CreatedChildEntity] = AWAIT(EntitySystem, CreateObject, ChildEntityName, ObjectTransformChild);

    uint64_t ParentEntityId = CreatedParentEntity->GetId();
    uint64_t ChildEntityId = CreatedChildEntity->GetId();

    bool ChildEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&ChildEntityUpdated, ChildEntityName](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ChildEntityName && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
            {
                ChildEntityUpdated = true;
            }
        });

    // Change Parent
    CreatedChildEntity->SetParentId(CreatedParentEntity->GetId());

    CreatedChildEntity->QueueUpdate();

    // Wait for update
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(ChildEntityUpdated);

    // The expected outcome is that rotation and scale are unaffected, but the child is translated to position [2,1,1]
    csp::common::Vector3 GlobalPosition = CreatedChildEntity->GetGlobalPosition();
    csp::common::Vector4 GlobalRotation = CreatedChildEntity->GetGlobalRotation();
    csp::common::Vector3 GlobalScale = CreatedChildEntity->GetGlobalScale();

    EXPECT_EQ(ObjectTransformExpected.Position == GlobalPosition, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.X == GlobalRotation.X, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Y == GlobalRotation.Y, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Z == GlobalRotation.Z, true);
    // When performing quaternion operations, W can be negative, so no point checking
    EXPECT_EQ(ObjectTransformExpected.Scale == GlobalScale, true);

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_ENTITY_GLOBAL_ROTATION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityGlobalRotationTest)
{
    // Tests the SpaceEntitySystem::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    // Parent has a position [0,0,0], and 1.507 radian (90 degree) rotation around the y axis
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081, 0, 0.7071055 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformExpected
        = { csp::common::Vector3 { 0, 0, 1 }, csp::common::Vector4 { 0, -0.7071081, 0, 0.7071055 }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransformParent);
    auto [CreatedChildEntity] = AWAIT(EntitySystem, CreateObject, ChildEntityName, ObjectTransformChild);

    uint64_t ParentEntityId = CreatedParentEntity->GetId();
    uint64_t ChildEntityId = CreatedChildEntity->GetId();

    bool ChildEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&ChildEntityUpdated, ChildEntityName](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ChildEntityName && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
            {
                ChildEntityUpdated = true;
            }
        });

    // Change Parent
    CreatedChildEntity->SetParentId(CreatedParentEntity->GetId());

    CreatedChildEntity->QueueUpdate();

    // Wait for update
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(ChildEntityUpdated);

    // expectation is that scale is unaffected, rotation is passed on from parent,
    // and child is displaced to position [0, 0, 1], within floating point accuracy limits
    csp::common::Vector3 GlobalPosition = CreatedChildEntity->GetGlobalPosition();
    csp::common::Vector4 GlobalRotation = CreatedChildEntity->GetGlobalRotation();
    csp::common::Vector3 GlobalScale = CreatedChildEntity->GetGlobalScale();

    EXPECT_EQ(ObjectTransformExpected.Position == GlobalPosition, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.X == GlobalRotation.X, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Y == GlobalRotation.Y, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Z == GlobalRotation.Z, true);
    EXPECT_EQ(ObjectTransformExpected.Scale == GlobalScale, true);

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_ENTITY_GLOBAL_SCALE_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityGlobalScaleTest)
{
    // Tests the SpaceEntitySystem::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";

    // Create a parent, positioned at the origin, rotated 90 degrees, with a scale of -0.5 on x axis and 0.5 on Y/Z axes
    // child created at a position of [1,0,0], no rotation, and scale of 1
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081, 0, 0.7071055 }, csp::common::Vector3 { -0.5f, 0.5f, 0.5f } };
    SpaceTransform ObjectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };

    SpaceTransform ObjectTransformExpected
        = { csp::common::Vector3 { 0, 0, -0.5 }, csp::common::Vector4 { 0, -0.7071081, 0, 0.7071055 }, csp::common::Vector3 { -0.5f, 0.5f, 0.5f } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransformParent);
    auto [CreatedChildEntity] = AWAIT(EntitySystem, CreateObject, ChildEntityName, ObjectTransformChild);

    uint64_t ParentEntityId = CreatedParentEntity->GetId();
    uint64_t ChildEntityId = CreatedChildEntity->GetId();

    bool ChildEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&ChildEntityUpdated, ChildEntityName](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ChildEntityName && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
            {
                ChildEntityUpdated = true;
            }
        });

    // Change Parent
    CreatedChildEntity->SetParentId(CreatedParentEntity->GetId());

    CreatedChildEntity->QueueUpdate();

    // Wait for update
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(ChildEntityUpdated);
    // expectation is that the global data will have position [0,0,-0.5] (scaled by -0.5, then rotated 90 degrees from [1,0,0] around Y axis)
    // rotation will be same as parent
    // scale will now be [-0.5,0.5,0.5], same as parent
    csp::common::Vector3 GlobalPosition = CreatedChildEntity->GetGlobalPosition();
    csp::common::Vector4 GlobalRotation = CreatedChildEntity->GetGlobalRotation();
    csp::common::Vector3 GlobalScale = CreatedChildEntity->GetGlobalScale();

    EXPECT_EQ(ObjectTransformExpected.Position == GlobalPosition, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.X == GlobalRotation.X, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Y == GlobalRotation.Y, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Z == GlobalRotation.Z, true);
    EXPECT_EQ(ObjectTransformExpected.Scale == GlobalScale, true);

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_ENTITY_GLOBAL_TRANSFORM_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityGlobalTransformTest)
{
    // Tests the SpaceEntitySystem::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081, 0, 0.7071055 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformChild
        = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 0.5f, 0.5f, 0.5f } };
    SpaceTransform ObjectTransformExpected
        = { csp::common::Vector3 { 0, 0, 1 }, csp::common::Vector4 { 0, -0.7071081, 0, 0.7071055 }, csp::common::Vector3 { 0.5f, 0.5f, 0.5f } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransformParent);
    auto [CreatedChildEntity] = AWAIT(EntitySystem, CreateObject, ChildEntityName, ObjectTransformChild);

    uint64_t ParentEntityId = CreatedParentEntity->GetId();
    uint64_t ChildEntityId = CreatedChildEntity->GetId();

    bool ChildEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&ChildEntityUpdated, ChildEntityName](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ChildEntityName && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
            {
                ChildEntityUpdated = true;
            }
        });

    // Change Parent
    CreatedChildEntity->SetParentId(CreatedParentEntity->GetId());

    CreatedChildEntity->QueueUpdate();

    // Wait for update
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(ChildEntityUpdated);
    SpaceTransform ObjectTransformActual = CreatedChildEntity->GetGlobalTransform();

    EXPECT_EQ(ObjectTransformExpected.Position == ObjectTransformActual.Position, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.X == ObjectTransformActual.Rotation.X, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Y == ObjectTransformActual.Rotation.Y, true);
    EXPECT_EQ(ObjectTransformExpected.Rotation.Z == ObjectTransformActual.Rotation.Z, true);
    EXPECT_EQ(ObjectTransformExpected.Scale == ObjectTransformActual.Scale, true);

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_ENTITY_ENTER_SPACE_REPLICATION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentEntityEnterSpaceReplicationTest)
{
    // Tests the SpaceEntitySystem::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    csp::systems::Profile TestUser = CreateTestUser();
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    csp::common::String RootEntityName = "RootEntity";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);
    auto [CreatedChildEntity] = AWAIT(EntitySystem, CreateObject, ChildEntityName, ObjectTransform);
    auto [CreatedRootEntity] = AWAIT(EntitySystem, CreateObject, RootEntityName, ObjectTransform);

    uint64_t ParentEntityId = CreatedParentEntity->GetId();
    uint64_t ChildEntityId = CreatedChildEntity->GetId();

    // Parents shouldn't be set yet
    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedRootEntity->GetParentEntity(), nullptr);

    EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 3);

    bool ChildEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&ChildEntityUpdated, ChildEntityName](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
        {
            if (Entity->GetName() == ChildEntityName && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
            {
                ChildEntityUpdated = true;
            }
        });

    // Change Parent
    CreatedChildEntity->SetParentId(CreatedParentEntity->GetId());

    CreatedChildEntity->QueueUpdate();

    // Wait for update
    WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
    EXPECT_TRUE(ChildEntityUpdated);

    EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);

    // Exit Space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Log out
    LogOut(UserSystem);

    // Log in again
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Enter space
    auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

    bool EntitiesCreated = false;

    auto EntitiesReadyCallback = [&EntitiesCreated](bool Success)
    {
        EntitiesCreated = true;
        EXPECT_TRUE(Success);
    };

    EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);

    WaitForCallbackWithUpdate(EntitiesCreated, EntitySystem);
    EXPECT_TRUE(EntitiesCreated);

    // Find our entities
    SpaceEntity* RetrievedParentEntity = EntitySystem->FindSpaceEntityById(ParentEntityId);
    EXPECT_TRUE(RetrievedParentEntity != nullptr);

    SpaceEntity* RetrievedChildEntity = EntitySystem->FindSpaceEntityById(ChildEntityId);
    EXPECT_TRUE(RetrievedChildEntity != nullptr);

    // Check entity is parented correctly
    EXPECT_EQ(RetrievedChildEntity->GetParentEntity(), RetrievedParentEntity);
    EXPECT_EQ(RetrievedParentEntity->GetChildEntities()->Size(), 1);
    EXPECT_EQ((*RetrievedParentEntity->GetChildEntities())[0], RetrievedChildEntity);

    EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);

    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

void RunParentChildDeletionTest(bool Local)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // If local is false, test DeserialiseFromPatch functionality
    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, !Local);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName1 = "ChildEntity1";
    csp::common::String ChildEntityName2 = "ChildEntity2";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);
    auto [CreatedChildEntity1] = AWAIT(EntitySystem, CreateObject, ChildEntityName1, ObjectTransform);
    auto [CreatedChildEntity2] = AWAIT(EntitySystem, CreateObject, ChildEntityName2, ObjectTransform);

    // Test setting the parent for the first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName1 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity1->SetParentId(CreatedParentEntity->GetId());

        // Parents shouldn't be set until after replication
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(EntitySystem->GetNumEntities(), 3);
        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 3);

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);

        EXPECT_EQ(EntitySystem->GetNumEntities(), 3);
        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);
    }

    // Test setting the parent for the second child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity2->SetParentId(CreatedParentEntity->GetId());
        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);

        EXPECT_EQ(EntitySystem->GetNumEntities(), 3);
        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 1);
    }

    // Delete the first child
    {
        bool DestroyCalled = false;

        auto DestroyCb = [&DestroyCalled](bool Success)
        {
            DestroyCalled = true;
            EXPECT_TRUE(Success);
        };

        EntitySystem->DestroyEntity(CreatedChildEntity1, DestroyCb);

        WaitForCallbackWithUpdate(DestroyCalled, EntitySystem);
        EXPECT_TRUE(DestroyCalled);

        // Check entity is  unparented correctly
        EXPECT_EQ(EntitySystem->GetNumEntities(), 2);

        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);
    }

    // Delete the parent
    {
        bool DestroyCalled = false;

        auto DestroyCb = [&DestroyCalled](bool Success)
        {
            DestroyCalled = true;
            EXPECT_TRUE(Success);
        };

        EntitySystem->DestroyEntity(CreatedParentEntity, DestroyCb);

        WaitForCallbackWithUpdate(DestroyCalled, EntitySystem);
        EXPECT_TRUE(DestroyCalled);

        // Ensure parent is deleted and child is re-parented
        EXPECT_EQ(EntitySystem->GetNumEntities(), 1);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        if (!Local)
        {
            auto [FlagSetResult2] = AWAIT(Connection, SetAllowSelfMessagingFlag, false);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Delete space
        DeleteSpace(SpaceSystem, Space.Id);

        // Log out
        LogOut(UserSystem);
    }
}

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_LOCAL_PARENT_CHILD_DELETION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentChildLocalDeletionTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::ApplyLocalPatch functionality
    // for deletion of child and parent entities
    RunParentChildDeletionTest(true);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_CHILD_DELETION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentChildDeletionTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::DeserializeFromPatch functionality
    // for deletion of child and parent entities
    RunParentChildDeletionTest(false);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CREATE_OBJECT_PARENT_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, CreateObjectParentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";

    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);
    auto [CreatedChildEntity] = AWAIT(CreatedParentEntity, CreateChildEntity, ChildEntityName, ObjectTransform);

    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity->GetParentEntity(), CreatedParentEntity);

    EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

void RunParentDeletionTest(bool Local)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    csp::systems::Profile TestUser = CreateTestUser();
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // If local is false, test DeserialiseFromPatch functionality
    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, !Local);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName1 = "ChildEntity1";
    csp::common::String ChildEntityName2 = "ChildEntity2";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);
    auto [CreatedChildEntity1] = AWAIT(EntitySystem, CreateObject, ChildEntityName1, ObjectTransform);
    auto [CreatedChildEntity2] = AWAIT(EntitySystem, CreateObject, ChildEntityName2, ObjectTransform);

    // Test setting the parent for the first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName1 && (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT))
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity1->SetParentId(CreatedParentEntity->GetId());

        // Parents shouldn't be set until after replication
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 3);

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);
    }

    // Test setting the parent for the second child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity2->SetParentId(CreatedParentEntity->GetId());

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);
    }

    // Delete the parent
    {
        bool LocalDestroyCalled = false;
        bool EntityDestroyCalled = false;
        bool ChildEntityUpdated = false;
        bool ChildEntityUpdated2 = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, &LocalDestroyCalled, &EntityDestroyCalled, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (ChildEntityUpdated)
                {
                    // Prevent from being called twice when AllowSelfMessaging is on
                    return;
                }

                if (Entity->GetName() == ChildEntityName1 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                    // Ensure this is called before both destroy callbacks
                    EXPECT_FALSE(LocalDestroyCalled);
                    EXPECT_FALSE(EntityDestroyCalled);
                }
            });

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated2, &LocalDestroyCalled, &EntityDestroyCalled, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& UpdateInfo)
            {
                if (ChildEntityUpdated2)
                {
                    // Prevent from being called twice when AllowSelfMessaging is on
                    return;
                }

                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated2 = true;
                    // Ensure this is called before both destroy callbacks
                    EXPECT_FALSE(LocalDestroyCalled);
                    EXPECT_FALSE(EntityDestroyCalled);
                }
            });

        CreatedParentEntity->SetDestroyCallback(
            [&EntityDestroyCalled](bool Success)
            {
                EntityDestroyCalled = true;
                EXPECT_TRUE(Success);
            });

        EntitySystem->DestroyEntity(CreatedParentEntity,
            [&LocalDestroyCalled](bool Success)
            {
                LocalDestroyCalled = true;
                EXPECT_TRUE(Success);
            });

        WaitForCallbackWithUpdate(LocalDestroyCalled, EntitySystem);
        EXPECT_TRUE(LocalDestroyCalled);

        WaitForCallbackWithUpdate(EntityDestroyCalled, EntitySystem);
        EXPECT_TRUE(EntityDestroyCalled);

        WaitForCallbackWithUpdate(ChildEntityUpdated, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated);

        WaitForCallbackWithUpdate(ChildEntityUpdated2, EntitySystem);
        EXPECT_TRUE(ChildEntityUpdated2);

        // Check children are unparented correctly
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(EntitySystem->GetNumEntities(), 2);
        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);
    }

    // Re-enter space to ensure updates were made to the server
    {
        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Log out
        LogOut(UserSystem);

        // Log in again
        LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

        // Enter space
        bool EntitiesCreated = false;

        auto EntitiesReadyCallback = [&EntitiesCreated](bool Success)
        {
            EntitiesCreated = true;
            EXPECT_TRUE(Success);
        };

        EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, EntitySystem);
        EXPECT_TRUE(EntitiesCreated);
    }

    // Ensure children have been unparented and are now root entities
    {
        auto RetrievedChildEntity1 = EntitySystem->FindSpaceEntity(ChildEntityName1);
        auto RetrievedChildEntity2 = EntitySystem->FindSpaceEntity(ChildEntityName2);

        EXPECT_EQ(RetrievedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(RetrievedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(EntitySystem->GetNumEntities(), 2);
        EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 2);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_LOCAL_PARENT_DELETION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentLocalDeletionTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::ApplyLocalPatch functionality
    // for deletion of child and parent entities
    RunParentDeletionTest(true);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PARENT_DELETION_TEST
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentDeletionTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::DeserializeFromPatch functionality
    // for deletion of child and parent entities
    RunParentDeletionTest(false);
}
#endif