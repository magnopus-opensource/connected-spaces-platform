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

#ifndef SKIP_INTERNAL_TESTS

	#include "CSP/CSPFoundation.h"
	#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
	#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
	#include "CSP/Multiplayer/MultiPlayerConnection.h"
	#include "CSP/Multiplayer/SpaceEntity.h"
	#include "CSP/Systems/Spaces/Space.h"
	#include "CSP/Systems/SystemsManager.h"
	#include "Multiplayer/SignalRMsgPackEntitySerialiser.h"
	#include "Multiplayer/SpaceEntityKeys.h"
	#include "PublicAPITests/SpaceSystemTestHelpers.h"
	#include "PublicAPITests/UserSystemTestHelpers.h"
	#include "TestHelpers.h"

	#include "gtest/gtest.h"
	#include <Awaitable.h>


using namespace csp::common;
using namespace csp::multiplayer;
using namespace csp::multiplayer::msgpack_typeids;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityUserSignalRSerialisationTest)
{
	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Set up multiplayer connection
	auto* Connection   = new csp::multiplayer::MultiplayerConnection(Space.Id);
	auto* EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Connect and initialise
	{
		auto [Error] = AWAIT(Connection, Connect);

		ASSERT_EQ(Error, ErrorCode::None);

		std::tie(Error) = AWAIT(Connection, InitialiseConnection);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	const csp::common::String& UserName		  = "MyUser";
	const SpaceTransform& UserTransform		  = {Vector3 {1.2f, 2.34f, 3.45f}, Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, Vector3 {1, 1, 1}};
	AvatarState UserAvatarState				  = AvatarState::Idle;
	const csp::common::String& UserAvatarId	  = "42";
	AvatarPlayMode UserAvatarPlayMode		  = AvatarPlayMode::Default;
	LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

	auto [User] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

	SignalRMsgPackEntitySerialiser Serialiser;

	auto* AvatarComponent = (AvatarSpaceComponent*) User->AddComponent(ComponentType::AvatarData);

	AvatarComponent->SetAvatarId("MyCoolAvatar");
	AvatarComponent->SetState(AvatarState::Flying);
	AvatarComponent->SetUserId("0123456789ABCDEF");

	User->Serialise(Serialiser);
	auto SerialisedUser = Serialiser.Finalise();

	EXPECT_TRUE(SerialisedUser.is_array() && SerialisedUser.as_array().size() == 7);

	auto& Array = SerialisedUser.as_array();

	EXPECT_TRUE(Array[0].is_uinteger() && Array[0].as_uinteger() == User->GetId());
	EXPECT_TRUE(Array[1].is_uinteger() && Array[1].as_uinteger() == (int) SpaceEntityType::Avatar);
	EXPECT_TRUE(Array[2].is_bool() && !Array[2].as_bool()); // IsTransferable
	EXPECT_TRUE(Array[3].is_bool() && !Array[3].as_bool()); // IsPersistant
	EXPECT_TRUE(Array[4].is_uinteger() && Array[4].as_uinteger() == User->GetOwnerId());
	EXPECT_TRUE(Array[5].is_null());										   // ParentId
	EXPECT_TRUE(Array[6].is_uint_map() && Array[6].as_uint_map().size() == 8); // Components

	auto& Components = Array[6].as_uint_map();

	for (auto& [Key, Component] : Components)
	{
		EXPECT_TRUE(Component.is_array() && Component.as_array().size() == 2);

		auto& ComponentArray = Component.as_array();

		EXPECT_TRUE(ComponentArray[0].is_uinteger());
		EXPECT_TRUE(ComponentArray[1].is_array() && ComponentArray[1].as_array().size() == 1);
		auto& ComponentValue = ComponentArray[1].as_array()[0];

		switch (ComponentArray[0].as_uinteger())
		{
			case ItemComponentData::UINT8_ARRAY:
			{
				EXPECT_TRUE(ComponentValue.is_raw());

				break;
			}
			case ItemComponentData::FLOAT_ARRAY:
			{
				EXPECT_TRUE(ComponentValue.is_array() && ComponentValue.as_array()[0].is_double());

				auto& Array = ComponentValue.as_array();

				if (Key == COMPONENT_KEY_VIEW_POSITION)
				{

					EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == User->GetPosition().X && Array[1].as_double() == User->GetPosition().Y
								&& Array[2].as_double() == User->GetPosition().Z);
				}
				else if (Key == COMPONENT_KEY_VIEW_ROTATION)
				{
					EXPECT_TRUE(Array.size() == 4 && Array[0].as_double() == User->GetRotation().X && Array[1].as_double() == User->GetRotation().Y
								&& Array[2].as_double() == User->GetRotation().Z && Array[3].as_double() == User->GetRotation().W);
				}
				else if (Key == COMPONENT_KEY_VIEW_SCALE)
				{
					EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == User->GetScale().X && Array[1].as_double() == User->GetScale().Y
								&& Array[2].as_double() == User->GetScale().Z);
				}
				else
				{
					FAIL();
				}

				break;
			}
			case ItemComponentData::STRING:
			{
				EXPECT_TRUE(ComponentValue.is_string());

				break;
			}
			case ItemComponentData::INT64:
			{
				EXPECT_TRUE(ComponentValue.is_integer());
				break;
			}
			case ItemComponentData::UINT16_DICTIONARY:
			{
				EXPECT_TRUE(ComponentValue.is_uint_map());
				break;
			}
			default:
				FAIL();
		}
	}

	{
		auto [Error] = AWAIT(Connection, Disconnect);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// Delete MultiplayerConnection
	delete Connection;

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityUserSignalRDeserialisationTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Set up multiplayer connection
	auto* Connection   = new csp::multiplayer::MultiplayerConnection(Space.Id);
	auto* EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Connect and initialise
	{
		auto [Error] = AWAIT(Connection, Connect);

		ASSERT_EQ(Error, ErrorCode::None);

		std::tie(Error) = AWAIT(Connection, InitialiseConnection);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	const csp::common::String& UserName		  = "MyUser";
	const SpaceTransform& UserTransform		  = {Vector3 {1.2f, 2.34f, 3.45f}, Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, Vector3 {1, 1, 1}};
	AvatarState UserAvatarState				  = AvatarState::Idle;
	const csp::common::String& UserAvatarId	  = "42";
	AvatarPlayMode UserAvatarPlayMode		  = AvatarPlayMode::Default;
	LocomotionModel UserAvatarLocomotionModel = LocomotionModel::Grounded;

	auto [User] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

	SignalRMsgPackEntitySerialiser Serialiser;

	auto* AvatarComponent = (AvatarSpaceComponent*) User->AddComponent(ComponentType::AvatarData);

	AvatarComponent->SetAvatarId("MyCoolAvatar");
	AvatarComponent->SetState(AvatarState::Flying);
	AvatarComponent->SetUserId("0123456789ABCDEF");

	auto AvatarComponentKey = AvatarComponent->GetId();

	User->Serialise(Serialiser);
	auto SerialisedUser = Serialiser.Finalise();

	SignalRMsgPackEntityDeserialiser Deserialiser(SerialisedUser);
	auto DeserialisedUser = CSP_NEW SpaceEntity();
	DeserialisedUser->Deserialise(Deserialiser);

	EXPECT_EQ(DeserialisedUser->GetId(), User->GetId());
	EXPECT_EQ(DeserialisedUser->GetName(), User->GetName());
	EXPECT_EQ(DeserialisedUser->GetPosition(), User->GetPosition());
	EXPECT_EQ(DeserialisedUser->GetRotation(), User->GetRotation());
	EXPECT_EQ(DeserialisedUser->GetOwnerId(), User->GetOwnerId());

	auto* DeserialisedComponentsMap = DeserialisedUser->GetComponents();

	EXPECT_TRUE(DeserialisedComponentsMap->HasKey(AvatarComponentKey));
	EXPECT_EQ(DeserialisedComponentsMap->Size(), AvatarComponentKey);

	auto* DeserialisedComponent = DeserialisedUser->GetComponent(1);

	EXPECT_EQ(DeserialisedComponent->GetComponentType(), ComponentType::AvatarData);

	auto* DeserialisedAvatarComponent = (AvatarSpaceComponent*) DeserialisedComponent;

	EXPECT_EQ(DeserialisedAvatarComponent->GetAvatarId(), AvatarComponent->GetAvatarId());
	EXPECT_EQ(DeserialisedAvatarComponent->GetState(), AvatarComponent->GetState());
	EXPECT_EQ(DeserialisedAvatarComponent->GetUserId(), AvatarComponent->GetUserId());

	// CSP_DELETE(DeserialisedUser);
	// CSP_DELETE(User);

	{
		auto [Error] = AWAIT(Connection, Disconnect);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// Delete MultiplayerConnection
	delete Connection;

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityObjectSignalRSerialisationTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Set up multiplayer connection
	auto* Connection   = new csp::multiplayer::MultiplayerConnection(Space.Id);
	auto* EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Connect and initialise
	{
		auto [Error] = AWAIT(Connection, Connect);

		ASSERT_EQ(Error, ErrorCode::None);

		std::tie(Error) = AWAIT(Connection, InitialiseConnection);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	csp::common::String ObjectName = "MyObject";
	SpaceTransform ObjectTransform = {Vector3 {1.2f, 2.34f, 3.45f}, Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, Vector3 {1, 1, 1}};

	auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	SignalRMsgPackEntitySerialiser Serialiser;

	const csp::common::String ModelAssetId = "NotARealId";

	auto* StaticModelComponent	 = (StaticModelSpaceComponent*) Object->AddComponent(ComponentType::StaticModel);
	auto StaticModelComponentKey = StaticModelComponent->GetId();
	StaticModelComponent->SetExternalResourceAssetId(ModelAssetId);

	Object->Serialise(Serialiser);
	auto SerialisedObject = Serialiser.Finalise();

	EXPECT_TRUE(SerialisedObject.is_array() && SerialisedObject.as_array().size() == 7);

	auto& Array = SerialisedObject.as_array();

	EXPECT_TRUE(Array[0].is_uinteger() && Array[0].as_uinteger() == Object->GetId());
	EXPECT_TRUE(Array[1].is_uinteger() && Array[1].as_uinteger() == 2); // PrefabId
	EXPECT_TRUE(Array[2].is_bool() && Array[2].as_bool());				// IsTransferable
	EXPECT_TRUE(Array[3].is_bool());									// IsPersistant
	EXPECT_TRUE(Array[4].is_uinteger() && Array[4].as_uinteger() == Object->GetOwnerId());
	EXPECT_TRUE(Array[5].is_null());										   // ParentId
	EXPECT_TRUE(Array[6].is_uint_map() && Array[6].as_uint_map().size() >= 4); // Components

	auto& Components = Array[6].as_uint_map();

	for (auto& [Key, Component] : Components)
	{
		EXPECT_TRUE(Component.is_array() && Component.as_array().size() == 2);

		auto& ComponentArray = Component.as_array();

		EXPECT_TRUE(ComponentArray[0].is_uinteger());
		EXPECT_TRUE(ComponentArray[1].is_array() && ComponentArray[1].as_array().size() == 1);
		auto& ComponentValue = ComponentArray[1].as_array()[0];

		switch (ComponentArray[0].as_uinteger())
		{
			case ItemComponentData::UINT8_ARRAY:
			{
				EXPECT_TRUE(ComponentValue.is_raw());

				break;
			}
			case ItemComponentData::FLOAT_ARRAY:
			{
				EXPECT_TRUE(ComponentValue.is_array() && ComponentValue.as_array()[0].is_double());

				auto& Array = ComponentValue.as_array();

				if (Key == COMPONENT_KEY_VIEW_POSITION)
				{
					EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == Object->GetPosition().X
								&& Array[1].as_double() == Object->GetPosition().Y && Array[2].as_double() == Object->GetPosition().Z);
				}
				else if (Key == COMPONENT_KEY_VIEW_ROTATION)
				{
					EXPECT_TRUE(Array.size() == 4 && Array[0].as_double() == Object->GetRotation().X
								&& Array[1].as_double() == Object->GetRotation().Y && Array[2].as_double() == Object->GetRotation().Z
								&& Array[3].as_double() == Object->GetRotation().W);
				}
				else if (Key == COMPONENT_KEY_VIEW_SCALE)
				{
					EXPECT_TRUE(Array.size() == 3 && Array[0].as_double() == Object->GetScale().X && Array[1].as_double() == Object->GetScale().Y
								&& Array[2].as_double() == Object->GetScale().Z);
				}
				else
				{
					FAIL();
				}

				break;
			}
			case ItemComponentData::STRING:
			{
				EXPECT_TRUE(ComponentValue.is_string());

				break;
			}
			case ItemComponentData::INT64:
			{
				EXPECT_TRUE(ComponentValue.is_integer());
				break;
			}
			case ItemComponentData::UINT16_DICTIONARY:
			{
				EXPECT_TRUE(ComponentValue.is_uint_map());
				break;
			}
			default:
				FAIL();
		}
	}

	{
		auto [Error] = AWAIT(Connection, Disconnect);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// Delete MultiplayerConnection
	delete Connection;

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}

CSP_INTERNAL_TEST(CSPEngine, SerialisationTests, SpaceEntityObjectSignalRDeserialisationTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Set up multiplayer connection
	auto* Connection   = new csp::multiplayer::MultiplayerConnection(Space.Id);
	auto* EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Connect and initialise
	{
		auto [Error] = AWAIT(Connection, Connect);

		ASSERT_EQ(Error, ErrorCode::None);

		std::tie(Error) = AWAIT(Connection, InitialiseConnection);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	csp::common::String ObjectName = "MyObject";
	SpaceTransform ObjectTransform = {Vector3 {1.2f, 2.34f, 3.45f}, Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, Vector3 {1, 1, 1}};

	auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

	SignalRMsgPackEntitySerialiser Serialiser;

	const csp::common::String ModelAssetId = "NotARealId";

	auto* StaticModelComponent	 = (StaticModelSpaceComponent*) Object->AddComponent(ComponentType::StaticModel);
	auto StaticModelComponentKey = StaticModelComponent->GetId();
	StaticModelComponent->SetExternalResourceAssetId(ModelAssetId);

	Object->Serialise(Serialiser);
	auto SerialisedObject = Serialiser.Finalise();

	SignalRMsgPackEntityDeserialiser Deserialiser(SerialisedObject);
	auto DeserialisedObject = CSP_NEW SpaceEntity();
	DeserialisedObject->Deserialise(Deserialiser);

	EXPECT_EQ(DeserialisedObject->GetId(), Object->GetId());
	EXPECT_EQ(DeserialisedObject->GetIsTransient(), Object->GetIsTransient());
	EXPECT_EQ(DeserialisedObject->GetName(), Object->GetName());
	EXPECT_EQ(DeserialisedObject->GetPosition(), Object->GetPosition());
	EXPECT_EQ(DeserialisedObject->GetRotation(), Object->GetRotation());
	EXPECT_EQ(DeserialisedObject->GetScale(), Object->GetScale());
	EXPECT_EQ(DeserialisedObject->GetOwnerId(), Object->GetOwnerId());

	EXPECT_EQ(DeserialisedObject->GetComponents()->Size(), 1);

	auto* DeserialisedComponent = (StaticModelSpaceComponent*) DeserialisedObject->GetComponent(COMPONENT_KEY_START_COMPONENTS);

	EXPECT_EQ(DeserialisedComponent->GetComponentType(), ComponentType::StaticModel);

	EXPECT_EQ(DeserialisedComponent->GetProperties()->Size(), ((size_t) StaticModelPropertyKeys::Num) - 1);
	EXPECT_EQ(DeserialisedComponent->GetExternalResourceAssetId(), "NotARealId");
	EXPECT_EQ(DeserialisedComponent->GetIsVisible(), true);

	CSP_DELETE(DeserialisedObject);

	{
		auto [Error] = AWAIT(Connection, Disconnect);

		ASSERT_EQ(Error, ErrorCode::None);
	}

	// Delete MultiplayerConnection
	delete Connection;

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}

} // namespace

#endif