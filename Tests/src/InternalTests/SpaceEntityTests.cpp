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
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Multiplayer/NetworkEventManagerImpl.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "TestHelpers.h"
#include "quickjspp.hpp"

#include "gtest/gtest.h"
#include <CSP/Multiplayer/Components/CustomSpaceComponent.h>
#include <CSP/Multiplayer/Components/SplineSpaceComponent.h>
#include <Memory/Memory.h>
#include <PublicAPITests/SpaceSystemTestHelpers.h>
#include <PublicAPITests/UserSystemTestHelpers.h>
#include <atomic>

using namespace csp::multiplayer;

namespace
{

MultiplayerConnection* Connection;
SpaceEntitySystem* EntitySystem;

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateAvatarForLeaderElection(csp::multiplayer::SpaceEntitySystem* EntitySystem)
{
    const csp::common::String& UserName = "Player 1";
    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String& UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;
    LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);
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

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_UPDATE_SPACE_ENTITY_GLOBAL_POSITION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceEntityTests, UpdateSpaceEntityGlobalPositionTest)
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

    std::atomic_bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    CreateAvatarForLeaderElection(EntitySystem);

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

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cout << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    // Wait until the property have been updated and correct callback has been recieved
    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3(1, 1, 1), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3(10, 10, 10), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_EQ(Object->GetGlobalPosition(), csp::common::Vector3(1, 1, 1));
        EXPECT_EQ(ChildObject->GetGlobalPosition(), csp::common::Vector3(11, 11, 11));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        const bool ScriptHasErrors = ChildObject->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

        bool EntityUpdated = false;

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

        // Create callback to process pending entity operations
        auto EntityUpdatedIsReady = [&EntityUpdated, &EntitySystem]()
        {
            EntitySystem->ProcessPendingEntityOperations();

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5));

        EXPECT_EQ(Object->GetGlobalPosition(), csp::common::Vector3::One());
        EXPECT_EQ(ChildObject->GetGlobalPosition(), csp::common::Vector3(2, 2, 2));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_UPDATE_SPACE_ENTITY_GLOBAL_ROTATION_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceEntityTests, UpdateSpaceEntityGlobalRotationTest)
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

    std::atomic_bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    CreateAvatarForLeaderElection(EntitySystem);

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

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cout << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    // Wait until the property have been updated and correct callback has been recieved
    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4(1, 1, 1, 1), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4(10, 10, 10, 10), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_EQ(Object->GetGlobalRotation(), csp::common::Vector4::One());
        EXPECT_EQ(ChildObject->GetGlobalRotation(), csp::common::Vector4(20, 20, 20, -20));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        const bool ScriptHasErrors = ChildObject->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

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
        auto EntityUpdatedIsReady = [&EntityUpdated, &EntitySystem]()
        {
            EntitySystem->ProcessPendingEntityOperations();

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5));

        EXPECT_EQ(Object->GetGlobalRotation(), csp::common::Vector4::One());
        EXPECT_EQ(ChildObject->GetGlobalRotation(), csp::common::Vector4(2, 2, 2, -2));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_UPDATE_SPACE_ENTITY_GLOBAL_SCALE_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceEntityTests, UpdateSpaceEntityGlobalScaleTest)
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

    std::atomic_bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    CreateAvatarForLeaderElection(EntitySystem);

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

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cout << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    // Wait until the property have been updated and correct callback has been recieved
    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3(2, 2, 2) };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3(10, 10, 10) };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_EQ(Object->GetGlobalScale(), csp::common::Vector3(2, 2, 2));
        EXPECT_EQ(ChildObject->GetGlobalScale(), csp::common::Vector3(20, 20, 20));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        const bool ScriptHasErrors = ChildObject->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

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
        auto EntityUpdatedIsReady = [&EntityUpdated, &EntitySystem]()
        {
            EntitySystem->ProcessPendingEntityOperations();

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5));

        EXPECT_EQ(Object->GetGlobalScale(), csp::common::Vector3(2, 2, 2));
        EXPECT_EQ(ChildObject->GetGlobalScale(), csp::common::Vector3(4, 4, 4));
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_UPDATE_SPACE_ENTITY_PARENT_ID_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceEntityTests, UpdateSpaceEntityParentIdTest)
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

    std::atomic_bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    CreateAvatarForLeaderElection(EntitySystem);

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

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cout << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    // Wait until the property have been updated and correct callback has been recieved
    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(EntitySystem, CreateObject, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        ChildObject->QueueUpdate();
        Object->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_NE(Object, ChildObject->GetParentEntity());

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        ChildObject->QueueUpdate();
        Object->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        const bool ScriptHasErrors = ChildObject->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

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
        auto EntityUpdatedIsReady = [&EntityUpdated, &EntitySystem]()
        {
            EntitySystem->ProcessPendingEntityOperations();

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5));

        EXPECT_EQ(Object, ChildObject->GetParentEntity());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_REMOVE_SPACE_ENTITY_PARENT_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceEntityTests, RemoveSpaceEntityParentTest)
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

    std::atomic_bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    CreateAvatarForLeaderElection(EntitySystem);

    const std::string ScriptText = R"xx(
		
		var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];

			entity.removeParentEntity();
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");

	)xx";

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cout << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    // Wait until the property have been updated and correct callback has been recieved
    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3(1, 1, 1), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3(10, 10, 10), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_EQ(Object, ChildObject->GetParentEntity());

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        const bool ScriptHasErrors = ChildObject->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

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
        auto EntityUpdatedIsReady = [&EntityUpdated, &EntitySystem]()
        {
            EntitySystem->ProcessPendingEntityOperations();

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5));

        EXPECT_NE(Object, ChildObject->GetParentEntity());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_GET_ROOT_HIERARCHY_ENTITIES_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceEntityTests, GetRootHierarchyEntitiesTest)
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

    std::atomic_bool ScriptSystemReady = false;

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cout << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    CreateAvatarForLeaderElection(EntitySystem);

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

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cout << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    // Wait until the property have been updated and correct callback has been recieved
    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        const csp::common::String ChildObjectName = "Child Object 1";
        SpaceTransform ChildObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [ChildObject] = AWAIT(Object, CreateChildEntity, ChildObjectName, ChildObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(ChildObject->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_EQ(Object->GetPosition(), csp::common::Vector3::Zero());
        EXPECT_EQ(ChildObject->GetPosition(), csp::common::Vector3::Zero());

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        ChildObject->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        Object->QueueUpdate();
        ChildObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        const bool ScriptHasErrors = ChildObject->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

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
        auto EntityUpdatedIsReady = [&EntityUpdated, &EntitySystem]()
        {
            EntitySystem->ProcessPendingEntityOperations();

            std::cout << "Waiting for EntityUpdatedIsReady" << std::endl;
            return (EntityUpdated == true);
        };

        // Wait until the property have been updated and correct callback has been recieved
        ResponseWaiter::WaitFor(EntityUpdatedIsReady, std::chrono::seconds(5));

        EXPECT_EQ(Object->GetPosition(), csp::common::Vector3::One());
        EXPECT_EQ(ChildObject->GetPosition(), csp::common::Vector3::Zero());
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif