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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void GetLODChain(csp::systems::AssetSystem* AssetSystem, const csp::systems::AssetCollection& AssetCollection, csp::systems::LODChain& OutLODChain)
{
    auto [Result] = AWAIT_PRE(AssetSystem, GetLODChain, RequestPredicate, AssetCollection);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutLODChain = Result.GetLODChain();

    EXPECT_EQ(OutLODChain.AssetCollectionId, AssetCollection.Id);
}

void RegisterAssetToLODChain(
    csp::systems::AssetSystem* AssetSystem, const csp::systems::AssetCollection& AssetCollection, csp::systems::Asset& Asset, int LODLevel)
{
    auto [Result] = AWAIT_PRE(AssetSystem, RegisterAssetToLODChain, RequestPredicate, AssetCollection, Asset, LODLevel);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    Asset = Result.GetAsset();
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_LOD_TESTS || RUN_LOD_GET_EMPTY_LODCHAIN_TEST
CSP_PUBLIC_TEST(CSPEngine, LODTests, GetEmptyLODChainTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create collection
    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

    // Get LOD chain
    csp::systems::LODChain Chain;
    GetLODChain(AssetSystem, AssetCollection, Chain);

    EXPECT_TRUE(Chain.LODAssets.IsEmpty());

    // Cleanup
    DeleteAssetCollection(AssetSystem, AssetCollection);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOD_TESTS || RUN_LOD_REGISTER_ASSETS_TO_LODCHAIN_TEST
CSP_PUBLIC_TEST(CSPEngine, LODTests, RegisterAssetsToLODChainTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
    const char* TestAssetName1 = "OLY-UNITTEST-ASSET1-REWIND";
    const char* TestAssetName2 = "OLY-UNITTEST-ASSET2-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    char UniqueAssetName1[256];
    SPRINTF(UniqueAssetName1, "%s-%s", TestAssetName1, GetUniqueString().c_str());

    char UniqueAssetName2[256];
    SPRINTF(UniqueAssetName2, "%s-%s", TestAssetName2, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Create collection
    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

    // Create assets
    csp::systems::Asset Asset1;
    CreateAsset(AssetSystem, AssetCollection, UniqueAssetName1, nullptr, nullptr, Asset1);

    csp::systems::Asset Asset2;
    CreateAsset(AssetSystem, AssetCollection, UniqueAssetName2, nullptr, nullptr, Asset2);

    // Register to LOD chain
    RegisterAssetToLODChain(AssetSystem, AssetCollection, Asset1, 0);
    RegisterAssetToLODChain(AssetSystem, AssetCollection, Asset2, 1);

    // Get LOD chain
    csp::systems::LODChain Chain;
    GetLODChain(AssetSystem, AssetCollection, Chain);

    EXPECT_EQ(Chain.LODAssets.Size(), 2);

    EXPECT_EQ(Chain.LODAssets[0].Level, 0);
    EXPECT_EQ(Chain.LODAssets[0].Asset.Id, Asset1.Id);

    EXPECT_EQ(Chain.LODAssets[1].Level, 1);
    EXPECT_EQ(Chain.LODAssets[1].Asset.Id, Asset2.Id);

    // Cleanup
    DeleteAsset(AssetSystem, AssetCollection, Asset1);
    DeleteAsset(AssetSystem, AssetCollection, Asset2);

    DeleteAssetCollection(AssetSystem, AssetCollection);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif
