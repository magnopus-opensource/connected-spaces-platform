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

#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/Multiplayer/Components/PropertyAnimationSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"


using namespace csp;
using namespace csp::common;
using namespace csp::multiplayer;


namespace
{

bool RequestPredicate(const systems::ResultBase& Result)
{
	return Result.GetResultCode() != systems::EResultCode::InProgress;
}


#if RUN_ALL_UNIT_TESTS || RUN_PROPERTYANIMATIONCOMPONENT_TESTS || RUN_PROPERTYANIMATIONCOMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ImageTests, ImageComponentTest)
{
	SetRandSeed();

	auto& SystemsManager = systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
	const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	{
		auto [Result] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

		EXPECT_EQ(Result.GetResultCode(), systems::EResultCode::Success);
	}

	// Set up multiplayer connection
	auto* Connection   = new MultiplayerConnection(Space.Id);
	auto* EntitySystem = Connection->GetSpaceEntitySystem();

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Connect and initialise
	{
		auto [Result] = AWAIT(Connection, Connect);

		ASSERT_EQ(Result, ErrorCode::None);

		std::tie(Result) = AWAIT(Connection, InitialiseConnection);

		ASSERT_EQ(Result, ErrorCode::None);
	}

	const String ObjectName				 = "Object 1";
	const SpaceTransform ObjectTransform = {Vector3::Zero(), Vector4::Identity(), Vector3::One()};

	SpaceEntity* Entity;

	// Create entity
	{
		auto [Result] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

		EXPECT_NE(Result, nullptr);

		Entity = Result;
	}

	// Create property animation component
	auto* Component = (PropertyAnimationSpaceComponent*) Entity->AddComponent(ComponentType::PropertyAnimation);
	Entity->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	// Verify component was created
	const auto* Components = Entity->GetComponents();

	EXPECT_EQ(Components->Size(), 1);

	const auto* FirstComponent = Components->operator[](0);

	EXPECT_EQ(FirstComponent->GetComponentType(), ComponentType::PropertyAnimation);

	// Update and validate component
	Component->SetName("MyCoolComponent");
	Component->SetLength(2.0f);
	Component->SetIsPlaying(true);

	List<PropertyAnimationTrack> Tracks;
	PropertyAnimationTrack Track;

	Track.PropertyName		= "position";
	Track.InterpolationMode = PropertyAnimationTrackInterpolationMode::Linear;

	List<PropertyAnimationKey> Keys;
	PropertyAnimationKey Key;

	Key.Time  = 0.5f;
	Key.Value = Vector3(1, 2, 3);

	Keys.Append(Key);
	Track.Keys = Keys;
	Tracks.Append(Track);
	Component->SetTracks(Tracks);

	Entity->QueueUpdate();
	EntitySystem->ProcessPendingEntityOperations();

	auto* StoredComponent = (PropertyAnimationSpaceComponent*) Entity->GetComponent(Component->GetId());

	EXPECT_EQ(StoredComponent->GetName(), "MyCoolComponent");
	EXPECT_EQ(StoredComponent->GetLength(), 2.0f);
	EXPECT_TRUE(StoredComponent->GetIsPlaying());

	const auto StoredTracks = StoredComponent->GetTracks();

	EXPECT_EQ(StoredTracks.Size(), 1);

	const auto& StoredTrack = StoredTracks[0];

	EXPECT_EQ(StoredTrack.PropertyName, "position");
	EXPECT_EQ(StoredTrack.InterpolationMode, PropertyAnimationTrackInterpolationMode::Linear);

	const auto& StoredKeys = StoredTrack.Keys;

	EXPECT_EQ(StoredKeys.Size(), 1);

	const auto& StoredKey = StoredKeys[0];

	EXPECT_EQ(StoredKey.Time, 0.5f);
	EXPECT_EQ(StoredKey.Value.GetReplicatedValueType(), ReplicatedValueType::Vector3);
	EXPECT_EQ(StoredKey.Value.GetVector3(), Vector3(1, 2, 3));

	AWAIT(Connection, Disconnect);
	delete Connection;

	SpaceSystem->ExitSpace();

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

} // namespace