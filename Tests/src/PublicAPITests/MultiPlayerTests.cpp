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
void OnUserCreated(SpaceEntity* inUser, OnlineRealtimeEngine* realtimeEngine);

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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

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

void SetRandomProperties(SpaceEntity* user, OnlineRealtimeEngine* realtimeEngine)
{
    if (user == nullptr)
    {
        return;
    }

    IsReadyForUpdate = false;

    char nameBuffer[10];
    SPRINTF(nameBuffer, "MyName%i", rand() % 100);
    user->SetName(nameBuffer);

    csp::common::Vector3 position = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    user->SetPosition(position);

    csp::common::Vector4 rotation
        = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    user->SetRotation(rotation);

    AvatarSpaceComponent* avatarComponent = static_cast<AvatarSpaceComponent*>(user->GetComponent(0));
    avatarComponent->SetState(static_cast<AvatarState>(rand() % 6));

    realtimeEngine->QueueEntityUpdate(user);
}

void OnConnect(OnlineRealtimeEngine* realtimeEngine)
{
    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState();

    realtimeEngine->CreateAvatar(userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId, userAvatarPlayMode,
        LocomotionModel::Grounded,
        [realtimeEngine](SpaceEntity* newAvatar)
        {
            EXPECT_NE(newAvatar, nullptr);

            std::cerr << "CreateAvatar Local Callback" << std::endl;

            EXPECT_EQ(newAvatar->GetEntityType(), SpaceEntityType::Avatar);

            if (newAvatar->GetEntityType() == SpaceEntityType::Avatar)
            {
                OnUserCreated(newAvatar, realtimeEngine);
            }
        });
}

