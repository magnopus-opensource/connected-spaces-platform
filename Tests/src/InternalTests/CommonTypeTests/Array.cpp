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

#include "CSP/Common/Array.h"
#include "CSP/Common/Optional.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayDefaultInitialisationTest)
{
    try
    {
        Array<int> instance;

        EXPECT_TRUE(instance.IsEmpty());
        EXPECT_EQ(instance.Size(), 0);
        EXPECT_EQ(instance.Data(), nullptr);
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
    constexpr int arraySize = 5;

    try
    {
        Array<ArrayTestClass> instance(arraySize);

        EXPECT_FALSE(instance.IsEmpty());
        EXPECT_EQ(instance.Size(), arraySize);
        EXPECT_NE(instance.Data(), nullptr);

        // All elements should be default-initialised
        for (int i = 0; i < arraySize; ++i)
        {
            EXPECT_EQ(instance[i].SomeField, 42);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArraySizeTooLargeInitialisationTest)
{
    constexpr size_t arraySize = SIZE_MAX;

    try
    {
        // This should throw an overflow exception because `sizeof(T) * ARRAY_SIZE` is greater than `SIZE_MAX`
        Array<ArrayTestClass> instance(arraySize);
    }
    catch (...)
    {
        return;
    }

    FAIL();
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayCopyInitialisationTest)
{
    constexpr int arraySize = 2;

    try
    {
        Array<int> otherInstance(arraySize);
        otherInstance[0] = 1337;
        otherInstance[1] = 1338;

        Array<int> instance(otherInstance);

        EXPECT_FALSE(instance.IsEmpty());
        EXPECT_EQ(instance.Size(), arraySize);
        EXPECT_NE(instance.Data(), nullptr);

        // All elements should match those in the other array, but should not have the same address
        for (int i = 0; i < arraySize; ++i)
        {
            EXPECT_EQ(instance[i], otherInstance[i]);
            EXPECT_NE(&instance[i], &otherInstance[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayInitializerListInitialisationTest)
{
    constexpr int arraySize = 5;

    try
    {
        int values[] = { 1, 2, 3, 4, 5 };
        Array<int> instance = { 1, 2, 3, 4, 5 };

        EXPECT_FALSE(instance.IsEmpty());
        EXPECT_EQ(instance.Size(), arraySize);
        EXPECT_NE(instance.Data(), nullptr);

        // All elements should match those in the previously-declared array
        for (int i = 0; i < arraySize; ++i)
        {
            EXPECT_EQ(instance[i], values[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayCopyAssignmentTest)
{
    constexpr int arraySize = 2;

    try
    {
        Array<int> otherInstance(arraySize);
        otherInstance[0] = 1337;
        otherInstance[1] = 1338;

        Array<int> instance;
        instance = otherInstance;

        EXPECT_FALSE(instance.IsEmpty());
        EXPECT_EQ(instance.Size(), arraySize);
        EXPECT_NE(instance.Data(), nullptr);

        // All elements should match those in the other array, but should not have the same address
        for (int i = 0; i < arraySize; ++i)
        {
            EXPECT_EQ(instance[i], otherInstance[i]);
            EXPECT_NE(&instance[i], &otherInstance[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayOfOptionalTest)
{
    constexpr int arraySize = 2;

    try
    {
        Array<Optional<int>> instance(arraySize);
        instance[0] = nullptr;
        instance[1] = 1337;

        EXPECT_FALSE(instance.IsEmpty());
        EXPECT_EQ(instance.Size(), arraySize);
        EXPECT_NE(instance.Data(), nullptr);

        EXPECT_FALSE(instance[0].HasValue());
        EXPECT_TRUE(instance[1].HasValue());
        EXPECT_EQ(*instance[1], 1337);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, ArrayOutOfBoundsTest)
{
    constexpr int arraySize = 5;

    try
    {
        Array<Optional<int>> instance(arraySize);
        [[maybe_unused]] auto& element = instance[arraySize];
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
        Array<String> instance = { "asd", "fgh", "jkl", "123" };

        auto convertedList = instance.ToList();

        EXPECT_EQ(convertedList.Size(), instance.Size());

        // All elements should match those in the array, but should not have the same address
        for (size_t i = 0; i < instance.Size(); ++i)
        {
            EXPECT_EQ(convertedList[i], instance[i]);
            EXPECT_NE(&convertedList[i], &instance[i]);
        }
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonArrayTests, EqualityTest)
{
    Array<String> empty1 = {};
    Array<String> empty2 = {};

    ASSERT_EQ(empty1, empty1);
    ASSERT_EQ(empty1, empty2);

    Array<String> singleInstance1 = { "Hello" };
    Array<String> singleInstance2 = { "Hello" };

    ASSERT_EQ(singleInstance1, singleInstance1);
    ASSERT_EQ(singleInstance1, singleInstance2);
    ASSERT_NE(empty1, singleInstance1);

    Array<String> singleInstanceDifferent = { "Goodbye" };
    ASSERT_NE(singleInstance1, singleInstanceDifferent);

    Array<String> multiInstance1 = { "Is", "it", "me", "you're", "looking", "for" };
    Array<String> multiInstance2 = { "Is", "it", "me", "you're", "looking", "for" };

    ASSERT_EQ(multiInstance1, multiInstance1);
    ASSERT_EQ(multiInstance1, multiInstance2);
    ASSERT_NE(singleInstance1, multiInstance1);
    ASSERT_NE(empty1, multiInstance1);

    Array<String> multiInstanceDifferent = { "I", "can", "see", "it", "in", "your", "eyes" };
    ASSERT_NE(multiInstance1, multiInstanceDifferent);
}
