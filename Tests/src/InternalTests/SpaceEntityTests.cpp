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
#include "quickjspp.hpp"

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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateAvatarForLeaderElection(csp::common::IRealtimeEngine* EntitySystem)
{
    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState();

    auto [Avatar]
        = AWAIT(EntitySystem, CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cout << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        EXPECT_EQ(Avatar->GetComponents()->Size(), 1);

        auto* AvatarComponent = Avatar->GetComponent(0);

        EXPECT_EQ(AvatarComponent->GetComponentType(), ComponentType::AvatarData);

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

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::atomic_bool ScriptLeaderReady = false;

    auto ScriptLeaderReadyCallback = [&ScriptLeaderReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(RealtimeEngine.get());

    const std::string ScriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.position = parent.globalPosition;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptLeaderIsReady = [&ScriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (ScriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        EXPECT_EQ(ResponseWaiter::WaitFor(ScriptLeaderIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3(1, 1, 1), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3(10, 10, 10), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        EXPECT_EQ(Object->GetGlobalPosition(), csp::common::Vector3(1, 1, 1));
        EXPECT_EQ(ChildObject->GetGlobalPosition(), csp::common::Vector3(11, 11, 11));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool EntityUpdated = false;

        // Need to set this before tick or it won't always get called.
        ChildObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (Entity->GetName() == "Child Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
                    {
                        std::cout << "Position Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        const bool ScriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &RealtimeEngine]()
        {
            ProcessPendingIfOnline(*RealtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetGlobalPosition(), csp::common::Vector3::One());
        EXPECT_EQ(ChildObject->GetGlobalPosition(), csp::common::Vector3(2, 2, 2));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(UpdateSpaceEntityGlobalRotation, UpdateSpaceEntityGlobalRotationTest)
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

    std::atomic_bool ScriptLeaderReady = false;

    auto ScriptLeaderReadyCallback = [&ScriptLeaderReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(RealtimeEngine.get());

    const std::string ScriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.rotation = parent.globalRotation;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (ScriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4(1, 1, 1, 1), csp::common::Vector3::One() };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4(10, 10, 10, 10), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        EXPECT_EQ(Object->GetGlobalRotation(), csp::common::Vector4::One());
        EXPECT_EQ(ChildObject->GetGlobalRotation(), csp::common::Vector4(20, 20, 20, -20));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool EntityUpdated = false;
        ChildObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (Entity->GetName() == "Child Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_ROTATION)
                    {
                        std::cout << "Rotation Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &RealtimeEngine]()
        {
            ProcessPendingIfOnline(*RealtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        const bool ScriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetGlobalRotation(), csp::common::Vector4::One());
        EXPECT_EQ(ChildObject->GetGlobalRotation(), csp::common::Vector4(2, 2, 2, -2));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(UpdateSpaceEntityGlobalScale, UpdateSpaceEntityGlobalScaleTest)
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

    std::atomic_bool ScriptLeaderReady = false;

    auto ScriptLeaderReadyCallback = [&ScriptLeaderReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(RealtimeEngine.get());

    const std::string ScriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.scale = parent.globalScale;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (ScriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3(2, 2, 2) };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3(10, 10, 10) };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        EXPECT_EQ(Object->GetGlobalScale(), csp::common::Vector3(2, 2, 2));
        EXPECT_EQ(ChildObject->GetGlobalScale(), csp::common::Vector3(20, 20, 20));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool EntityUpdated = false;

        ChildObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (Entity->GetName() == "Child Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_SCALE)
                    {
                        std::cout << "Scale Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &RealtimeEngine]()
        {
            ProcessPendingIfOnline(*RealtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        const bool ScriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetGlobalScale(), csp::common::Vector3(2, 2, 2));
        EXPECT_EQ(ChildObject->GetGlobalScale(), csp::common::Vector3(4, 4, 4));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
TEST_P(UpdateSpaceEntityParentId, UpdateSpaceEntityParentIdTest)
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

    std::atomic_bool ScriptLeaderReady = false;

    auto ScriptLeaderReadyCallback = [&ScriptLeaderReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(RealtimeEngine.get());

    const std::string ScriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];

			var parent = TheEntitySystem.getEntityByName("Object 1");

			entity.parentId = parent.id;
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (ScriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ChildObjectName, ChildObjectTransform, csp::common::Optional<uint64_t> {});

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        ChildObject->QueueUpdate();
        Object->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        EXPECT_NE(Object, ChildObject->GetParentEntity());

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool EntityUpdated = false;

        ChildObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (Entity->GetName() == "Child Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                    {
                        std::cout << "Parent Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &RealtimeEngine]()
        {
            ProcessPendingIfOnline(*RealtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        ChildObject->QueueUpdate();
        Object->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        const bool ScriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object, ChildObject->GetParentEntity());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(RemoveSpaceEntityParent, RemoveSpaceEntityParentTest)
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

    std::atomic_bool ScriptLeaderReady = false;

    auto ScriptLeaderReadyCallback = [&ScriptLeaderReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(RealtimeEngine.get());

    const std::string ScriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];

			entity.removeParentEntity();
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (ScriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3(1, 1, 1), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3(10, 10, 10), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        EXPECT_EQ(Object, ChildObject->GetParentEntity());

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool EntityUpdated = false;

        ChildObject->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                if (Entity->GetName() == "Child Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_PARENT)
                    {
                        std::cout << "Parent Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &RealtimeEngine]()
        {
            ProcessPendingIfOnline(*RealtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        const bool ScriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_NE(Object, ChildObject->GetParentEntity());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(GetRootHierarchyEntities, GetRootHierarchyEntitiesTest)
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

    std::atomic_bool ScriptLeaderReady = false;

    auto ScriptLeaderReadyCallback = [&ScriptLeaderReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptLeaderReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptLeaderReadyCallback);
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    CreateAvatarForLeaderElection(RealtimeEngine.get());

    const std::string ScriptText = R"xx(
		
		globalThis.onTick = () => {
			var entities = TheEntitySystem.getRootHierarchyEntities();

			for (let i = 0; i < entities.length; i++)
			{
				entities[i].position = [1, 1, 1];
			}
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptLeaderReady]()
        {
            std::cout << "Waiting for ScriptLeaderReady" << std::endl;
            return (ScriptLeaderReady == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        EXPECT_EQ(Object->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(ChildObject->GetPosition(), csp::common::Vector3::Zero());

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript().Invoke();

        bool EntityUpdated = false;

        Object->SetUpdateCallback(
            [&EntityUpdated](SpaceEntity* Entity, SpaceEntityUpdateFlags Flags, csp::common::Array<ComponentUpdateInfo>&)
            {
                auto Namer = Entity->GetName();
                if (Entity->GetName() == "Object 1")
                {
                    if (Flags & SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
                    {
                        std::cout << "Position Updated" << std::endl;
                        EntityUpdated = true;
                    }
                }
            });

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &RealtimeEngine]()
        {
            ProcessPendingIfOnline(*RealtimeEngine);

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        ProcessPendingIfOnline(*RealtimeEngine);

        const bool ScriptHasErrors = ChildObject->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        // Wait until the property have been updated and correct callback has been recieved
        ASSERT_TRUE(ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5)));

        EXPECT_EQ(Object->GetPosition(), csp::common::Vector3::One());
        EXPECT_EQ(ChildObject->GetPosition(), csp::common::Vector3::Zero());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
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
    RAIIMockLogger MockLogger {};
    csp::systems::ScriptSystem& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    SpaceEntity Entity { nullptr, ScriptSystem, LogSystem };

    // Ensure the lock error message is called when we try and lock an entity that is already locked
    const csp::common::String LockErrorMsg = "Entity is already locked.";
    const csp::common::LogLevel LogLevel = csp::common::LogLevel::Error;
    EXPECT_CALL(MockLogger.MockLogCallback, Call(LogLevel, LockErrorMsg)).Times(1);

    // Set the entity as locked first
    Entity.Lock();
    // Check that we error if we try to lock again
    Entity.Lock();
}
CSP_PUBLIC_TEST(CSPEngine, UnlockPrerequisites, UnlockPrerequisitesTest)
{
    RAIIMockLogger MockLogger {};
    csp::systems::ScriptSystem& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    SpaceEntity Entity { nullptr, ScriptSystem, LogSystem };

    // Ensure the unlock error message is called when we try and unlock an entity that is already unlocked
    const csp::common::String UnlockErrorMsg = "Entity is not currently locked.";
    const csp::common::LogLevel LogLevel = csp::common::LogLevel::Error;
    EXPECT_CALL(MockLogger.MockLogCallback, Call(LogLevel, UnlockErrorMsg)).Times(1);

    Entity.Unlock();
}