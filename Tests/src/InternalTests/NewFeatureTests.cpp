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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_NEWFEATURE_TESTS)

#include "CSP/CSPFoundation.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "Memory/Memory.h"
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
            csp::common::List<int> List;

            EXPECT_TRUE(List.Size() == 0);
        }
        {
            csp::common::List<int> List;
            List.Append(42);

            EXPECT_TRUE(List.Size() == 1);
            EXPECT_TRUE(List[0] == 42);
        }
        {
            csp::common::List<int> List = { 42, 1337, 80085 };

            EXPECT_TRUE(List.Size() == 3);
            EXPECT_TRUE(List[0] == 42 && List[1] == 1337 && List[2] == 80085);
        }
        {
            csp::common::List<int> List = { 1, 2, 3, 4, 5 };
            List.Remove(2);

            EXPECT_TRUE(List.Size() == 4);
            EXPECT_TRUE(List[0] == 1 && List[1] == 2 && List[2] == 4 && List[3] == 5);
        }
        {
            csp::common::List<int> List;
            List.Append(1);
            List.Append(2);
            List.Append(3);
            List.Append(4);
            List.Append(5);

            EXPECT_TRUE(List.Size() == 5);
            EXPECT_TRUE(List[0] == 1 && List[1] == 2 && List[2] == 3 && List[3] == 4 && List[4] == 5);
        }
        {
            csp::common::List<int> List1 { 1, 2, 3 };
            csp::common::List<int> List2(List1);

            EXPECT_TRUE(List1.Size() == 3);
            EXPECT_TRUE(List1[0] == 1 && List1[1] == 2 && List1[2] == 3);
            EXPECT_TRUE(List2.Size() == 3);
            EXPECT_TRUE(List2[0] == 1 && List2[1] == 2 && List2[2] == 3);
        }
        {
            csp::common::List<csp::common::String> List;
            csp::common::String ListItem = "test item";
            List.Append(ListItem);

            EXPECT_TRUE(List[0] == ListItem);
        }
        {
            csp::common::List<csp::common::String> List;
            csp::common::String Item = "test item";
            List.Append(Item);

            EXPECT_TRUE(List[0] == "test item");
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
            csp::common::Map<int, csp::common::String> Map;
            Map[42] = "asd";

            EXPECT_TRUE(Map.Size() == 1);
            EXPECT_TRUE(Map.HasKey(42));
            EXPECT_TRUE(Map[42] == "asd");

            auto* _Keys = Map.Keys();
            auto& Keys = *_Keys;

            EXPECT_TRUE(Keys.Size() == 1);
            EXPECT_TRUE(Keys[0] == 42);

            CSP_DELETE(_Keys);

            auto* _Values = Map.Values();
            auto& Values = *_Values;

            EXPECT_TRUE(Values.Size() == 1);
            EXPECT_TRUE(Values[0] == "asd");

            CSP_DELETE(_Values);
        }
        {
            csp::common::Map<csp::common::String, int> Map;
            Map["asd"] = 42;

            EXPECT_TRUE(Map.Size() == 1);
            EXPECT_TRUE(Map.HasKey("asd"));
            EXPECT_TRUE(Map["asd"] == 42);

            auto* _Keys = Map.Keys();
            auto& Keys = *_Keys;

            EXPECT_TRUE(Keys.Size() == 1);
            EXPECT_TRUE(Keys[0] == "asd");

            CSP_DELETE(_Keys);

            auto* _Values = Map.Values();
            auto& Values = *_Values;

            EXPECT_TRUE(Values.Size() == 1);
            EXPECT_TRUE(Values[0] == 42);

            CSP_DELETE(_Values);
        }
        {
            csp::common::Map<int, csp::common::String> Map;
            Map[1] = "one";
            Map[2] = "too";
            Map[43] = "thorty free";

            Map.Remove(2);

            EXPECT_TRUE(Map.Size() == 2);

            auto* _Keys = Map.Keys();
            auto& Keys = *_Keys;

            EXPECT_TRUE(Keys.Size() == 2);
            EXPECT_TRUE(Keys[0] == 1 && Keys[1] == 43);
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

    auto DeviceId1 = csp::CSPFoundation::GetDeviceId();

    // Shutdown and re-initialise Foundation to verify we get the same DeviceID
    csp::CSPFoundation::Shutdown();
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto DeviceId2 = csp::CSPFoundation::GetDeviceId();

    ASSERT_EQ(DeviceId1, DeviceId2);

    csp::CSPFoundation::Shutdown();
}

