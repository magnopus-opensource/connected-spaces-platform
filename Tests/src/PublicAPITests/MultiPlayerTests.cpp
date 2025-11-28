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
#include "CSP/Common/CSPAsyncScheduler.h"
#include "CSP/Common/ContinuationUtils.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "MultiplayerTestRunnerProcess.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "signalrclient/signalr_value.h"
#include "gtest/gtest.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <thread>

#include "Mocks/SignalRConnectionMock.h"

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

void InitialiseTestingConnection();
void OnUserCreated(SpaceEntity* InUser, OnlineRealtimeEngine* RealtimeEngine);

std::atomic_bool IsTestComplete;
std::atomic_bool IsDisconnected;
std::atomic_bool IsReadyForUpdate;
SpaceEntity* TestSpaceEntity;

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit = 20000;
const int NumberOfEntityUpdateTicks = 5;
int ReceivedEntityUpdatesCount;

bool EventSent = false;
bool EventReceived = false;

csp::common::ReplicatedValue ObjectFloatProperty;
csp::common::ReplicatedValue ObjectBoolProperty;
csp::common::ReplicatedValue ObjectIntProperty;
csp::common::ReplicatedValue ObjectStringProperty;

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void InitialiseTestingConnection()
{
    IsTestComplete = false;
    IsDisconnected = false;
    IsReadyForUpdate = false;
    TestSpaceEntity = nullptr;

    WaitForTestTimeoutCountMs = 0;
    ReceivedEntityUpdatesCount = 0;

    EventSent = false;
    EventReceived = false;

    ObjectFloatProperty = csp::common::ReplicatedValue(2.3f);
    ObjectBoolProperty = csp::common::ReplicatedValue(true);
    ObjectIntProperty = csp::common::ReplicatedValue(static_cast<int64_t>(42));
    ObjectStringProperty = "My replicated string";
}

void SetRandomProperties(SpaceEntity* User, OnlineRealtimeEngine* RealtimeEngine)
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

    RealtimeEngine->QueueEntityUpdate(User);
}

void OnConnect(OnlineRealtimeEngine* RealtimeEngine)
{
    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState();

    RealtimeEngine->CreateAvatar(UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode,
        [RealtimeEngine](SpaceEntity* NewAvatar)
        {
            EXPECT_NE(NewAvatar, nullptr);

            std::cerr << "CreateAvatar Local Callback" << std::endl;

            EXPECT_EQ(NewAvatar->GetEntityType(), SpaceEntityType::Avatar);

            if (NewAvatar->GetEntityType() == SpaceEntityType::Avatar)
            {
                OnUserCreated(NewAvatar, RealtimeEngine);
            }
        });
}

