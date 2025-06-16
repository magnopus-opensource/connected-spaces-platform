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

// Invalid
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, InvalidTest)
{
    ReplicatedValue MyValue;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::InvalidType);
}

// Bool
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, BoolConstructorTest)
{
    ReplicatedValue MyValue(true);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetBoolTest)
{
    ReplicatedValue MyValue;
    MyValue.SetBool(true);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, BoolAssignmentTest)
{
    ReplicatedValue MyValue;
    MyValue = true;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Boolean);
    EXPECT_TRUE(MyValue.GetBool() == true);
}

// Float
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, FloatConstructorTest)
{
    ReplicatedValue MyValue(12345.6789f);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetFloatTest)
{
    ReplicatedValue MyValue;
    MyValue.SetFloat(12345.6789f);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, FloatAssignmentTest)
{
    ReplicatedValue MyValue;
    MyValue = 12345.6789f;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Float);
    EXPECT_TRUE(MyValue.GetFloat() == 12345.6789f);
}

// Int
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, IntConstructorTest)
{
    const int64_t MyInt = 42;
    ReplicatedValue MyValue(MyInt);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetIntTest)
{
    const int64_t MyInt = 42;
    ReplicatedValue MyValue;
    MyValue.SetInt(MyInt);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, IntAssignmentTest)
{
    const int64_t MyInt = 42;
    ReplicatedValue MyValue;
    MyValue = MyInt;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Integer);
    EXPECT_TRUE(MyValue.GetInt() == 42);
}

// String
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringConstructorTest)
{
    const csp::common::String MyString("This is a string");

    ReplicatedValue MyValue(MyString);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetStringTest)
{
    const csp::common::String MyString("This is a string");

    ReplicatedValue MyValue;
    MyValue.SetString(MyString);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringAssignmentTest)
{
    const csp::common::String MyString("This is a string");

    ReplicatedValue MyValue;
    MyValue = MyString;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == MyString);
}

// String literal
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringLiteralConstructorTest)
{
    ReplicatedValue MyValue("This is a string");

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetStringLiteralTest)
{
    ReplicatedValue MyValue;
    MyValue.SetString("This is a string");

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, StringLiteralAssignmentTest)
{
    ReplicatedValue MyValue;
    MyValue = "This is a string";

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::String);
    EXPECT_TRUE(MyValue.GetString() == "This is a string");
}

// Vector2
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector2ConstructorTest)
{
    const csp::common::Vector2 Vector2(5.0f, 1.0f);
    ReplicatedValue MyValue(Vector2);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector2);
    EXPECT_TRUE(MyValue.GetVector2() == Vector2);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetVector2Test)
{
    const csp::common::Vector2 Vector2(5.0f, 1.0f);
    ReplicatedValue MyValue;
    MyValue.SetVector2(Vector2);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector2);
    EXPECT_TRUE(MyValue.GetVector2() == Vector2);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector2AssignmentTest)
{
    const csp::common::Vector2 Vector2(5.0f, 1.0f);
    ReplicatedValue MyValue;
    MyValue = Vector2;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector2);
    EXPECT_TRUE(MyValue.GetVector2() == Vector2);
}

// Vector3
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector3ConstructorTest)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    ReplicatedValue MyValue(Vector3);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetVector3Test)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    ReplicatedValue MyValue;
    MyValue.SetVector3(Vector3);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector3AssignmentTest)
{
    const csp::common::Vector3 Vector3(5.0f, 1.0f, 3.0f);
    ReplicatedValue MyValue;
    MyValue = Vector3;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector3);
    EXPECT_TRUE(MyValue.GetVector3() == Vector3);
}

// Vector4
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector4ConstructorTest)
{
    const csp::common::Vector4 Vector4(5.0f, 1.0f, 3.0f, 2.0f);
    ReplicatedValue MyValue(Vector4);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector4);
    EXPECT_TRUE(MyValue.GetVector4() == Vector4);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetVector4Test)
{
    const csp::common::Vector4 Vector4(5.0f, 1.0f, 3.0f, 2.0f);
    ReplicatedValue MyValue;
    MyValue.SetVector4(Vector4);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector4);
    EXPECT_TRUE(MyValue.GetVector4() == Vector4);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, Vector4AssignmentTest)
{
    const csp::common::Vector4 Vector4(5.0f, 1.0f, 3.0f, 2.0f);
    ReplicatedValue MyValue;
    MyValue = Vector4;

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::Vector4);
    EXPECT_TRUE(MyValue.GetVector4() == Vector4);
}

// String Map
CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MapConstructorTest)
{
    csp::common::Map<csp::common::String, ReplicatedValue> MyMap;
    MyMap["Key1"] = "Test1";
    MyMap["Key2"] = "Test2";
    MyMap["Key3"] = "Test3";

    ReplicatedValue MyValue(MyMap);

    EXPECT_TRUE(MyValue.GetReplicatedValueType() == ReplicatedValueType::StringMap);
    EXPECT_TRUE(MyValue.GetStringMap() == MyMap);
}

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, SetMapTest)
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

CSP_PUBLIC_TEST(CSPEngine, ReplicatedValueTests, MapAssignmentTest)
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