#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/SequenceHierarchy.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Sequence/Sequence.h"
#include "Debug/Logging.h"
#include "PublicTestBase.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::multiplayer;

namespace
{

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit = 20000;

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCE_HIERARCHY_TESTS || RUN_SEQUENCE_HIERARCHY_CREATE_SEQUENCE_KEY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, CreateSequenceKeyTest)
{
	// Test root hierarchy using a null parent id
	{
		const csp::common::String SpaceId = "12345";
		const csp::common::String Key	  = csp::multiplayer::CreateSequenceKey(nullptr, SpaceId);

		EXPECT_EQ(Key, "EntityHierarchy:" + SpaceId);
	}

	// Test branch hierarchy using a parent id
	{
		const uint64_t ParentId			  = 111;
		const csp::common::String SpaceId = "12345";
		const csp::common::String Key	  = csp::multiplayer::CreateSequenceKey(ParentId, SpaceId);

		EXPECT_EQ(Key, "EntityHierarchy:" + SpaceId + ":" + "m_Id_" + std::to_string(ParentId).c_str());
	}
}
#endif

// Manual hierarchy connection test for receiving objects from another client
#if 0
const csp::common::String SpaceId	= "66b5f151175b7f15fb1b3840";
const csp::common::String SpaceName = "MV_HIERARCHY_TEST_SPACE2";

void CreateObjectTest()
{
	std::cout << "starting\n";

	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	LoadTestAccountCredentials();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId, AlternativeLoginEmail, AlternativeLoginPassword);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, SpaceId);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	bool GotAllEntities = false;

	EntitySystem->SetInitialEntitiesRetrievedCallback(
		[&](bool)
		{
			GotAllEntities = true;
		});

	while (GotAllEntities == false)
	{
		EntitySystem->ProcessPendingEntityOperations();
		std::this_thread::sleep_for(10ms);
	}

	// Create Entities
	csp::common::String ChildEntityName = "NewTestEntity";

	csp::multiplayer::SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	auto Parent = EntitySystem->FindSpaceEntity("ParentEntity");

	if (Parent == nullptr)
	{
		std::cout << "Could not find parent\n";
	}
	else
	{
		std::cout << "Found parent\n";
	}

	auto [CreatedChildEntity] = AWAIT(Parent, CreateChildEntity, ChildEntityName, ObjectTransform);

	auto Start	   = std::chrono::steady_clock::now();
	auto Current   = std::chrono::steady_clock::now();
	float TestTime = 0;

	std::cout << "about to loop\n";

	while (TestTime < 60)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();

		EntitySystem->ProcessPendingEntityOperations();
	}

	LogOut(UserSystem);
}

void SetupSpace()
{
	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	LoadTestAccountCredentials();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, SpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeLoginEmail, true, "", "");

	LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, ManualHierarchyMultipleConnectionTest)
{

	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, SpaceId);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	bool GotAllEntities = false;

	EntitySystem->SetInitialEntitiesRetrievedCallback(
		[&](bool)
		{
			GotAllEntities = true;
		});

	while (GotAllEntities == false)
	{
		EntitySystem->ProcessPendingEntityOperations();
		std::this_thread::sleep_for(10ms);
	}


	for (int i = EntitySystem->GetNumEntities() - 1; i >= 0; --i)
	{
		AWAIT(EntitySystem, DestroyEntity, EntitySystem->GetEntityByIndex(i));
	}

	bool ChildCreated = false;

	EntitySystem->SetEntityCreatedCallback(
		[&ChildCreated](SpaceEntity* Entity)
		{
			if (Entity->GetName() == "NewTestEntity")
			{
				ChildCreated = true;
			}
		});

	// Create Entities
	csp::common::String ParentEntityName = "ParentEntity";
	SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);

	while (ChildCreated == false)
	{
		EntitySystem->ProcessPendingEntityOperations();
		std::this_thread::sleep_for(10ms);
	}

	EXPECT_EQ(CreatedParentEntity->GetParentEntity(), nullptr);
	EXPECT_EQ(CreatedParentEntity->GetChildEntities()->Size(), 1);
	EXPECT_EQ((*CreatedParentEntity->GetChildEntities())[0]->GetParentEntity(), CreatedParentEntity);

	EXPECT_EQ(EntitySystem->GetRootHierarchyEntities()->Size(), 1);

	// Exit Space
	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCE_HIERARCHY_TESTS || RUN_SEQUENCE_HIERARCHY_REGISTERSEQUENCE_HIERARCHYUPDATED_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, RegisterSequenceHierarchyUpdatedTest)
{

	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE-MAG";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Enter space
	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	// Create Entities
	csp::common::String ParentEntityName = "ParentEntity";
	csp::common::String ChildEntityName	 = "ChildEntity";

	SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	auto [CreatedParentEntity] = AWAIT(EntitySystem, CreateObject, ParentEntityName, ObjectTransform);
	auto [CreatedChildEntity]  = AWAIT(EntitySystem, CreateChildEntity, ChildEntityName, CreatedParentEntity->GetId(), ObjectTransform);

	// Test creation at root
	{
		bool Called = false;

		auto ChangedCallback = [&Called](const csp::multiplayer::SequenceHierarchyChangedParams& Params)
		{
			Called = true;

			EXPECT_EQ(Params.UpdateType, ESequenceUpdateType::Create);
			EXPECT_EQ(Params.ParentId, 0);
			EXPECT_EQ(Params.IsRoot, true);
		};

		EntitySystem->SetSequenceHierarchyChangedCallback(ChangedCallback);

		// Add new here

		EXPECT_TRUE(Called);

		EntitySystem->SetSequenceHierarchyChangedCallback(nullptr);
	}

	// Test creation with parent
	{
		uint64_t ParentId = CreatedParentEntity->GetId();
		bool Called		  = false;

		auto ChangedCallback = [&Called, ParentId](const csp::multiplayer::SequenceHierarchyChangedParams& Params)
		{
			Called = true;

			EXPECT_EQ(Params.UpdateType, ESequenceUpdateType::Create);
			EXPECT_EQ(Params.ParentId, ParentId);
			EXPECT_EQ(Params.IsRoot, false);
		};

		EntitySystem->SetSequenceHierarchyChangedCallback(ChangedCallback);

		// Add new here

		EXPECT_TRUE(Called);

		EntitySystem->SetSequenceHierarchyChangedCallback(nullptr);
	}

	// Check Callback is called with deleting
	{
		bool Called = false;

		auto ChangedCallback = [&Called](const csp::multiplayer::SequenceHierarchyChangedParams& Params)
		{
			Called = true;

			EXPECT_EQ(Params.UpdateType, ESequenceUpdateType::Delete);
			EXPECT_EQ(Params.ParentId, 0);
			EXPECT_EQ(Params.IsRoot, true);
		};

		EntitySystem->SetSequenceHierarchyChangedCallback(ChangedCallback);

		// Add new here

		EXPECT_TRUE(Called);

		EntitySystem->SetSequenceHierarchyChangedCallback(nullptr);
	}

	// Cleanup

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif