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

void OnUserCreated(SpaceEntity* InUser);

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void OnUserCreated(SpaceEntity* InUser)
{
    EXPECT_EQ(InUser->GetComponents()->Size(), 1);

    auto* AvatarComponent = InUser->GetComponent(0);

    EXPECT_EQ(AvatarComponent->GetComponentType(), ComponentType::AvatarData);

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
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& ScriptSystem = *SystemsManager.GetScriptSystem();

    std::string TestMessage;

    ScriptSystem.Initialise();

    auto Fn = [&TestMessage](const char* Str)
    {
        TestMessage = Str;
        CSP_LOG_MSG(csp::common::LogLevel::Log, Str);
        std::cout << Str << "\n";
    };

    constexpr int ContextId = 0;

    ScriptSystem.CreateContext(ContextId);

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

TEST_P(CreateScript, CreateScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

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
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});
        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript().Invoke();

        const bool ScriptHasErrors = Object->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);

        Object->QueueUpdate();

        RealtimeEngine->ProcessPendingEntityOperations();
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(RunScript, RunScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

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

    auto EntityCreatedCallback = [](SpaceEntity* /*Entity*/) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto EntitiesReadyCallback = [](int /*NumEntitiesFetched*/) { std::cerr << "EntitiesReadyCallback called" << std::endl; };

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetRemoteEntityCreatedCallback(EntityCreatedCallback);
    RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptSystemReadyCallback);
    }

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode);
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

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (ScriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    // Create an AnimatedModelComponent and have the script update it's position
    {
        RealtimeEngine->SetRemoteEntityCreatedCallback([](SpaceEntity* /*Entity*/) {});

        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        AnimatedModelSpaceComponent* AnimatedModelComponent
            = static_cast<AnimatedModelSpaceComponent*>(Object->AddComponent(ComponentType::AnimatedModel));
        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript().Invoke();

        csp::CSPFoundation::Tick();

        const bool ScriptHasErrors = Object->GetScript().HasError();
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

TEST_P(AvatarScript, AvatarScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

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

    Avatar->GetScript().SetScriptSource(AvatarScriptText.c_str());
    Avatar->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

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

TEST_P(ScriptLog, ScriptLogTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

    EXPECT_EQ(Avatar->GetEntityType(), SpaceEntityType::Avatar);
    EXPECT_EQ(Avatar->GetName(), UserName);

    RAIIMockLogger MockLogger;
    // Expect 2 logs
    // The script logger naeively adds spaces to allow lots of arguments to be passed, which is why the test data has a weird trailing space.
    csp::common::String CSPLogMsg = "Testing CSP.Log ";
    csp::common::String OKOLogMsg = "Testing OKO.Log ";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(MockLogger.MockLogCallback, Call(CSPLogMsg));
    EXPECT_CALL(MockLogger.MockLogCallback, Call(OKOLogMsg));

    std::string AvatarCSPLogScriptText = R"xx(

        import * as CSP from "CSP";

        CSP.Log("Testing CSP.Log");

    )xx";

    Avatar->GetScript().SetScriptSource(AvatarCSPLogScriptText.c_str());
    Avatar->GetScript().Invoke();

    std::string AvatarOKOLogScriptText = R"xx(

        import * as OKO from "OKO";

        OKO.Log("Testing OKO.Log");

    )xx";

    Avatar->GetScript().SetScriptSource(AvatarOKOLogScriptText.c_str());
    Avatar->GetScript().Invoke();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(DeleteScript, DeleteScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode);
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

    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create script
    auto* ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure position is set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    // Delete script component
    CreatedObject->RemoveComponent(ScriptComponent->GetId());

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Tick to attempt to call scripts tick event
    csp::CSPFoundation::Tick();

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure position is still set to 0
    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(DeleteAndChangeComponent, DeleteAndChangeComponentTest)
{
    // Test for: OB-864
    // Second script deletion test adds a second component to the object with the script
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode);
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

    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create animated model component
    auto* AnimatedComponent = static_cast<AnimatedModelSpaceComponent*>(CreatedObject->AddComponent(ComponentType::AnimatedModel));

    // Create script
    auto* ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations(); // Crash!

    // Make a component update
    AnimatedComponent->SetPosition(csp::common::Vector3::One());

    // Delete script component
    CreatedObject->RemoveComponent(ScriptComponent->GetId());

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure entity update doesn't crash
    csp::CSPFoundation::Tick();

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(AddSecondScript, AddSecondScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](int) {});

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        // Since we're waiting on patches, the test will often run to fast and hit the patch rate limit.
        static_cast<OnlineRealtimeEngine*>(RealtimeEngine.get())->SetEntityPatchRateLimitEnabled(false);
        // Disable leader election, as it's not relevant and it's annoying to wait for the callbacks (which you have to do or the scripts won't run)
        static_cast<OnlineRealtimeEngine*>(RealtimeEngine.get())->DisableLeaderElection();
    }

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object
    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Add the script
    const std::string ScriptText = R"xx(
		
        var entities = TheEntitySystem.getEntities();
		var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		globalThis.onTick = () => {
			var entity = entities[entityIndex];
			entity.position = [1, 1, 1];
		}
 
		ThisEntity.subscribeToMessage("entityTick", "onTick");
		  
    )xx";

    auto* ScriptComponent = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ASSERT_NE(CreatedObject->FindFirstComponentOfType(ComponentType::ScriptData), nullptr);
    ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    auto WaitForPatchFuture = [&CreatedObject]()
    {
        std::shared_ptr<std::promise<void>> PatchPromise = std::make_shared<std::promise<void>>();
        std::shared_future<void> PatchFuture = PatchPromise->get_future().share();

        std::shared_ptr<std::once_flag> OnceFlag = std::make_shared<std::once_flag>();
        CreatedObject->SetPatchSentCallback(
            [PatchPromise, OnceFlag](bool ok)
            {
                // Don't double-set the promise if we get more than one patch. (C++20 std::latch would be cleaner)
                std::call_once(*OnceFlag,
                    [PatchPromise, ok] {
                        ok ? PatchPromise->set_value()
                           : PatchPromise->set_exception(std::make_exception_ptr(std::runtime_error("Unexpected error waiting for patch")));
                    });
            });

        return PatchFuture;
    };

    const auto PatchWaiter = WaitForPatchFuture();

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    PatchWaiter.wait();

    // Remove the script without doing anything
    CreatedObject->RemoveComponent(ScriptComponent->GetId());

    const auto PatchWaiter2 = WaitForPatchFuture();
    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();
    PatchWaiter2.wait();

    ASSERT_EQ(CreatedObject->FindFirstComponentOfType(ComponentType::ScriptData), nullptr);
    // We have not ticked yet, so the object position should still be zero
    ASSERT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::Zero());

    // Add the script yet again
    auto* ScriptComponent2 = static_cast<ScriptSpaceComponent*>(CreatedObject->AddComponent(ComponentType::ScriptData));
    ScriptComponent2->SetScriptSource(csp::common::String(ScriptText.c_str()));
    CreatedObject->GetScript().Invoke();

    ASSERT_NE(CreatedObject->FindFirstComponentOfType(ComponentType::ScriptData), nullptr);

    // Tick this time, and observe the position update
    const auto PatchWaiter3 = WaitForPatchFuture();
    CreatedObject->QueueUpdate();
    csp::CSPFoundation::Tick();
    RealtimeEngine->ProcessPendingEntityOperations();
    PatchWaiter3.wait();

    EXPECT_EQ(CreatedObject->GetPosition(), csp::common::Vector3::One());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(ScriptDeltaTime, ScriptDeltaTimeTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode);
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
        auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        Object->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->GetScript().Invoke();

        csp::CSPFoundation::Tick();

        const bool ScriptHasErrors = Object->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(CustomComponentScriptInterfaceSubscription, CustomComponentScriptInterfaceSubscriptionTest)

