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
#include <future>
#include <gmock/gmock.h>

using namespace csp::multiplayer;

void OnUserCreated(SpaceEntity* inUser);

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void OnUserCreated(SpaceEntity* inUser)
{
    EXPECT_EQ(inUser->GetComponents()->Size(), 1);

    auto* avatarComponent = inUser->GetComponent(0);

    EXPECT_EQ(avatarComponent->GetComponentType(), ComponentType::AvatarData);

    std::cerr << "OnUserCreated" << std::endl;
}

namespace CSPEngine
{

class ScriptBinding : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class CreateScript : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class RunScript : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class AvatarScript : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class ScriptLog : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class DeleteScript : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class DeleteAndChangeComponent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class AddSecondScript : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class ScriptDeltaTime : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class CustomComponentScriptInterfaceSubscription : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class MultipleScriptComponent : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

class ModifyExistingScript : public PublicTestBaseWithParam<csp::common::RealtimeEngineType>
{
};

TEST_P(ScriptBinding, ScriptBindingTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& scriptSystem = *systemsManager.GetScriptSystem();

    std::string testMessage;

    scriptSystem.Initialise();

    auto fn = [&testMessage](const char* str)
    {
        testMessage = str;
        CSP_LOG_MSG(csp::common::LogLevel::Log, str);
        std::cout << str << "\n";
    };

    constexpr int contextId = 0;

    scriptSystem.CreateContext(contextId);

    qjs::Context::Module* module = (qjs::Context::Module*)scriptSystem.GetModule(contextId, "CSPTest");

    module->function("RunFunction", fn);

    std::string scriptText = R"xx(

        import * as CSPTest from "CSPTest";
        CSPTest.RunFunction('Hello Test');

        globalThis.onCallback = function()
        {   
            CSPTest.RunFunction('Hello Callback');
        }

    )xx";

    bool noScriptErrors = scriptSystem.RunScript(contextId, scriptText.c_str());

    EXPECT_TRUE(noScriptErrors);
    EXPECT_TRUE(testMessage == "Hello Test");

    scriptSystem.RunScript(contextId, "onCallback()");

    EXPECT_TRUE(testMessage == "Hello Callback");

    scriptSystem.DestroyContext(contextId);
    scriptSystem.Shutdown();
}

