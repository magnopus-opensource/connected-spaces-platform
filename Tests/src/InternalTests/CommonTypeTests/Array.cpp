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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_COMMONTYPE_TESTS) || defined(RUN_COMMONTYPE_ARRAY_TESTS)

#include "CSP/Common/Array.h"
#include "CSP/Common/Optional.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayDefaultInitialisationTest)
{
    try
    {
        Array<int> Instance;

        EXPECT_TRUE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), 0);
        EXPECT_EQ(Instance.Data(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

class ArrayTestClass
{
public:
    ArrayTestClass()
        : SomeField(42) {};

    int SomeField;
};

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArraySizeInitialisationTest)
{
    constexpr int ARRAY_SIZE = 5;

    try
    {
        Array<ArrayTestClass> Instance(ARRAY_SIZE);

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), ARRAY_SIZE);
        EXPECT_NE(Instance.Data(), nullptr);

        // All elements should be default-initialised
        for (int i = 0; i < ARRAY_SIZE; ++i)
        {
            EXPECT_EQ(Instance[i].SomeField, 42);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArraySizeTooLargeInitialisationTest)
{
    constexpr size_t ARRAY_SIZE = SIZE_MAX;

    try
    {
        // This should throw an overflow exception because `sizeof(T) * ARRAY_SIZE` is greater than `SIZE_MAX`
        Array<ArrayTestClass> Instance(ARRAY_SIZE);
    }
    catch (...)
    {
        return;
    }

    FAIL();
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayBufferInitialisationTest)
{
    constexpr int ARRAY_SIZE = 5;

    try
    {
        int Values[] = { 1, 2, 3, 4, 5 };
        Array<int> Instance(Values, ARRAY_SIZE);

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), ARRAY_SIZE);
        EXPECT_NE(Instance.Data(), nullptr);

        // All elements should match those in the provided buffer, but should not have the same address
        for (int i = 0; i < ARRAY_SIZE; ++i)
        {
            EXPECT_EQ(Instance[i], Values[i]);
            EXPECT_NE(&Instance[i], &Values[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayBufferNullptrInitialisationTest)
{
    try
    {
        Array<int> Instance(nullptr, 5);

        EXPECT_TRUE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), 0);
        EXPECT_EQ(Instance.Data(), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayCopyInitialisationTest)
{
    constexpr int ARRAY_SIZE = 2;

    try
    {
        Array<int> OtherInstance(ARRAY_SIZE);
        OtherInstance[0] = 1337;
        OtherInstance[1] = 1338;

        Array<int> Instance(OtherInstance);

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), ARRAY_SIZE);
        EXPECT_NE(Instance.Data(), nullptr);

        // All elements should match those in the other array, but should not have the same address
        for (int i = 0; i < ARRAY_SIZE; ++i)
        {
            EXPECT_EQ(Instance[i], OtherInstance[i]);
            EXPECT_NE(&Instance[i], &OtherInstance[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayInitializerListInitialisationTest)
{
    constexpr int ARRAY_SIZE = 5;

    try
    {
        int Values[] = { 1, 2, 3, 4, 5 };
        Array<int> Instance = { 1, 2, 3, 4, 5 };

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), ARRAY_SIZE);
        EXPECT_NE(Instance.Data(), nullptr);

        // All elements should match those in the previously-declared array
        for (int i = 0; i < ARRAY_SIZE; ++i)
        {
            EXPECT_EQ(Instance[i], Values[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayCopyAssignmentTest)
{
    constexpr int ARRAY_SIZE = 2;

    try
    {
        Array<int> OtherInstance(ARRAY_SIZE);
        OtherInstance[0] = 1337;
        OtherInstance[1] = 1338;

        Array<int> Instance;
        Instance = OtherInstance;

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), ARRAY_SIZE);
        EXPECT_NE(Instance.Data(), nullptr);

        // All elements should match those in the other array, but should not have the same address
        for (int i = 0; i < ARRAY_SIZE; ++i)
        {
            EXPECT_EQ(Instance[i], OtherInstance[i]);
            EXPECT_NE(&Instance[i], &OtherInstance[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayOfOptionalTest)
{
    constexpr int ARRAY_SIZE = 2;

    try
    {
        Array<Optional<int>> Instance(ARRAY_SIZE);
        Instance[0] = nullptr;
        Instance[1] = 1337;

        EXPECT_FALSE(Instance.IsEmpty());
        EXPECT_EQ(Instance.Size(), ARRAY_SIZE);
        EXPECT_NE(Instance.Data(), nullptr);

        EXPECT_FALSE(Instance[0].HasValue());
        EXPECT_TRUE(Instance[1].HasValue());
        EXPECT_EQ(*Instance[1], 1337);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayOutOfBoundsTest)
{
    constexpr int ARRAY_SIZE = 5;

    try
    {
        Array<Optional<int>> Instance(ARRAY_SIZE);
        [[maybe_unused]] auto& Element = Instance[ARRAY_SIZE];
    }
    catch (const std::out_of_range&)
    {
        return;
    }

    FAIL();
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayToListTest)
{
    try
    {
        Array<String> Instance = { "asd", "fgh", "jkl", "123" };

        auto ConvertedList = Instance.ToList();

        EXPECT_EQ(ConvertedList.Size(), Instance.Size());

        // All elements should match those in the array, but should not have the same address
        for (int i = 0; i < Instance.Size(); ++i)
        {
            EXPECT_EQ(ConvertedList[i], Instance[i]);
            EXPECT_NE(&ConvertedList[i], &Instance[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

#endif