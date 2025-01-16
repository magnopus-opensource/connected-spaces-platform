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

void OnUserCreated(SpaceEntity* InUser);

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void OnUserCreated(SpaceEntity* InUser)
{
    EXPECT_EQ(InUser->GetComponents()->Size(), 1);

    auto* AvatarComponent = InUser->GetComponent(0);

    EXPECT_EQ(AvatarComponent->GetComponentType(), ComponentType::AvatarData);

    std::cerr << "OnUserCreated" << std::endl;
}

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPTSYSTEM_SCRIPT_BINDING_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ScriptBindingTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& ScriptSystem = *SystemsManager.GetScriptSystem();

    std::string TestMessage;

    ScriptSystem.Initialise();

    auto Fn = [&TestMessage](const char* Str)
    {
        TestMessage = Str;
        CSP_LOG_MSG(csp::systems::LogLevel::Log, Str);
        std::cout << Str << "\n";
    };

    constexpr int ContextId = 0;

    ScriptSystem.CreateContext(ContextId);

    qjs::Context* Context = (qjs::Context*)ScriptSystem.GetContext(ContextId);
    qjs::Context::Module* Module = (qjs::Context::Module*)ScriptSystem.GetModule(ContextId, "CSPTest");

    Module->function("RunFunction", Fn);

    std::string ScriptText = R"xx(

        import * as CSPTest from "CSPTest";
        CSPTest.RunFunction('Hello Test');

        globalThis.onCallback = function()
        {   
            CSPTest.RunFunction('Hello Callback');
        }

    )xx";

    bool NoScriptErrors = ScriptSystem.RunScript(ContextId, ScriptText.c_str());

    EXPECT_TRUE(NoScriptErrors);
    EXPECT_TRUE(TestMessage == "Hello Test");

    ScriptSystem.RunScript(ContextId, "onCallback()");

    EXPECT_TRUE(TestMessage == "Hello Callback");

    ScriptSystem.DestroyContext(ContextId);
    ScriptSystem.Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPT_CREATE_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, CreateScriptTest)
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // we'll be using this in a few places below as part of the test, so we declare it upfront
    const std::string ScriptText = R"xx(

         var entities = TheEntitySystem.getEntities();
		  var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		  globalThis.onClick = (_evtName, params) => {
		    const { id, cid } = JSON.parse(params);
		    CSP.Log(`Clicked entityId: ${id} componentId: ${cid}`);
		  }

		  globalThis.onTick = () => {
		    CSP.Log('Tick');
		  }

		  ThisEntity.subscribeToMessage("buttonPressed", "onClick");
		  ThisEntity.subscribeToMessage("entityTick", "onTick");

			CSP.Log('Printing to the log from a script');
		  
    )xx";

    // Let's create a simple script and see if we can invoke it OK
    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);
        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript()->Invoke();

        const bool ScriptHasErrors = Object->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

        Object->QueueUpdate();

        EntitySystem->ProcessPendingEntityOperations();
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, RunScriptTest)
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

    auto EntityCreatedCallback = [](SpaceEntity* Entity) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto EntitiesReadyCallback = [](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "EntitiesReadyCallback called" << std::endl;
    };

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    EntitySystem->SetEntityCreatedCallback(EntityCreatedCallback);
    EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    // we'll be using this in a few places below as part of the test, so we declare it upfront
    const std::string ScriptText = R"xx(

        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);
		
		globalThis.onTick = () => {
            OKO.Log('onTick Called');
			var model = entities[entityIndex].getAnimatedModelComponents()[0];
			model.position = [10, 10, 10];
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cerr << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    // Create an AnimatedModelComponent and have the script update it's position
    {
        EntitySystem->SetEntityCreatedCallback([](SpaceEntity* Entity) {});

        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        AnimatedModelSpaceComponent* AnimatedModelComponent
            = static_cast<AnimatedModelSpaceComponent*>(Object->AddComponent(ComponentType::AnimatedModel));
        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        const bool ScriptHasErrors = Object->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);

        EXPECT_EQ(AnimatedModelComponent->GetPosition().X, 10.f);
        EXPECT_EQ(AnimatedModelComponent->GetPosition().Y, 10.f);
        EXPECT_EQ(AnimatedModelComponent->GetPosition().Z, 10.f);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPT_AVATAR_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, AvatarScriptTest)
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);
    // TODO: Verify these values
    /*EXPECT_EQ(Avatar->GetPosition(), UserTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), UserTransform.Rotation);*/

    std::string AvatarScriptText = R"xx(

        import * as CSP from "CSP";

        CSP.Log("Entering AvatarScriptTest Script");

        var avatars = TheEntitySystem.getAvatars();

        for (let i=0; i<avatars.length; ++i)
        {
            CSP.Log(JSON.stringify(avatars[i].name));
            CSP.Log(JSON.stringify(avatars[i].id));
            CSP.Log(JSON.stringify(avatars[i].position));
            CSP.Log(JSON.stringify(avatars[i].rotation));
            CSP.Log(JSON.stringify(avatars[i].scale));
        }

        avatars[0].position = [3, 2, 5];
        CSP.Log(JSON.stringify(avatars[0].position));

    )xx";

    Avatar->GetScript()->SetScriptSource(AvatarScriptText.c_str());
    Avatar->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    auto& Components = *Avatar->GetComponents();

    EXPECT_EQ(Components.Size(), 2);

    auto* Component = Components[0];

    EXPECT_EQ(Component->GetComponentType(), ComponentType::AvatarData);

    auto* ScriptComponent = Components[1];

    EXPECT_EQ(ScriptComponent->GetComponentType(), ComponentType::ScriptData);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPT_LOG_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ScriptLogTest)
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);

    std::string AvatarScriptText = R"xx(

        import * as CSP from "CSP";

        CSP.Log("Testing CSP.Log");

    )xx";

    Avatar->GetScript()->SetScriptSource(AvatarScriptText.c_str());
    Avatar->GetScript()->Invoke();

    std::string AvatarOKOScriptText = R"xx(

        import * as OKO from "OKO";

        OKO.Log("Testing OKO.Log");

    )xx";

    Avatar->GetScript()->SetScriptSource(AvatarScriptText.c_str());
    Avatar->GetScript()->Invoke();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_DELETE_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, DeleteScriptTest)
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

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string ScriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [10, 10, 10];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    // Create object
    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create script
    auto* ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript()->Invoke();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Ensure position is set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    // Delete script component
    CreatedObject->RemoveComponent(ScriptComponent->GetId());

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Tick to attempt to call scripts tick event
    csp::CSPFoundation::Tick();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Ensure position is still set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPT_DELETE_AND_CHANGE_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, DeleteAndChangeComponentTest)
{
    // Test for: OB-864
    // Second script deletion test adds a second component to the object with the script
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string ScriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [10, 10, 10];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    // Create object
    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create animated model component
    auto* AnimatedComponent = static_cast<AnimatedModelSpaceComponent*>(CreatedObject->AddComponent(ComponentType::AnimatedModel));

    // Create script
    auto* ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript()->Invoke();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Make a component update
    AnimatedComponent->SetPosition(csp::common::Vector3::One());

    // Delete script component
    CreatedObject->RemoveComponent(ScriptComponent->GetId());

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Ensure entity update doesn't crash
    csp::CSPFoundation::Tick();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_ADD_SECOND_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, AddSecondScriptTest)
{
    // Test for OB-1407
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

    std::atomic_bool ScriptSystemReady = false;

    auto EntityCreatedCallback = [](SpaceEntity* Entity) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto EntitiesReadyCallback = [](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "EntitiesReadyCallback called" << std::endl;
    };

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    EntitySystem->SetEntityCreatedCallback(EntityCreatedCallback);
    EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string ScriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [1, 1, 1];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cerr << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    // Create object
    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    bool PatchPending = true;
    CreatedObject->SetPatchSentCallback([&PatchPending](bool ok) { PatchPending = false; });

    // Create script
    auto* ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript()->Invoke();

    CreatedObject->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    PatchPending = true;

    // Delete script component
    CreatedObject->RemoveComponent(ScriptComponent->GetId());

    CreatedObject->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    PatchPending = true;

    // Ensure position is set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    // Re-add script component
    ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript()->Invoke();

    CreatedObject->QueueUpdate();

    while (PatchPending)
    {
        EntitySystem->ProcessPendingEntityOperations();
        csp::CSPFoundation::Tick();
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::One());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPT_DELTA_TIME_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ScriptDeltaTimeTest)
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string ScriptText = R"xx(

        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);
		
		globalThis.onTick = (_evtName, params) => {
			 const { deltaTimeMS } = JSON.parse(params);
			 CSP.Log(_evtName);
			 CSP.Log(deltaTimeMS);
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript()->Invoke();

        csp::CSPFoundation::Tick();

        const bool ScriptHasErrors = Object->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_CUSTOM_COMPONENT_SCRIPT_INTERFACE_SUBSCRIPTION_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, CustomComponentScriptInterfaceSubscriptionTest)
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

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    std::atomic_bool ScriptSystemReady = false;

    auto EntityCreatedCallback = [](SpaceEntity* Entity) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto EntitiesReadyCallback = [](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "EntitiesReadyCallback called" << std::endl;
    };

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "ScriptSystemReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    EntitySystem->SetEntityCreatedCallback(EntityCreatedCallback);
    EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
    EntitySystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    // Create object to represent the audio
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create audio component
    auto* CustomComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);

    CustomComponent->SetCustomProperty("Number", ReplicatedValue(int64_t(0)));
    CustomComponent->SetCustomProperty("NumberChanged", ReplicatedValue(false));

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script
    std::string ScriptText = R"xx(
		var custom = ThisEntity.getCustomComponents()[0];
		custom.setCustomProperty("testFloat", 1.234);
		custom.setCustomProperty("testInt", 1234);
		globalThis.onValueChanged = () => {
		  custom.setCustomProperty("NumberChanged", true);
		}  
		// subscribe to entity events 
		ThisEntity.subscribeToPropertyChange(custom.id, custom.getCustomPropertySubscriptionKey("Number"), "valueChanged");
		ThisEntity.subscribeToMessage("valueChanged", "onValueChanged");
		)xx";

    auto ScriptSystemIsReady = [&ScriptSystemReady]()
    {
        std::cerr << "Waiting for ScriptSystemReady" << std::endl;
        return (ScriptSystemReady == true);
    };

    EXPECT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);

    CreatedObject->GetScript()->SetScriptSource(ScriptText.c_str());
    CreatedObject->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(CustomComponent->GetCustomProperty("testFloat").GetFloat(), 1.234f);
    EXPECT_EQ(CustomComponent->GetCustomProperty("testInt").GetInt(), 1234);
    EXPECT_EQ(CustomComponent->GetCustomProperty("Number").GetInt(), 0);
    EXPECT_FALSE(CustomComponent->GetCustomProperty("NumberChanged").GetBool());

    CustomComponent->SetCustomProperty("Number", ReplicatedValue(int64_t(100)));

    EXPECT_EQ(CustomComponent->GetCustomProperty("Number").GetInt(), 100);
    EXPECT_TRUE(CustomComponent->GetCustomProperty("NumberChanged").GetBool());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_MULTIPLE_SCRIPT_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, MultipleScriptComponentTest)
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

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserState, UserAvatarId, UserAvatarPlayMode);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    // Create space object
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [SpaceEntity] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Attempt to add 2 script components
    auto Comp1 = SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);
    auto Comp2 = SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);

    SpaceEntity->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Only 1 script component should be on the object
    EXPECT_EQ(SpaceEntity->GetComponents()->Size(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete MultiplayerConnection

    // Log out
    LogOut(UserSystem);
}
#endif

// This test will be fixed and re-instated as part of OF-1539
#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_MODIFY_EXISTING_SCRIPT_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ScriptSystemTests, ModifyExistingScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

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

    // we'll be using this in a few places below as part of the test, so we declare it upfront
    const std::string ScriptText = R"xx(

		 var entities = TheEntitySystem.getEntities();
		  var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		  globalThis.onClick = (_evtName, params) => {
			const { id, cid } = JSON.parse(params);
			CSP.Log(`Clicked entityId: ${id} componentId: ${cid}`);
		  }

		  globalThis.onTick = () => {
			CSP.Log('Tick');
		  }

		  ThisEntity.subscribeToMessage("buttonPressed", "onClick");
		  ThisEntity.subscribeToMessage("entityTick", "onTick");

			CSP.Log('Printing to the log from a script');

	)xx";

    // For our first phase of this script test, we simply an object with a script component, and assign it
    // a valid script, tell CHS about it and then bail out of the connection
    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = Awaitable(&SpaceEntitySystem::CreateObject, EntitySystem, ObjectName, ObjectTransform).Await();
        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->QueueUpdate();

        EntitySystem->ProcessPendingEntityOperations();
    }

    //------------------------------------------------------------
    // For our second phase of the test, we attempt to take an entity that already exists (we created it in phase 1), modify the script source and
    // re-invoke the script

    bool EntityHasBeenRecreated = false;
    // we're gonna wanna wait till the entity is created before we can do our test
    EntitySystem->SetEntityCreatedCallback([&EntityHasBeenRecreated](csp::multiplayer::SpaceEntity* Object) { EntityHasBeenRecreated = true; });

    // spin till we recreate the entity from phase 1 locally, having received it back from CHS
    while (EntityHasBeenRecreated == false)
    {
    }

    // interesting part of phase 2 begins!
    {
        csp::multiplayer::SpaceEntity* Object = EntitySystem->GetEntityByIndex(0);
        // grab the script component we created in phase 1 (we should make this kind of thing easier)
        const csp::common::Array<ComponentBase*>& Components = *Object->GetComponents()->Values();
        ScriptSpaceComponent* ScriptComponent = nullptr;
        for (size_t i = 0; Components.Size(); i++)
        {
            if (Components[i]->GetComponentType() == csp::multiplayer::ComponentType::ScriptData)
            {
                ScriptComponent = dynamic_cast<ScriptSpaceComponent*>(Components[i]);
                break;
            }
        }

        // phew! now we have that we can attempt to modify script source again and re-invoke - this is the part that we really want to test
        // can we successfully modify a pre-existing script, and re-invoke it without script errors?
        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript()->Invoke();

        const bool ScriptHasErrors = Object->GetScript()->HasError();
        EXPECT_FALSE(ScriptHasErrors);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace