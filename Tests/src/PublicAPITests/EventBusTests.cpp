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
#include "AssetSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/EventSerialisation.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "filesystem.hpp"
#include "signalrclient/signalr_value.h"

#include "gtest/gtest.h"
#include <CSP/Multiplayer/Components/ImageSpaceComponent.h>
#include <CSP/Multiplayer/Components/LightSpaceComponent.h>
#include <chrono>
#include <filesystem>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

void InitialiseTestingConnection();

std::atomic_bool IsTestComplete;
std::atomic_bool IsDisconnected;
std::atomic_bool IsReadyForUpdate;
SpaceEntity* TestUser;

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit = 20000;
const int NumberOfEntityUpdateTicks = 5;
int ReceivedEntityUpdatesCount;

bool EventSent = false;
bool EventReceived = false;

ReplicatedValue ObjectFloatProperty;
ReplicatedValue ObjectBoolProperty;
ReplicatedValue ObjectIntProperty;
ReplicatedValue ObjectStringProperty;

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void InitialiseTestingConnection()
{
    IsTestComplete = false;
    IsDisconnected = false;
    IsReadyForUpdate = false;
    TestUser = nullptr;

    WaitForTestTimeoutCountMs = 0;
    ReceivedEntityUpdatesCount = 0;

    EventSent = false;
    EventReceived = false;

    ObjectFloatProperty = ReplicatedValue(2.3f);
    ObjectBoolProperty = ReplicatedValue(true);
    ObjectIntProperty = ReplicatedValue(static_cast<int64_t>(42));
    ObjectStringProperty = "My replicated string";
}

typedef std::function<void(const csp::multiplayer::AssetDetailBlobParams&)> TestCallbackHandler;

class TestSystem : public csp::systems::SystemBase
{
public:
    // Test callback
    csp::multiplayer::EventBus::ParameterisedCallbackHandler TestCallback;

    TestSystem(csp::multiplayer::EventBus* InEventBus)
        : SystemBase(nullptr, InEventBus)
    {
        RegisterSystemCallback();
    }

    ~TestSystem() { DeregisterSystemCallback(); }

    void RegisterSystemCallback()
    {
        if (!EventBusPtr)
        {
            CSP_LOG_ERROR_MSG("Error: Failed to register TestSystem. EventBus must be instantiated in the MultiplayerConnection first.");
            return;
        }

        if (!TestCallback)
        {
            return;
        }

        if (EventBusPtr)
        {
            EventBusPtr->ListenNetworkEvent("TestEvent", this);
        }
    }

    void DeregisterSystemCallback()
    {
        if (EventBusPtr)
        {
            EventBusPtr->StopListenNetworkEvent("TestEvent");
        }
    }

    void OnEvent(const std::vector<signalr::value>& EventValues)
    {
        if (!TestCallback)
        {
            return;
        }

        csp::multiplayer::EventDeserialiser Deserialiser;
        Deserialiser.Parse(EventValues);

        TestCallback(true, Deserialiser.GetEventData());
    }

    void SetSystemCallback(csp::multiplayer::EventBus::ParameterisedCallbackHandler Callback)
    {
        TestCallback = Callback;
        RegisterSystemCallback();
    }
};

