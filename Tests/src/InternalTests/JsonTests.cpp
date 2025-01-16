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
#ifndef SKIP_INTERNAL_TESTS

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "Json/JsonSerializer.h"

using namespace csp::json;

struct TestObjectProps
{
    int32_t Int32Member = 0;
    uint32_t Uint32Member = 0;
    int64_t Int64Member = 0;
    uint64_t Uint64Member = 0;
    float FloatMember = 0.f;
    float DoubleMember = 0.0;
    csp::common::String StringMember = "";
    const char* CharPtrMember = "";
};

void ToJson(JsonSerializer& Serializer, const TestObjectProps& Obj)
{
    Serializer.SerializeMember("int32Member", Obj.Int32Member);
    Serializer.SerializeMember("uint32Member", Obj.Uint32Member);
    Serializer.SerializeMember("int64Member", Obj.Int64Member);
    Serializer.SerializeMember("uint64Member", Obj.Uint64Member);
    Serializer.SerializeMember("floatMember", Obj.FloatMember);
    Serializer.SerializeMember("doubleMember", Obj.DoubleMember);
    Serializer.SerializeMember("stringMember", Obj.StringMember);
    Serializer.SerializeMember("charPtrMember", Obj.CharPtrMember);
}

void FromJson(const JsonDeserializer& Deserializer, TestObjectProps& Obj)
{
    Deserializer.DeserializeMember("int32Member", Obj.Int32Member);
    Deserializer.DeserializeMember("uint32Member", Obj.Uint32Member);
    Deserializer.DeserializeMember("int64Member", Obj.Int64Member);
    Deserializer.DeserializeMember("uint64Member", Obj.Uint64Member);
    Deserializer.DeserializeMember("floatMember", Obj.FloatMember);
    Deserializer.DeserializeMember("doubleMember", Obj.DoubleMember);
    Deserializer.DeserializeMember("stringMember", Obj.StringMember);
    Deserializer.DeserializeMember("charPtrMember", Obj.CharPtrMember);
}

struct TestOptionalPropObject
{
    int32_t Int32Member1 = 0;
    int32_t Int32Member2 = 0;
};

void ToJson(JsonSerializer& Serializer, const TestOptionalPropObject& Obj) { Serializer.SerializeMember("int32Member1", Obj.Int32Member1); }

void FromJson(const JsonDeserializer& Deserializer, TestOptionalPropObject& Obj)
{
    Deserializer.DeserializeMember("int32Member1", Obj.Int32Member1);

    EXPECT_FALSE(Deserializer.HasProperty("int32Member2"));
}

struct TestNestedObject
{
    csp::common::String StringMember = "";
};

struct TestParentObject
{
    int32_t Int32Member = 0;
    TestNestedObject Obj;
    float FloatMember = 0;
};

void ToJson(JsonSerializer& Serializer, const TestParentObject& Obj)
{
    Serializer.SerializeMember("int32Member", Obj.Int32Member);
    Serializer.SerializeMember("obj", Obj.Obj);
    Serializer.SerializeMember("floatMember", Obj.FloatMember);
}

void FromJson(const JsonDeserializer& Deserializer, TestParentObject& Obj)
{
    Deserializer.DeserializeMember("int32Member", Obj.Int32Member);
    Deserializer.DeserializeMember("obj", Obj.Obj);
    Deserializer.DeserializeMember("floatMember", Obj.FloatMember);
}

void ToJson(JsonSerializer& Serializer, const TestNestedObject& Obj) { Serializer.SerializeMember("stringMember", Obj.StringMember); }

void FromJson(const JsonDeserializer& Deserializer, TestNestedObject& Obj) { Deserializer.DeserializeMember("stringMember", Obj.StringMember); }

struct TestContainerObject
{
    csp::common::Array<int> IntMembers;
    csp::common::List<float> FloatMembers;
};

void ToJson(JsonSerializer& Serializer, const TestContainerObject& Obj)
{
    Serializer.SerializeMember("intMembers", Obj.IntMembers);
    Serializer.SerializeMember("floatMembers", Obj.FloatMembers);
}

void FromJson(const JsonDeserializer& Deserializer, TestContainerObject& Obj)
{
    Deserializer.DeserializeMember("intMembers", Obj.IntMembers);
    Deserializer.DeserializeMember("floatMembers", Obj.FloatMembers);
}

struct TestObjectContainerObject
{
    csp::common::Array<TestParentObject> ArrayMember;
    csp::common::List<TestParentObject> ListMember;
};

void ToJson(JsonSerializer& Serializer, const TestObjectContainerObject& Obj)
{
    Serializer.SerializeMember("arrayMember", Obj.ArrayMember);
    Serializer.SerializeMember("listMember", Obj.ListMember);
}

