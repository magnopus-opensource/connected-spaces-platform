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

	#include "TestHelpers.h"

	#include <gtest/gtest.h>


using namespace csp::common;

class OptionalTestClass
{
public:
	OptionalTestClass(const int SomeFieldValue) : SomeField(SomeFieldValue) {};

	int SomeField;

	int CopyCount = 0;
	int MoveCount = 0;

	bool Moved = false;

	OptionalTestClass(const OptionalTestClass& Other)
	{
		SomeField = Other.SomeField;
		CopyCount = Other.CopyCount + 1;
		MoveCount = Other.MoveCount;
	}

	OptionalTestClass(OptionalTestClass&& Other)
	{
		SomeField	= Other.SomeField;
		CopyCount	= Other.CopyCount;
		MoveCount	= Other.MoveCount + 1;
		Other.Moved = true;
	}
};

class OptionalExtraTestClass : public OptionalTestClass
{
public:
	OptionalExtraTestClass(const int SomeFieldValue, const int ExtraFieldValue) : OptionalTestClass(SomeFieldValue), ExtraField(ExtraFieldValue) {};

	int ExtraField;
};


CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalDefaultInitialisationTest)
{
	try
	{
		Optional<OptionalTestClass> OptionalInstance;

		EXPECT_FALSE(OptionalInstance.HasValue());
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
	}
	catch (...)
	{
		FAIL();
	}
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalWithPointerInitialisationTest)
{

	try
	{
		auto TestClassInstancePtr = std::make_unique<OptionalTestClass>(1);
		Optional<OptionalTestClass> OptionalInstance(TestClassInstancePtr.get());

		EXPECT_TRUE(OptionalInstance.HasValue());
		EXPECT_EQ(&(*OptionalInstance), TestClassInstancePtr.get());
		EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstancePtr->SomeField);
		EXPECT_EQ(OptionalInstance->SomeField, TestClassInstancePtr->SomeField);
	}
	catch (...)
	{
		FAIL();
	}
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalWithPointerAndDestructorInitialisationTest)
{
	try
	{
		bool DestructorRan = false;

		std::function<void(OptionalTestClass*)> CustomDestructor = [&](OptionalTestClass* Pointer)
		{
			DestructorRan = true;
		};

		{
			auto TestClassInstancePtr = std::make_unique<OptionalTestClass>(2);
			Optional<OptionalTestClass> OptionalInstance(TestClassInstancePtr.get(), CustomDestructor);

			EXPECT_TRUE(OptionalInstance.HasValue());
			EXPECT_EQ(&(*OptionalInstance), TestClassInstancePtr.get());
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

/*
 * This constructor is causing me problems. I've tried a few things but every time I seem to get an error
 * with it trying to use `Optional(const U& InValue)` rather than `Optional(const U* InValue)`. Not sure
 * if there is a way that I am missing or if maybe we should remove this constructor? I cannot see it used
 * anywhere.
 */
// CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalWithExtendedClassPointerInitialisationTest)
//{
//	try
//	{
//		std::unique_ptr<OptionalExtraTestClass> TestClassInstance = std::make_unique<OptionalExtraTestClass>(3, 4);
//		Optional<OptionalTestClass> OptionalInstance(TestClassInstance.get());
//
//		EXPECT_TRUE(OptionalInstance.HasValue());
//		EXPECT_EQ((*OptionalInstance).SomeField, TestClassInstance->SomeField);
//		EXPECT_EQ(OptionalInstance->SomeField, TestClassInstance->SomeField);
//	}
//	catch (...)
//	{
//		FAIL();
//	}
// }

/*
 * Should we expect this to work?
 * It seems like because we have not got an instace in the optional that it does not use the Optional(const Optional<T>& Other)
 * constructor despite the declaration that OptionalTestClass is the T that we are using.
 */
// CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, CopyOptionalWithNoValueInitialisationTest)
//{
//	try
//	{
//		Optional<OptionalTestClass> OriginalOptionalInstance();
//		Optional<OptionalTestClass> CopyOptionalInstance(OriginalOptionalInstance);
//
//		EXPECT_FALSE(CopyOptionalInstance.HasValue());
//		EXPECT_EQ(*CopyOptionalInstance, nullptr);
//	}
//	catch (...)
//	{
//		FAIL();
//	}
//}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalWithExtendedClassReferenceInitialisationTest)
{
	try
	{
		OptionalExtraTestClass TestClassInstance(5, 6);
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

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalWithValueInitialisationTest)
{
	try
	{
		OptionalTestClass TestClassInstance(7);
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

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, OptionalWithMoveValueInitialisationTest)
{
	try
	{
		OptionalTestClass TestClassInstance(8);
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


CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, CopyOptionalWithValueInitialisationTest)
{
	try
	{
		OptionalTestClass TestClassInstance(9);
		Optional<OptionalTestClass> OriginalOptionalInstance(TestClassInstance);

		EXPECT_TRUE(OriginalOptionalInstance.HasValue());
		EXPECT_EQ(OriginalOptionalInstance->CopyCount, 1);

		Optional<OptionalTestClass> CopyOptionalInstance(OriginalOptionalInstance);

		EXPECT_TRUE(CopyOptionalInstance.HasValue());

		EXPECT_EQ(OriginalOptionalInstance->CopyCount, 1);
		EXPECT_EQ(CopyOptionalInstance->CopyCount, 2);

		EXPECT_EQ((*CopyOptionalInstance).SomeField, TestClassInstance.SomeField);
		EXPECT_EQ(CopyOptionalInstance->SomeField, TestClassInstance.SomeField);
	}
	catch (...)
	{
		FAIL();
	}
}

CSP_INTERNAL_TEST(CSPEngine, CommonOptionalTests, MoveOptionalWithValueInitialisationTest)
{
	try
	{
		OptionalTestClass TestClassInstance(10);
		Optional<OptionalTestClass> OriginalOptionalInstance(TestClassInstance);
		Optional<OptionalTestClass> MoveOptionalInstance(std::move(OriginalOptionalInstance));

		EXPECT_TRUE(MoveOptionalInstance.HasValue());
		EXPECT_EQ((*MoveOptionalInstance).SomeField, TestClassInstance.SomeField);
		EXPECT_EQ(MoveOptionalInstance->SomeField, TestClassInstance.SomeField);

		EXPECT_TRUE(OriginalOptionalInstance->Moved);
		EXPECT_FALSE(MoveOptionalInstance->Moved);
		EXPECT_EQ(MoveOptionalInstance->MoveCount, 1);
	}
	catch (...)
	{
		FAIL();
	}
}
#endif