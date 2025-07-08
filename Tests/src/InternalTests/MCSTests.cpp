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

#include "Awaitable.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "PublicAPITests/SpaceSystemTestHelpers.h"
#include "PublicAPITests/UserSystemTestHelpers.h"
#include <CSP/Multiplayer/Components/ImageSpaceComponent.h>
#include <CSP/Multiplayer/Components/LightSpaceComponent.h>

#include <gtest/gtest.h>

using namespace csp::multiplayer;

// bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

// Test constructor values of ObjectMessage are correct.
CSP_INTERNAL_TEST(CSPEngine, MCSTests, ObjectMessageConstructorTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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

// CSP_PUBLIC_TEST(CSPEngine, MCSTests, ObjectDeleteComponentTestReenterSpace2222)
//{
//     SetRandSeed();
//
//     auto& SystemsManager = csp::systems::SystemsManager::Get();
//     auto* UserSystem = SystemsManager.GetUserSystem();
//     auto* SpaceSystem = SystemsManager.GetSpaceSystem();
//     auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
//
//     csp::common::String UserId;
//
//     // Log in
//     LogInAsNewTestUser(UserSystem, UserId);
//
//     // Create space
//     csp::systems::Space Space;
//     CreateDefaultTestSpace(SpaceSystem, Space);
//
//     // Enter space
//     auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
//
//     EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);
//
//     const csp::common::String ObjectName = "Object 1";
//     SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
//
//     auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);
//
//     bool PatchPending = true;
//     Object->SetPatchSentCallback([&PatchPending](bool /*ok*/) { PatchPending = false; });
//
//     auto* ComponentToKeep = (StaticModelSpaceComponent*)Object->AddComponent(ComponentType::StaticModel);
//     ComponentToKeep->SetComponentName("ComponentNameKeep");
//     auto KeepKey = ComponentToKeep->GetId();
//     auto* ComponentToDelete = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);
//     ComponentToDelete->SetComponentName("ComponentNameDelete");
//     auto DeleteKey = ComponentToDelete->GetId();
//     Object->QueueUpdate();
//
//     while (PatchPending)
//     {
//         EntitySystem->ProcessPendingEntityOperations();
//         std::this_thread::sleep_for(10ms);
//     }
//
//     PatchPending = true;
//
//     // Ensure values are set correctly
//     EXPECT_EQ(ComponentToKeep->GetComponentName(), "ComponentNameKeep");
//     EXPECT_EQ(ComponentToDelete->GetComponentName(), "ComponentNameDelete");
//
//     auto& Components = *Object->GetComponents();
//
//     EXPECT_EQ(Components.Size(), 2);
//     EXPECT_TRUE(Components.HasKey(KeepKey));
//     EXPECT_TRUE(Components.HasKey(DeleteKey));
//     const bool TestValue = true;
//     mcs::ItemComponentData ComponentValue { TestValue };
//
//     const auto ComponentKeys = Components.Keys();
//     for (size_t i = 0; i < ComponentKeys->Size(); ++i)
//     {
//          auto [Result1] = AWAIT_PRE(ComponentValue, GetComponentById, RequestPredicate, ComponentKeys->operator[](i));
//              //csp::multiplayer::mcs::ItemComponentData, csp::multiplayer::mcs::GetComponentById, RequestPredicate, ComponentKeys->operator[](i));
//          EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);
//         //auto [Result] = Awaitable(&GetComponentById, csp::multiplayer::mcs::ItemComponentData,
//         ComponentKeys->operator[](i)).Await(RequestPredicate);
//         //EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
//
//         //auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);
//         //EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
//
//          csp::multiplayer::mcs::ItemComponentData OutItemComponentData = Result1.GetAssetCollection();
//         //csp::multiplayer::mcs::ItemComponentData OutItemComponentData = Result.GetAssetCollection();
//     }
//
//     delete (ComponentKeys);
//
//     // Delete component
//     Object->RemoveComponent(ComponentToDelete->GetId());
//     Object->QueueUpdate();
//     while (PatchPending)
//     {
//         EntitySystem->ProcessPendingEntityOperations();
//         std::this_thread::sleep_for(10ms);
//     }
//     EXPECT_FALSE(PatchPending);
//
//     // Check deletion has happened
//     auto& RealComponents = *Object->GetComponents();
//
//     EXPECT_EQ(RealComponents.Size(), 1);
//     EXPECT_TRUE(RealComponents.HasKey(KeepKey));
//     EXPECT_FALSE(RealComponents.HasKey(DeleteKey));
//
//     // Exit space and enter again, making sure the entities have been created
//     auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
//
//     // Wait a few seconds for the CHS database to update
//     std::this_thread::sleep_for(std::chrono::seconds(8));
//
//     bool EntitiesCreated = false;
//
//     auto EntitiesReadyCallback = [&EntitiesCreated](bool Success)
//     {
//         EntitiesCreated = true;
//         EXPECT_TRUE(Success);
//     };
//
//     EntitySystem->SetInitialEntitiesRetrievedCallback(EntitiesReadyCallback);
//
//     auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
//     EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);
//
//     WaitForCallbackWithUpdate(EntitiesCreated, EntitySystem);
//     EXPECT_TRUE(EntitiesCreated);
//
//     // Retrieve components in space
//     SpaceEntity* FoundEntity = EntitySystem->FindSpaceObject(ObjectName);
//     EXPECT_TRUE(FoundEntity != nullptr);
//     auto& FoundComponents = *FoundEntity->GetComponents();
//
//     // Check the right component has been deleted
//     EXPECT_EQ(FoundComponents.Size(), 1);
//     EXPECT_TRUE(FoundComponents.HasKey(KeepKey));
//     EXPECT_FALSE(FoundComponents.HasKey(DeleteKey));
//     EXPECT_EQ(FoundEntity->GetComponent(0)->GetComponentName(), "ComponentNameKeep");
//
//     // Exit space
//     auto [ExitSpaceResult2] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
//
//     // Delete space
//     DeleteSpace(SpaceSystem, Space.Id);
//
//     // Log out
//     LogOut(UserSystem);
// }
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