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

#include "CSP/Systems/Spatial/PointOfInterestSystem.h"
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "gtest/gtest.h"

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

namespace
{

void CreatePointOfInterest(csp::systems::PointOfInterestSystem* poiSystem, const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    const csp::common::Optional<csp::systems::GeoLocation>& location, const csp::common::Optional<csp::systems::AssetCollection>& assetCollection,
    csp::systems::PointOfInterest& outPoi)
{
    const char* poiTitle = "CSP-UNITTEST-POI-TITLE";
    const char* poiDescription = "CSP-UNITTEST-POI-DESC-MAG";
    const char* poiOwner = "CSP-UNITTEST-OWNER";
    const auto poiType = csp::systems::EPointOfInterestType::DEFAULT;

    csp::systems::GeoLocation poiLocation;

    if (location.HasValue())
    {
        poiLocation = *location;
    }
    else
    {
        poiLocation.Latitude = 90.0; // default values for the tests
        poiLocation.Longitude = 180.0;
    }

    csp::systems::AssetCollection poiAssetCollection;

    if (assetCollection.HasValue())
    {
        poiAssetCollection = *assetCollection;
    }
    else
    {
        // for the POI creation only the ID is relevant
        poiAssetCollection.Id = "CSP-UNITTEST-ASSET-COLLECTION-ID";
    }

    char uniquePoiName[256];
    SPRINTF(uniquePoiName, "%s-%s", poiTitle, GetUniqueString().c_str());

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::CreatePOI, poiSystem, poiTitle, poiDescription, uniquePoiName, tags, poiType,
        poiOwner, poiLocation, poiAssetCollection)
                        .Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outPoi = Result.GetPointOfInterest();
        std::cerr << "POI Created: Name=" << outPoi.Name << " Id=" << outPoi.Id << std::endl;
    }
}

void DeletePointOfInterest(csp::systems::PointOfInterestSystem* poiSystem, const csp::systems::PointOfInterest& poi)
{
    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::DeletePOI, poiSystem, poi).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cerr << "POI Deleted: Name=" << poi.Name << " Id=" << poi.Id << std::endl;
    }
}

void GetAssetCollectionFromPOI(
    csp::systems::AssetSystem* assetSystem, const csp::systems::PointOfInterest& poi, csp::systems::AssetCollection& outAssetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::GetAssetCollectionById, assetSystem, poi.AssetCollectionId).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outAssetCollection = Result.GetAssetCollection();
    }
}

void CreateAssetCollection(
    csp::systems::AssetSystem* assetSystem, csp::systems::Space& space, const char* name, csp::systems::AssetCollection& outAssetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::CreateAssetCollection, assetSystem, space.Id, nullptr, name, nullptr,
        csp::systems::EAssetCollectionType::DEFAULT, nullptr)
                        .Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outAssetCollection = Result.GetAssetCollection();
    }
}

void DeleteAssetCollection(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::DeleteAssetCollection, assetSystem, assetCollection).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