TEST_P(CreateScript, CreateScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // we'll be using this in a few places below as part of the test, so we declare it upfront
    const std::string scriptText = R"xx(

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
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});
        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        Object->GetScript().Invoke();

        const bool scriptHasErrors = Object->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        Object->QueueUpdate();

        ProcessPendingIfOnline(*realtimeEngine);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(RunScript, RunScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::atomic_bool scriptSystemReady = false;

    auto entityCreatedCallback = [](SpaceEntity* /*Entity*/) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto entitiesReadyCallback = [](int /*NumEntitiesFetched*/) { std::cerr << "EntitiesReadyCallback called" << std::endl; };

    auto scriptSystemReadyCallback = [&scriptSystemReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptSystemReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetRemoteEntityCreatedCallback(entityCreatedCallback);
    }
    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptSystemReadyCallback);
    }

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    // we'll be using this in a few places below as part of the test, so we declare it upfront
    const std::string scriptText = R"xx(

        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);
		
		globalThis.onTick = () => {
            OKO.Log('onTick Called');
			var model = entities[entityIndex].getAnimatedModelComponents()[0];
			model.position = [10, 10, 10];
		}

		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (scriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    // Create an AnimatedModelComponent and have the script update it's position
    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        AnimatedModelSpaceComponent* animatedModelComponent
            = static_cast<AnimatedModelSpaceComponent*>(Object->AddComponent(ComponentType::AnimatedModel));
        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        Object->GetScript().Invoke();

        csp::CSPFoundation::Tick();

        const bool scriptHasErrors = Object->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);

        EXPECT_EQ(animatedModelComponent->GetPosition().X, 10.f);
        EXPECT_EQ(animatedModelComponent->GetPosition().Y, 10.f);
        EXPECT_EQ(animatedModelComponent->GetPosition().Z, 10.f);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(AvatarScript, AvatarScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    csp::common::String userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), userName);
    // TODO: Verify these values
    /*EXPECT_EQ(Avatar->GetPosition(), UserTransform.Position);
    EXPECT_EQ(Avatar->GetRotation(), UserTransform.Rotation);*/

    std::string avatarScriptText = R"xx(

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

    Avatar->GetScript().SetScriptSource(avatarScriptText.c_str());
    Avatar->GetScript().Invoke();

    ProcessPendingIfOnline(*realtimeEngine);

    auto& components = *Avatar->GetComponents();

    EXPECT_EQ(components.Size(), 2);

    auto* component = components[0];

    EXPECT_EQ(component->GetComponentType(), ComponentType::AvatarData);

    auto* scriptComponent = components[1];

    EXPECT_EQ(scriptComponent->GetComponentType(), ComponentType::ScriptData);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ScriptLog, ScriptLogTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    AvatarState userAvatarState = AvatarState::Idle;
    csp::common::String userAvatarId = "MyCoolAvatar";
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), userName);

    RAIIMockLogger mockLogger;
    // Expect 2 logs
    // The script logger naeively adds spaces to allow lots of arguments to be passed, which is why the test data has a weird trailing space.
    csp::common::String cspLogMsg = "Testing CSP.Log ";
    csp::common::String okoLogMsg = "Testing OKO.Log ";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(testing::_, testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, cspLogMsg));
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, okoLogMsg));

    std::string avatarCspLogScriptText = R"xx(

        import * as CSP from "CSP";

        CSP.Log("Testing CSP.Log");

    )xx";

    Avatar->GetScript().SetScriptSource(avatarCspLogScriptText.c_str());
    Avatar->GetScript().Invoke();

    std::string avatarOkoLogScriptText = R"xx(

        import * as OKO from "OKO";

        OKO.Log("Testing OKO.Log");

    )xx";

    Avatar->GetScript().SetScriptSource(avatarOkoLogScriptText.c_str());
    Avatar->GetScript().Invoke();

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(DeleteScript, DeleteScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string scriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [10, 10, 10];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    // Create object
    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create script
    auto* scriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    // Ensure position is set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    // Delete script component
    CreatedObject->RemoveComponent(scriptComponent->GetId());

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    // Tick to attempt to call scripts tick event
    csp::CSPFoundation::Tick();

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    // Ensure position is still set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(DeleteAndChangeComponent, DeleteAndChangeComponentTest)
{
    // Test for: OB-864
    // Second script deletion test adds a second component to the object with the script
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string scriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [10, 10, 10];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    // Create object
    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create animated model component
    auto* animatedComponent = static_cast<AnimatedModelSpaceComponent*>(CreatedObject->AddComponent(ComponentType::AnimatedModel));

    // Create script
    auto* scriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine); // Crash!

    // Make a component update
    animatedComponent->SetPosition(csp::common::Vector3::One());

    // Delete script component
    CreatedObject->RemoveComponent(scriptComponent->GetId());

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    // Ensure entity update doesn't crash
    csp::CSPFoundation::Tick();

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(AddSecondScript, AddSecondScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](int) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        // Since we're waiting on patches, the test will often run to fast and hit the patch rate limit.
        static_cast<OnlineRealtimeEngine*>(realtimeEngine.get())->SetEntityPatchRateLimitEnabled(false);

        // Disable leader election, as it's not relevant and it's annoying to wait for the callbacks (which you have to do or the scripts won't run)
        // This has to happen after EnterSpace, as EnterSpace sets this value on entry.
        static_cast<OnlineRealtimeEngine*>(realtimeEngine.get())->DisableLeaderElection();
    }

    // Create object
    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Add the script
    const std::string scriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [1, 1, 1];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    auto* scriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ASSERT_NE(CreatedObject->FindFirstComponentOfType(ComponentType::ScriptData), nullptr);
    scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    auto waitForPatchFuture = [&CreatedObject]()
    {
        std::shared_ptr<std::promise<void>> patchPromise = std::make_shared<std::promise<void>>();
        std::shared_future<void> patchFuture = patchPromise->get_future().share();

        std::shared_ptr<std::once_flag> onceFlag = std::make_shared<std::once_flag>();
        CreatedObject->SetPatchSentCallback(
            [patchPromise, onceFlag](bool ok)
            {
                // Don't double-set the promise if we get more than one patch. (C++20 std::latch would be cleaner)
                std::call_once(*onceFlag,
                    [patchPromise, ok] {
                        ok ? patchPromise->set_value()
                           : patchPromise->set_exception(std::make_exception_ptr(std::runtime_error("Unexpected error waiting for patch")));
                    });
            });

        return patchFuture;
    };

    const auto patchWaiter = waitForPatchFuture();

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    patchWaiter.wait();

    // Remove the script without doing anything
    CreatedObject->RemoveComponent(scriptComponent->GetId());

    const auto patchWaiter2 = waitForPatchFuture();
    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);
    patchWaiter2.wait();

    ASSERT_EQ(CreatedObject->FindFirstComponentOfType(ComponentType::ScriptData), nullptr);
    // We have not ticked yet, so the object position should still be zero
    ASSERT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    // Add the script yet again
    auto* scriptComponent2 = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    scriptComponent2->SetScriptSource(csp::common::String(scriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    ASSERT_NE(CreatedObject->FindFirstComponentOfType(ComponentType::ScriptData), nullptr);

    // Tick this time, and observe the position update
    const auto patchWaiter3 = waitForPatchFuture();
    CreatedObject->QueueUpdate();
    csp::CSPFoundation::Tick();
    ProcessPendingIfOnline(*realtimeEngine);
    patchWaiter3.wait();

    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::One());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(ScriptDeltaTime, ScriptDeltaTimeTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    const std::string scriptText = R"xx(

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
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        ProcessPendingIfOnline(*realtimeEngine);

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        Object->GetScript().Invoke();

        csp::CSPFoundation::Tick();

        const bool scriptHasErrors = Object->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(CustomComponentScriptInterfaceSubscription, CustomComponentScriptInterfaceSubscriptionTest)

{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::atomic_bool scriptSystemReady = false;

    auto entityCreatedCallback = [](SpaceEntity* /*Entity*/) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto entitiesReadyCallback = [](int /*NumEntitiesFetched*/) { std::cerr << "EntitiesReadyCallback called" << std::endl; };

    auto scriptSystemReadyCallback = [&scriptSystemReady](bool ok)
    {
        EXPECT_EQ(ok, true);
        std::cout << "ScriptLeaderReadyCallback called" << std::endl;
        scriptSystemReady = true;
    };

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine.get())->SetRemoteEntityCreatedCallback(entityCreatedCallback);
    }

    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<OnlineRealtimeEngine*>(realtimeEngine.get())->SetScriptLeaderReadyCallback(scriptSystemReadyCallback);
    }

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    // Create object to represent the audio
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create audio component
    auto* customComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);

    customComponent->SetCustomProperty("Number", csp::common::ReplicatedValue(int64_t(0)));
    customComponent->SetCustomProperty("NumberChanged", csp::common::ReplicatedValue(false));

    CreatedObject->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    // Setup script
    std::string scriptText = R"xx(
		var custom = ThisEntity.getCustomComponents()[0];
		custom.applicationOrigin = "TestApplicationOrigin";
		custom.setCustomProperty("testFloat", 1.234);
		custom.setCustomProperty("testInt", 1234);
		globalThis.onValueChanged = () => {
		  custom.setCustomProperty("NumberChanged", true);
		}  
		// subscribe to entity events 
		ThisEntity.subscribeToPropertyChange(custom.id, custom.getCustomPropertySubscriptionKey("Number"), "valueChanged");
		ThisEntity.subscribeToMessage("valueChanged", "onValueChanged");
		)xx";

    if (engineType == csp::common::RealtimeEngineType::Online)
    {
        auto scriptSystemIsReady = [&scriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (scriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(scriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    CreatedObject->GetScript().SetScriptSource(scriptText.c_str());
    CreatedObject->GetScript().Invoke();

    ProcessPendingIfOnline(*realtimeEngine);

    ASSERT_EQ(customComponent->GetCustomProperty("testFloat").GetFloat(), 1.234f);
    ASSERT_EQ(customComponent->GetCustomProperty("testInt").GetInt(), 1234);
    ASSERT_EQ(customComponent->GetCustomProperty("Number").GetInt(), 0);
    ASSERT_FALSE(customComponent->GetCustomProperty("NumberChanged").GetBool());
    EXPECT_EQ(customComponent->GetApplicationOrigin(), "TestApplicationOrigin");

    customComponent->SetCustomProperty("Number", csp::common::ReplicatedValue(int64_t(100)));

    ASSERT_EQ(customComponent->GetCustomProperty("Number").GetInt(), 100);
    ASSERT_TRUE(customComponent->GetCustomProperty("NumberChanged").GetBool());

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

TEST_P(MultipleScriptComponent, MultipleScriptComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String userName = "Player 1";
    SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;
    csp::common::String userAvatarId = "MyCoolAvatar";

    AvatarState userState = AvatarState::Idle;
    AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    const auto loginState = userSystem->GetLoginState();

    auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userState, userAvatarId,
        userAvatarPlayMode, LocomotionModel::Grounded);
    EXPECT_NE(Avatar, nullptr);

    std::cerr << "CreateAvatar Local Callback" << std::endl;

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);

    if (Avatar->GetEntityType() == SpaceEntityType::Avatar)
    {
        OnUserCreated(Avatar);
    }

    // Create space object
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [SpaceEntity] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Attempt to add 2 script components
    SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);
    SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);

    SpaceEntity->QueueUpdate();
    ProcessPendingIfOnline(*realtimeEngine);

    // Only 1 script component should be on the object
    EXPECT_EQ(SpaceEntity->GetComponents()->Size(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete MultiplayerConnection

    // Log out
    LogOut(userSystem);
}

// This test will be fixed and re-instated as part of OF-1539
TEST_P(ModifyExistingScript, ModifyExistingScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::common::RealtimeEngineType engineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> realtimeEngine { systemsManager.MakeRealtimeEngine(engineType) };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // we'll be using this in a few places below as part of the test, so we declare it upfront
    const std::string scriptText = R"xx(

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // For our first phase of this script test, we simply an object with a script component, and assign it
    // a valid script, tell CHS about it and then bail out of the connection
    {
        const csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = Awaitable(&csp::common::IRealtimeEngine::CreateEntity, static_cast<csp::common::IRealtimeEngine*>(realtimeEngine.get()),
            objectName, objectTransform, csp::common::Optional<uint64_t>())
                            .Await();
        ScriptSpaceComponent* scriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        Object->QueueUpdate();

        ProcessPendingIfOnline(*realtimeEngine);
    }

    //------------------------------------------------------------
    // For our second phase of the test, we attempt to take an entity that already exists (we created it in phase 1), modify the script source and
    // re-invoke the script

    // interesting part of phase 2 begins!
    {
        csp::multiplayer::SpaceEntity* object = realtimeEngine->GetEntityByIndex(0);
        // grab the script component we created in phase 1 (we should make this kind of thing easier)
        const csp::common::Array<ComponentBase*>& components = *object->GetComponents()->Values();
        ScriptSpaceComponent* scriptComponent = nullptr;
        for (size_t i = 0; components.Size(); i++)
        {
            if (components[i]->GetComponentType() == csp::multiplayer::ComponentType::ScriptData)
            {
                scriptComponent = dynamic_cast<ScriptSpaceComponent*>(components[i]);
                break;
            }
        }

        // phew! now we have that we can attempt to modify script source again and re-invoke - this is the part that we really want to test
        // can we successfully modify a pre-existing script, and re-invoke it without script errors?
        scriptComponent->SetScriptSource(csp::common::String(scriptText.c_str()));
        object->GetScript().Invoke();

        const bool scriptHasErrors = object->GetScript().HasError();
        EXPECT_FALSE(scriptHasErrors);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

INSTANTIATE_TEST_SUITE_P(ScriptSystemTests, ScriptBinding,
    testing::Values(csp::common::RealtimeEngineType::Offline)); // Dosent actually use the realtime engine, but stick to the pattern because
                                                                // everything else does
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, CreateScript, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, RunScript, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, AvatarScript, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, ScriptLog, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, DeleteScript, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, DeleteAndChangeComponent, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, AddSecondScript, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, ScriptDeltaTime, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(ScriptSystemTests, CustomComponentScriptInterfaceSubscription,
    testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, MultipleScriptComponent, testing::Values(csp::common::RealtimeEngineType::Offline, csp::common::RealtimeEngineType::Online));
INSTANTIATE_TEST_SUITE_P(
    ScriptSystemTests, ModifyExistingScript, testing::Values(csp::common::RealtimeEngineType::Online, csp::common::RealtimeEngineType::Offline));

}