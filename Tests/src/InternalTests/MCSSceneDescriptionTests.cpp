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

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "Multiplayer/MCS/MCSTypes.h"
#include "Json/JsonSerializer.h"

using namespace csp::multiplayer;

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ObjectMessageSerializeTest)
{
    const uint64_t TestId = 1;
    const uint64_t TestType = 2;
    const bool TestIsTransferable = true;
    const bool TestIsPersistent = true;
    const uint64_t TestOwnerId = 3;
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage Object { TestId, TestType, TestIsTransferable, TestIsPersistent, TestOwnerId, TestParentId, TestComponents };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(Object);

    mcs::ObjectMessage DeserializedObject { 0, 0, false, false, 0, 0, {} };
    csp::json::JsonDeserializer::Deserialize(SerializedValue.c_str(), DeserializedObject);

    EXPECT_EQ(DeserializedObject, Object);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeBoolTest)
{
    const bool TestValue = true;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeInt64Test)
{
    const int64_t TestValue = -10;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeUInt64Test)
{
    const uint64_t TestValue = 10;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeDoubleTest)
{
    const double TestValue = 10.1;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeFloatTest)
{
    const float TestValue = 10.1f;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeStringTest)
{
    const std::string TestValue = "Test";
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeFloatVectorTest)
{
    const std::vector<float> TestValue = { 1.1f, 2.2f, 3.3f };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeStringMapTest)
{
    const std::map<std::string, mcs::ItemComponentData> TestValue
        = { { "Key1", mcs::ItemComponentData { 1.1f } }, { "Key2", mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSSceneDescriptionTests, ItemComponentDataSerializeUIntMapTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> TestValue
        = { { 0, mcs::ItemComponentData { 1.1f } }, { 1, mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}