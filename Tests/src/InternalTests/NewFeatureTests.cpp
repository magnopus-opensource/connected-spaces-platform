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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "Systems/Assets/LODHelpers.h"

#include "PublicTestBase.h"
#include "TestHelpers.h"

#include "Debug/Logging.h"

#include <gtest/gtest.h>

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, ListTest)
{
    try
    {
        {
            csp::common::List<int> list;

            EXPECT_TRUE(list.Size() == 0);
        }
        {
            csp::common::List<int> list;
            list.Append(42);

            EXPECT_TRUE(list.Size() == 1);
            EXPECT_TRUE(list[0] == 42);
        }
        {
            csp::common::List<int> list = { 42, 1337, 80085 };

            EXPECT_TRUE(list.Size() == 3);
            EXPECT_TRUE(list[0] == 42 && list[1] == 1337 && list[2] == 80085);
        }
        {
            csp::common::List<int> list = { 1, 2, 3, 4, 5 };
            list.Remove(2);

            EXPECT_TRUE(list.Size() == 4);
            EXPECT_TRUE(list[0] == 1 && list[1] == 2 && list[2] == 4 && list[3] == 5);
        }
        {
            csp::common::List<int> list;
            list.Append(1);
            list.Append(2);
            list.Append(3);
            list.Append(4);
            list.Append(5);

            EXPECT_TRUE(list.Size() == 5);
            EXPECT_TRUE(list[0] == 1 && list[1] == 2 && list[2] == 3 && list[3] == 4 && list[4] == 5);
        }
        {
            csp::common::List<int> list1 { 1, 2, 3 };
            csp::common::List<int> list2(list1);

            EXPECT_TRUE(list1.Size() == 3);
            EXPECT_TRUE(list1[0] == 1 && list1[1] == 2 && list1[2] == 3);
            EXPECT_TRUE(list2.Size() == 3);
            EXPECT_TRUE(list2[0] == 1 && list2[1] == 2 && list2[2] == 3);
        }
        {
            csp::common::List<csp::common::String> list;
            csp::common::String listItem = "test item";
            list.Append(listItem);

            EXPECT_TRUE(list[0] == listItem);
        }
        {
            csp::common::List<csp::common::String> list;
            csp::common::String item = "test item";
            list.Append(item);

            EXPECT_TRUE(list[0] == "test item");
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, MapTest)
{
    try
    {
        {
            csp::common::Map<int, csp::common::String> map;
            map[42] = "asd";

            EXPECT_TRUE(map.Size() == 1);
            EXPECT_TRUE(map.HasKey(42));
            EXPECT_TRUE(map[42] == "asd");

            auto* rawKeys = map.Keys();
            auto& keys = *rawKeys;

            EXPECT_TRUE(keys.Size() == 1);
            EXPECT_TRUE(keys[0] == 42);

            delete (rawKeys);

            auto* rawValues = map.Values();
            auto& values = *rawValues;

            EXPECT_TRUE(values.Size() == 1);
            EXPECT_TRUE(values[0] == "asd");

            delete (rawValues);
        }
        {
            csp::common::Map<csp::common::String, int> map;
            map["asd"] = 42;

            EXPECT_TRUE(map.Size() == 1);
            EXPECT_TRUE(map.HasKey("asd"));
            EXPECT_TRUE(map["asd"] == 42);

            auto* rawKeys = map.Keys();
            auto& keys = *rawKeys;

            EXPECT_TRUE(keys.Size() == 1);
            EXPECT_TRUE(keys[0] == "asd");

            delete (rawKeys);

            auto* rawValues = map.Values();
            auto& values = *rawValues;

            EXPECT_TRUE(values.Size() == 1);
            EXPECT_TRUE(values[0] == 42);

            delete (rawValues);
        }
        {
            csp::common::Map<int, csp::common::String> map;
            map[1] = "one";
            map[2] = "too";
            map[43] = "thorty free";

            map.Remove(2);

            EXPECT_TRUE(map.Size() == 2);

            auto* rawKeys = map.Keys();
            auto& keys = *rawKeys;

            EXPECT_TRUE(keys.Size() == 2);
            EXPECT_TRUE(keys[0] == 1 && keys[1] == 43);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, GetDeviceIdTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto deviceId1 = csp::CSPFoundation::GetDeviceId();

    // Shutdown and re-initialise Foundation to verify we get the same DeviceID
    csp::CSPFoundation::Shutdown();
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto deviceId2 = csp::CSPFoundation::GetDeviceId();

    ASSERT_EQ(deviceId1, deviceId2);

    csp::CSPFoundation::Shutdown();
}

class MyCoolClass
{
public:
    MyCoolClass() { CSP_LOG_MSG(csp::common::LogLevel::Log, "MyCoolClass::MyCoolClass() called!"); }

    ~MyCoolClass() { CSP_LOG_MSG(csp::common::LogLevel::Log, "MyCoolClass::~MyCoolClass() called!"); }
};

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, OptionalAssignmentOperatorTest)
{
    // assign a string into an empty optional
    csp::common::String string = "Just a random string";

    csp::common::Optional<csp::common::String> optString;
    optString = string;
    ASSERT_EQ(*optString, string);

    // assign a different string into an non-empty optional
    csp::common::String string2 = "Another random string";
    optString = string2;
    ASSERT_EQ(*optString, string2);

    // assign a vector into an empty optional
    csp::common::Optional<std::vector<int>> optVector;
    std::vector<int> vector { 1, 2, 3 };
    optVector = vector;
    ASSERT_EQ(*optVector, vector);

    // assign a vector into an non-empty optional
    std::vector<int> vector2 { 5, 9, 12, 15, 19 };
    optVector = vector2;
    ASSERT_EQ(*optVector, vector2);

    // Test Optional<T*> constructors
    {
        auto instance = (MyCoolClass*)std::malloc(sizeof(MyCoolClass));
        new (instance) MyCoolClass();
        csp::common::Optional<MyCoolClass> optionalInstance(instance);
    }

    {
        auto instance = new MyCoolClass();
        csp::common::Optional<MyCoolClass> optionalInstance(instance);
    }

    {
        auto instance = new MyCoolClass();
        csp::common::Optional<MyCoolClass> optionalInstance(instance, [](auto pointer) { delete pointer; });
    }
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODStyleVar)
{
    const csp::common::String testLodStyleVar = "lod:0";
    csp::common::String lodStyleVar = csp::systems::CreateLODStyleVar(0);

    EXPECT_EQ(testLodStyleVar, lodStyleVar);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, GetLODLevelFromStylesArray)
{
    const int testLodLevel = 0;
    const csp::common::String testLodStyleVar = "lod:0";

    csp::common::Array<csp::common::String> testStyles { testLodStyleVar };

    int lodLevel = csp::systems::GetLODLevelFromStylesArray(testStyles);

    EXPECT_EQ(testLodLevel, lodLevel);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, GetLODLevelFromStylesArrayOtherDataTest)
{
    const int testLodLevel = 0;
    const csp::common::String testLodStyleVar = "lod:0";
    const csp::common::String testTagData = "TagData";

    csp::common::Array<csp::common::String> testStyles { testTagData, testLodStyleVar };

    int lodLevel = csp::systems::GetLODLevelFromStylesArray(testStyles);

    EXPECT_EQ(testLodLevel, lodLevel);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODChainFromAssetsTest)
{
    const csp::common::String testCollectionId = "TestCollectionId";
    const csp::common::String testAssetId1 = "TestAssetId1";
    const csp::common::String testAssetId2 = "TestAssetId2";
    const csp::common::String testAssetId3 = "TestAssetId3";

    csp::systems::Asset testAsset1, testAsset2, testAsset3;

    testAsset1.Id = testAssetId1;
    testAsset2.Id = testAssetId2;
    testAsset3.Id = testAssetId3;

    testAsset1.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(0) };
    testAsset2.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(1) };
    testAsset3.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(2) };

    // Adding TestAssets to collection out of sequence to ensure they are
    // correctly sorted by the call to CreateLODChainFromAssets().
    csp::common::Array<csp::systems::Asset> testAssets { testAsset1, testAsset3, testAsset2 };

    csp::systems::LODChain chain = csp::systems::CreateLODChainFromAssets(testAssets, testCollectionId);

    EXPECT_EQ(chain.AssetCollectionId, testCollectionId);
    EXPECT_EQ(chain.LODAssets.Size(), 3);

    EXPECT_EQ(chain.LODAssets[0].Level, 0);
    EXPECT_EQ(chain.LODAssets[0].Asset.Id, testAssetId1);

    EXPECT_EQ(chain.LODAssets[1].Level, 1);
    EXPECT_EQ(chain.LODAssets[1].Asset.Id, testAssetId2);

    EXPECT_EQ(chain.LODAssets[2].Level, 2);
    EXPECT_EQ(chain.LODAssets[2].Asset.Id, testAssetId3);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODChainFromAssetsSingleModelTest)
{
    const csp::common::String testCollectionId = "TestCollectionId";
    const csp::common::String testAssetId1 = "TestAssetId1";

    csp::systems::Asset testAsset1;
    testAsset1.Id = testAssetId1;

    // Don't set the style so we can test that this function returns the asset as the default model

    csp::common::Array<csp::systems::Asset> testAssets { testAsset1 };

    csp::systems::LODChain chain = csp::systems::CreateLODChainFromAssets(testAssets, testCollectionId);

    EXPECT_EQ(chain.AssetCollectionId, testCollectionId);
    EXPECT_EQ(chain.LODAssets.Size(), 1);

    EXPECT_EQ(chain.LODAssets[0].Level, 0);
    EXPECT_EQ(chain.LODAssets[0].Asset.Id, testAssetId1);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODChainFromAssetsMultipleModelTest)
{
    const csp::common::String testCollectionId = "TestCollectionId";
    const csp::common::String testAssetId1 = "TestAssetId1";
    const csp::common::String testAssetId2 = "TestAssetId2";

    csp::systems::Asset testAsset1, testAsset2;
    testAsset1.Id = testAssetId1;
    testAsset2.Id = testAssetId2;

    // Don't set the styles so we can test that this return no LODs as it can't resolve a default

    csp::common::Array<csp::systems::Asset> testAssets { testAsset1, testAsset2 };

    csp::systems::LODChain chain = csp::systems::CreateLODChainFromAssets(testAssets, testCollectionId);

    EXPECT_EQ(chain.AssetCollectionId, testCollectionId);
    EXPECT_EQ(chain.LODAssets.Size(), 0);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, ValidateNewLODLevelForChainTest)
{
    const csp::common::String testCollectionId = "TestCollectionId";
    const csp::common::String testAssetId1 = "TestAssetId1";
    const csp::common::String testAssetId2 = "TestAssetId2";
    const csp::common::String testAssetId3 = "TestAssetId3";

    csp::systems::Asset testAsset1, testAsset2, testAsset3;
    testAsset1.Id = testAssetId1;
    testAsset2.Id = testAssetId2;
    testAsset3.Id = testAssetId3;

    testAsset1.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(0) };
    testAsset2.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(1) };
    testAsset3.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(2) };

    csp::common::Array<csp::systems::Asset> testAssets { testAsset1, testAsset2, testAsset3 };

    csp::systems::LODChain testChain = csp::systems::CreateLODChainFromAssets(testAssets, testCollectionId);

    bool valid = csp::systems::ValidateNewLODLevelForChain(testChain, 3);
    EXPECT_TRUE(valid);

    valid = csp::systems::ValidateNewLODLevelForChain(testChain, 2);
    EXPECT_FALSE(valid);
}