class MyCoolClass
{
public:
    MyCoolClass() { CSP_LOG_MSG(csp::systems::LogLevel::Log, "MyCoolClass::MyCoolClass() called!"); }

    ~MyCoolClass() { CSP_LOG_MSG(csp::systems::LogLevel::Log, "MyCoolClass::~MyCoolClass() called!"); }
};

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, OptionalAssignmentOperatorTest)
{
    // assign a string into an empty optional
    csp::common::String String = "Just a random string";

    csp::common::Optional<csp::common::String> OptString;
    OptString = String;
    ASSERT_EQ(*OptString, String);

    // assign a different string into an non-empty optional
    csp::common::String String2 = "Another random string";
    OptString = String2;
    ASSERT_EQ(*OptString, String2);

    // assign a vector into an empty optional
    csp::common::Optional<std::vector<int>> OptVector;
    std::vector<int> Vector { 1, 2, 3 };
    OptVector = Vector;
    ASSERT_EQ(*OptVector, Vector);

    // assign a vector into an non-empty optional
    std::vector<int> Vector2 { 5, 9, 12, 15, 19 };
    OptVector = Vector2;
    ASSERT_EQ(*OptVector, Vector2);

    // Test Optional<T*> constructors
    {
        auto Instance = (MyCoolClass*)CSP_ALLOC(sizeof(MyCoolClass));
        new (Instance) MyCoolClass();
        csp::common::Optional<MyCoolClass> OptionalInstance(Instance);
    }

    {
        auto Instance = CSP_NEW MyCoolClass();
        csp::common::Optional<MyCoolClass> OptionalInstance(Instance);
    }

    {
        auto Instance = new MyCoolClass();
        csp::common::Optional<MyCoolClass> OptionalInstance(Instance, [](auto Pointer) { delete Pointer; });
    }
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODStyleVar)
{
    const csp::common::String TestLODStyleVar = "lod:0";
    csp::common::String LODStyleVar = csp::systems::CreateLODStyleVar(0);

    EXPECT_EQ(TestLODStyleVar, LODStyleVar);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, GetLODLevelFromStylesArray)
{
    const int TestLodLevel = 0;
    const csp::common::String TestLODStyleVar = "lod:0";

    csp::common::Array<csp::common::String> TestStyles { TestLODStyleVar };

    int LODLevel = csp::systems::GetLODLevelFromStylesArray(TestStyles);

    EXPECT_EQ(TestLodLevel, LODLevel);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, GetLODLevelFromStylesArrayOtherDataTest)
{
    const int TestLodLevel = 0;
    const csp::common::String TestLODStyleVar = "lod:0";
    const csp::common::String TestTagData = "TagData";

    csp::common::Array<csp::common::String> TestStyles { TestTagData, TestLODStyleVar };

    int LODLevel = csp::systems::GetLODLevelFromStylesArray(TestStyles);

    EXPECT_EQ(TestLodLevel, LODLevel);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODChainFromAssetsTest)
{
    const csp::common::String TestCollectionId = "TestCollectionId";
    const csp::common::String TestAssetId1 = "TestAssetId1";
    const csp::common::String TestAssetId2 = "TestAssetId2";
    const csp::common::String TestAssetId3 = "TestAssetId3";

    csp::systems::Asset TestAsset1, TestAsset2, TestAsset3;

    TestAsset1.Id = TestAssetId1;
    TestAsset2.Id = TestAssetId2;
    TestAsset3.Id = TestAssetId3;

    TestAsset1.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(0) };
    TestAsset2.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(1) };
    TestAsset3.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(2) };

    csp::common::Array<csp::systems::Asset> TestAssets { TestAsset1, TestAsset2, TestAsset3 };

    csp::systems::LODChain Chain = csp::systems::CreateLODChainFromAssets(TestAssets, TestCollectionId);

    EXPECT_EQ(Chain.AssetCollectionId, TestCollectionId);
    EXPECT_EQ(Chain.LODAssets.Size(), 3);

    EXPECT_EQ(Chain.LODAssets[0].Level, 0);
    EXPECT_EQ(Chain.LODAssets[0].Asset.Id, TestAssetId1);

    EXPECT_EQ(Chain.LODAssets[1].Level, 1);
    EXPECT_EQ(Chain.LODAssets[1].Asset.Id, TestAssetId2);

    EXPECT_EQ(Chain.LODAssets[2].Level, 2);
    EXPECT_EQ(Chain.LODAssets[2].Asset.Id, TestAssetId3);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODChainFromAssetsSingleModelTest)
{
    const csp::common::String TestCollectionId = "TestCollectionId";
    const csp::common::String TestAssetId1 = "TestAssetId1";

    csp::systems::Asset TestAsset1;
    TestAsset1.Id = TestAssetId1;

    // Don't set the style so we can test that this function returns the asset as the default model

    csp::common::Array<csp::systems::Asset> TestAssets { TestAsset1 };

    csp::systems::LODChain Chain = csp::systems::CreateLODChainFromAssets(TestAssets, TestCollectionId);

    EXPECT_EQ(Chain.AssetCollectionId, TestCollectionId);
    EXPECT_EQ(Chain.LODAssets.Size(), 1);

    EXPECT_EQ(Chain.LODAssets[0].Level, 0);
    EXPECT_EQ(Chain.LODAssets[0].Asset.Id, TestAssetId1);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, CreateLODChainFromAssetsMultipleModelTest)
{
    const csp::common::String TestCollectionId = "TestCollectionId";
    const csp::common::String TestAssetId1 = "TestAssetId1";
    const csp::common::String TestAssetId2 = "TestAssetId2";

    csp::systems::Asset TestAsset1, TestAsset2;
    TestAsset1.Id = TestAssetId1;
    TestAsset2.Id = TestAssetId2;

    // Don't set the styles so we can test that this return no LODs as it can't resolve a default

    csp::common::Array<csp::systems::Asset> TestAssets { TestAsset1, TestAsset2 };

    csp::systems::LODChain Chain = csp::systems::CreateLODChainFromAssets(TestAssets, TestCollectionId);

    EXPECT_EQ(Chain.AssetCollectionId, TestCollectionId);
    EXPECT_EQ(Chain.LODAssets.Size(), 0);
}

CSP_INTERNAL_TEST(CSPEngine, NewFeatureTests, ValidateNewLODLevelForChainTest)
{
    const csp::common::String TestCollectionId = "TestCollectionId";
    const csp::common::String TestAssetId1 = "TestAssetId1";
    const csp::common::String TestAssetId2 = "TestAssetId2";
    const csp::common::String TestAssetId3 = "TestAssetId3";

    csp::systems::Asset TestAsset1, TestAsset2, TestAsset3;
    TestAsset1.Id = TestAssetId1;
    TestAsset2.Id = TestAssetId2;
    TestAsset3.Id = TestAssetId3;

    TestAsset1.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(0) };
    TestAsset2.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(1) };
    TestAsset3.Styles = csp::common::Array<csp::common::String> { csp::systems::CreateLODStyleVar(2) };

    csp::common::Array<csp::systems::Asset> TestAssets { TestAsset1, TestAsset2, TestAsset3 };

    csp::systems::LODChain TestChain = csp::systems::CreateLODChainFromAssets(TestAssets, TestCollectionId);

    bool Valid = csp::systems::ValidateNewLODLevelForChain(TestChain, 3);
    EXPECT_TRUE(Valid);

    Valid = csp::systems::ValidateNewLODLevelForChain(TestChain, 2);
    EXPECT_FALSE(Valid);
}

#endif