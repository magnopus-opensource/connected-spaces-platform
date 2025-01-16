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
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

#if RUN_ALL_UNIT_TESTS || RUN_REPLICATEDVALUE_TESTS
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, InvalidTest)
{
    ReplicatedValue MyValue;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, BoolConstructorTest)
{
    ReplicatedValue MyValue(true);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetBoolTest)
{
    ReplicatedValue MyValue;
    MyValue.SetBool(true);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, BoolAssignmentTest)
{
    ReplicatedValue MyValue;
    MyValue = true;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, IntConstructorTest)
{
    const int64_t MyInt = 42;
    ReplicatedValue MyValue(MyInt);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetIntTest)
{
    const int64_t MyInt = 42;
    ReplicatedValue MyValue;
    MyValue.SetInt(MyInt);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, IntAssignmentTest)
{
    const int64_t MyInt = 42;
    ReplicatedValue MyValue;
    MyValue = MyInt;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, FloatConstructorTest)
{
    ReplicatedValue MyValue(12345.6789f);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetFloatTest)
{
    ReplicatedValue MyValue;
    MyValue.SetFloat(12345.6789f);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, FloatAssignmentTest)
{
    ReplicatedValue MyValue;
    MyValue = 12345.6789f;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, Vector3ConstructorTest)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    ReplicatedValue MyValue(Vector3);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetVector3Test)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    ReplicatedValue MyValue;
    MyValue.SetVector3(Vector3);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, StringConstructorTest)
{
    const csp::common::String MyString("This is a string");

    ReplicatedValue MyValue(MyString);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetStringTest)
{
    const csp::common::String MyString("This is a string");

    ReplicatedValue MyValue;
    MyValue.SetString(MyString);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, StringAssignmentTest)
{
    const csp::common::String MyString("This is a string");

    ReplicatedValue MyValue;
    MyValue = MyString;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, StringLiteralConstructorTest)
{
    ReplicatedValue MyValue("This is a string");

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetStringLiteralTest)
{
    ReplicatedValue MyValue;
    MyValue.SetString("This is a string");

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, StringLiteralAssignmentTest)
{
    ReplicatedValue MyValue;
    MyValue = "This is a string";

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, MapConstructorTest)
{
    csp::common::Map<csp::common::String, ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    ReplicatedValue MyValue(MyMap);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, SetMapTest)
{
    ReplicatedValue MyValue;
    csp::common::Map<csp::common::String, ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    MyValue.SetStringMap(MyMap);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTestsv2, MapAssignmentTest)
{
    ReplicatedValue MyValue;
    csp::common::Map<csp::common::String, ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    MyValue = MyMap;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}
#endif