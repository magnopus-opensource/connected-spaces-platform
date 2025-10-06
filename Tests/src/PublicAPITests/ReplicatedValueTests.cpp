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
#include "CSP/Common/ReplicatedValue.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, InvalidTest)
{
    csp::common::ReplicatedValue MyValue;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, BoolConstructorTest)
{
    csp::common::ReplicatedValue MyValue(true);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetBoolTest)
{
    csp::common::ReplicatedValue MyValue;
    MyValue.SetBool(true);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, BoolAssignmentTest)
{
    csp::common::ReplicatedValue MyValue;
    MyValue = true;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, IntConstructorTest)
{
    const int64_t MyInt = 42;
    csp::common::ReplicatedValue MyValue(MyInt);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetIntTest)
{
    const int64_t MyInt = 42;
    csp::common::ReplicatedValue MyValue;
    MyValue.SetInt(MyInt);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, IntAssignmentTest)
{
    const int64_t MyInt = 42;
    csp::common::ReplicatedValue MyValue;
    MyValue = MyInt;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, FloatConstructorTest)
{
    csp::common::ReplicatedValue MyValue(12345.6789f);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetFloatTest)
{
    csp::common::ReplicatedValue MyValue;
    MyValue.SetFloat(12345.6789f);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, FloatAssignmentTest)
{
    csp::common::ReplicatedValue MyValue;
    MyValue = 12345.6789f;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector3ConstructorTest)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    csp::common::ReplicatedValue MyValue(Vector3);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetVector3Test)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    csp::common::ReplicatedValue MyValue;
    MyValue.SetVector3(Vector3);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringConstructorTest)
{
    const csp::common::String MyString("This is a string");

    csp::common::ReplicatedValue MyValue(MyString);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetStringTest)
{
    const csp::common::String MyString("This is a string");

    csp::common::ReplicatedValue MyValue;
    MyValue.SetString(MyString);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringAssignmentTest)
{
    const csp::common::String MyString("This is a string");

    csp::common::ReplicatedValue MyValue;
    MyValue = MyString;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringLiteralConstructorTest)
{
    csp::common::ReplicatedValue MyValue("This is a string");

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetStringLiteralTest)
{
    csp::common::ReplicatedValue MyValue;
    MyValue.SetString("This is a string");

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringLiteralAssignmentTest)
{
    csp::common::ReplicatedValue MyValue;
    MyValue = "This is a string";

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MapConstructorTest)
{
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    csp::common::ReplicatedValue MyValue(MyMap);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetMapTest)
{
    csp::common::ReplicatedValue MyValue;
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    MyValue.SetStringMap(MyMap);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MapAssignmentTest)
{
    csp::common::ReplicatedValue MyValue;
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    MyValue = MyMap;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}

// Tests move logic with a basic type
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveConstructorIntTest)
{
    csp::common::ReplicatedValue Value { 10ll };
    csp::common::ReplicatedValue NewValue { std::move(Value) };

    EXPECT_EQ(NewValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(NewValue.GetInt(), 10ll);

    EXPECT_EQ(Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

// Tests move logic with a more complex type
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveConstructorStringTest)
{
    csp::common::ReplicatedValue Value { "Test" };
    csp::common::ReplicatedValue NewValue { std::move(Value) };

    EXPECT_EQ(NewValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(NewValue.GetString(), "Test");

    EXPECT_EQ(Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveAssignmentIntTest)
{
    csp::common::ReplicatedValue Value { 10ll };
    csp::common::ReplicatedValue NewValue { 5ll };

    NewValue = std::move(Value);

    EXPECT_EQ(NewValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(NewValue.GetInt(), 10ll);

    EXPECT_EQ(Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveAssignmentStringTest)
{
    csp::common::ReplicatedValue Value { "Test" };
    csp::common::ReplicatedValue NewValue { "Other" };

    NewValue = std::move(Value);

    EXPECT_EQ(NewValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(NewValue.GetString(), "Test");

    EXPECT_EQ(Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, EqualityOperatorTest)
{
    csp::common::ReplicatedValue Value { "Test" };
    csp::common::ReplicatedValue Value2 { "Test" };

    EXPECT_TRUE(Value == Value2);

    Value2.SetString("Test2");

    EXPECT_FALSE(Value == Value2);

    Value2.SetInt(5);

    EXPECT_FALSE(Value == Value2);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, InequalityOperatorTest)
{
    csp::common::ReplicatedValue Value { "Test" };
    csp::common::ReplicatedValue Value2 { "Test" };

    EXPECT_FALSE(Value != Value2);

    Value2.SetString("Test2");

    EXPECT_TRUE(Value != Value2);

    Value2.SetInt(5);

    EXPECT_TRUE(Value != Value2);
}