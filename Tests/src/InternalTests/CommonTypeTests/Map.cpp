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

#include "CSP/Common/Map.h"

#include "CSP/Common/Optional.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

// Test case to check if default initialisation works correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapDefaultInitialisationTest)
{
    try
    {
        Map<int, int> instance;

        EXPECT_EQ(instance.Size(), 0);
    }
    catch (...)
    {
        FAIL();
    }
}

// Test case to check if elements are initialised with a list correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapInitialiseListTest)
{
    Map<int, String> myMap;
    myMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    // Check if the map contains the initial values
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_EQ(myMap[1], "One");
    EXPECT_TRUE(myMap.HasKey(2));
    EXPECT_EQ(myMap[2], "Two");
    EXPECT_TRUE(myMap.HasKey(3));
    EXPECT_EQ(myMap[3], "Three");

    // Check if the map does not contain an invalid key
    EXPECT_FALSE(myMap.HasKey(4));

    EXPECT_EQ(myMap.Size(), 3);
}

// Test case to check if elements are initialised with a Map correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapInitialiseMapTest)
{
    Map<int, String> oldMap;
    oldMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    Map<int, String> myMap(oldMap);

    // Check if the map contains the initial values
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_TRUE(myMap.HasKey(2));
    EXPECT_TRUE(myMap.HasKey(3));

    // Check if the map does not contain an invalid key
    EXPECT_FALSE(myMap.HasKey(4));

    EXPECT_EQ(myMap.Size(), 3);
}

// Test case to check if elements are initialised with a Map correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapMoveConstructorTest)
{
    Map<int, String> oldMap;
    oldMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    Map<int, String> myMap(std::move(oldMap));

    // Check if the map contains the initial values
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_TRUE(myMap.HasKey(2));
    EXPECT_TRUE(myMap.HasKey(3));

    // Check if the map does not contain an invalid key
    EXPECT_FALSE(myMap.HasKey(4));

    EXPECT_EQ(myMap.Size(), 3);
}

// Test case to check if elements are initialised with a Map correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapCopyAssignmentTest)
{
    Map<int, String> oldMap;
    oldMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    Map<int, String> myMap = { { 1, "Default" } };

    myMap = oldMap;

    // Check if the map contains the initial values
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_EQ(myMap[1], "One");
    EXPECT_TRUE(myMap.HasKey(2));
    EXPECT_TRUE(myMap.HasKey(3));

    // Check if the map does not contain an invalid key
    EXPECT_FALSE(myMap.HasKey(4));

    EXPECT_EQ(myMap.Size(), 3);
}

// Test case to check if elements are initialised with a Map correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapMoveAssignmentTest)
{
    Map<int, String> oldMap;
    oldMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    Map<int, String> myMap = { { 1, "Default" } };

    myMap = std::move(oldMap);

    // Check if the map contains the initial values
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_EQ(myMap[1], "One");
    EXPECT_TRUE(myMap.HasKey(2));
    EXPECT_TRUE(myMap.HasKey(3));

    // Check if the map does not contain an invalid key
    EXPECT_FALSE(myMap.HasKey(4));

    EXPECT_EQ(myMap.Size(), 3);
}

// Test case to check if elements are assigned correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapAssignmentTest)
{
    Map<int, String> myMap;
    myMap[1] = "One";
    myMap[2] = "Two";
    myMap[3] = "Three";

    // Check if the map contains the initial values
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_TRUE(myMap.HasKey(2));
    EXPECT_TRUE(myMap.HasKey(3));

    // Check if the map does not contain an invalid key
    EXPECT_FALSE(myMap.HasKey(4));

    EXPECT_EQ(myMap.Size(), 3);
}

// Test case to check if elements are removed correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapDeletionTest)
{
    Map<int, String> myMap;
    myMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    // Remove an element from the map
    myMap.Remove(2);

    // Check if the removed element is not in the map
    EXPECT_FALSE(myMap.HasKey(2));

    // Check if the other elements are still in the map
    EXPECT_TRUE(myMap.HasKey(1));
    EXPECT_TRUE(myMap.HasKey(3));

    EXPECT_EQ(myMap.Size(), 2);
}

// Test case to check if elements are updated correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapUpdateTest)
{
    Map<int, String> myMap;
    myMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    // Update an element in the map
    myMap[2] = "NewTwo";

    // Check if the updated element has the correct value
    EXPECT_EQ(myMap[2], "NewTwo");

    EXPECT_EQ(myMap.Size(), 3);
}

CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapKeysTest)
{
    Map<int, String> myMap;
    myMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    const auto* myKeys = myMap.Keys();

    EXPECT_EQ(myKeys->Size(), 3);
    EXPECT_FALSE(myKeys->Data() == nullptr);

    std::free(const_cast<Array<int>*>(myKeys));
}

CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapValuesTest)
{
    Map<int, String> myMap;
    myMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    const auto* myValues = myMap.Values();

    EXPECT_EQ(myValues->Size(), 3);
    EXPECT_FALSE(myValues->Data() == nullptr);

    std::free(const_cast<Array<String>*>(myValues));
}

CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapClearTest)
{
    Map<int, String> myMap;
    myMap = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };

    EXPECT_EQ(myMap.Size(), 3);

    myMap.Clear();

    EXPECT_EQ(myMap.Size(), 0);
}
