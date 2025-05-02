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

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeVariantTest)
{
    const SignalRSerializableValue Value = std::string { "Test" };

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    SignalRSerializableValue DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeVariantArrayTest)
{
    const std::vector<SignalRSerializableValue> Value { { 1ll }, { 2ull }, { 3.0 }, { true }, { "Test" }, { nullptr } };

    SignalRSerializer Serializer;
    Serializer.StartArray();

    for (const auto& Element : Value)
    {
        Serializer.WriteValue(Element);
    }

    Serializer.EndArray();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::vector<SignalRSerializableValue> DeserializedValue;

    size_t ArraySize = 0;
    Deserializer.EnterArray(ArraySize);

    for (size_t i = 0; i < ArraySize; ++i)
    {
        SignalRSerializableValue DeserializedElement;
        Deserializer.ReadValue(DeserializedElement);
        DeserializedValue.push_back(DeserializedElement);
    }

    Deserializer.ExitArray();

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeArrayTest)
{
    const auto Value = std::make_tuple(1ll, 2ull, 3.0, true, "Test", nullptr);

    SignalRSerializer Serializer;
    Serializer.StartArray();

    Serializer.WriteValue(std::get<0>(Value));
    Serializer.WriteValue(std::get<1>(Value));
    Serializer.WriteValue(std::get<2>(Value));
    Serializer.WriteValue(std::get<3>(Value));
    Serializer.WriteValue(std::get<4>(Value));
    Serializer.WriteValue(std::get<5>(Value));

    Serializer.EndArray();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };
    std::tuple<int64_t, uint64_t, double, bool, std::string, nullptr_t> DeserializedValue;

    size_t ArraySize = 0;
    Deserializer.EnterArray(ArraySize);
    {
        Deserializer.ReadValue(std::get<0>(DeserializedValue));
        Deserializer.ReadValue(std::get<1>(DeserializedValue));
        Deserializer.ReadValue(std::get<2>(DeserializedValue));
        Deserializer.ReadValue(std::get<3>(DeserializedValue));
        Deserializer.ReadValue(std::get<4>(DeserializedValue));
        Deserializer.ReadValue(std::get<5>(DeserializedValue));
    }
    Deserializer.ExitArray();

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeUintMapTest)
{
    const std::map<uint64_t, SignalRSerializableValue> Value { { 0, { 1ll } }, { 1, { 2ull } }, { 2, { 3.0 } }, { 3, { true } }, { 4, { "Test" } },
        { 5, { nullptr } } };

    SignalRSerializer Serializer;
    Serializer.StartUintMap();

    for (const auto& Pair : Value)
    {
        Serializer.WriteKeyValue(Pair.first, Pair.second);
    }

    Serializer.EndUintMap();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::map<uint64_t, SignalRSerializableValue> DeserializedValue;

    size_t MapSize = 0;
    Deserializer.EnterUintMap(MapSize);

    for (size_t i = 0; i < MapSize; ++i)
    {
        std::pair<uint64_t, SignalRSerializableValue> DeserializedPair;
        Deserializer.ReadKeyValue(DeserializedPair);
        DeserializedValue[DeserializedPair.first] = DeserializedPair.second;
    }

    Deserializer.ExitUintMap();

    EXPECT_EQ(DeserializedValue, Value);
}

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeStringMapTest)
{
    const std::map<std::string, SignalRSerializableValue> Value { { "0", { 1ll } }, { "1", { 2ull } }, { "2", { 3.0 } }, { "3", { true } },
        { "4", { "Test" } }, { "5", { nullptr } } };

    SignalRSerializer Serializer;
    Serializer.StartStringMap();

    for (const auto& Pair : Value)
    {
        Serializer.WriteKeyValue(Pair.first, Pair.second);
    }

    Serializer.EndStringMap();

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    std::map<std::string, SignalRSerializableValue> DeserializedValue;

    size_t MapSize = 0;
    Deserializer.EnterStringMap(MapSize);

    for (size_t i = 0; i < MapSize; ++i)
    {
        std::pair<std::string, SignalRSerializableValue> DeserializedPair;
        Deserializer.ReadKeyValue(DeserializedPair);
        DeserializedValue[DeserializedPair.first] = DeserializedPair.second;
    }

    Deserializer.ExitStringMap();

    EXPECT_EQ(DeserializedValue, Value);
}

struct TestObject1 : public ISignalRSerializable, public ISignalRDeserializable
{
    std::vector<SignalRSerializableValue> Values2;

    void Serialize(SignalRSerializer& Serializer) const override
    {
        Serializer.StartArray();

        for (const auto& Element : Values2)
        {
            Serializer.WriteValue(Element);
        }

        Serializer.EndArray();
    }

    void Deserialize(SignalRDeserializer& Deserializer) override
    {
        size_t ArraySize = 0;
        Deserializer.EnterArray(ArraySize);

        for (size_t i = 0; i < ArraySize; ++i)
        {
            SignalRSerializableValue Element;
            Deserializer.ReadValue(Element);
            Values2.push_back(Element);
        }

        Deserializer.ExitArray();
    }

    // TODO: change when we implement deserializer
    bool operator==(const TestObject1& Other) const { return Values2 == Other.Values2; }
};

struct TestObject2 : public ISignalRSerializable, public ISignalRDeserializable
{
    int64_t Int64Member = 0;
    uint64_t Uint64Member = 0;
    double DoubleMember = 0;
    bool BoolMember = false;
    std::string StringMember;
    std::optional<bool> OptionalMember;
    std::map<uint64_t, TestObject1> UintMapMember;

    void Serialize(SignalRSerializer& Serializer) const override
    {
        Serializer.StartArray();
        {
            Serializer.WriteValue(Int64Member);
            Serializer.WriteValue(Uint64Member);
            Serializer.WriteValue(DoubleMember);
            Serializer.WriteValue(BoolMember);
            Serializer.WriteValue(StringMember);
            Serializer.WriteValue(OptionalMember);

            Serializer.StartUintMap();

            for (const auto& Pair : UintMapMember)
            {
                Serializer.WriteKeyValue(Pair.first, Pair.second);
            }

            Serializer.EndUintMap();
        }
        Serializer.EndArray();
    }

    void Deserialize(SignalRDeserializer& Deserializer) override
    {
        size_t ArraySize = 0;
        Deserializer.EnterArray(ArraySize);
        {
            Deserializer.ReadValue(Int64Member);
            Deserializer.ReadValue(Uint64Member);
            Deserializer.ReadValue(DoubleMember);
            Deserializer.ReadValue(BoolMember);
            Deserializer.ReadValue(StringMember);
            Deserializer.ReadValue(OptionalMember);

            size_t MapSize = 0;
            Deserializer.EnterUintMap(MapSize);

            for (size_t j = 0; j < MapSize; ++j)
            {
                std::pair<uint64_t, TestObject1> Pair;
                Deserializer.ReadKeyValue(Pair);
                UintMapMember[Pair.first] = Pair.second;
            }

            Deserializer.ExitUintMap();
        }
        Deserializer.ExitArray();
    }

    bool operator==(const TestObject2& Other) const
    {
        return (Int64Member == Other.Int64Member && Uint64Member == Other.Uint64Member && DoubleMember == Other.DoubleMember
            && BoolMember == Other.BoolMember && StringMember == Other.StringMember && OptionalMember == Other.OptionalMember
            && UintMapMember == Other.UintMapMember);
    }
};

CSP_INTERNAL_TEST(CSPEngine, SignalRSerializerTests, SerializeObjectTest)
{
    TestObject1 Child1;
    Child1.Values2.push_back(std::string { "Test1" });
    Child1.Values2.push_back(false);

    TestObject1 Child2;
    Child2.Values2.push_back(5ull);
    Child1.Values2.push_back(std::string { "Test2" });
    Child2.Values2.push_back(true);

    TestObject2 Value;
    Value.Int64Member = 1ll;
    Value.Uint64Member = 2ull;
    Value.DoubleMember = 3.0;
    Value.BoolMember = true;
    Value.StringMember = "Test";
    Value.OptionalMember = true;

    Value.UintMapMember[0] = Child1;
    Value.UintMapMember[1] = Child2;

    SignalRSerializer Serializer;
    Serializer.WriteValue(Value);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    TestObject2 DeserializedValue;
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, Value);
}

// TODO: exception tests - log type and depth