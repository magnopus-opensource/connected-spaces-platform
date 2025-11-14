/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/Common/List.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

namespace
{
/* Use a struct because trivial types might not express some behaviors */
struct TestStruct
{
    int x = 0;
    std::string str = "Default";
};
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListInsertAtStartTest)
{
    csp::common::List<TestStruct> TestList;

    ASSERT_EQ(TestList.Size(), 0);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 0);

    TestStruct ToInsert;
    ToInsert.str = "ToInsert";
    TestList.Insert(0, ToInsert);

    ASSERT_EQ(TestList.Size(), 1);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 1);
    ASSERT_EQ(TestList[0].str, "ToInsert");

    TestStruct ToInsertBefore;
    ToInsertBefore.str = "ToInsertBefore";
    TestList.Insert(0, ToInsertBefore);

    ASSERT_EQ(TestList.Size(), 2);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 2);
    ASSERT_EQ(TestList[0].str, "ToInsertBefore");
    ASSERT_EQ(TestList[1].str, "ToInsert");
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListInsertAtEndTest)
{
    csp::common::List<TestStruct> TestList;

    ASSERT_EQ(TestList.Size(), 0);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 0);

    TestStruct ToInsert;
    ToInsert.str = "ToInsert";
    TestList.Insert(0, ToInsert);

    ASSERT_EQ(TestList.Size(), 1);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 1);
    ASSERT_EQ(TestList[0].str, "ToInsert");

    TestStruct ToInsertAfter;
    ToInsertAfter.str = "ToInsertAfter";
    TestList.Insert(1, ToInsertAfter);

    ASSERT_EQ(TestList.Size(), 2);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 2);
    ASSERT_EQ(TestList[0].str, "ToInsert");
    ASSERT_EQ(TestList[1].str, "ToInsertAfter");
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListInsertMiddleTests)
{
    csp::common::List<TestStruct> TestList;

    ASSERT_EQ(TestList.Size(), 0);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 0);

    TestStruct One;
    One.str = "One";

    TestStruct Four;
    Four.str = "Four";

    TestList.Append(One);
    TestList.Append(Four);

    ASSERT_EQ(TestList.Size(), 2);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 2);
    ASSERT_EQ(TestList[0].str, "One");
    ASSERT_EQ(TestList[1].str, "Four");

    TestStruct Two;
    Two.str = "Two";

    TestList.Insert(1, Two);

    ASSERT_EQ(TestList.Size(), 3);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 3);
    ASSERT_EQ(TestList[0].str, "One");
    ASSERT_EQ(TestList[1].str, "Two");
    ASSERT_EQ(TestList[2].str, "Four");

    TestStruct Three;
    Three.str = "Three";

    TestList.Insert(2, Three);

    ASSERT_EQ(TestList.Size(), 4);
    ASSERT_EQ(std::distance(TestList.begin(), TestList.end()), 4);
    ASSERT_EQ(TestList[0].str, "One");
    ASSERT_EQ(TestList[1].str, "Two");
    ASSERT_EQ(TestList[2].str, "Three");
    ASSERT_EQ(TestList[3].str, "Four");
}