{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

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

    auto EntityCreatedCallback = [](SpaceEntity* /*Entity*/) { std::cerr << "EntityCreatedCallback called" << std::endl; };

    auto EntitiesReadyCallback = [](int /*NumEntitiesFetched*/) { std::cerr << "EntitiesReadyCallback called" << std::endl; };

    auto ScriptSystemReadyCallback = [&ScriptSystemReady](bool Ok)
    {
        EXPECT_EQ(Ok, true);
        std::cerr << "ScriptLeaderReadyCallback called" << std::endl;
        ScriptSystemReady = true;
    };

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetRemoteEntityCreatedCallback(EntityCreatedCallback);
    RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        static_cast<OnlineRealtimeEngine*>(RealtimeEngine.get())->SetScriptLeaderReadyCallback(ScriptSystemReadyCallback);
    }

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode);
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
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create audio component
    auto* CustomComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);

    CustomComponent->SetCustomProperty("Number", csp::common::ReplicatedValue(int64_t(0)));
    CustomComponent->SetCustomProperty("NumberChanged", csp::common::ReplicatedValue(false));

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

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

    if (EngineType == csp::common::RealtimeEngineType::Online)
    {
        auto ScriptSystemIsReady = [&ScriptSystemReady]()
        {
            std::cerr << "Waiting for ScriptSystemReady" << std::endl;
            return (ScriptSystemReady == true);
        };

        ASSERT_EQ(ResponseWaiter::WaitFor(ScriptSystemIsReady, std::chrono::seconds(5)), true);
    }

    CreatedObject->GetScript().SetScriptSource(ScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    ASSERT_EQ(CustomComponent->GetCustomProperty("testFloat").GetFloat(), 1.234f);
    ASSERT_EQ(CustomComponent->GetCustomProperty("testInt").GetInt(), 1234);
    ASSERT_EQ(CustomComponent->GetCustomProperty("Number").GetInt(), 0);
    ASSERT_FALSE(CustomComponent->GetCustomProperty("NumberChanged").GetBool());

    CustomComponent->SetCustomProperty("Number", csp::common::ReplicatedValue(int64_t(100)));

    ASSERT_EQ(CustomComponent->GetCustomProperty("Number").GetInt(), 100);
    ASSERT_TRUE(CustomComponent->GetCustomProperty("NumberChanged").GetBool());

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

TEST_P(MultipleScriptComponent, MultipleScriptComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    csp::common::String UserAvatarId = "MyCoolAvatar";

    AvatarState UserState = AvatarState::Idle;
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    const auto LoginState = UserSystem->GetLoginState();

    auto [Avatar] = AWAIT(
        RealtimeEngine.get(), CreateAvatar, UserName, LoginState.UserId, UserTransform, IsVisible, UserState, UserAvatarId, UserAvatarPlayMode);
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
    auto [SpaceEntity] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Attempt to add 2 script components
    SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);
    SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);

    SpaceEntity->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Only 1 script component should be on the object
    EXPECT_EQ(SpaceEntity->GetComponents()->Size(), 1);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete MultiplayerConnection

    // Log out
    LogOut(UserSystem);
}

