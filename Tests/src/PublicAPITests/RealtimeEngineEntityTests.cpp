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

TEST_P(CreateAvatar, CreateAvatarTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;
    LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
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
    EXPECT_EQ(AvatarComponent->GetIsVisible(), IsVisible);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(CreateCreatorAvatar, CreateCreatorAvatarTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    const csp::common::String& UserName = "Creator 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolCreatorAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Creator;
    LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
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
    EXPECT_EQ(AvatarComponent->GetIsVisible(), IsVisible);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(AvatarMovementDirection, AvatarMovementDirectionTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
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

TEST_P(ObjectCreate, ObjectCreateTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

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

TEST_P(ObjectAddComponent, ObjectAddComponentTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    bool PatchPending = true;
    Object->SetPatchSentCallback([&PatchPending](bool /*ok*/) { PatchPending = false; });

    const csp::common::String ModelAssetId = "NotARealId";

    auto* StaticModelComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    auto StaticModelComponentKey = StaticModelComponent->GetId();
    StaticModelComponent->SetExternalResourceAssetId(ModelAssetId);

    if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();
        while (PatchPending)
        {
            RealtimeEngine->ProcessPendingEntityOperations();
            std::this_thread::sleep_for(10ms);
        }
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

    if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();

        while (PatchPending)
        {
            RealtimeEngine->ProcessPendingEntityOperations();
            std::this_thread::sleep_for(10ms);
        }
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

TEST_P(ObjectRemoveComponent, ObjectRemoveComponentTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    bool PatchPending = true;
    Object->SetPatchSentCallback([&PatchPending](bool /*ok*/) { PatchPending = false; });

    const csp::common::String ModelAssetId = "NotARealId";

    auto* StaticModelComponent = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
    auto StaticModelComponentKey = StaticModelComponent->GetId();
    StaticModelComponent->SetExternalResourceAssetId(ModelAssetId);
    auto* ImageComponent = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
    auto ImageComponentKey = ImageComponent->GetId();
    ImageComponent->SetImageAssetId("TestID");
    if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();

        while (PatchPending)
        {
            RealtimeEngine->ProcessPendingEntityOperations();
            std::this_thread::sleep_for(10ms);
        }
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

    if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        Object->QueueUpdate();

        while (PatchPending)
        {
            RealtimeEngine->ProcessPendingEntityOperations();
            std::this_thread::sleep_for(10ms);
        }
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

TEST_P(ObjectRemoveComponentTestReenterSpace, ObjectRemoveComponentTestReenterSpace)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::String UserId;
    const csp::common::String ObjectName = "Object 1";

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    bool EntitiesCreated = false;
    auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

    uint16_t KeepKey = 0;
    uint16_t DeleteKey = 0;

    {
        // Enter space
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());

        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        bool PatchPending = true;
        Object->SetPatchSentCallback([&PatchPending](bool /*ok*/) { PatchPending = false; });

        auto* ComponentToKeep = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
        ComponentToKeep->SetComponentName("ComponentNameKeep");
        KeepKey = ComponentToKeep->GetId();
        auto* ComponentToDelete = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
        ComponentToDelete->SetComponentName("ComponentNameDelete");
        DeleteKey = ComponentToDelete->GetId();

        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            Object->QueueUpdate();

            while (PatchPending)
            {
                RealtimeEngine->ProcessPendingEntityOperations();
                std::this_thread::sleep_for(10ms);
            }
        }

        PatchPending = true;

        // Ensure values are set correctly
        EXPECT_EQ(ComponentToKeep->GetComponentName(), "ComponentNameKeep");
        EXPECT_EQ(ComponentToDelete->GetComponentName(), "ComponentNameDelete");

        auto& Components = *Object->GetComponents();

        EXPECT_EQ(Components.Size(), 2);
        EXPECT_TRUE(Components.HasKey(KeepKey));
        EXPECT_TRUE(Components.HasKey(DeleteKey));

        // Delete component
        Object->RemoveComponent(ComponentToDelete->GetId());
        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            Object->QueueUpdate();
            while (PatchPending)
            {
                RealtimeEngine->ProcessPendingEntityOperations();
                std::this_thread::sleep_for(10ms);
            }
            EXPECT_FALSE(PatchPending);
        }

        // Check deletion has happened
        auto& RealComponents = *Object->GetComponents();

        EXPECT_EQ(RealComponents.Size(), 1);
        EXPECT_TRUE(RealComponents.HasKey(KeepKey));
        EXPECT_FALSE(RealComponents.HasKey(DeleteKey));

        // Exit space and enter again, making sure the entities have been created
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Wait a few seconds for the CHS database to update
        std::this_thread::sleep_for(std::chrono::seconds(8));
    }
    {
        EntitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
        EXPECT_TRUE(EntitiesCreated);

        // Retrieve components in space
        SpaceEntity* FoundEntity = RealtimeEngine->FindSpaceObject(ObjectName);
        EXPECT_TRUE(FoundEntity != nullptr);
        auto& FoundComponents = *FoundEntity->GetComponents();

        // Check the right component has been deleted
        EXPECT_EQ(FoundComponents.Size(), 1);
        EXPECT_TRUE(FoundComponents.HasKey(KeepKey));
        EXPECT_FALSE(FoundComponents.HasKey(DeleteKey));
        EXPECT_EQ(FoundEntity->GetComponent(0)->GetComponentName(), "ComponentNameKeep");

        // Exit space
        auto [ExitSpaceResult2] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(DeleteMultipleEntities, DeleteMultipleEntitiesTest)
{
    // Test for OB-1046
    // If the rate limiter hasn't processed all PendingOutgoingUpdates after SpaceEntity deletion it will crash when trying to process them

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create 3 seperate objects to ensure there is too many updates for the rate limiter to process in one tick

    // Create object
    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});
    CreatedObject->AddComponent(ComponentType::Image);
    CreatedObject->QueueUpdate();

    // Create object 2
    auto [CreatedObject2] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});
    CreatedObject2->AddComponent(ComponentType::Image);
    CreatedObject2->QueueUpdate();

    // Create object 3
    auto [CreatedObject3] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});
    CreatedObject3->AddComponent(ComponentType::Image);
    CreatedObject3->QueueUpdate();

    // Destroy Entites
    RealtimeEngine->DestroyEntity(CreatedObject, [](bool) {});
    RealtimeEngine->DestroyEntity(CreatedObject2, [](bool) {});
    RealtimeEngine->DestroyEntity(CreatedObject3, [](bool) {});

    csp::CSPFoundation::Tick();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntitySelection, EntitySelectionTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

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

