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
#include <CSP/Multiplayer/Components/AudioSpaceComponent.h>
#include <CSP/Multiplayer/Components/ConversationSpaceComponent.h>
#include <CSP/Multiplayer/Components/CustomSpaceComponent.h>
#include <CSP/Multiplayer/Components/FogSpaceComponent.h>
#include <CSP/Multiplayer/Components/ImageSpaceComponent.h>
#include <CSP/Multiplayer/Components/PortalSpaceComponent.h>
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

bool RequestPredicate(const csp::services::ResultBase& Result)
{
	return Result.GetResultCode() != csp::services::EResultCode::InProgress;
}

void OnConnect()
{
	csp::common::String UserName = "Player 1";
	SpaceTransform UserTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};
	csp::common::String UserAvatarId = "MyCoolAvatar";

	AvatarState UserState			  = AvatarState::Idle;
	AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

	EntitySystem->CreateAvatar(UserName,
							   UserTransform,
							   UserState,
							   UserAvatarId,
							   UserAvatarPlayMode,
							   [](SpaceEntity* NewAvatar)
							   {
								   EXPECT_NE(NewAvatar, nullptr);

								   std::cerr << "CreateAvatar Local Callback" << std::endl;

								   EXPECT_EQ(NewAvatar->GetEntityType(), SpaceEntityType::Avatar);

								   if (NewAvatar->GetEntityType() == SpaceEntityType::Avatar)
								   {
									   OnUserCreated(NewAvatar);
								   }
							   });
}

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
	auto& ScriptSystem	 = *SystemsManager.GetScriptSystem();

	std::string TestMessage;

	ScriptSystem.Initialise();

	auto Fn = [&TestMessage](const char* Str)
	{
		TestMessage = Str;
		FOUNDATION_LOG_MSG(csp::systems::LogLevel::Log, Str);
		std::cout << Str << "\n";
	};

	constexpr int ContextId = 0;

	ScriptSystem.CreateContext(ContextId);

	qjs::Context* Context		 = (qjs::Context*) ScriptSystem.GetContext(ContextId);
	qjs::Context::Module* Module = (qjs::Context::Module*) ScriptSystem.GetModule(ContextId, "CSPTest");

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

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
		EntitySystem->SetEntityCreatedCallback(
			[](SpaceEntity* Entity)
			{
			});

		const csp::common::String ObjectName  = "Object 1";
		SpaceTransform ObjectTransform		  = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
		auto [Object]						  = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);
		ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

		ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
		Object->GetScript()->Invoke();

		const bool ScriptHasErrors = Object->GetScript()->HasError();
		EXPECT_FALSE(ScriptHasErrors);

		Object->QueueUpdate();

		EntitySystem->ProcessPendingEntityOperations();
	}

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	std::atomic_bool ScriptSystemReady = false;

	auto EntityCreatedCallback = [](SpaceEntity* Entity)
	{
		std::cerr << "EntityCreatedCallback called" << std::endl;
	};

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

	SpaceSystem->SetEntityCreatedCallback(EntityCreatedCallback);
	SpaceSystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
	SpaceSystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();
	OnConnect();

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
		EntitySystem->SetEntityCreatedCallback(
			[](SpaceEntity* Entity)
			{
			});

		const csp::common::String ObjectName = "Object 1";
		SpaceTransform ObjectTransform		 = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
		auto [Object]						 = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

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

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	csp::common::String UserName = "Player 1";
	SpaceTransform UserTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};
	AvatarState UserAvatarState		  = AvatarState::Idle;
	csp::common::String UserAvatarId  = "MyCoolAvatar";
	AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

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

	// Delete MultiplayerConnection
	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	csp::common::String UserName = "Player 1";
	SpaceTransform UserTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};
	AvatarState UserAvatarState		  = AvatarState::Idle;
	csp::common::String UserAvatarId  = "MyCoolAvatar";
	AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

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

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_PORTAL_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, PortalScriptInterfaceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	// Create object to represent the portal
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Create portal component
	auto* PortalComponent = (PortalSpaceComponent*) CreatedObject->AddComponent(ComponentType::Portal);

	auto InitialPosition = csp::common::Vector3 {1.1f, 2.2f, 3.3f};
	PortalComponent->SetSpaceId("initialTestSpaceId");
	PortalComponent->SetIsEnabled(false);
	PortalComponent->SetPosition(InitialPosition);
	PortalComponent->SetRadius(123.123f);

	CreatedObject->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	EXPECT_EQ(PortalComponent->GetSpaceId(), "initialTestSpaceId");
	EXPECT_EQ(PortalComponent->GetIsEnabled(), false);
	EXPECT_FLOAT_EQ(PortalComponent->GetPosition().X, InitialPosition.X);
	EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Y, InitialPosition.Y);
	EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Z, InitialPosition.Z);
	EXPECT_EQ(PortalComponent->GetRadius(), 123.123f);

	// Setup script
	std::string PortalScriptText = R"xx(
		var portal = ThisEntity.getPortalComponents()[0];
		portal.spaceId = "secondTestSpaceId";
		portal.isEnabled = true;
		portal.position = [4.4, 5.5, 6.6];
		portal.radius = 456.456;
    )xx";

	CreatedObject->GetScript()->SetScriptSource(PortalScriptText.c_str());
	CreatedObject->GetScript()->Invoke();

	EntitySystem->ProcessPendingEntityOperations();

	EXPECT_EQ(PortalComponent->GetSpaceId(), "secondTestSpaceId");
	EXPECT_EQ(PortalComponent->GetIsEnabled(), true);
	EXPECT_FLOAT_EQ(PortalComponent->GetPosition().X, 4.4f);
	EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Y, 5.5f);
	EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Z, 6.6f);
	EXPECT_FLOAT_EQ(PortalComponent->GetRadius(), 456.456f);

	// Cleanup
	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete MultiplayerConnection
	delete Connection;

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

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
	SpaceTransform ObjectTransform		 = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};

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

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

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
	SpaceTransform ObjectTransform		 = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};

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

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	std::atomic_bool ScriptSystemReady = false;

	auto EntityCreatedCallback = [](SpaceEntity* Entity)
	{
		std::cerr << "EntityCreatedCallback called" << std::endl;
	};

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

	SpaceSystem->SetEntityCreatedCallback(EntityCreatedCallback);
	SpaceSystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
	SpaceSystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();
	OnConnect();

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
	SpaceTransform ObjectTransform		 = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};

	auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	bool PatchPending = true;
	CreatedObject->SetPatchSentCallback(
		[&PatchPending](bool ok)
		{
			PatchPending = false;
		});

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

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_AUDIO_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, AudioScriptInterfaceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	// Create object to represent the audio
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Create audio component
	auto* AudioComponent = (AudioSpaceComponent*) CreatedObject->AddComponent(ComponentType::Audio);

	CreatedObject->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	// Setup script
	std::string AudioScriptText = R"xx(
	
		const assetId			= "TEST_ASSET_ID";
		const assetCollectionId = "TEST_COLLECTION_ID";

		var audio = ThisEntity.getAudioComponents()[0];
		audio.position = [1,1,1];
		audio.playbackState = 2;
		audio.audioType = 1;
		audio.audioAssetId = assetId;
		audio.assetCollectionId = assetCollectionId;
		audio.attenuationRadius = 100;
		audio.isLoopPlayback = true;
		audio.timeSincePlay = 1;
		audio.volume = 0.75;
    )xx";

	CreatedObject->GetScript()->SetScriptSource(AudioScriptText.c_str());
	CreatedObject->GetScript()->Invoke();

	EntitySystem->ProcessPendingEntityOperations();

	// Ensure values are set correctly
	csp::common::String AssetId			  = "TEST_ASSET_ID";
	csp::common::String AssetCollectionId = "TEST_COLLECTION_ID";

	EXPECT_EQ(AudioComponent->GetPosition(), csp::common::Vector3::One());
	EXPECT_EQ(AudioComponent->GetPlaybackState(), AudioPlaybackState::Play);
	EXPECT_EQ(AudioComponent->GetAudioType(), AudioType::Spatial);
	EXPECT_EQ(AudioComponent->GetAudioAssetId(), AssetId);
	EXPECT_EQ(AudioComponent->GetAssetCollectionId(), AssetCollectionId);
	EXPECT_EQ(AudioComponent->GetAttenuationRadius(), 100.f);
	EXPECT_EQ(AudioComponent->GetIsLoopPlayback(), true);
	EXPECT_EQ(AudioComponent->GetTimeSincePlay(), 1.f);
	EXPECT_EQ(AudioComponent->GetVolume(), 0.75f);

	// Test invalid volume values
	AudioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 1.75;
    )xx";
	CreatedObject->GetScript()->Invoke();
	EntitySystem->ProcessPendingEntityOperations();
	EXPECT_EQ(AudioComponent->GetVolume(), 0.75f);

	AudioScriptText = R"xx(M
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = -2.75;
    )xx";
	CreatedObject->GetScript()->SetScriptSource(AudioScriptText.c_str());
	CreatedObject->GetScript()->Invoke();
	EntitySystem->ProcessPendingEntityOperations();
	EXPECT_EQ(AudioComponent->GetVolume(), 0.75f);

	// Test boundary volume values
	AudioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 1.0;
    )xx";
	CreatedObject->GetScript()->SetScriptSource(AudioScriptText.c_str());
	CreatedObject->GetScript()->Invoke();
	EntitySystem->ProcessPendingEntityOperations();
	EXPECT_EQ(AudioComponent->GetVolume(), 1.f);

	AudioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 0.0;
    )xx";
	CreatedObject->GetScript()->SetScriptSource(AudioScriptText.c_str());
	CreatedObject->GetScript()->Invoke();
	EntitySystem->ProcessPendingEntityOperations();
	EXPECT_EQ(AudioComponent->GetVolume(), 0.f);

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SPLINE_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, SplineScriptInterfaceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	// Create object to represent the spline
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Create spline component
	auto* SplineComponent							  = (SplineSpaceComponent*) CreatedObject->AddComponent(ComponentType::Spline);
	csp::common::List<csp::common::Vector3> WayPoints = {{0, 0, 0}, {0, 1000, 0}, {0, 2000, 0}, {0, 3000, 0}, {0, 4000, 0}, {0, 5000, 0}};

	CreatedObject->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	// Setup script
	const std::string SplineScriptText = R"xx(
	
		var spline = ThisEntity.getSplineComponents()[0];
		
		var waypoints = [[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
		spline.setWaypoints(waypoints);
		var positionResult = spline.getLocationAlongSpline(1);
		
    )xx";

	CreatedObject->GetScript()->SetScriptSource(SplineScriptText.c_str());
	CreatedObject->GetScript()->Invoke();

	EntitySystem->ProcessPendingEntityOperations();

	EXPECT_EQ(SplineComponent->GetWaypoints().Size(), WayPoints.Size());

	// expect final waypoint to be the same
	EXPECT_EQ(SplineComponent->GetWaypoints()[0], WayPoints[0]);

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

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
		SpaceTransform ObjectTransform		 = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
		auto [Object]						 = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

		ScriptSpaceComponent* ScriptComponent = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

		Object->QueueUpdate();
		EntitySystem->ProcessPendingEntityOperations();

		ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
		Object->GetScript()->Invoke();

		csp::CSPFoundation::Tick();

		const bool ScriptHasErrors = Object->GetScript()->HasError();
		EXPECT_FALSE(ScriptHasErrors);
	}

	// Delete MultiplayerConnection
	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	std::atomic_bool ScriptSystemReady = false;

	auto EntityCreatedCallback = [](SpaceEntity* Entity)
	{
		std::cerr << "EntityCreatedCallback called" << std::endl;
	};

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

	SpaceSystem->SetEntityCreatedCallback(EntityCreatedCallback);
	SpaceSystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
	SpaceSystem->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();
	OnConnect();

	// Create object to represent the audio
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Create audio component
	auto* CustomComponent = (CustomSpaceComponent*) CreatedObject->AddComponent(ComponentType::Custom);

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

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_IMAGE_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ImageScriptInterfaceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);

	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	// Create object to represent the image
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Create image component
	auto* ImageComponent = (ImageSpaceComponent*) CreatedObject->AddComponent(ComponentType::Image);
	// Create script component
	auto* ScriptComponent = (ScriptSpaceComponent*) CreatedObject->AddComponent(ComponentType::ScriptData);

	CreatedObject->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	EXPECT_EQ(ImageComponent->GetIsVisible(), true);
	EXPECT_EQ(ImageComponent->GetIsEmissive(), false);
	EXPECT_EQ(ImageComponent->GetDisplayMode(), DisplayMode::DoubleSided);
	EXPECT_EQ(ImageComponent->GetBillboardMode(), BillboardMode::Off);

	// Setup script
	const std::string ImageScriptText = R"xx(
	
		var image = ThisEntity.getImageComponents()[0];
		
		image.isVisible = false;
		image.isEmissive = true;
		image.displayMode = 2;
		image.billboardMode = 1;
    )xx";

	ScriptComponent->SetScriptSource(ImageScriptText.c_str());
	CreatedObject->GetScript()->Invoke();

	EntitySystem->ProcessPendingEntityOperations();

	const bool ScriptHasErrors = CreatedObject->GetScript()->HasError();
	EXPECT_FALSE(ScriptHasErrors);

	EXPECT_EQ(ImageComponent->GetIsVisible(), false);
	EXPECT_EQ(ImageComponent->GetIsEmissive(), true);
	EXPECT_EQ(ImageComponent->GetDisplayMode(), DisplayMode::DoubleSidedReversed);
	EXPECT_EQ(ImageComponent->GetBillboardMode(), BillboardMode::Billboard);

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_FOG_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, FogScriptInterfaceTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	// Create object to represent the fog
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Create fog component
	auto* FogComponent = (FogSpaceComponent*) CreatedObject->AddComponent(ComponentType::Fog);

	CreatedObject->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	// Setup script
	const std::string FogScriptText = R"xx(
		var fog = ThisEntity.getFogComponents()[0];
		fog.fogMode = 1;
		fog.position = [1, 1, 1];
		fog.rotation = [1, 1, 1, 2];
		fog.scale = [2, 2, 2];
		fog.startDistance = 1.1;
		fog.endDistance = 2.2;
		fog.color = [1, 1, 1];
		fog.density = 3.3;
		fog.heightFalloff = 4.4;
		fog.maxOpacity = 5.5;
		fog.isVolumetric = true;
    )xx";

	CreatedObject->GetScript()->SetScriptSource(FogScriptText.c_str());
	CreatedObject->GetScript()->Invoke();

	EntitySystem->ProcessPendingEntityOperations();

	EXPECT_EQ(FogComponent->GetFogMode(), FogMode::Exponential);
	EXPECT_EQ(FogComponent->GetPosition(), csp::common::Vector3::One());
	EXPECT_EQ(FogComponent->GetRotation(), csp::common::Vector4(1, 1, 1, 2));
	EXPECT_EQ(FogComponent->GetScale(), csp::common::Vector3(2, 2, 2));
	EXPECT_FLOAT_EQ(FogComponent->GetStartDistance(), 1.1f);
	EXPECT_FLOAT_EQ(FogComponent->GetEndDistance(), 2.2f);
	EXPECT_EQ(FogComponent->GetColor(), csp::common::Vector3::One());
	EXPECT_FLOAT_EQ(FogComponent->GetDensity(), 3.3f);
	EXPECT_FLOAT_EQ(FogComponent->GetHeightFalloff(), 4.4f);
	EXPECT_FLOAT_EQ(FogComponent->GetMaxOpacity(), 5.5f);
	EXPECT_TRUE(FogComponent->GetIsVolumetric());

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_CONVERSATION_COMPONENT_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ConversationComponentScriptTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	{
		auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);
		EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);
		Connection	 = EnterResult.GetConnection();
		EntitySystem = Connection->GetSpaceEntitySystem();

		EntitySystem->SetEntityCreatedCallback(
			[](SpaceEntity* Entity)
			{
			});

		// Create object to represent the conversation
		csp::common::String ObjectName = "Object 1";
		SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
		auto [CreatedObject]		   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

		// Create conversation component
		auto* ConversationComponent = (ConversationSpaceComponent*) CreatedObject->AddComponent(ComponentType::Conversation);

		SpaceTransform DefaultTransform = SpaceTransform();

		EXPECT_EQ(ConversationComponent->GetIsVisible(), true);
		EXPECT_EQ(ConversationComponent->GetIsActive(), true);

		EXPECT_EQ(ConversationComponent->GetPosition().X, DefaultTransform.Position.X);
		EXPECT_EQ(ConversationComponent->GetPosition().Y, DefaultTransform.Position.Y);
		EXPECT_EQ(ConversationComponent->GetPosition().Z, DefaultTransform.Position.Z);

		EXPECT_EQ(ConversationComponent->GetRotation().W, DefaultTransform.Rotation.W);
		EXPECT_EQ(ConversationComponent->GetRotation().X, DefaultTransform.Rotation.X);
		EXPECT_EQ(ConversationComponent->GetRotation().Y, DefaultTransform.Rotation.Y);
		EXPECT_EQ(ConversationComponent->GetRotation().Z, DefaultTransform.Rotation.Z);

		CreatedObject->QueueUpdate();
		EntitySystem->ProcessPendingEntityOperations();

		// Setup script
		std::string ConversationScriptText = R"xx(
			var conversation = ThisEntity.getConversationComponents()[0];
			conversation.isVisible = false;
			conversation.isActive = false;
			conversation.position = [1,2,3];
			conversation.rotation = [4,5,6,7];
		)xx";

		CreatedObject->GetScript()->SetScriptSource(ConversationScriptText.c_str());
		CreatedObject->GetScript()->Invoke();

		EntitySystem->ProcessPendingEntityOperations();

		EXPECT_FALSE(ConversationComponent->GetIsVisible());
		EXPECT_FALSE(ConversationComponent->GetIsActive());

		csp::common::Vector3 NewPosition(1, 2, 3);
		EXPECT_EQ(ConversationComponent->GetPosition().X, NewPosition.X);
		EXPECT_EQ(ConversationComponent->GetPosition().Y, NewPosition.Y);
		EXPECT_EQ(ConversationComponent->GetPosition().Z, NewPosition.Z);

		csp::common::Vector4 NewRotation(4, 5, 6, 7);
		EXPECT_EQ(ConversationComponent->GetRotation().W, NewRotation.W);
		EXPECT_EQ(ConversationComponent->GetRotation().X, NewRotation.X);
		EXPECT_EQ(ConversationComponent->GetRotation().Y, NewRotation.Y);
		EXPECT_EQ(ConversationComponent->GetRotation().Z, NewRotation.Z);
	};

	AWAIT(SpaceSystem, ExitSpaceAndDisconnect, Connection);

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, true);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::services::EResultCode::Success);

	Connection	 = EnterResult.GetConnection();
	EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	// Create space object
	csp::common::String ObjectName = "Object 1";
	SpaceTransform ObjectTransform = {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
	auto [SpaceEntity]			   = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	// Attempt to add 2 script components
	auto Comp1 = SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);
	auto Comp2 = SpaceEntity->AddComponent(csp::multiplayer::ComponentType::ScriptData);

	SpaceEntity->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	// Only 1 script component should be on the object
	EXPECT_EQ(SpaceEntity->GetComponents()->Size(), 1);

	// Disconnect from the SignalR server
	auto [Ok] = AWAIT(Connection, Disconnect);

	EXPECT_TRUE(Ok);

	SpaceSystem->ExitSpace();

	// Delete MultiplayerConnection
	delete Connection;

	// Log out
	LogOut(UserSystem);
}
#endif

