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

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "MultiplayerTestRunnerProcess.h"
#include "RAIIMockLogger.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <future>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

// Extracted because it's a lot of fluff that isn't the point of the test.
// You don't actually need to properly enter a space to use the event bus, just entering the scope of a space.
// If a space doesn't already exist entering the scope is rejected, I wonder if this is how we want this to work, is there
// no concept of sending an event to clients outside of a space? We already initialize the connection outside of the space
// for a similar reason. Perhaps there should be a "Not in space" scope, sort of like how everyone joins the chat lobby
// when you open the server browser in multiplayer games, (i'm thinking Starcraft II, but the concept is in a lot of places).
csp::systems::Space CreateTestSpaceAndEnterScope(csp::systems::SpaceSystem* SpaceSystem, csp::multiplayer::MultiplayerConnection* Connection)
{
    // Create space
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };
    Connection->SetScopes(Space.Id, ErrorCallback);

    return Space;
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RegisterDeregister)
{
    using namespace csp::multiplayer;
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* NetworkEventBus = SystemsManager.GetEventBus();

    const csp::common::Array<NetworkEventRegistration> InitialRegisteredEvents = NetworkEventBus->AllRegistrations();
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId", "TestEventName" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    const csp::common::Array<NetworkEventRegistration> AddedRegistration = NetworkEventBus->AllRegistrations();

    EXPECT_TRUE(AddedRegistration.Size() == InitialRegisteredEvents.Size() + 1);
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName" }));

    NetworkEventBus->StopListenNetworkEvent(NetworkEventRegistration { "TestReceiverId", "TestEventName" });

    const csp::common::Array<NetworkEventRegistration> RemovedRegistration = NetworkEventBus->AllRegistrations();
    EXPECT_TRUE(RemovedRegistration.Size() == InitialRegisteredEvents.Size());
    EXPECT_FALSE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName" }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RegisterDeregisterMulti)
{
    using namespace csp::multiplayer;
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* NetworkEventBus = SystemsManager.GetEventBus();

    const csp::common::Array<NetworkEventRegistration> InitialRegisteredEvents = NetworkEventBus->AllRegistrations();
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId", "TestEventName" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId", "TestEventName1" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId", "TestEventName2" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId1", "TestEventName" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId1", "TestEventName1" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId1", "TestEventName2" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {});
    const csp::common::Array<NetworkEventRegistration> AddedRegistration = NetworkEventBus->AllRegistrations();

    EXPECT_TRUE(AddedRegistration.Size() == InitialRegisteredEvents.Size() + 6);
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName" }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName1" }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName2" }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName" }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName1" }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName2" }));

    NetworkEventBus->StopListenNetworkEvent(NetworkEventRegistration { "TestReceiverId", "TestEventName" });

    const csp::common::Array<NetworkEventRegistration> RemovedRegistration = NetworkEventBus->AllRegistrations();
    EXPECT_TRUE(RemovedRegistration.Size() == InitialRegisteredEvents.Size() + 5);
    EXPECT_FALSE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName" }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName1" }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName2" }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName" }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName1" }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName2" }));

    NetworkEventBus->StopListenAllNetworkEvents("TestReceiverId1");

    const csp::common::Array<NetworkEventRegistration> RemovedAllTestReceivedOneRegistrations = NetworkEventBus->AllRegistrations();
    EXPECT_TRUE(RemovedAllTestReceivedOneRegistrations.Size() == InitialRegisteredEvents.Size() + 2);
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName" }));
    EXPECT_TRUE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName1" }));
    EXPECT_TRUE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { "TestReceiverId", "TestEventName2" }));
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName" }));
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName1" }));
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { "TestReceiverId1", "TestEventName2" }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectNullEvent)
{
    using namespace csp::multiplayer;

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* NetworkEventBus = SystemsManager.GetEventBus();

    const csp::common::String Error = "Error: Expected non-null callback.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Error)).Times(1);

    EXPECT_FALSE(NetworkEventBus->ListenNetworkEvent(NetworkEventRegistration { "TestReceiverId", "TestEventName" }, nullptr));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectDuplicateRegistration)
{
    using namespace csp::multiplayer;

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* NetworkEventBus = SystemsManager.GetEventBus();

    const csp::common::String Success1 = "Registering network event. EventReceiverId: TestReceiverId, Event: TestEventName.";
    const csp::common::String Success2 = "Registering network event. EventReceiverId: TestReceiverId1, Event: TestEventName.";
    const csp::common::String Success3 = "Registering network event. EventReceiverId: TestReceiverId, Event: TestEventName1.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Success1)).Times(1);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Success2)).Times(1);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Success3)).Times(1);

    const csp::common::String Error
        = "Attempting to register a duplicate network event receiver with EventReceiverId: TestReceiverId1, Event: TestEventName. Registration "
          "denied.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Error)).Times(1);

    EXPECT_TRUE(NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId", "TestEventName" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {}));
    EXPECT_TRUE(NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId1", "TestEventName" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {}));
    EXPECT_TRUE(NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId", "TestEventName1" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {}));
    EXPECT_FALSE(NetworkEventBus->ListenNetworkEvent(
        NetworkEventRegistration { "TestReceiverId1", "TestEventName" }, [](const csp::multiplayer::NetworkEventData& NetworkEventData) {}));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectUnknownDeregistration)
{
    using namespace csp::multiplayer;

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* NetworkEventBus = SystemsManager.GetEventBus();

    const csp::common::String Error
        = "Could not find network event registration with EventReceiverId: TestReceiverId, Event: TestEventName. Deregistration denied.";
    const csp::common::String Error1
        = "Could not find any network event registration with EventReceiverId: TestReceiverId. No events were deregistered.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Error)).Times(1);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(Error1)).Times(1);

    EXPECT_FALSE(NetworkEventBus->StopListenNetworkEvent(NetworkEventRegistration { "TestReceiverId", "TestEventName" }));
    EXPECT_FALSE(NetworkEventBus->StopListenAllNetworkEvents("TestReceiverId"));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SingleEventSingleReciever)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* NetworkEventBus = SystemsManager.GetEventBus();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture = NetworkEventPromise.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> ValsToSend
        = { csp::common::ReplicatedValue { "TestVal" }, csp::common::ReplicatedValue { 1.0f } };

    NetworkEventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("TestReceiverId", "TestEvent"),
        [&NetworkEventPromise](const csp::multiplayer::NetworkEventData& NetworkEventData)
        { NetworkEventPromise.set_value(NetworkEventData.EventValues); });

    NetworkEventBus->SendNetworkEventToClient("TestEvent", ValsToSend, Connection->GetClientId(), ErrorCallback);

    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals = NetworkEventFuture.get();
    EXPECT_EQ(ReceivedVals.Size(), 2);
    EXPECT_EQ(ReceivedVals[0].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(ReceivedVals[1].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(ReceivedVals[0].GetString(), "TestVal");
    EXPECT_EQ(ReceivedVals[1].GetFloat(), 1.0f);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SingleEventMultiReciever)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* NetworkEventBus = SystemsManager.GetEventBus();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture = NetworkEventPromise.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> ValsToSend
        = { csp::common::ReplicatedValue { "TestVal" }, csp::common::ReplicatedValue { 1.0f } };

    NetworkEventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("TestReceiverId", "TestEvent"),
        [&NetworkEventPromise](const csp::multiplayer::NetworkEventData& NetworkEventData)
        { NetworkEventPromise.set_value(NetworkEventData.EventValues); });

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise1;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture1 = NetworkEventPromise1.get_future();

    NetworkEventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("TestReceiverId1", "TestEvent"),
        [&NetworkEventPromise1](const csp::multiplayer::NetworkEventData& NetworkEventData)
        { NetworkEventPromise1.set_value(NetworkEventData.EventValues); });

    NetworkEventBus->SendNetworkEventToClient("TestEvent", ValsToSend, Connection->GetClientId(), ErrorCallback);

    // Both recievers should recieve this event
    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals = NetworkEventFuture.get();
    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals1 = NetworkEventFuture1.get();
    EXPECT_EQ(ReceivedVals.Size(), ReceivedVals1.Size());
    EXPECT_EQ(ReceivedVals[0].GetReplicatedValueType(), ReceivedVals1[0].GetReplicatedValueType());
    EXPECT_EQ(ReceivedVals[1].GetReplicatedValueType(), ReceivedVals1[1].GetReplicatedValueType());
    EXPECT_EQ(ReceivedVals[0].GetString(), ReceivedVals1[0].GetString());
    EXPECT_EQ(ReceivedVals[1].GetFloat(), ReceivedVals1[1].GetFloat());
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, MultiEventSingleReceiver)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* NetworkEventBus = SystemsManager.GetEventBus();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture = NetworkEventPromise.get_future();

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise1;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture1 = NetworkEventPromise1.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> ValsToSend
        = { csp::common::ReplicatedValue { "TestVal" }, csp::common::ReplicatedValue { 1.0f } };

    NetworkEventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("TestReceiverId", "TestEvent"),
        [&NetworkEventPromise](const csp::multiplayer::NetworkEventData& NetworkEventData)
        { NetworkEventPromise.set_value(NetworkEventData.EventValues); });
    NetworkEventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("TestReceiverId", "TestEvent1"),
        [&NetworkEventPromise1](const csp::multiplayer::NetworkEventData& NetworkEventData)
        { NetworkEventPromise1.set_value(NetworkEventData.EventValues); });

    NetworkEventBus->SendNetworkEventToClient("TestEvent", ValsToSend, Connection->GetClientId(), ErrorCallback);

    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals = NetworkEventFuture.get();
    EXPECT_EQ(ReceivedVals.Size(), 2);
    EXPECT_EQ(ReceivedVals[0].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(ReceivedVals[1].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(ReceivedVals[0].GetString(), "TestVal");
    EXPECT_EQ(ReceivedVals[1].GetFloat(), 1.0f);

    // The other event should not have been recieved as it has not been fired
    EXPECT_TRUE(NetworkEventFuture1.wait_for(std::chrono::milliseconds { 0 }) != std::future_status::ready);

    NetworkEventBus->SendNetworkEventToClient("TestEvent1", ValsToSend, Connection->GetClientId(), ErrorCallback);
    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals1 = NetworkEventFuture1.get();
    EXPECT_EQ(ReceivedVals1.Size(), 2);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, TestNoConnectionRegistration)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    RAIIMockLogger MockLogger {};

    const csp::common::String NoConnectionError = "Error : Multiplayer connection is unavailable, NetworkEventBus cannot start listening to events.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(NoConnectionError)).Times(1);

    // Bit of a cheat. The internal logic (at point of writing test), is more interested in whether Connection->SignalRConnection() is null, however
    // we inject the Connection object so we just use the check against that as a proxy for whether the error is emitted. Good enough!
    csp::multiplayer::NetworkEventBus NoConnectionEventBus { nullptr, *SystemsManager.GetLogSystem() };
    EXPECT_FALSE(NoConnectionEventBus.StartEventMessageListening());
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, TestMulticastEventToAllClients)
{
    // Spin up 2 other clients
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* NetworkEventBus = SystemsManager.GetEventBus();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create users
    auto TestRunnerUser1 = CreateTestUser();
    auto TestRunnerUser2 = CreateTestUser();

    csp::systems::Space TestSpace = CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    MultiplayerTestRunnerProcess EventBusPingRunner1
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::EVENT_BUS_PING)
              .SetSpaceId(TestSpace.Id.c_str())
              .SetLoginEmail(TestRunnerUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(30);

    MultiplayerTestRunnerProcess EventBusPingRunner2
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::EVENT_BUS_PING)
              .SetSpaceId(TestSpace.Id.c_str())
              .SetLoginEmail(TestRunnerUser2.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(30);

    std::future<void> RunnerListening1 = EventBusPingRunner1.ReadyForAssertionsFuture();
    std::future<void> RunnerListening2 = EventBusPingRunner2.ReadyForAssertionsFuture();

    EventBusPingRunner1.StartProcess();
    EventBusPingRunner2.StartProcess();

    // Wait for the runners to both be listening
    RunnerListening1.wait();
    RunnerListening2.wait();

    // Register interest in the ping response
    std::promise<void> TwoPingsResponsePromise;
    std::future<void> TwoPingsResponseFuture = TwoPingsResponsePromise.get_future();
    std::atomic<int> ReceivedPings = 0;

    NetworkEventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("TestReceiver", "EventPingResponse"),
        [&ReceivedPings, &TwoPingsResponsePromise](const csp::multiplayer::NetworkEventData& NetworkEventData)
        {
            std::cout << "Received Event Bus Ping." << std::endl;
            // fetch_add returns the old value for thready reasons. (std::latch would be better here given c++20)
            if (ReceivedPings.fetch_add(1) + 1 == 2)
            {
                TwoPingsResponsePromise.set_value();
            }
        });

    // Send the ping event to all clients
    NetworkEventBus->SendNetworkEvent("EventPingRequest", {}, [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); });

    // Expect to have had two responses
    auto Status = TwoPingsResponseFuture.wait_for(30s);
    EXPECT_EQ(Status, std::future_status::ready) << "TwoPingsResponsePromise was never set";
}
