/*
 * Copyright 2025 Magnopus LLC

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

#include "Multiplayer/SignalRSerializer.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::multiplayer;

// Test we can serialize/deserialize int64 values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeIntTest)
{
    const int64_t value = 2;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    int64_t deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we can serialize/deserialize uint64 values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintTest)
{
    const uint64_t value = 1;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    uint64_t deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

/*
    This is important, as signalr only supports int64 signed integer types,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeShortTest)
{
    const short value = -2;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    short deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we throw an exception if the value being deserialized is larger than the input type.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintInvalidSizeTest)
{
    const uint64_t value = 99999999;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    uint16_t deserializedValue;
    EXPECT_THROW(deserializer.ReadValue(deserializedValue), std::runtime_error);
}

// Test we throw an exception if the value being deserialized is larger than the input type.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeIntInvalidSizeTest)
{
    const int64_t value = 99999999;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    int16_t deserializedValue;
    EXPECT_THROW(deserializer.ReadValue(deserializedValue), std::runtime_error);
}

// Test we throw an exception if the value being deserialized is larger than the input type.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeIntInvalidSizeMinTest)
{
    const int64_t value = -99999999;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    int16_t deserializedValue;
    EXPECT_THROW(deserializer.ReadValue(deserializedValue), std::runtime_error);
}

/*
    This is important, as signalr only supports uint64 unsigned integer types,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUshortTest)
{
    const unsigned short value = 2;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    unsigned short deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we can serialize/deserialize double values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeDoubleTest)
{
    const double value = 3.0;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    double deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

/*
    This is important, as signalr only supports doubles, and not floats,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeFloatTest)
{
    const float value = 3.f;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    float deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we can serialize/deserialize bool values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeBoolTest)
{
    const bool value = false;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    bool deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we can serialize/deserialize string values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringTest)
{
    const std::string value = "Test";

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    std::string deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we can serialize/deserialize optionals when theyre unset.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUnsetOptionalTest)
{
    const std::optional<std::string> value;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    std::optional<std::string> deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

// Test we can serialize/deserialize optionals when theyre set.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeSetOptionalTest)
{
    const std::optional<std::string> value = "Test";

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    std::optional<std::string> deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeArrayTest)
{
    std::vector<float> value { -0.1f, 1.f, 2.f, 3.f };

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };
    std::vector<float> deserializedValue;

    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeArrayMultipleTypesTest)
{
    const auto value = std::make_tuple(1ll, 2ull, 3.0, true, std::string { "Test" }, nullptr);

    SignalRSerializer serializer;
    serializer.StartWriteArray();
    {
        serializer.WriteValue(std::get<0>(value));
        serializer.WriteValue(std::get<1>(value));
        serializer.WriteValue(std::get<2>(value));
        serializer.WriteValue(std::get<3>(value));
        serializer.WriteValue(std::get<4>(value));
        serializer.WriteValue(std::get<5>(value));
    }
    serializer.EndWriteArray();

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };
    std::tuple<int64_t, uint64_t, double, bool, std::string, std::nullptr_t> deserializedValue;

    size_t arraySize = 0;
    deserializer.StartReadArray(arraySize);
    {
        deserializer.ReadValue(std::get<0>(deserializedValue));
        deserializer.ReadValue(std::get<1>(deserializedValue));
        deserializer.ReadValue(std::get<2>(deserializedValue));
        deserializer.ReadValue(std::get<3>(deserializedValue));
        deserializer.ReadValue(std::get<4>(deserializedValue));
        deserializer.ReadValue(std::get<5>(deserializedValue));
    }
    deserializer.EndReadArray();

    EXPECT_EQ(deserializedValue, value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeArrayMultipleArraysTest)
{
    const auto value = std::make_tuple(1ll, 2ull, std::make_tuple(5ull, std::string { "Test2" }), 3.0, true, std::string { "Test" }, nullptr);

    SignalRSerializer serializer;
    serializer.StartWriteArray();
    {
        serializer.WriteValue(std::get<0>(value));
        serializer.WriteValue(std::get<1>(value));

        serializer.StartWriteArray();
        {
            serializer.WriteValue(std::get<0>(std::get<2>(value)));
            serializer.WriteValue(std::get<1>(std::get<2>(value)));
        }
        serializer.EndWriteArray();

        serializer.WriteValue(std::get<3>(value));
        serializer.WriteValue(std::get<4>(value));
        serializer.WriteValue(std::get<5>(value));
        serializer.WriteValue(std::get<6>(value));
    }
    serializer.EndWriteArray();

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };
    std::tuple<int64_t, uint64_t, std::tuple<uint64_t, std::string>, double, bool, std::string, std::nullptr_t> deserializedValue;

    size_t arraySize = 0;
    deserializer.StartReadArray(arraySize);
    {
        deserializer.ReadValue(std::get<0>(deserializedValue));
        deserializer.ReadValue(std::get<1>(deserializedValue));

        size_t arraySize2 = 0;
        deserializer.StartReadArray(arraySize2);
        {
            deserializer.ReadValue(std::get<0>(std::get<2>(deserializedValue)));
            deserializer.ReadValue(std::get<1>(std::get<2>(deserializedValue)));
        }
        deserializer.EndReadArray();

        deserializer.ReadValue(std::get<3>(deserializedValue));
        deserializer.ReadValue(std::get<4>(deserializedValue));
        deserializer.ReadValue(std::get<5>(deserializedValue));
        deserializer.ReadValue(std::get<6>(deserializedValue));
    }
    deserializer.EndReadArray();

    EXPECT_EQ(deserializedValue, value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintMapTest)
{
    std::map<uint64_t, std::string> value { { 0, "Test1" }, { 1, "Test2" }, { 2, "Test3" } };

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };
    std::map<uint64_t, std::string> deserializedValue;

    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintMapMultipleTypesTest)
{
    std::pair<uint64_t, int64_t> pair1 { 0, 1 };
    std::pair<uint64_t, uint64_t> pair2 { 1, 2ull };
    std::pair<uint64_t, double> pair3 { 2, 3.0 };
    std::pair<uint64_t, bool> pair4 { 3, true };
    std::pair<uint64_t, std::string> pair5 { 4, "Test1" };
    std::pair<uint64_t, std::nullptr_t> pair6 { 5, nullptr };

    SignalRSerializer serializer;
    serializer.StartWriteUintMap();
    {
        serializer.WriteKeyValue(pair1.first, pair1.second);
        serializer.WriteKeyValue(pair2.first, pair2.second);
        serializer.WriteKeyValue(pair3.first, pair3.second);
        serializer.WriteKeyValue(pair4.first, pair4.second);
        serializer.WriteKeyValue(pair5.first, pair5.second);
        serializer.WriteKeyValue(pair6.first, pair6.second);
    }
    serializer.EndWriteUintMap();

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    std::pair<uint64_t, int64_t> deserializedPair1;
    std::pair<uint64_t, uint64_t> deserializedPair2;
    std::pair<uint64_t, double> deserializedPair3;
    std::pair<uint64_t, bool> deserializedPair4;
    std::pair<uint64_t, std::string> deserializedPair5;
    std::pair<uint64_t, std::nullptr_t> deserializedPair6;

    size_t arraySize = 0;
    deserializer.StartReadUintMap(arraySize);
    {
        deserializer.ReadKeyValue(deserializedPair1);
        deserializer.ReadKeyValue(deserializedPair2);
        deserializer.ReadKeyValue(deserializedPair3);
        deserializer.ReadKeyValue(deserializedPair4);
        deserializer.ReadKeyValue(deserializedPair5);
        deserializer.ReadKeyValue(deserializedPair6);
    }
    deserializer.EndReadUintMap();

    EXPECT_EQ(pair1, deserializedPair1);
    EXPECT_EQ(pair2, deserializedPair2);
    EXPECT_EQ(pair3, deserializedPair3);
    EXPECT_EQ(pair4, deserializedPair4);
    EXPECT_EQ(pair5, deserializedPair5);
    EXPECT_EQ(pair6, deserializedPair6);
}

/*
    This is important, as signalr only supports uint64 uint map keys,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUshortMapTest)
{
    std::map<unsigned short, std::string> value { { 0, "Test1" }, { 1, "Test2" }, { 2, "Test3" } };

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };
    std::map<unsigned short, std::string> deserializedValue;

    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

/*
    This is important, as signalr only supports uint64 uint map keys,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeShortMapMultipleTypes)
{
    std::pair<unsigned short, int64_t> pair1 { 0, 1 };
    std::pair<unsigned short, uint64_t> pair2 { 1, 2ull };
    std::pair<unsigned short, double> pair3 { 2, 3.0 };

    SignalRSerializer serializer;
    serializer.StartWriteUintMap();
    {
        serializer.WriteKeyValue(pair1.first, pair1.second);
        serializer.WriteKeyValue(pair2.first, pair2.second);
        serializer.WriteKeyValue(pair3.first, pair3.second);
    }
    serializer.EndWriteUintMap();

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    std::pair<unsigned short, int64_t> deserializedPair1;
    std::pair<unsigned short, uint64_t> deserializedPair2;
    std::pair<unsigned short, double> deserializedPair3;

    size_t arraySize = 0;
    deserializer.StartReadUintMap(arraySize);
    {
        deserializer.ReadKeyValue(deserializedPair1);
        deserializer.ReadKeyValue(deserializedPair2);
        deserializer.ReadKeyValue(deserializedPair3);
    }
    deserializer.EndReadUintMap();

    EXPECT_EQ(pair1, deserializedPair1);
    EXPECT_EQ(pair2, deserializedPair2);
    EXPECT_EQ(pair3, deserializedPair3);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringMapTest)
{
    std::map<std::string, std::string> value { { "0", "Test1" }, { "1", "Test2" }, { "2", "Test3" } };

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };
    std::map<std::string, std::string> deserializedValue;

    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringMapMultipleTypes)
{
    std::pair<std::string, int64_t> pair1 { "0", 1 };
    std::pair<std::string, uint64_t> pair2 { "1", 2ull };
    std::pair<std::string, double> pair3 { "2", 3.0 };
    std::pair<std::string, bool> pair4 { "3", true };
    std::pair<std::string, std::string> pair5 { "4", "Test1" };
    std::pair<std::string, std::nullptr_t> pair6 { "5", nullptr };

    SignalRSerializer serializer;
    serializer.StartWriteStringMap();
    {
        serializer.WriteKeyValue(pair1.first, pair1.second);
        serializer.WriteKeyValue(pair2.first, pair2.second);
        serializer.WriteKeyValue(pair3.first, pair3.second);
        serializer.WriteKeyValue(pair4.first, pair4.second);
        serializer.WriteKeyValue(pair5.first, pair5.second);
        serializer.WriteKeyValue(pair6.first, pair6.second);
    }
    serializer.EndWriteStringMap();

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    std::pair<std::string, int64_t> deserializedPair1;
    std::pair<std::string, uint64_t> deserializedPair2;
    std::pair<std::string, double> deserializedPair3;
    std::pair<std::string, bool> deserializedPair4;
    std::pair<std::string, std::string> deserializedPair5;
    std::pair<std::string, std::nullptr_t> deserializedPair6;

    size_t arraySize = 0;
    deserializer.StartReadStringMap(arraySize);
    {
        deserializer.ReadKeyValue(deserializedPair1);
        deserializer.ReadKeyValue(deserializedPair2);
        deserializer.ReadKeyValue(deserializedPair3);
        deserializer.ReadKeyValue(deserializedPair4);
        deserializer.ReadKeyValue(deserializedPair5);
        deserializer.ReadKeyValue(deserializedPair6);
    }
    deserializer.EndReadStringMap();

    EXPECT_EQ(pair1, deserializedPair1);
    EXPECT_EQ(pair2, deserializedPair2);
    EXPECT_EQ(pair3, deserializedPair3);
    EXPECT_EQ(pair4, deserializedPair4);
    EXPECT_EQ(pair5, deserializedPair5);
    EXPECT_EQ(pair6, deserializedPair6);
}

struct TestObject1 : public ISignalRSerializable, public ISignalRDeserializable
{
    std::vector<std::string> Values;

    void Serialize(SignalRSerializer& serializer) const override { serializer.WriteValue(Values); }
    void Deserialize(SignalRDeserializer& deserializer) override { deserializer.ReadValue(Values); }

    bool operator==(const TestObject1& other) const { return Values == other.Values; }
};

struct TestObject2 : public ISignalRSerializable, public ISignalRDeserializable
{
    int64_t Int64Member = 0;
    uint64_t Uint64Member = 0;
    double DoubleMember = 0;
    bool BoolMember = false;
    std::string StringMember;
    std::optional<TestObject1> OptionalMember;
    std::vector<TestObject1> ArrayMember;
    std::map<uint64_t, TestObject1> UintMapMember;
    std::map<std::string, TestObject1> StringMapMember;

    void Serialize(SignalRSerializer& serializer) const override
    {
        serializer.StartWriteArray();
        {
            serializer.WriteValue(Int64Member);
            serializer.WriteValue(Uint64Member);
            serializer.WriteValue(DoubleMember);
            serializer.WriteValue(BoolMember);
            serializer.WriteValue(StringMember);
            serializer.WriteValue(OptionalMember);
            serializer.WriteValue(ArrayMember);
            serializer.WriteValue(UintMapMember);
            serializer.WriteValue(StringMapMember);
        }
        serializer.EndWriteArray();
    }

    void Deserialize(SignalRDeserializer& deserializer) override
    {
        size_t arraySize = 0;
        deserializer.StartReadArray(arraySize);
        {
            deserializer.ReadValue(Int64Member);
            deserializer.ReadValue(Uint64Member);
            deserializer.ReadValue(DoubleMember);
            deserializer.ReadValue(BoolMember);
            deserializer.ReadValue(StringMember);
            deserializer.ReadValue(OptionalMember);
            deserializer.ReadValue(ArrayMember);
            deserializer.ReadValue(UintMapMember);
            deserializer.ReadValue(StringMapMember);
        }
        deserializer.EndReadArray();
    }

    bool operator==(const TestObject2& other) const
    {
        return (Int64Member == other.Int64Member && Uint64Member == other.Uint64Member && DoubleMember == other.DoubleMember
            && BoolMember == other.BoolMember && StringMember == other.StringMember && OptionalMember == other.OptionalMember
            && ArrayMember == other.ArrayMember && UintMapMember == other.UintMapMember && StringMapMember == other.StringMapMember);
    }
};

// Todo: split these into more focused tests.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeObjectTest)
{
    TestObject1 child1;
    child1.Values.push_back(std::string { "Test1" });
    child1.Values.push_back(std::string { "Test2" });

    TestObject1 child2;
    child2.Values.push_back(std::string { "Test3" });
    child2.Values.push_back(std::string { "Test4" });

    TestObject1 child3;
    child3.Values.push_back(std::string { "Test5" });
    child3.Values.push_back(std::string { "Test6" });

    TestObject1 child4;
    child4.Values.push_back(std::string { "Test7" });
    child4.Values.push_back(std::string { "Test8" });

    TestObject1 child23;

    TestObject2 value;
    value.Int64Member = 1ll;
    value.Uint64Member = 2ull;
    value.DoubleMember = 3.0;
    value.BoolMember = true;
    value.StringMember = "Test";
    value.OptionalMember = child1;
    value.ArrayMember.push_back(child2);
    value.UintMapMember[0] = child3;
    value.StringMapMember["0"] = child4;

    SignalRSerializer serializer;
    serializer.WriteValue(value);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    TestObject2 deserializedValue;
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, value);
}