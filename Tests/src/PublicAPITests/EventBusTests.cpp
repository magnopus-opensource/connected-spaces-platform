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
#include "AssetSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "filesystem.hpp"
#include "signalrclient/signalr_value.h"

#include "gtest/gtest.h"
#include <CSP/Multiplayer/Components/ImageSpaceComponent.h>
#include <CSP/Multiplayer/Components/LightSpaceComponent.h>
#include <chrono>
#include <filesystem>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

void InitialiseTestingConnection();
// void OnConnect();
// void OnDisconnect(bool ok);
// void OnUserCreated(SpaceEntity* InUser, SpaceEntitySystem* EntitySystem);

std::atomic_bool IsTestComplete;
std::atomic_bool IsDisconnected;
std::atomic_bool IsReadyForUpdate;
SpaceEntity* TestUser;

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit	= 20000;
const int NumberOfEntityUpdateTicks = 5;
int ReceivedEntityUpdatesCount;

bool EventSent	   = false;
bool EventReceived = false;

ReplicatedValue ObjectFloatProperty;
ReplicatedValue ObjectBoolProperty;
ReplicatedValue ObjectIntProperty;
ReplicatedValue ObjectStringProperty;

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

void InitialiseTestingConnection()
{
	IsTestComplete	 = false;
	IsDisconnected	 = false;
	IsReadyForUpdate = false;
	TestUser		 = nullptr;

	WaitForTestTimeoutCountMs  = 0;
	ReceivedEntityUpdatesCount = 0;


	EventSent	  = false;
	EventReceived = false;

	ObjectFloatProperty	 = ReplicatedValue(2.3f);
	ObjectBoolProperty	 = ReplicatedValue(true);
	ObjectIntProperty	 = ReplicatedValue(static_cast<int64_t>(42));
	ObjectStringProperty = "My replicated string";
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_EVENTBUS_TESTS || RUN_EVENTBUS_EVENT_EMPTY_TEST
CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventEmptyTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EventBus		 = SystemsManager.GetEventBus();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	EventBus->ListenEvent("TestEvent",
						  [](bool ok, csp::common::Array<ReplicatedValue> Data)
						  {
							  EXPECT_TRUE(ok);

							  std::cerr << "Test Event Received " << ok << std::endl;
						  });

	EventBus->ListenEvent("TestEvent",
						  [](bool ok, csp::common::Array<ReplicatedValue> Data)
						  {
							  EXPECT_TRUE(ok);

							  EventReceived = true;

							  if (EventSent)
							  {
								  IsTestComplete = true;
							  }

							  std::cerr << "Second Test Event Received " << ok << std::endl;
						  });

	Connection->SendNetworkEventToClient("TestEvent",
										 {},
										 Connection->GetClientId(),
										 [](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);

											 EventSent = true;

											 if (EventReceived)
											 {
												 IsTestComplete = true;
											 }

											 std::cerr << "Test Event Sent " << (Error == ErrorCode::None ? "true" : "false") << std::endl;
										 });

	while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
	{
		std::this_thread::sleep_for(50ms);
		WaitForTestTimeoutCountMs += 50;
	}

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTBUS_TESTS || RUN_EVENTBUS_EVENT_MULTITYPE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventMultiTypeTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EventBus		 = SystemsManager.GetEventBus();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

	InitialiseTestingConnection();

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	EventBus->ListenEvent("MultiTypeEvent",
						  [](bool ok, csp::common::Array<ReplicatedValue> Data)
						  {
							  EXPECT_TRUE(ok);

							  std::cerr << "Multi Type Event Received " << ok << "  Payload: " << std::endl;

							  for (int i = 0; i < Data.Size(); ++i)
							  {
								  if (Data[i].GetReplicatedValueType() == ReplicatedValueType::Boolean)
								  {
									  printf("%s\n", Data[i].GetBool() ? "true" : "false");
								  }
								  else if (Data[i].GetReplicatedValueType() == ReplicatedValueType::Integer)
								  {
									  printf("%lli\n", Data[i].GetInt());
								  }
								  else if (Data[i].GetReplicatedValueType() == ReplicatedValueType::Float)
								  {
									  printf("%f\n", Data[i].GetFloat());
								  }
							  }

							  EventReceived = true;

							  if (EventSent)
							  {
								  IsTestComplete = true;
							  }
						  });

	ReplicatedValue EventInt((int64_t) -1);
	ReplicatedValue EventFloat(1234.567890f);

	Connection->SendNetworkEventToClient("MultiTypeEvent",
										 {EventInt, EventFloat},
										 Connection->GetClientId(),
										 [EventInt, EventFloat](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);

											 EventSent = true;

											 if (EventReceived)
											 {
												 IsTestComplete = true;
											 }

											 printf("%lli, %f, \n", EventInt.GetInt(), EventFloat.GetFloat());
										 });

	while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
	{
		std::this_thread::sleep_for(50ms);
		WaitForTestTimeoutCountMs += 50;
	}

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif