/*
 * Copyright 2024 Magnopus LLC

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
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Multiplayer/NetworkEventManagerImpl.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
#include "PublicAPITests/SpaceSystemTestHelpers.h"
#include "PublicAPITests/UserSystemTestHelpers.h"
#include "gtest/gtest-param-test.h"
#include "gtest/gtest.h"
#include <atomic>

using namespace csp::multiplayer;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateAvatarForLeaderElection(csp::common::IRealtimeEngine* entitySystem)
{
    const csp::common::String& userName = "Player 1";
    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String& userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState();

    auto [Avatar] = AWAIT(entitySystem, CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cout << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        EXPECT_EQ(Avatar->GetComponents()->Size(), 1);

        auto* avatarComponent = Avatar->GetComponent(0);

        EXPECT_EQ(avatarComponent->GetComponentType(), ComponentType::AvatarData);

        std::cout << "OnUserCreated" << std::endl;
    }
}

} // namespace

namespace CSPEngine
{

class UpdateSpaceEntityGlobalPosition : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class UpdateSpaceEntityGlobalRotation : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class UpdateSpaceEntityGlobalScale : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class UpdateSpaceEntityParentId : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class RemoveSpaceEntityParent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class GetRootHierarchyEntities : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

TEST_P(UpdateSpaceEntityGlobalPosition, UpdateSpaceEntityGlobalPositionTest)
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

    std::atomic_bool scriptLeaderReady = false;

    auto scriptLeaderReadyCallback = [&scriptLeaderReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(realtimeEngine.get());

    const std::string scriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.position = parent.globalPosition;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptLeaderIsReady = [&scriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (scriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        EXPECT_EQ(ResponseWaiter::WaitFor(scriptLeaderIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3(1, 1, 1), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String childObjectName = "Child Object 1";
        SpaceTransform childObjectTransform = { csp::common::Vector3(10, 10, 10), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, childObjectName, childObjectTransform);

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        EXPECT_EQ(Object->GetGlobalPosition(), csp::common::Vector3(1, 1, 1));
        EXPECT_EQ(ChildObject->GetGlobalPosition(), csp::common::Vector3(11, 11, 11));

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool entityUpdated = false;

        // Need to set this before tick or it won't always get called.
        ChildObject->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (entity->GetName() == "Child Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
                    {
                        std::cout << "Position Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        const bool scriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        // Create callback to process pending entity operations
        auto entityUpdatedIsReady = [&entityUpdated, &realtimeEngine]()
        {
            ProcessPendingIfOnline(*realtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (entityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(entityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetGlobalPosition(), csp::common::Vector3::One());
        EXPECT_EQ(ChildObject->GetGlobalPosition(), csp::common::Vector3(2, 2, 2));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(UpdateSpaceEntityGlobalRotation, UpdateSpaceEntityGlobalRotationTest)
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

    std::atomic_bool scriptLeaderReady = false;

    auto scriptLeaderReadyCallback = [&scriptLeaderReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(realtimeEngine.get());

    const std::string scriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.rotation = parent.globalRotation;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (scriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4(1, 1, 1, 1), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String childObjectName = "Child Object 1";
        SpaceTransform childObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4(10, 10, 10, 10), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, childObjectName, childObjectTransform);

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        EXPECT_EQ(Object->GetGlobalRotation(), csp::common::Vector4::One());
        EXPECT_EQ(ChildObject->GetGlobalRotation(), csp::common::Vector4(20, 20, 20, -20));

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool entityUpdated = false;
        ChildObject->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (entity->GetName() == "Child Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_ROTATION)
                    {
                        std::cout << "Rotation Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto entityUpdatedIsReady = [&entityUpdated, &realtimeEngine]()
        {
            ProcessPendingIfOnline(*realtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (entityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        const bool scriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(entityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetGlobalRotation(), csp::common::Vector4::One());
        EXPECT_EQ(ChildObject->GetGlobalRotation(), csp::common::Vector4(2, 2, 2, -2));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(UpdateSpaceEntityGlobalScale, UpdateSpaceEntityGlobalScaleTest)
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

    std::atomic_bool scriptLeaderReady = false;

    auto scriptLeaderReadyCallback = [&scriptLeaderReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(realtimeEngine.get());

    const std::string scriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.scale = parent.globalScale;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (scriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3(2, 2, 2) };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String childObjectName = "Child Object 1";
        SpaceTransform childObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3(10, 10, 10) };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, childObjectName, childObjectTransform);

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        EXPECT_EQ(Object->GetGlobalScale(), csp::common::Vector3(2, 2, 2));
        EXPECT_EQ(ChildObject->GetGlobalScale(), csp::common::Vector3(20, 20, 20));

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool entityUpdated = false;

        ChildObject->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (entity->GetName() == "Child Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_SCALE)
                    {
                        std::cout << "Scale Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto entityUpdatedIsReady = [&entityUpdated, &realtimeEngine]()
        {
            ProcessPendingIfOnline(*realtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (entityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        const bool scriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(entityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetGlobalScale(), csp::common::Vector3(2, 2, 2));
        EXPECT_EQ(ChildObject->GetGlobalScale(), csp::common::Vector3(4, 4, 4));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
TEST_P(UpdateSpaceEntityParentId, UpdateSpaceEntityParentIdTest)
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

    std::atomic_bool scriptLeaderReady = false;

    auto scriptLeaderReadyCallback = [&scriptLeaderReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(realtimeEngine.get());

    const std::string scriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];

			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.parentId = parent.id;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (scriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String childObjectName = "Child Object 1";
        SpaceTransform childObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(realtimeEngine.get(), CreateEntity, childObjectName, childObjectTransform, csp::common::Optional<uint64_t> {});

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        ChildObject->QueueUpdate();
        Object->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        EXPECT_NE(Object, ChildObject->GetParentEntity());

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool entityUpdated = false;

        ChildObject->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (entity->GetName() == "Child Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                    {
                        std::cout << "Parent Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto entityUpdatedIsReady = [&entityUpdated, &realtimeEngine]()
        {
            ProcessPendingIfOnline(*realtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (entityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        ChildObject->QueueUpdate();
        Object->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        const bool scriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(entityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object, ChildObject->GetParentEntity());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(RemoveSpaceEntityParent, RemoveSpaceEntityParentTest)
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

    std::atomic_bool scriptLeaderReady = false;

    auto scriptLeaderReadyCallback = [&scriptLeaderReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(realtimeEngine.get());

    const std::string scriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];

			entity.removeParentEntity();
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (scriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3(1, 1, 1), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String childObjectName = "Child Object 1";
        SpaceTransform childObjectTransform = { csp::common::Vector3(10, 10, 10), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, childObjectName, childObjectTransform);

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        EXPECT_EQ(Object, ChildObject->GetParentEntity());

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool entityUpdated = false;

        ChildObject->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (entity->GetName() == "Child Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                    {
                        std::cout << "Parent Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto entityUpdatedIsReady = [&entityUpdated, &realtimeEngine]()
        {
            ProcessPendingIfOnline(*realtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (entityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        const bool scriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(entityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_NE(Object, ChildObject->GetParentEntity());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(GetRootHierarchyEntities, GetRootHierarchyEntitiesTest)
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

    std::atomic_bool scriptLeaderReady = false;

    auto scriptLeaderReadyCallback = [&scriptLeaderReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(realtimeEngine.get());

    const std::string scriptText = R"xx(
		
		globalThis.onTick = () => {
			var entities = TheEntitySystem.getRootHierarchyEntities();

			for (let i = 0; i < entities.length; i++)
			{
				entities[i].position = [1, 1, 1];
			}
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (scriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String childObjectName = "Child Object 1";
        SpaceTransform childObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, childObjectName, childObjectTransform);

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        EXPECT_EQ(Object->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(ChildObject->GetPosition(), csp::common::Vector3::Zero());

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool entityUpdated = false;

        Object->SetUpdateCallback(
            [&entityUpdated](SpaceEntity* entity, SpaceEntityUpdateFlags flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                auto namer = entity->GetName();
                if (entity->GetName() == "Object 1")
                {
                    if (flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
                    {
                        std::cout << "Position Updated" << std::endl;
                        entityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto entityUpdatedIsReady = [&entityUpdated, &realtimeEngine]()
        {
            ProcessPendingIfOnline(*realtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (entityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        const bool scriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(entityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetPosition(), csp::common::Vector3::One());
        EXPECT_EQ(ChildObject->GetPosition(), csp::common::Vector3::Zero());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

INSTANTIATE_TEST_SUITE_P(SpaceEntityTests, UpdateSpaceEntityGlobalPosition,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

INSTANTIATE_TEST_SUITE_P(SpaceEntityTests, UpdateSpaceEntityGlobalRotation,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

INSTANTIATE_TEST_SUITE_P(SpaceEntityTests, UpdateSpaceEntityGlobalScale,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

INSTANTIATE_TEST_SUITE_P(
    SpaceEntityTests, UpdateSpaceEntityParentId, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

INSTANTIATE_TEST_SUITE_P(
    SpaceEntityTests, RemoveSpaceEntityParent, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

INSTANTIATE_TEST_SUITE_P(
    SpaceEntityTests, GetRootHierarchyEntities, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));

}
CSP_PUBLIC_TEST(CSPEngine, LockPrerequisites, LockPrerequisitesTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::ScriptSystem& scriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();
    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    SpaceEntity entity { nullptr, scriptSystem, logSystem };

    // Ensure the lock error message is called when we try and lock an entity that is already locked
    const csp::common::String lockErrorMsg = "Entity is already locked.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, lockErrorMsg)).Times(1);

    // Set the entity as locked first
    entity.Lock();
    // Check that we error if we try to lock again
    entity.Lock();
}
CSP_PUBLIC_TEST(CSPEngine, UnlockPrerequisites, UnlockPrerequisitesTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::ScriptSystem& scriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();
    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    SpaceEntity entity { nullptr, scriptSystem, logSystem };

    // Ensure the unlock error message is called when we try and unlock an entity that is already unlocked
    const csp::common::String unlockErrorMsg = "Entity is not currently locked.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, unlockErrorMsg)).Times(1);

    entity.Unlock();
}