TEST_P(InvalidComponentFields, InvalidComponentFieldsTest)
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

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String ModelAssetId = "NotARealId";

    Object->AddComponent(ComponentType::Invalid);

    // Process component creation
    Object->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntityGlobalPosition, EntityGlobalPositionTest)
{
    // Tests the OnlineRealtimeEngine::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
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

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity]
        = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName, ObjectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        RealtimeEngine->ProcessPendingEntityOperations();
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

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& /*Result*/) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntityGlobalRotation, EntityGlobalRotationTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    // Parent has a position [0,0,0], and 1.507 radian (90 degree) rotation around the y axis
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformExpected
        = { csp::common::Vector3 { 0, 0, 1 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 1, 1, 1 } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity]
        = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName, ObjectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        RealtimeEngine->ProcessPendingEntityOperations();
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

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& /*Result*/) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntityGlobalScale, EntityGlobalScaleTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";

    // Create a parent, positioned at the origin, rotated 90 degrees, with a scale of -0.5 on x axis and 0.5 on Y/Z axes
    // child created at a position of [1,0,0], no rotation, and scale of 1
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { -0.5f, 0.5f, 0.5f } };
    SpaceTransform ObjectTransformChild = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 1, 1, 1 } };

    SpaceTransform ObjectTransformExpected = { csp::common::Vector3 { 0, 0, -0.5f }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f },
        csp::common::Vector3 { -0.5f, 0.5f, 0.5f } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity]
        = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName, ObjectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        RealtimeEngine->ProcessPendingEntityOperations();
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

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& /*Result*/) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntityGlobalTransform, EntityGlobalTransformTest)
{
    // Tests the OnlineRealtimeEngineWeak::OnAllEntitiesCreated
    // for ParentId and ChildEntities
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities for testing heirarchy transforms
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";
    SpaceTransform ObjectTransformParent
        = { csp::common::Vector3 { 0, 0, 0 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 1, 1, 1 } };
    SpaceTransform ObjectTransformChild
        = { csp::common::Vector3 { 1, 0, 0 }, csp::common::Vector4 { 0, 0, 0, 1 }, csp::common::Vector3 { 0.5f, 0.5f, 0.5f } };
    SpaceTransform ObjectTransformExpected
        = { csp::common::Vector3 { 0, 0, 1 }, csp::common::Vector4 { 0, -0.7071081f, 0, 0.7071055f }, csp::common::Vector3 { 0.5f, 0.5f, 0.5f } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity]
        = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransformParent, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName, ObjectTransformChild, csp::common::Optional<uint64_t> {});

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
    while (!ChildEntityUpdated && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        RealtimeEngine->ProcessPendingEntityOperations();
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

    SpaceSystem->ExitSpace([](const csp::systems::NullResult& /*Result*/) {});

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(CreateObjectParent, CreateObjectParentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create Entities
    csp::common::String ParentEntityName = "ParentEntity";
    csp::common::String ChildEntityName = "ChildEntity";

    SpaceTransform ObjectTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };

    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

    auto [CreatedParentEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity] = AWAIT(CreatedParentEntity, CreateChildEntity, ChildEntityName, ObjectTransform);

    EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
    EXPECT_EQ(CreatedChildEntity->GetParentEntity(), CreatedParentEntity);

    EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntityLockAddComponent, EntityLockAddComponentTest)
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

    // Enter a space and lock an entity
    {
        csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
        std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        // Enter space
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create Entity
        csp::multiplayer::SpaceEntity* CreatedEntity = CreateTestObject(RealtimeEngine.get());

        // Lock Entity
        CreatedEntity->Lock();

        // Apply patch
        CreatedEntity->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        // Entity should be locked now
        EXPECT_TRUE(CreatedEntity->IsLocked());

        {
            // Ensure the add component error message is logged when we try to add a component to a locked entity.
            static const csp::common::String AddComponentErrorMsg = "Entity is locked. New components can not be added to a locked Entity.";

            RAIIMockLogger MockLogger {};
            EXPECT_CALL(MockLogger.MockLogCallback, Call(AddComponentErrorMsg)).Times(1);

            // Attempt to add a component to a locked entity
            auto NewComponent = CreatedEntity->AddComponent(ComponentType::StaticModel);

            EXPECT_EQ(NewComponent, nullptr);
        }

        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(EntityLockRemoveComponent, EntityLockRemoveComponentTest)
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

    // Enter a space and lock an entity
    {
        csp::common::RealtimeEngineType RealtimeEngineType = GetParam();
        std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        // Enter space
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create Entity
        csp::multiplayer::SpaceEntity* CreatedEntity = CreateTestObject(RealtimeEngine.get());

        // Add a component to the entity
        auto NewComponent = CreatedEntity->AddComponent(ComponentType::StaticModel);
        EXPECT_NE(NewComponent, nullptr);

        // Lock Entity
        CreatedEntity->Lock();

        // Apply patch
        CreatedEntity->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        // Entity should be locked now
        EXPECT_TRUE(CreatedEntity->IsLocked());

        {
            // Ensure the remove component error message is logged when we try to remove a component from a locked entity.
            static const csp::common::String RemoveComponentErrorMsg = "Entity is locked. Components can not be removed from a locked Entity.";

            RAIIMockLogger MockLogger {};
            EXPECT_CALL(MockLogger.MockLogCallback, Call(RemoveComponentErrorMsg)).Times(1);

            // Attempt to remove a component from a locked entity
            CreatedEntity->RemoveComponent(NewComponent->GetId());
        }

        // Exit Space
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
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

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    csp::common::RealtimeEngineType RealtimeEngineType = std::get<0>(GetParam());
    const bool Local = std::get<1>(GetParam());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
    {
        // Ensure patch rate limiting is off, as we're sending patches in quick succession.
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetEntityPatchRateLimitEnabled(false);
    }

    // If local is false, test DeserialiseFromPatch functionality
    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, !Local);
    EXPECT_EQ(FlagSetResult, csp::multiplayer::ErrorCode::None);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    {
        // Create Entity
        const csp::common::String EntityName = "Entity";
        const SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

        auto [CreatedEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, EntityName, ObjectTransform, csp::common::Optional<uint64_t> {});

        // New entity should default to unlocked
        EXPECT_FALSE(CreatedEntity->IsLocked());

        // Test entity locks correctly
        {
            bool EntityUpdated = false;

            CreatedEntity->SetUpdateCallback(
                [&EntityUpdated, CreatedEntity](
                    SpaceEntity* /*Entity*/, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_LOCK_TYPE)
                    {
                        EntityUpdated = true;
                    }
                });

            // Lock Entity
            CreatedEntity->Lock();

            // Entity shouldn't be locked until we apply our patch
            if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
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
            RealtimeEngine->ProcessPendingEntityOperations();

            WaitForCallbackWithUpdate(EntityUpdated, RealtimeEngine.get());
            EXPECT_TRUE(EntityUpdated);

            // Entity should be locked now
            EXPECT_TRUE(CreatedEntity->IsLocked());
        }

        // Test entity unlocks correctly
        {
            bool EntityUpdated = false;

            CreatedEntity->SetUpdateCallback(
                [&EntityUpdated, CreatedEntity](
                    SpaceEntity* /*Entity*/, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_LOCK_TYPE)
                    {
                        EntityUpdated = true;
                    }
                });

            // Unlock Entity
            CreatedEntity->Unlock();

            if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
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
            RealtimeEngine->ProcessPendingEntityOperations();

            WaitForCallbackWithUpdate(EntityUpdated, RealtimeEngine.get());
            EXPECT_TRUE(EntityUpdated);

            // Entity shouldn't be locked now
            EXPECT_FALSE(CreatedEntity->IsLocked());
        }
    }

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(ParentDeletion, ParentDeletionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    csp::common::RealtimeEngineType RealtimeEngineType = std::get<0>(GetParam());
    const bool Local = std::get<1>(GetParam());

    // Log in
    csp::common::String UserId;
    csp::systems::Profile TestUser = CreateTestUser();
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };

    bool EntitiesCreated = false;

    auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

    RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);
    RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

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

    auto [CreatedParentEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ParentEntityName, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity1] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName1, ObjectTransform, csp::common::Optional<uint64_t> {});
    auto [CreatedChildEntity2] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildEntityName2, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Test setting the parent for the first child
    {
        bool ChildEntityUpdated = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
            {
                if (Entity->GetName() == ChildEntityName1 && (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT))
                {
                    ChildEntityUpdated = true;
                }
            });

        CreatedChildEntity1->SetParentId(CreatedParentEntity->GetId());

        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {

            // Parents shouldn't be set until after replication
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
            EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 3);

            CreatedChildEntity1->QueueUpdate();
            WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
            EXPECT_TRUE(ChildEntityUpdated);
        }
        else
        {
            // Parents should be set immediately
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
            EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);
        }
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

        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            CreatedChildEntity2->QueueUpdate();
            WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
            EXPECT_TRUE(ChildEntityUpdated);
        }
    }

    // Delete the parent
    {
        bool LocalDestroyCalled = false;
        bool EntityDestroyCalled = false;
        bool ChildEntityUpdated = false;
        bool ChildEntityUpdated2 = false;

        CreatedChildEntity1->SetUpdateCallback(
            [&ChildEntityUpdated, &LocalDestroyCalled, &EntityDestroyCalled, ChildEntityName1](
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
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
                SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>& /*UpdateInfo*/)
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

        RealtimeEngine->DestroyEntity(CreatedParentEntity,
            [&LocalDestroyCalled](bool Success)
            {
                LocalDestroyCalled = true;
                EXPECT_TRUE(Success);
            });

        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            WaitForCallbackWithUpdate(LocalDestroyCalled, RealtimeEngine.get());
            WaitForCallbackWithUpdate(EntityDestroyCalled, RealtimeEngine.get());
            WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
            WaitForCallbackWithUpdate(ChildEntityUpdated2, RealtimeEngine.get());
        }

        EXPECT_TRUE(LocalDestroyCalled);
        EXPECT_TRUE(EntityDestroyCalled);
        EXPECT_TRUE(ChildEntityUpdated);
        EXPECT_TRUE(ChildEntityUpdated2);

        // Check children are unparented correctly
        EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(RealtimeEngine->GetNumEntities(), 2);
        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);
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
        EntitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
        }

        EXPECT_TRUE(EntitiesCreated);
    }

    // Ensure children have been unparented and are now root entities
    {
        auto RetrievedChildEntity1 = RealtimeEngine->FindSpaceEntity(ChildEntityName1);
        auto RetrievedChildEntity2 = RealtimeEngine->FindSpaceEntity(ChildEntityName2);

        EXPECT_EQ(RetrievedChildEntity1->GetParentEntity(), nullptr);
        EXPECT_EQ(RetrievedChildEntity2->GetParentEntity(), nullptr);

        EXPECT_EQ(RealtimeEngine->GetNumEntities(), 2);
        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 2);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(ParentChildDeletion, ParentChildDeletionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    csp::common::RealtimeEngineType RealtimeEngineType = std::get<0>(GetParam());
    const bool Local = std::get<1>(GetParam());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(RealtimeEngineType) };
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

        if (RealtimeEngineType == csp::common::RealtimeEngineType::Online)
        {
            // Parents shouldn't be set until after replication
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
            EXPECT_EQ(RealtimeEngine->GetNumEntities(), 3);
            EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 3);

            CreatedChildEntity1->QueueUpdate();
            WaitForCallbackWithUpdate(ChildEntityUpdated, RealtimeEngine.get());
            EXPECT_TRUE(ChildEntityUpdated);
        }
        else
        {
            // Parents should be set immediately
            EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
            EXPECT_EQ(CreatedChildEntity1->GetParentEntity(), CreatedParentEntity);
            EXPECT_EQ(CreatedChildEntity2->GetParentEntity(), nullptr);
        }

        EXPECT_EQ(RealtimeEngine->GetNumEntities(), 3);
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

        EXPECT_EQ(RealtimeEngine->GetNumEntities(), 3);
        EXPECT_EQ(RealtimeEngine->GetRootHierarchyEntities()->Size(), 1);
    }

    // Delete the first child
    {
        bool DestroyCalled = false;

        auto DestroyCb = [&DestroyCalled](bool Success)
        {
            DestroyCalled = true;
            EXPECT_TRUE(Success);
        };

        RealtimeEngine->DestroyEntity(CreatedChildEntity1, DestroyCb);

        WaitForCallbackWithUpdate(DestroyCalled, RealtimeEngine.get());
        EXPECT_TRUE(DestroyCalled);

        // Check entity is  unparented correctly
        EXPECT_EQ(RealtimeEngine->GetNumEntities(), 2);

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

        RealtimeEngine->DestroyEntity(CreatedParentEntity, DestroyCb);

        WaitForCallbackWithUpdate(DestroyCalled, RealtimeEngine.get());
        EXPECT_TRUE(DestroyCalled);

        // Ensure parent is deleted and child is re-parented
        EXPECT_EQ(RealtimeEngine->GetNumEntities(), 1);
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