bool AreTestAssetCollectionsEqual(const csp::systems::AssetCollection& lhs, const csp::systems::AssetCollection& rhs)
{
    // checking only the fields that have been used in these tests
    bool areEqual = true;

    areEqual &= (lhs.Id == rhs.Id);
    areEqual &= (lhs.Name == rhs.Name);
    areEqual &= (lhs.SpaceId == rhs.SpaceId);

    return areEqual;
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, CreatePOITest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::PointOfInterest pointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, nullptr, nullptr, pointOfInterest);

    DeletePointOfInterest(poiSystem, pointOfInterest);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, CreatePOIWithTagsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::common::Optional<csp::common::Array<csp::common::String>> tags(2);
    (*tags)[0] = "POITag1";
    (*tags)[1] = "POITag2";

    csp::systems::PointOfInterest pointOfInterest;
    CreatePointOfInterest(poiSystem, tags, nullptr, nullptr, pointOfInterest);

    DeletePointOfInterest(poiSystem, pointOfInterest);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, GetPOIInsideCircularAreaTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::GeoLocation poiLocation;
    poiLocation.Latitude = 45.0;
    poiLocation.Longitude = 160.0;

    csp::systems::PointOfInterest pointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, poiLocation, nullptr, pointOfInterest);

    // Search for the newly created POI inside a circular area
    csp::systems::GeoLocation searchLocationOrigin;
    searchLocationOrigin.Latitude = 44.0;
    searchLocationOrigin.Longitude = 160.0;
    double searchRadius = 130000;

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::GetPOIsInArea, poiSystem, searchLocationOrigin, searchRadius,
        csp::systems::EPointOfInterestType::DEFAULT)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::Array<csp::systems::PointOfInterest>& poiCollection = Result.GetPOIs();

    // we should have at least the POI we've created
    EXPECT_TRUE(poiCollection.Size() > 0);

    bool poiFound = false;

    for (size_t idx = 0; idx < poiCollection.Size(); ++idx)
    {
        if (poiCollection[idx].Name == pointOfInterest.Name)
        {
            poiFound = true;
            break;
        }
    }

    EXPECT_TRUE(poiFound);

    DeletePointOfInterest(poiSystem, pointOfInterest);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, GetPOIOutsideCircularAreaTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::GeoLocation poiLocation;
    poiLocation.Latitude = 45.0;
    poiLocation.Longitude = 160.0;

    csp::systems::PointOfInterest pointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, poiLocation, nullptr, pointOfInterest);

    csp::systems::GeoLocation searchLocationOrigin;
    searchLocationOrigin.Latitude = 0;
    searchLocationOrigin.Longitude = 0;
    double searchRadius = 1000;

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::GetPOIsInArea, poiSystem, searchLocationOrigin, searchRadius,
        csp::systems::EPointOfInterestType::DEFAULT)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetPOIs().Size(), 0);

    DeletePointOfInterest(poiSystem, pointOfInterest);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, GetPOIInsideCircularAreaExcludeOtherTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::GeoLocation insidePoiLocation;
    insidePoiLocation.Latitude = 45.0;
    insidePoiLocation.Longitude = 160.0;

    csp::systems::PointOfInterest insidePointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, insidePoiLocation, nullptr, insidePointOfInterest);

    csp::systems::GeoLocation outsidePoiLocation;
    outsidePoiLocation.Latitude = -45.0;
    outsidePoiLocation.Longitude = -160.0;

    csp::systems::PointOfInterest outsidePointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, outsidePoiLocation, nullptr, outsidePointOfInterest);

    // Search for the newly created POI inside a circular area
    csp::systems::GeoLocation searchLocationOrigin;
    searchLocationOrigin.Latitude = 44.0;
    searchLocationOrigin.Longitude = 160.0;
    double searchRadius = 130000;

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::GetPOIsInArea, poiSystem, searchLocationOrigin, searchRadius,
        csp::systems::EPointOfInterestType::DEFAULT)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::common::Array<csp::systems::PointOfInterest>& poiCollection = Result.GetPOIs();

    EXPECT_EQ(poiCollection.Size(), 1);
    EXPECT_EQ(poiCollection[0].Name, insidePointOfInterest.Name);

    DeletePointOfInterest(poiSystem, insidePointOfInterest);
    DeletePointOfInterest(poiSystem, outsidePointOfInterest);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, GetAssetCollectionFromPOITest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    char uniqueAssetCollectionName[256];
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space, uniqueAssetCollectionName, assetCollection);

    csp::systems::PointOfInterest pointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, nullptr, assetCollection, pointOfInterest);

    csp::systems::AssetCollection retrievedAssetCollection;
    GetAssetCollectionFromPOI(assetSystem, pointOfInterest, retrievedAssetCollection);

    EXPECT_TRUE(AreTestAssetCollectionsEqual(retrievedAssetCollection, assetCollection));

    DeleteAssetCollection(assetSystem, assetCollection);

    DeleteSpace(spaceSystem, space.Id);

    DeletePointOfInterest(poiSystem, pointOfInterest);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, QuerySpacePOITest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // The default POI we will be using during the test.
    csp::systems::PointOfInterest defaultPoi;

    csp::systems::GeoLocation spaceGeolocation;

    // Create a space POI and a default POI.
    {
        spaceGeolocation.Latitude = RandomRangeDouble(-90.0f, 90.0f);
        spaceGeolocation.Longitude = RandomRangeDouble(-180.0f, 180.0f);

        float spaceOrientation = 90.0f;

        csp::common::Array<csp::systems::GeoLocation> spaceGeofence(4);

        csp::systems::GeoLocation geoFence0;
        geoFence0.Latitude = 5.5;
        geoFence0.Longitude = 6.6;
        spaceGeofence[0] = geoFence0;
        spaceGeofence[3] = geoFence0;

        csp::systems::GeoLocation geoFence1;
        geoFence1.Latitude = 7.7;
        geoFence1.Longitude = 8.8;
        spaceGeofence[1] = geoFence1;

        csp::systems::GeoLocation geoFence2;
        geoFence2.Latitude = 9.9;
        geoFence2.Longitude = 10.0;
        spaceGeofence[2] = geoFence2;

        // Create space POI via the space system.
        auto [AddSpacePOIGeoResult]
            = AWAIT_PRE(spaceSystem, UpdateSpaceGeoLocation, RequestPredicate, space.Id, spaceGeolocation, spaceOrientation, spaceGeofence);
        EXPECT_EQ(AddSpacePOIGeoResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create default POI through the POI system at the same location.
        CreatePointOfInterest(poiSystem, nullptr, spaceGeolocation, nullptr, defaultPoi);
    }

    // Test retrieving the space POI only.
    {
        auto [GetPOIsResult]
            = AWAIT_PRE(poiSystem, GetPOIsInArea, RequestPredicate, spaceGeolocation, 5.0f, csp::systems::EPointOfInterestType::SPACE);
        EXPECT_EQ(GetPOIsResult.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::common::Array<csp::systems::PointOfInterest>& poIs = GetPOIsResult.GetPOIs();
        EXPECT_GE(poIs.Size(), 1);

        // There may be more than one POI at this location, so we search for the one we have created and expect to find it.
        bool foundSpacePoi = false;
        for (uint32_t i = 0; i < poIs.Size(); i++)
        {
            // All the returned POIs should be of type SPACE, since that is what we searched for.
            EXPECT_EQ(poIs[i].Type, csp::systems::EPointOfInterestType::SPACE);
            if (poIs[i].SpaceId == space.Id)
            {
                foundSpacePoi = true;
                break;
            }
        }
        EXPECT_TRUE(foundSpacePoi);
    }

    // Test retrieving the default POI only.
    {
        auto [GetPOIsResult]
            = AWAIT_PRE(poiSystem, GetPOIsInArea, RequestPredicate, spaceGeolocation, 5.0f, csp::systems::EPointOfInterestType::DEFAULT);
        EXPECT_EQ(GetPOIsResult.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::common::Array<csp::systems::PointOfInterest>& poIs = GetPOIsResult.GetPOIs();
        EXPECT_GE(poIs.Size(), 1);

        // There may be more than one POI at this location, so we search for the one we have created and expect to find it.
        bool foundDefaultPoi = false;
        for (uint32_t i = 0; i < poIs.Size(); i++)
        {
            // All the returned POIs should be of type DEFAULT, since that is what we searched for.
            EXPECT_EQ(poIs[i].Type, csp::systems::EPointOfInterestType::DEFAULT);
            if (poIs[i].Id == defaultPoi.Id)
            {
                foundDefaultPoi = true;
                break;
            }
        }
        EXPECT_TRUE(foundDefaultPoi);
    }

    // Test retrieving the space POI _and_ the default POI, by not specifying the type of POI we want returned.
    {
        auto [GetPOIsResult] = AWAIT_PRE(poiSystem, GetPOIsInArea, RequestPredicate, spaceGeolocation, 5.0f, nullptr);
        EXPECT_EQ(GetPOIsResult.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::common::Array<csp::systems::PointOfInterest>& poIs = GetPOIsResult.GetPOIs();
        EXPECT_GE(poIs.Size(), 1);

        // Multiple POIs have been created at the same location, so we need to validate that the expected POIs are found.
        csp::systems::PointOfInterest poiTypeDefault;
        csp::systems::PointOfInterest poiTypeSpace;

        for (uint32_t i = 0; i < poIs.Size(); i++)
        {
            if (poIs[i].Id == defaultPoi.Id)
            {
                poiTypeDefault = poIs[i];
            }

            if (poIs[i].SpaceId == space.Id)
            {
                poiTypeSpace = poIs[i];
            }
        }

        EXPECT_NE(poiTypeDefault.Id, "");
        DeletePointOfInterest(poiSystem, poiTypeDefault);

        EXPECT_NE(poiTypeSpace.Id, "");
        DeletePointOfInterest(poiSystem, poiTypeSpace);
    }

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PointOfInterestSystemTests, DeletePOITest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* poiSystem = systemsManager.GetPointOfInterestSystem();

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::GeoLocation poiLocation;
    poiLocation.Latitude = 45.0;
    poiLocation.Longitude = 160.0;

    csp::systems::PointOfInterest pointOfInterest;
    CreatePointOfInterest(poiSystem, nullptr, poiLocation, nullptr, pointOfInterest);

    auto [Result] = Awaitable(&csp::systems::PointOfInterestSystem::DeletePOI, poiSystem, pointOfInterest).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    LogOut(userSystem);
}
