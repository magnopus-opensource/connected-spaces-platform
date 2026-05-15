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
    csp::common::List<TestStruct> testList;

    ASSERT_EQ(testList.Size(), 0);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 0);

    TestStruct toInsert;
    toInsert.str = "ToInsert";
    testList.Insert(0, toInsert);

    ASSERT_EQ(testList.Size(), 1);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 1);
    ASSERT_EQ(testList[0].str, "ToInsert");

    TestStruct toInsertBefore;
    toInsertBefore.str = "ToInsertBefore";
    testList.Insert(0, toInsertBefore);

    ASSERT_EQ(testList.Size(), 2);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 2);
    ASSERT_EQ(testList[0].str, "ToInsertBefore");
    ASSERT_EQ(testList[1].str, "ToInsert");
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListInsertAtEndTest)
{
    csp::common::List<TestStruct> testList;

    ASSERT_EQ(testList.Size(), 0);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 0);

    TestStruct toInsert;
    toInsert.str = "ToInsert";
    testList.Insert(0, toInsert);

    ASSERT_EQ(testList.Size(), 1);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 1);
    ASSERT_EQ(testList[0].str, "ToInsert");

    TestStruct toInsertAfter;
    toInsertAfter.str = "ToInsertAfter";
    testList.Insert(1, toInsertAfter);

    ASSERT_EQ(testList.Size(), 2);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 2);
    ASSERT_EQ(testList[0].str, "ToInsert");
    ASSERT_EQ(testList[1].str, "ToInsertAfter");
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListInsertMiddleTests)
{
    csp::common::List<TestStruct> testList;

    ASSERT_EQ(testList.Size(), 0);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 0);

    TestStruct one;
    one.str = "One";

    TestStruct four;
    four.str = "Four";

    testList.Append(one);
    testList.Append(four);

    ASSERT_EQ(testList.Size(), 2);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 2);
    ASSERT_EQ(testList[0].str, "One");
    ASSERT_EQ(testList[1].str, "Four");

    TestStruct two;
    two.str = "Two";

    testList.Insert(1, two);

    ASSERT_EQ(testList.Size(), 3);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 3);
    ASSERT_EQ(testList[0].str, "One");
    ASSERT_EQ(testList[1].str, "Two");
    ASSERT_EQ(testList[2].str, "Four");

    TestStruct three;
    three.str = "Three";

    testList.Insert(2, three);

    ASSERT_EQ(testList.Size(), 4);
    ASSERT_EQ(std::distance(testList.begin(), testList.end()), 4);
    ASSERT_EQ(testList[0].str, "One");
    ASSERT_EQ(testList[1].str, "Two");
    ASSERT_EQ(testList[2].str, "Three");
    ASSERT_EQ(testList[3].str, "Four");
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListReverseIteratorTest)
{
    csp::common::List<TestStruct> testList;

    TestStruct one;
    one.str = "One";
    TestStruct two;
    two.str = "Two";
    TestStruct three;
    three.str = "Three";

    testList.Append(one);
    testList.Append(two);
    testList.Append(three);

    ASSERT_EQ(std::distance(testList.rbegin(), testList.rend()), 3);

    auto rIt = testList.rbegin();
    ASSERT_EQ(rIt->str, "Three");
    ++rIt;
    ASSERT_EQ(rIt->str, "Two");
    ++rIt;
    ASSERT_EQ(rIt->str, "One");
    ++rIt;
    ASSERT_EQ(rIt, testList.rend());
}

CSP_INTERNAL_TEST(CSPEngine, CommonListTests, ListConstReverseIteratorTest)
{
    csp::common::List<TestStruct> testList;

    TestStruct one;
    one.str = "One";
    TestStruct two;
    two.str = "Two";
    TestStruct three;
    three.str = "Three";

    testList.Append(one);
    testList.Append(two);
    testList.Append(three);

    const auto& constList = testList;

    ASSERT_EQ(std::distance(constList.crbegin(), constList.crend()), 3);

    auto crIt = constList.crbegin();
    ASSERT_EQ(crIt->str, "Three");
    ++crIt;
    ASSERT_EQ(crIt->str, "Two");
    ++crIt;
    ASSERT_EQ(crIt->str, "One");
    ++crIt;
    ASSERT_EQ(crIt, constList.crend());
}
