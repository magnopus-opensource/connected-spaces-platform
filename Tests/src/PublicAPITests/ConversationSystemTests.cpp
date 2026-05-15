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
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "SpaceSystemTestHelpers.h"
#include "Systems/Conversation/ConversationSystemHelpers.h"
#include "Systems/Conversation/ConversationSystemInternal.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

namespace
{

void OnConnect();
void OnDisconnect(bool ok);
void OnDelete();

std::atomic_bool IsTestComplete;
std::atomic_bool IsDisconnected;
std::atomic_bool IsReadyForUpdate;
SpaceEntity* TestUser;
SpaceEntity* TestObject;

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit = 20000;
const int NumberOfEntityUpdateTicks = 5;
int ReceivedEntityUpdatesCount;

bool EventReceived = false;

csp::common::ReplicatedValue ObjectFloatProperty;
csp::common::ReplicatedValue ObjectBoolProperty;
csp::common::ReplicatedValue ObjectIntProperty;
csp::common::ReplicatedValue ObjectStringProperty;

csp::common::String ConversationId;

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
} // namespace

// Tests that events are correctly sent to the correct component from the conversation system
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationSystemEventTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    // Create 2 objects with a conversation component each
    csp::multiplayer::SpaceEntity* object1 = CreateTestObject(realtimeEngine.get(), "Object1");
    auto* conversationComponent1 = (ConversationSpaceComponent*)object1->AddComponent(ComponentType::Conversation);
    conversationComponent1->SetConversationId("TestId1");

    csp::multiplayer::SpaceEntity* object2 = CreateTestObject(realtimeEngine.get(), "Object2");
    auto* conversationComponent2 = (ConversationSpaceComponent*)object2->AddComponent(ComponentType::Conversation);
    conversationComponent2->SetConversationId("TestId2");

    object1->QueueUpdate();
    object2->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Test that when we send an event with the first components id, that only the first component receives the event.
    {
        bool callbackCalled1 = false;
        bool callbackCalled2 = false;

        auto callback1 = [&callbackCalled1](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled1 = true; };
        auto callback2 = [&callbackCalled2](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled2 = true; };

        conversationComponent1->SetConversationUpdateCallback(callback1);
        conversationComponent2->SetConversationUpdateCallback(callback2);

        csp::common::ConversationNetworkEventData params;
        params.MessageType = csp::multiplayer::ConversationEventType::NewConversation;
        params.MessageInfo.ConversationId = conversationComponent1->GetConversationId();

        bool eventSent = false;

        systemsManager.GetEventBus()->SendNetworkEventToClient(NetworkEventBus::StringFromNetworkEvent(NetworkEventBus::NetworkEvent::Conversation),
            csp::systems::ConversationSystemHelpers::MessageInfoToReplicatedValueArray(params.MessageType, params.MessageInfo),
            connection->GetClientId(), [&eventSent](csp::multiplayer::ErrorCode) { eventSent = true; });

        WaitForCallback(eventSent);
        WaitForCallback(callbackCalled1);
        // Callback2 shouldn't be called, as the event is for Callback1.
        // Just in case something is wrong, give a small wait time for the event to come through.
        WaitForCallback(callbackCalled2, 1);

        // Ensure the event was sent successfully
        EXPECT_TRUE(eventSent);
        // The event was for ConversationComponent1, so this should be called
        EXPECT_TRUE(callbackCalled1);
        // The event wasn't for ConversationComponent2, so this shouldn't be called
        EXPECT_FALSE(callbackCalled2);
    }

    // Do the same test, but ensure it works correctly when the event is sent to ConversationComponent2
    {
        bool callbackCalled1 = false;
        bool callbackCalled2 = false;

        const auto callback1 = [&callbackCalled1](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled1 = true; };
        const auto callback2 = [&callbackCalled2](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled2 = true; };

        conversationComponent1->SetConversationUpdateCallback(callback1);
        conversationComponent2->SetConversationUpdateCallback(callback2);

        csp::common::ConversationNetworkEventData params;
        params.MessageType = csp::multiplayer::ConversationEventType::NewMessage;
        params.MessageInfo.ConversationId = conversationComponent2->GetConversationId();

        bool eventSent = false;

        systemsManager.GetEventBus()->SendNetworkEventToClient(NetworkEventBus::StringFromNetworkEvent(NetworkEventBus::NetworkEvent::Conversation),
            csp::systems::ConversationSystemHelpers::MessageInfoToReplicatedValueArray(params.MessageType, params.MessageInfo),
            connection->GetClientId(), [&eventSent](csp::multiplayer::ErrorCode) { eventSent = true; });

        WaitForCallback(eventSent);
        WaitForCallback(callbackCalled1, 1);
        // Callback1 shouldn't be called, as the event is for Callback2.
        // Just in case something is wrong, give a small wait time for the event to come through.
        WaitForCallback(callbackCalled2);

        // Ensure the event was sent successfully
        EXPECT_TRUE(eventSent);
        // The event was for ConversationComponent2, so this should be called
        EXPECT_TRUE(callbackCalled2);
        // The event wasn't for ConversationComponent1, so this shouldn't be called
        EXPECT_FALSE(callbackCalled1);
    }

    // Cleanup
    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

// Tests that events are correctly stored, and then sent when the corresponding component exists.
// This can happen if a multiplayer event reaches another client before they have processed their component creation patch.
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationSystemEventDelayTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* connection = systemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    static constexpr const char* testConversationId = "New Test Message";

    // Send an event to a yet to exist conversation component
    csp::common::ConversationNetworkEventData params;
    params.MessageType = csp::multiplayer::ConversationEventType::NewMessage;
    params.MessageInfo.ConversationId = testConversationId;

    bool eventSent = false;

    systemsManager.GetEventBus()->SendNetworkEventToClient(NetworkEventBus::StringFromNetworkEvent(NetworkEventBus::NetworkEvent::Conversation),
        csp::systems::ConversationSystemHelpers::MessageInfoToReplicatedValueArray(params.MessageType, params.MessageInfo), connection->GetClientId(),
        [&eventSent](csp::multiplayer::ErrorCode) { eventSent = true; });

    WaitForCallback(eventSent);

    // Sleep a bit longer to ensure we receive the event
    std::this_thread::sleep_for(std::chrono::milliseconds { 2 });

    // Create object to represent the conversation
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());
    auto* conversationComponent = (ConversationSpaceComponent*)object->AddComponent(ComponentType::Conversation);

    // Ensure the conversation id is set so the event system can find the component
    conversationComponent->SetConversationId(testConversationId);

    object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Test that the conversation component receives the event
    {
        bool callbackCalled = false;

        const auto callback = [&callbackCalled](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled = true; };

        conversationComponent->SetConversationUpdateCallback(callback);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);
    }

    // Ensure the event is removed from the buffer by checking it isnt fired again when flushed
    {
        bool callbackCalled = false;

        const auto callback = [&callbackCalled](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled = true; };

        conversationComponent->SetConversationUpdateCallback(callback);

        // No need to wait, as the buffer will be flushed immediately upon seting the callback again
        EXPECT_FALSE(callbackCalled);
    }

    // Cleanup
    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}