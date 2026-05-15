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

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "Json/JsonSerializer.h"
#include "Json/JsonParseHelper.h"
#include <CSP/Systems/SystemsManager.h>

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
    std::vector<float> ArrayMember = { 1.1f, 2.2f, 3.3f };
    std::map<std::string, float> MapMember = { { "Key1", 1.1f }, { "Key2", 2.2f } };
};

void ToJson(JsonSerializer& serializer, const TestObjectProps& obj)
{
    serializer.SerializeMember("int32Member", obj.Int32Member);
    serializer.SerializeMember("uint32Member", obj.Uint32Member);
    serializer.SerializeMember("int64Member", obj.Int64Member);
    serializer.SerializeMember("uint64Member", obj.Uint64Member);
    serializer.SerializeMember("floatMember", obj.FloatMember);
    serializer.SerializeMember("doubleMember", obj.DoubleMember);
    serializer.SerializeMember("stringMember", obj.StringMember);
    serializer.SerializeMember("arrayMember", obj.ArrayMember);
    serializer.SerializeMember("mapMember", obj.MapMember);
}

void FromJson(const JsonDeserializer& deserializer, TestObjectProps& obj)
{
    deserializer.DeserializeMember("int32Member", obj.Int32Member);
    deserializer.DeserializeMember("uint32Member", obj.Uint32Member);
    deserializer.DeserializeMember("int64Member", obj.Int64Member);
    deserializer.DeserializeMember("uint64Member", obj.Uint64Member);
    deserializer.DeserializeMember("floatMember", obj.FloatMember);
    deserializer.DeserializeMember("doubleMember", obj.DoubleMember);
    deserializer.DeserializeMember("stringMember", obj.StringMember);
    deserializer.DeserializeMember("arrayMember", obj.ArrayMember);
    deserializer.DeserializeMember("mapMember", obj.MapMember);
}

struct TestOptionalPropObject
{
    int32_t Int32Member1 = 0;
    int32_t Int32Member2 = 0;
};

void ToJson(JsonSerializer& serializer, const TestOptionalPropObject& obj) { serializer.SerializeMember("int32Member1", obj.Int32Member1); }

void FromJson(const JsonDeserializer& deserializer, TestOptionalPropObject& obj)
{
    deserializer.DeserializeMember("int32Member1", obj.Int32Member1);

    EXPECT_FALSE(deserializer.HasProperty("int32Member2"));
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

void ToJson(JsonSerializer& serializer, const TestParentObject& obj)
{
    serializer.SerializeMember("int32Member", obj.Int32Member);
    serializer.SerializeMember("obj", obj.Obj);
    serializer.SerializeMember("floatMember", obj.FloatMember);
}

void FromJson(const JsonDeserializer& deserializer, TestParentObject& obj)
{
    deserializer.DeserializeMember("int32Member", obj.Int32Member);
    deserializer.DeserializeMember("obj", obj.Obj);
    deserializer.DeserializeMember("floatMember", obj.FloatMember);
}

void ToJson(JsonSerializer& serializer, const TestNestedObject& obj) { serializer.SerializeMember("stringMember", obj.StringMember); }

void FromJson(const JsonDeserializer& deserializer, TestNestedObject& obj) { deserializer.DeserializeMember("stringMember", obj.StringMember); }

struct TestContainerObject
{
    csp::common::Array<int> IntMembers;
    csp::common::List<float> FloatMembers;
};

struct TestStdContainerObject
{
    std::vector<float> Vector;
    std::map<std::string, int> Map;
};

void ToJson(JsonSerializer& serializer, const TestStdContainerObject& obj)
{
    serializer.SerializeMember("vectorMembers", obj.Vector);
    serializer.SerializeMember("mapMembers", obj.Map);
}

void FromJson(const JsonDeserializer& deserializer, TestStdContainerObject& obj)
{
    deserializer.DeserializeMember("vectorMembers", obj.Vector);
    deserializer.DeserializeMember("mapMembers", obj.Map);
}

void ToJson(JsonSerializer& serializer, const TestContainerObject& obj)
{
    serializer.SerializeMember("intMembers", obj.IntMembers);
    serializer.SerializeMember("floatMembers", obj.FloatMembers);
}

void FromJson(const JsonDeserializer& deserializer, TestContainerObject& obj)
{
    deserializer.DeserializeMember("intMembers", obj.IntMembers);
    deserializer.DeserializeMember("floatMembers", obj.FloatMembers);
}

struct TestObjectContainerObject
{
    csp::common::Array<TestParentObject> ArrayMember;
    csp::common::List<TestParentObject> ListMember;
};

void ToJson(JsonSerializer& serializer, const TestObjectContainerObject& obj)
{
    serializer.SerializeMember("arrayMember", obj.ArrayMember);
    serializer.SerializeMember("listMember", obj.ListMember);
}

void FromJson(const JsonDeserializer& deserializer, TestObjectContainerObject& obj)
{
    deserializer.DeserializeMember("arrayMember", obj.ArrayMember);
    deserializer.DeserializeMember("listMember", obj.ListMember);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonPropertiesTest)
{
    TestObjectProps obj;
    obj.Int32Member = 1;
    obj.Uint32Member = 2;
    obj.Int64Member = 3;
    obj.Uint64Member = 4;
    obj.FloatMember = 5.f;
    obj.DoubleMember = 6.0;
    obj.StringMember = "Test";

    const csp::common::String result = JsonSerializer::Serialize(obj);

    TestObjectProps obj2;
    JsonDeserializer::Deserialize(result, obj2);

    EXPECT_EQ(obj.Int32Member, obj2.Int32Member);
    EXPECT_EQ(obj.Uint32Member, obj2.Uint32Member);
    EXPECT_EQ(obj.Int64Member, obj2.Int64Member);
    EXPECT_EQ(obj.Uint64Member, obj2.Uint64Member);
    EXPECT_EQ(obj.FloatMember, obj2.FloatMember);
    EXPECT_EQ(obj.DoubleMember, obj2.DoubleMember);
    EXPECT_EQ(obj.StringMember, obj2.StringMember);
    EXPECT_EQ(obj.ArrayMember, obj2.ArrayMember);
    EXPECT_EQ(obj.MapMember, obj2.MapMember);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonOptionalPropertyTest)
{
    TestOptionalPropObject obj;
    obj.Int32Member1 = 5;
    obj.Int32Member2 = 6;

    const csp::common::String result = JsonSerializer::Serialize(obj);

    TestOptionalPropObject obj2;
    JsonDeserializer::Deserialize(result, obj2);

    EXPECT_EQ(obj.Int32Member1, obj2.Int32Member1);
    EXPECT_FALSE(obj.Int32Member2 == obj2.Int32Member2);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonNestedObjectTest)
{
    TestParentObject parent;
    parent.Int32Member = 1;
    parent.Obj.StringMember = "Test";
    parent.FloatMember = 2.f;

    const csp::common::String result = JsonSerializer::Serialize(parent);

    TestParentObject parent2;
    JsonDeserializer::Deserialize(result, parent2);

    auto s1 = parent.Obj.StringMember;
    auto s2 = parent2.Obj.StringMember;

    EXPECT_EQ(parent.Int32Member, parent2.Int32Member);
    EXPECT_EQ(parent.Obj.StringMember, parent2.Obj.StringMember);
    EXPECT_EQ(parent.FloatMember, parent2.FloatMember);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonContainerObjectTest)
{
    TestContainerObject obj;

    obj.IntMembers = csp::common::Array<int>(3);
    obj.IntMembers[0] = 1;
    obj.IntMembers[1] = 2;
    obj.IntMembers[2] = 3;

    obj.FloatMembers.Append(4.f);
    obj.FloatMembers.Append(5.f);
    obj.FloatMembers.Append(6.f);

    const csp::common::String result = JsonSerializer::Serialize(obj);

    TestContainerObject obj2;
    JsonDeserializer::Deserialize(result, obj2);

    EXPECT_EQ(obj.IntMembers.Size(), obj2.IntMembers.Size());
    EXPECT_EQ(obj.IntMembers[0], obj2.IntMembers[0]);
    EXPECT_EQ(obj.IntMembers[1], obj2.IntMembers[1]);
    EXPECT_EQ(obj.IntMembers[2], obj2.IntMembers[2]);

    EXPECT_EQ(obj.FloatMembers.Size(), obj2.FloatMembers.Size());
    EXPECT_EQ(obj.FloatMembers[0], obj2.FloatMembers[0]);
    EXPECT_EQ(obj.FloatMembers[1], obj2.FloatMembers[1]);
    EXPECT_EQ(obj.FloatMembers[2], obj2.FloatMembers[2]);
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonEmptyContainerObjectTest)
{
    TestContainerObject obj;

    obj.IntMembers = csp::common::Array<int>(0);

    const csp::common::String result = JsonSerializer::Serialize(obj);

    TestContainerObject obj2;
    JsonDeserializer::Deserialize(result, obj2);

    EXPECT_EQ(obj.IntMembers.Size(), obj2.IntMembers.Size());
    EXPECT_EQ(obj.FloatMembers.Size(), obj2.FloatMembers.Size());
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonObjectContainerObjectTest)
{
    TestObjectContainerObject obj;

    obj.ArrayMember = csp::common::Array<TestParentObject>(2);

    TestParentObject nestedObj1;
    nestedObj1.Int32Member = 1;
    nestedObj1.Obj.StringMember = "Test";
    nestedObj1.FloatMember = 2.f;

    TestParentObject nestedObj2;
    nestedObj1.Int32Member = 3;
    nestedObj1.Obj.StringMember = "Test1";
    nestedObj1.FloatMember = 4.f;

    obj.ArrayMember[0] = nestedObj1;
    obj.ArrayMember[1] = nestedObj2;

    TestParentObject nestedObj3;
    nestedObj1.Int32Member = 5;
    nestedObj1.Obj.StringMember = "Test2";
    nestedObj1.FloatMember = 6.f;

    TestParentObject nestedObj4;
    nestedObj1.Int32Member = 7;
    nestedObj1.Obj.StringMember = "Test3";
    nestedObj1.FloatMember = 8.f;

    obj.ListMember.Append(nestedObj3);
    obj.ListMember.Append(nestedObj4);

    const csp::common::String result = JsonSerializer::Serialize(obj);

    TestObjectContainerObject obj2;
    JsonDeserializer::Deserialize(result, obj2);

    EXPECT_EQ(obj.ArrayMember.Size(), obj2.ArrayMember.Size());
    EXPECT_EQ(obj.ListMember.Size(), obj2.ListMember.Size());

    for (int i = 0; i < 2; ++i)
    {
        EXPECT_EQ(obj.ArrayMember[i].Int32Member, obj.ArrayMember[i].Int32Member);
        EXPECT_EQ(obj.ArrayMember[i].Obj.StringMember, obj.ArrayMember[i].Obj.StringMember);
        EXPECT_EQ(obj.ArrayMember[i].FloatMember, obj.ArrayMember[i].FloatMember);

        EXPECT_EQ(obj.ListMember[i].Int32Member, obj.ListMember[i].Int32Member);
        EXPECT_EQ(obj.ListMember[i].Obj.StringMember, obj.ListMember[i].Obj.StringMember);
        EXPECT_EQ(obj.ListMember[i].FloatMember, obj.ListMember[i].FloatMember);
    }
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, JsonEmptyStdContainerObjectTest)
{
    TestStdContainerObject obj;

    const csp::common::String result = JsonSerializer::Serialize(obj);

    TestStdContainerObject obj2;
    JsonDeserializer::Deserialize(result, obj2);

    EXPECT_EQ(obj.Vector.size(), obj2.Vector.size());
    EXPECT_EQ(obj.Map.size(), obj2.Map.size());
}

CSP_INTERNAL_TEST(CSPEngine, JsonTests, ParseWithErrorLogging_LogsFailure)
{
    // Initialize CSP so we can test that ParseWithErrorLogging logs errors to the LogSystem.
    // Note: If we need to write additional tests around logging in the future, consider creating a separate test fixture.
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    // Set a log callback to capture log messages for verification.
    std::string log;
    logSystem.SetLogCallback([&]([[maybe_unused]] csp::common::LogLevel level, const csp::common::String& message) { log += message; });

    const csp::common::String invalidJson = "{ invalid json }";
    rapidjson::Document doc;

    rapidjson::ParseResult result = ParseWithErrorLogging(doc, invalidJson, "ParseWithErrorLogging_LogsFailure");

    EXPECT_NE(result, rapidjson::kParseErrorNone);
    
    EXPECT_EQ(
        log,
        "Error: ParseWithErrorLogging_LogsFailure: JSON parse error: Missing a name for object member. (at offset 2). Context: { invalid json }");

    csp::CSPFoundation::Shutdown();
}
