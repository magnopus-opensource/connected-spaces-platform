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
csp::systems::Space CreateTestSpaceAndEnterScope(csp::systems::SpaceSystem* spaceSystem, csp::multiplayer::MultiplayerConnection* connection)
{
    // Create space
    const char* testSpaceName = "CSP_UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP_UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::systems::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

    auto errorCallback = [](ErrorCode error) { ASSERT_EQ(error, ErrorCode::None); };
    bool callbackCalled = false;
    connection->SetScopes(space.Id).then(async::inline_scheduler(),
        [&callbackCalled](std::tuple<signalr::value, std::exception_ptr> resultPair)
        {
            EXPECT_TRUE(std::get<1>(resultPair) == nullptr);
            callbackCalled = true;
        });
    WaitForCallback(callbackCalled);

    return space;
}

} // namespace

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RegisterDeregister)
{
    using namespace csp::multiplayer;
    auto& systemsManager = csp::systems::SystemsManager::Get();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const csp::common::Array<NetworkEventRegistration> initialRegisteredEvents = systemsManager.GetEventBus()->AllRegistrations();
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId, eventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    const csp::common::Array<NetworkEventRegistration> addedRegistration = systemsManager.GetEventBus()->AllRegistrations();

    EXPECT_TRUE(addedRegistration.Size() == initialRegisteredEvents.Size() + 1);
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName }));

    systemsManager.GetEventBus()->StopListenNetworkEvent(NetworkEventRegistration { receiverId, eventName });

    const csp::common::Array<NetworkEventRegistration> removedRegistration = systemsManager.GetEventBus()->AllRegistrations();
    EXPECT_TRUE(removedRegistration.Size() == initialRegisteredEvents.Size());
    EXPECT_FALSE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RegisterDeregisterMulti)
{
    using namespace csp::multiplayer;
    auto& systemsManager = csp::systems::SystemsManager::Get();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const char* receiverId2 = "TestReceiverId2";
    const char* eventName2 = "TestEventName2";

    const char* eventName3 = "TestEventName3";

    const csp::common::Array<NetworkEventRegistration> initialRegisteredEvents = systemsManager.GetEventBus()->AllRegistrations();
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId, eventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId, eventName2 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId, eventName3 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId2, eventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId2, eventName2 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId2, eventName3 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    const csp::common::Array<NetworkEventRegistration> addedRegistration = systemsManager.GetEventBus()->AllRegistrations();

    EXPECT_TRUE(addedRegistration.Size() == initialRegisteredEvents.Size() + 6);
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName }));
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName2 }));
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName3 }));
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId2, eventName }));
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId2, eventName2 }));
    EXPECT_TRUE(addedRegistration.ToList().Contains(NetworkEventRegistration { receiverId2, eventName3 }));

    systemsManager.GetEventBus()->StopListenNetworkEvent(NetworkEventRegistration { receiverId, eventName });

    const csp::common::Array<NetworkEventRegistration> removedRegistration = systemsManager.GetEventBus()->AllRegistrations();
    EXPECT_TRUE(removedRegistration.Size() == initialRegisteredEvents.Size() + 5);
    EXPECT_FALSE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName }));
    EXPECT_TRUE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName2 }));
    EXPECT_TRUE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId, eventName3 }));
    EXPECT_TRUE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId2, eventName }));
    EXPECT_TRUE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId2, eventName2 }));
    EXPECT_TRUE(removedRegistration.ToList().Contains(NetworkEventRegistration { receiverId2, eventName3 }));

    systemsManager.GetEventBus()->StopListenAllNetworkEvents(receiverId2);

    const csp::common::Array<NetworkEventRegistration> removedAllTestReceivedOneRegistrations = systemsManager.GetEventBus()->AllRegistrations();
    EXPECT_TRUE(removedAllTestReceivedOneRegistrations.Size() == initialRegisteredEvents.Size() + 2);
    EXPECT_FALSE(removedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { receiverId, eventName }));
    EXPECT_TRUE(removedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { receiverId, eventName2 }));
    EXPECT_TRUE(removedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { receiverId, eventName3 }));
    EXPECT_FALSE(removedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { receiverId2, eventName }));
    EXPECT_FALSE(removedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { receiverId2, eventName2 }));
    EXPECT_FALSE(removedAllTestReceivedOneRegistrations.ToList().Contains(NetworkEventRegistration { receiverId2, eventName3 }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectNullEvent)
{
    using namespace csp::multiplayer;

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();

    const csp::common::String error = "Error: Expected non-null callback.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    systemsManager.GetEventBus()->ListenNetworkEvent(NetworkEventRegistration { receiverId, eventName }, nullptr);
    auto allRegistrations = systemsManager.GetEventBus()->AllRegistrations();
    EXPECT_FALSE(std::any_of(allRegistrations.begin(), allRegistrations.end(),
        [receiverId](const NetworkEventRegistration& registration) { return registration.EventReceiverId == receiverId; }));
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectDuplicateRegistration)
{
    using namespace csp::multiplayer;

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const char* receiverId2 = "TestReceiverId2";
    const char* eventName2 = "TestEventName2";

    const csp::common::String success1 = fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", receiverId, eventName).c_str();
    const csp::common::String success2 = fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", receiverId2, eventName).c_str();
    const csp::common::String success3 = fmt::format("Registering network event. EventReceiverId: {}, Event: {}.", receiverId, eventName2).c_str();
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, success1)).Times(1);
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, success2)).Times(1);
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, success3)).Times(1);

    const csp::common::String error
        = fmt::format("Attempting to register a duplicate network event receiver with EventReceiverId: {}, Event: {}. Registration "
                      "denied.",
            receiverId2, eventName)
              .c_str();
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, error)).Times(1);

    const auto startSize = systemsManager.GetEventBus()->AllRegistrations().Size();

    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId, eventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(systemsManager.GetEventBus()->AllRegistrations().Size(), startSize + 1);
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId2, eventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(systemsManager.GetEventBus()->AllRegistrations().Size(), startSize + 2);
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId, eventName2 }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(systemsManager.GetEventBus()->AllRegistrations().Size(), startSize + 3);

    // This one should be rejected
    systemsManager.GetEventBus()->ListenNetworkEvent(
        NetworkEventRegistration { receiverId2, eventName }, [](const csp::common::NetworkEventData& /*NetworkEventData*/) { });
    EXPECT_EQ(systemsManager.GetEventBus()->AllRegistrations().Size(), startSize + 3);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, RejectUnknownDeregistration)
{
    using namespace csp::multiplayer;

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const csp::common::String error
        = fmt::format("Could not find network event registration with EventReceiverId: {}, Event: {}. Deregistration denied.", receiverId, eventName)
              .c_str();
    const csp::common::String error1
        = fmt::format("Could not find any network event registration with EventReceiverId: {}. No events were deregistered.", receiverId).c_str();
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, error)).Times(1);
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, error1)).Times(1);

    systemsManager.GetEventBus()->StopListenNetworkEvent(NetworkEventRegistration { receiverId, eventName });
    systemsManager.GetEventBus()->StopListenAllNetworkEvents(receiverId);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SingleEventSingleReciever)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const char* testValValue = "TestVal";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(systemsManager.GetSpaceSystem(), connection);

    auto errorCallback = [](ErrorCode error) { ASSERT_EQ(error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> networkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> networkEventFuture = networkEventPromise.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> valsToSend
        = { csp::common::ReplicatedValue { testValValue }, csp::common::ReplicatedValue { 1.0f } };

    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId, eventName),
        [&networkEventPromise](const csp::common::NetworkEventData& networkEventData)
        { networkEventPromise.set_value(networkEventData.EventValues); });

    systemsManager.GetEventBus()->SendNetworkEventToClient(eventName, valsToSend, connection->GetClientId(), errorCallback);

    const csp::common::Array<csp::common::ReplicatedValue> receivedVals = networkEventFuture.get();
    EXPECT_EQ(receivedVals.Size(), 2);
    EXPECT_EQ(receivedVals[0].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(receivedVals[1].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(receivedVals[0].GetString(), testValValue);
    EXPECT_EQ(receivedVals[1].GetFloat(), 1.0f);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, SingleEventMultiReciever)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const char* receiverId2 = "TestReceiverId2";

    const char* testValValue = "TestVal";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(systemsManager.GetSpaceSystem(), connection);

    auto errorCallback = [](ErrorCode error) { ASSERT_EQ(error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> networkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> networkEventFuture = networkEventPromise.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> valsToSend
        = { csp::common::ReplicatedValue { testValValue }, csp::common::ReplicatedValue { 1.0f } };

    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId, eventName),
        [&networkEventPromise](const csp::common::NetworkEventData& networkEventData)
        { networkEventPromise.set_value(networkEventData.EventValues); });

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> networkEventPromise1;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> networkEventFuture1 = networkEventPromise1.get_future();

    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId2, eventName),
        [&networkEventPromise1](const csp::common::NetworkEventData& networkEventData)
        { networkEventPromise1.set_value(networkEventData.EventValues); });

    systemsManager.GetEventBus()->SendNetworkEventToClient(eventName, valsToSend, connection->GetClientId(), errorCallback);

    // Both recievers should recieve this event
    const csp::common::Array<csp::common::ReplicatedValue> receivedVals = networkEventFuture.get();
    const csp::common::Array<csp::common::ReplicatedValue> receivedVals1 = networkEventFuture1.get();
    EXPECT_EQ(receivedVals.Size(), receivedVals1.Size());
    EXPECT_EQ(receivedVals[0].GetReplicatedValueType(), receivedVals1[0].GetReplicatedValueType());
    EXPECT_EQ(receivedVals[1].GetReplicatedValueType(), receivedVals1[1].GetReplicatedValueType());
    EXPECT_EQ(receivedVals[0].GetString(), receivedVals1[0].GetString());
    EXPECT_EQ(receivedVals[1].GetFloat(), receivedVals1[1].GetFloat());
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, MultiEventSingleReceiver)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    const char* receiverId = "TestReceiverId";
    const char* eventName = "TestEventName";

    const char* eventName2 = "TestEventName2";

    const char* testValValue = "TestVal";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(systemsManager.GetSpaceSystem(), connection);

    auto errorCallback = [](ErrorCode error) { ASSERT_EQ(error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> networkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> networkEventFuture = networkEventPromise.get_future();

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> networkEventPromise1;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> networkEventFuture1 = networkEventPromise1.get_future();

    const csp::common::Array<csp::common::ReplicatedValue> valsToSend
        = { csp::common::ReplicatedValue { testValValue }, csp::common::ReplicatedValue { 1.0f } };

    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId, eventName),
        [&networkEventPromise](const csp::common::NetworkEventData& networkEventData)
        { networkEventPromise.set_value(networkEventData.EventValues); });
    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId, eventName2),
        [&networkEventPromise1](const csp::common::NetworkEventData& networkEventData)
        { networkEventPromise1.set_value(networkEventData.EventValues); });

    systemsManager.GetEventBus()->SendNetworkEventToClient(eventName, valsToSend, connection->GetClientId(), errorCallback);

    const csp::common::Array<csp::common::ReplicatedValue> receivedVals = networkEventFuture.get();
    EXPECT_EQ(receivedVals.Size(), 2);
    EXPECT_EQ(receivedVals[0].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(receivedVals[1].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(receivedVals[0].GetString(), testValValue);
    EXPECT_EQ(receivedVals[1].GetFloat(), 1.0f);

    // The other event should not have been recieved as it has not been fired
    EXPECT_TRUE(networkEventFuture1.wait_for(std::chrono::milliseconds { 0 }) != std::future_status::ready);

    systemsManager.GetEventBus()->SendNetworkEventToClient(eventName2, valsToSend, connection->GetClientId(), errorCallback);
    const csp::common::Array<csp::common::ReplicatedValue> receivedVals1 = networkEventFuture1.get();
    EXPECT_EQ(receivedVals1.Size(), 2);
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, TestNoConnectionRegistration)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    RAIIMockLogger mockLogger {};

    const csp::common::String noConnectionError = "Error : Multiplayer connection is unavailable, NetworkEventBus cannot start listening to events.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, noConnectionError)).Times(1);

    // Bit of a cheat. The internal logic (at point of writing test), is more interested in whether Connection->SignalRConnection() is null, however
    // we inject the Connection object so we just use the check against that as a proxy for whether the error is emitted. Good enough!
    csp::multiplayer::NetworkEventBus noConnectionEventBus { nullptr, *systemsManager.GetLogSystem() };
    EXPECT_FALSE(noConnectionEventBus.StartEventMessageListening());
}

