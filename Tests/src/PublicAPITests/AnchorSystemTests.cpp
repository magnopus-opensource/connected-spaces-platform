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
#include "CSP/Common/Array.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Spatial/AnchorSystem.h"
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "signalrclient/signalr_value.h"

#include "gtest/gtest.h"

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateAnchor(csp::systems::AnchorSystem* anchorSystem, const csp::common::String& assetCollectionId,
    const csp::common::Optional<csp::systems::GeoLocation>& location, csp::systems::Anchor& outAnchor,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags)
{
    char uniqueThirdPartyAnchorId[256];
    SPRINTF(uniqueThirdPartyAnchorId, "CSP-UNITTEST-ID-%s", GetUniqueString().c_str());

    auto anchorPosition = csp::systems::OlyAnchorPosition(100.0, 100.0, 100.0);
    auto anchorRotation = csp::systems::OlyRotation(100.0, 100.0, 100.0, 100.0);
    auto anchorLocation = location.HasValue() ? *location : csp::systems::GeoLocation(180.0, 90.0);
    auto anchorTags = tags.HasValue() ? *tags : csp::common::Array<csp::common::String> { "Test1", "Test2" };
    auto anchorSpatialKeyValue = spatialKeyValue.HasValue()
        ? *spatialKeyValue
        : csp::common::Map<csp::common::String, csp::common::String> { { "TestKey1", "TestValue1" }, { "TestKey2", "TestValue2" } };

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::CreateAnchor, anchorSystem, csp::systems::AnchorProvider::GoogleCloudAnchors,
        uniqueThirdPartyAnchorId, assetCollectionId, anchorLocation, anchorPosition, anchorRotation, anchorSpatialKeyValue, anchorTags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outAnchor = Result.GetAnchor();
        std::cerr << "Anchor Created: Id=" << outAnchor.Id << std::endl;
    }
}

void CreateAnchorInSpace(csp::systems::AnchorSystem* anchorSystem, const csp::common::String& spaceId, uint64_t spaceEntityId,
    const csp::common::String& assetCollectionId, const csp::common::Optional<csp::systems::GeoLocation>& location, csp::systems::Anchor& outAnchor,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags)
{
    char uniqueThirdPartyAnchorId[256];
    SPRINTF(uniqueThirdPartyAnchorId, "CSP-UNITTEST-ID-%s", GetUniqueString().c_str());

    auto anchorPosition = csp::systems::OlyAnchorPosition(100.0, 100.0, 100.0);
    auto anchorRotation = csp::systems::OlyRotation(100.0, 100.0, 100.0, 100.0);
    auto anchorLocation = location.HasValue() ? *location : csp::systems::GeoLocation(180.0, 90.0);
    auto anchorTags = tags.HasValue() ? *tags : csp::common::Array<csp::common::String> { "Test1", "Test2" };
    auto anchorSpatialKeyValue = spatialKeyValue.HasValue()
        ? *spatialKeyValue
        : csp::common::Map<csp::common::String, csp::common::String> { { "TestKey1", "TestValue1" }, { "TestKey2", "TestValue2" } };

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::CreateAnchorInSpace, anchorSystem, csp::systems::AnchorProvider::GoogleCloudAnchors,
        uniqueThirdPartyAnchorId, spaceId, spaceEntityId, assetCollectionId, anchorLocation, anchorPosition, anchorRotation, anchorSpatialKeyValue,
        anchorTags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outAnchor = Result.GetAnchor();
        std::cerr << "Anchor Created: Id=" << outAnchor.Id << std::endl;
    }
}

void DeleteAnchors(csp::systems::AnchorSystem* anchorSystem, const csp::common::Array<csp::common::String>& anchorIDs)
{
    auto [Result] = Awaitable(&csp::systems::AnchorSystem::DeleteAnchors, anchorSystem, anchorIDs).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        for (size_t idx = 0; idx < anchorIDs.Size(); idx++)
        {
            std::cerr << "Anchor Deleted: "
                      << "Id=" << anchorIDs[idx] << std::endl;
        }
    }
}

void CreateAnchorResolution(
    csp::systems::AnchorSystem* anchorSystem, const csp::common::String& anchorId, csp::systems::AnchorResolution& outAnchorResolution)
{
    const bool successfullyResolved = true;
    const int resolveAttempted = 3;
    const double resolveTime = 1000;
    const csp::common::String testTag = "TestTag";
    csp::common::Array<csp::common::String> tags { testTag };

    auto [Result] = Awaitable(
        &csp::systems::AnchorSystem::CreateAnchorResolution, anchorSystem, anchorId, successfullyResolved, resolveAttempted, resolveTime, tags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& anchorResolution = Result.GetAnchorResolution();

    EXPECT_FALSE(anchorResolution.Id.IsEmpty());
    EXPECT_EQ(anchorResolution.AnchorId, anchorId);
    EXPECT_EQ(anchorResolution.SuccessfullyResolved, successfullyResolved);
    EXPECT_EQ(anchorResolution.ResolveAttempted, resolveAttempted);
    EXPECT_EQ(anchorResolution.Tags.Size(), 1);
    EXPECT_EQ(anchorResolution.Tags[0], testTag);

    outAnchorResolution = anchorResolution;
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, CreateAnchorTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSET-COLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(
        assetSystem, nullptr, nullptr, uniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection);

    csp::systems::Anchor anchor;
    CreateAnchor(anchorSystem, assetCollection.Id, nullptr, anchor, nullptr, nullptr);
    csp::common::Array<csp::common::String> createdAnchorIds(1);
    createdAnchorIds[0] = anchor.Id;

    EXPECT_EQ(anchor.ThirdPartyAnchorProvider, csp::systems::AnchorProvider::GoogleCloudAnchors);
    EXPECT_TRUE(anchor.SpaceId.IsEmpty());
    EXPECT_EQ(anchor.AssetCollectionId, assetCollection.Id);

    DeleteAnchors(anchorSystem, createdAnchorIds);
    DeleteAssetCollection(assetSystem, assetCollection);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, CreateAnchorInSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSET-COLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String objectName = "Object 1";
    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection);

    csp::systems::Anchor anchor;
    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject->GetId(), assetCollection.Id, nullptr, anchor, nullptr, nullptr);
    csp::common::Array<csp::common::String> createdAnchorIds(1);
    createdAnchorIds[0] = anchor.Id;

    EXPECT_EQ(anchor.ThirdPartyAnchorProvider, csp::systems::AnchorProvider::GoogleCloudAnchors);
    EXPECT_EQ(anchor.SpaceId, space.Id);
    EXPECT_EQ(anchor.SpaceEntityId, CreatedObject->GetId());
    EXPECT_EQ(anchor.AssetCollectionId, assetCollection.Id);

    DeleteAnchors(anchorSystem, createdAnchorIds);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(assetSystem, assetCollection);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, DeleteMultipleAnchorsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSET-COLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    char uniqueAssetCollectionName1[256];
    SPRINTF(uniqueAssetCollectionName1, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());
    char uniqueAssetCollectionName2[256];
    SPRINTF(uniqueAssetCollectionName2, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    csp::common::String objectName1 = "Object 1";
    auto [CreatedObject1] = AWAIT(realtimeEngine.get(), CreateEntity, objectName1, objectTransform, csp::common::Optional<uint64_t> {});
    csp::common::String objectName2 = "Object 2";
    auto [CreatedObject2] = AWAIT(realtimeEngine.get(), CreateEntity, objectName2, objectTransform, csp::common::Optional<uint64_t> {});

    csp::systems::AssetCollection assetCollection1;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName1, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection1);
    csp::systems::AssetCollection assetCollection2;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName2, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection2);

    csp::common::Array<csp::common::String> createdAnchorIds(2);
    csp::systems::Anchor anchor;

    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject1->GetId(), assetCollection1.Id, nullptr, anchor, nullptr, nullptr);
    createdAnchorIds[0] = anchor.Id;

    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject2->GetId(), assetCollection2.Id, nullptr, anchor, nullptr, nullptr);
    createdAnchorIds[1] = anchor.Id;

    auto [PreDeleteGetResult]
        = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInSpace, anchorSystem, space.Id, nullptr, nullptr).Await(RequestPredicate);
    EXPECT_EQ(PreDeleteGetResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(PreDeleteGetResult.GetAnchors().Size(), 2);

    DeleteAnchors(anchorSystem, createdAnchorIds);

    auto [PostDeleteGetResult]
        = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInSpace, anchorSystem, space.Id, nullptr, nullptr).Await(RequestPredicate);
    EXPECT_EQ(PostDeleteGetResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(PostDeleteGetResult.GetAnchors().Size(), 0);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(assetSystem, assetCollection1);
    DeleteAssetCollection(assetSystem, assetCollection2);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, GetAnchorsInsideCircularAreaTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSET-COLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    csp::common::Array<csp::common::String> spaceId(1);
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);
    spaceId[0] = space.Id;

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String objectName = "Object 1";
    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection);

    csp::systems::GeoLocation anchorLocation;
    anchorLocation.Latitude = 45.0;
    anchorLocation.Longitude = 160.0;

    csp::systems::Anchor anchor;
    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject->GetId(), assetCollection.Id, anchorLocation, anchor, nullptr, nullptr);

    // Search for the newly created Anchor inside a circular area
    csp::common::Array<csp::systems::Anchor> anchorCollection;

    csp::systems::GeoLocation searchLocationOrigin;
    searchLocationOrigin.Latitude = 44.0;
    searchLocationOrigin.Longitude = 160.0;
    double searchRadius = 130000.0;
    const csp::common::Array<csp::common::String> tags = { "Test1", "Test2" };
    const csp::common::Array<csp::common::String> spacialKeys = { "TestKey1", "TestKey2" };
    const csp::common::Array<csp::common::String> spacialValues = { "TestValue1", "TestValue2" };

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInArea, anchorSystem, searchLocationOrigin, searchRadius, spacialKeys,
        spacialValues, tags, true, spaceId, nullptr, nullptr)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        const auto& resultAnchors = Result.GetAnchors();
        anchorCollection = csp::common::Array<csp::systems::Anchor>(resultAnchors.Size());

        for (size_t idx = 0; idx < resultAnchors.Size(); ++idx)
        {
            anchorCollection[idx] = resultAnchors[idx];
        }

        // we should have at least the Anchor we've created
        EXPECT_GT(anchorCollection.Size(), 0);

        bool anchorFound = false;

        for (size_t idx = 0; idx < anchorCollection.Size(); ++idx)
        {
            if (anchorCollection[idx].Id == anchor.Id)
            {
                auto returnSpatialKeyValue = anchorCollection[idx].SpatialKeyValue;
                EXPECT_EQ(returnSpatialKeyValue.Size(), spacialValues.Size());

                for (size_t i = 0; i < returnSpatialKeyValue.Size(); ++i)
                {
                    EXPECT_TRUE(returnSpatialKeyValue.HasKey(spacialKeys[i]));
                    EXPECT_EQ(returnSpatialKeyValue[spacialKeys[i]], spacialValues[i]);
                }

                auto returnTags = anchorCollection[idx].Tags;
                EXPECT_EQ(returnTags.Size(), tags.Size());

                for (size_t i = 0; i < returnTags.Size(); ++i)
                {
                    EXPECT_EQ(returnTags[i], tags[i]);
                }

                anchorFound = true;
                break;
            }
        }

        EXPECT_TRUE(anchorFound);
    }
    else
    {
        std::cerr << "GetAnchorsInArea failed with ResultCode: " << (int)Result.GetResultCode() << " HttpResultCode: " << Result.GetHttpResultCode()
                  << std::endl;
    }

    csp::common::Array<csp::common::String> createdAnchorIds(1);
    createdAnchorIds[0] = anchor.Id;
    DeleteAnchors(anchorSystem, createdAnchorIds);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(assetSystem, assetCollection);
    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, GetAnchorsInSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSET-COLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    char uniqueAssetCollectionName1[256];
    SPRINTF(uniqueAssetCollectionName1, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());
    char uniqueAssetCollectionName2[256];
    SPRINTF(uniqueAssetCollectionName2, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    csp::common::String objectName1 = "Object 1";
    auto [CreatedObject1] = AWAIT(realtimeEngine.get(), CreateEntity, objectName1, objectTransform, csp::common::Optional<uint64_t> {});
    csp::common::String objectName2 = "Object 2";
    auto [CreatedObject2] = AWAIT(realtimeEngine.get(), CreateEntity, objectName2, objectTransform, csp::common::Optional<uint64_t> {});

    ASSERT_TRUE(CreatedObject1);
    ASSERT_TRUE(CreatedObject2);

    csp::systems::AssetCollection assetCollection1;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName1, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection1);
    csp::systems::AssetCollection assetCollection2;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName2, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection2);

    csp::common::Array<csp::common::String> createdAnchorIds(2);
    csp::systems::Anchor anchor1;
    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject1->GetId(), assetCollection1.Id, nullptr, anchor1, nullptr, nullptr);
    createdAnchorIds[0] = anchor1.Id;

    csp::systems::Anchor anchor2;
    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject2->GetId(), assetCollection2.Id, nullptr, anchor2, nullptr, nullptr);
    createdAnchorIds[1] = anchor2.Id;

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInSpace, anchorSystem, space.Id, nullptr, nullptr).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto anchors = Result.GetAnchors();
    EXPECT_EQ(anchors.Size(), 2);

    int anchorsFound = 0;
    for (size_t i = 0; i < anchors.Size(); ++i)
    {
        EXPECT_EQ(anchors[i].SpaceId, space.Id);
        if (anchors[i].ThirdPartyAnchorId == anchor1.ThirdPartyAnchorId || anchors[i].ThirdPartyAnchorId == anchor2.ThirdPartyAnchorId)
        {
            ++anchorsFound;
        }
    }
    EXPECT_EQ(anchorsFound, 2);

    DeleteAnchors(anchorSystem, createdAnchorIds);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(assetSystem, assetCollection1);
    DeleteAssetCollection(assetSystem, assetCollection2);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, GetAnchorsByAssetCollectionIdTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", "CSP-UNITTEST-SPACE-MAG", GetUniqueString().c_str());
    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", "CSP-UNITTEST-ASSET-COLLECTION-MAG", GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(
        assetSystem, nullptr, nullptr, uniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection);

    csp::systems::Anchor anchor1;
    CreateAnchor(anchorSystem, assetCollection.Id, nullptr, anchor1, nullptr, nullptr);

    csp::systems::Anchor anchor2;
    CreateAnchor(anchorSystem, assetCollection.Id, nullptr, anchor2, nullptr, nullptr);

    // Get and validate anchors
    {
        auto [Result] = AWAIT_PRE(anchorSystem, GetAnchorsByAssetCollectionId, RequestPredicate, assetCollection.Id, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& anchors = Result.GetAnchors();

        EXPECT_EQ(anchors.Size(), 2);

        bool found1 = false, found2 = false;

        for (size_t i = 0; i < anchors.Size(); ++i)
        {
            if (anchors[i].Id == anchor1.Id)
            {
                found1 = true;
            }
            else if (anchors[i].Id == anchor2.Id)
            {
                found2 = true;
            }
        }

        EXPECT_TRUE(found1 && found2);
    }

    DeleteAnchors(anchorSystem, { anchor1.Id, anchor2.Id });
    DeleteAssetCollection(assetSystem, assetCollection);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, CreateAnchorResolutionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* anchorSystem = systemsManager.GetAnchorSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSET-COLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());
    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    csp::common::String objectName = "Object 1";
    csp::multiplayer::SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, assetCollection);

    // Create Anchor
    csp::systems::Anchor anchor;
    CreateAnchorInSpace(anchorSystem, space.Id, CreatedObject->GetId(), assetCollection.Id, nullptr, anchor, nullptr, nullptr);
    csp::common::Array<csp::common::String> createdAnchorIds(1);
    createdAnchorIds[0] = anchor.Id;

    // Create AnchorResolution
    csp::systems::AnchorResolution anchorResolution;
    CreateAnchorResolution(anchorSystem, anchor.Id, anchorResolution);

    // Cleanup
    DeleteAnchors(anchorSystem, createdAnchorIds);
    DeleteAssetCollection(assetSystem, assetCollection);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}