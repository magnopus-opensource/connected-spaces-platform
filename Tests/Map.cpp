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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_COMMONTYPE_TESTS) || defined(RUN_COMMONTYPE_MAP_TESTS)

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
		Map<int, int> Instance;

		EXPECT_EQ(Instance.Size(), 0);
		EXPECT_EQ(Instance.Values(), nullptr);
	}
	catch (...)
	{
		FAIL();
	}
}

// Test case to check if elements are initialised with a list correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapInitialiseListTest)
{
	Map<int, csp::common::String> MyMap;
	MyMap = {{1, "One"}, {2, "Two"}, {3, "Three"}};

	// Check if the map contains the initial values
	EXPECT_TRUE(MyMap.HasKey(1));
	EXPECT_TRUE(MyMap.HasKey(2));
	EXPECT_TRUE(MyMap.HasKey(3));

	// Check if the map does not contain an invalid key
	EXPECT_FALSE(MyMap.HasKey(4));
}

// Test case to check if elements are initialised with a Map correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapInitialiseMapTest)
{
	Map<int, csp::common::String> OldMap;
	OldMap = {{1, "One"}, {2, "Two"}, {3, "Three"}};

	Map<int, String> MyMap(OldMap);

	// Check if the map contains the initial values
	EXPECT_TRUE(MyMap.HasKey(1));
	EXPECT_TRUE(MyMap.HasKey(2));
	EXPECT_TRUE(MyMap.HasKey(3));

	// Check if the map does not contain an invalid key
	EXPECT_FALSE(MyMap.HasKey(4));
}

// Test case to check if elements are assigned correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapAssignmentTest)
{
	Map<int, String> MyMap;
	MyMap[1] = "One";
	MyMap[2] = "Two";
	MyMap[3] = "Three";

	// Check if the map contains the initial values
	EXPECT_TRUE(MyMap.HasKey(1));
	EXPECT_TRUE(MyMap.HasKey(2));
	EXPECT_TRUE(MyMap.HasKey(3));

	// Check if the map does not contain an invalid key
	EXPECT_FALSE(MyMap.HasKey(4));
}

// Test case to check if elements are removed correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapDeletionTest)
{
	Map<int, String> MyMap;
	MyMap = {{1, "One"}, {2, "Two"}, {3, "Three"}};

	// Remove an element from the map
	MyMap.Remove(2);

	// Check if the removed element is not in the map
	EXPECT_FALSE(MyMap.HasKey(2));

	// Check if the other elements are still in the map
	EXPECT_TRUE(MyMap.HasKey(1));
	EXPECT_TRUE(MyMap.HasKey(3));
}

// Test case to check if elements are updated correctly
CSP_INTERNAL_TEST(CSPEngine, CommonMapTests, MapUpdateTest)
{
	Map<int, String> MyMap;
	MyMap = {{1, "One"}, {2, "Two"}, {3, "Three"}};

	// Update an element in the map
	MyMap[2] = "NewTwo";

	// Check if the updated element has the correct value
	EXPECT_EQ(MyMap[2], "NewTwo");
}

#endif