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
#include "CSP/Common/Array.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <Awaitable.h>
#include <filesystem>

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void GetLODChain(csp::systems::AssetSystem* assetSystem, const csp::systems::AssetCollection& assetCollection, csp::systems::LODChain& outLodChain)
{
    auto [Result] = AWAIT_PRE(assetSystem, GetLODChain, RequestPredicate, assetCollection);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    outLodChain = Result.GetLODChain();

    EXPECT_EQ(outLodChain.AssetCollectionId, assetCollection.Id);
}

void RegisterAssetToLODChain(
    csp::systems::AssetSystem* assetSystem, const csp::systems::AssetCollection& assetCollection, csp::systems::Asset& asset, int lodLevel)
{
    auto [Result] = AWAIT_PRE(assetSystem, RegisterAssetToLODChain, RequestPredicate, assetCollection, asset, lodLevel);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    asset = Result.GetAsset();
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, LODTests, GetEmptyLODChainTest)
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

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Get LOD chain
    csp::systems::LODChain chain;
    GetLODChain(assetSystem, assetCollection, chain);

    EXPECT_TRUE(chain.LODAssets.IsEmpty());

    // Cleanup
    DeleteAssetCollection(assetSystem, assetCollection);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LODTests, RegisterAssetsToLODChainTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName1 = "CSP-UNITTEST-ASSET1-MAG";
    const char* testAssetName2 = "CSP-UNITTEST-ASSET2-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName1[256];
    SPRINTF(uniqueAssetName1, "%s-%s", testAssetName1, GetUniqueString().c_str());

    char uniqueAssetName2[256];
    SPRINTF(uniqueAssetName2, "%s-%s", testAssetName2, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Create collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create assets
    csp::systems::Asset asset1;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName1, nullptr, nullptr, asset1);

    csp::systems::Asset asset2;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName2, nullptr, nullptr, asset2);

    // Register to LOD chain
    RegisterAssetToLODChain(assetSystem, assetCollection, asset1, 0);
    RegisterAssetToLODChain(assetSystem, assetCollection, asset2, 1);

    // Get LOD chain
    csp::systems::LODChain chain;
    GetLODChain(assetSystem, assetCollection, chain);

    EXPECT_EQ(chain.LODAssets.Size(), 2);

    EXPECT_EQ(chain.LODAssets[0].Level, 0);
    EXPECT_EQ(chain.LODAssets[0].Asset.Id, asset1.Id);

    EXPECT_EQ(chain.LODAssets[1].Level, 1);
    EXPECT_EQ(chain.LODAssets[1].Asset.Id, asset2.Id);

    // Cleanup
    DeleteAsset(assetSystem, assetCollection, asset1);
    DeleteAsset(assetSystem, assetCollection, asset2);

    DeleteAssetCollection(assetSystem, assetCollection);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}
