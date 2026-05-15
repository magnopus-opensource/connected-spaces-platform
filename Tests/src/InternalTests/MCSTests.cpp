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

#include "Multiplayer/MCS/MCSTypes.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace csp::multiplayer;

// Test constructor values of ObjectMessage are correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageConstructorTest)
{
    const uint64_t testId = 1;
    const uint64_t testType = 2;
    const bool testIsTransferable = false;
    const bool testIsPersistent = true;
    const uint64_t testOwnerId = 3;
    const std::optional<uint64_t> testParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> testComponents;
    testComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage object { testId, testType, testIsTransferable, testIsPersistent, testOwnerId, testParentId, testComponents };

    EXPECT_EQ(object.GetId(), testId);
    EXPECT_EQ(object.GetType(), testType);
    EXPECT_EQ(object.GetIsTransferable(), testIsTransferable);
    EXPECT_EQ(object.GetIsPersistent(), testIsPersistent);
    EXPECT_EQ(object.GetOwnerId(), testOwnerId);
    EXPECT_EQ(object.GetParentId(), testParentId);
    EXPECT_EQ(object.GetComponents(), testComponents);
}

// Test constructor values of ObjectPatch are correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchConstructorTest)
{
    const uint64_t testId = 1;
    const uint64_t testOwnerId = 2;
    const bool testDestroy = false;
    const bool testShouldUpdateParent = true;
    const std::optional<uint64_t> testParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> testComponents;
    testComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectPatch object { testId, testOwnerId, testDestroy, testShouldUpdateParent, testParentId, testComponents };

    EXPECT_EQ(object.GetId(), testId);
    EXPECT_EQ(object.GetOwnerId(), testOwnerId);
    EXPECT_EQ(object.GetDestroy(), testDestroy);
    EXPECT_EQ(object.GetShouldUpdateParent(), testShouldUpdateParent);
    EXPECT_EQ(object.GetParentId(), testParentId);
    EXPECT_EQ(object.GetComponents(), testComponents);
}

// Test serialization of ObjectMessage is correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageSerializeTest)
{
    const uint64_t testId = 1;
    const uint64_t testType = 2;
    const bool testIsTransferable = true;
    const bool testIsPersistent = true;
    const uint64_t testOwnerId = 3;
    const std::optional<uint64_t> testParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> testComponents;
    testComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage object { testId, testType, testIsTransferable, testIsPersistent, testOwnerId, testParentId, testComponents };

    SignalRSerializer serializer;
    serializer.WriteValue(object);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ObjectMessage deserializedObject { 0, 0, false, false, 0, 0, {} };
    deserializer.ReadValue(deserializedObject);

    EXPECT_EQ(deserializedObject, object);
}

// Test serialization of ObjectPatch is correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchSerializeTest)
{
    const uint64_t testId = 1;
    const uint64_t testOwnerId = 2;
    const bool testDestroy = false;
    const bool testShouldUpdateParent = false;
    const std::optional<uint64_t> testParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> testComponents;
    testComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectPatch object { testId, testOwnerId, testDestroy, testShouldUpdateParent, testParentId, testComponents };

    SignalRSerializer serializer;
    serializer.WriteValue(object);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ObjectPatch deserializedObject { 0, 0, false, false, 0, {} };
    deserializer.ReadValue(deserializedObject);

    EXPECT_EQ(deserializedObject, object);
}

/*

Now we want to test our ItemComponentData Serialization and Deserialization with ALL variant types that we support
This SHOULD be kept up-to-date if new types are added in the future!
*/

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeBoolTest)
{
    const bool testValue = true;
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeInt64Test)
{
    const int64_t testValue = 10;
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeUint64Test)
{
    const uint64_t testValue = 10;
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeDoubleTest)
{
    const double testValue = 10.1;
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeFloatTest)
{
    const float testValue = 10.1f;
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeStringTest)
{
    const std::string testValue = "Test";
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeFloatVectorTest)
{
    const std::vector<float> testValue = { 1.1f, 2.2f, 3.3f };
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeStringMapTest)
{
    const std::map<std::string, mcs::ItemComponentData> testValue
        = { { "Key1", mcs::ItemComponentData { 1.1f } }, { "Key2", mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ItemComponentDataSerializeUIntMapTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> testValue
        = { { 0, mcs::ItemComponentData { 1.1f } }, { 1, mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData componentValue { testValue };

    SignalRSerializer serializer;
    serializer.WriteValue(componentValue);

    signalr::value serializedValue = serializer.Get();

    SignalRDeserializer deserializer { serializedValue };

    mcs::ItemComponentData deserializedValue {};
    deserializer.ReadValue(deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}