void FromJson(const JsonDeserializer& Deserializer, TestObjectContainerObject& Obj)
{
    Deserializer.DeserializeMember("arrayMember", Obj.ArrayMember);
    Deserializer.DeserializeMember("listMember", Obj.ListMember);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonPropertiesTest)
{
    TestObjectProps Obj;
    Obj.Int32Member = 1;
    Obj.Uint32Member = 2;
    Obj.Int64Member = 3;
    Obj.Uint64Member = 4;
    Obj.FloatMember = 5.f;
    Obj.DoubleMember = 6.0;
    Obj.StringMember = "Test";
    Obj.CharPtrMember = "Test2";

    const csp::common::String result = JsonSerializer::Serialize(Obj);

    TestObjectProps Obj2;
    JsonDeserializer::Deserialize(result, Obj2);

    EXPECT_EQ(Obj.Int32Member, Obj2.Int32Member);
    EXPECT_EQ(Obj.Uint32Member, Obj2.Uint32Member);
    EXPECT_EQ(Obj.Int64Member, Obj2.Int64Member);
    EXPECT_EQ(Obj.Uint64Member, Obj2.Uint64Member);
    EXPECT_EQ(Obj.FloatMember, Obj2.FloatMember);
    EXPECT_EQ(Obj.DoubleMember, Obj2.DoubleMember);
    EXPECT_EQ(Obj.StringMember, Obj2.StringMember);
    EXPECT_TRUE(strcmp(Obj.CharPtrMember, Obj2.CharPtrMember));
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonOptionalPropertyTest)
{
    TestOptionalPropObject Obj;
    Obj.Int32Member1 = 5;
    Obj.Int32Member2 = 6;

    const csp::common::String result = JsonSerializer::Serialize(Obj);

    TestOptionalPropObject Obj2;
    JsonDeserializer::Deserialize(result, Obj2);

    EXPECT_EQ(Obj.Int32Member1, Obj2.Int32Member1);
    EXPECT_FALSE(Obj.Int32Member2 == Obj2.Int32Member2);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonNestedObjectTest)
{
    TestParentObject Parent;
    Parent.Int32Member = 1;
    Parent.Obj.StringMember = "Test";
    Parent.FloatMember = 2.f;

    const csp::common::String result = JsonSerializer::Serialize(Parent);

    TestParentObject Parent2;
    JsonDeserializer::Deserialize(result, Parent2);

    auto s1 = Parent.Obj.StringMember;
    auto s2 = Parent2.Obj.StringMember;

    EXPECT_EQ(Parent.Int32Member, Parent2.Int32Member);
    EXPECT_EQ(Parent.Obj.StringMember, Parent2.Obj.StringMember);
    EXPECT_EQ(Parent.FloatMember, Parent2.FloatMember);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonContainerObjectTest)
{
    TestContainerObject Obj;

    Obj.IntMembers = csp::common::Array<int>(3);
    Obj.IntMembers[0] = 1;
    Obj.IntMembers[1] = 2;
    Obj.IntMembers[2] = 3;

    Obj.FloatMembers.Append(4.f);
    Obj.FloatMembers.Append(5.f);
    Obj.FloatMembers.Append(6.f);

    const csp::common::String result = JsonSerializer::Serialize(Obj);

    TestContainerObject Obj2;
    JsonDeserializer::Deserialize(result, Obj2);

    EXPECT_EQ(Obj.IntMembers.Size(), Obj.IntMembers.Size());
    EXPECT_EQ(Obj.IntMembers[0], Obj2.IntMembers[0]);
    EXPECT_EQ(Obj.IntMembers[1], Obj2.IntMembers[1]);
    EXPECT_EQ(Obj.IntMembers[2], Obj2.IntMembers[2]);

    EXPECT_EQ(Obj.FloatMembers.Size(), Obj.FloatMembers.Size());
    EXPECT_EQ(Obj.FloatMembers[0], Obj2.FloatMembers[0]);
    EXPECT_EQ(Obj.FloatMembers[1], Obj2.FloatMembers[1]);
    EXPECT_EQ(Obj.FloatMembers[2], Obj2.FloatMembers[2]);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonObjectContainerObjectTest)
{
    TestObjectContainerObject Obj;

    Obj.ArrayMember = csp::common::Array<TestParentObject>(2);

    TestParentObject NestedObj1;
    NestedObj1.Int32Member = 1;
    NestedObj1.Obj.StringMember = "Test";
    NestedObj1.FloatMember = 2.f;

    TestParentObject NestedObj2;
    NestedObj1.Int32Member = 3;
    NestedObj1.Obj.StringMember = "Test1";
    NestedObj1.FloatMember = 4.f;

    Obj.ArrayMember[0] = NestedObj1;
    Obj.ArrayMember[1] = NestedObj2;

    TestParentObject NestedObj3;
    NestedObj1.Int32Member = 5;
    NestedObj1.Obj.StringMember = "Test2";
    NestedObj1.FloatMember = 6.f;

    TestParentObject NestedObj4;
    NestedObj1.Int32Member = 7;
    NestedObj1.Obj.StringMember = "Test3";
    NestedObj1.FloatMember = 8.f;

    Obj.ListMember.Append(NestedObj3);
    Obj.ListMember.Append(NestedObj4);

    const csp::common::String result = JsonSerializer::Serialize(Obj);

    TestObjectContainerObject Obj2;
    JsonDeserializer::Deserialize(result, Obj2);

    EXPECT_EQ(Obj.ArrayMember.Size(), Obj2.ArrayMember.Size());
    EXPECT_EQ(Obj.ListMember.Size(), Obj2.ListMember.Size());

    for (int i = 0; i < 2; ++i)
    {
        EXPECT_EQ(Obj.ArrayMember[i].Int32Member, Obj.ArrayMember[i].Int32Member);
        EXPECT_EQ(Obj.ArrayMember[i].Obj.StringMember, Obj.ArrayMember[i].Obj.StringMember);
        EXPECT_EQ(Obj.ArrayMember[i].FloatMember, Obj.ArrayMember[i].FloatMember);

        EXPECT_EQ(Obj.ListMember[i].Int32Member, Obj.ListMember[i].Int32Member);
        EXPECT_EQ(Obj.ListMember[i].Obj.StringMember, Obj.ListMember[i].Obj.StringMember);
        EXPECT_EQ(Obj.ListMember[i].FloatMember, Obj.ListMember[i].FloatMember);
    }
}

#endif