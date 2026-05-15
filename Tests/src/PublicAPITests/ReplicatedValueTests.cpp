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
    csp::common::ReplicatedValue myValue;

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, BoolConstructorTest)
{
    csp::common::ReplicatedValue myValue(true);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean);
    EXPECT_TRUE(myValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetBoolTest)
{
    csp::common::ReplicatedValue myValue;
    myValue.SetBool(true);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean);
    EXPECT_TRUE(myValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, BoolAssignmentTest)
{
    csp::common::ReplicatedValue myValue;
    myValue = true;

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean);
    EXPECT_TRUE(myValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, IntConstructorTest)
{
    const int64_t myInt = 42;
    csp::common::ReplicatedValue myValue(myInt);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer);
    EXPECT_TRUE(myValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetIntTest)
{
    const int64_t myInt = 42;
    csp::common::ReplicatedValue myValue;
    myValue.SetInt(myInt);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer);
    EXPECT_TRUE(myValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, IntAssignmentTest)
{
    const int64_t myInt = 42;
    csp::common::ReplicatedValue myValue;
    myValue = myInt;

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer);
    EXPECT_TRUE(myValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, FloatConstructorTest)
{
    csp::common::ReplicatedValue myValue(12345.6789f);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float);
    EXPECT_TRUE(myValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetFloatTest)
{
    csp::common::ReplicatedValue myValue;
    myValue.SetFloat(12345.6789f);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float);
    EXPECT_TRUE(myValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, FloatAssignmentTest)
{
    csp::common::ReplicatedValue myValue;
    myValue = 12345.6789f;

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float);
    EXPECT_TRUE(myValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector3ConstructorTest)
{
    const csp::common::Vector3 vector3(5.0f, 1.0f, 3.0f);
    csp::common::ReplicatedValue myValue(vector3);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector3);
    EXPECT_TRUE(myValue.GetVector3() == vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetVector3Test)
{
    const csp::common::Vector3 vector3(5.0f, 1.0f, 3.0f);
    csp::common::ReplicatedValue myValue;
    myValue.SetVector3(vector3);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector3);
    EXPECT_TRUE(myValue.GetVector3() == vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringConstructorTest)
{
    const csp::common::String myString("This is a string");

    csp::common::ReplicatedValue myValue(myString);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(myValue.GetString() == myString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetStringTest)
{
    const csp::common::String myString("This is a string");

    csp::common::ReplicatedValue myValue;
    myValue.SetString(myString);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(myValue.GetString() == myString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringAssignmentTest)
{
    const csp::common::String myString("This is a string");

    csp::common::ReplicatedValue myValue;
    myValue = myString;

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(myValue.GetString() == myString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringLiteralConstructorTest)
{
    csp::common::ReplicatedValue myValue("This is a string");

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(myValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetStringLiteralTest)
{
    csp::common::ReplicatedValue myValue;
    myValue.SetString("This is a string");

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(myValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringLiteralAssignmentTest)
{
    csp::common::ReplicatedValue myValue;
    myValue = "This is a string";

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::String);
    EXPECT_TRUE(myValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MapConstructorTest)
{
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> myMap;
    myMap["Key1"] = "Test1";
    myMap["Key2"] = "Test2";
    myMap["Key3"] = "Test3";

    csp::common::ReplicatedValue myValue(myMap);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap);
    EXPECT_TRUE(myValue.GetStringMap() == myMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetMapTest)
{
    csp::common::ReplicatedValue myValue;
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> myMap;
    myMap["Key1"] = "Test1";
    myMap["Key2"] = "Test2";
    myMap["Key3"] = "Test3";

    myValue.SetStringMap(myMap);

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap);
    EXPECT_TRUE(myValue.GetStringMap() == myMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MapAssignmentTest)
{
    csp::common::ReplicatedValue myValue;
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> myMap;
    myMap["Key1"] = "Test1";
    myMap["Key2"] = "Test2";
    myMap["Key3"] = "Test3";

    myValue = myMap;

    EXPECT_TRUE(myValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap);
    EXPECT_TRUE(myValue.GetStringMap() == myMap);
}

// Tests move logic with a basic type
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveConstructorIntTest)
{
    csp::common::ReplicatedValue value { static_cast<int64_t>(10ll) };
    csp::common::ReplicatedValue newValue { std::move(value) };

    EXPECT_EQ(newValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(newValue.GetInt(), 10ll);

    EXPECT_EQ(value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

// Tests move logic with a more complex type
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveConstructorStringTest)
{
    csp::common::ReplicatedValue value { "Test" };
    csp::common::ReplicatedValue newValue { std::move(value) };

    EXPECT_EQ(newValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(newValue.GetString(), "Test");

    EXPECT_EQ(value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveAssignmentIntTest)
{
    csp::common::ReplicatedValue value { static_cast<int64_t>(10ll) };
    csp::common::ReplicatedValue newValue { static_cast<int64_t>(5ll) };

    newValue = std::move(value);

    EXPECT_EQ(newValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(newValue.GetInt(), 10ll);

    EXPECT_EQ(value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MoveAssignmentStringTest)
{
    csp::common::ReplicatedValue value { "Test" };
    csp::common::ReplicatedValue newValue { "Other" };

    newValue = std::move(value);

    EXPECT_EQ(newValue.GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(newValue.GetString(), "Test");

    EXPECT_EQ(value.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, EqualityOperatorTest)
{
    csp::common::ReplicatedValue value { "Test" };
    csp::common::ReplicatedValue value2 { "Test" };

    EXPECT_TRUE(value == value2);

    value2.SetString("Test2");

    EXPECT_FALSE(value == value2);

    value2.SetInt(5);

    EXPECT_FALSE(value == value2);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, InequalityOperatorTest)
{
    csp::common::ReplicatedValue value { "Test" };
    csp::common::ReplicatedValue value2 { "Test" };

    EXPECT_FALSE(value != value2);

    value2.SetString("Test2");

    EXPECT_TRUE(value != value2);

    value2.SetInt(5);

    EXPECT_TRUE(value != value2);
}