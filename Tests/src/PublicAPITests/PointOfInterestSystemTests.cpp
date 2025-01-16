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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

namespace
{

void CreatePointOfInterest(csp::systems::PointOfInterestSystem* POISystem, const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
    const csp::common::Optional<csp::systems::GeoLocation>& Location, const csp::common::Optional<csp::systems::AssetCollection>& AssetCollection,
    csp::systems::PointOfInterest& OutPOI)
{
    const char* POITitle = "OLY-UNITTEST-POI-TITLE";
    const char* POIDescription = "OLY-UNITTEST-POI-DESC-REWIND";
    const char* POIOwner = "OLY-UNITTEST-OWNER";
    const auto POIType = csp::systems::EPointOfInterestType::DEFAULT;

    csp::systems::GeoLocation POILocation;

    if (Location.HasValue())
    {
        POILocation = *Location;
    }
    else
    {
        POILocation.Latitude = 90.0; // default values for the tests
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
    SPRINTF(UniquePOIName, "%s-%s", POITitle, GetUniqueString().c_str());

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::CreatePOI, POISystem, POITitle, POIDescription, UniquePOIName, Tags, POIType,
        POIOwner, POILocation, POIAssetCollection)
                        .Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutPOI = Result.GetPointOfInterest();
        std::cerr << "POI Created: Name=" << OutPOI.Name << " Id=" << OutPOI.Id << std::endl;
    }
}

void DeletePointOfInterest(csp::systems::PointOfInterestSystem* POISystem, const csp::systems::PointOfInterest& POI)
{
    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::DeletePOI, POISystem, POI).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cerr << "POI Deleted: Name=" << POI.Name << " Id=" << POI.Id << std::endl;
    }
}

void GetAssetCollectionFromPOI(
    csp::systems::AssetSystem* AssetSystem, const csp::systems::PointOfInterest& POI, csp::systems::AssetCollection& OutAssetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::GetAssetCollectionById, AssetSystem, POI.AssetCollectionId).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutAssetCollection = Result.GetAssetCollection();
    }
}

void CreateAssetCollection(
    csp::systems::AssetSystem* AssetSystem, csp::systems::Space& Space, const char* Name, csp::systems::AssetCollection& OutAssetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::CreateAssetCollection, AssetSystem, Space.Id, nullptr, Name, nullptr,
        csp::systems::EAssetCollectionType::DEFAULT, nullptr)
                        .Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutAssetCollection = Result.GetAssetCollection();
    }
}

void DeleteAssetCollection(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::DeleteAssetCollection, AssetSystem, AssetCollection).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

bool AreTestAssetCollectionsEqual(const csp::systems::AssetCollection& Lhs, const csp::systems::AssetCollection& Rhs)
{
    // checking only the fields that have been used in these tests
    bool AreEqual = true;

    AreEqual &= (Lhs.Id == Rhs.Id);
    AreEqual &= (Lhs.Name == Rhs.Name);
    AreEqual &= (Lhs.SpaceId == Rhs.SpaceId);

    return AreEqual;
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_CREATEPOI_TEST
CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, CreatePOITest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* POISystem = SystemsManager.GetPointOfInterestSystem();

    csp::common::String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* POISystem = SystemsManager.GetPointOfInterestSystem();

    csp::common::String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* POISystem = SystemsManager.GetPointOfInterestSystem();

    csp::common::String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::GeoLocation POILocation;
    POILocation.Latitude = 45.0;
    POILocation.Longitude = 160.0;

    csp::systems::PointOfInterest PointOfInterest;
    CreatePointOfInterest(POISystem, nullptr, POILocation, nullptr, PointOfInterest);

    // Search for the newly created POI inside a circular area
    csp::common::Array<csp::systems::PointOfInterest> POICollection;

    csp::systems::GeoLocation SearchLocationOrigin;
    SearchLocationOrigin.Latitude = 44.0;
    SearchLocationOrigin.Longitude = 160.0;
    double SearchRadius = 130000;

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::GetPOIsInArea, POISystem, SearchLocationOrigin, SearchRadius,
        csp::systems::EPointOfInterestType::DEFAULT)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        const auto& ResultPOIs = Result.GetPOIs();
        POICollection = csp::common::Array<csp::systems::PointOfInterest>(ResultPOIs.Size());

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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* POISystem = SystemsManager.GetPointOfInterestSystem();

    csp::common::String UserId;

    LogInAsNewTestUser(UserSystem, UserId);

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

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

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_QUERY_POI_TYPE_TEST
CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, QuerySpacePOITest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* POISystem = SystemsManager.GetPointOfInterestSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    const char* TestSpaceName = "CSP-TEST-SPACE";
    const char* TestSpaceDescription = "CSP-TEST-SPACEDESC";
    const char* TestAssetCollectionName = "CSP-TEST-ASSETCOLLECTION";

    // The default POI we will be using during the test.
    csp::systems::PointOfInterest DefaultPOI;

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::systems::GeoLocation SpaceGeolocation;

    // Create a space POI and a default POI.
    {
        SpaceGeolocation.Latitude = RandomRangeDouble(-90.0f, 90.0f);
        SpaceGeolocation.Longitude = RandomRangeDouble(-180.0f, 180.0f);

        float SpaceOrientation = 90.0f;

        csp::common::Array<csp::systems::GeoLocation> SpaceGeofence(4);

        csp::systems::GeoLocation GeoFence0;
        GeoFence0.Latitude = 5.5;
        GeoFence0.Longitude = 6.6;
        SpaceGeofence[0] = GeoFence0;
        SpaceGeofence[3] = GeoFence0;

        csp::systems::GeoLocation GeoFence1;
        GeoFence1.Latitude = 7.7;
        GeoFence1.Longitude = 8.8;
        SpaceGeofence[1] = GeoFence1;

        csp::systems::GeoLocation GeoFence2;
        GeoFence2.Latitude = 9.9;
        GeoFence2.Longitude = 10.0;
        SpaceGeofence[2] = GeoFence2;

        // Create space POI via the space system.
        auto [AddSpacePOIGeoResult]
            = AWAIT_PRE(SpaceSystem, UpdateSpaceGeoLocation, RequestPredicate, Space.Id, SpaceGeolocation, SpaceOrientation, SpaceGeofence);
        EXPECT_EQ(AddSpacePOIGeoResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create default POI through the POI system at the same location.
        CreatePointOfInterest(POISystem, nullptr, SpaceGeolocation, nullptr, DefaultPOI);
    }

    // Test retrieving the space POI only.
    {
        auto [GetPOIsResult]
            = AWAIT_PRE(POISystem, GetPOIsInArea, RequestPredicate, SpaceGeolocation, 5.0f, csp::systems::EPointOfInterestType::SPACE);
        EXPECT_EQ(GetPOIsResult.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::common::Array<csp::systems::PointOfInterest>& POIs = GetPOIsResult.GetPOIs();
        EXPECT_GE(POIs.Size(), 1);

        // There may be more than one POI at this location, so we search for the one we have created and expect to find it.
        bool FoundSpacePOI = false;
        for (uint32_t i = 0; i < POIs.Size(); i++)
        {
            // All the returned POIs should be of type SPACE, since that is what we searched for.
            EXPECT_EQ(POIs[i].Type, csp::systems::EPointOfInterestType::SPACE);
            if (POIs[i].SpaceId == Space.Id)
            {
                FoundSpacePOI = true;
                break;
            }
        }
        EXPECT_TRUE(FoundSpacePOI);
    }

    // Test retrieving the default POI only.
    {
        auto [GetPOIsResult]
            = AWAIT_PRE(POISystem, GetPOIsInArea, RequestPredicate, SpaceGeolocation, 5.0f, csp::systems::EPointOfInterestType::DEFAULT);
        EXPECT_EQ(GetPOIsResult.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::common::Array<csp::systems::PointOfInterest>& POIs = GetPOIsResult.GetPOIs();
        EXPECT_GE(POIs.Size(), 1);

        // There may be more than one POI at this location, so we search for the one we have created and expect to find it.
        bool FoundDefaultPOI = false;
        for (uint32_t i = 0; i < POIs.Size(); i++)
        {
            // All the returned POIs should be of type DEFAULT, since that is what we searched for.
            EXPECT_EQ(POIs[i].Type, csp::systems::EPointOfInterestType::DEFAULT);
            if (POIs[i].Id == DefaultPOI.Id)
            {
                FoundDefaultPOI = true;
                break;
            }
        }
        EXPECT_TRUE(FoundDefaultPOI);
    }

    // Test retrieving the space POI _and_ the default POI, by not specifying the type of POI we want returned.
    {
        auto [GetPOIsResult] = AWAIT_PRE(POISystem, GetPOIsInArea, RequestPredicate, SpaceGeolocation, 5.0f, nullptr);
        EXPECT_EQ(GetPOIsResult.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::common::Array<csp::systems::PointOfInterest>& POIs = GetPOIsResult.GetPOIs();
        EXPECT_GE(POIs.Size(), 1);

        // There may be more than one POI at this location, so we search for the one we have created and expect to find it.
        bool FoundSpacePOI = false;
        bool FoundDefaultPOI = false;
        for (uint32_t i = 0; i < POIs.Size(); i++)
        {
            if (POIs[i].Id == DefaultPOI.Id)
            {
                FoundDefaultPOI = true;
            }

            if (POIs[i].SpaceId == Space.Id)
            {
                FoundSpacePOI = true;
            }
        }
        EXPECT_TRUE(FoundDefaultPOI && FoundSpacePOI);
    }

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif