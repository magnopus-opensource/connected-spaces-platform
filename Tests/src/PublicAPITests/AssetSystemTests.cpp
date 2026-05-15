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
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Array.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "RAIIMockLogger.h"
#include "Services/PrototypeService/PrototypeServiceApiMock.h"
#include "SpaceSystemTestHelpers.h"
#include "Systems/ResultHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

/**/
#include <Awaitable.h>
/**/

#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/NetworkEventManagerImpl.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <future>
#include <gmock/gmock.h>

using namespace csp::multiplayer;

namespace chs_prototype = csp::services::generated::prototypeservice;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

bool RequestPredicateWithProgress(const csp::systems::ResultBase& result)
{
    if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        PrintProgress(result.GetRequestProgress());

        return false;
    }

    return true;
}

} // namespace

void CreateAssetCollection(csp::systems::AssetSystem* assetSystem, const csp::common::Optional<csp::common::String>& spaceId,
    const csp::common::Optional<csp::common::String>& parentId, const csp::common::String& name,
    const csp::common::Optional<csp::systems::EAssetCollectionType>& assetCollectionType,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, csp::systems::AssetCollection& outAssetCollection)
{
    csp::systems::EAssetCollectionType type = csp::systems::EAssetCollectionType::DEFAULT;

    if (assetCollectionType.HasValue())
    {
        type = *assetCollectionType;
    }

    auto [Result] = Awaitable(&csp::systems::AssetSystem::CreateAssetCollection, assetSystem, spaceId, parentId, name, nullptr, type, tags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outAssetCollection = Result.GetAssetCollection();
}

void DeleteAssetCollection(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::DeleteAssetCollection, assetSystem, assetCollection).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void GetAssetCollections(
    csp::systems::AssetSystem* assetSystem, csp::systems::Space& space, csp::common::Array<csp::systems::AssetCollection>& outAssetCollections)
{
    csp::common::Array<csp::systems::EAssetCollectionType> prototypeTypes = { csp::systems::EAssetCollectionType::DEFAULT };
    csp::common::Array<csp::common::String> groupIds = { space.Id };

    auto [Result] = AWAIT_PRE(
        assetSystem, FindAssetCollections, RequestPredicate, nullptr, nullptr, nullptr, prototypeTypes, nullptr, groupIds, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& assetCollections = Result.GetAssetCollections();
    outAssetCollections = csp::common::Array<csp::systems::AssetCollection>(assetCollections.Size());

    for (size_t i = 0U; i < assetCollections.Size(); ++i)
    {
        outAssetCollections[i] = assetCollections[i];
    }
}

void GetAssetCollectionByName(
    csp::systems::AssetSystem* assetSystem, const csp::common::String& assetCollectionName, csp::systems::AssetCollection& outAssetCollection)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::GetAssetCollectionByName, assetSystem, assetCollectionName).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outAssetCollection = Result.GetAssetCollection();
}

void GetAssetCollectionsByIds(csp::systems::AssetSystem* assetSystem, const csp::common::Array<csp::common::String>& ids,
    csp::common::Array<csp::systems::AssetCollection>& outAssetCollections)
{
    EXPECT_FALSE(ids.IsEmpty());

    auto [Result]
        = AWAIT_PRE(assetSystem, FindAssetCollections, RequestPredicate, ids, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& assetCollections = Result.GetAssetCollections();
    outAssetCollections = csp::common::Array<csp::systems::AssetCollection>(assetCollections.Size());

    for (size_t i = 0U; i < assetCollections.Size(); ++i)
    {
        outAssetCollections[i] = assetCollections[i];
    }
}

void CreateAsset(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, const csp::common::String& name,
    const csp::common::Optional<csp::common::String>& thirdPartyPackagedAssetIdentifier,
    const csp::common::Optional<csp::systems::EThirdPartyPlatform>& thirdPartyPlatform, csp::systems::Asset& outAsset)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::CreateAsset, assetSystem, assetCollection, name, thirdPartyPackagedAssetIdentifier,
        thirdPartyPlatform, csp::systems::EAssetType::MODEL)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outAsset = Result.GetAsset();
}

void UploadAssetData(csp::systems::AssetSystem* assetSystem, const csp::systems::AssetCollection& assetCollection, const csp::systems::Asset& asset,
    const csp::systems::FileAssetDataSource& source, csp::common::String& outUri)
{
    auto [Result] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outUri = Result.GetUri();
}

void UploadAssetData(csp::systems::AssetSystem* assetSystem, const csp::systems::AssetCollection& assetCollection, const csp::systems::Asset& asset,
    const csp::systems::BufferAssetDataSource& source, csp::common::String& outUri)
{
    auto [Result] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outUri = Result.GetUri();
}

void GetAssetById(csp::systems::AssetSystem* assetSystem, const csp::common::String& assetCollectionId, const csp::common::String& assetId,
    csp::systems::Asset& outAsset)
{
    auto [Result] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollectionId, assetId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outAsset = Result.GetAsset();
}

void DeleteAsset(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, csp::systems::Asset& asset)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::DeleteAsset, assetSystem, assetCollection, asset).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void UpdateAsset(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& /*AssetCollection*/, csp::systems::Asset& asset)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::UpdateAsset, assetSystem, asset).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void GetAssetsInCollection(
    csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, csp::common::Array<csp::systems::Asset>& outAssets)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::GetAssetsInCollection, assetSystem, assetCollection).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& assets = Result.GetAssets();
    outAssets = csp::common::Array<csp::systems::Asset>(assets.Size());

    for (size_t i = 0U; i < assets.Size(); ++i)
    {
        outAssets[i] = assets[i];
    }
}

void GetAssetsByCollectionIds(
    csp::systems::AssetSystem* assetSystem, const csp::common::Array<csp::common::String>& ids, csp::common::Array<csp::systems::Asset>& outAssets)
{
    EXPECT_FALSE(ids.IsEmpty());

    auto [Result] = Awaitable(&csp::systems::AssetSystem::GetAssetsByCollectionIds, assetSystem, ids).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& assets = Result.GetAssets();
    outAssets = csp::common::Array<csp::systems::Asset>(assets.Size());

    for (size_t i = 0U; i < assets.Size(); ++i)
    {
        outAssets[i] = assets[i];
    }
}

void UpdateAssetCollectionMetadata(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection,
    const csp::common::Map<csp::common::String, csp::common::String>& inMetaData,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    csp::common::Map<csp::common::String, csp::common::String>& outMetaData)
{
    auto [Result] = Awaitable(&csp::systems::AssetSystem::UpdateAssetCollectionMetadata, assetSystem, assetCollection, inMetaData, tags)
                        .Await(RequestPredicate);
    auto resultAssetCollection = Result.GetAssetCollection();

    // Check Result Data has only changed MetData
    EXPECT_EQ(resultAssetCollection.Id, assetCollection.Id);

    EXPECT_EQ(resultAssetCollection.ParentId, assetCollection.ParentId);

    EXPECT_EQ(resultAssetCollection.Name, assetCollection.Name);

    EXPECT_EQ(resultAssetCollection.Id, assetCollection.Id);

    EXPECT_NE(resultAssetCollection.UpdatedAt, assetCollection.UpdatedAt);

    auto assetCollectionTags = resultAssetCollection.Tags;

    for (size_t i = 0U; i < assetCollectionTags.Size(); ++i)
    {
        EXPECT_EQ(assetCollectionTags[i], assetCollection.Tags[i]);
    }

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outMetaData = resultAssetCollection.GetMetadataImmutable();
}

void GetAssetCollectionCount(csp::systems::AssetSystem* assetSystem, const csp::common::Optional<csp::common::Array<csp::common::String>>& ids,
    const csp::common::Optional<csp::common::String>& parentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& names,
    const csp::common::Optional<csp::common::Array<csp::systems::EAssetCollectionType>>& types,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, uint64_t& count)
{

    auto [Result] = AWAIT_PRE(assetSystem, GetAssetCollectionCount, RequestPredicate, ids, parentId, names, types, tags, spaceIds);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    count = Result.GetCount();
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, AssetCollectionEqualityTest)
{
    const csp::systems::AssetCollection assetCollection1;
    csp::systems::AssetCollection assetCollection2;

    ASSERT_EQ(assetCollection1, assetCollection1);
    ASSERT_EQ(assetCollection1, assetCollection2);

    assetCollection2.Id = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.Name = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.Type = csp::systems::EAssetCollectionType::COMMENT;
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.Tags = { "Test" };
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.PointOfInterestId = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.ParentId = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.SpaceId = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.CreatedBy = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.CreatedAt = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.UpdatedBy = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.UpdatedAt = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.IsUnique = true;
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    assetCollection2.Version = "Test";
    ASSERT_NE(assetCollection1, assetCollection2);
    assetCollection2 = assetCollection1;

    csp::common::Map<csp::common::String, csp::common::String>& metadata = assetCollection2.GetMetadataMutable();
    metadata["TestKey"] = "TestValue";
    ASSERT_NE(assetCollection1, assetCollection2);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, CreateAssetCollectionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Get asset collections
    csp::common::Array<csp::systems::AssetCollection> assetCollections;
    GetAssetCollections(assetSystem, space, assetCollections);

    EXPECT_EQ(assetCollections.Size(), 1);
    EXPECT_EQ(assetCollections[0].Name, uniqueAssetCollectionName);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, CreateAssetCollectionNoSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create asset collection
    csp::systems::AssetCollection newAssetCollection;
    CreateAssetCollection(assetSystem, nullptr, nullptr, uniqueAssetCollectionName, nullptr, nullptr, newAssetCollection);

    // Get asset collections
    csp::systems::AssetCollection assetCollection;
    GetAssetCollectionByName(assetSystem, uniqueAssetCollectionName, assetCollection);

    EXPECT_EQ(assetCollection.Name, uniqueAssetCollectionName);
    EXPECT_TRUE(assetCollection.SpaceId.IsEmpty());

    // Delete asset collection
    DeleteAssetCollection(assetSystem, newAssetCollection);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, GetAssetCollectionsByIdsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName1[256];
    SPRINTF(uniqueAssetCollectionName1, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetCollectionName2[256];
    SPRINTF(uniqueAssetCollectionName2, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collections
    csp::systems::AssetCollection assetCollection1, assetCollection2;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName1, nullptr, nullptr, assetCollection1);
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName2, nullptr, nullptr, assetCollection2);

    // Get asset collections
    csp::common::Array<csp::systems::AssetCollection> assetCollections;
    GetAssetCollectionsByIds(assetSystem, { assetCollection1.Id, assetCollection2.Id }, assetCollections);

    EXPECT_EQ(assetCollections.Size(), 2);

    bool found1 = false, found2 = false;

    for (size_t i = 0; i < assetCollections.Size(); ++i)
    {
        auto& assetCollection = assetCollections[i];

        if (assetCollection.Id == assetCollection1.Id)
        {
            found1 = true;
        }
        else if (assetCollection.Id == assetCollection2.Id)
        {
            found2 = true;
        }
    }

    EXPECT_TRUE(found1 && found2);

    // Delete asset collections
    DeleteAssetCollection(assetSystem, assetCollection1);
    DeleteAssetCollection(assetSystem, assetCollection2);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, CreateAssetTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String thirdPartyPackagedAssetIdentifier;
    thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    std::cout << userId << "\n";

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, thirdPartyPackagedAssetIdentifier, nullptr, asset);

    // Get assets
    csp::common::Array<csp::systems::Asset> assets;
    GetAssetsInCollection(assetSystem, assetCollection, assets);

    EXPECT_EQ(assets.Size(), 1);
    EXPECT_EQ(assets[0].Name, uniqueAssetName);
    EXPECT_EQ(assets[0].ThirdPartyPackagedAssetIdentifier, thirdPartyPackagedAssetIdentifier);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, CreateAssetNoSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String thirdPartyPackagedAssetIdentifier;
    thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    std::cout << userId << "\n";

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, nullptr, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, thirdPartyPackagedAssetIdentifier, nullptr, asset);

    // Get assets
    csp::common::Array<csp::systems::Asset> assets;
    GetAssetsInCollection(assetSystem, assetCollection, assets);

    EXPECT_EQ(assets.Size(), 1);
    EXPECT_EQ(assets[0].Name, uniqueAssetName);
    EXPECT_EQ(assets[0].ThirdPartyPackagedAssetIdentifier, thirdPartyPackagedAssetIdentifier);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UpdateExternalUriAssetTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";
    const char* testExternalUri = "https://github.com/KhronosGroup/glTF-Sample-Models/raw/master/2.0/Duck/glTF-Binary/Duck.glb";
    const char* testExternalMimeType = "model/gltf-binary";
    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String thirdPartyPackagedAssetIdentifier;
    thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, thirdPartyPackagedAssetIdentifier, nullptr, asset);

    // Get assets
    csp::common::Array<csp::systems::Asset> assets;
    GetAssetsInCollection(assetSystem, assetCollection, assets);

    EXPECT_EQ(assets.Size(), 1);
    EXPECT_EQ(assets[0].Name, uniqueAssetName);
    EXPECT_EQ(assets[0].ThirdPartyPackagedAssetIdentifier, thirdPartyPackagedAssetIdentifier);
    EXPECT_EQ(assets[0].Uri, "");

    assets[0].ExternalUri = testExternalUri;
    assets[0].ExternalMimeType = testExternalMimeType;

    auto [Result] = Awaitable(&csp::systems::AssetSystem::UpdateAsset, assetSystem, assets[0]).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    // Get assets
    GetAssetsInCollection(assetSystem, assetCollection, assets);

    EXPECT_EQ(Result.GetAsset().Uri, testExternalUri);
    EXPECT_EQ(Result.GetAsset().MimeType, testExternalMimeType);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, GetAssetsByCollectionIdsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName1[256], uniqueAssetCollectionName2[256];
    SPRINTF(uniqueAssetCollectionName1, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());
    SPRINTF(uniqueAssetCollectionName2, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName1[256], uniqueAssetName2[256], uniqueAssetName3[256];
    SPRINTF(uniqueAssetName1, "%s-%s", testAssetName, GetUniqueString().c_str());
    SPRINTF(uniqueAssetName2, "%s-%s", testAssetName, GetUniqueString().c_str());
    SPRINTF(uniqueAssetName3, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collections
    csp::systems::AssetCollection assetCollection1, assetCollection2;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName1, nullptr, nullptr, assetCollection1);
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName2, nullptr, nullptr, assetCollection2);

    // Create assets
    csp::systems::Asset asset1, asset2, asset3;
    CreateAsset(assetSystem, assetCollection1, uniqueAssetName1, nullptr, nullptr, asset1);
    CreateAsset(assetSystem, assetCollection1, uniqueAssetName2, nullptr, nullptr, asset2);
    CreateAsset(assetSystem, assetCollection2, uniqueAssetName3, nullptr, nullptr, asset3);

    // Get assets
    csp::common::Array<csp::systems::Asset> assets;
    GetAssetsByCollectionIds(assetSystem, { assetCollection1.Id, assetCollection2.Id }, assets);

    EXPECT_EQ(assets.Size(), 3);

    bool found1 = false, found2 = false, found3 = false;

    for (size_t i = 0; i < assets.Size(); ++i)
    {
        auto& asset = assets[i];

        if (asset.Id == asset1.Id)
        {
            found1 = true;
        }
        else if (asset.Id == asset2.Id)
        {
            found2 = true;
        }
        else if (asset.Id == asset3.Id)
        {
            found3 = true;
        }
    }

    EXPECT_TRUE(found1 && found2 && found3);

    // Delete assets
    DeleteAsset(assetSystem, assetCollection2, asset3);
    DeleteAsset(assetSystem, assetCollection1, asset2);
    DeleteAsset(assetSystem, assetCollection1, asset1);

    // Delete asset collections
    DeleteAssetCollection(assetSystem, assetCollection2);
    DeleteAssetCollection(assetSystem, assetCollection1);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, FindAssetCollectionsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName1[256];
    SPRINTF(uniqueAssetCollectionName1, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetCollectionName2[256];
    SPRINTF(uniqueAssetCollectionName2, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetCollectionName3[256];
    SPRINTF(uniqueAssetCollectionName3, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);
    const auto tag = csp::common::Array<csp::common::String>({ space.Id });

    csp::systems::AssetCollection assetCollection1, assetCollection2, assetCollection3;
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName1, csp::systems::EAssetCollectionType::SPACE_THUMBNAIL, nullptr, assetCollection1);
    CreateAssetCollection(
        assetSystem, space.Id, nullptr, uniqueAssetCollectionName2, csp::systems::EAssetCollectionType::SPACE_THUMBNAIL, tag, assetCollection2);
    CreateAssetCollection(assetSystem, space.Id, assetCollection1.Id, uniqueAssetCollectionName3, nullptr, nullptr, assetCollection3);

    // Search by space
    {
        csp::common::Array<csp::common::String> spaceIds = { space.Id };

        auto [Result]
            = AWAIT_PRE(assetSystem, FindAssetCollections, RequestPredicate, nullptr, nullptr, nullptr, nullptr, nullptr, spaceIds, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 4);
    }

    // Search by parentId
    {
        auto [Result] = AWAIT_PRE(
            assetSystem, FindAssetCollections, RequestPredicate, nullptr, assetCollection1.Id, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 1);
        EXPECT_EQ(Result.GetAssetCollections()[0].Id, assetCollection3.Id);
        EXPECT_EQ(Result.GetAssetCollections()[0].Name, assetCollection3.Name);
    }

    // Search by Tag
    {
        auto [Result]
            = AWAIT_PRE(assetSystem, FindAssetCollections, RequestPredicate, nullptr, nullptr, nullptr, nullptr, tag, nullptr, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 1);
        EXPECT_EQ(Result.GetAssetCollections()[0].Id, assetCollection2.Id);
        EXPECT_EQ(Result.GetAssetCollections()[0].Name, assetCollection2.Name);
    }

    // Search by names and types
    {
        csp::common::Array<csp::common::String> assetNames = { uniqueAssetCollectionName1, uniqueAssetCollectionName2 };

        // Search for Default types with these names
        csp::common::Array<csp::systems::EAssetCollectionType> searchTypes = { csp::systems::EAssetCollectionType::DEFAULT };

        auto [EmptyResult] = AWAIT_PRE(
            assetSystem, FindAssetCollections, RequestPredicate, nullptr, nullptr, assetNames, searchTypes, nullptr, nullptr, nullptr, nullptr);

        EXPECT_EQ(EmptyResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(EmptyResult.GetAssetCollections().Size(), 0);

        // Then search names and space thumbnail type
        searchTypes = { csp::systems::EAssetCollectionType::SPACE_THUMBNAIL };

        auto [Result] = AWAIT_PRE(
            assetSystem, FindAssetCollections, RequestPredicate, nullptr, nullptr, assetNames, searchTypes, nullptr, nullptr, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 2);

        bool foundFirstAssetCollection = false, foundSecondAssetCollection = false;

        const auto& retrievedAssetCollections = Result.GetAssetCollections();

        for (size_t idx = 0; idx < retrievedAssetCollections.Size(); ++idx)
        {
            auto& currentAsset = retrievedAssetCollections[idx];

            if (currentAsset.Id == assetCollection1.Id)
            {
                foundFirstAssetCollection = true;
            }
            else if (currentAsset.Id == assetCollection2.Id)
            {
                foundSecondAssetCollection = true;
            }
        }

        EXPECT_EQ(foundFirstAssetCollection && foundSecondAssetCollection, true);
    }

    // Test Pagination
    {
        csp::common::Array<csp::common::String> spaceIds = { space.Id };

        auto [Result] = AWAIT_PRE(assetSystem, FindAssetCollections, RequestPredicate, nullptr, nullptr, nullptr, nullptr, nullptr, spaceIds, 1, 1);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 1);
    }

    DeleteAssetCollection(assetSystem, assetCollection3);
    DeleteAssetCollection(assetSystem, assetCollection1);
    DeleteAssetCollection(assetSystem, assetCollection2);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, GetAssetsByDifferentCriteriaTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueFirstAssetName[256];
    SPRINTF(uniqueFirstAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    char uniqueSecondAssetName[256];
    SPRINTF(uniqueSecondAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    csp::systems::Asset firstAsset;
    CreateAsset(assetSystem, assetCollection, uniqueFirstAssetName, nullptr, nullptr, firstAsset);

    csp::systems::Asset secondAsset;
    CreateAsset(assetSystem, assetCollection, uniqueSecondAssetName, nullptr, nullptr, secondAsset);

    {
        // search by asset id
        csp::common::Array<csp::common::String> assetIds = { firstAsset.Id };
        auto [Result] = AWAIT_PRE(assetSystem, GetAssetsByCriteria, RequestPredicate, { assetCollection.Id }, assetIds, nullptr, nullptr);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssets().Size(), 1);
        EXPECT_EQ(Result.GetAssets()[0].Id, firstAsset.Id);
        EXPECT_EQ(Result.GetAssets()[0].Name, firstAsset.Name);
    }
    {
        // search by asset name
        csp::common::Array<csp::common::String> assetNames = { firstAsset.Name };
        auto [Result] = AWAIT_PRE(assetSystem, GetAssetsByCriteria, RequestPredicate, { assetCollection.Id }, nullptr, assetNames, nullptr);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssets().Size(), 1);
        EXPECT_EQ(Result.GetAssets()[0].Id, firstAsset.Id);
        EXPECT_EQ(Result.GetAssets()[0].Name, firstAsset.Name);
    }
    {
        // search by asset names and types, both assets are of type Model
        csp::common::Array<csp::common::String> assetNames = { firstAsset.Name, secondAsset.Name };

        csp::common::Array<csp::systems::EAssetType> assetTypes = { csp::systems::EAssetType::VIDEO };
        auto [EmptyResult] = AWAIT_PRE(assetSystem, GetAssetsByCriteria, RequestPredicate, { assetCollection.Id }, nullptr, assetNames, assetTypes);
        EXPECT_EQ(EmptyResult.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(EmptyResult.GetAssets().Size(), 0);

        // next to Model append Video too
        assetTypes = { csp::systems::EAssetType::VIDEO, csp::systems::EAssetType::MODEL };
        auto [Result] = AWAIT_PRE(assetSystem, GetAssetsByCriteria, RequestPredicate, { assetCollection.Id }, nullptr, assetNames, assetTypes);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssets().Size(), 2);

        bool foundFirstAsset = false, foundSecondAsset = false;

        const auto& retrievedAssets = Result.GetAssets();

        for (size_t idx = 0; idx < retrievedAssets.Size(); ++idx)
        {
            auto& currentAsset = retrievedAssets[idx];

            if (currentAsset.Id == firstAsset.Id)
            {
                foundFirstAsset = true;
            }
            else if (currentAsset.Id == secondAsset.Id)
            {
                foundSecondAsset = true;
            }
        }

        EXPECT_EQ(foundFirstAsset && foundSecondAsset, true);
    }

    DeleteAsset(assetSystem, assetCollection, firstAsset);
    DeleteAsset(assetSystem, assetCollection, secondAsset);
    DeleteAssetCollection(assetSystem, assetCollection);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, GetAssetsFromMultipleAssetCollectionsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueFirstAssetCollectionName[256];
    SPRINTF(uniqueFirstAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueSecondAssetCollectionName[256];
    SPRINTF(uniqueSecondAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueFirstAssetName[256];
    SPRINTF(uniqueFirstAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    char uniqueSecondAssetName[256];
    SPRINTF(uniqueSecondAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    csp::systems::AssetCollection firstAssetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueFirstAssetCollectionName, nullptr, nullptr, firstAssetCollection);

    csp::systems::Asset firstAsset;
    CreateAsset(assetSystem, firstAssetCollection, uniqueFirstAssetName, nullptr, nullptr, firstAsset);

    csp::systems::AssetCollection secondAssetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueSecondAssetCollectionName, nullptr, nullptr, secondAssetCollection);

    csp::systems::Asset secondAsset;
    CreateAsset(assetSystem, secondAssetCollection, uniqueSecondAssetName, nullptr, nullptr, secondAsset);

    //{
    //	// try to search but don't specify any asset collection Ids, only add one Asset Id though
    //	csp::common::Array<csp::common::String> AssetIds = {FirstAsset.Id};
    //	csp::common::Array<csp::common::String> AssetCollIds;
    //	auto [Result] = AWAIT_PRE(AssetSystem, GetAssetsByCriteria, RequestPredicate, AssetCollIds, AssetIds, nullptr, nullptr);
    //	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    //}
    {
        // search by both asset collection Ids at the same time
        csp::common::Array<csp::common::String> assetCollectionIds = { firstAssetCollection.Id, secondAssetCollection.Id };
        auto [Result] = AWAIT_PRE(assetSystem, GetAssetsByCriteria, RequestPredicate, assetCollectionIds, nullptr, nullptr, nullptr);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssets().Size(), 2);
        const auto& retrievedAssets = Result.GetAssets();

        bool foundFirstAsset = false, foundSecondAsset = false;
        for (size_t idx = 0; idx < retrievedAssets.Size(); ++idx)
        {
            auto& currentAsset = retrievedAssets[idx];

            if (currentAsset.Id == firstAsset.Id)
            {
                foundFirstAsset = true;
            }
            else if (currentAsset.Id == secondAsset.Id)
            {
                foundSecondAsset = true;
            }
        }

        EXPECT_EQ(foundFirstAsset && foundSecondAsset, true);
    }
    {
        // search by both asset collection Ids and only one Asset Id
        csp::common::Array<csp::common::String> assetCollectionIds = { firstAssetCollection.Id, secondAssetCollection.Id };
        csp::common::Array<csp::common::String> assetIds = { secondAsset.Id };
        auto [Result] = AWAIT_PRE(assetSystem, GetAssetsByCriteria, RequestPredicate, assetCollectionIds, assetIds, nullptr, nullptr);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetAssets().Size(), 1);
        EXPECT_EQ(Result.GetAssets()[0].Id, secondAsset.Id);
        EXPECT_EQ(Result.GetAssets()[0].Name, secondAsset.Name);
    }

    DeleteAsset(assetSystem, firstAssetCollection, firstAsset);
    DeleteAsset(assetSystem, secondAssetCollection, secondAsset);
    DeleteAssetCollection(assetSystem, firstAssetCollection);
    DeleteAssetCollection(assetSystem, secondAssetCollection);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UploadAssetAsFileTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto filePath = std::filesystem::absolute("assets/test.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();
    const csp::common::String& fileNoMimeType = "";
    const csp::common::String& fileMimeType = "application/json";

    printf("Uploading asset data without mime type...\n");

    // Upload data
    auto [UploadNoMimeResult] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(UploadNoMimeResult.GetResultCode(), csp::systems::EResultCode::Success);

    asset.Uri = UploadNoMimeResult.GetUri();

    printf("Getting asset to check for default mime type.\n");

    auto [AssetNoMimeResult] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollection.Id, asset.Id);

    EXPECT_NE(AssetNoMimeResult.GetAsset().MimeType, fileNoMimeType);
    EXPECT_EQ(AssetNoMimeResult.GetAsset().MimeType, "application/octet-stream");

    // Set a mime type
    source.SetMimeType(fileMimeType);

    printf("Uploading asset data with correct mime type...\n");

    // Upload data with MimeType
    auto [UploadResult] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(UploadResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(UploadResult.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    asset.Uri = UploadResult.GetUri();

    printf("Getting asset to check for correct mime type.\n");

    auto [AssetResult] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollection.Id, asset.Id);

    EXPECT_EQ(AssetResult.GetAsset().MimeType, fileMimeType);

    printf("Downloading asset data...\n");

    // Get data
    auto [Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, asset);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t downloadedAssetDataSize = Result.GetDataLength();
    auto downloadedAssetData = new uint8_t[downloadedAssetDataSize];
    memcpy(downloadedAssetData, Result.GetData(), downloadedAssetDataSize);

    FILE* file = fopen(filePath.string().c_str(), "rb");
    uintmax_t fileSize = std::filesystem::file_size(filePath);
    auto* fileData = new unsigned char[fileSize];
    fread(fileData, fileSize, 1, file);
    fclose(file);

    EXPECT_EQ(downloadedAssetDataSize, fileSize);
    EXPECT_EQ(memcmp(downloadedAssetData, fileData, fileSize), 0);

    delete[] fileData;
    delete[] downloadedAssetData;

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UploadAssetAsIncorrectFileTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto filePath = std::filesystem::absolute("assets/Incorrect_File.jpg");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    // Upload data
    auto [Result] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::AssetInvalidFileContents);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UploadAssetAsFileNoSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, nullptr, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto filePath = std::filesystem::absolute("assets/test.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();
    const csp::common::String& fileNoMimeType = "";
    const csp::common::String& fileMimeType = "application/json";

    printf("Uploading asset data without mime type...\n");

    // Upload data
    auto [UploadNoMimeResult] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(UploadNoMimeResult.GetResultCode(), csp::systems::EResultCode::Success);

    asset.Uri = UploadNoMimeResult.GetUri();

    printf("Getting asset to check for default mime type.\n");

    auto [AssetNoMimeResult] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollection.Id, asset.Id);

    EXPECT_NE(AssetNoMimeResult.GetAsset().MimeType, fileNoMimeType);
    EXPECT_EQ(AssetNoMimeResult.GetAsset().MimeType, "application/octet-stream");

    // Set a mime type
    source.SetMimeType(fileMimeType);

    printf("Uploading asset data with correct mime type...\n");

    // Upload data with MimeType
    auto [UploadResult] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);

    EXPECT_EQ(UploadResult.GetResultCode(), csp::systems::EResultCode::Success);

    asset.Uri = UploadResult.GetUri();

    printf("Getting asset to check for correct mime type.\n");

    auto [AssetResult] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollection.Id, asset.Id);

    EXPECT_EQ(AssetResult.GetAsset().MimeType, fileMimeType);

    printf("Downloading asset data...\n");

    // Get data
    auto [Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, asset);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t downloadedAssetDataSize = Result.GetDataLength();
    auto downloadedAssetData = new uint8_t[downloadedAssetDataSize];
    memcpy(downloadedAssetData, Result.GetData(), downloadedAssetDataSize);

    FILE* file = fopen(filePath.string().c_str(), "rb");
    uintmax_t fileSize = std::filesystem::file_size(filePath);
    auto* fileData = new unsigned char[fileSize];
    fread(fileData, fileSize, 1, file);
    fclose(file);

    EXPECT_EQ(downloadedAssetDataSize, fileSize);
    EXPECT_EQ(memcmp(downloadedAssetData, fileData, fileSize), 0);

    delete[] fileData;
    delete[] downloadedAssetData;

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UploadAssetWithUnencodedSpace)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    constexpr const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    constexpr const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    constexpr const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    constexpr const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto filePath = std::filesystem::absolute("assets/TestWith Space.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    // Upload data
    auto [UploadResult] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);
    EXPECT_EQ(UploadResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get uploaded asset
    auto [AssetResult] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollection.Id, asset.Id);
    std::string uriStr = std::string { AssetResult.GetAsset().Uri.c_str() };

    // Check uri is encoded as expected
    EXPECT_TRUE(uriStr.find("TestWith%20Space") != std::string::npos);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UploadAssetWithEncodedSpace)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    constexpr const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    constexpr const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    constexpr const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    constexpr const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto filePath = std::filesystem::absolute("assets/TestWithEncoded%20Space.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    // Upload data
    auto [UploadResult] = AWAIT_PRE(assetSystem, UploadAssetData, RequestPredicateWithProgress, assetCollection, asset, source);
    EXPECT_EQ(UploadResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get uploaded asset
    auto [AssetResult] = AWAIT_PRE(assetSystem, GetAssetById, RequestPredicate, assetCollection.Id, asset.Id);
    std::string uriStr = std::string { AssetResult.GetAsset().Uri.c_str() };

    // Check uri is encoded as expected
    EXPECT_TRUE(uriStr.find("TestWithEncoded%20Space") != std::string::npos);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UploadAssetAsBufferTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    asset.FileName = "test.json";

    auto uploadFilePath = std::filesystem::absolute("assets/test.json");
    FILE* uploadFile = fopen(uploadFilePath.string().c_str(), "rb");
    uintmax_t uploadFileSize = std::filesystem::file_size(uploadFilePath);
    auto* uploadFileData = new unsigned char[uploadFileSize];
    fread(uploadFileData, uploadFileSize, 1, uploadFile);
    fclose(uploadFile);

    csp::systems::BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData;
    bufferSource.BufferLength = uploadFileSize;

    bufferSource.SetMimeType("application/json");

    printf("Uploading asset data...\n");

    // Upload data
    UploadAssetData(assetSystem, assetCollection, asset, bufferSource, asset.Uri);

    printf("Downloading asset data...\n");

    // Get data
    auto [Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, asset);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    size_t downloadedAssetDataSize = Result.GetDataLength();
    auto downloadedAssetData = new uint8_t[downloadedAssetDataSize];
    memcpy(downloadedAssetData, Result.GetData(), downloadedAssetDataSize);

    EXPECT_EQ(downloadedAssetDataSize, uploadFileSize);
    EXPECT_EQ(memcmp(downloadedAssetData, uploadFileData, uploadFileSize), 0);

    delete[] uploadFileData;
    delete[] downloadedAssetData;

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UpdateAssetDataAsFileTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    // Upload data
    auto filePath = std::filesystem::absolute("assets/test.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    source.SetMimeType("application/json");

    printf("Uploading asset data...\n");

    csp::common::String uri;

    UploadAssetData(assetSystem, assetCollection, asset, source, uri);

    csp::systems::Asset updatedAsset;
    GetAssetById(assetSystem, assetCollection.Id, asset.Id, updatedAsset);

    EXPECT_EQ(asset.Id, updatedAsset.Id);

    // Replace data
    filePath = std::filesystem::absolute("assets/test2.json");
    source.FilePath = filePath.u8string().c_str();

    printf("Uploading new asset data...\n");

    csp::common::String uri2;
    UploadAssetData(assetSystem, assetCollection, asset, source, uri2);

    EXPECT_NE(uri, uri2);

    csp::systems::Asset updatedAsset2;
    GetAssetById(assetSystem, assetCollection.Id, asset.Id, updatedAsset2);

    EXPECT_EQ(updatedAsset.Id, updatedAsset2.Id);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UpdateAssetDataAsBufferTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto& initialAssetId = asset.Id;

    // Upload data
    auto filePath = std::filesystem::absolute("assets/test.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    source.SetMimeType("application/json");

    printf("Uploading asset data...\n");

    csp::common::String uri;
    UploadAssetData(assetSystem, assetCollection, asset, source, uri);

    // Replace data
    asset.FileName = "test2.json";

    auto updateFilePath = std::filesystem::absolute("assets/test2.json");
    FILE* updateFile = fopen(updateFilePath.string().c_str(), "rb");
    uintmax_t updateFileSize = std::filesystem::file_size(updateFilePath);
    auto* updateFileData = new unsigned char[updateFileSize];
    fread(updateFileData, updateFileSize, 1, updateFile);
    fclose(updateFile);

    csp::systems::BufferAssetDataSource bufferSource;
    bufferSource.Buffer = updateFileData;
    bufferSource.BufferLength = updateFileSize;
    bufferSource.SetMimeType("application/json");

    printf("Uploading new asset data...\n");

    csp::common::String uri2;
    UploadAssetData(assetSystem, assetCollection, asset, bufferSource, uri2);

    EXPECT_NE(uri, uri2);

    csp::systems::Asset updatedAsset;
    GetAssetById(assetSystem, assetCollection.Id, asset.Id, updatedAsset);

    EXPECT_EQ(initialAssetId, updatedAsset.Id);

    delete[] updateFileData;

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, UpdateAssetCollectionMetadataTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    csp::common::Array<csp::common::String> tags = { "tag-test" };

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, tags, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueSpaceName, nullptr, tags, assetCollection);

    csp::systems::AssetCollection idAssetCollection;
    // Update MetaData
    csp::common::Map<csp::common::String, csp::common::String> metaDataMapIn;
    csp::common::Map<csp::common::String, csp::common::String> metaDataMapOut;
    metaDataMapIn[uniqueSpaceName] = uniqueSpaceName;

    UpdateAssetCollectionMetadata(assetSystem, assetCollection, metaDataMapIn, tags, metaDataMapOut);
    EXPECT_TRUE(metaDataMapOut.HasKey(uniqueSpaceName));

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, GetAssetDataSizeTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION";
    const char* testAssetName = "CSP-UNITTEST-ASSET";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, nullptr, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    // Upload data
    asset.FileName = "asimplejsonfile.json";
    csp::common::String assetData = "{ \"some_value\": 42 }";
    csp::systems::BufferAssetDataSource source;
    source.Buffer = (void*)assetData.c_str();
    source.BufferLength = assetData.Length();
    source.SetMimeType("application/json");

    printf("Uploading asset data...\n");

    csp::common::String uri;
    UploadAssetData(assetSystem, assetCollection, asset, source, uri);

    // Get updated asset
    csp::systems::Asset updatedAsset;
    GetAssetById(assetSystem, assetCollection.Id, asset.Id, updatedAsset);

    EXPECT_EQ(asset.Id, updatedAsset.Id);

    // Get asset data size
    {
        auto [Result] = AWAIT_PRE(assetSystem, GetAssetDataSize, RequestPredicate, updatedAsset);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetValue(), assetData.Length());
    }

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Log out
    LogOut(userSystem);
}
CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, ThirdPartyPackagedAssetIdentifierTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    csp::common::String thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";
    csp::common::String thirdPartyPackagedAssetIdentifierLocal = "OKO interoperable assets Test Local";

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    std::cout << userId << "\n";

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);

    // Get assets
    csp::common::Array<csp::systems::Asset> assets;
    GetAssetsInCollection(assetSystem, assetCollection, assets);

    EXPECT_EQ(assets.Size(), 1);
    EXPECT_EQ(assets[0].Name, uniqueAssetName);
    EXPECT_EQ(assets[0].ThirdPartyPackagedAssetIdentifier, "");
    EXPECT_EQ(assets[0].ThirdPartyPlatformType, csp::systems::EThirdPartyPlatform::None);
    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    CreateAsset(assetSystem, assetCollection, uniqueAssetName, thirdPartyPackagedAssetIdentifier, csp::systems::EThirdPartyPlatform::Unity, asset);

    // Get assets
    GetAssetsInCollection(assetSystem, assetCollection, assets);

    EXPECT_EQ(assets.Size(), 1);
    EXPECT_EQ(assets[0].Name, uniqueAssetName);
    EXPECT_EQ(assets[0].ThirdPartyPackagedAssetIdentifier, thirdPartyPackagedAssetIdentifier);
    EXPECT_EQ(assets[0].ThirdPartyPlatformType, csp::systems::EThirdPartyPlatform::Unity);

    assets[0].ThirdPartyPackagedAssetIdentifier = thirdPartyPackagedAssetIdentifierLocal;
    EXPECT_EQ(assets[0].ThirdPartyPackagedAssetIdentifier, thirdPartyPackagedAssetIdentifierLocal);
    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, AssetProcessedCallbackTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Setup Asset callback
    bool assetDetailBlobChangedCallbackCalled = false;
    csp::common::String callbackAssetId;

    auto assetDetailBlobChangedCallback
        = [&assetDetailBlobChangedCallbackCalled, &callbackAssetId](const csp::common::AssetDetailBlobChangedNetworkEventData& networkEventData)
    {
        if (assetDetailBlobChangedCallbackCalled)
        {
            return;
        }

        EXPECT_EQ(networkEventData.ChangeType, csp::common::EAssetChangeType::Created);
        EXPECT_EQ(networkEventData.AssetType, csp::systems::EAssetType::MODEL);

        callbackAssetId = networkEventData.AssetId;
        assetDetailBlobChangedCallbackCalled = true;
    };

    assetSystem->SetAssetDetailBlobChangedCallback(assetDetailBlobChangedCallback);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);

    // Upload data
    auto filePath = std::filesystem::absolute("assets/test.json");
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    source.SetMimeType("application/json");

    csp::common::String uri;
    UploadAssetData(assetSystem, assetCollection, asset, source, uri);

    WaitForCallback(assetDetailBlobChangedCallbackCalled);

    EXPECT_TRUE(assetDetailBlobChangedCallbackCalled);
    EXPECT_EQ(callbackAssetId, asset.Id);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, AssetProcessGracefulFailureCallbackTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Setup Asset callback
    bool assetDetailBlobChangedCallbackCalled = false;

    auto assetDetailBlobChangedCallback
        = [&assetDetailBlobChangedCallbackCalled](const csp::common::AssetDetailBlobChangedNetworkEventData& networkEventData)
    {
        if (assetDetailBlobChangedCallbackCalled)
        {
            return;
        }

        EXPECT_EQ(networkEventData.ChangeType, csp::common::EAssetChangeType::Invalid);
        EXPECT_EQ(networkEventData.AssetType, csp::systems::EAssetType::IMAGE);

        assetDetailBlobChangedCallbackCalled = true;
    };

    assetSystem->SetAssetDetailBlobChangedCallback(assetDetailBlobChangedCallback);

    csp::common::ReplicatedValue param1 = static_cast<int64_t>(csp::common::EAssetChangeType::Invalid);
    csp::common::ReplicatedValue param2 = "";
    csp::common::ReplicatedValue param3 = "";
    csp::common::ReplicatedValue param4 = "";
    csp::common::ReplicatedValue param5 = "";

    systemsManager.GetEventBus()->SendNetworkEventToClient(
        NetworkEventBus::StringFromNetworkEvent(NetworkEventBus::NetworkEvent::AssetDetailBlobChanged), { param1, param2, param3, param4, param5 },
        connection->GetClientId(), [](ErrorCode error) { EXPECT_EQ(error, ErrorCode::None); });

    // Wait for message
    WaitForCallback(assetDetailBlobChangedCallbackCalled);
    EXPECT_TRUE(assetDetailBlobChangedCallbackCalled);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, DownloadAssetDataInvalidURLTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Attempt to download asset
    {
        csp::systems::Asset asset;
        asset.Uri = "https://world-streaming.magnopus-dev.cloud/123456789/123456789/1/NotAnImage.PNG?t=1234567890123";

        auto [Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, asset);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 403);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(DISABLED_CSPEngine, AssetSystemTests, CopyAssetCollectionTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* spaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";
    auto filePath = std::filesystem::absolute("assets/test.json");

    char sourceSpaceName[256];
    SPRINTF(sourceSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char destSpaceName[256];
    SPRINTF(destSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::systems::AssetCollection sourceAssetCollection;

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create 'source' space and asset collection
    csp::systems::Space sourceSpace;
    {
        printf("Creating source space and asset collection.\n");

        CreateSpace(
            spaceSystem, sourceSpaceName, spaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, sourceSpace);

        char assetCollectionName[256];
        SPRINTF(assetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

        char assetName[256];
        SPRINTF(assetName, "%s-%s", testAssetName, GetUniqueString().c_str());

        // Create an asset collection that belongs to the source space with a single valid asset
        CreateAssetCollection(assetSystem, sourceSpace.Id, nullptr, assetCollectionName, nullptr, nullptr, sourceAssetCollection);

        // Create an asset that belongs to the source collection
        csp::systems::Asset asset;
        CreateAsset(assetSystem, sourceAssetCollection, assetName, nullptr, nullptr, asset);

        // Upload data for the source asset we have created
        csp::systems::FileAssetDataSource source;
        source.FilePath = filePath.u8string().c_str();
        source.SetMimeType("application/json");

        printf("Uploading source asset data...\n");

        csp::common::String uri;
        UploadAssetData(assetSystem, sourceAssetCollection, asset, source, uri);
    }

    // Create 'dest' space and invoke the copy
    csp::systems::Space destSpace;
    csp::common::Array<csp::systems::AssetCollection> destAssetCollections;
    {
        printf("Creating dest space and invoking the copy...\n");

        CreateSpace(
            spaceSystem, destSpaceName, spaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, destSpace);

        csp::common::Array<csp::systems::AssetCollection> sourceAssetCollections = { sourceAssetCollection };
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, sourceAssetCollections, destSpace.Id, false);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetHttpResultCode(), 200);

        destAssetCollections = Result.GetAssetCollections();
    }

    {
        printf("Validating the copied asset collection and its data...\n");

        EXPECT_EQ(destAssetCollections.Size(), 1);
        EXPECT_NE(destAssetCollections[0].Id, sourceAssetCollection.Id);
        EXPECT_EQ(destAssetCollections[0].SpaceId, destSpace.Id);
        EXPECT_EQ(destAssetCollections[0].Type, sourceAssetCollection.Type);
        EXPECT_EQ(destAssetCollections[0].Tags.Size(), 1);
        EXPECT_EQ(destAssetCollections[0].Tags[0],
            csp::common::String("origin-") + sourceAssetCollection.Id); // we expect the services to automatically denote the origin asset

        csp::common::Array<csp::systems::Asset> destAssets;
        GetAssetsInCollection(assetSystem, destAssetCollections[0], destAssets);

        EXPECT_EQ(destAssets.Size(), 1);

        // Get the copied data and compare it with our source
        auto [Result] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicateWithProgress, destAssets[0]);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        size_t downloadedAssetDataSize = Result.GetDataLength();
        auto downloadedAssetData = new uint8_t[downloadedAssetDataSize];
        memcpy(downloadedAssetData, Result.GetData(), downloadedAssetDataSize);

        FILE* file = fopen(filePath.string().c_str(), "rb");
        uintmax_t fileSize = std::filesystem::file_size(filePath);
        auto* fileData = new unsigned char[fileSize];
        fread(fileData, fileSize, 1, file);
        fclose(file);

        EXPECT_EQ(downloadedAssetDataSize, fileSize);
        EXPECT_EQ(memcmp(downloadedAssetData, fileData, fileSize), 0);
    }

    {
        printf("Validating that we must have at least one asset collection to copy...\n");

        RAIIMockLogger mockLogger {};
        const csp::common::String error = "No source asset collections were provided whilst attempting to perform a copy to another space.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

        const csp::common::Array<csp::systems::AssetCollection> assetCollections;
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, assetCollections, destSpace.Id, false);
        // This response is simulated by CSP prior to making the request to the service. Returns an invalid result.
        EXPECT_EQ(Result, MakeInvalid<csp::systems::AssetCollectionsResult>());
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 0);
    }

    {
        printf("Validating we cannot perform a copy if the asset has no space ID...\n");

        csp::systems::AssetCollection noSpaceIdAssetCollection;

        RAIIMockLogger mockLogger {};
        const csp::common::String error
            = "An asset with no space ID was provided whilst attempting to perform a copy to another space. All assets must have a valid space ID.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

        const csp::common::Array<csp::systems::AssetCollection> assetCollections = { noSpaceIdAssetCollection };
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, assetCollections, destSpace.Id, false);
        // This response is simulated by CSP prior to making the request to the service. Returns an invalid result.
        EXPECT_EQ(Result, MakeInvalid<csp::systems::AssetCollectionsResult>());
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 0);
    }

    {
        printf("Validating we cannot perform a copy of assets that belong to different spaces but still get the async response...\n");

        csp::systems::AssetCollection firstSpaceAssetCollection;
        firstSpaceAssetCollection.SpaceId = "123456";

        csp::systems::AssetCollection secondSpaceAssetCollection;
        secondSpaceAssetCollection.SpaceId = "456789";

        RAIIMockLogger mockLogger {};
        const csp::common::String error = "All asset collections must belong to the same space for a copy operation.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

        const csp::common::Array<csp::systems::AssetCollection> assetCollections = { firstSpaceAssetCollection, secondSpaceAssetCollection };
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, assetCollections, destSpace.Id, false);
        // This response is simulated by CSP prior to making the request to the service. Returns an invalid result.
        EXPECT_EQ(Result, MakeInvalid<csp::systems::AssetCollectionsResult>());
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 0);
    }

    {
        printf("Validating we encounter failures when providing malformed asset collection IDs...\n");

        csp::systems::AssetCollection firstSpaceAssetCollection;
        firstSpaceAssetCollection.SpaceId = sourceSpace.Id;
        firstSpaceAssetCollection.Id = "AnInvalidlId";

        const csp::common::Array<csp::systems::AssetCollection> assetCollections = { firstSpaceAssetCollection };
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, assetCollections, destSpace.Id, false);
        // This response is returned by the service.
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 400);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 0);
        // The response does not come with an x-errorcode.
        EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
    }

    {
        printf("Validating we encounter failures when a malformed destination space ID is provided...\n");

        const csp::common::String invalidDestSpaceId = "AnInvalidlId";
        const csp::common::Array<csp::systems::AssetCollection> assetCollections = { sourceAssetCollection };
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, assetCollections, invalidDestSpaceId, false);
        // This response is returned by the service.
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 400);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 0);
        // The response does not come with an x-errorcode.
        EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
    }

    {
        printf("Validating that we cannot copy an asset to the same space...\n");

        const csp::common::Array<csp::systems::AssetCollection> assetCollections = { sourceAssetCollection };
        auto [Result] = AWAIT_PRE(assetSystem, CopyAssetCollectionsToSpace, RequestPredicate, assetCollections, sourceSpace.Id, false);
        // This response is returned by the service.
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 400);
        EXPECT_EQ(Result.GetAssetCollections().Size(), 0);
        // The response does not come with an x-errorcode.
        EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
    }

    // Delete spaces
    DeleteSpace(spaceSystem, sourceSpace.Id);
    DeleteSpace(spaceSystem, destSpace.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AssetSystemTests, GetAssetCollectionCountTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName1[256];
    SPRINTF(uniqueAssetCollectionName1, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetCollectionName2[256];
    SPRINTF(uniqueAssetCollectionName2, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetCollectionName3[256];
    SPRINTF(uniqueAssetCollectionName3, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create asset collections

    csp::common::Array<csp::common::String> assetCollection1Tags { "TestTag" };

    csp::systems::AssetCollection assetCollection1, assetCollection2, assetCollection3;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName1, csp::systems::EAssetCollectionType::SPACE_THUMBNAIL,
        assetCollection1Tags, assetCollection1);
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName2, nullptr, nullptr, assetCollection2);
    CreateAssetCollection(assetSystem, space.Id, assetCollection1.Id, uniqueAssetCollectionName3, nullptr, nullptr, assetCollection3);

    // Get Asset collection count associated with their ids
    {
        uint64_t count = 0;
        GetAssetCollectionCount(assetSystem,
            csp::common::Array<csp::common::String> { assetCollection1.Id, assetCollection2.Id, assetCollection3.Id }, nullptr, nullptr, nullptr,
            nullptr, csp::common::Array<csp::common::String> { space.Id }, count);

        EXPECT_EQ(count, 3);
    }

    // Get Asset collection count associated with their parent id
    {
        uint64_t count = 0;
        GetAssetCollectionCount(
            assetSystem, nullptr, assetCollection1.Id, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { space.Id }, count);

        EXPECT_EQ(count, 1);
    }

    // Get Asset collection count associated with their names
    {
        uint64_t count = 0;
        GetAssetCollectionCount(assetSystem, nullptr, nullptr,
            csp::common::Array<csp::common::String> { uniqueAssetCollectionName1, uniqueAssetCollectionName2, uniqueAssetCollectionName3 }, nullptr,
            nullptr, csp::common::Array<csp::common::String> { space.Id }, count);

        EXPECT_EQ(count, 3);
    }

    // Get Asset collection count associated with their type
    {
        uint64_t count = 0;
        GetAssetCollectionCount(assetSystem, nullptr, nullptr, nullptr,
            csp::common::Array<csp::systems::EAssetCollectionType> { csp::systems::EAssetCollectionType::DEFAULT }, nullptr,
            csp::common::Array<csp::common::String> { space.Id }, count);

        EXPECT_EQ(count, 2);
    }

    // Get Asset collection count associated with their tags
    {
        uint64_t count = 0;
        GetAssetCollectionCount(
            assetSystem, nullptr, nullptr, nullptr, nullptr, assetCollection1Tags, csp::common::Array<csp::common::String> { space.Id }, count);

        EXPECT_EQ(count, 1);
    }

    // Get Asset collection count associated with their space ids only
    {
        uint64_t count = 0;
        GetAssetCollectionCount(
            assetSystem, nullptr, nullptr, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { space.Id }, count);

        // 3 + 1 for the space thumbnail
        EXPECT_EQ(count, 4);
    }

    // Cleanup
    DeleteAssetCollection(assetSystem, assetCollection1);
    DeleteAssetCollection(assetSystem, assetCollection2);

    DeleteSpace(spaceSystem, space.Id);

    LogOut(userSystem);
}

class AssetCollectionTypeDtoMock
    : public PublicTestBaseWithParam<std::tuple<csp::common::String, csp::systems::EAssetCollectionType, csp::common::String>>
{
};

TEST_P(AssetCollectionTypeDtoMock, AssetCollectionTypeDtoMockTest)
{
    const auto prototypeServiceMock = std::make_unique<csp::services::generated::prototypeservice::PrototypeApiMock>();

    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Error);

    // Test parameters
    const csp::common::String& dtoTypeString = std::get<0>(GetParam());
    const csp::systems::EAssetCollectionType expectedAssetCollectionType = std::get<1>(GetParam());
    const csp::common::String& expectedLogMessage = std::get<2>(GetParam());

    const csp::common::String& mockAssetCollectionId = "1234";

    // The promise
    std::promise<csp::systems::AssetCollectionResult> resultPromise;
    std::future<csp::systems::AssetCollectionResult> resultFuture = resultPromise.get_future();

    EXPECT_CALL(*prototypeServiceMock, prototypesIdGet)
        .WillOnce(
            [dtoTypeString](const chs_prototype::IPrototypeApiBase::prototypesIdGetParams& /*Params*/,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/)
            {
                chs_prototype::PrototypeDto dto;
                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);

                csp::web::HttpPayload payload;

                const csp::common::String requestBody = R"(
                {
                  "type": ")"
                    + dtoTypeString + R"("
                }
            )";

                payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                payload.SetContent(requestBody);

                response.GetMutablePayload() = payload;
                responseHandler->OnHttpResponse(response);
            });

    csp::systems::AssetCollectionResultCallback callback
        = [&resultPromise](const csp::systems::AssetCollectionResult& result) { resultPromise.set_value(result); };

    csp::services::ResponseHandlerPtr responseHandler = prototypeServiceMock->CreateHandler<csp::systems::AssetCollectionResultCallback,
        csp::systems::AssetCollectionResult, void, csp::services::generated::prototypeservice::PrototypeDto>(callback, nullptr);

    RAIIMockLogger mockLogger {};
    if (expectedLogMessage.IsEmpty() == false)
    {
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(expectedLogMessage))).Times(1);
    }

    prototypeServiceMock->prototypesIdGet({ mockAssetCollectionId }, responseHandler, csp::common::CancellationToken::Dummy());
    auto result = resultFuture.get();

    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
    EXPECT_EQ(result.GetAssetCollection().Type, expectedAssetCollectionType);
}

INSTANTIATE_TEST_SUITE_P(AssetSystemTests, AssetCollectionTypeDtoMock,
    testing::Values(std::make_tuple("Default", csp::systems::EAssetCollectionType::DEFAULT, ""),
        std::make_tuple("FoundationInternal", csp::systems::EAssetCollectionType::FOUNDATION_INTERNAL, ""),
        std::make_tuple("CommentContainer", csp::systems::EAssetCollectionType::COMMENT_CONTAINER, ""),
        std::make_tuple("Comment", csp::systems::EAssetCollectionType::COMMENT, ""),
        std::make_tuple("SpaceThumbnail", csp::systems::EAssetCollectionType::SPACE_THUMBNAIL, ""),
        std::make_tuple("NotARealType", csp::systems::EAssetCollectionType::DEFAULT,
            "Encountered unknown prototype type whilst processing an asset collection DTO:")));