void OnUserCreated(SpaceEntity* InUser, OnlineRealtimeEngine* RealtimeEngine)
{
    EXPECT_EQ(InUser->GetComponents()->Size(), 1);

    auto* AvatarComponent = InUser->GetComponent(0);

    EXPECT_EQ(AvatarComponent->GetComponentType(), ComponentType::AvatarData);

    TestSpaceEntity = InUser;
    TestSpaceEntity->SetUpdateCallback(
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
                for (size_t i = 0; i < InComponentUpdateInfoArray.Size(); ++i)
                {
                    uint16_t ComponentID = InComponentUpdateInfoArray[i].ComponentId;

                    if (ComponentID < csp::multiplayer::COMPONENT_KEYS_START_VIEWS)
                    {
                        std::cerr << "Component Updated: ID: " << ComponentID << std::endl;

                        const csp::common::Map<uint32_t, csp::common::ReplicatedValue>& Properties
                            = *UpdatedUser->GetComponent(ComponentID)->GetProperties();
                        const csp::common::Array<uint32_t>* PropertyKeys = Properties.Keys();

                        for (size_t j = 0; j < PropertyKeys->Size(); ++j)
                        {
                            if (j >= 3) // We only randomise the first 3 properties, so we don't really need to print any more
                            {
                                break;
                            }

                            uint32_t PropertyID = PropertyKeys->operator[](j);
                            std::cerr << "\tProperty ID: " << PropertyID;

                            const csp::common::ReplicatedValue& Property = Properties[PropertyID];

                            switch (Property.GetReplicatedValueType())
                            {
                            case csp::common::ReplicatedValueType::Integer:
                                std::cerr << "\tValue: " << Property.GetInt() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::String:
                                std::cerr << "\tValue: " << Property.GetString() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Float:
                                std::cerr << "\tValue: " << Property.GetFloat() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Boolean:
                                std::cerr << "\tValue: " << Property.GetBool() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Vector3:
                                std::cerr << "\tValue: {" << Property.GetVector3().X << ", " << Property.GetVector3().Y << ", "
                                          << Property.GetVector3().Z << "}" << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Vector4:
                                std::cerr << "\tValue: {" << Property.GetVector4().X << ", " << Property.GetVector4().Y << ", "
                                          << Property.GetVector4().Z << ", " << Property.GetVector4().W << "}" << std::endl;
                                break;
                            default:
                                break;
                            }
                        }

                        delete (PropertyKeys);
                    }
                }
            }

            if (InUser == TestSpaceEntity)
            {
                ReceivedEntityUpdatesCount++;
                IsReadyForUpdate = true;
            }
        });

    TestSpaceEntity->SetDestroyCallback(
        [](bool Ok)
        {
            if (Ok)
            {
                std::cerr << "Destroy Callback Complete!" << std::endl;
            }
        });

    std::cerr << "OnUserCreated" << std::endl;

    SetRandomProperties(InUser, RealtimeEngine);
}

} // namespace
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ManualConnectionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    bool CallbackCalled = false;

    Connection->SetConnectionCallback([&CallbackCalled](const csp::common::String& /*Message*/) { CallbackCalled = true; });

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    WaitForCallback(CallbackCalled);
    EXPECT_TRUE(CallbackCalled);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterSpaceResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

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

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SignalRConnectionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    InitialiseTestingConnection();

    auto Headers = Connection->Connection->HTTPHeaders();
    ASSERT_NE(Headers.find("X-DeviceUDID"), Headers.end());

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SignalRKeepAliveTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    InitialiseTestingConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

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

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityReplicationTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    InitialiseTestingConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    OnConnect(RealtimeEngine.get());

    WaitForTestTimeoutCountMs = 0;

    while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        RealtimeEngine->ProcessPendingEntityOperations();

        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;

        if (ReceivedEntityUpdatesCount < NumberOfEntityUpdateTicks)
        {
            if (IsReadyForUpdate)
            {
                SetRandomProperties(TestSpaceEntity, RealtimeEngine.get());
            }
        }
        else if (ReceivedEntityUpdatesCount == NumberOfEntityUpdateTicks && IsReadyForUpdate) // Send a final update that doesn't change the data
        {
            IsReadyForUpdate = false;
            RealtimeEngine->QueueEntityUpdate(TestSpaceEntity);
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

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SelfReplicationTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    if (FlagSetResult == ErrorCode::None)
    {
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform
            = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

        RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        EXPECT_EQ(CreatedObject->GetName(), ObjectName);
        EXPECT_EQ(CreatedObject->GetPosition(), ObjectTransform.Position);
        EXPECT_EQ(CreatedObject->GetRotation(), ObjectTransform.Rotation);
        EXPECT_EQ(CreatedObject->GetScale(), ObjectTransform.Scale);

        auto ModelComponent = dynamic_cast<StaticModelSpaceComponent*>(CreatedObject->AddComponent(ComponentType::StaticModel));
        ModelComponent->SetExternalResourceAssetId("SomethingElse");
        ModelComponent->SetExternalResourceAssetCollectionId("Something");

        bool EntityUpdated = false;

        CreatedObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
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
            RealtimeEngine->ProcessPendingEntityOperations();
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

// This test currently requires manual steps and will be reviewed as part of OF-1535.
CSP_PUBLIC_TEST(DISABLED_CSPEngine, MultiplayerTests, ConnectionInterruptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    bool Interrupted = false;
    bool Disconnected = false;

    Connection->SetNetworkInterruptionCallback([&Interrupted](csp::common::String /*Message*/) { Interrupted = true; });

    Connection->SetDisconnectionCallback([&Disconnected](csp::common::String /*Message*/) { Disconnected = true; });

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = Awaitable(&OnlineRealtimeEngine::CreateAvatar, RealtimeEngine.get(), UserName, LoginState.UserId, UserTransform, IsVisible,
        UserAvatarState, UserAvatarId, UserAvatarPlayMode)
                        .Await();

    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    long long TestTime = 0;

    // Interrupt connection here
    while (!Interrupted && TestTime < 60)
    {
        std::this_thread::sleep_for(50ms);

        SetRandomProperties(TestSpaceEntity, RealtimeEngine.get());

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

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

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

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto TestUser1 = CreateTestUser();
    auto TestUser2 = CreateTestUser();

    MultiplayerTestRunnerProcess CreateAvatarRunner
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR)
              .SetSpaceId(Space.Id.c_str())
              .SetLoginEmail(TestUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60)
              .SetEndpoint(EndpointBaseURI());

    MultiplayerTestRunnerProcess CreateAvatarRunner2
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR)
              .SetSpaceId(Space.Id.c_str())
              .SetLoginEmail(TestUser2.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60)
              .SetEndpoint(EndpointBaseURI());

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
            FAIL() << "CreateAvatar process timed out before it was ready for assertions.";
        }
    }

    // We must tick the entities or our local CSP instance wont know about the changes the other processes have made.
    RealtimeEngine->TickEntities();

    // Check there are 2 avatars in the space.
    // (The two external processes added one each, our process here in the test project just joined the room, didnt add an avatar)
    EXPECT_EQ(RealtimeEngine->GetNumAvatars(), 2);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

// Derived type that allows us to access protected members of OnlineRealtimeEngineWeak
class InternalOnlineRealtimeEngine : public csp::multiplayer::OnlineRealtimeEngine
{
    ~InternalOnlineRealtimeEngine();

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
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ManyEntitiesTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(RealtimeEngine->GetNumEntities(), 0);
    EXPECT_EQ(RealtimeEngine->GetNumObjects(), 0);

    // Create a bunch of entities
    constexpr size_t NUM_ENTITIES_TO_CREATE = 15;
    constexpr char ENTITY_NAME_PREFIX[] = "Object_";

    SpaceTransform Transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    for (size_t i = 0; i < NUM_ENTITIES_TO_CREATE; ++i)
    {
        csp::common::String Name = ENTITY_NAME_PREFIX;
        Name.Append(std::to_string(i).c_str());

        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, Name, Transform, csp::common::Optional<uint64_t> {});

        EXPECT_NE(Object, nullptr);
    }

    EXPECT_EQ(RealtimeEngine->GetNumEntities(), NUM_ENTITIES_TO_CREATE);
    EXPECT_EQ(RealtimeEngine->GetNumObjects(), NUM_ENTITIES_TO_CREATE);

    RealtimeEngine->ProcessPendingEntityOperations();

    // Clear all entities locally
    auto InternalEntitySystem = static_cast<InternalOnlineRealtimeEngine*>(RealtimeEngine.get());
    InternalEntitySystem->ClearEntities();

    EXPECT_EQ(RealtimeEngine->GetNumEntities(), 0);
    EXPECT_EQ(RealtimeEngine->GetNumObjects(), 0);

    // Retrieve all entities and verify count
    auto GotAllEntities = false;

    RealtimeEngine->RetrieveAllEntities(
        [&](int NumEntitiesFetched)
        {
            GotAllEntities = true;
            std::cout << NumEntitiesFetched << std::endl;
        });

    while (!GotAllEntities)
    {
        std::this_thread::sleep_for(100ms);
    }

    RealtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(RealtimeEngine->GetNumEntities(), NUM_ENTITIES_TO_CREATE);
    // We created objects exclusively, so this should also be true.
    EXPECT_EQ(RealtimeEngine->GetNumEntities(), RealtimeEngine->GetNumObjects());

    const auto PreLeaveEntities = RealtimeEngine->GetNumEntities();
    const auto PreLeaveObjects = RealtimeEngine->GetNumObjects();
    const auto PreLeaveAvatars = RealtimeEngine->GetNumAvatars();

    auto [ExitResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    EXPECT_EQ(ExitResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Validate that leaving a space has no impact on the entities contained in the realtimeEngine, clients may reuse it if they wish.
    EXPECT_EQ(RealtimeEngine->GetNumEntities(), PreLeaveEntities);
    EXPECT_EQ(RealtimeEngine->GetNumObjects(), PreLeaveObjects);
    EXPECT_EQ(RealtimeEngine->GetNumAvatars(), PreLeaveAvatars);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

void RunParentEntityReplicationTest(bool Local)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // If local is false, test DeserialiseFromPatch functionality
    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, !Local);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName1 = "ChildEntity1";
    csp::common::String ChildEntityName2 = "ChildEntity2";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity1] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName1, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity2] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName2, ObjectTransform, csp::common::Optional<uint64_t> {});

    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

    EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 3);

    // Test setting the parent for the first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
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

        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 3);

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());

        EXPECT_TRUE(ChildEntityUpdated);

        // Check entity1 is parented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity1);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    // Test setting the parent for the second child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity2->SetParentId(CreatedParentEntity->GetId());

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());

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

        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 1);
    }

    // Remove parent from first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (Entity->GetName() == ChildEntityName1 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity1->RemoveParentEntity();

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
        EXPECT_TRUE(ChildEntityUpdated);

        // Check entity is  unparented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    // Remove parent from second child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName2](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (Entity->GetName() == ChildEntityName2 && Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity2->RemoveParentEntity();

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
        EXPECT_TRUE(ChildEntityUpdated);

        // Check entity is  unparented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 0);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 3);
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

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentEntityLocalReplicationTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::ApplyLocalPatch functionality
    // for ParentId and ChildEntities
    RunParentEntityReplicationTest(true);
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ParentEntityReplicationTest)
{
    // Tests the SpaceEntity::SerializeFromPatch and SpaceEntity::DeserializeFromPatch functionality
    // for ParentId and ChildEntities
    RunParentEntityReplicationTest(false);
}

// This test is to be fixed as part of OF-1651.
CSP_PUBLIC_TEST(DISABLED_CSPEngine, MultiplayerTests, ParentEntityEnterSpaceReplicationTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    csp::systems::Profile TestUser = CreateTestUser();
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    bool EntitiesCreated = false;
    auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    csp::common::String RootEntityName = "RootEntity";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedRootEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, RootEntityName, ObjectTransform, csp::common::Optional<uint64_t> {});

    uint64_t ParentEntityId = CreatedParentEntity->GetId();
    uint64_t ChildEntityId = CreatedChildEntity->GetId();

    // Parents shouldn't be set yet
    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedRootEntity->GetParentEntity(), nullptr);

    EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 3);

    bool ChildEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&ChildEntityUpdated, ChildEntityName](
            SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
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
    WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
    EXPECT_TRUE(ChildEntityUpdated);

    EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);

    // Exit Space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Log out
    LogOut(UserSystem);

    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Log in again
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Enter space
    auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

    WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
    EXPECT_TRUE(EntitiesCreated);

    // Find our entities
    SpaceEntity* RetrievedParentEntity = RealtimeEngine->FindSpaceEntityById(ParentEntityId);
    EXPECT_TRUE(RetrievedParentEntity != nullptr);

    SpaceEntity* RetrievedChildEntity = RealtimeEngine->FindSpaceEntityById(ChildEntityId);
    EXPECT_TRUE(RetrievedChildEntity != nullptr);

    // Check entity is parented correctly
    EXPECT_EQ(RetrievedChildEntity->GetParentEntity(), RetrievedParentEntity);
    EXPECT_EQ(RetrievedParentEntity->GetChildEntities()->Size(), 1);
    EXPECT_EQ((*RetrievedParentEntity->GetChildEntities())[0], RetrievedChildEntity);

    EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);

    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

namespace
{
class MockMultiplayerErrorCallback
{
public:
    MOCK_METHOD(void, Call, (csp::multiplayer::ErrorCode), ());
};

class MockConnectionCallback
{
public:
    MOCK_METHOD(void, Call, (const csp::common::String&), ());
};

void StartAlwaysSucceeds(SignalRConnectionMock& SignalRMock)
{
    ON_CALL(SignalRMock, Start).WillByDefault([](std::function<void(std::exception_ptr)> Callback) { Callback(nullptr); });
}
void StopAlwaysSucceeds(SignalRConnectionMock& SignalRMock)
{
    ON_CALL(SignalRMock, Stop).WillByDefault([](std::function<void(std::exception_ptr)> Callback) { Callback(nullptr); });
}
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRStartErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem LogSystem;
    SignalRConnectionMock* SignalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection Connection { LogSystem, *SignalRMock };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };

    // The start function will throw internally
    EXPECT_CALL(*SignalRMock, Start)
        .WillOnce([](std::function<void(std::exception_ptr)> Callback)
            { Callback(std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"))); });

    // Then the error callback we be called with an unknown error code
    MockMultiplayerErrorCallback MockErrorCallback;
    EXPECT_CALL(MockErrorCallback, Call(csp::multiplayer::ErrorCode::Unknown));

    // And the disconnection callback will be called with a message (weird)
    MockConnectionCallback MockDisconnectionCallback;
    EXPECT_CALL(MockDisconnectionCallback, Call(csp::common::String("MultiplayerConnection::Start, Error when starting SignalR connection.")));

    Connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &MockDisconnectionCallback, std::placeholders::_1));
    Connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &MockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRInvokeDeleteObjectsErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem LogSystem;
    SignalRConnectionMock* SignalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection Connection { LogSystem, *SignalRMock };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*SignalRMock);
    StopAlwaysSucceeds(*SignalRMock);

    // Invoke function for delete objects errors
    EXPECT_CALL(*SignalRMock, Invoke)
        .WillOnce(
            [](const std::string& /*DeleteObjectsMethodName*/, const signalr::value& /*DeleteEntityMessage*/,
                std::function<void(const signalr::value&, std::exception_ptr)> Callback)
            {
                auto Value = signalr::value("Irrelevant value from DeleteObjects");
                auto Except = std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"));
                Callback(Value, Except);
                return async::make_task(std::make_tuple(Value, Except));
            });

    // Then the error callback we be called with an no error code
    MockMultiplayerErrorCallback MockErrorCallback;
    EXPECT_CALL(MockErrorCallback, Call(csp::multiplayer::ErrorCode::None));

    // And the disconnection callback will be called with a message (weird)
    MockConnectionCallback MockDisconnectionCallback;
    EXPECT_CALL(MockDisconnectionCallback,
        Call(csp::common::String("MultiplayerConnection::DeleteEntities, Unexpected error response from SignalR \"DeleteObjects\" invocation.")));

    Connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &MockDisconnectionCallback, std::placeholders::_1));
    Connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &MockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRInvokeGetClientIdErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem LogSystem;
    SignalRConnectionMock* SignalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection Connection { LogSystem, *SignalRMock };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*SignalRMock);
    StopAlwaysSucceeds(*SignalRMock);

    EXPECT_CALL(*SignalRMock, Invoke)
        .WillRepeatedly(
            [&Connection](const std::string& HubMethodName, const signalr::value& /*Message*/,
                std::function<void(const signalr::value&, std::exception_ptr)> Callback)
            {
                if (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::DELETE_OBJECTS))
                {
                    // Succeed deleting objects
                    auto Value = signalr::value("Irrelevant value from DeleteObjects");
                    Callback(Value, nullptr);
                    return async::make_task(std::make_tuple(Value, std::exception_ptr(nullptr)));
                }
                else if (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GET_CLIENT_ID))
                {
                    // Fail getting client Id
                    auto Value = signalr::value("Irrelevant value from GetClientId");
                    auto Except = std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"));
                    Callback(Value, Except);
                    return async::make_task(std::make_tuple(Value, Except));
                }

                // Just a default case, shouldn't matter
                return async::make_task(std::make_tuple(signalr::value("mock value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    // Then the error callback we be called with no error code
    MockMultiplayerErrorCallback MockErrorCallback;
    EXPECT_CALL(MockErrorCallback, Call(csp::multiplayer::ErrorCode::None));

    // And the disconnection callback will be called with a message
    MockConnectionCallback MockDisconnectionCallback;
    EXPECT_CALL(
        MockDisconnectionCallback, Call(csp::common::String("MultiplayerConnection::RequestClientId, Error when starting requesting Client Id.")));

    Connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &MockDisconnectionCallback, std::placeholders::_1));
    Connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &MockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRInvokeStartListeningErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem LogSystem;
    SignalRConnectionMock* SignalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection Connection { LogSystem, *SignalRMock };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*SignalRMock);
    StopAlwaysSucceeds(*SignalRMock);

    EXPECT_CALL(*SignalRMock, Invoke)
        .WillRepeatedly(
            [&Connection](const std::string& HubMethodName, const signalr::value& /*Message*/,
                std::function<void(const signalr::value&, std::exception_ptr)> Callback)
            {
                if (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::DELETE_OBJECTS))
                {
                    // Succeed deleting objects
                    auto Value = signalr::value("Irrelevant value from DeleteObjects");
                    Callback(Value, nullptr);
                    return async::make_task(std::make_tuple(Value, std::exception_ptr(nullptr)));
                }
                else if (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GET_CLIENT_ID))
                {
                    // Succeed getting client Id
                    Callback(signalr::value(std::uint64_t(0)), nullptr);
                    return async::make_task(std::make_tuple(signalr::value(std::uint64_t(0)), std::exception_ptr(nullptr)));
                }
                else if (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::START_LISTENING))
                {
                    // Fail to start listening
                    auto Except = std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"));
                    Callback(signalr::value(std::uint64_t(0)), Except);
                    return async::make_task(std::make_tuple(signalr::value(std::uint64_t(0)), Except));
                }

                // Just a default case, shouldn't matter
                return async::make_task(std::make_tuple(signalr::value("mock value"),
                    std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"))));
            });

    // Then the error callback we be called with no error code
    MockMultiplayerErrorCallback MockErrorCallback;
    EXPECT_CALL(MockErrorCallback, Call(csp::multiplayer::ErrorCode::None));

    // And the disconnection callback will be called with a message
    MockConnectionCallback MockDisconnectionCallback;
    EXPECT_CALL(MockDisconnectionCallback, Call(csp::common::String("MultiplayerConnection::StartListening, Error when starting listening.")));

    Connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &MockDisconnectionCallback, std::placeholders::_1));
    Connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &MockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenAllSignalRSucceedsThenSuccessCallbacksCalled)
{
    csp::common::LogSystem LogSystem;
    SignalRConnectionMock* SignalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection Connection { LogSystem, *SignalRMock };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*SignalRMock);
    StopAlwaysSucceeds(*SignalRMock);

    EXPECT_CALL(*SignalRMock, Invoke)
        .WillRepeatedly(
            [&Connection](const std::string& HubMethodName, const signalr::value& /*Message*/,
                std::function<void(const signalr::value&, std::exception_ptr)> Callback)
            {
                if (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::DELETE_OBJECTS))
                {
                    // Succeed deleting objects
                    auto Value = signalr::value("Irrelevant value from DeleteObjects");
                    Callback(Value, nullptr);
                    return async::make_task(std::make_tuple(Value, std::exception_ptr(nullptr)));
                }
                else if ((HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GET_CLIENT_ID))
                    || (HubMethodName == Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::START_LISTENING)))
                {
                    // Succeed getting client Id
                    Callback(signalr::value(std::uint64_t(0)), nullptr);
                    return async::make_task(std::make_tuple(signalr::value(std::uint64_t(0)), std::exception_ptr(nullptr)));
                }

                // Just a default case, shouldn't matter
                return async::make_task(std::make_tuple(signalr::value("mock value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    // Then the error callback will be called with no error
    MockMultiplayerErrorCallback MockErrorCallback;
    EXPECT_CALL(MockErrorCallback, Call(ErrorCode::None));

    // And the connection callback with be called
    MockConnectionCallback MockSuccessConnectionCallback;
    EXPECT_CALL(MockSuccessConnectionCallback, Call(csp::common::String("Successfully connected to SignalR hub.")));

    // And the disconnection callback will not be called
    MockConnectionCallback MockDisconnectionCallback;
    EXPECT_CALL(MockDisconnectionCallback, Call(::testing::_)).Times(0);

    Connection.SetConnectionCallback(std::bind(&MockConnectionCallback::Call, &MockSuccessConnectionCallback, std::placeholders::_1));
    Connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &MockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, TestParseMultiplayerError)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // ParseMultiplayerError is odd, it seems only concerned with understanding this "Scopes_ConcurrentUsersQuota error"
    // I'm actually not sure if CHS even still throws this format of errors, this could be completely redundant...
    auto [ErrorCodeTooManyUsers, MsgTooManyUsers] = Connection->ParseMultiplayerError(std::runtime_error("error code: Scopes_ConcurrentUsersQuota"));
    EXPECT_EQ(ErrorCodeTooManyUsers, csp::multiplayer::ErrorCode::SpaceUserLimitExceeded);
    EXPECT_EQ(MsgTooManyUsers, "error code: Scopes_ConcurrentUsersQuota");

    auto [ErrorCodeUnknown, MsgUnknown] = Connection->ParseMultiplayerError(std::runtime_error("Some unknown error"));
    EXPECT_EQ(ErrorCodeUnknown, csp::multiplayer::ErrorCode::Unknown);
    EXPECT_EQ(MsgUnknown, "Some unknown error");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityLockPersistanceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    const csp::systems::Profile TestUser = CreateTestUser();
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space
    bool EntitiesCreated = false;

    auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

    // Ensure patch rate limiting is off, as we're sending patches in quick succession.
    RealtimeEngine->SetEntityPatchRateLimitEnabled(false);

    // Enter a space and lock an entity
    {
        // Enter space
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create Entity
        const csp::common::String EntityName = "Entity";
        const SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

        auto [CreatedEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, EntityName, ObjectTransform, csp::common::Optional<uint64_t> {});

        // Lock Entity
        CreatedEntity->Lock();

        // Apply patch
        CreatedEntity->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        // Entity should be locked now
        EXPECT_TRUE(CreatedEntity->IsLocked());
    }

    // Re-enter space to ensure updates were made to the server
    {
        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Log out
        LogOut(UserSystem);

        // Wait a few seconds for the CHS database to update
        std::this_thread::sleep_for(std::chrono::seconds(8));

        // Log in again
        LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

        // Enter space
        EntitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
        EXPECT_TRUE(EntitiesCreated);

        SpaceEntity* Entity = RealtimeEngine->GetEntityByIndex(0);
        EXPECT_TRUE(Entity->IsLocked());
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/* Check that the SignalR connection is not used when we login without creating a multiplayer connection
 * Ideally, this should be as simple as checking if MultiplayerConnection is null, managed by whether the
 * user chooses to instantiate and inject a multiplayer connection or not, but we're not there yet
 */
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, MultiplayerTestsMock, TestNoSignalRCommunicationWhenLoggedInWithoutConnection)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    const csp::systems::Profile TestUser = CreateTestUser();
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword, false, true);

    std::unique_ptr<csp::multiplayer::OfflineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOfflineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    EXPECT_FALSE(Connection->IsConnected());

    // Expect that none of the signalR methods are ever called
    EXPECT_CALL(*SignalRMock, Start).Times(0);
    EXPECT_CALL(*SignalRMock, Stop).Times(0);
    EXPECT_CALL(*SignalRMock, GetConnectionState).Times(0);
    EXPECT_CALL(*SignalRMock, GetConnectionId).Times(0);
    EXPECT_CALL(*SignalRMock, SetDisconnected).Times(0);
    EXPECT_CALL(*SignalRMock, On).Times(0);
    EXPECT_CALL(*SignalRMock, Invoke).Times(0);
    EXPECT_CALL(*SignalRMock, Send).Times(0);
    EXPECT_CALL(*SignalRMock, HTTPHeaders).Times(0);

    // Do some stuff
    const csp::common::String LocalSpaceId = "LocalSpace";
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, LocalSpaceId, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String EntityName = "Entity";
    const SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

    auto [CreatedEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, EntityName, ObjectTransform, csp::common::Optional<uint64_t> {});

    csp::common::Vector3 Position = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    CreatedEntity->SetPosition(Position);

    csp::common::Vector4 Rotation
        = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    CreatedEntity->SetRotation(Rotation);

    csp::CSPFoundation::Tick();
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, TestMultiplayerDisconnectionWhenNewMultiplayerSessionInitiated)
{
    csp::common::LogSystem LogSystem;
    SignalRConnectionMock* SignalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection Connection { LogSystem, *SignalRMock };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };

    // Intercept the 'OnRequestToDisconnect' event and dummy expected response for new user login on different client
    ON_CALL(*SignalRMock, On)
        .WillByDefault(
            [&Connection](
                const std::string& EventName, const ISignalRConnection::MethodInvokedHandler& Handler, csp::common::LogSystem& /*LogSystem*/)
            {
                if (EventName == "OnRequestToDisconnect")
                {
                    Handler(std::vector { signalr::value("New Multiplayer Session Initiated") });
                }

                return true;
            });

    // Then the error callback we be called
    MockMultiplayerErrorCallback MockErrorCallback;

    // And the disconnection callback will be called with a message
    MockConnectionCallback MockDisconnectionCallback;
    EXPECT_CALL(MockDisconnectionCallback, Call(csp::common::String("New Multiplayer Session Initiated")));

    Connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &MockDisconnectionCallback, std::placeholders::_1));
    Connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &MockErrorCallback, std::placeholders::_1), "", "", "");
}