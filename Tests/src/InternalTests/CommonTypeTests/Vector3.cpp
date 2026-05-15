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

#include "CSP/Common/Vector.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector3DefaultInitialisationTest)
{
    try
    {
        Vector3 instance;

        EXPECT_EQ(instance.X, 0);
        EXPECT_EQ(instance.Y, 0);
        EXPECT_EQ(instance.Z, 0);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector3ValueInitialisationTest)
{
    try
    {
        Vector3 instance(1, 2, 3);

        EXPECT_EQ(instance.X, 1);
        EXPECT_EQ(instance.Y, 2);
        EXPECT_EQ(instance.Z, 3);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector3EqualityTest)
{
    try
    {
        Vector3 instance1(1, 2, 3);
        Vector3 instance2(1, 2, 3);

        EXPECT_TRUE(instance1 == instance2);
        EXPECT_FALSE(instance1 != instance2);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector3NonEqualityTest)
{
    try
    {
        Vector3 instance1(1, 2, 3);
        Vector3 instance2(3, 2, 1);

        EXPECT_FALSE(instance1 == instance2);
        EXPECT_TRUE(instance1 != instance2);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector3ZeroTest)
{
    try
    {
        Vector3 instance = Vector3::Zero();

        EXPECT_EQ(instance.X, 0);
        EXPECT_EQ(instance.Y, 0);
        EXPECT_EQ(instance.Z, 0);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonVectorTests, Vector3OneTest)
{
    try
    {
        Vector3 instance = Vector3::One();

        EXPECT_EQ(instance.X, 1);
        EXPECT_EQ(instance.Y, 1);
        EXPECT_EQ(instance.Z, 1);
    }
    catch (...)
    {
        FAIL();
    }
}
