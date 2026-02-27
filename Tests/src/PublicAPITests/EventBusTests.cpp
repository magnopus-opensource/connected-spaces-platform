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
#include <fmt/format.h>
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
    const char* TestSpaceName = "CSP_UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP_UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, Space);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };
    bool CallbackCalled = false;
    Connection->SetScopes(Space.Id).then(async::inline_scheduler(),
        [&CallbackCalled](std::tuple<signalr::value, std::exception_ptr> ResultPair)
        {
            EXPECT_TRUE(std::get<1>(ResultPair) == nullptr);
            CallbackCalled = true;
        });
    WaitForCallback(CallbackCalled);

    return Space;
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RegisterDeregister)
{
    using namespace csp::multiplayer;
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const csp::common::Array<NetworkEventRegistration> InitialRegisteredEvents = SystemsManager.GetEventBus()->AllRegistrations();
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId, EventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    const csp::common::Array<NetworkEventRegistration> AddedRegistration = SystemsManager.GetEventBus()->AllRegistrations();

    EXPECT_TRUE(AddedRegistration.Size() == InitialRegisteredEvents.Size() + 1);
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName }));

    SystemsManager.GetEventBus()->StopListenNetworkEvent(NetworkEventRegistration { ReceiverId, EventName });

    const csp::common::Array<NetworkEventRegistration> RemovedRegistration = SystemsManager.GetEventBus()->AllRegistrations();
    EXPECT_TRUE(RemovedRegistration.Size() == InitialRegisteredEvents.Size());
    EXPECT_FALSE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RegisterDeregisterMulti)
{
    using namespace csp::multiplayer;
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const char* ReceiverId2 = "TestReceiverId2";
    const char* EventName2 = "TestEventName2";

    const char* EventName3 = "TestEventName3";

    const csp::common::Array<NetworkEventRegistration> InitialRegisteredEvents = SystemsManager.GetEventBus()->AllRegistrations();
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId, EventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId, EventName2 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId, EventName3 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId2, EventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId2, EventName2 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId2, EventName3 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    const csp::common::Array<NetworkEventRegistration> AddedRegistration = SystemsManager.GetEventBus()->AllRegistrations();

    EXPECT_TRUE(AddedRegistration.Size() == InitialRegisteredEvents.Size() + 6);
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName2 }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName3 }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName2 }));
    EXPECT_TRUE(AddedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName3 }));

    SystemsManager.GetEventBus()->StopListenNetworkEvent(NetworkEventRegistration { ReceiverId, EventName });

    const csp::common::Array<NetworkEventRegistration> RemovedRegistration = SystemsManager.GetEventBus()->AllRegistrations();
    EXPECT_TRUE(RemovedRegistration.Size() == InitialRegisteredEvents.Size() + 5);
    EXPECT_FALSE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName2 }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName3 }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName2 }));
    EXPECT_TRUE(RemovedRegistration.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName3 }));

    SystemsManager.GetEventBus()->StopListenAllNetworkEvents(ReceiverId2);

    const csp::common::Array<NetworkEventRegistration> RemovedAllTestReceivedOneRegistrations = SystemsManager.GetEventBus()->AllRegistrations();
    EXPECT_TRUE(RemovedAllTestReceivedOneRegistrations.Size() == InitialRegisteredEvents.Size() + 2);
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName }));
    EXPECT_TRUE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName2 }));
    EXPECT_TRUE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { ReceiverId, EventName3 }));
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName }));
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName2 }));
    EXPECT_FALSE(RemovedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { ReceiverId2, EventName3 }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectNullEvent)
{
    using namespace csp::multiplayer;

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();

    const csp::common::String Error = "Error: Expected non-null callback.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Error)).Times(1);

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    SystemsManager.GetEventBus()->ListenNetworkEvent(NetworkEventRegistration { ReceiverId, EventName }, nullptr);
    auto AllRegistrations = SystemsManager.GetEventBus()->AllRegistrations();
    EXPECT_FALSE(std::any_of(AllRegistrations.begin(), AllRegistrations.end(),
        [ReceiverId](const NetworkEventRegistration& Registration) { return Registration.EventReceiverId == ReceiverId; }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectDuplicateRegistration)
{
    using namespace csp::multiplayer;

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const char* ReceiverId2 = "TestReceiverId2";
    const char* EventName2 = "TestEventName2";

    const csp::common::String Success1 = fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", ReceiverId, EventName).c_str();
    const csp::common::String Success2 = fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", ReceiverId2, EventName).c_str();
    const csp::common::String Success3 = fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", ReceiverId, EventName2).c_str();
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, Success1)).Times(1);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, Success2)).Times(1);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, Success3)).Times(1);

    const csp::common::String Error
        = fmt::format("Attempting to register a duplicate network event receiver with EventReceiverId: {}, Event: {}. Registration "
                      "denied.",
            ReceiverId2, EventName)
              .c_str();
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, Error)).Times(1);

    const auto StartSize = SystemsManager.GetEventBus()->AllRegistrations().Size();

    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId, EventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(SystemsManager.GetEventBus()->AllRegistrations().Size(), StartSize + 1);
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId2, EventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(SystemsManager.GetEventBus()->AllRegistrations().Size(), StartSize + 2);
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId, EventName2 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(SystemsManager.GetEventBus()->AllRegistrations().Size(), StartSize + 3);

    // This one should be rejected
    SystemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { ReceiverId2, EventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(SystemsManager.GetEventBus()->AllRegistrations().Size(), StartSize + 3);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectUnknownDeregistration)
{
    using namespace csp::multiplayer;

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const csp::common::String Error
        = fmt::format("Could not find network event registration with EventReceiverId: {}, Event: {}. Deregistration denied.", ReceiverId, EventName)
              .c_str();
    const csp::common::String Error1
        = fmt::format("Could not find any network event registration with EventReceiverId: {}. No events were deregistered.", ReceiverId).c_str();
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, Error)).Times(1);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, Error1)).Times(1);

    SystemsManager.GetEventBus()->StopListenNetworkEvent(NetworkEventRegistration { ReceiverId, EventName });
    SystemsManager.GetEventBus()->StopListenAllNetworkEvents(ReceiverId);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SingleEventSingleReciever)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const char* TestValValue = "TestVal";

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture = NetworkEventPromise.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> ValsToSend
        = { csp::common::ReplicatedValue { TestValValue }, csp::common::ReplicatedValue { 1.0f } };

    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId, EventName),
        [&NetworkEventPromise](const csp::common::NetworkEventData& NetworkEventData)
        { NetworkEventPromise.set_value(NetworkEventData.EventValues); });

    SystemsManager.GetEventBus()->SendNetworkEventToClient(EventName, ValsToSend, Connection->GetClientId(), ErrorCallback);

    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals = NetworkEventFuture.get();
    EXPECT_EQ(ReceivedVals.Size(), 2);
    EXPECT_EQ(ReceivedVals[0].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(ReceivedVals[1].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(ReceivedVals[0].GetString(), TestValValue);
    EXPECT_EQ(ReceivedVals[1].GetFloat(), 1.0f);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SingleEventMultiReciever)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const char* ReceiverId2 = "TestReceiverId2";

    const char* TestValValue = "TestVal";

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture = NetworkEventPromise.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> ValsToSend
        = { csp::common::ReplicatedValue { TestValValue }, csp::common::ReplicatedValue { 1.0f } };

    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId, EventName),
        [&NetworkEventPromise](const csp::common::NetworkEventData& NetworkEventData)
        { NetworkEventPromise.set_value(NetworkEventData.EventValues); });

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise1;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture1 = NetworkEventPromise1.get_future();

    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId2, EventName),
        [&NetworkEventPromise1](const csp::common::NetworkEventData& NetworkEventData)
        { NetworkEventPromise1.set_value(NetworkEventData.EventValues); });

    SystemsManager.GetEventBus()->SendNetworkEventToClient(EventName, ValsToSend, Connection->GetClientId(), ErrorCallback);

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
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "TestEventName";

    const char* EventName2 = "TestEventName2";

    const char* TestValValue = "TestVal";

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
        = { csp::common::ReplicatedValue { TestValValue }, csp::common::ReplicatedValue { 1.0f } };

    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId, EventName),
        [&NetworkEventPromise](const csp::common::NetworkEventData& NetworkEventData)
        { NetworkEventPromise.set_value(NetworkEventData.EventValues); });
    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId, EventName2),
        [&NetworkEventPromise1](const csp::common::NetworkEventData& NetworkEventData)
        { NetworkEventPromise1.set_value(NetworkEventData.EventValues); });

    SystemsManager.GetEventBus()->SendNetworkEventToClient(EventName, ValsToSend, Connection->GetClientId(), ErrorCallback);

    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals = NetworkEventFuture.get();
    EXPECT_EQ(ReceivedVals.Size(), 2);
    EXPECT_EQ(ReceivedVals[0].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(ReceivedVals[1].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(ReceivedVals[0].GetString(), TestValValue);
    EXPECT_EQ(ReceivedVals[1].GetFloat(), 1.0f);

    // The other event should not have been recieved as it has not been fired
    EXPECT_TRUE(NetworkEventFuture1.wait_for(std::chrono::milliseconds { 0 }) != std::future_status::ready);

    SystemsManager.GetEventBus()->SendNetworkEventToClient(EventName2, ValsToSend, Connection->GetClientId(), ErrorCallback);
    const csp::common::Array<csp::common::ReplicatedValue> ReceivedVals1 = NetworkEventFuture1.get();
    EXPECT_EQ(ReceivedVals1.Size(), 2);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, TestNoConnectionRegistration)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    RAIIMockLogger MockLogger {};

    const csp::common::String NoConnectionError = "Error : Multiplayer connection is unavailable, NetworkEventBus cannot start listening to events.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, NoConnectionError)).Times(1);

    // Bit of a cheat. The internal logic (at point of writing test), is more interested in whether Connection->SignalRConnection() is null, however
    // we inject the Connection object so we just use the check against that as a proxy for whether the error is emitted. Good enough!
    csp::multiplayer::NetworkEventBus NoConnectionEventBus { nullptr, *SystemsManager.GetLogSystem() };
    EXPECT_FALSE(NoConnectionEventBus.StartEventMessageListening());
}

CSP_PUBLIC_TEST(DISABLED_CSPEngine, EventBusTests, TestMulticastEventToAllClients)
{
    // Spin up 2 other clients
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create users
    auto TestRunnerUser1 = CreateTestUser();
    auto TestRunnerUser2 = CreateTestUser();

    csp::systems::Space TestSpace = CreateTestSpaceAndEnterScope(SpaceSystem, Connection);

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

    const char* ReceiverId = "TestReceiverId";
    const char* PintRequestEventName = "EventPingRequest";
    const char* PingResponseEventName = "EventPingResponse";

    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId, PingResponseEventName),
        [&ReceivedPings, &TwoPingsResponsePromise](const csp::common::NetworkEventData& /*NetworkEventData*/)
        {
            std::cout << "Received Event Bus Ping." << std::endl;
            // fetch_add returns the old value for thready reasons. (std::latch would be better here given c++20)
            if (ReceivedPings.fetch_add(1) + 1 == 2)
            {
                TwoPingsResponsePromise.set_value();
            }
        });

    // Send the ping event to all clients
    SystemsManager.GetEventBus()->SendNetworkEvent(PintRequestEventName, {}, [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); });

    // Expect to have had two responses
    auto Status = TwoPingsResponseFuture.wait_for(30s);
    EXPECT_EQ(Status, std::future_status::ready) << "TwoPingsResponsePromise was never set";
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventDispatchReplicatedValueException)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    RAIIMockLogger MockLogger {};
    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    const csp::common::String Error = "NetworkEventBus: Failed to deserialize event 'AsyncCallCompleted'. Registered events will not be fired.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, Error)).Times(1);

    const char* ReceiverId = "TestReceiverId";
    const char* EventName = "AsyncCallCompleted";

    int64_t TestValue = 5;

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(SystemsManager.GetSpaceSystem(), Connection);

    auto ErrorCallback = [](ErrorCode Error) { ASSERT_EQ(Error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> NetworkEventFuture = NetworkEventPromise.get_future();

    // The AsyncCallCompleted event is expected to be sent with the following payload:
    // - csp::common::String OperationName
    // - csp::common::Map<csp::common::String, csp::common::String> References
    // - bool Success
    // - csp::common::String StatusReason
    // The following data is invalid and will result in a ReplicatedValueException being thrown.
    // Note: There is currently no way to observe the error with the current api and so we are having to check for the error log instead.
    const csp::common::Array<csp::common::ReplicatedValue> EventData = { csp::common::ReplicatedValue { TestValue } };

    SystemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ReceiverId, EventName),
        [&NetworkEventPromise](const csp::common::NetworkEventData& NetworkEventData)
        {
            NetworkEventPromise.set_value(NetworkEventData.EventValues);
        });

    SystemsManager.GetEventBus()->SendNetworkEventToClient(EventName, EventData, Connection->GetClientId(), ErrorCallback);

    EXPECT_NE(NetworkEventFuture.wait_for(5s), std::future_status::ready) << "Network Event will not be sent due to ReplicatedValueException being thrown for invalid ReplicatedValue type.";
}