// TODO: Re-instate test
// Removing until we have a proper fix for https://magnopus.atlassian.net/browse/OB-329
/*
#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_MODIFY_EXISTING_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ModifyExistingScriptTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto* Connection   = new csp::multiplayer::MultiplayerConnection(Space.Id);
	auto* EntitySystem = Connection->GetSpaceEntitySystem();

	// Connect to the SignalR server
	auto [Ok] = Awaitable(&MultiplayerConnection::Connect, Connection).Await();
	if(!Ok) { return; }

	std::tie(Ok) = Awaitable(&MultiplayerConnection::InitialiseConnection, Connection).Await();
	if(!Ok) { return; }

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
		SpaceTransform ObjectTransform		= {csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One()};
		auto [Object] = Awaitable(&SpaceEntitySystem::CreateObject, EntitySystem, ObjectName, ObjectTransform).Await();
		ScriptSpaceComponent* ScriptComponent	 = static_cast<ScriptSpaceComponent*>(Object->AddComponent(ComponentType::ScriptData));

		ScriptComponent->SetScriptSource(csp::common::String(ScriptText.c_str()));
		Object->QueueUpdate();

		EntitySystem->ProcessPendingEntityOperations();
	}

	// Disconnect from the SignalR server
	std::tie(Ok) = Awaitable(&MultiplayerConnection::Disconnect, Connection).Await();
	if(!Ok) { return; }

	// Delete MultiplayerConnection
	delete Connection;

	//------------------------------------------------------------
	// For our second phase of the test, we attempt to take an entity that already exists (we created it in phase 1), modify the script source and
	// re-invoke the script

	// Re-form the connection for the second phase of this test
	Connection   = new csp::multiplayer::MultiplayerConnection(Space.Id);
	EntitySystem = Connection->GetSpaceEntitySystem();

	bool EntityHasBeenRecreated = false;
	// we're gonna wanna wait till the entity is created before we can do our test
	EntitySystem->SetEntityCreatedCallback(
	[&EntityHasBeenRecreated](csp::multiplayer::SpaceEntity* Object)
	{
		EntityHasBeenRecreated = true;
	});

	// Connect to the SignalR server
	auto [Ok2] = Awaitable(&MultiplayerConnection::Connect, Connection).Await();
	if(!Ok2) { return; }

	std::tie(Ok2) = Awaitable(&MultiplayerConnection::InitialiseConnection, Connection).Await();
	if(!Ok2) { return; }

	// spin till we recreate the entity from phase 1 locally, having received it back from CHS
	while(EntityHasBeenRecreated == false) {}

	// interesting part of phase 2 begins!
	{
		csp::multiplayer::SpaceEntity* Object = EntitySystem->GetEntityByIndex(0);
		// grab the script component we created in phase 1 (we should make this kind of thing easier)
		const csp::common::Array<ComponentBase*>& Components = *Object->GetComponents()->Values();
		ScriptSpaceComponent* ScriptComponent = nullptr;
		for(size_t i = 0; Components.Size(); i++)
		{
			if(Components[i]->GetComponentType() == csp::multiplayer::ComponentType::ScriptData)
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

	// Disconnect from the SignalR server
	std::tie(Ok2) = Awaitable(&MultiplayerConnection::Disconnect, Connection).Await();
	if(!Ok2) { return; }

	// Delete MultiplayerConnection
	delete Connection;

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif
*/

} // namespace