// This test will be fixed and re-instated as part of OF-1539
TEST_P(ModifyExistingScript, ModifyExistingScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::common::RealtimeEngineType EngineType = GetParam();
    std::unique_ptr<csp::common::IRealtimeEngine> RealtimeEngine { SystemsManager.MakeRealtimeEngine(EngineType) };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

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

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // For our first phase of this script test, we simply an object with a script component, and assign it
    // a valid script, tell CHS about it and then bail out of the connection
    {
        const csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [Object] = Awaitable(&csp::common::IRealtimeEngine::CreateEntity, static_cast<csp::common::IRealtimeEngine*>(RealtimeEngine.get()),
            ObjectName, ObjectTransform, csp::common::Optional<uint64_t>())
                            .Await();
        ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

        ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
        Object->QueueUpdate();

        RealtimeEngine->ProcessPendingEntityOperations();
    }

    //------------------------------------------------------------
    // For our second phase of the test, we attempt to take an entity that already exists (we created it in phase 1), modify the script source and
    // re-invoke the script

    // interesting part of phase 2 begins!
    {
        csp::multiplayer::SpaceEntity* Object = RealtimeEngine->GetEntityByIndex(0);
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
        Object->GetScript().Invoke();

        const bool ScriptHasErrors = Object->GetScript().HasError();
        EXPECT_FALSE(ScriptHasErrors);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
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