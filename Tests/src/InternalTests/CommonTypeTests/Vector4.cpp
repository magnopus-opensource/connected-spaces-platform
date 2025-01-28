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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_COMMONTYPE_TESTS) || defined(RUN_COMMONTYPE_VECTOR4_TESTS)

#include "CSP/Common/Vector.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector43DefaultInitialisationTest)
{
    try
    {
        Vector4 Instance;

        EXPECT_EQ(Instance.X, 0);
        EXPECT_EQ(Instance.Y, 0);
        EXPECT_EQ(Instance.Z, 0);
        EXPECT_EQ(Instance.W, 0);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector4ValueInitialisationTest)
{
    try
    {
        Vector4 Instance(1, 2, 3, 4);

        EXPECT_EQ(Instance.X, 1);
        EXPECT_EQ(Instance.Y, 2);
        EXPECT_EQ(Instance.Z, 3);
        EXPECT_EQ(Instance.W, 4);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector4EqualityTest)
{
    try
    {
        Vector4 Instance1(1, 2, 3, 4);
        Vector4 Instance2(1, 2, 3, 4);

        EXPECT_TRUE(Instance1 == Instance2);
        EXPECT_FALSE(Instance1 != Instance2);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector4NonEqualityTest)
{
    try
    {
        Vector4 Instance1(1, 2, 3, 4);
        Vector4 Instance2(4, 3, 2, 1);

        EXPECT_FALSE(Instance1 == Instance2);
        EXPECT_TRUE(Instance1 != Instance2);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector4ZeroTest)
{
    try
    {
        Vector4 Instance = Vector4::Zero();

        EXPECT_EQ(Instance.X, 0);
        EXPECT_EQ(Instance.Y, 0);
        EXPECT_EQ(Instance.Z, 0);
        EXPECT_EQ(Instance.W, 0);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector4OneTest)
{
    try
    {
        Vector4 Instance = Vector4::One();

        EXPECT_EQ(Instance.X, 1);
        EXPECT_EQ(Instance.Y, 1);
        EXPECT_EQ(Instance.Z, 1);
        EXPECT_EQ(Instance.W, 1);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector4IdentityTest)
{
    try
    {
        Vector4 Instance = Vector4::Identity();

        EXPECT_EQ(Instance.X, 0);
        EXPECT_EQ(Instance.Y, 0);
        EXPECT_EQ(Instance.Z, 0);
        EXPECT_EQ(Instance.W, 1);
    }
    catch (...)
    {
        FAIL();
    }
}

#endif