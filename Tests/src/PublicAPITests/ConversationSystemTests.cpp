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

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_CREATE_CONVERSATION_ID
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, CreateConversationId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String DefaultTestUserId;

    // Log in
    LogInAsNewTestUser(UserSystem, DefaultTestUserId);

    const auto DefaultTestUserDisplayName = GetFullProfileByUserId(UserSystem, DefaultTestUserId).DisplayName;

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Create object to represent the conversation
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create conversation component
    auto ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

    auto [ConversationResult] = AWAIT(ConversationComponent, CreateConversation, "DefaultConversation");

    EXPECT_EQ(ConversationResult.GetResultCode(), csp::systems::EResultCode::Success);
    ConversationId = ConversationResult.GetValue();

    auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationInfo, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetConversationInfo().ConversationId, ConversationId);
    EXPECT_EQ(Result.GetConversationInfo().UserId, DefaultTestUserId);
    EXPECT_EQ(Result.GetConversationInfo().Message, "DefaultConversation");
    EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");

    const auto DefaultConversationMessage = "this is a message from the tests world";
    csp::multiplayer::MessageInfo CreatedMessageInfo;
    csp::multiplayer::MessageInfo RetrievedMessageInfo;
    csp::common::String FirstMessageId;

    // Add message to Conversation
    {
        auto [AddMessageResult] = AWAIT(ConversationComponent, AddMessage, DefaultConversationMessage);

        EXPECT_EQ(AddMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_EQ(ConversationId, AddMessageResult.GetMessageInfo().ConversationId);
    }

    // Get message From Conversation
    {
        auto [GetMessagesResult] = AWAIT(ConversationComponent, GetMessagesFromConversation, 0, 1);

        EXPECT_EQ(GetMessagesResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto& Messages = GetMessagesResult.GetMessages();
        EXPECT_EQ(Messages.Size(), 1);
        EXPECT_EQ(Messages[0].Message, DefaultConversationMessage);
    }

    {
        auto [DeleteConversationResult] = AWAIT(ConversationComponent, DeleteConversation);

        EXPECT_EQ(DeleteConversationResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_GET_MESSAGES_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, GetMessagesTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String DefaultTestUserId;

    // Create test user
    csp::systems::Profile SpaceCreatorUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, DefaultTestUserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);
    const auto UserDisplayName = GetFullProfileByUserId(UserSystem, DefaultTestUserId).DisplayName;

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // add the second test user to the space
    csp::systems::Profile AlternativeTestUser = CreateTestUser();
    auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeTestUser.Email, true, "", "");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, false);

    csp::multiplayer::MessageInfo CreatedMessageInfo;
    csp::multiplayer::MessageInfo RetrievedMessageInfo;

    csp::common::String ConversationId;
    csp::common::String FirstMessageId;
    csp::common::String SecondMessageId;
    const auto DefaultConversationMessage = "this is a message from the tests world";

    uint64_t ConversationObjectId;

    // Create object to represent the conversation
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    ConversationObjectId = CreatedObject->GetId();

    // Create conversation component
    auto ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);
    auto FirstConversationComponentId = ConversationComponent->GetId();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    auto [ConversationResult] = AWAIT_PRE(ConversationComponent, CreateConversation, RequestPredicate, "TestMessage");

    EXPECT_EQ(ConversationResult.GetResultCode(), csp::systems::EResultCode::Success);
    ConversationId = ConversationResult.GetValue();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationInfo, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetConversationInfo().UserId, DefaultTestUserId);
        EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
        EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
    }

    // Add message to Conversation
    {
        auto [AddMessageResult] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, DefaultConversationMessage);

        EXPECT_EQ(AddMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        CreatedMessageInfo = AddMessageResult.GetMessageInfo();

        EXPECT_EQ(ConversationId, CreatedMessageInfo.ConversationId);

        ConversationId = CreatedMessageInfo.ConversationId;
        FirstMessageId = CreatedMessageInfo.MessageId;
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    LogOut(UserSystem);

    // Log in with the second account
    csp::common::String SecondTestUserId;
    LogIn(UserSystem, SecondTestUserId, AlternativeTestUser.Email, GeneratedTestAccountPassword);

    bool EntitiesRetrieved = false;

    auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetInitialEntitiesRetrievedCallback(
        [&EntitiesRetrieved](bool Ok)
        {
            if (Ok)
            {
                EntitiesRetrieved = true;
            }
        });

    while (!EntitiesRetrieved)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    auto ConversationEntity = EntitySystem->FindSpaceEntityById(ConversationObjectId);

    ConversationSpaceComponent* RetrievedConversationComponent
        = (ConversationSpaceComponent*)ConversationEntity->GetComponent(FirstConversationComponentId);

    auto [AddMessageResult] = AWAIT_PRE(RetrievedConversationComponent, AddMessage, RequestPredicate, DefaultConversationMessage);

    EXPECT_EQ(AddMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

    SecondMessageId = AddMessageResult.GetMessageInfo().MessageId;

    // check that the second user can retrieve both added messages
    {
        auto [GetFirstMessageResult] = AWAIT_PRE(RetrievedConversationComponent, GetMessageInfo, RequestPredicate, FirstMessageId);

        EXPECT_EQ(GetFirstMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        RetrievedMessageInfo = GetFirstMessageResult.GetMessageInfo();
        EXPECT_EQ(RetrievedMessageInfo.MessageId, FirstMessageId);
        EXPECT_EQ(RetrievedMessageInfo.UserId, DefaultTestUserId);
        EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
        EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);

        auto [GetSecondMessageResult] = AWAIT_PRE(RetrievedConversationComponent, GetMessageInfo, RequestPredicate, SecondMessageId);

        EXPECT_EQ(GetSecondMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        RetrievedMessageInfo = GetSecondMessageResult.GetMessageInfo();
        EXPECT_EQ(RetrievedMessageInfo.MessageId, SecondMessageId);
        EXPECT_EQ(RetrievedMessageInfo.UserId, SecondTestUserId);
        EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
        EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);
    }

    // check that the second user can retrieve the messages from the conversation using pagination
    {
        auto [GetMessagesResult] = AWAIT_PRE(RetrievedConversationComponent, GetMessagesFromConversation, RequestPredicate, 0, 1);
        EXPECT_EQ(GetMessagesResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto Messages = GetMessagesResult.GetMessages();
        EXPECT_EQ(Messages.Size(), 1);
        EXPECT_EQ(GetMessagesResult.GetTotalCount(), 2);

        const auto Message = Messages[0];
        EXPECT_FALSE(Message.MessageId.IsEmpty());
        EXPECT_FALSE(Message.UserId.IsEmpty());
        EXPECT_EQ(Message.ConversationId, ConversationId);
        EXPECT_EQ(Message.Message, DefaultConversationMessage);
    }

    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    LogOut(UserSystem);

    // Log in again with the default user
    LogIn(UserSystem, DefaultTestUserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);

    auto [EnterResult3] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult3.GetResultCode(), csp::systems::EResultCode::Success);

    EntitiesRetrieved = false;

    EntitySystem->SetInitialEntitiesRetrievedCallback(
        [&EntitiesRetrieved](bool Ok)
        {
            if (Ok)
            {
                EntitiesRetrieved = true;
            }
        });

    while (!EntitiesRetrieved)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    ConversationEntity = EntitySystem->FindSpaceEntityById(ConversationObjectId);

    ConversationComponent = (ConversationSpaceComponent*)ConversationEntity->GetComponent(0);

    // check that the default user can retrieve both added messages
    {
        auto [GetFirstMessageResult] = AWAIT(ConversationComponent, GetMessageInfo, FirstMessageId);

        EXPECT_EQ(GetFirstMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        RetrievedMessageInfo = GetFirstMessageResult.GetMessageInfo();
        EXPECT_EQ(RetrievedMessageInfo.MessageId, FirstMessageId);
        EXPECT_EQ(RetrievedMessageInfo.UserId, DefaultTestUserId);
        EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
        EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);

        auto [GetSecondMessageResult] = AWAIT(ConversationComponent, GetMessageInfo, SecondMessageId);

        EXPECT_EQ(GetSecondMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        RetrievedMessageInfo = GetSecondMessageResult.GetMessageInfo();
        EXPECT_EQ(RetrievedMessageInfo.MessageId, SecondMessageId);
        EXPECT_EQ(RetrievedMessageInfo.UserId, SecondTestUserId);
        EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
        EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);
    }

    // check that the default user can retrieve the messages from the conversation using pagination
    {
        auto [GetMessagesResult] = AWAIT_PRE(ConversationComponent, GetMessagesFromConversation, RequestPredicate, 1, 1);
        EXPECT_EQ(GetMessagesResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto Messages = GetMessagesResult.GetMessages();
        EXPECT_EQ(Messages.Size(), 1);
        EXPECT_EQ(GetMessagesResult.GetTotalCount(), 2);

        const auto Message = Messages[0];
        EXPECT_FALSE(Message.MessageId.IsEmpty());
        EXPECT_FALSE(Message.UserId.IsEmpty());
        EXPECT_EQ(Message.ConversationId, ConversationId);
        EXPECT_EQ(Message.Message, DefaultConversationMessage);
    }

    {
        auto [DeleteConversationResult] = AWAIT(ConversationComponent, DeleteConversation);

        EXPECT_EQ(DeleteConversationResult.GetResultCode(), csp::systems::EResultCode::Success);
    }

    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_TWO_CONVERSATIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, TwoConversationsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EventBus = SystemsManager.GetEventBus();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Create test user
    csp::systems::Profile SpaceCreatorUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, UserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);
    const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    // create a second test user
    csp::systems::Profile AlternativeTestUser = CreateTestUser();

    // add the second test user to the space
    auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeTestUser.Email, true, "", "");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String FirstConversationId;
    csp::common::String SecondConversationId;

    csp::common::String FirstMessageIdToBeDeleted;
    csp::common::String SecondMessageIdToBeDeleted;

    const auto DefaultConversationMessage = "this is a message from the tests world";

    uint64_t ConversationObjectId;

    // Create object to represent the conversation
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    ConversationObjectId = CreatedObject->GetId();

    // Create conversation component
    auto ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    auto [ConversationResult] = AWAIT(ConversationComponent, CreateConversation, "Test Conversation 1 Message");

    EXPECT_EQ(ConversationResult.GetResultCode(), csp::systems::EResultCode::Success);

    FirstConversationId = ConversationResult.GetValue();

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationInfo, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
        EXPECT_EQ(Result.GetConversationInfo().Message, "Test Conversation 1 Message");
        EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
    }

    // Add message to Conversation 1
    {
        auto [AddMessageResult] = AWAIT(ConversationComponent, AddMessage, DefaultConversationMessage);

        EXPECT_EQ(AddMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_EQ(FirstConversationId, AddMessageResult.GetMessageInfo().ConversationId);
    }

    // Add message to Conversation 1
    {
        auto [AddMessageResult] = AWAIT(ConversationComponent, AddMessage, DefaultConversationMessage);

        EXPECT_EQ(AddMessageResult.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_EQ(FirstConversationId, AddMessageResult.GetMessageInfo().ConversationId);

        FirstMessageIdToBeDeleted = AddMessageResult.GetMessageInfo().MessageId;
    }

    /* {
            CreatedMessageInfo
                    = ConversationComponent->AddMessage(UserDisplayName, DefaultConversationMessage);

            EXPECT_EQ(CreatedMessageInfo.UserID, UserId);
            EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
            EXPECT_EQ(CreatedMessageInfo.UserDisplayName, DefaultTestUserDisplayName);
            EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);

            std::cerr << "Message with Id: " << CreatedMessageInfo.Id << " was added by " << DefaultTestUserDisplayName
                              << " to conversation: Id: " << FirstConversationId.c_str() << std::endl;
    }*/

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    LogOut(UserSystem);

    // Log in with the second account
    csp::common::String SecondTestUserId;
    LogIn(UserSystem, SecondTestUserId, AlternativeTestUser.Email, GeneratedTestAccountPassword);

    auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    bool EntitiesRetrieved = false;

    EntitySystem->SetInitialEntitiesRetrievedCallback(
        [&EntitiesRetrieved](bool Ok)
        {
            if (Ok)
            {
                EntitiesRetrieved = true;
            }
        });

    while (!EntitiesRetrieved)
    {
        EntitySystem->ProcessPendingEntityOperations();
        std::this_thread::sleep_for(10ms);
    }

    auto* FirstConversationEntity = EntitySystem->FindSpaceEntityById(ConversationObjectId);

    auto* FirstConversationComponent = (ConversationSpaceComponent*)FirstConversationEntity->GetComponent(0);

    // Add message to Conversation 1
    {
        auto [AddMessageToConversation1Result] = AWAIT(FirstConversationComponent, AddMessage, DefaultConversationMessage);
        auto CreatedMessageInfo = AddMessageToConversation1Result.GetMessageInfo();

        EXPECT_EQ(CreatedMessageInfo.UserId, SecondTestUserId);
        EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
        EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);
    }

    // Add message to Conversation 1
    {
        auto [AddMessageToConversation1Result] = AWAIT(FirstConversationComponent, AddMessage, DefaultConversationMessage);
        auto CreatedMessageInfo = AddMessageToConversation1Result.GetMessageInfo();

        EXPECT_EQ(CreatedMessageInfo.UserId, SecondTestUserId);
        EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
        EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);
    }

    uint64_t Conversation2ObjectId;

    // Create object to represent the conversation
    csp::common::String Object2Name = "Object 2";
    auto [CreatedObject2] = AWAIT(EntitySystem, CreateObject, Object2Name, ObjectTransform);

    Conversation2ObjectId = CreatedObject2->GetId();

    // Create conversation component
    auto ConversationComponent2 = (ConversationSpaceComponent*)CreatedObject2->AddComponent(ComponentType::Conversation);

    auto [Conversation2Result] = AWAIT(ConversationComponent2, CreateConversation, "Test Conversation 2 Message");

    EXPECT_EQ(Conversation2Result.GetResultCode(), csp::systems::EResultCode::Success);

    SecondConversationId = Conversation2Result.GetValue();

    {
        auto [Result] = AWAIT_PRE(ConversationComponent2, GetConversationInfo, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetConversationInfo().UserId, SecondTestUserId);
        EXPECT_EQ(Result.GetConversationInfo().Message, "Test Conversation 2 Message");
        EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
    }

    // Add a message to Conversation 2
    {
        auto [AddMessageToConversation2Result] = AWAIT(ConversationComponent2, AddMessage, DefaultConversationMessage);
        auto CreatedMessageInfo = AddMessageToConversation2Result.GetMessageInfo();

        EXPECT_EQ(CreatedMessageInfo.UserId, SecondTestUserId);
        EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);

        SecondMessageIdToBeDeleted = CreatedMessageInfo.MessageId;
    }

    // Retrieve all messages from first conversation
    {
        auto [Conversation1MessagesResult] = AWAIT(FirstConversationComponent, GetMessagesFromConversation, nullptr, nullptr);

        EXPECT_EQ(Conversation1MessagesResult.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& Messages = Conversation1MessagesResult.GetMessages();

        EXPECT_EQ(Messages.Size(), 4);
        EXPECT_EQ(Conversation1MessagesResult.GetTotalCount(), 4);
    }

    // Delete one message from first conversation
    {
        auto [Result] = AWAIT_PRE(FirstConversationComponent, DeleteMessage, RequestPredicate, FirstMessageIdToBeDeleted);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve again remaining messages from first conversation
    {
        auto [Conversation1MessagesResult] = AWAIT(FirstConversationComponent, GetMessagesFromConversation, nullptr, nullptr);

        EXPECT_EQ(Conversation1MessagesResult.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& Messages = Conversation1MessagesResult.GetMessages();

        EXPECT_EQ(Messages.Size(), 3);
        EXPECT_EQ(Conversation1MessagesResult.GetTotalCount(), 3);
    }

    // Delete first conversation entirely
    {
        auto [Result] = AWAIT_PRE(FirstConversationComponent, DeleteConversation, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve all messages from second conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent2, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& Messages = Result.GetMessages();

        EXPECT_EQ(Messages.Size(), 1);
        EXPECT_EQ(Result.GetTotalCount(), 1);
    }

    // Delete the only message from the second conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent2, DeleteMessage, RequestPredicate, SecondMessageIdToBeDeleted);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve the messages from the second conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent2, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& Messages = Result.GetMessages();

        EXPECT_EQ(Messages.Size(), 0);
        EXPECT_EQ(Result.GetTotalCount(), 0);
    }

    // Delete second conversation entirely even if it doesn't contain messages anymore
    {
        auto [Result] = AWAIT_PRE(ConversationComponent2, DeleteConversation, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    LogOut(UserSystem);

    // Log in with the space creator in order to delete it
    LogIn(UserSystem, UserId, SpaceCreatorUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

/** Removing until network event ticket OF-1387 is completed
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_NEWMESSAGE_NETWORKEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationNewMessageNetworkEventTest)
{
        SetRandSeed();

        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto* UserSystem	 = SystemsManager.GetUserSystem();
        auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
        auto* AssetSystem	 = SystemsManager.GetAssetSystem();
        auto* Connection	 = SystemsManager.GetMultiplayerConnection();
        auto* EventBus		 = SystemsManager.GetEventBus();
        auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

        const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
        const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
        const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
        const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        char UniqueAssetCollectionName[256];
        SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

        char UniqueAssetName[256];
        SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

        // Log in
        csp::common::String UserId;
        LogInAsNewTestUser(UserSystem, UserId);
        const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

        // Create space
        csp::systems::Space Space;
        CreateSpace(SpaceSystem,
                                UniqueSpaceName,
                                TestSpaceDescription,
                                csp::systems::SpaceAttributes::Private,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr,
                                Space);

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback(
                [](csp::multiplayer::SpaceEntity* Entity)
                {
                });

        // Setup Asset callback
        bool ConversationNewMessagecallbackCalled = false;
        csp::common::String ConversationId;

        auto ConversationNewMessageCallback = [&ConversationNewMessagecallbackCalled, &ConversationId](const ConversationSystemParams& Info)
        {
                if (ConversationNewMessagecallbackCalled)
                {
                        return;
                }
                EXPECT_EQ(Info.MessageType, ConversationMessageType::NewMessage);
                EXPECT_EQ(Info.MessageValue, ConversationId);
                ConversationNewMessagecallbackCalled = true;
        };

        ConversationSystem* ConversationSystem = Connection->GetConversationSystem();

        ConversationSystem->SetConversationSystemCallback(ConversationNewMessageCallback);

        auto [Result]  = AWAIT_PRE(ConversationSystem, CreateConversation, RequestPredicate, "TestMessage");
        ConversationId = Result.GetValue();

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        {
                auto [Result] = AWAIT_PRE(ConversationSystem, GetConversationInformation, RequestPredicate, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        {
                auto [Result] = AWAIT(ConversationSystem, AddMessageToConversation, ConversationId, "Test");

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        EventBus->SendNetworkEventToClient("ConversationSystem",
                                                                           {ReplicatedValue((int64_t) ConversationMessageType::NewMessage),
ConversationId}, Connection->GetClientId(),
                                                                           [](ErrorCode Error)
                                                                           {
                                                                                   ASSERT_EQ(Error, ErrorCode::None);
                                                                           });

        // Wait for message
        auto Start		 = std::chrono::steady_clock::now();
        auto Current	 = std::chrono::steady_clock::now();
        int64_t TestTime = 0;

        while (!ConversationNewMessagecallbackCalled && TestTime < 20)
        {
                std::this_thread::sleep_for(50ms);

                Current	 = std::chrono::steady_clock::now();
                TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        EXPECT_TRUE(ConversationNewMessagecallbackCalled);

        {
                auto [Result] = AWAIT(ConversationSystem, DeleteConversation, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Delete space
        DeleteSpace(SpaceSystem, Space.Id);

        // Log out
        LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_DELETEMESSAGE_NETWORKEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationDeleteMessageNetworkEventTest)
{
        SetRandSeed();

        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto* UserSystem	 = SystemsManager.GetUserSystem();
        auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
        auto* AssetSystem	 = SystemsManager.GetAssetSystem();
        auto* Connection	 = SystemsManager.GetMultiplayerConnection();
        auto* EventBus		 = SystemsManager.GetEventBus();
        auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

        const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
        const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
        const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
        const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        char UniqueAssetCollectionName[256];
        SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

        char UniqueAssetName[256];
        SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

        // Log in
        csp::common::String UserId;
        LogInAsNewTestUser(UserSystem, UserId);
        const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

        // Create space
        csp::systems::Space Space;
        CreateSpace(SpaceSystem,
                                UniqueSpaceName,
                                TestSpaceDescription,
                                csp::systems::SpaceAttributes::Private,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr,
                                Space);

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback(
                [](csp::multiplayer::SpaceEntity* Entity)
                {
                });

        // Setup Asset callback
        bool ConversationDeleteMessagecallbackCalled = false;
        csp::common::String ConversationId;
        csp::common::String MessageId;

        auto ConversationDeleteMessageCallback = [&ConversationDeleteMessagecallbackCalled, &MessageId](const ConversationSystemParams& Info)
        {
                if (Info.MessageType == ConversationMessageType::DeleteMessage)
                {
                        if (ConversationDeleteMessagecallbackCalled)
                        {
                                return;
                        }

                        EXPECT_EQ(Info.MessageValue, MessageId);
                        ConversationDeleteMessagecallbackCalled = true;
                }
        };

        ConversationSystem* ConversationSystem = Connection->GetConversationSystem();
        auto [Result]						   = AWAIT_PRE(ConversationSystem, CreateConversation, RequestPredicate,
"TestMessage"); ConversationId						   = Result.GetValue();

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        {
                auto [Result] = AWAIT_PRE(ConversationSystem, GetConversationInformation, RequestPredicate, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        {
                auto [Result] = AWAIT(ConversationSystem, AddMessageToConversation, ConversationId, "Test");

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

                MessageId = Result.GetMessageInfo().MessageId;
        }

        {
                ConversationSystem->SetConversationSystemCallback(ConversationDeleteMessageCallback);
                auto [Result] = AWAIT(ConversationSystem, DeleteMessage, MessageId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        EventBus->SendNetworkEventToClient("ConversationSystem",
                                                                           {ReplicatedValue((int64_t) ConversationMessageType::DeleteMessage),
MessageId}, Connection->GetClientId(),
                                                                           [](ErrorCode Error)
                                                                           {
                                                                                   ASSERT_EQ(Error, ErrorCode::None);
                                                                           });

        // Wait for message
        auto Start		 = std::chrono::steady_clock::now();
        auto Current	 = std::chrono::steady_clock::now();
        int64_t TestTime = 0;

        while (!ConversationDeleteMessagecallbackCalled && TestTime < 20)
        {
                std::this_thread::sleep_for(50ms);

                Current	 = std::chrono::steady_clock::now();
                TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        EXPECT_TRUE(ConversationDeleteMessagecallbackCalled);

        {
                auto [Result] = AWAIT(ConversationSystem, DeleteConversation, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Delete space
        DeleteSpace(SpaceSystem, Space.Id);

        // Log out
        LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_DELETECONVERSATION_NETWORKEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationDeleteConversationNetworkEventTest)
{
        SetRandSeed();

        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto* UserSystem	 = SystemsManager.GetUserSystem();
        auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
        auto* AssetSystem	 = SystemsManager.GetAssetSystem();
        auto* Connection	 = SystemsManager.GetMultiplayerConnection();
        auto* EventBus		 = SystemsManager.GetEventBus();
        auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

        const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
        const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
        const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
        const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        char UniqueAssetCollectionName[256];
        SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

        char UniqueAssetName[256];
        SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

        // Log in
        csp::common::String UserId;
        LogInAsNewTestUser(UserSystem, UserId);
        const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

        // Create space
        csp::systems::Space Space;
        CreateSpace(SpaceSystem,
                                UniqueSpaceName,
                                TestSpaceDescription,
                                csp::systems::SpaceAttributes::Private,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr,
                                Space);

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback(
                [](csp::multiplayer::SpaceEntity* Entity)
                {
                });

        // Setup Asset callback
        bool ConversationDeleteConversationcallbackCalled = false;
        csp::common::String ConversationId;
        csp::common::String MessageId;

        auto ConversationDeleteConversationCallback
                = [&ConversationDeleteConversationcallbackCalled, &ConversationId](const ConversationSystemParams& Info)
        {
                if (Info.MessageType == ConversationMessageType::DeleteConversation)
                {
                        if (ConversationDeleteConversationcallbackCalled)
                        {
                                return;
                        }

                        EXPECT_EQ(Info.MessageValue, ConversationId);

                        ConversationDeleteConversationcallbackCalled = true;
                }
        };

        ConversationSystem* ConversationSystem = Connection->GetConversationSystem();

        auto [Result]  = AWAIT_PRE(ConversationSystem, CreateConversation, RequestPredicate, "TestMessage");
        ConversationId = Result.GetValue();

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        {
                auto [Result] = AWAIT_PRE(ConversationSystem, GetConversationInformation, RequestPredicate, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        {
                auto [Result] = AWAIT(ConversationSystem, AddMessageToConversation, ConversationId, "Test");

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

                MessageId = Result.GetMessageInfo().MessageId;
        }

        {
                ConversationSystem->SetConversationSystemCallback(ConversationDeleteConversationCallback);
                auto [Result] = AWAIT(ConversationSystem, DeleteConversation, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        EventBus->SendNetworkEventToClient("ConversationSystem",
                                                                           {ReplicatedValue((int64_t) ConversationMessageType::DeleteConversation),
ConversationId}, Connection->GetClientId(),
                                                                           [](ErrorCode Error)
                                                                           {
                                                                                   ASSERT_EQ(Error, ErrorCode::None);
                                                                           });

        // Wait for message
        auto Start		 = std::chrono::steady_clock::now();
        auto Current	 = std::chrono::steady_clock::now();
        int64_t TestTime = 0;

        while (!ConversationDeleteConversationcallbackCalled && TestTime < 20)
        {
                std::this_thread::sleep_for(50ms);

                Current	 = std::chrono::steady_clock::now();
                TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        EXPECT_TRUE(ConversationDeleteConversationcallbackCalled);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Delete space
        DeleteSpace(SpaceSystem, Space.Id);

        // Log out
        LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_UPDATE_CONVERSATION_INFO
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, UpdateConversationInfo)
{
        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto* UserSystem	 = SystemsManager.GetUserSystem();
        auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
        auto* Connection	 = SystemsManager.GetMultiplayerConnection();
        auto* EventBus		 = SystemsManager.GetEventBus();
        auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

        const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-OKO";
        const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-OKO";

        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        // Log in
        csp::common::String UserId;
        LogInAsNewTestUser(UserSystem, UserId);
        const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

        // Create space
        csp::systems::Space Space;
        CreateSpace(SpaceSystem,
                                UniqueSpaceName,
                                TestSpaceDescription,
                                csp::systems::SpaceAttributes::Private,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr,
                                Space);

    // Setup Asset callback
    bool ConversationConversationInfocallbackCalled = false;
    csp::common::String ConversationId;
    csp::common::String MessageId;

        // Setup Asset callback
        bool ConversationConversationInfocallbackCalled = false;
        csp::common::String ConversationId;
        csp::common::String MessageId;

        auto ConversationConversationInformationCallback
                = [&ConversationConversationInfocallbackCalled, &ConversationId](const ConversationSystemParams& Info)
        {
                if (ConversationConversationInfocallbackCalled)
                {
                        return;
                }
                EXPECT_EQ(Info.MessageType, ConversationMessageType::ConversationInformation);
                EXPECT_EQ(Info.MessageValue, ConversationId);
                ConversationConversationInfocallbackCalled = true;
        };

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback(
                [](csp::multiplayer::SpaceEntity* Entity)
                {
                });

        auto* ConvSystem = Connection->GetConversationSystem();

        auto [Result]  = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
        ConversationId = Result.GetValue();

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        {
                auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        ConversationSystem* ConversationSystem = Connection->GetConversationSystem();
        ConversationSystem->SetConversationSystemCallback(ConversationConversationInformationCallback);

        {
                auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        {
                MessageInfo NewData	   = MessageInfo();
                NewData.Message		   = "TestMessage1";
                NewData.IsConversation = true;

                auto [Result] = AWAIT_PRE(ConvSystem, SetConversationInformation, RequestPredicate, ConversationId, NewData);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage1");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        EventBus->SendNetworkEventToClient("ConversationSystem",
                                                                           {ReplicatedValue((int64_t)
ConversationMessageType::ConversationInformation), ConversationId}, Connection->GetClientId(),
                                                                           [](ErrorCode Error)
                                                                           {
                                                                                   ASSERT_EQ(Error, ErrorCode::None);
                                                                           });

        {
                auto [Result] = AWAIT(ConvSystem, DeleteConversation, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Wait for message
        auto Start		 = std::chrono::steady_clock::now();
        auto Current	 = std::chrono::steady_clock::now();
        int64_t TestTime = 0;

        while (!ConversationConversationInfocallbackCalled && TestTime < 20)
        {
                std::this_thread::sleep_for(50ms);

                Current	 = std::chrono::steady_clock::now();
                TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        EXPECT_TRUE(ConversationConversationInfocallbackCalled);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Delete space
        DeleteSpace(SpaceSystem, Space.Id);
        LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_UPDATE_MESSAGE_INFO
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, UpdateMessageInfo)
{
        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto* UserSystem	 = SystemsManager.GetUserSystem();
        auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
        auto* Connection	 = SystemsManager.GetMultiplayerConnection();
        auto* EventBus		 = SystemsManager.GetEventBus();
        auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

        const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-OKO";
        const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-OKO";

        char UniqueSpaceName[256];
        SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

        // Log in
        csp::common::String UserId;
        LogInAsNewTestUser(UserSystem, UserId);
        const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

        // Create space
        csp::systems::Space Space;
        CreateSpace(SpaceSystem,
                                UniqueSpaceName,
                                TestSpaceDescription,
                                csp::systems::SpaceAttributes::Private,
                                nullptr,
                                nullptr,
                                nullptr,
                                nullptr,
                                Space);

        // Setup Asset callback
        bool ConversationMessageInfoCallbackCalled = false;
        csp::common::String ConversationId;
        csp::common::String MessageId;

        auto ConversationMessageInformationCallback = [&ConversationMessageInfoCallbackCalled, &MessageId](const ConversationSystemParams& Info)
        {
                if (ConversationMessageInfoCallbackCalled || Info.MessageType == ConversationMessageType::NewMessage)
                {
                        return;
                }

                EXPECT_EQ(Info.MessageType, ConversationMessageType::MessageInformation);
                EXPECT_EQ(Info.MessageValue, MessageId);

                ConversationMessageInfoCallbackCalled = true;
        };

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback(
                [](csp::multiplayer::SpaceEntity* Entity)
                {
                });

        auto* ConvSystem = Connection->GetConversationSystem();

        auto [Result]  = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
        ConversationId = Result.GetValue();

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        {
                auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
                EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
                EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        ConversationSystem* ConversationSystem = Connection->GetConversationSystem();
        ConversationSystem->SetConversationSystemCallback(ConversationMessageInformationCallback);

        {
                auto [Result] = AWAIT_PRE(ConvSystem, AddMessageToConversation, RequestPredicate, ConversationId, "test");

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

                MessageId = Result.GetMessageInfo().MessageId;

                EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        {
                auto [Result] = AWAIT_PRE(ConvSystem, GetMessageInformation, RequestPredicate, MessageId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        {
                MessageInfo NewData = MessageInfo();
                NewData.Message		= "NewTest";

                auto [Result] = AWAIT_PRE(ConvSystem, SetMessageInformation, RequestPredicate, MessageId, NewData);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
                EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
                EXPECT_EQ(Result.GetMessageInfo().Message, NewData.Message);
        }

        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        EventBus->SendNetworkEventToClient("ConversationSystem",
                                                                           {ReplicatedValue((int64_t) ConversationMessageType::MessageInformation),
MessageId}, Connection->GetClientId(),
                                                                           [](ErrorCode Error)
                                                                           {
                                                                                   ASSERT_EQ(Error, ErrorCode::None);
                                                                           });

        {
                auto [Result] = AWAIT(ConvSystem, DeleteConversation, ConversationId);

                EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Wait for message
        auto Start		 = std::chrono::steady_clock::now();
        auto Current	 = std::chrono::steady_clock::now();
        int64_t TestTime = 0;

        while (!ConversationMessageInfoCallbackCalled && TestTime < 20)
        {
                std::this_thread::sleep_for(50ms);

                Current	 = std::chrono::steady_clock::now();
                TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        EXPECT_TRUE(ConversationMessageInfoCallbackCalled);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Delete space
        DeleteSpace(SpaceSystem, Space.Id);
        LogOut(UserSystem);
}
#endif**/

} // namespace