void OnUserCreated(SpaceEntity* inUser, OnlineRealtimeEngine* realtimeEngine)
{
    EXPECT_EQ(inUser->GetComponents()->Size(), 1);

    auto* avatarComponent = inUser->GetComponent(0);

    EXPECT_EQ(avatarComponent->GetComponentType(), ComponentType::AvatarData);

    TestSpaceEntity = inUser;
    TestSpaceEntity->SetUpdateCallback(
        [inUser](SpaceEntity* updatedUser, SpaceEntityUpdateFlags inUpdateFlags, csp::common::Array<ComponentUpdateInfo> inComponentUpdateInfoArray)
        {
            if (inUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_NAME)
            {
                std::cerr << "Name Updated: " << updatedUser->GetName() << std::endl;
            }

            if (inUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
            {
                std::cerr << "Position Updated: X:" << updatedUser->GetPosition().X << " Y:" << updatedUser->GetPosition().Y
                          << " Z:" << updatedUser->GetPosition().Z << std::endl;
            }

            if (inUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_ROTATION)
            {
                std::cerr << "Rotation Updated: X:" << updatedUser->GetRotation().X << " Y:" << updatedUser->GetRotation().Y
                          << " Z:" << updatedUser->GetRotation().Z << " W:" << updatedUser->GetRotation().W << std::endl;
            }

            if (inUpdateFlags & SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS)
            {
                for (size_t i = 0; i < inComponentUpdateInfoArray.Size(); ++i)
                {
                    uint16_t componentId = inComponentUpdateInfoArray[i].ComponentId;

                    if (componentId < csp::multiplayer::COMPONENT_KEYS_START_VIEWS)
                    {
                        std::cerr << "Component Updated: ID: " << componentId << std::endl;

                        const csp::common::Map<uint32_t, csp::common::ReplicatedValue>& properties
                            = *updatedUser->GetComponent(componentId)->GetProperties();
                        const csp::common::Array<uint32_t>* propertyKeys = properties.Keys();

                        for (size_t j = 0; j < propertyKeys->Size(); ++j)
                        {
                            if (j >= 3) // We only randomise the first 3 properties, so we don't really need to print any more
                            {
                                break;
                            }

                            uint32_t propertyId = propertyKeys->operator[](j);
                            std::cerr << "\tProperty ID: " << propertyId;

                            const csp::common::ReplicatedValue& property = properties[propertyId];

                            switch (property.GetReplicatedValueType())
                            {
                            case csp::common::ReplicatedValueType::Integer:
                                std::cerr << "\tValue: " << property.GetInt() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::String:
                                std::cerr << "\tValue: " << property.GetString() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Float:
                                std::cerr << "\tValue: " << property.GetFloat() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Boolean:
                                std::cerr << "\tValue: " << property.GetBool() << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Vector3:
                                std::cerr << "\tValue: {" << property.GetVector3().X << ", " << property.GetVector3().Y << ", "
                                          << property.GetVector3().Z << "}" << std::endl;
                                break;
                            case csp::common::ReplicatedValueType::Vector4:
                                std::cerr << "\tValue: {" << property.GetVector4().X << ", " << property.GetVector4().Y << ", "
                                          << property.GetVector4().Z << ", " << property.GetVector4().W << "}" << std::endl;
                                break;
                            default:
                                break;
                            }
                        }

                        delete (propertyKeys);
                    }
                }
            }

            if (inUser == TestSpaceEntity)
            {
                ReceivedEntityUpdatesCount++;
                IsReadyForUpdate = true;
            }
        });

    TestSpaceEntity->SetDestroyCallback(
        [](bool ok)
        {
            if (ok)
            {
                std::cerr << "Destroy Callback Complete!" << std::endl;
            }
        });

    std::cerr << "OnUserCreated" << std::endl;

    SetRandomProperties(inUser, realtimeEngine);
}

} // namespace
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ManualConnectionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    bool callbackCalled = false;

    connection->SetConnectionCallback([&callbackCalled](const csp::common::String& /*Message*/) { callbackCalled = true; });

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    WaitForCallback(callbackCalled);
    EXPECT_TRUE(callbackCalled);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterSpaceResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    realtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    EXPECT_EQ(CreatedObject->GetName(), objectName);
    EXPECT_EQ(CreatedObject->GetPosition(), objectTransform.Position);
    EXPECT_EQ(CreatedObject->GetRotation(), objectTransform.Rotation);
    EXPECT_EQ(CreatedObject->GetScale(), objectTransform.Scale);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SignalRConnectionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    InitialiseTestingConnection();

    auto headers = connection->m_connection->HTTPHeaders();
    ASSERT_NE(headers.find("X-DeviceUDID"), headers.end());

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SignalRKeepAliveTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    InitialiseTestingConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    WaitForTestTimeoutCountMs = 0;
    int keepAliveInterval = 200;

    while (WaitForTestTimeoutCountMs < keepAliveInterval)
    {
        std::this_thread::sleep_for(20ms);
        WaitForTestTimeoutCountMs += 20;
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityReplicationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    InitialiseTestingConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    OnConnect(realtimeEngine.get());

    WaitForTestTimeoutCountMs = 0;

    while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        realtimeEngine->ProcessPendingEntityOperations();

        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;

        if (ReceivedEntityUpdatesCount < NumberOfEntityUpdateTicks)
        {
            if (IsReadyForUpdate)
            {
                SetRandomProperties(TestSpaceEntity, realtimeEngine.get());
            }
        }
        else if (ReceivedEntityUpdatesCount == NumberOfEntityUpdateTicks && IsReadyForUpdate) // Send a final update that doesn't change the data
        {
            IsReadyForUpdate = false;
            realtimeEngine->QueueEntityUpdate(TestSpaceEntity);
        }
        else
        {
            IsTestComplete = true;
        }
    }

    EXPECT_TRUE(IsTestComplete);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, SelfReplicationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    if (FlagSetResult == ErrorCode::None)
    {
        csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform
            = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

        realtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        EXPECT_EQ(CreatedObject->GetName(), objectName);
        EXPECT_EQ(CreatedObject->GetPosition(), objectTransform.Position);
        EXPECT_EQ(CreatedObject->GetRotation(), objectTransform.Rotation);
        EXPECT_EQ(CreatedObject->GetScale(), objectTransform.Scale);

        auto modelComponent = dynamic_cast<StaticModelSpaceComponent*>(CreatedObject->AddComponent(ComponentType::StaticModel));
        modelComponent->SetExternalResourceAssetId("SomethingElse");
        modelComponent->SetExternalResourceAssetCollectionId("Something");

        bool entityUpdated = false;

        CreatedObject->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (entity->GetName() == "Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_SCALE)
                    {
                        std::cerr << "Scale Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });
        CreatedObject->SetScale(csp::common::Vector3 { 3.0f, 3.0f, 3.0f });
        CreatedObject->QueueUpdate();

        while (!entityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
        {
            realtimeEngine->ProcessPendingEntityOperations();
            std::this_thread::sleep_for(50ms);
            WaitForTestTimeoutCountMs += 50;
        }

        EXPECT_LE(WaitForTestTimeoutCountMs, WaitForTestTimeoutLimit);

        EXPECT_EQ(CreatedObject->GetScale().X, 3.0f);
        EXPECT_EQ(CreatedObject->GetScale().Y, 3.0f);
        EXPECT_EQ(CreatedObject->GetScale().Z, 3.0f);
    }

    auto [FlagSetResult2] = AWAIT(connection, SetAllowSelfMessagingFlag, false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

// This test currently requires manual steps and will be reviewed as part of OF-1535.
CSP_PUBLIC_TEST(DISABLED_CSPEngine, MultiplayerTests, ConnectionInterruptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    bool interrupted = false;
    bool disconnected = false;

    connection->SetNetworkInterruptionCallback([&interrupted](csp::common::String /*Message*/) { interrupted = true; });

    connection->SetDisconnectionCallback([&disconnected](csp::common::String /*Message*/) { disconnected = true; });

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    csp::common::String userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    realtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = Awaitable(&OnlineRealtimeEngine::CreateAvatar, realtimeEngine.get(), userName, loginState.UserId, userTransform, isVisible,
        userAvatarState, userAvatarId, userAvatarPlayMode, LocomotionModel::Grounded)
                        .Await();

    auto start = std::chrono::steady_clock::now();
    auto current = std::chrono::steady_clock::now();
    long long testTime = 0;

    // Interrupt connection here
    while (!interrupted && testTime < 60)
    {
        std::this_thread::sleep_for(50ms);

        SetRandomProperties(TestSpaceEntity, realtimeEngine.get());

        current = std::chrono::steady_clock::now();
        testTime = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();

        csp::CSPFoundation::Tick();
    }

    EXPECT_TRUE(interrupted);

    // Delete space
    Awaitable(&csp::systems::SpaceSystem::DeleteSpace, spaceSystem, space.Id).Await();

    // Log out
    Awaitable(&csp::systems::UserSystem::Logout, userSystem).Await();
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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    auto thisProcessTestUser = CreateTestUser();

    // Log in
    LogIn(userSystem, userId, thisProcessTestUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Unlisted, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto testUser1 = CreateTestUser();
    auto testUser2 = CreateTestUser();

    MultiplayerTestRunnerProcess createAvatarRunner
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR)
              .SetSpaceId(space.Id.c_str())
              .SetLoginEmail(testUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60)
              .SetEndpoint(EndpointBaseURI());

    MultiplayerTestRunnerProcess createAvatarRunner2
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR)
              .SetSpaceId(space.Id.c_str())
              .SetLoginEmail(testUser2.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60)
              .SetEndpoint(EndpointBaseURI());

    std::array<MultiplayerTestRunnerProcess, 2> runners = { createAvatarRunner, createAvatarRunner2 };
    std::array<std::future<void>, 2> readyForAssertionsFutures = { runners[0].ReadyForAssertionsFuture(), runners[1].ReadyForAssertionsFuture() };

    // Start all the MultiplayerTestRunners
    for (auto& runner : runners)
    {
        runner.StartProcess();
    }

    // Wait until the processes have reached the point where we're ready to assert
    for (auto& future : readyForAssertionsFutures)
    {
        // Just being safe here, so we dont hang forever in case of catastrophe.
        auto status = future.wait_for(std::chrono::seconds(60));

        if (status == std::future_status::timeout)
        {
            FAIL() << "CreateAvatar process timed out before it was ready for assertions.";
        }
    }

    // We must tick the entities or our local CSP instance wont know about the changes the other processes have made.
    realtimeEngine->TickEntities();

    // Check there are 2 avatars in the space.
    // (The two external processes added one each, our process here in the test project just joined the room, didnt add an avatar)
    EXPECT_EQ(realtimeEngine->GetNumAvatars(), 2);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

// Derived type that allows us to access protected members of OnlineRealtimeEngineWeak
class InternalOnlineRealtimeEngine : public csp::multiplayer::OnlineRealtimeEngine
{
    ~InternalOnlineRealtimeEngine();

public:
    void ClearEntities()
    {
        std::scoped_lock<std::recursive_mutex> entitiesLocker(*m_entitiesLock);

        m_entities.Clear();
        m_objects.Clear();
        m_avatars.Clear();
    }
};

// Disabled by default as it can be slow
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ManyEntitiesTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(realtimeEngine->GetNumEntities(), 0);
    EXPECT_EQ(realtimeEngine->GetNumObjects(), 0);

    // Create a bunch of entities
    constexpr size_t numEntitiesToCreate = 15;
    constexpr char entityNamePrefix[] = "Object_";

    SpaceTransform transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    for (size_t i = 0; i < numEntitiesToCreate; ++i)
    {
        csp::common::String name = entityNamePrefix;
        name.Append(std::to_string(i).c_str());

        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, name, transform, csp::common::Optional<uint64_t> {});

        EXPECT_NE(Object, nullptr);
    }

    EXPECT_EQ(realtimeEngine->GetNumEntities(), numEntitiesToCreate);
    EXPECT_EQ(realtimeEngine->GetNumObjects(), numEntitiesToCreate);

    realtimeEngine->ProcessPendingEntityOperations();

    // Clear all entities locally
    auto internalEntitySystem = static_cast<InternalOnlineRealtimeEngine*>(realtimeEngine.get());
    internalEntitySystem->ClearEntities();

    EXPECT_EQ(realtimeEngine->GetNumEntities(), 0);
    EXPECT_EQ(realtimeEngine->GetNumObjects(), 0);

    // Retrieve all entities and verify count
    auto gotAllEntities = false;

    realtimeEngine->RetrieveAllEntities(
        [&](int numEntitiesFetched)
        {
            gotAllEntities = true;
            std::cout << numEntitiesFetched << std::endl;
        });

    while (!gotAllEntities)
    {
        std::this_thread::sleep_for(100ms);
    }

    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(realtimeEngine->GetNumEntities(), numEntitiesToCreate);
    // We created objects exclusively, so this should also be true.
    EXPECT_EQ(realtimeEngine->GetNumEntities(), realtimeEngine->GetNumObjects());

    const auto preLeaveEntities = realtimeEngine->GetNumEntities();
    const auto preLeaveObjects = realtimeEngine->GetNumObjects();
    const auto preLeaveAvatars = realtimeEngine->GetNumAvatars();

    auto [ExitResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    EXPECT_EQ(ExitResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Validate that leaving a space has no impact on the entities contained in the realtimeEngine, clients may reuse it if they wish.
    EXPECT_EQ(realtimeEngine->GetNumEntities(), preLeaveEntities);
    EXPECT_EQ(realtimeEngine->GetNumObjects(), preLeaveObjects);
    EXPECT_EQ(realtimeEngine->GetNumAvatars(), preLeaveAvatars);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

void RunParentEntityReplicationTest(bool local)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // If local is false, test DeserialiseFromPatch functionality
    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, !local);

    // Create Entities
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName1 = "ChildEntity1";
    csp::common::String childEntityName2 = "ChildEntity2";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    realtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity] = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity1] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName1, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity2] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName2, objectTransform, csp::common::Optional<uint64_t> {});

    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

    EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 3);

    // Test setting the parent for the first child
    {
        bool childEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&childEntityUpdated, childEntityName1](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (entity->GetName() == childEntityName1 && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    childEntityUpdated = true;
                }
            });

        CreatedChildEntity1->SetParentId(CreatedParentEntity->GetId());

        // Parents shouldn't be set until after replication
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 3);

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());

        EXPECT_TRUE(childEntityUpdated);

        // Check entity1 is parented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity1);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    // Test setting the parent for the second child
    {
        bool childEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&childEntityUpdated, childEntityName2](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (entity->GetName() == childEntityName2 && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    childEntityUpdated = true;
                }
            });

        CreatedChildEntity2->SetParentId(CreatedParentEntity->GetId());

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());

        EXPECT_TRUE(childEntityUpdated);

        // Check all entities are parented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 2);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[1], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 1);
    }

    // Remove parent from first child
    {
        bool childEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&childEntityUpdated, childEntityName1](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (entity->GetName() == childEntityName1 && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    childEntityUpdated = true;
                }
            });

        CreatedChildEntity1->RemoveParentEntity();

        CreatedChildEntity1->QueueUpdate();

        WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
        EXPECT_TRUE(childEntityUpdated);

        // Check entity is  unparented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    // Remove parent from second child
    {
        bool childEntityUpdated = false;

        CreatedChildEntity2->SetUpdateCallback(
            [&childEntityUpdated, childEntityName2](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (entity->GetName() == childEntityName2 && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    childEntityUpdated = true;
                }
            });

        CreatedChildEntity2->RemoveParentEntity();

        CreatedChildEntity2->QueueUpdate();

        WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
        EXPECT_TRUE(childEntityUpdated);

        // Check entity is  unparented correctly
        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 0);

        EXPECT_EQ(CreatedChildEntity1->GetChildEntities()->Size(), 0);
        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);

        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 3);
    }

    if (!local)
    {
        auto [FlagSetResult2] = AWAIT(connection, SetAllowSelfMessagingFlag, false);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    csp::systems::Profile testUser = CreateTestUser();
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    bool entitiesCreated = false;
    auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName = "ChildEntity";
    csp::common::String rootEntityName = "RootEntity";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    realtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity] = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedRootEntity] = AWAIT(realtimeEngine.get(), CreateEntity, rootEntityName, objectTransform, csp::common::Optional<uint64_t> {});

    uint64_t parentEntityId = CreatedParentEntity->GetId();
    uint64_t childEntityId = CreatedChildEntity->GetId();

    // Parents shouldn't be set yet
    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedRootEntity->GetParentEntity(), nullptr);

    EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 3);

    bool childEntityUpdated = false;

    CreatedChildEntity->SetUpdateCallback(
        [&childEntityUpdated, childEntityName](
            SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
        {
            if (entity->GetName() == childEntityName && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
            {
                childEntityUpdated = true;
            }
        });

    // Change Parent
    CreatedChildEntity->SetParentId(CreatedParentEntity->GetId());

    CreatedChildEntity->QueueUpdate();

    // Wait for update
    WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
    EXPECT_TRUE(childEntityUpdated);

    EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);

    // Exit Space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Log out
    LogOut(userSystem);

    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Log in again
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Enter space
    auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

    WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());
    EXPECT_TRUE(entitiesCreated);

    // Find our entities
    SpaceEntity* retrievedParentEntity = realtimeEngine->FindSpaceEntityById(parentEntityId);
    EXPECT_TRUE(retrievedParentEntity != nullptr);

    SpaceEntity* retrievedChildEntity = realtimeEngine->FindSpaceEntityById(childEntityId);
    EXPECT_TRUE(retrievedChildEntity != nullptr);

    // Check entity is parented correctly
    EXPECT_EQ(retrievedChildEntity->GetParentEntity(), retrievedParentEntity);
    EXPECT_EQ(retrievedParentEntity->GetChildEntities()->Size(), 1);
    EXPECT_EQ((*retrievedParentEntity->GetChildEntities())[0], retrievedChildEntity);

    EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);

    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
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

