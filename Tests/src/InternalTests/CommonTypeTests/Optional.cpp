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

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_COMMONTYPE_TESTS) || defined(RUN_COMMONTYPE_OPTIONAL_TESTS)

#include "CSP/Common/Optional.h"
#include "Memory/Memory.h"

#include "CSP/Memory/DllAllocator.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

class OptionalExtraTestClass;

class OtherTestClass
{
public:
    OtherTestClass(const int OtherValue)
        : OtherField(OtherValue) {};

    int OtherField;
};

class OptionalTestClass
{
public:
    OptionalTestClass(const int SomeFieldValue)
        : SomeField(SomeFieldValue) {};

    int SomeField;

    OptionalTestClass(const OptionalTestClass& Other) { SomeField = Other.SomeField; }

    OptionalTestClass(OptionalTestClass&& Other) { SomeField = Other.SomeField; }

    OptionalTestClass(OptionalTestClass* Other) { SomeField = Other->SomeField; }

    OptionalTestClass(const OtherTestClass& OtherInstance) { SomeField = OtherInstance.OtherField; }

    OptionalTestClass(OtherTestClass* OtherInstancePtr) { SomeField = OtherInstancePtr->OtherField; }
};

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalDefaultInitialisationTest)
{
    try
    {
        Optional<OptionalTestClass> OptionalInstance;

        EXPECT_FALSE(OptionalInstance.HasValue());
        EXPECT_EQ(&(*OptionalInstance), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalNullptrInitialisationTest)
{
    try
    {
        Optional<OptionalTestClass> OptionalInstance(nullptr);

        EXPECT_FALSE(OptionalInstance.HasValue());
        EXPECT_EQ(&(*OptionalInstance), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalPointerInitialisationTest)
{
    try
    {
        auto* TestClassInstancePtr = CSP_NEW OptionalTestClass(2);
        Optional<OptionalTestClass> OptionalInstance(TestClassInstancePtr);

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ(&(*OptionalInstance), TestClassInstancePtr);
        EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstancePtr->SomeField);
        EXPECT_EQ(OptionalInstance->SomeField, TestClassInstancePtr->SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalPointerAndDestructorInitialisationTest)
{
    try
    {
        bool DestructorRan = false;

        std::function<void(OptionalTestClass*)> CustomDestructor = [&](OptionalTestClass* Pointer)
        {
            Pointer->~OptionalTestClass();
            CSP_DELETE(Pointer);
            DestructorRan = true;
        };

        // Scope block used to force the call on the destructor.
        {
            auto* TestClassInstancePtr = CSP_NEW OptionalTestClass(3);
            Optional<OptionalTestClass> OptionalInstance(TestClassInstancePtr, CustomDestructor);

            EXPECT_TRUE(OptionalInstance.HasValue());
            EXPECT_EQ(&(*OptionalInstance), TestClassInstancePtr);
            EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstancePtr->SomeField);
            EXPECT_EQ(OptionalInstance->SomeField, TestClassInstancePtr->SomeField);
        }

        EXPECT_TRUE(DestructorRan);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOtherClassPointerInitialisationTest)
{
    try
    {
        auto* OtherClassInstancePtr = CSP_NEW OtherTestClass(4);
        Optional<OptionalTestClass> OptionalInstance = Optional<OptionalTestClass>((OtherTestClass*)OtherClassInstancePtr);

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ((*OptionalInstance).SomeField, OtherClassInstancePtr->OtherField);
        EXPECT_EQ(OptionalInstance->SomeField, OtherClassInstancePtr->OtherField);

        CSP_DELETE(OtherClassInstancePtr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalNoValueCopyInitialisationTest)
{
    try
    {
        Optional<OptionalTestClass> OriginalOptionalInstance = Optional<OptionalTestClass>();
        Optional<OptionalTestClass> CopyOptionalInstance(OriginalOptionalInstance);

        EXPECT_FALSE(CopyOptionalInstance.HasValue());
        EXPECT_EQ(&(*CopyOptionalInstance), nullptr);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOtherClassInitialisationTest)
{
    try
    {
        OtherTestClass OtherClassInstance(5);
        Optional<OptionalTestClass> OptionalInstance = Optional<OptionalTestClass>(OtherClassInstance);

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ((*OptionalInstance).SomeField, OtherClassInstance.OtherField);
        EXPECT_EQ(OptionalInstance->SomeField, OtherClassInstance.OtherField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalNonNullInitialisationTest)
{
    try
    {
        OptionalTestClass TestClassInstance(6);
        Optional<OptionalTestClass> OptionalInstance(TestClassInstance);

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(OptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalInnerTypeMoveInitialisationTest)
{
    try
    {
        OptionalTestClass TestClassInstance(7);
        Optional<OptionalTestClass> OptionalInstance(std::move(TestClassInstance));

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(OptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalNonNullCopyInitialisationTest)
{
    try
    {
        OptionalTestClass TestClassInstance(8);
        Optional<OptionalTestClass> OriginalOptionalInstance(TestClassInstance);

        EXPECT_TRUE(OriginalOptionalInstance.HasValue());

        Optional<OptionalTestClass> CopyOptionalInstance(OriginalOptionalInstance);

        EXPECT_TRUE(CopyOptionalInstance.HasValue());
        EXPECT_EQ((*CopyOptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(CopyOptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOptionalTypeMoveInitialisationTest)
{
    try
    {
        OptionalTestClass TestClassInstance(9);
        Optional<OptionalTestClass> OriginalOptionalInstance(TestClassInstance);
        Optional<OptionalTestClass> MoveOptionalInstance(std::move(OriginalOptionalInstance));

        EXPECT_TRUE(MoveOptionalInstance.HasValue());
        EXPECT_EQ((*MoveOptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(MoveOptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOperatorStarTest)
{
    // This is tested in pretty much all other tests but an explicit test is added for completion.
    try
    {
        OptionalTestClass TestClassInstance(10);
        Optional<OptionalTestClass> OptionalInstance(TestClassInstance);

        EXPECT_TRUE(OptionalInstance.HasValue());

        auto ReturnedTestClassInstance = *OptionalInstance;

        EXPECT_EQ(ReturnedTestClassInstance.SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOperatorArrowTest)
{
    // This is tested in pretty much all other tests but an explicit test is added for completion.
    try
    {
        OptionalTestClass TestClassInstance(11);
        Optional<OptionalTestClass> OptionalInstance(TestClassInstance);

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ(OptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalInnerTypeOperatorEqualsTest)
{
    try
    {
        OptionalTestClass TestClassInstance(12);
        Optional<OptionalTestClass> OptionalInstance = TestClassInstance;

        EXPECT_TRUE(OptionalInstance.HasValue());
        EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(OptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOptionalTypeOperatorEqualsTest)
{
    try
    {
        OptionalTestClass TestClassInstance(13);
        Optional<OptionalTestClass> FirstOptionalInstance(TestClassInstance);
        Optional<OptionalTestClass> SecondOptionalInstance = FirstOptionalInstance;

        EXPECT_TRUE(SecondOptionalInstance.HasValue());
        EXPECT_EQ((*SecondOptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(SecondOptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalOptionalTypeMoveOperatorEqualsTest)
{
    try
    {
        OptionalTestClass TestClassInstance(14);
        Optional<OptionalTestClass> FirstOptionalInstance(TestClassInstance);
        Optional<OptionalTestClass> SecondOptionalInstance = std::move(FirstOptionalInstance);

        EXPECT_TRUE(SecondOptionalInstance.HasValue());
        EXPECT_EQ((*SecondOptionalInstance).SomeField, TestClassInstance.SomeField);
        EXPECT_EQ(SecondOptionalInstance->SomeField, TestClassInstance.SomeField);
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalNoValueOptionalTypeMoveOperatorEqualsTest)
{
    try
    {
        Optional<OptionalTestClass> FirstOptionalInstance(nullptr);
        Optional<OptionalTestClass> SecondOptionalInstance = std::move(FirstOptionalInstance);

        EXPECT_FALSE(SecondOptionalInstance.HasValue());
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalAssignNull)
{
    csp::common::Optional<uint64_t> OptionalInstance = 5;

    EXPECT_EQ(*OptionalInstance, 5);

    OptionalInstance = nullptr;

    EXPECT_FALSE(OptionalInstance.HasValue());
}
#endif