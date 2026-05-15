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

#include "CSP/Common/Optional.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::common;

class OptionalExtraTestClass;

class OtherTestClass
{
public:
    OtherTestClass(const int otherValue)
        : OtherField(otherValue) {};

    int OtherField;
};

class OptionalTestClass
{
public:
    OptionalTestClass(const int someFieldValue)
        : SomeField(someFieldValue) {};

    int SomeField;

    OptionalTestClass(const OptionalTestClass& other) { SomeField = other.SomeField; }

    OptionalTestClass(OptionalTestClass&& other) { SomeField = other.SomeField; }

    OptionalTestClass(OptionalTestClass* other) { SomeField = other->SomeField; }

    OptionalTestClass(const OtherTestClass& otherInstance) { SomeField = otherInstance.OtherField; }

    OptionalTestClass(OtherTestClass* otherInstancePtr) { SomeField = otherInstancePtr->OtherField; }
};

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalDefaultInitialisationTest)
{
    try
    {
        Optional<OptionalTestClass> optionalInstance;

        EXPECT_FALSE(optionalInstance.HasValue());
        EXPECT_EQ(&(*optionalInstance), nullptr);
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
        Optional<OptionalTestClass> optionalInstance(nullptr);

        EXPECT_FALSE(optionalInstance.HasValue());
        EXPECT_EQ(&(*optionalInstance), nullptr);
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
        auto* testClassInstancePtr = new OptionalTestClass(2);
        Optional<OptionalTestClass> optionalInstance(testClassInstancePtr);

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ(&(*optionalInstance), testClassInstancePtr);
        EXPECT_EQ((*optionalInstance).SomeField, testClassInstancePtr->SomeField);
        EXPECT_EQ(optionalInstance->SomeField, testClassInstancePtr->SomeField);
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
        bool destructorRan = false;

        std::function<void(OptionalTestClass*)> customDestructor = [&](OptionalTestClass* pointer)
        {
            pointer->~OptionalTestClass();
            delete (pointer);
            destructorRan = true;
        };

        // Scope block used to force the call on the destructor.
        {
            auto* testClassInstancePtr = new OptionalTestClass(3);
            Optional<OptionalTestClass> optionalInstance(testClassInstancePtr, customDestructor);

            EXPECT_TRUE(optionalInstance.HasValue());
            EXPECT_EQ(&(*optionalInstance), testClassInstancePtr);
            EXPECT_EQ((*optionalInstance).SomeField, testClassInstancePtr->SomeField);
            EXPECT_EQ(optionalInstance->SomeField, testClassInstancePtr->SomeField);
        }

        EXPECT_TRUE(destructorRan);
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
        auto* otherClassInstancePtr = new OtherTestClass(4);
        Optional<OptionalTestClass> optionalInstance = Optional<OptionalTestClass>((OtherTestClass*)otherClassInstancePtr);

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ((*optionalInstance).SomeField, otherClassInstancePtr->OtherField);
        EXPECT_EQ(optionalInstance->SomeField, otherClassInstancePtr->OtherField);

        delete (otherClassInstancePtr);
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
        Optional<OptionalTestClass> originalOptionalInstance = Optional<OptionalTestClass>();
        Optional<OptionalTestClass> copyOptionalInstance(originalOptionalInstance);

        EXPECT_FALSE(copyOptionalInstance.HasValue());
        EXPECT_EQ(&(*copyOptionalInstance), nullptr);
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
        OtherTestClass otherClassInstance(5);
        Optional<OptionalTestClass> optionalInstance = Optional<OptionalTestClass>(otherClassInstance);

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ((*optionalInstance).SomeField, otherClassInstance.OtherField);
        EXPECT_EQ(optionalInstance->SomeField, otherClassInstance.OtherField);
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
        OptionalTestClass testClassInstance(6);
        Optional<OptionalTestClass> optionalInstance(testClassInstance);

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ((*optionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(optionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(7);
        Optional<OptionalTestClass> optionalInstance(std::move(testClassInstance));

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ((*optionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(optionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(8);
        Optional<OptionalTestClass> originalOptionalInstance(testClassInstance);

        EXPECT_TRUE(originalOptionalInstance.HasValue());

        Optional<OptionalTestClass> copyOptionalInstance(originalOptionalInstance);

        EXPECT_TRUE(copyOptionalInstance.HasValue());
        EXPECT_EQ((*copyOptionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(copyOptionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(9);
        Optional<OptionalTestClass> originalOptionalInstance(testClassInstance);
        Optional<OptionalTestClass> moveOptionalInstance(std::move(originalOptionalInstance));

        EXPECT_TRUE(moveOptionalInstance.HasValue());
        EXPECT_EQ((*moveOptionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(moveOptionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(10);
        Optional<OptionalTestClass> optionalInstance(testClassInstance);

        EXPECT_TRUE(optionalInstance.HasValue());

        auto returnedTestClassInstance = *optionalInstance;

        EXPECT_EQ(returnedTestClassInstance.SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(11);
        Optional<OptionalTestClass> optionalInstance(testClassInstance);

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ(optionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(12);
        Optional<OptionalTestClass> optionalInstance = testClassInstance;

        EXPECT_TRUE(optionalInstance.HasValue());
        EXPECT_EQ((*optionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(optionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(13);
        Optional<OptionalTestClass> firstOptionalInstance(testClassInstance);
        Optional<OptionalTestClass> secondOptionalInstance = firstOptionalInstance;

        EXPECT_TRUE(secondOptionalInstance.HasValue());
        EXPECT_EQ((*secondOptionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(secondOptionalInstance->SomeField, testClassInstance.SomeField);
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
        OptionalTestClass testClassInstance(14);
        Optional<OptionalTestClass> firstOptionalInstance(testClassInstance);
        Optional<OptionalTestClass> secondOptionalInstance = std::move(firstOptionalInstance);

        EXPECT_TRUE(secondOptionalInstance.HasValue());
        EXPECT_EQ((*secondOptionalInstance).SomeField, testClassInstance.SomeField);
        EXPECT_EQ(secondOptionalInstance->SomeField, testClassInstance.SomeField);
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
        Optional<OptionalTestClass> firstOptionalInstance(nullptr);
        Optional<OptionalTestClass> secondOptionalInstance = std::move(firstOptionalInstance);

        EXPECT_FALSE(secondOptionalInstance.HasValue());
    }
    catch (...)
    {
        FAIL();
    }
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalAssignNull)
{
    csp::common::Optional<uint64_t> optionalInstance = 5;

    EXPECT_EQ(*optionalInstance, 5);

    optionalInstance = nullptr;

    EXPECT_FALSE(optionalInstance.HasValue());
}