CSP_PUBLIC_TEST(DISABLED_CSPEngine, EventBusTests, TestMulticastEventToAllClients)
{
    // Spin up 2 other clients
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create users
    auto testRunnerUser1 = CreateTestUser();
    auto testRunnerUser2 = CreateTestUser();

    csp::systems::Space testSpace = CreateTestSpaceAndEnterScope(spaceSystem, connection);

    MultiplayerTestRunnerProcess eventBusPingRunner1
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::EVENT_BUS_PING)
              .SetSpaceId(testSpace.Id.c_str())
              .SetLoginEmail(testRunnerUser1.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(30);

    MultiplayerTestRunnerProcess eventBusPingRunner2
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::EVENT_BUS_PING)
              .SetSpaceId(testSpace.Id.c_str())
              .SetLoginEmail(testRunnerUser2.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(30);

    std::future<void> runnerListening1 = eventBusPingRunner1.ReadyForAssertionsFuture();
    std::future<void> runnerListening2 = eventBusPingRunner2.ReadyForAssertionsFuture();

    eventBusPingRunner1.StartProcess();
    eventBusPingRunner2.StartProcess();

    // Wait for the runners to both be listening
    runnerListening1.wait();
    runnerListening2.wait();

    // Register interest in the ping response
    std::promise<void> twoPingsResponsePromise;
    std::future<void> twoPingsResponseFuture = twoPingsResponsePromise.get_future();
    std::atomic<int> receivedPings = 0;

    const char* receiverId = "TestReceiverId";
    const char* pintRequestEventName = "EventPingRequest";
    const char* pingResponseEventName = "EventPingResponse";

    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId, pingResponseEventName),
        [&receivedPings, &twoPingsResponsePromise](const csp::common::NetworkEventData& /*NetworkEventData*/)
        {
            std::cout << "Received Event Bus Ping." << std::endl;
            // fetch_add returns the old value for thready reasons. (std::latch would be better here given c++20)
            if (receivedPings.fetch_add(1) + 1 == 2)
            {
                twoPingsResponsePromise.set_value();
            }
        });

    // Send the ping event to all clients
    systemsManager.GetEventBus()->SendNetworkEvent(pintRequestEventName, {}, [](ErrorCode error) { ASSERT_EQ(error, ErrorCode::None); });

    // Expect to have had two responses
    auto status = twoPingsResponseFuture.wait_for(30s);
    EXPECT_EQ(status, std::future_status::ready) << "TwoPingsResponsePromise was never set";
}

CSP_PUBLIC_TEST(CSPEngine, EventBusTests, EventDispatchReplicatedValueException)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    RAIIMockLogger mockLogger {};
    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(mockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    const csp::common::String error = "NetworkEventBus: Failed to deserialize event 'AsyncCallCompleted'. Registered events will not be fired.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, error)).Times(1);

    const char* receiverId = "TestReceiverId";
    const char* eventName = "AsyncCallCompleted";

    int64_t testValue = 5;

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    CreateTestSpaceAndEnterScope(systemsManager.GetSpaceSystem(), connection);

    auto errorCallback = [](ErrorCode error) { ASSERT_EQ(error, ErrorCode::None); };

    std::promise<csp::common::Array<csp::common::ReplicatedValue>> networkEventPromise;
    std::future<csp::common::Array<csp::common::ReplicatedValue>> networkEventFuture = networkEventPromise.get_future();

    // The AsyncCallCompleted event is expected to be sent with the following payload:
    // - csp::common::String OperationName
    // - csp::common::Map<csp::common::String, csp::common::String> References
    // - bool Success
    // - csp::common::String StatusReason
    // The following data is invalid and will result in a ReplicatedValueException being thrown.
    // Note: There is currently no way to observe the error with the current api and so we are having to check for the error log instead.
    const csp::common::Array<csp::common::ReplicatedValue> eventData = { csp::common::ReplicatedValue { testValue } };

    systemsManager.GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(receiverId, eventName),
        [&networkEventPromise](const csp::common::NetworkEventData& networkEventData)
        {
            networkEventPromise.set_value(networkEventData.EventValues);
        });

    systemsManager.GetEventBus()->SendNetworkEventToClient(eventName, eventData, connection->GetClientId(), errorCallback);

    EXPECT_NE(networkEventFuture.wait_for(5s), std::future_status::ready) << "Network Event will not be sent due to ReplicatedValueException being thrown for invalid ReplicatedValue type.";
}
