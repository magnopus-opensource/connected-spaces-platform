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
#include "CSP/Common/Array.h"
#include "CSP/Common/Optional.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <CSP/Systems/Spatial/PointOfInterestSystem.h>
#include <CSP/Systems/Spatial/SpatialDataTypes.h>



namespace
{

bool RequestPredicate(const csp::services::ResultBase& Result)
{
	return Result.GetResultCode() != csp::services::EResultCode::InProgress;
}

} // namespace



namespace
{

void CreatePointOfInterest(csp::systems::PointOfInterestSystem* POISystem,
						   const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
						   const csp::common::Optional<csp::systems::GeoLocation>& Location,
						   const csp::common::Optional<csp::systems::AssetCollection>& AssetCollection,
						   csp::systems::PointOfInterest& OutPOI)
{
	const char* POITitle	   = "OLY-UNITTEST-POI-TITLE";
	const char* POIDescription = "OLY-UNITTEST-POI-DESC-REWIND";
	const char* POIOwner	   = "OLY-UNITTEST-OWNER";
	const auto POIType		   = csp::systems::EPointOfInterestType::DEFAULT;

	csp::systems::GeoLocation POILocation;

	if (Location.HasValue())
	{
		POILocation = *Location;
	}
	else
	{
		POILocation.Latitude  = 90.0; // default values for the tests
		POILocation.Longitude = 180.0;
	}

	csp::systems::AssetCollection POIAssetCollection;

	if (AssetCollection.HasValue())
	{
		POIAssetCollection = *AssetCollection;
	}
	else
	{
		// for the POI creation only the ID is relevant
		POIAssetCollection.Id = "OLY-UNITTEST-ASSET-COLLECTION-ID";
	}

	char UniquePOIName[256];
	SPRINTF(UniquePOIName, "%s-%s", POITitle, GetUniqueHexString().c_str());

	auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::CreatePOI,
							  POISystem,
							  POITitle,
							  POIDescription,
							  UniquePOIName,
							  Tags,
							  POIType,
							  POIOwner,
							  POILocation,
							  POIAssetCollection)
						.Await(RequestPredicate);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		OutPOI = Result.GetPointOfInterest();
		std::cerr << "POI Created: Name=" << OutPOI.Name << " Id=" << OutPOI.Id << std::endl;
	}
}

void DeletePointOfInterest(csp::systems::PointOfInterestSystem* POISystem, const csp::systems::PointOfInterest& POI)
{
	auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::DeletePOI, POISystem, POI).Await(RequestPredicate);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		std::cerr << "POI Deleted: Name=" << POI.Name << " Id=" << POI.Id << std::endl;
	}
}

void GetAssetCollectionFromPOI(csp::systems::AssetSystem* AssetSystem,
							   const csp::systems::PointOfInterest& POI,
							   csp::systems::AssetCollection& OutAssetCollection)
{
	auto [Result] = Awaitable(&csp::systems::AssetSystem::GetAssetCollectionById, AssetSystem, POI.AssetCollectionId).Await(RequestPredicate);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		OutAssetCollection = Result.GetAssetCollection();
	}
}

void CreateAssetCollection(csp::systems::AssetSystem* AssetSystem,
						   csp::systems::Space& Space,
						   const char* Name,
						   csp::systems::AssetCollection& OutAssetCollection)
{
	auto [Result] = Awaitable(&csp::systems::AssetSystem::CreateAssetCollection,
							  AssetSystem,
							  Space.Id,
							  nullptr,
							  Name,
							  nullptr,
							  csp::systems::EAssetCollectionType::DEFAULT,
							  nullptr)
						.Await(RequestPredicate);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		OutAssetCollection = Result.GetAssetCollection();
	}
}

void DeleteAssetCollection(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection)
{
	auto [Result] = Awaitable(&csp::systems::AssetSystem::DeleteAssetCollection, AssetSystem, AssetCollection).Await(RequestPredicate);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
}

bool AreTestAssetCollectionsEqual(const csp::systems::AssetCollection& Lhs, const csp::systems::AssetCollection& Rhs)
{
	// checking only the fields that have been used in these tests
	bool AreEqual = true;

	AreEqual &= (Lhs.Id == Rhs.Id);
	AreEqual &= (Lhs.Name == Rhs.Name);
	AreEqual &= (Lhs.SpaceIds[0] == Rhs.SpaceIds[0]);

	return AreEqual;
}

} // namespace



#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_CREATEPOI_TEST
CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, CreatePOITest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* POISystem		 = SystemsManager.GetPointOfInterestSystem();

	csp::common::String UserId;

	LogIn(UserSystem, UserId);

	csp::systems::PointOfInterest PointOfInterest;
	CreatePointOfInterest(POISystem, nullptr, nullptr, nullptr, PointOfInterest);

	DeletePointOfInterest(POISystem, PointOfInterest);

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_CREATEPOI_WITH_TAGS_TEST
CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, CreatePOIWithTagsTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* POISystem		 = SystemsManager.GetPointOfInterestSystem();

	csp::common::String UserId;

	LogIn(UserSystem, UserId);

	csp::common::Optional<csp::common::Array<csp::common::String>> Tags(2);
	(*Tags)[0] = "POITag1";
	(*Tags)[1] = "POITag2";

	csp::systems::PointOfInterest PointOfInterest;
	CreatePointOfInterest(POISystem, Tags, nullptr, nullptr, PointOfInterest);

	DeletePointOfInterest(POISystem, PointOfInterest);

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_GETPOI_INSIDE_CIRCULAR_AREA_TEST
CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, GetPOIInsideCircularAreaTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* POISystem		 = SystemsManager.GetPointOfInterestSystem();

	csp::common::String UserId;

	LogIn(UserSystem, UserId);

	csp::systems::GeoLocation POILocation;
	POILocation.Latitude  = 45.0;
	POILocation.Longitude = 160.0;

	csp::systems::PointOfInterest PointOfInterest;
	CreatePointOfInterest(POISystem, nullptr, POILocation, nullptr, PointOfInterest);

	// Search for the newly created POI inside a circular area
	csp::common::Array<csp::systems::PointOfInterest> POICollection;

	csp::systems::GeoLocation SearchLocationOrigin;
	SearchLocationOrigin.Latitude  = 44.0;
	SearchLocationOrigin.Longitude = 160.0;
	double SearchRadius			   = 130000;

	auto [Result]
		= Awaitable(&csp::systems::PointOfInterestSystem::GetPOIsInArea, POISystem, SearchLocationOrigin, SearchRadius).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		const auto& ResultPOIs = Result.GetPOIs();
		POICollection		   = csp::common::Array<csp::systems::PointOfInterest>(ResultPOIs.Size());

		for (int idx = 0; idx < ResultPOIs.Size(); ++idx)
		{
			POICollection[idx] = ResultPOIs[idx];
		}
	}

	// we should have at least the POI we've created
	EXPECT_TRUE(POICollection.Size() > 0);

	bool POIFound = false;

	for (int idx = 0; idx < POICollection.Size(); ++idx)
	{
		if (POICollection[idx].Name == PointOfInterest.Name)
		{
			POIFound = true;
			break;
		}
	}

	EXPECT_TRUE(POIFound);

	DeletePointOfInterest(POISystem, PointOfInterest);

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_GET_ASSETCOLLECTION_FROM_POI_TEST
CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, GetAssetCollectionFromPOITest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* POISystem		 = SystemsManager.GetPointOfInterestSystem();

	csp::common::String UserId;

	LogIn(UserSystem, UserId);

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueHexString().c_str());

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	csp::systems::AssetCollection AssetCollection;
	CreateAssetCollection(AssetSystem, Space, UniqueAssetCollectionName, AssetCollection);

	csp::systems::PointOfInterest PointOfInterest;
	CreatePointOfInterest(POISystem, nullptr, nullptr, AssetCollection, PointOfInterest);

	csp::systems::AssetCollection RetrievedAssetCollection;
	GetAssetCollectionFromPOI(AssetSystem, PointOfInterest, RetrievedAssetCollection);

	EXPECT_TRUE(AreTestAssetCollectionsEqual(RetrievedAssetCollection, AssetCollection));

	DeleteAssetCollection(AssetSystem, AssetCollection);

	DeleteSpace(SpaceSystem, Space.Id);

	DeletePointOfInterest(POISystem, PointOfInterest);

	LogOut(UserSystem);
}
#endif