TestSystem* TestSystem1;
TestSystem* TestSystem2;

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_EVENTBUS_TESTS || RUN_EVENTBUS_EVENT_EMPTY_TEST
CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventEmptyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    EventBus->ListenNetworkEvent("TestEvent",
        [](bool ok, csp::common::Array<ReplicatedValue> Data)
        {
            EXPECT_TRUE(ok);

            std::cerr << "Test Event Received " << ok << std::endl;
        });

    EventBus->ListenNetworkEvent("TestEvent",
        [](bool ok, csp::common::Array<ReplicatedValue> Data)
        {
            EXPECT_TRUE(ok);

            EventReceived = true;

            if (EventSent)
            {
                IsTestComplete = true;
            }

            std::cerr << "Second Test Event Received " << ok << std::endl;
        });

    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(),
        [](ErrorCode Error)
        {
            ASSERT_EQ(Error, ErrorCode::None);

            EventSent = true;

            if (EventReceived)
            {
                IsTestComplete = true;
            }

            std::cerr << "Test Event Sent " << (Error == ErrorCode::None ? "true" : "false") << std::endl;
        });

    while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTBUS_TESTS || RUN_EVENTBUS_EVENT_MULTITYPE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventMultiTypeTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    EventBus->ListenNetworkEvent("MultiTypeEvent",
        [](bool ok, csp::common::Array<ReplicatedValue> Data)
        {
            EXPECT_TRUE(ok);

            std::cerr << "Multi Type Event Received " << ok << "  Payload: " << std::endl;

            for (int i = 0; i < Data.Size(); ++i)
            {
                if (Data[i].GetReplicatedValueType() == ReplicatedValueType::Boolean)
                {
                    printf("%s\n", Data[i].GetBool() ? "true" : "false");
                }
                else if (Data[i].GetReplicatedValueType() == ReplicatedValueType::Integer)
                {
                    printf("%lli\n", Data[i].GetInt());
                }
                else if (Data[i].GetReplicatedValueType() == ReplicatedValueType::Float)
                {
                    printf("%f\n", Data[i].GetFloat());
                }
            }

            EventReceived = true;

            if (EventSent)
            {
                IsTestComplete = true;
            }
        });

    ReplicatedValue EventInt((int64_t)-1);
    ReplicatedValue EventFloat(1234.567890f);

    EventBus->SendNetworkEventToClient("MultiTypeEvent", { EventInt, EventFloat }, Connection->GetClientId(),
        [EventInt, EventFloat](ErrorCode Error)
        {
            ASSERT_EQ(Error, ErrorCode::None);

            EventSent = true;

            if (EventReceived)
            {
                IsTestComplete = true;
            }

            printf("%lli, %f, \n", EventInt.GetInt(), EventFloat.GetFloat());
        });

    while (!IsTestComplete && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimit)
    {
        std::this_thread::sleep_for(50ms);
        WaitForTestTimeoutCountMs += 50;
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTBUS_TESTS || RUN_EVENTBUS_EVENT_CALLBACKS_SYSTEMS_TEST
CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventCallbacksSystemsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    TestSystem1 = CSP_NEW TestSystem(EventBus);
    TestSystem2 = CSP_NEW TestSystem(EventBus);

    auto& LogSystem = *SystemsManager.GetLogSystem();
    std::atomic_bool LogConfirmed = false;
    csp::common::String TestMsg;

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Set up Log callback
    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Set up Test callback
    bool TestCallback1Called = false, TestCallback2Called = false;
    int TestCallbackId = 0;

    csp::multiplayer::EventBus::ParameterisedCallbackHandler TestCallback1
        = [&TestCallback1Called, &TestCallbackId](bool ok, const csp::common::Array<csp::multiplayer::ReplicatedValue>& Params)
    {
        EXPECT_TRUE(ok);

        if (TestCallback1Called)
        {
            return;
        }

        TestCallback1Called = true;
        TestCallbackId = 1111;
    };

    csp::multiplayer::EventBus::ParameterisedCallbackHandler TestCallback2
        = [&TestCallback2Called, &TestCallbackId](bool ok, const csp::common::Array<csp::multiplayer::ReplicatedValue>& Params)
    {
        EXPECT_TRUE(ok);

        if (TestCallback2Called)
        {
            return;
        }

        TestCallback2Called = true;
        TestCallbackId = 2222;
    };

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    // Test that registering a system works
    TestSystem1->SetSystemCallback(TestCallback1);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback1Called);
    EXPECT_TRUE(TestCallback1Called);
    EXPECT_EQ(TestCallbackId, 1111);
    TestCallback1Called = false;

    // Test that registering a system when there already is a registered system does not work
    TestMsg = "Error: there is already a system registered for TestEvent. Deregister it first.";
    TestSystem2->SetSystemCallback(TestCallback2);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback1Called);
    EXPECT_TRUE(TestCallback1Called);
    EXPECT_EQ(TestCallbackId, 1111);
    TestCallback1Called = false;
    WaitForCallback(TestCallback2Called);
    EXPECT_FALSE(TestCallback2Called);
    EXPECT_TRUE(LogConfirmed);
    TestMsg = "";
    LogConfirmed = false;

    // Deregister the system and test that registering a new one now works
    EventBus->StopListenNetworkEvent("TestEvent");
    TestSystem2->SetSystemCallback(TestCallback2);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback2Called);
    EXPECT_TRUE(TestCallback2Called);
    EXPECT_EQ(TestCallbackId, 2222);
    TestCallback2Called = false;

    // Test that registering a callback when there already is a registered system does not work
    TestMsg = "Error: there is already a system registered for TestEvent. Deregister the system before registering a callback.";
    TestCallback1Called = false; // clean up
    EventBus->ListenNetworkEvent("TestEvent", TestCallback1);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback2Called);
    EXPECT_TRUE(TestCallback2Called);
    EXPECT_EQ(TestCallbackId, 2222);
    TestCallback2Called = false;
    WaitForCallback(TestCallback1Called);
    EXPECT_FALSE(TestCallback1Called);
    EXPECT_TRUE(LogConfirmed);
    TestMsg = "";
    LogConfirmed = false;

    // Test that registering a callback for a new event works
    EventBus->StopListenNetworkEvent("TestEvent");
    EventBus->ListenNetworkEvent("TestEvent", TestCallback1);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback1Called);
    EXPECT_TRUE(TestCallback1Called);
    EXPECT_EQ(TestCallbackId, 1111);
    TestCallback1Called = false;

    // Test that registering a system when there already is a registered callback does not work
    TestMsg = "Error: there is already a callback registered for TestEvent.";
    TestSystem1->SetSystemCallback(TestCallback2);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback1Called);
    EXPECT_TRUE(TestCallback1Called);
    EXPECT_EQ(TestCallbackId, 1111);
    TestCallback1Called = false;
    WaitForCallback(TestCallback2Called);
    EXPECT_FALSE(TestCallback2Called);
    EXPECT_TRUE(LogConfirmed);
    TestMsg = "";
    LogConfirmed = false;

    // Test that deregistering a system that is registered works
    EventBus->StopListenNetworkEvent("TestEvent"); // clean up
    EventBus->ListenNetworkEvent("TestEvent", TestSystem1);
    EventBus->StopListenNetworkEvent("TestEvent");
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    TestCallback1Called = false;
    WaitForCallback(TestCallback1Called);
    EXPECT_FALSE(TestCallback1Called);

    // Test that registering a callback when there already is a registered callback works
    EventBus->StopListenNetworkEvent("TestEvent"); // clean up
    TestMsg = "The callback set for TestEvent was overwritten with a new callback.";
    EventBus->ListenNetworkEvent("TestEvent", TestCallback1);
    EventBus->ListenNetworkEvent("TestEvent", TestCallback2);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback1Called);
    EXPECT_FALSE(TestCallback1Called);
    WaitForCallback(TestCallback2Called);
    EXPECT_TRUE(TestCallback2Called);
    EXPECT_EQ(TestCallbackId, 2222);
    TestCallback2Called = false;
    EXPECT_TRUE(LogConfirmed);
    TestMsg = "";
    LogConfirmed = false;

    // Test that deregistering an event that is not registered does nothing
    EventBus->StopListenNetworkEvent("NonExistingEvent");
    WaitForCallback(TestCallback2Called);
    EXPECT_FALSE(TestCallback2Called);

    // Test that deregistering a callback that is registered works
    EventBus->StopListenNetworkEvent("TestEvent"); // clean up
    EventBus->ListenNetworkEvent("TestEvent", TestCallback1);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    EventBus->StopListenNetworkEvent("TestEvent");
    WaitForCallback(TestCallback1Called);
    EXPECT_FALSE(TestCallback1Called);

    // Test that deregistering a system that is registered works
    TestSystem1->SetSystemCallback(TestCallback1);
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    EventBus->StopListenNetworkEvent("TestEvent");
    WaitForCallback(TestCallback1Called);
    EXPECT_FALSE(TestCallback1Called);

    LogSystem.ClearAllCallbacks();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTBUS_TESTS || RUN_EVENTBUS_SETCALLBACKBEFORECONNECTED_TEST
CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SetCallbackBeforeConnectedTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    TestSystem1 = CSP_NEW TestSystem(EventBus);

    auto& LogSystem = *SystemsManager.GetLogSystem();
    std::atomic_bool LogConfirmed = false;
    csp::common::String TestMsg;

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    // Set all the callbacks

    // Setup Connection callback
    bool ConnectionCallbackCalled = false;
    csp::common::String ConnectionMessage;

    auto ConnectionCallback = [&ConnectionCallbackCalled, &ConnectionMessage](csp::common::String Message)
    {
        if (ConnectionCallbackCalled)
        {
            return;
        }

        ConnectionMessage = Message;
        ConnectionCallbackCalled = true;
    };
    Connection->SetConnectionCallback(ConnectionCallback);

    EXPECT_EQ(Connection->GetConnectionState(), ConnectionState::Disconnected);

    // Set up Test callback
    bool TestCallback1Called = false;
    int TestCallbackId = 0;

    csp::multiplayer::EventBus::ParameterisedCallbackHandler TestCallback1
        = [&TestCallback1Called, &TestCallbackId](bool ok, const csp::common::Array<csp::multiplayer::ReplicatedValue>& Params)
    {
        EXPECT_TRUE(ok);

        if (TestCallback1Called)
        {
            return;
        }

        TestCallback1Called = true;
        TestCallbackId = 1111;
    };
    TestSystem1->SetSystemCallback(TestCallback1);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    csp::common::String UserId;

    // Log in -- i.e. establish the multiplayer connection
    LogInAsNewTestUser(UserSystem, UserId);

    // Check Connection callback was called
    WaitForCallback(ConnectionCallbackCalled);
    EXPECT_TRUE(ConnectionCallbackCalled);
    EXPECT_EQ(ConnectionMessage, "Success");
    EXPECT_EQ(Connection->GetConnectionState(), ConnectionState::Connected);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    InitialiseTestingConnection();

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Check system callback was called
    EventBus->SendNetworkEventToClient("TestEvent", {}, Connection->GetClientId(), ErrorCallback);
    WaitForCallback(TestCallback1Called);
    EXPECT_TRUE(TestCallback1Called);
    EXPECT_EQ(TestCallbackId, 1111);
    TestCallback1Called = false;

    // Clean up
    LogSystem.ClearAllCallbacks();

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif