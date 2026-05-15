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
#include "CSP/Common/Optional.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "MultiplayerTestRunnerProcess.h"
#include "RAIIMockLogger.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "signalrclient/signalr_value.h"

#include "gtest/gtest.h"
#include <array>
#include <chrono>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{
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

} // namespace

namespace CSPEngine
{

class CreateAvatar : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class CreateCreatorAvatar : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class AvatarMovementDirection : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class ObjectCreate : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class ObjectAddComponent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class ObjectRemoveComponent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class ObjectRemoveComponentTestReenterSpace : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class DeleteMultipleEntities : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntitySelection : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class InvalidComponentFields : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntityGlobalPosition : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntityGlobalRotation : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntityGlobalScale : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntityGlobalTransform : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class CreateObjectParent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntityLockAddComponent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};
class EntityLockRemoveComponent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class EntityUpdateCallback : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

TEST_P(CreateAvatar, CreateAvatarTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String& userName = "Player 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;
    LocomotionModel userAvatarLocomotionModel = LocomotionModel::FreeCamera;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, userAvatarLocomotionModel);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), userName);
    EXPECT_EQ(Avatar->GetPosition(), userTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), userTransform.Rotation);

    auto& components = *Avatar->GetComponents();

    EXPECT_EQ(components.Size(), 1);

    auto* component = components[0];

    EXPECT_EQ(component->GetComponentType(), ComponentType::AvatarData);

    // Verify the values of UserAvatarState and UserAvatarPlayMode
    auto* avatarComponent = dynamic_cast<AvatarSpaceComponent*>(component);

    EXPECT_NE(avatarComponent, nullptr);
    EXPECT_EQ(avatarComponent->GetState(), userAvatarState);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), userAvatarPlayMode);
    EXPECT_EQ(avatarComponent->GetLocomotionModel(), userAvatarLocomotionModel);
    EXPECT_EQ(avatarComponent->GetIsVisible(), isVisible);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(CreateCreatorAvatar, CreateCreatorAvatarTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String& userName = "Creator 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "MyCoolCreatorAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Creator;
    LocomotionModel userAvatarLocomotionModel = LocomotionModel::Grounded;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, userAvatarLocomotionModel);
    EXPECT_NE(Avatar, nullptr);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), userName);
    EXPECT_EQ(Avatar->GetPosition(), userTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), userTransform.Rotation);

    auto& components = *Avatar->GetComponents();

    EXPECT_EQ(components.Size(), 1);

    auto* component = components[0];

    EXPECT_EQ(component->GetComponentType(), ComponentType::AvatarData);

    // Verify the values of UserAvatarState and UserAvatarPlayMode
    AvatarSpaceComponent* avatarComponent = dynamic_cast<AvatarSpaceComponent*>(component);
    EXPECT_NE(avatarComponent, nullptr);
    EXPECT_EQ(avatarComponent->GetState(), userAvatarState);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), userAvatarPlayMode);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), AvatarPlayMode::Creator);
    EXPECT_EQ(avatarComponent->GetLocomotionModel(), userAvatarLocomotionModel);
    EXPECT_EQ(avatarComponent->GetIsVisible(), isVisible);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(AvatarMovementDirection, AvatarMovementDirectionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String& userName = "Player 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    auto& components = *Avatar->GetComponents();
    EXPECT_EQ(components.Size(), 1);

    auto* component = components[0];
    EXPECT_EQ(component->GetComponentType(), ComponentType::AvatarData);

    AvatarSpaceComponent* avatarComponent = dynamic_cast<AvatarSpaceComponent*>(component);
    EXPECT_NE(avatarComponent, nullptr);

    // test setting and getting movement direction
    avatarComponent->SetMovementDirection(csp::common::Vector3::One());

    Avatar->QueueUpdate();

    EXPECT_EQ(avatarComponent->GetMovementDirection(), csp::common::Vector3::One());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ObjectCreate, ObjectCreateTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    InitialiseTestingConnection();

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    EXPECT_EQ(CreatedObject->GetName(), objectName);
    EXPECT_EQ(CreatedObject->GetPosition(), objectTransform.Position);
    EXPECT_EQ(CreatedObject->GetRotation(), objectTransform.Rotation);
    EXPECT_EQ(CreatedObject->GetScale(), objectTransform.Scale);
    EXPECT_EQ(CreatedObject->GetThirdPartyRef(), "");

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ObjectAddComponent, ObjectAddComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    bool patchPending = true;
    Object->SetPatchSentCallback([&patchPending](bool /*ok*/) { patchPending = false; });

    const csp::common::String modelAssetId = "NotARealId";

    auto* staticModelComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    auto staticModelComponentKey = staticModelComponent->GetId();
    staticModelComponent->SetExternalResourceAssetId(modelAssetId);

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();
        while (patchPending)
        {
            ProcessPendingIfOnline(*realtimeEngine);
            std::this_thread::sleep_for(10ms);
        }
    }

    patchPending = true;

    auto& components = *Object->GetComponents();

    EXPECT_EQ(components.Size(), 1);
    EXPECT_TRUE(components.HasKey(staticModelComponentKey));

    auto* fetchedStaticModelComponent = Object->GetComponent(staticModelComponentKey);

    EXPECT_EQ(fetchedStaticModelComponent->GetComponentType(), ComponentType::StaticModel);
    auto* realStaticModelComponent = (StaticModelSpaceComponent*)fetchedStaticModelComponent;

    EXPECT_EQ(realStaticModelComponent->GetExternalResourceAssetId(), modelAssetId);

    const csp::common::String imageAssetId = "AlsoNotARealId";

    auto* imageComponent = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
    auto imageModelComponentKey = imageComponent->GetId();
    imageComponent->SetImageAssetId(imageAssetId);

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();

        while (patchPending)
        {
            ProcessPendingIfOnline(*realtimeEngine);
            std::this_thread::sleep_for(10ms);
        }
    }

    EXPECT_EQ(Object->GetComponents()->Size(), 2);
    EXPECT_TRUE(components.HasKey(staticModelComponentKey));
    EXPECT_TRUE(components.HasKey(imageModelComponentKey));

    auto* fetchedImageComponent = Object->GetComponent(imageModelComponentKey);

    EXPECT_EQ(fetchedImageComponent->GetComponentType(), ComponentType::Image);
    auto* realImageComponent = (ImageSpaceComponent*)fetchedImageComponent;

    EXPECT_EQ(realImageComponent->GetImageAssetId(), imageAssetId);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ObjectRemoveComponent, ObjectRemoveComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    bool patchPending = true;
    Object->SetPatchSentCallback([&patchPending](bool /*ok*/) { patchPending = false; });

    const csp::common::String modelAssetId = "NotARealId";

    auto* staticModelComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    auto staticModelComponentKey = staticModelComponent->GetId();
    staticModelComponent->SetExternalResourceAssetId(modelAssetId);
    auto* imageComponent = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
    auto imageComponentKey = imageComponent->GetId();
    imageComponent->SetImageAssetId("TestID");
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();

        while (patchPending)
        {
            ProcessPendingIfOnline(*realtimeEngine);
            std::this_thread::sleep_for(10ms);
        }
    }

    patchPending = true;

    auto& components = *Object->GetComponents();

    EXPECT_EQ(components.Size(), 2);
    EXPECT_TRUE(components.HasKey(staticModelComponentKey));
    EXPECT_TRUE(components.HasKey(imageComponentKey));

    auto* fetchedStaticModelComponent = Object->GetComponent(staticModelComponentKey);

    EXPECT_EQ(fetchedStaticModelComponent->GetComponentType(), ComponentType::StaticModel);
    auto* realStaticModelComponent = (StaticModelSpaceComponent*)fetchedStaticModelComponent;

    EXPECT_EQ(realStaticModelComponent->GetExternalResourceAssetId(), modelAssetId);

    Object->RemoveComponent(staticModelComponentKey);
    Object->RemoveComponent(imageComponentKey);

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();

        while (patchPending)
        {
            ProcessPendingIfOnline(*realtimeEngine);
            std::this_thread::sleep_for(10ms);
        }
    }

    auto& realComponents = *Object->GetComponents();

    EXPECT_EQ(realComponents.Size(), 0);
    EXPECT_FALSE(realComponents.HasKey(staticModelComponentKey));
    EXPECT_FALSE(realComponents.HasKey(imageComponentKey));

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ObjectRemoveComponentTestReenterSpace, ObjectRemoveComponentTestReenterSpace)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    bool entitiesCreated = false;
    auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

    const csp::common::String objectName = "Object 1";

    uint16_t keepKey = 0;
    uint16_t deleteKey = 0;

    {
        // Enter space
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());

        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        bool patchPending = true;
        Object->SetPatchSentCallback([&patchPending](bool /*ok*/) { patchPending = false; });

        auto* componentToKeep = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
        componentToKeep->SetComponentName("ComponentNameKeep");
        keepKey = componentToKeep->GetId();
        auto* componentToDelete = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
        componentToDelete->SetComponentName("ComponentNameDelete");
        deleteKey = componentToDelete->GetId();

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            Object->QueueUpdate();

            while (patchPending)
            {
                ProcessPendingIfOnline(*realtimeEngine);
                std::this_thread::sleep_for(10ms);
            }
        }

        patchPending = true;

        // Ensure values are set correctly
        EXPECT_EQ(componentToKeep->GetComponentName(), "ComponentNameKeep");
        EXPECT_EQ(componentToDelete->GetComponentName(), "ComponentNameDelete");

        auto& components = *Object->GetComponents();

        EXPECT_EQ(components.Size(), 2);
        EXPECT_TRUE(components.HasKey(keepKey));
        EXPECT_TRUE(components.HasKey(deleteKey));

        // Delete component
        Object->RemoveComponent(componentToDelete->GetId());
        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            Object->QueueUpdate();
            while (patchPending)
            {
                ProcessPendingIfOnline(*realtimeEngine);
                std::this_thread::sleep_for(10ms);
            }
            EXPECT_FALSE(patchPending);
        }

        // Check deletion has happened
        auto& realComponents = *Object->GetComponents();

        EXPECT_EQ(realComponents.Size(), 1);
        EXPECT_TRUE(realComponents.HasKey(keepKey));
        EXPECT_FALSE(realComponents.HasKey(deleteKey));

        // Exit space and enter again, making sure the entities have been created
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        // Wait a few seconds for the CHS database to update
        std::this_thread::sleep_for(std::chrono::seconds(8));
    }
    {
        entitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());
        EXPECT_TRUE(entitiesCreated);

        // Retrieve components in space
        SpaceEntity* foundEntity = realtimeEngine->FindSpaceObject(objectName);
        EXPECT_TRUE(foundEntity != nullptr);
        auto& foundComponents = *foundEntity->GetComponents();

        // Check the right component has been deleted
        EXPECT_EQ(foundComponents.Size(), 1);
        EXPECT_TRUE(foundComponents.HasKey(keepKey));
        EXPECT_FALSE(foundComponents.HasKey(deleteKey));
        EXPECT_EQ(foundEntity->GetComponent(0)->GetComponentName(), "ComponentNameKeep");

        // Exit space
        auto [ExitSpaceResult2] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        realtimeEngine.reset();
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(DeleteMultipleEntities, DeleteMultipleEntitiesTest)
{
    // Test for OB-1046
    // If the rate limiter hasn't processed all PendingOutgoingUpdates after SpaceEntity deletion it will crash when trying to process them

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create 3 seperate objects to ensure there is too many updates for the rate limiter to process in one tick

    // Create object
    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});
    CreatedObject->AddComponent(ComponentType::Image);
    CreatedObject->QueueUpdate();

    // Create object 2
    auto [CreatedObject2] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});
    CreatedObject2->AddComponent(ComponentType::Image);
    CreatedObject2->QueueUpdate();

    // Create object 3
    auto [CreatedObject3] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});
    CreatedObject3->AddComponent(ComponentType::Image);
    CreatedObject3->QueueUpdate();

    // Destroy Entites
    realtimeEngine->DestroyEntity(CreatedObject, [](bool) { });
    realtimeEngine->DestroyEntity(CreatedObject2, [](bool) { });
    realtimeEngine->DestroyEntity(CreatedObject3, [](bool) { });

    csp::CSPFoundation::Tick();

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntitySelection, EntitySelectionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::String& userName = "Player 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    CreatedObject->Select();

    EXPECT_TRUE(CreatedObject->IsSelected());

    CreatedObject->Deselect();

    EXPECT_FALSE(CreatedObject->IsSelected());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(InvalidComponentFields, InvalidComponentFieldsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String callbackAssetId;

    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String modelAssetId = "NotARealId";

    Object->AddComponent(ComponentType::Invalid);

    // Process component creation
    Object->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityGlobalPosition, EntityGlobalPositionTest)
{
    // Tests the OnlineRealtimeEngine::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName = "ChildEntity";
    // create a parent child entity, where the parent is positioned at the position [1,1,1], and the child is position [1,0,0] relative to the parent
    SpaceTransform objectTransformParent
        = { csp::common::Vector3 { 1, 1, 1 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform objectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform objectTransformExpected
        = { csp::common::Vector3 { 2, 1, 1 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedParentEntity]
        = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName, objectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!childEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        ProcessPendingIfOnline(*realtimeEngine);
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(childEntityUpdated);

    // The expected outcome is that rotation and scale are unaffected, but the child is translated to position [2,1,1]
    csp::common::Vector3 globalPosition = CreatedChildEntity->GetGlobalPosition();
    csp::common::Vector4 globalRotation = CreatedChildEntity->GetGlobalRotation();
    csp::common::Vector3 globalScale = CreatedChildEntity->GetGlobalScale();

    EXPECT_EQ(objectTransformExpected.Position == globalPosition, true);
    EXPECT_EQ(objectTransformExpected.Rotation.X == globalRotation.X, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Y == globalRotation.Y, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Z == globalRotation.Z, true);
    // When performing quaternion operations, W can be negative, so no point checking
    EXPECT_EQ(objectTransformExpected.Scale == globalScale, true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityGlobalRotation, EntityGlobalRotationTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName = "ChildEntity";
    // Parent has a position [0,0,0], and 1.507 radian (90 degree) rotation around the y axis
    SpaceTransform objectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform objectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform objectTransformExpected
        = { csp::common::Vector3 { 0, 0, 1 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedParentEntity]
        = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName, objectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!childEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        ProcessPendingIfOnline(*realtimeEngine);
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(childEntityUpdated);

    // expectation is that scale is unaffected, rotation is passed on from parent,
    // and child is displaced to position [0, 0, 1], within floating point accuracy limits
    csp::common::Vector3 globalPosition = CreatedChildEntity->GetGlobalPosition();
    csp::common::Vector4 globalRotation = CreatedChildEntity->GetGlobalRotation();
    csp::common::Vector3 globalScale = CreatedChildEntity->GetGlobalScale();

    EXPECT_EQ(objectTransformExpected.Position == globalPosition, true);
    EXPECT_EQ(objectTransformExpected.Rotation.X == globalRotation.X, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Y == globalRotation.Y, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Z == globalRotation.Z, true);
    EXPECT_EQ(objectTransformExpected.Scale == globalScale, true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityGlobalScale, EntityGlobalScaleTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName = "ChildEntity";

    // Create a parent, positioned at the origin, rotated 90 degrees, with a scale of -0.5 on x axis and 0.5 on Y/Z axes
    // child created at a position of [1,0,0], no rotation, and scale of 1
    SpaceTransform objectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { -0.5f, 0.5f, 0.5f } };
    SpaceTransform objectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };

    SpaceTransform objectTransformExpected = { csp::common::Vector3 { 0, 0, -0.5f }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f },
        csp::common::Vector3 { -0.5f, 0.5f, 0.5f } };

    auto [CreatedParentEntity]
        = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName, objectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!childEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        ProcessPendingIfOnline(*realtimeEngine);
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(childEntityUpdated);
    // expectation is that the global data will have position [0,0,-0.5] (scaled by -0.5, then rotated 90 degrees from [1,0,0] around Y axis)
    // rotation will be same as parent
    // scale will now be [-0.5,0.5,0.5], same as parent
    csp::common::Vector3 globalPosition = CreatedChildEntity->GetGlobalPosition();
    csp::common::Vector4 globalRotation = CreatedChildEntity->GetGlobalRotation();
    csp::common::Vector3 globalScale = CreatedChildEntity->GetGlobalScale();

    EXPECT_EQ(objectTransformExpected.Position == globalPosition, true);
    EXPECT_EQ(objectTransformExpected.Rotation.X == globalRotation.X, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Y == globalRotation.Y, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Z == globalRotation.Z, true);
    EXPECT_EQ(objectTransformExpected.Scale == globalScale, true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityGlobalTransform, EntityGlobalTransformTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName = "ChildEntity";
    SpaceTransform objectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform objectTransformChild
        = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 0.5f, 0.5f, 0.5f } };
    SpaceTransform objectTransformExpected
        = { csp::common::Vector3 { 0, 0, 1 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 0.5f, 0.5f, 0.5f } };

    auto [CreatedParentEntity]
        = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName, objectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!childEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        ProcessPendingIfOnline(*realtimeEngine);
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    EXPECT_TRUE(childEntityUpdated);
    SpaceTransform objectTransformActual = CreatedChildEntity->GetGlobalTransform();

    EXPECT_EQ(objectTransformExpected.Position == objectTransformActual.Position, true);
    EXPECT_EQ(objectTransformExpected.Rotation.X == objectTransformActual.Rotation.X, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Y == objectTransformActual.Rotation.Y, true);
    EXPECT_EQ(objectTransformExpected.Rotation.Z == objectTransformActual.Rotation.Z, true);
    EXPECT_EQ(objectTransformExpected.Scale == objectTransformActual.Scale, true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(CreateObjectParent, CreateObjectParentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName = "ChildEntity";

    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedParentEntity] = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(CreatedParentEntity, CreateChildEntity, childEntityName, objectTransform);

    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity->GetParentEntity(), CreatedParentEntity);

    EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityLockAddComponent, EntityLockAddComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    const csp::systems::Profile testUser = CreateTestUser();
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Enter a space and lock an entity
    {
        std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

        // Enter space
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create Entity
        csp::multiplayer::SpaceEntity* createdEntity = CreateTestObject(realtimeEngine.get());

        // Lock Entity
        createdEntity->Lock();

        // Apply patch
        createdEntity->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        // Entity should be locked now
        EXPECT_TRUE(createdEntity->IsLocked());

        {
            // Ensure the add component error message is logged when we try to add a component to a locked entity.
            static const csp::common::String addComponentErrorMsg
                = "Failed to add component: Entity is locked, skipping update. Entity name: Object";

            RAIIMockLogger mockLogger {};
            EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, addComponentErrorMsg)).Times(1);

            // Attempt to add a component to a locked entity
            auto newComponent = createdEntity->AddComponent(ComponentType::StaticModel);

            EXPECT_EQ(newComponent, nullptr);
        }

        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        realtimeEngine.reset();
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityLockRemoveComponent, EntityLockRemoveComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    const csp::systems::Profile testUser = CreateTestUser();
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Enter a space and lock an entity
    {
        std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

        // Enter space
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create Entity
        csp::multiplayer::SpaceEntity* createdEntity = CreateTestObject(realtimeEngine.get());

        // Add a component to the entity
        auto newComponent = createdEntity->AddComponent(ComponentType::StaticModel);
        EXPECT_NE(newComponent, nullptr);

        // Lock Entity
        createdEntity->Lock();

        // Apply patch
        createdEntity->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        // Entity should be locked now
        EXPECT_TRUE(createdEntity->IsLocked());

        {
            // Ensure the remove component error message is logged when we try to remove a component from a locked entity.
            static const csp::common::String removeComponentErrorMsg
                = "Failed to remove component: Entity is locked, skipping update. Entity name: Object";

            RAIIMockLogger mockLogger {};
            EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, removeComponentErrorMsg)).Times(1);

            // Attempt to remove a component from a locked entity
            createdEntity->RemoveComponent(newComponent->GetId());
        }

        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        realtimeEngine.reset();
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(EntityUpdateCallback, EntityUpdateCallbackTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::RealtimeEngineType realtimeEngineType = GetParam();

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    bool callbackCalled = false;

    CreatedObject->SetUpdateCallback(
        [&callbackCalled](SpaceEntity*, SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>&) { callbackCalled = true; });

    // Set the entity name to the same value as it currently is.
    CreatedObject->SetName(objectName);

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        CreatedObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);
    }

    // Ensure the callback wasn't called.
    EXPECT_FALSE(callbackCalled);

    // Cleanup
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

INSTANTIATE_TEST_SUITE_P(
    RealtimeEngineEntityTests, CreateAvatar, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, CreateCreatorAvatar,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, AvatarMovementDirection,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    RealtimeEngineEntityTests, ObjectCreate, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, ObjectAddComponent,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, ObjectRemoveComponent,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, ObjectRemoveComponentTestReenterSpace,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, DeleteMultipleEntities,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    RealtimeEngineEntityTests, EntitySelection, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, InvalidComponentFields,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityGlobalPosition,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityGlobalRotation,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    RealtimeEngineEntityTests, EntityGlobalScale, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityGlobalTransform,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, CreateObjectParent,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityLockAddComponent,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityLockRemoveComponent,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityUpdateCallback,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

// The bool here is "Local". It means for the online engine, do we enable the SetAllowSelfMessagingFlag to test local messaging.
// Is supposed to test the SpaceEntity::SerializeFromPatch and SpaceEntity::ApplyLocalPatch functionality
// Not sure why these specific tests care about this, this doc is being written as part of a migration. Nonetheless, maintain the coverage.
class EntityLock : public PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool>>
{
};
class ParentDeletion : public PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool>>
{
};
class ParentChildDeletion : public PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool>>
{
};

TEST_P(EntityLock, EntityLockTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    csp::common::RealtimeEngineType realtimeEngineType = std::get<0>(GetParam());
    const bool local = std::get<1>(GetParam());

    // Log in
    csp::common::String userId;

    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        // Ensure patch rate limiting is off, as we're sending patches in quick succession.
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetEntityPatchRateLimitEnabled(false);

        // If local is false, test DeserialiseFromPatch functionality
        auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, !local);
        EXPECT_EQ(FlagSetResult, csp::multiplayer::ErrorCode::None);
    }

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    {
        // Create Entity
        const csp::common::String entityName = "Entity";
        const SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

        auto [CreatedEntity] = AWAIT(realtimeEngine.get(), CreateEntity, entityName, objectTransform, csp::common::Optional<uint64_t> {});

        // New entity should default to unlocked
        EXPECT_FALSE(CreatedEntity->IsLocked());

        // Test entity locks correctly
        {
            bool entityUpdated = false;

            CreatedEntity->SetUpdateCallback(
                [&entityUpdated, CreatedEntity](
                    SpaceEntity* /*Entity*/, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_LOCK_TYPE)
                    {
                        entityUpdated = true;
                    }
                });

            // Lock Entity
            CreatedEntity->Lock();

            // Entity shouldn't be locked until we apply our patch
            if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
            {
                // Entity shouldn't be locked until we apply our patch
                EXPECT_FALSE(CreatedEntity->IsLocked());
            }
            else
            {
                // Entity should be locked immediately
                EXPECT_TRUE(CreatedEntity->IsLocked());
            }

            // Apply patch
            CreatedEntity->QueueUpdate();
            ProcessPendingIfOnline(*realtimeEngine);

            WaitForCallbackWithUpdate(entityUpdated, realtimeEngine.get());
            EXPECT_TRUE(entityUpdated);

            // Entity should be locked now
            EXPECT_TRUE(CreatedEntity->IsLocked());
        }

        // Test entity unlocks correctly
        {
            bool entityUpdated = false;

            CreatedEntity->SetUpdateCallback(
                [&entityUpdated, CreatedEntity](
                    SpaceEntity* /*Entity*/, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_LOCK_TYPE)
                    {
                        entityUpdated = true;
                    }
                });

            // Unlock Entity
            CreatedEntity->Unlock();

            if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
            {
                // Entity should still be locked until we apply our patch
                EXPECT_TRUE(CreatedEntity->IsLocked());
            }
            else
            {
                // Entity should be unlocked immediately
                EXPECT_FALSE(CreatedEntity->IsLocked());
            }

            // Apply patch
            CreatedEntity->QueueUpdate();
            ProcessPendingIfOnline(*realtimeEngine);

            WaitForCallbackWithUpdate(entityUpdated, realtimeEngine.get());
            EXPECT_TRUE(entityUpdated);

            // Entity shouldn't be locked now
            EXPECT_FALSE(CreatedEntity->IsLocked());
        }
    }

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ParentDeletion, ParentDeletionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    csp::common::RealtimeEngineType realtimeEngineType = std::get<0>(GetParam());
    const bool local = std::get<1>(GetParam());

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    const csp::systems::Profile testUser = CreateTestUser();
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };

    bool entitiesCreated = false;

    auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        // If local is false, test DeserialiseFromPatch functionality
        auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, !local);
    }

    // Create Entities
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName1 = "ChildEntity1";
    csp::common::String childEntityName2 = "ChildEntity2";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedParentEntity] = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity1] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName1, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity2] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName2, objectTransform, csp::common::Optional<uint64_t> {});

    // Test setting the parent for the first child
    {
        bool childEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&childEntityUpdated, childEntityName1](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (entity->GetName() == childEntityName1 && (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT))
                {
                    childEntityUpdated = true;
                }
            });

        CreatedChildEntity1->SetParentId(CreatedParentEntity->GetId());

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {

            // Parents shouldn't be set until after replication
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
            EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 3);

            CreatedChildEntity1->QueueUpdate();
            WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
            EXPECT_TRUE(childEntityUpdated);
        }
        else
        {
            // Parents should be set immediately
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
            EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);
        }
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

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            CreatedChildEntity2->QueueUpdate();
            WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
            EXPECT_TRUE(childEntityUpdated);
        }
    }

    // Delete the parent
    {
        bool localDestroyCalled = false;
        bool entityDestroyCalled = false;
        bool childEntityUpdated = false;
        bool childEntityUpdated2 = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&childEntityUpdated, &localDestroyCalled, &entityDestroyCalled, childEntityName1](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (childEntityUpdated)
                {
                    // Prevent from being called twice when AllowSelfMessaging is on
                    return;
                }

                if (entity->GetName() == childEntityName1 && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    childEntityUpdated = true;
                    // Ensure this is called before both destroy callbacks
                    EXPECT_FALSE(localDestroyCalled);
                    EXPECT_FALSE(entityDestroyCalled);
                }
            });

        CreatedChildEntity2->SetUpdateCallback(
            [&childEntityUpdated2, &localDestroyCalled, &entityDestroyCalled, childEntityName2](
                SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (childEntityUpdated2)
                {
                    // Prevent from being called twice when AllowSelfMessaging is on
                    return;
                }

                if (entity->GetName() == childEntityName2 && flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                {
                    childEntityUpdated2 = true;
                    // Ensure this is called before both destroy callbacks
                    EXPECT_FALSE(localDestroyCalled);
                    EXPECT_FALSE(entityDestroyCalled);
                }
            });

        CreatedParentEntity->SetDestroyCallback(
            [&entityDestroyCalled](bool success)
            {
                entityDestroyCalled = true;
                EXPECT_TRUE(success);
            });

        realtimeEngine->DestroyEntity(CreatedParentEntity,
            [&localDestroyCalled](bool success)
            {
                localDestroyCalled = true;
                EXPECT_TRUE(success);
            });

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            WaitForCallbackWithUpdate(localDestroyCalled, realtimeEngine.get());
            WaitForCallbackWithUpdate(entityDestroyCalled, realtimeEngine.get());
            WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
            WaitForCallbackWithUpdate(childEntityUpdated2, realtimeEngine.get());
        }

        EXPECT_TRUE(localDestroyCalled);
        EXPECT_TRUE(entityDestroyCalled);
        EXPECT_TRUE(childEntityUpdated);
        EXPECT_TRUE(childEntityUpdated2);

        // Check children are unparented correctly
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(realtimeEngine->GetNumEntities(), 2);
        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    // Re-enter space to ensure updates were made to the server
    {
        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        // Log out
        LogOut(userSystem);

        // Log in again
        LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

        // Enter space
        entitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());
        }

        EXPECT_TRUE(entitiesCreated);
    }

    // Ensure children have been unparented and are now root entities
    {
        auto retrievedChildEntity1 = realtimeEngine->FindSpaceEntity(childEntityName1);
        auto retrievedChildEntity2 = realtimeEngine->FindSpaceEntity(childEntityName2);

        EXPECT_EQ(retrievedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(retrievedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(realtimeEngine->GetNumEntities(), 2);
        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    realtimeEngine.reset();

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ParentChildDeletion, ParentChildDeletionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    csp::common::RealtimeEngineType realtimeEngineType = std::get<0>(GetParam());
    const bool local = std::get<1>(GetParam());

    // Log in
    bool useMultiplayerConnection = false;
    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        useMultiplayerConnection = true;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, useMultiplayerConnection);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(realtimeEngineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        // If local is false, test DeserialiseFromPatch functionality
        auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, !local);
    }

    // Create Entities
    csp::common::String parentEntityName = "ParentEntity";
    csp::common::String childEntityName1 = "ChildEntity1";
    csp::common::String childEntityName2 = "ChildEntity2";
    SpaceTransform objectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedParentEntity] = AWAIT(realtimeEngine.get(), CreateEntity, parentEntityName, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity1] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName1, objectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity2] = AWAIT(realtimeEngine.get(), CreateEntity, childEntityName2, objectTransform, csp::common::Optional<uint64_t> {});

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

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            // Parents shouldn't be set until after replication
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
            EXPECT_EQ(realtimeEngine->GetNumEntities(), 3);
            EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 3);

            CreatedChildEntity1->QueueUpdate();
            WaitForCallbackWithUpdate(childEntityUpdated, realtimeEngine.get());
            EXPECT_TRUE(childEntityUpdated);
        }
        else
        {
            // Parents should be set immediately
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
        }

        EXPECT_EQ(realtimeEngine->GetNumEntities(), 3);
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

        EXPECT_EQ(realtimeEngine->GetNumEntities(), 3);
        EXPECT_EQ(realtimeEngine->GetRootHierarchyEntities()->Size(), 1);
    }

    // Delete the first child
    {
        bool destroyCalled = false;

        auto destroyCb = [&destroyCalled](bool success)
        {
            destroyCalled = true;
            EXPECT_TRUE(success);
        };

        realtimeEngine->DestroyEntity(CreatedChildEntity1, destroyCb);

        WaitForCallbackWithUpdate(destroyCalled, realtimeEngine.get());
        EXPECT_TRUE(destroyCalled);

        // Check entity is  unparented correctly
        EXPECT_EQ(realtimeEngine->GetNumEntities(), 2);

        EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), CreatedParentEntity);

        EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
        EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0], CreatedChildEntity2);

        EXPECT_EQ(CreatedChildEntity2->GetChildEntities()->Size(), 0);
    }

    // Delete the parent
    {
        bool destroyCalled = false;

        auto destroyCb = [&destroyCalled](bool success)
        {
            destroyCalled = true;
            EXPECT_TRUE(success);
        };

        realtimeEngine->DestroyEntity(CreatedParentEntity, destroyCb);

        WaitForCallbackWithUpdate(destroyCalled, realtimeEngine.get());
        EXPECT_TRUE(destroyCalled);

        // Ensure parent is deleted and child is re-parented
        EXPECT_EQ(realtimeEngine->GetNumEntities(), 1);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        if (realtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            if (!local)
            {
                auto [FlagSetResult2] = AWAIT(connection, SetAllowSelfMessagingFlag, false);
            }
        }

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        realtimeEngine.reset();

        // Delete space
        DeleteSpace(spaceSystem, space.Id);

        // Log out
        LogOut(userSystem);
    }
}

INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, EntityLock,
    testing::Values(std::make_tuple(csp::common::RealtimeEngineType::Offline, false), std::make_tuple(csp::common::RealtimeEngineType::Online, true),
        std::make_tuple(csp::common::RealtimeEngineType::Online, false)));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, ParentDeletion,
    testing::Values(std::make_tuple(csp::common::RealtimeEngineType::Offline, false), std::make_tuple(csp::common::RealtimeEngineType::Online, true),
        std::make_tuple(csp::common::RealtimeEngineType::Online, false)));
INSTANTIATE_TEST_SUITE_P(RealtimeEngineEntityTests, ParentChildDeletion,
    testing::Values(std::make_tuple(csp::common::RealtimeEngineType::Offline, false), std::make_tuple(csp::common::RealtimeEngineType::Online, true),
        std::make_tuple(csp::common::RealtimeEngineType::Online, false)));
}