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

#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/MCSComponentPacker.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

#include <limits>

using namespace csp::multiplayer;

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ComponentPackerPreservesLargeComponentTypeId)
{
    constexpr auto LargeTypeId = std::numeric_limits<uint64_t>::max();

    auto LogSystem = csp::common::LogSystem {};
    auto ScriptSystem = csp::systems::ScriptSystem::MakeInitialised();
    auto Engine = csp::multiplayer::OfflineRealtimeEngine {
        LogSystem,
        *ScriptSystem,
        {
            csp::multiplayer::ComponentSchema {
                csp::multiplayer::ComponentSchema::TypeIdType { LargeTypeId },
                "Example",
                {
                    csp::multiplayer::ComponentProperty {
                        csp::multiplayer::ComponentProperty::KeyType { 0 },
                        "value",
                        "hello",
                    },
                },
            },
        },
    };

    auto [Entity] = AWAIT(&Engine, CreateEntity, "Test Entity", csp::multiplayer::SpaceTransform {}, csp::common::Optional<uint64_t> {});
    ASSERT_NE(Entity, nullptr);

    auto* Component = Entity->AddComponentByTypeId(LargeTypeId);
    ASSERT_NE(Component, nullptr);
    Component->SetSchemaProperty(0, "hello");

    auto Packer = MCSComponentPacker {};
    Packer.WriteValue(Component->GetId(), Component);

    const auto Expected = std::map<uint16_t, mcs::ItemComponentData> {
        {
            Component->GetId(),
            mcs::ItemComponentData {
                std::map<uint16_t, mcs::ItemComponentData> {
                    { COMPONENT_KEY_COMPONENTTYPE, mcs::ItemComponentData { LargeTypeId } },
                    { static_cast<uint16_t>(SpaceEntityComponentKey::Name), mcs::ItemComponentData { std::string {} } },
                    { uint16_t { 0 }, mcs::ItemComponentData { std::string { "hello" } } },
                },
            },
        },
    };

    EXPECT_EQ(Packer.GetComponents(), Expected);
}

// Test constructor values of ObjectMessage are correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageConstructorTest)
{
    const uint64_t TestId = 1;
    const uint64_t TestType = 2;
    const bool TestIsTransferable = false;
    const bool TestIsPersistent = true;
    const uint64_t TestOwnerId = 3;
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage Object { TestId, TestType, TestIsTransferable, TestIsPersistent, TestOwnerId, TestParentId, TestComponents };

    EXPECT_EQ(Object.GetId(), TestId);
    EXPECT_EQ(Object.GetType(), TestType);
    EXPECT_EQ(Object.GetIsTransferable(), TestIsTransferable);
    EXPECT_EQ(Object.GetIsPersistent(), TestIsPersistent);
    EXPECT_EQ(Object.GetOwnerId(), TestOwnerId);
    EXPECT_EQ(Object.GetParentId(), TestParentId);
    EXPECT_EQ(Object.GetComponents(), TestComponents);
}

// Test constructor values of ObjectPatch are correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchConstructorTest)
{
    const uint64_t TestId = 1;
    const uint64_t TestOwnerId = 2;
    const bool TestDestroy = false;
    const bool TestShouldUpdateParent = true;
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectPatch Object { TestId, TestOwnerId, TestDestroy, TestShouldUpdateParent, TestParentId, TestComponents };

    EXPECT_EQ(Object.GetId(), TestId);
    EXPECT_EQ(Object.GetOwnerId(), TestOwnerId);
    EXPECT_EQ(Object.GetDestroy(), TestDestroy);
    EXPECT_EQ(Object.GetShouldUpdateParent(), TestShouldUpdateParent);
    EXPECT_EQ(Object.GetParentId(), TestParentId);
    EXPECT_EQ(Object.GetComponents(), TestComponents);
}

// Test serialization of ObjectMessage is correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageSerializeTest)
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

    SignalRSerializer Serializer;
    Serializer.WriteValue(Object);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ObjectMessage DeserializedObject { 0, 0, false, false, 0, 0, {} };
    Deserializer.ReadValue(DeserializedObject);

    EXPECT_EQ(DeserializedObject, Object);
}

// Test serialization of ObjectPatch is correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchSerializeTest)
{
    const uint64_t TestId = 1;
    const uint64_t TestOwnerId = 2;
    const bool TestDestroy = false;
    const bool TestShouldUpdateParent = false;
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectPatch Object { TestId, TestOwnerId, TestDestroy, TestShouldUpdateParent, TestParentId, TestComponents };

    SignalRSerializer Serializer;
    Serializer.WriteValue(Object);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ObjectPatch DeserializedObject { 0, 0, false, false, 0, {} };
    Deserializer.ReadValue(DeserializedObject);

    EXPECT_EQ(DeserializedObject, Object);
}

/*

Now we want to test our ItemComponentData Serialization and Deserialization with ALL variant types that we support
This SHOULD be kept up-to-date if new types are added in the future!
*/

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeBoolTest)
{
    const bool TestValue = true;
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeInt64Test)
{
    const int64_t TestValue = 10;
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeUint64Test)
{
    const uint64_t TestValue = 10;
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeDoubleTest)
{
    const double TestValue = 10.1;
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeFloatTest)
{
    const float TestValue = 10.1f;
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeStringTest)
{
    const std::string TestValue = "Test";
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeFloatVectorTest)
{
    const std::vector<float> TestValue = { 1.1f, 2.2f, 3.3f };
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeStringMapTest)
{
    const std::map<std::string, mcs::ItemComponentData> TestValue
        = { { "Key1", mcs::ItemComponentData { 1.1f } }, { "Key2", mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeUIntMapTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> TestValue
        = { { 0, mcs::ItemComponentData { 1.1f } }, { 1, mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData ComponentValue { TestValue };

    SignalRSerializer Serializer;
    Serializer.WriteValue(ComponentValue);

    signalr::value SerializedValue = Serializer.Get();

    SignalRDeserializer Deserializer { SerializedValue };

    mcs::ItemComponentData DeserializedValue {};
    Deserializer.ReadValue(DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}