void StartAlwaysSucceeds(SignalRConnectionMock& signalRMock)
{
    ON_CALL(signalRMock, Start).WillByDefault([](std::function<void(std::exception_ptr)> callback) { callback(nullptr); });
}
void StopAlwaysSucceeds(SignalRConnectionMock& signalRMock)
{
    ON_CALL(signalRMock, Stop).WillByDefault([](std::function<void(std::exception_ptr)> callback) { callback(nullptr); });
}
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRStartErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem logSystem;
    SignalRConnectionMock* signalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection connection { logSystem, *signalRMock };
    csp::multiplayer::NetworkEventBus networkEventBus { &connection, logSystem };

    // The start function will throw internally
    EXPECT_CALL(*signalRMock, Start)
        .WillOnce([](std::function<void(std::exception_ptr)> callback)
            { callback(std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"))); });

    // Then the error callback we be called with an unknown error code
    MockMultiplayerErrorCallback mockErrorCallback;
    EXPECT_CALL(mockErrorCallback, Call(csp::multiplayer::ErrorCode::Unknown));

    // And the disconnection callback will be called with a message (weird)
    MockConnectionCallback mockDisconnectionCallback;
    EXPECT_CALL(mockDisconnectionCallback, Call(csp::common::String("MultiplayerConnection::Start, Error when starting SignalR connection.")));

    connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &mockDisconnectionCallback, std::placeholders::_1));
    connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &mockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRInvokeDeleteObjectsErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem logSystem;
    SignalRConnectionMock* signalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection connection { logSystem, *signalRMock };
    csp::multiplayer::NetworkEventBus networkEventBus { &connection, logSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*signalRMock);
    StopAlwaysSucceeds(*signalRMock);

    // Invoke function for delete objects errors
    EXPECT_CALL(*signalRMock, Invoke)
        .WillOnce(
            [](const std::string& /*DeleteObjectsMethodName*/, const signalr::value& /*DeleteEntityMessage*/,
                std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                auto value = signalr::value("Irrelevant value from DeleteObjects");
                auto except = std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"));
                callback(value, except);
                return async::make_task(std::make_tuple(value, except));
            });

    // Then the error callback we be called with an no error code
    MockMultiplayerErrorCallback mockErrorCallback;
    EXPECT_CALL(mockErrorCallback, Call(csp::multiplayer::ErrorCode::None));

    // And the disconnection callback will be called with a message (weird)
    MockConnectionCallback mockDisconnectionCallback;
    EXPECT_CALL(mockDisconnectionCallback,
        Call(csp::common::String("MultiplayerConnection::DeleteEntities, Unexpected error response from SignalR \"DeleteObjects\" invocation.")));

    connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &mockDisconnectionCallback, std::placeholders::_1));
    connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &mockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRInvokeGetClientIdErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem logSystem;
    SignalRConnectionMock* signalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection connection { logSystem, *signalRMock };
    csp::multiplayer::NetworkEventBus networkEventBus { &connection, logSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*signalRMock);
    StopAlwaysSucceeds(*signalRMock);

    EXPECT_CALL(*signalRMock, Invoke)
        .WillRepeatedly(
            [&connection](const std::string& hubMethodName, const signalr::value& /*Message*/,
                std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                if (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::DELETE_OBJECTS))
                {
                    // Succeed deleting objects
                    auto value = signalr::value("Irrelevant value from DeleteObjects");
                    callback(value, nullptr);
                    return async::make_task(std::make_tuple(value, std::exception_ptr(nullptr)));
                }
                else if (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GET_CLIENT_ID))
                {
                    // Fail getting client Id
                    auto value = signalr::value("Irrelevant value from GetClientId");
                    auto except = std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"));
                    callback(value, except);
                    return async::make_task(std::make_tuple(value, except));
                }

                // Just a default case, shouldn't matter
                return async::make_task(std::make_tuple(signalr::value("mock value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    // Then the error callback we be called with no error code
    MockMultiplayerErrorCallback mockErrorCallback;
    EXPECT_CALL(mockErrorCallback, Call(csp::multiplayer::ErrorCode::None));

    // And the disconnection callback will be called with a message
    MockConnectionCallback mockDisconnectionCallback;
    EXPECT_CALL(
        mockDisconnectionCallback, Call(csp::common::String("MultiplayerConnection::RequestClientId, Error when starting requesting Client Id.")));

    connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &mockDisconnectionCallback, std::placeholders::_1));
    connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &mockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenSignalRInvokeStartListeningErrorsThenDisconnectionFunctionsCalled)
{
    csp::common::LogSystem logSystem;
    SignalRConnectionMock* signalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection connection { logSystem, *signalRMock };
    csp::multiplayer::NetworkEventBus networkEventBus { &connection, logSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*signalRMock);
    StopAlwaysSucceeds(*signalRMock);

    EXPECT_CALL(*signalRMock, Invoke)
        .WillRepeatedly(
            [&connection](const std::string& hubMethodName, const signalr::value& /*Message*/,
                std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                if (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::DELETE_OBJECTS))
                {
                    // Succeed deleting objects
                    auto value = signalr::value("Irrelevant value from DeleteObjects");
                    callback(value, nullptr);
                    return async::make_task(std::make_tuple(value, std::exception_ptr(nullptr)));
                }
                else if (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GET_CLIENT_ID))
                {
                    // Succeed getting client Id
                    callback(signalr::value(std::uint64_t(0)), nullptr);
                    return async::make_task(std::make_tuple(signalr::value(std::uint64_t(0)), std::exception_ptr(nullptr)));
                }
                else if (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::START_LISTENING))
                {
                    // Fail to start listening
                    auto except = std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"));
                    callback(signalr::value(std::uint64_t(0)), except);
                    return async::make_task(std::make_tuple(signalr::value(std::uint64_t(0)), except));
                }

                // Just a default case, shouldn't matter
                return async::make_task(std::make_tuple(signalr::value("mock value"),
                    std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"))));
            });

    // Then the error callback we be called with no error code
    MockMultiplayerErrorCallback mockErrorCallback;
    EXPECT_CALL(mockErrorCallback, Call(csp::multiplayer::ErrorCode::None));

    // And the disconnection callback will be called with a message
    MockConnectionCallback mockDisconnectionCallback;
    EXPECT_CALL(mockDisconnectionCallback, Call(csp::common::String("Multiplayer Error. mock exception")));

    connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &mockDisconnectionCallback, std::placeholders::_1));
    connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &mockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, WhenAllSignalRSucceedsThenSuccessCallbacksCalled)
{
    csp::common::LogSystem logSystem;
    SignalRConnectionMock* signalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection connection { logSystem, *signalRMock };
    csp::multiplayer::NetworkEventBus networkEventBus { &connection, logSystem };

    // Start and stop will call their callbacks
    StartAlwaysSucceeds(*signalRMock);
    StopAlwaysSucceeds(*signalRMock);

    EXPECT_CALL(*signalRMock, Invoke)
        .WillRepeatedly(
            [&connection](const std::string& hubMethodName, const signalr::value& /*Message*/,
                std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                if (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::DELETE_OBJECTS))
                {
                    // Succeed deleting objects
                    auto value = signalr::value("Irrelevant value from DeleteObjects");
                    callback(value, nullptr);
                    return async::make_task(std::make_tuple(value, std::exception_ptr(nullptr)));
                }
                else if ((hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GET_CLIENT_ID))
                    || (hubMethodName == connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::START_LISTENING)))
                {
                    // Succeed getting client Id
                    callback(signalr::value(std::uint64_t(0)), nullptr);
                    return async::make_task(std::make_tuple(signalr::value(std::uint64_t(0)), std::exception_ptr(nullptr)));
                }

                // Just a default case, shouldn't matter
                return async::make_task(std::make_tuple(signalr::value("mock value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    // Then the error callback will be called with no error
    MockMultiplayerErrorCallback mockErrorCallback;
    EXPECT_CALL(mockErrorCallback, Call(ErrorCode::None));

    // And the connection callback with be called
    MockConnectionCallback mockSuccessConnectionCallback;
    EXPECT_CALL(mockSuccessConnectionCallback, Call(csp::common::String("Successfully connected to SignalR hub.")));

    // And the disconnection callback will not be called
    MockConnectionCallback mockDisconnectionCallback;
    EXPECT_CALL(mockDisconnectionCallback, Call(::testing::_)).Times(0);

    connection.SetConnectionCallback(std::bind(&MockConnectionCallback::Call, &mockSuccessConnectionCallback, std::placeholders::_1));
    connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &mockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, TestParseMultiplayerError)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();

    // ParseMultiplayerError is odd, it seems only concerned with understanding this "Scopes_ConcurrentUsersQuota error"
    // I'm actually not sure if CHS even still throws this format of errors, this could be completely redundant...
    auto [ErrorCodeTooManyUsers, MsgTooManyUsers] = connection->ParseMultiplayerError(std::runtime_error("error code: Scopes_ConcurrentUsersQuota"));
    EXPECT_EQ(ErrorCodeTooManyUsers, csp::multiplayer::ErrorCode::SpaceUserLimitExceeded);
    EXPECT_EQ(MsgTooManyUsers, "error code: Scopes_ConcurrentUsersQuota");

    auto [ErrorCodeUnknown, MsgUnknown] = connection->ParseMultiplayerError(std::runtime_error("Some unknown error"));
    EXPECT_EQ(ErrorCodeUnknown, csp::multiplayer::ErrorCode::Unknown);
    EXPECT_EQ(MsgUnknown, "Some unknown error");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, EntityLockPersistanceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    const csp::systems::Profile testUser = CreateTestUser();
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Enter space
    bool entitiesCreated = false;

    auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

    // Ensure patch rate limiting is off, as we're sending patches in quick succession.
    realtimeEngine->SetEntityPatchRateLimitEnabled(false);

    // Enter a space and lock an entity
    {
        // Enter space
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create Entity
        const csp::common::String entityName = "Entity";
        const SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

        auto [CreatedEntity] = AWAIT(realtimeEngine.get(), CreateEntity, entityName, objectTransform, csp::common::Optional<uint64_t> {});

        // Lock Entity
        CreatedEntity->Lock();

        // Apply patch
        CreatedEntity->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        // Entity should be locked now
        EXPECT_TRUE(CreatedEntity->IsLocked());
    }

    // Re-enter space to ensure updates were made to the server
    {
        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        // Log out
        LogOut(userSystem);

        // Wait a few seconds for the CHS database to update
        std::this_thread::sleep_for(std::chrono::seconds(8));

        // Log in again
        LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

        // Enter space
        entitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());
        EXPECT_TRUE(entitiesCreated);

        SpaceEntity* entity = realtimeEngine->GetEntityByIndex(0);
        EXPECT_TRUE(entity->IsLocked());
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/* Check that the SignalR connection is not used when we login without creating a multiplayer connection
 * Ideally, this should be as simple as checking if MultiplayerConnection is null, managed by whether the
 * user chooses to instantiate and inject a multiplayer connection or not, but we're not there yet
 */
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, MultiplayerTestsMock, TestNoSignalRCommunicationWhenLoggedInWithoutConnection)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    EXPECT_CALL(*m_webClientMock, SendRequest)
        .WillOnce(
            [](auto verb, const auto& uri, auto& payload, auto* responseCallback, const auto& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                ASSERT_EQ(verb, csp::web::ERequestVerb::POST);
                ASSERT_TRUE(csp::common::String(uri.GetAsString()).Contains("/mag-user/api/v1/users/login"));
                ASSERT_TRUE(payload.GetContent().Contains("mock.user@magnopus.com"));
                ASSERT_TRUE(payload.GetContent().Contains(GeneratedTestAccountPassword));

                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                response.GetMutablePayload().SetContent(R"({
                    "accessToken": "MockAccessToken",
                    "accessTokenExpiresAt": "1970-01-01T00:00:00.000+00:00",
                    "refreshToken": "MockRefreshToken",
                    "userId": "MockUserId",
                    "deviceId": "MockDeviceId"
                })");
                responseCallback->OnHttpResponse(response);
            });

    // Log in
    csp::common::String userId;
    LogIn(userSystem, userId, "mock.user@magnopus.com", GeneratedTestAccountPassword, false, true);

    std::unique_ptr<csp::multiplayer::OfflineRealtimeEngine> realtimeEngine { systemsManager.MakeOfflineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    EXPECT_FALSE(connection->IsConnected());

    // Expect that none of the signalR methods are ever called
    EXPECT_CALL(*m_signalRMock, Start).Times(0);
    EXPECT_CALL(*m_signalRMock, Stop).Times(0);
    EXPECT_CALL(*m_signalRMock, GetConnectionState).Times(0);
    EXPECT_CALL(*m_signalRMock, GetConnectionId).Times(0);
    EXPECT_CALL(*m_signalRMock, SetDisconnected).Times(0);
    EXPECT_CALL(*m_signalRMock, On).Times(0);
    EXPECT_CALL(*m_signalRMock, Invoke).Times(0);
    EXPECT_CALL(*m_signalRMock, Send).Times(0);
    EXPECT_CALL(*m_signalRMock, HTTPHeaders).Times(0);

    // Do some stuff
    const csp::common::String localSpaceId = "LocalSpace";
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, localSpaceId, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String entityName = "Entity";
    const SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

    auto [CreatedEntity] = AWAIT(realtimeEngine.get(), CreateEntity, entityName, objectTransform, csp::common::Optional<uint64_t> {});

    csp::common::Vector3 position = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    CreatedEntity->SetPosition(position);

    csp::common::Vector4 rotation
        = { static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100) };
    CreatedEntity->SetRotation(rotation);

    csp::CSPFoundation::Tick();
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, TestMultiplayerDisconnectionWhenNewMultiplayerSessionInitiated)
{
    csp::common::LogSystem logSystem;
    SignalRConnectionMock* signalRMock = new SignalRConnectionMock();
    csp::multiplayer::MultiplayerConnection connection { logSystem, *signalRMock };
    csp::multiplayer::NetworkEventBus networkEventBus { &connection, logSystem };

    // Intercept the 'OnRequestToDisconnect' event and dummy expected response for new user login on different client
    ON_CALL(*signalRMock, On)
        .WillByDefault(
            [&connection](
                const std::string& eventName, const ISignalRConnection::MethodInvokedHandler& handler, csp::common::LogSystem& /*LogSystem*/)
            {
                if (eventName == "OnRequestToDisconnect")
                {
                    handler(std::vector { signalr::value("New Multiplayer Session Initiated") });
                }

                return true;
            });

    // Then the error callback we be called
    MockMultiplayerErrorCallback mockErrorCallback;

    // And the disconnection callback will be called with a message
    MockConnectionCallback mockDisconnectionCallback;
    EXPECT_CALL(mockDisconnectionCallback, Call(csp::common::String("New Multiplayer Session Initiated")));

    connection.SetDisconnectionCallback(std::bind(&MockConnectionCallback::Call, &mockDisconnectionCallback, std::placeholders::_1));
    connection.Connect(std::bind(&MockMultiplayerErrorCallback::Call, &mockErrorCallback, std::placeholders::_1), "", "", "");
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, ConnectionInterruptedTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    const csp::systems::Profile testUser = CreateTestUser();

    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create avatar
    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    csp::common::String userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    realtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = Awaitable(&OnlineRealtimeEngine::CreateAvatar, realtimeEngine.get(), userName, loginState.UserId, userTransform, isVisible,
        userAvatarState, userAvatarId, userAvatarPlayMode, LocomotionModel::Grounded)
                        .Await();

    // Set network interrupted callback
    bool interrupted = false;
    connection->SetNetworkInterruptionCallback([&interrupted](csp::common::String /*Message*/) { interrupted = true; });

    // Cause signalr to fail
    connection->__CauseFailure();

    WaitForCallback(interrupted);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
    Ensures IsEntityModifiable works under the correct conditions.
    Check OnlineRealtimeEngine::IsEntityModifiable docs for details.
*/
CSP_PUBLIC_TEST(CSPEngine, MultiplayerTests, IsModifiableTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    const csp::systems::Profile testUser = CreateTestUser();

    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String entityName = "Entity1";

    SpaceEntity* entity = CreateTestObject(realtimeEngine.get());

    // Entity should be modifiable when first created as it is not locked by default.
    EXPECT_EQ(realtimeEngine->IsEntityModifiable(entity), ModifiableStatus::Modifiable);

    entity->Lock();
    entity->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Entity should not be modifiable now it is locked.
    EXPECT_EQ(realtimeEngine->IsEntityModifiable(entity), ModifiableStatus::EntityLocked);

    entity->Unlock();
    entity->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Entity should be modifiable again.
    EXPECT_EQ(realtimeEngine->IsEntityModifiable(entity), ModifiableStatus::Modifiable);

    // Make the entity non-transferable, and not owned by this client.
    entity->m_isTransferable = false;
    entity->m_ownerId = 0;

    EXPECT_EQ(realtimeEngine->IsEntityModifiable(entity), ModifiableStatus::EntityNotOwnedAndUntransferable);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}