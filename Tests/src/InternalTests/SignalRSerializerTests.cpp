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
    const int64_t Value = 2;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    int64_t DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// Test we can serialize/deserialize uint64 values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintTest)
{
    const uint64_t Value = 1;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    uint64_t DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

/*
    This is important, as signalr only supports int64 signed integer types,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeShortTest)
{
    const short Value = -2;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    short DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

/*
    This is important, as signalr only supports uint64 unsigned integer types,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUshortTest)
{
    const unsigned short Value = 2;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    unsigned short DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// Test we can serialize/deserialize double values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeDoubleTest)
{
    const double Value = 3.0;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    double DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

/*
    This is important, as signalr only supports doubles, and not floats,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeFloatTest)
{
    const float Value = 3.f;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    float DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// Test we can serialize/deserialize bool values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeBoolTest)
{
    const bool Value = false;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    bool DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// Test we can serialize/deserialize string values.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringTest)
{
    const std::string Value = "Test";

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::string DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// Test we can serialize/deserialize optionals when theyre unset.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUnsetOptionalTest)
{
    const std::optional<std::string> Value;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::optional<std::string> DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// Test we can serialize/deserialize optionals when theyre set.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeSetOptionalTest)
{
    const std::optional<std::string> Value = "Test";

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::optional<std::string> DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeArrayTest)
{
    std::vector<float> Value { -0.1f, 1.f, 2.f, 3.f };

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };
    std::vector<float> DeserializedValue;

    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeArrayMultipleTypes)
{
    const auto Value = std::make_tuple(1ll, 2ull, 3.0, true, std::string { "Test" }, nullptr);

    SignalRSerializer Serializer;
    Serializer.StartWriteArray();
    {
        Serializer.WriteValue(std::get<0>(Value));
        Serializer.WriteValue(std::get<1>(Value));
        Serializer.WriteValue(std::get<2>(Value));
        Serializer.WriteValue(std::get<3>(Value));
        Serializer.WriteValue(std::get<4>(Value));
        Serializer.WriteValue(std::get<5>(Value));
    }
    Serializer.EndWriteArray();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };
    std::tuple<int64_t, uint64_t, double, bool, std::string, nullptr_t> DeserializedValue;

    size_t ArraySize = 0;
    Deserializer.StartReadArray(ArraySize);
    {
        Deserializer.ReadValue(std::get<0>(DeserializedValue));
        Deserializer.ReadValue(std::get<1>(DeserializedValue));
        Deserializer.ReadValue(std::get<2>(DeserializedValue));
        Deserializer.ReadValue(std::get<3>(DeserializedValue));
        Deserializer.ReadValue(std::get<4>(DeserializedValue));
        Deserializer.ReadValue(std::get<5>(DeserializedValue));
    }
    Deserializer.EndReadArray();

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintMapTest)
{
    std::map<uint64_t, std::string> Value { { 0, "Test1" }, { 1, "Test2" }, { 2, "Test3" } };

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };
    std::map<uint64_t, std::string> DeserializedValue;

    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintMapMultipleTypes)
{
    std::pair<uint64_t, int64_t> Pair1 { 0, 1 };
    std::pair<uint64_t, uint64_t> Pair2 { 1, 2ull };
    std::pair<uint64_t, double> Pair3 { 2, 3.0 };
    std::pair<uint64_t, bool> Pair4 { 3, true };
    std::pair<uint64_t, std::string> Pair5 { 4, "Test1" };
    std::pair<uint64_t, nullptr_t> Pair6 { 5, nullptr };

    SignalRSerializer Serializer;
    Serializer.StartWriteUintMap();
    {
        Serializer.WriteKeyValue(Pair1.first, Pair1.second);
        Serializer.WriteKeyValue(Pair2.first, Pair2.second);
        Serializer.WriteKeyValue(Pair3.first, Pair3.second);
        Serializer.WriteKeyValue(Pair4.first, Pair4.second);
        Serializer.WriteKeyValue(Pair5.first, Pair5.second);
        Serializer.WriteKeyValue(Pair6.first, Pair6.second);
    }
    Serializer.EndWriteUintMap();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::pair<uint64_t, int64_t> DeserializedPair1;
    std::pair<uint64_t, uint64_t> DeserializedPair2;
    std::pair<uint64_t, double> DeserializedPair3;
    std::pair<uint64_t, bool> DeserializedPair4;
    std::pair<uint64_t, std::string> DeserializedPair5;
    std::pair<uint64_t, nullptr_t> DeserializedPair6;

    size_t ArraySize = 0;
    Deserializer.StartReadUintMap(ArraySize);
    {
        Deserializer.ReadKeyValue(DeserializedPair1);
        Deserializer.ReadKeyValue(DeserializedPair2);
        Deserializer.ReadKeyValue(DeserializedPair3);
        Deserializer.ReadKeyValue(DeserializedPair4);
        Deserializer.ReadKeyValue(DeserializedPair5);
        Deserializer.ReadKeyValue(DeserializedPair6);
    }
    Deserializer.EndReadUintMap();

    EXPECT_EQ(Pair1, DeserializedPair1);
    EXPECT_EQ(Pair2, DeserializedPair2);
    EXPECT_EQ(Pair3, DeserializedPair3);
    EXPECT_EQ(Pair4, DeserializedPair4);
    EXPECT_EQ(Pair5, DeserializedPair5);
    EXPECT_EQ(Pair6, DeserializedPair6);
}

/*
    This is important, as signalr only supports uint64 uint map keys,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUshortMapTest)
{
    std::map<unsigned short, std::string> Value { { 0, "Test1" }, { 1, "Test2" }, { 2, "Test3" } };

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };
    std::map<unsigned short, std::string> DeserializedValue;

    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

/*
    This is important, as signalr only supports uint64 uint map keys,
    so we need to test our internal conversion logic.
*/
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeShortMapMultipleTypes)
{
    std::pair<unsigned short, int64_t> Pair1 { 0, 1 };
    std::pair<unsigned short, uint64_t> Pair2 { 1, 2ull };
    std::pair<unsigned short, double> Pair3 { 2, 3.0 };

    SignalRSerializer Serializer;
    Serializer.StartWriteUintMap();
    {
        Serializer.WriteKeyValue(Pair1.first, Pair1.second);
        Serializer.WriteKeyValue(Pair2.first, Pair2.second);
        Serializer.WriteKeyValue(Pair3.first, Pair3.second);
    }
    Serializer.EndWriteUintMap();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::pair<unsigned short, int64_t> DeserializedPair1;
    std::pair<unsigned short, uint64_t> DeserializedPair2;
    std::pair<unsigned short, double> DeserializedPair3;

    size_t ArraySize = 0;
    Deserializer.StartReadUintMap(ArraySize);
    {
        Deserializer.ReadKeyValue(DeserializedPair1);
        Deserializer.ReadKeyValue(DeserializedPair2);
        Deserializer.ReadKeyValue(DeserializedPair3);
    }
    Deserializer.EndReadUintMap();

    EXPECT_EQ(Pair1, DeserializedPair1);
    EXPECT_EQ(Pair2, DeserializedPair2);
    EXPECT_EQ(Pair3, DeserializedPair3);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringMapTest)
{
    std::map<std::string, std::string> Value { { "0", "Test1" }, { "1", "Test2" }, { "2", "Test3" } };

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };
    std::map<std::string, std::string> DeserializedValue;

    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringMapMultipleTypes)
{
    std::pair<std::string, int64_t> Pair1 { "0", 1 };
    std::pair<std::string, uint64_t> Pair2 { "1", 2ull };
    std::pair<std::string, double> Pair3 { "2", 3.0 };
    std::pair<std::string, bool> Pair4 { "3", true };
    std::pair<std::string, std::string> Pair5 { "4", "Test1" };
    std::pair<std::string, nullptr_t> Pair6 { "5", nullptr };

    SignalRSerializer Serializer;
    Serializer.StartWriteStringMap();
    {
        Serializer.WriteKeyValue(Pair1.first, Pair1.second);
        Serializer.WriteKeyValue(Pair2.first, Pair2.second);
        Serializer.WriteKeyValue(Pair3.first, Pair3.second);
        Serializer.WriteKeyValue(Pair4.first, Pair4.second);
        Serializer.WriteKeyValue(Pair5.first, Pair5.second);
        Serializer.WriteKeyValue(Pair6.first, Pair6.second);
    }
    Serializer.EndWriteStringMap();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::pair<std::string, int64_t> DeserializedPair1;
    std::pair<std::string, uint64_t> DeserializedPair2;
    std::pair<std::string, double> DeserializedPair3;
    std::pair<std::string, bool> DeserializedPair4;
    std::pair<std::string, std::string> DeserializedPair5;
    std::pair<std::string, nullptr_t> DeserializedPair6;

    size_t ArraySize = 0;
    Deserializer.StartReadStringMap(ArraySize);
    {
        Deserializer.ReadKeyValue(DeserializedPair1);
        Deserializer.ReadKeyValue(DeserializedPair2);
        Deserializer.ReadKeyValue(DeserializedPair3);
        Deserializer.ReadKeyValue(DeserializedPair4);
        Deserializer.ReadKeyValue(DeserializedPair5);
        Deserializer.ReadKeyValue(DeserializedPair6);
    }
    Deserializer.EndReadStringMap();

    EXPECT_EQ(Pair1, DeserializedPair1);
    EXPECT_EQ(Pair2, DeserializedPair2);
    EXPECT_EQ(Pair3, DeserializedPair3);
    EXPECT_EQ(Pair4, DeserializedPair4);
    EXPECT_EQ(Pair5, DeserializedPair5);
    EXPECT_EQ(Pair6, DeserializedPair6);
}

struct TestObject1 : public ISignalRSerializable, public ISignalRDeserializable
{
    std::vector<std::string> Values;

    void Serialize(SignalRSerializer& Serializer) const override { Serializer.WriteValue(Values); }
    void Deserialize(SignalRDeserializer& Deserializer) override { Deserializer.ReadValue(Values); }

    bool operator==(const TestObject1& Other) const { return Values == Other.Values; }
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

    void Serialize(SignalRSerializer& Serializer) const override
    {
        Serializer.StartWriteArray();
        {
            Serializer.WriteValue(Int64Member);
            Serializer.WriteValue(Uint64Member);
            Serializer.WriteValue(DoubleMember);
            Serializer.WriteValue(BoolMember);
            Serializer.WriteValue(StringMember);
            Serializer.WriteValue(OptionalMember);
            Serializer.WriteValue(ArrayMember);
            Serializer.WriteValue(UintMapMember);
            Serializer.WriteValue(StringMapMember);
        }
        Serializer.EndWriteArray();
    }

    void Deserialize(SignalRDeserializer& Deserializer) override
    {
        size_t ArraySize = 0;
        Deserializer.StartReadArray(ArraySize);
        {
            Deserializer.ReadValue(Int64Member);
            Deserializer.ReadValue(Uint64Member);
            Deserializer.ReadValue(DoubleMember);
            Deserializer.ReadValue(BoolMember);
            Deserializer.ReadValue(StringMember);
            Deserializer.ReadValue(OptionalMember);
            Deserializer.ReadValue(ArrayMember);
            Deserializer.ReadValue(UintMapMember);
            Deserializer.ReadValue(StringMapMember);
        }
        Deserializer.EndReadArray();
    }

    bool operator==(const TestObject2& Other) const
    {
        return (Int64Member == Other.Int64Member && Uint64Member == Other.Uint64Member && DoubleMember == Other.DoubleMember
            && BoolMember == Other.BoolMember && StringMember == Other.StringMember && OptionalMember == Other.OptionalMember
            && ArrayMember == Other.ArrayMember && UintMapMember == Other.UintMapMember && StringMapMember == Other.StringMapMember);
    }
};

// Todo: split these into more focused tests.
CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeObjectTest)
{
    TestObject1 Child1;
    Child1.Values.push_back(std::string { "Test1" });
    Child1.Values.push_back(std::string { "Test2" });

    TestObject1 Child2;
    Child2.Values.push_back(std::string { "Test3" });
    Child2.Values.push_back(std::string { "Test4" });

    TestObject1 Child3;
    Child3.Values.push_back(std::string { "Test5" });
    Child3.Values.push_back(std::string { "Test6" });

    TestObject1 Child4;
    Child4.Values.push_back(std::string { "Test7" });
    Child4.Values.push_back(std::string { "Test8" });

    TestObject1 Child23;

    TestObject2 Value;
    Value.Int64Member = 1ll;
    Value.Uint64Member = 2ull;
    Value.DoubleMember = 3.0;
    Value.BoolMember = true;
    Value.StringMember = "Test";
    Value.OptionalMember = Child1;
    Value.ArrayMember.push_back(Child2);
    Value.UintMapMember[0] = Child3;
    Value.StringMapMember["0"] = Child4;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    TestObject2 DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}