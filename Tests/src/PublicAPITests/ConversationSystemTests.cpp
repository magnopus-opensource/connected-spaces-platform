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
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
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
MultiplayerConnection* Connection;
SpaceEntitySystem* EntitySystem;
SpaceEntity* TestUser;
SpaceEntity* TestObject;

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

csp::common::String ConversationId;

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
} // namespace

// Tests that events are correctly sent to the correct component from the conversation system
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_EVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationSystemEventTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    // Create 2 objects with a conversation component each
    csp::multiplayer::SpaceEntity* Object1 = CreateTestObject(EntitySystem, "Object1");
    auto* ConversationComponent1 = (ConversationSpaceComponent*)Object1->AddComponent(ComponentType::Conversation);
    ConversationComponent1->SetConversationId("TestId1");

    csp::multiplayer::SpaceEntity* Object2 = CreateTestObject(EntitySystem, "Object2");
    auto* ConversationComponent2 = (ConversationSpaceComponent*)Object2->AddComponent(ComponentType::Conversation);
    ConversationComponent2->SetConversationId("TestId2");

    Object1->QueueUpdate();
    Object2->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Test that when we send an event with the first components id, that only the first component receives the event.
    {
        bool CallbackCalled1 = false;
        bool CallbackCalled2 = false;

        auto Callback1 = [&CallbackCalled1](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled1 = true; };
        auto Callback2 = [&CallbackCalled2](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled2 = true; };

        ConversationComponent1->SetConversationUpdateCallback(Callback1);
        ConversationComponent2->SetConversationUpdateCallback(Callback2);

        csp::multiplayer::ConversationEventParams Params;
        Params.MessageType = csp::multiplayer::ConversationEventType::NewMessage;
        Params.MessageInfo.ConversationId = ConversationComponent1->GetConversationId();

        bool EventSent = false;

        EventBus->SendNetworkEventToClient("Conversation", csp::systems::ConversationSystemHelpers::MessageInfoToReplicatedValueArray(Params),
            Connection->GetClientId(), [&EventSent](csp::multiplayer::ErrorCode) { EventSent = true; });

        WaitForCallback(EventSent);
        WaitForCallback(CallbackCalled1);
        // Callback2 shouldn't be called, as the event is for Callback1.
        // Just in case something is wrong, give a small wait time for the event to come through.
        WaitForCallback(CallbackCalled2, 1);

        // Ensure the event was sent successfully
        EXPECT_TRUE(EventSent);
        // The event was for ConversationComponent1, so this should be called
        EXPECT_TRUE(CallbackCalled1);
        // The event wasn't for ConversationComponent2, so this shouldn't be called
        EXPECT_FALSE(CallbackCalled2);
    }

    // Do the same test, but ensure it works correctly when the event is sent to ConversationComponent2
    {
        bool CallbackCalled1 = false;
        bool CallbackCalled2 = false;

        const auto Callback1 = [&CallbackCalled1](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled1 = true; };
        const auto Callback2 = [&CallbackCalled2](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled2 = true; };

        ConversationComponent1->SetConversationUpdateCallback(Callback1);
        ConversationComponent2->SetConversationUpdateCallback(Callback2);

        csp::multiplayer::ConversationEventParams Params;
        Params.MessageType = csp::multiplayer::ConversationEventType::NewMessage;
        Params.MessageInfo.ConversationId = ConversationComponent2->GetConversationId();

        bool EventSent = false;

        EventBus->SendNetworkEventToClient("Conversation", csp::systems::ConversationSystemHelpers::MessageInfoToReplicatedValueArray(Params),
            Connection->GetClientId(), [&EventSent](csp::multiplayer::ErrorCode) { EventSent = true; });

        WaitForCallback(EventSent);
        WaitForCallback(CallbackCalled1);
        // Callback1 shouldn't be called, as the event is for Callback2.
        // Just in case something is wrong, give a small wait time for the event to come through.
        WaitForCallback(CallbackCalled2, 1);

        // Ensure the event was sent successfully
        EXPECT_TRUE(EventSent);
        // The event was for ConversationComponent2, so this should be called
        EXPECT_TRUE(CallbackCalled2);
        // The event wasn't for ConversationComponent1, so this shouldn't be called
        EXPECT_FALSE(CallbackCalled1);
    }

    // Cleanup
    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

// Tests that events are correctly stored, and then sent when the corresponding component exists.
// This can happen if a multiplayer event reaches another client before they have processed their component creation patch.
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_EVENT_DELAY_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationSystemEventDelayTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    static constexpr const char* TestConversationId = "New Test Message";

    // Send an event to a yet to exist conversation component
    csp::multiplayer::ConversationEventParams Params;
    Params.MessageType = csp::multiplayer::ConversationEventType::NewMessage;
    Params.MessageInfo.ConversationId = TestConversationId;

    bool EventSent = false;

    EventBus->SendNetworkEventToClient("Conversation", csp::systems::ConversationSystemHelpers::MessageInfoToReplicatedValueArray(Params),
        Connection->GetClientId(), [&EventSent](csp::multiplayer::ErrorCode) { EventSent = true; });

    WaitForCallback(EventSent);

    // Sleep a bit longer to ensure we receive the event
    std::this_thread::sleep_for(std::chrono::milliseconds { 2 });

    // Create object to represent the conversation
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);
    auto* ConversationComponent = (ConversationSpaceComponent*)Object->AddComponent(ComponentType::Conversation);

    // Ensure the conversation id is set so the event system can find the component
    ConversationComponent->SetConversationId(TestConversationId);

    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Test that the conversation component receives the event
    {
        bool CallbackCalled = false;

        const auto Callback = [&CallbackCalled](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled = true; };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);
    }

    // Ensure the event is removed from the buffer by checking it isnt fired again when flushed
    {
        bool CallbackCalled = false;

        const auto Callback = [&CallbackCalled](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled = true; };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        // No need to wait, as the buffer will be flushed immediately upon seting the callback again
        EXPECT_FALSE(CallbackCalled);
    }

    // Cleanup
    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif