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
    const uint64_t TestId = 1;
    const uint64_t TestType = 2;
    const bool TestIsTransferable = false;
    const bool TestIsPersistant = true;
    const uint64_t TestOwnerId = 3;
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage Object { TestId, TestType, TestIsTransferable, TestIsPersistant, TestOwnerId, TestParentId, TestComponents };

    EXPECT_EQ(Object.GetId(), TestId);
    EXPECT_EQ(Object.GetType(), TestType);
    EXPECT_EQ(Object.GetIsTransferable(), TestIsTransferable);
    EXPECT_EQ(Object.GetIsPersistant(), TestIsPersistant);
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
    const bool TestIsPersistant = true;
    const uint64_t TestOwnerId = 3;
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage Object { TestId, TestType, TestIsTransferable, TestIsPersistant, TestOwnerId, TestParentId, TestComponents };

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

// Temporary tests to show that our new serializaer is creating the same output as the old one.
// This will be removed in the next PR when we remove the old serializer.

namespace signalr
{
inline bool operator==(const value& lhs, const value& rhs)
{
    if (lhs.type() != rhs.type())
        return false;

    switch (lhs.type())
    {
    case value_type::null:
        return true;

    case value_type::boolean:
        return lhs.as_bool() == rhs.as_bool();

    case value_type::integer:
        return lhs.as_integer() == rhs.as_integer();

    case value_type::uinteger:
        return lhs.as_uinteger() == rhs.as_uinteger();

    case value_type::float64:
        return lhs.as_double() == rhs.as_double();

    case value_type::string:
        return lhs.as_string() == rhs.as_string();

    case value_type::raw:
    {
        size_t len1, len2;
        const auto* data1 = lhs.as_raw(len1);
        const auto* data2 = rhs.as_raw(len2);
        return len1 == len2 && std::equal(data1, data1 + len1, data2);
    }

    case value_type::array:
    {
        const auto& a1 = lhs.as_array();
        const auto& a2 = rhs.as_array();
        return a1 == a2;
    }

    case value_type::string_map:
    {
        const auto& m1 = lhs.as_string_map();
        const auto& m2 = rhs.as_string_map();
        return m1 == m2; // std::map and signalr::value have operator==
    }

    case value_type::uint_map:
    {
        const auto& m1 = lhs.as_uint_map();
        const auto& m2 = rhs.as_uint_map();
        return m1 == m2;
    }

    default:
        return false; // unknown or unsupported type
    }
}
}

#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Common/Convert.h"
#include "Multiplayer/SignalRMsgPackEntitySerialiser.h"
#include "Multiplayer/SpaceEntityKeys.h"

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageSerializationComparisonTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // First create a space entity
    SpaceEntity* TestEntity = new SpaceEntity();
    TestEntity->Id = 1;
    TestEntity->Type = SpaceEntityType::Object;
    TestEntity->IsTransferable = true;
    TestEntity->IsPersistant = true;
    TestEntity->OwnerId = 2;
    TestEntity->ShouldUpdateParent = true;
    TestEntity->ParentId = 3;

    // Serialize the space entity using the old method
    SignalRMsgPackEntitySerialiser OldSerializer;
    TestEntity->Serialise(OldSerializer);

    // Create MCS object and serialize using the new method
    mcs::ObjectMessage MCSObject = TestEntity->CreateObjectMessage();

    SignalRSerializer NewSerializer;
    NewSerializer.WriteValue(MCSObject);

    // Compare the values
    signalr::value OldSerializedValue = OldSerializer.Finalise();
    signalr::value NewSerializedValue = NewSerializer.Get();

    EXPECT_EQ(OldSerializedValue, NewSerializedValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageSerializationComponentsComparisonTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // First create a space entity
    SpaceEntity* TestEntity = new SpaceEntity();
    TestEntity->Id = 1;
    TestEntity->Type = SpaceEntityType::Object;
    TestEntity->IsTransferable = true;
    TestEntity->IsPersistant = true;
    TestEntity->OwnerId = 2;
    TestEntity->ShouldUpdateParent = true;
    TestEntity->ParentId = 3;

    auto* TestComponent = static_cast<CustomSpaceComponent*>(TestEntity->AddComponent(ComponentType::Custom));
    TestComponent->SetComponentName("Name");

    int64_t Prop1 = 10ll;
    TestComponent->SetCustomProperty("Prop1", Prop1);

    csp::common::Map<csp::common::String, ReplicatedValue> Map1;
    Map1["Key1"] = "Test1";
    Map1["Key2"] = 5ll;
    Map1["Key3"] = 10.5f;

    TestComponent->SetCustomProperty("MyMap1", Map1);

    // Serialize the space entity using the old method
    SignalRMsgPackEntitySerialiser OldSerializer;
    TestEntity->Serialise(OldSerializer);

    // Create MCS object and serialize using the new method
    mcs::ObjectMessage MCSObject = TestEntity->CreateObjectMessage();

    SignalRSerializer NewSerializer;
    NewSerializer.WriteValue(MCSObject);

    // Compare the values
    signalr::value OldSerializedValue = OldSerializer.Finalise();
    signalr::value NewSerializedValue = NewSerializer.Get();

    EXPECT_EQ(OldSerializedValue, NewSerializedValue);
}

// Test that the optional parent works correctly
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageSerializationUnsetParentComparisonTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // First create a space entity
    SpaceEntity* TestEntity = new SpaceEntity();
    TestEntity->Id = 1;
    TestEntity->Type = SpaceEntityType::Object;
    TestEntity->IsTransferable = true;
    TestEntity->IsPersistant = true;
    TestEntity->OwnerId = 2;
    TestEntity->ParentId = nullptr;

    // Serialize the space entity using the old method
    SignalRMsgPackEntitySerialiser OldSerializer;
    TestEntity->Serialise(OldSerializer);

    // Create MCS object and serialize using the new method
    std::map<uint16_t, mcs::ItemComponentData> Components;

    // Write view components
    Components[COMPONENT_KEY_VIEW_ENTITYNAME] = { std::string { TestEntity->GetName().c_str() } };
    Components[COMPONENT_KEY_VIEW_POSITION]
        = { std::vector<float> { TestEntity->GetPosition().X, TestEntity->GetPosition().Y, TestEntity->GetPosition().Z } };
    Components[COMPONENT_KEY_VIEW_ROTATION] = { std::vector<float> {
        TestEntity->GetRotation().X, TestEntity->GetRotation().Y, TestEntity->GetRotation().Z, TestEntity->GetRotation().W } };
    Components[COMPONENT_KEY_VIEW_SCALE] = { std::vector<float> { TestEntity->GetScale().X, TestEntity->GetScale().Y, TestEntity->GetScale().Z } };
    Components[COMPONENT_KEY_VIEW_SELECTEDCLIENTID] = { static_cast<int64_t>(TestEntity->GetSelectingClientID()) };
    Components[COMPONENT_KEY_VIEW_THIRDPARTYREF] = { std::string { TestEntity->GetThirdPartyRef() } };
    Components[COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM] = { static_cast<int64_t>(TestEntity->GetThirdPartyPlatformType()) };
    Components[COMPONENT_KEY_VIEW_LOCKTYPE] = { static_cast<int64_t>(TestEntity->EntityLock) };

    mcs::ObjectMessage MCSObject { TestEntity->Id, static_cast<uint64_t>(TestEntity->Type), TestEntity->IsTransferable, TestEntity->IsPersistant,
        TestEntity->OwnerId, Convert(TestEntity->ParentId), Components };

    SignalRSerializer NewSerializer;
    NewSerializer.WriteValue(MCSObject);

    // Compare the values
    signalr::value OldSerializedValue = OldSerializer.Finalise();
    signalr::value NewSerializedValue = NewSerializer.Get();

    EXPECT_EQ(OldSerializedValue, NewSerializedValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchSerializationComparisonTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // First create a space entity
    SpaceEntity* TestEntity = new SpaceEntity();
    TestEntity->Id = 1;
    TestEntity->OwnerId = 2;
    TestEntity->ShouldUpdateParent = true;
    TestEntity->ParentId = 3;

    // Serialize the space entity as patch using the old method
    SignalRMsgPackEntitySerialiser OldSerializer;
    TestEntity->SerialisePatch(OldSerializer);

    mcs::ObjectPatch MCSObject { TestEntity->Id, TestEntity->OwnerId, false, TestEntity->ShouldUpdateParent, Convert(TestEntity->ParentId), {} };

    SignalRSerializer NewSerializer;
    NewSerializer.WriteValue(MCSObject);

    // Compare the values
    signalr::value OldSerializedValue = OldSerializer.Finalise();
    signalr::value NewSerializedValue = NewSerializer.Get();

    EXPECT_EQ(OldSerializedValue, NewSerializedValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchSerializationNoParentUpdateComparisonTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // First create a space entity
    SpaceEntity* TestEntity = new SpaceEntity();
    TestEntity->Id = 1;
    TestEntity->OwnerId = 2;

    // Serialize the space entity as patch using the old method
    SignalRMsgPackEntitySerialiser OldSerializer;
    TestEntity->SerialisePatch(OldSerializer);

    mcs::ObjectPatch MCSObject { TestEntity->Id, TestEntity->OwnerId, false, TestEntity->ShouldUpdateParent, Convert(TestEntity->ParentId), {} };

    SignalRSerializer NewSerializer;
    NewSerializer.WriteValue(MCSObject);

    // Compare the values
    signalr::value OldSerializedValue = OldSerializer.Finalise();
    signalr::value NewSerializedValue = NewSerializer.Get();

    EXPECT_EQ(OldSerializedValue, NewSerializedValue);
}

CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectPatchSerializationComponentsComparisonTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    // First create a space entity
    SpaceEntity* TestEntity = new SpaceEntity();
    TestEntity->Id = 1;
    TestEntity->OwnerId = 2;
    TestEntity->ParentId = 3;

    const csp::common::String TestName = "TestName";
    const csp::common::Vector3 TestPosition = csp::common::Vector3::One();
    const csp::common::Vector4 TestRotation = csp::common::Vector4 { 3.f, 3.f, 3.f, 3.f };
    const csp::common::Vector3 TestScale = csp::common::Vector3 { 2.f, 2.f, 2.f };
    const uint64_t TestSelectionState = 10;
    const csp::common::String TestThirdPartyRef = "TestRef";
    LockType TestLockType = LockType::UserAgnostic;

    TestEntity->SetName(TestName);
    TestEntity->SetPosition(TestPosition);
    TestEntity->SetRotation(TestRotation);
    TestEntity->SetScale(TestScale);
    TestEntity->InternalSetSelectionStateOfEntity(true, TestSelectionState);
    TestEntity->SetThirdPartyRef(TestThirdPartyRef);
    // TestEntity->SetThirdPartyPlatformType(csp::systems::EThirdPartyPlatform::UNREAL);
    TestEntity->Lock();

    // Serialize the space entity as patch using the old method
    SignalRMsgPackEntitySerialiser OldSerializer;
    TestEntity->SerialisePatch(OldSerializer);

    // Create MCS object and serialize using the new method
    std::map<uint16_t, mcs::ItemComponentData> Components;

    // Write view components
    Components[COMPONENT_KEY_VIEW_ENTITYNAME] = { std::string { TestName.c_str() } };
    Components[COMPONENT_KEY_VIEW_POSITION] = { std::vector<float> { TestPosition.X, TestPosition.Y, TestPosition.Z } };
    Components[COMPONENT_KEY_VIEW_ROTATION] = { std::vector<float> { TestRotation.X, TestRotation.Y, TestRotation.Z, TestRotation.W } };
    Components[COMPONENT_KEY_VIEW_SCALE] = { std::vector<float> { TestScale.X, TestScale.Y, TestScale.Z } };
    Components[COMPONENT_KEY_VIEW_SELECTEDCLIENTID] = { static_cast<int64_t>(TestSelectionState) };
    Components[COMPONENT_KEY_VIEW_THIRDPARTYREF] = { std::string { TestThirdPartyRef } };
    // Components[COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM] = { static_cast<int64_t>(TestEntity->GetThirdPartyPlatformType()) };
    Components[COMPONENT_KEY_VIEW_LOCKTYPE] = { static_cast<int64_t>(TestLockType) };

    mcs::ObjectPatch MCSObject { TestEntity->Id, TestEntity->OwnerId, false, TestEntity->ShouldUpdateParent, Convert(TestEntity->ParentId),
        Components };

    SignalRSerializer NewSerializer;
    NewSerializer.WriteValue(MCSObject);

    // Compare the values
    signalr::value OldSerializedValue = OldSerializer.Finalise();
    signalr::value NewSerializedValue = NewSerializer.Get();

    EXPECT_EQ(OldSerializedValue, NewSerializedValue);
}
