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
bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

void CreateSequenceHierarchy(SpaceEntitySystem* EntitySystem,
							 csp::common::Optional<uint64_t> ParentId,
							 const csp::common::Array<uint64_t>& HierarchyItemIds,
							 csp::multiplayer::SequenceHierarchy& OutSequenceHierarchy,
							 csp::systems::EResultCode ExpectedResultCode				   = csp::systems::EResultCode::Success,
							 csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::multiplayer::SpaceEntitySystem::CreateSequenceHierarchy, EntitySystem, ParentId, HierarchyItemIds).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		const csp::multiplayer::SequenceHierarchy& SequenceHierarchy = Result.GetSequenceHierarchy();

		EXPECT_EQ(SequenceHierarchy.HasParent(), ParentId.HasValue());

		if (SequenceHierarchy.HasParent() && ParentId.HasValue())
		{
			EXPECT_EQ(SequenceHierarchy.ParentId, *ParentId);
		}

		EXPECT_EQ(SequenceHierarchy.Ids.Size(), HierarchyItemIds.Size());

		for (int i = 0; i < SequenceHierarchy.Ids.Size(); ++i)
		{
			EXPECT_EQ(SequenceHierarchy.Ids[i], HierarchyItemIds[i]);
		}

		OutSequenceHierarchy = SequenceHierarchy;
	}
}

void UpdateSequenceHierarchy(SpaceEntitySystem* EntitySystem,
							 csp::common::Optional<uint64_t> ParentId,
							 const csp::common::Array<uint64_t>& HierarchyItemIds,
							 csp::multiplayer::SequenceHierarchy& OutSequenceHierarchy,
							 csp::systems::EResultCode ExpectedResultCode				   = csp::systems::EResultCode::Success,
							 csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result]
		= Awaitable(&csp::multiplayer::SpaceEntitySystem::UpdateSequenceHierarchy, EntitySystem, ParentId, HierarchyItemIds).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		const csp::multiplayer::SequenceHierarchy& SequenceHierarchy = Result.GetSequenceHierarchy();

		EXPECT_EQ(SequenceHierarchy.HasParent(), ParentId.HasValue());

		if (SequenceHierarchy.HasParent() && ParentId.HasValue())
		{
			EXPECT_EQ(SequenceHierarchy.ParentId, *ParentId);
		}

		EXPECT_EQ(SequenceHierarchy.Ids.Size(), HierarchyItemIds.Size());

		for (int i = 0; i < SequenceHierarchy.Ids.Size(); ++i)
		{
			EXPECT_EQ(SequenceHierarchy.Ids[i], HierarchyItemIds[i]);
		}

		OutSequenceHierarchy = SequenceHierarchy;
	}
}

void DeleteSequenceHierarchy(SpaceEntitySystem* EntitySystem,
							 csp::common::Optional<uint64_t> ParentId,
							 csp::systems::EResultCode ExpectedResultCode				   = csp::systems::EResultCode::Success,
							 csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::multiplayer::SpaceEntitySystem::DeleteSequenceHierarchy, EntitySystem, ParentId).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);
}

void GetSequenceHierarchy(SpaceEntitySystem* EntitySystem,
						  csp::common::Optional<uint64_t> ParentId,
						  csp::multiplayer::SequenceHierarchy& OutSequenceHierarchy,
						  csp::systems::EResultCode ExpectedResultCode					= csp::systems::EResultCode::Success,
						  csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::multiplayer::SpaceEntitySystem::GetSequenceHierarchy, EntitySystem, ParentId).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	const csp::multiplayer::SequenceHierarchy& SequenceHierarchy = Result.GetSequenceHierarchy();

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		EXPECT_EQ(SequenceHierarchy.HasParent(), ParentId.HasValue());

		if (SequenceHierarchy.HasParent() && ParentId.HasValue())
		{
			EXPECT_EQ(SequenceHierarchy.ParentId, *ParentId);
		}

		OutSequenceHierarchy = SequenceHierarchy;
	}
}

void GetAllSequenceHierarchies(SpaceEntitySystem* EntitySystem,
							   csp::common::Array<csp::multiplayer::SequenceHierarchy>& OutSequenceHierarchies,
							   csp::systems::EResultCode ExpectedResultCode					 = csp::systems::EResultCode::Success,
							   csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = Awaitable(&csp::multiplayer::SpaceEntitySystem::GetAllSequenceHierarchies, EntitySystem).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
	EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

	const csp::common::Array<csp::multiplayer::SequenceHierarchy>& SequenceHierarchies = Result.GetSequenceHierarchyCollection();

	if (ExpectedResultCode == csp::systems::EResultCode::Success)
	{
		OutSequenceHierarchies = SequenceHierarchies;
	}
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

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCE_HIERARCHY_TESTS || RUN_SEQUENCE_HIERARCHY_CREATE_SEQUENCE_HIERARCHY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, CreateSequenceHierarchyTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
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
	csp::common::String EntityName1 = "Entity1";
	csp::common::String EntityName2 = "Entity2";
	csp::common::String EntityName3 = "Entity3";
	SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	auto [Entity1] = AWAIT(EntitySystem, CreateObject, EntityName1, ObjectTransform);
	auto [Entity2] = AWAIT(EntitySystem, CreateObject, EntityName2, ObjectTransform);
	auto [Entity3] = AWAIT(EntitySystem, CreateObject, EntityName3, ObjectTransform);

	// Create root sequence hierarchy
	{
		SequenceHierarchy SequenceHierarchy;
		CreateSequenceHierarchy(EntitySystem, nullptr, {Entity1->GetId(), Entity2->GetId(), Entity3->GetId()}, SequenceHierarchy);
		DeleteSequenceHierarchy(EntitySystem, nullptr);
	}

	// Create branch sequence hierarchy
	{
		SequenceHierarchy SequenceHierarchy;
		CreateSequenceHierarchy(EntitySystem, Entity1->GetId(), {Entity2->GetId(), Entity3->GetId()}, SequenceHierarchy);
		DeleteSequenceHierarchy(EntitySystem, Entity1->GetId());
	}

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCE_HIERARCHY_TESTS || RUN_SEQUENCE_HIERARCHY_UPDATE_SEQUENCE_HIERARCHY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, UpdateSequenceHierarchyTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
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
	csp::common::String EntityName1 = "Entity1";
	csp::common::String EntityName2 = "Entity2";
	csp::common::String EntityName3 = "Entity3";
	SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	auto [Entity1] = AWAIT(EntitySystem, CreateObject, EntityName1, ObjectTransform);
	auto [Entity2] = AWAIT(EntitySystem, CreateObject, EntityName2, ObjectTransform);
	auto [Entity3] = AWAIT(EntitySystem, CreateObject, EntityName3, ObjectTransform);

	// Create root sequence hierarchy
	{
		SequenceHierarchy SequenceHierarchy;
		CreateSequenceHierarchy(EntitySystem, nullptr, {Entity1->GetId(), Entity2->GetId()}, SequenceHierarchy);

		// Add a new id
		UpdateSequenceHierarchy(EntitySystem, nullptr, {Entity1->GetId(), Entity2->GetId(), Entity3->GetId()}, SequenceHierarchy);
		// Remove an id
		UpdateSequenceHierarchy(EntitySystem, nullptr, {Entity2->GetId(), Entity3->GetId()}, SequenceHierarchy);

		DeleteSequenceHierarchy(EntitySystem, nullptr);
	}

	// Create branch sequence hierarchy
	{
		SequenceHierarchy SequenceHierarchy;
		CreateSequenceHierarchy(EntitySystem, Entity1->GetId(), {Entity2->GetId()}, SequenceHierarchy);

		// Add a new id
		UpdateSequenceHierarchy(EntitySystem, Entity1->GetId(), {Entity2->GetId(), Entity3->GetId()}, SequenceHierarchy);
		// Remove an id
		UpdateSequenceHierarchy(EntitySystem, Entity1->GetId(), {Entity3->GetId()}, SequenceHierarchy);

		DeleteSequenceHierarchy(EntitySystem, Entity1->GetId());
	}

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCE_HIERARCHY_TESTS || RUN_SEQUENCE_HIERARCHY_GET_SEQUENCE_HIERARCHY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, GetSequenceHierarchyTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
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
	csp::common::String EntityName1 = "Entity1";
	csp::common::String EntityName2 = "Entity2";
	csp::common::String EntityName3 = "Entity3";
	SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	auto [Entity1] = AWAIT(EntitySystem, CreateObject, EntityName1, ObjectTransform);
	auto [Entity2] = AWAIT(EntitySystem, CreateObject, EntityName2, ObjectTransform);
	auto [Entity3] = AWAIT(EntitySystem, CreateObject, EntityName3, ObjectTransform);

	// Create root sequence hierarchy
	{
		SequenceHierarchy CreatedSequenceHierarchy;
		CreateSequenceHierarchy(EntitySystem, nullptr, {Entity1->GetId(), Entity2->GetId(), Entity3->GetId()}, CreatedSequenceHierarchy);

		SequenceHierarchy RetreivedSequenceHierarchy;
		GetSequenceHierarchy(EntitySystem, nullptr, RetreivedSequenceHierarchy);

		DeleteSequenceHierarchy(EntitySystem, nullptr);
	}

	// Create branch sequence hierarchy
	{
		SequenceHierarchy CreatedSequenceHierarchy;
		CreateSequenceHierarchy(EntitySystem, Entity1->GetId(), {Entity2->GetId(), Entity3->GetId()}, CreatedSequenceHierarchy);

		SequenceHierarchy RetreivedSequenceHierarchy;
		GetSequenceHierarchy(EntitySystem, Entity1->GetId(), RetreivedSequenceHierarchy);

		DeleteSequenceHierarchy(EntitySystem, Entity1->GetId());
	}

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SEQUENCE_HIERARCHY_TESTS || RUN_SEQUENCE_HIERARCHY_GET_SEQUENCE_HIERARCHY_TEST
CSP_PUBLIC_TEST(CSPEngine, SequenceHierarchyTests, GetAllSequenceHierarchiesTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
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
	csp::common::String EntityName1 = "Entity1";
	csp::common::String EntityName2 = "Entity2";
	csp::common::String EntityName3 = "Entity3";
	SpaceTransform ObjectTransform
		= {csp::common::Vector3 {1.452322f, 2.34f, 3.45f}, csp::common::Vector4 {4.1f, 5.1f, 6.1f, 7.1f}, csp::common::Vector3 {1, 1, 1}};

	EntitySystem->SetEntityCreatedCallback(
		[](SpaceEntity* Entity)
		{
		});

	auto [Entity1] = AWAIT(EntitySystem, CreateObject, EntityName1, ObjectTransform);
	auto [Entity2] = AWAIT(EntitySystem, CreateObject, EntityName2, ObjectTransform);
	auto [Entity3] = AWAIT(EntitySystem, CreateObject, EntityName3, ObjectTransform);

	// Create root hierarchy
	SequenceHierarchy CreatedRootSequenceHierarchy;
	CreateSequenceHierarchy(EntitySystem, nullptr, {Entity1->GetId(), Entity2->GetId(), Entity3->GetId()}, CreatedRootSequenceHierarchy);

	// Create branch hierarchy
	SequenceHierarchy CreatedBranchSequenceHierarchy;
	CreateSequenceHierarchy(EntitySystem, Entity1->GetId(), {Entity2->GetId(), Entity3->GetId()}, CreatedBranchSequenceHierarchy);

	csp::common::Array<SequenceHierarchy> Hierarchies;
	GetAllSequenceHierarchies(EntitySystem, Hierarchies);

	EXPECT_EQ(Hierarchies.Size(), 2);

	bool RootFound	 = false;
	bool ParentFound = false;

	for (size_t i = 0; i < Hierarchies.Size(); ++i)
	{
		if (Hierarchies[i].HasParent() == false)
		{
			RootFound = true;
			EXPECT_EQ(Hierarchies[i].Ids.Size(), 3);
		}
		else
		{
			ParentFound = true;
			EXPECT_EQ(Hierarchies[i].Ids.Size(), 2);
			EXPECT_EQ(Hierarchies[i].ParentId, Entity1->GetId());
		}
	}

	EXPECT_TRUE(RootFound);
	EXPECT_TRUE(ParentFound);

	// Cleanup
	DeleteSequenceHierarchy(EntitySystem, nullptr);
	DeleteSequenceHierarchy(EntitySystem, Entity1->GetId());

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
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
	auto [CreatedChildEntity]  = AWAIT(CreatedParentEntity, CreateChildEntity, ChildEntityName, ObjectTransform);

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

		AWAIT_PRE(EntitySystem, CreateSequenceHierarchy, RequestPredicate, nullptr, {CreatedChildEntity->GetId()});

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

		AWAIT_PRE(EntitySystem, CreateSequenceHierarchy, RequestPredicate, ParentId, {});

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

		DeleteSequenceHierarchy(EntitySystem, nullptr);

		EXPECT_TRUE(Called);

		EntitySystem->SetSequenceHierarchyChangedCallback(nullptr);
	}

	// Cleanup
	DeleteSequenceHierarchy(EntitySystem, CreatedParentEntity->GetId());

	auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif