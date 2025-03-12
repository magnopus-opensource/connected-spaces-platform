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

#include "../AssetSystemTestHelpers.h"
#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "MultiplayerTestRunnerProcess.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

}

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentTest)
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

    const char* TestSpaceName2 = "OLY-UNITTEST-SPACE-REWIND-2";
    const char* TestSpaceDescription2 = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueSpaceName2[256];
    SPRINTF(UniqueSpaceName2, "%s-%s", TestSpaceName2, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    const csp::common::String UserName = "Player 1";
    const SpaceTransform UserTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    const AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String UserAvatarId = "MyCoolAvatar";
    const AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        auto [Avatar] = AWAIT(EntitySystem, CreateAvatar, UserName, UserTransform, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

        // Create object to represent the conversation
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create conversation component
        auto* ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

        EXPECT_EQ(ConversationComponent->GetIsVisible(), true);
        EXPECT_EQ(ConversationComponent->GetIsActive(), true);

        ConversationComponent->SetIsActive(false);
        ConversationComponent->SetIsVisible(false);

        EXPECT_EQ(ConversationComponent->GetIsVisible(), false);
        EXPECT_EQ(ConversationComponent->GetIsActive(), false);

        SpaceTransform DefaultTransform;

        EXPECT_EQ(ConversationComponent->GetPosition().X, DefaultTransform.Position.X);
        EXPECT_EQ(ConversationComponent->GetPosition().Y, DefaultTransform.Position.Y);
        EXPECT_EQ(ConversationComponent->GetPosition().Z, DefaultTransform.Position.Z);

        csp::common::Vector3 NewPosition(1, 2, 3);
        ConversationComponent->SetPosition(NewPosition);

        EXPECT_EQ(ConversationComponent->GetPosition().X, NewPosition.X);
        EXPECT_EQ(ConversationComponent->GetPosition().Y, NewPosition.Y);
        EXPECT_EQ(ConversationComponent->GetPosition().Z, NewPosition.Z);

        EXPECT_EQ(ConversationComponent->GetRotation().W, DefaultTransform.Rotation.W);
        EXPECT_EQ(ConversationComponent->GetRotation().X, DefaultTransform.Rotation.X);
        EXPECT_EQ(ConversationComponent->GetRotation().Y, DefaultTransform.Rotation.Y);
        EXPECT_EQ(ConversationComponent->GetRotation().Z, DefaultTransform.Rotation.Z);

        csp::common::Vector4 NewRotation(4, 5, 6, 7);
        ConversationComponent->SetRotation(NewRotation);

        EXPECT_EQ(ConversationComponent->GetRotation().W, NewRotation.W);
        EXPECT_EQ(ConversationComponent->GetRotation().X, NewRotation.X);
        EXPECT_EQ(ConversationComponent->GetRotation().Y, NewRotation.Y);
        EXPECT_EQ(ConversationComponent->GetRotation().Z, NewRotation.Z);

        EXPECT_EQ(ConversationComponent->GetTitle(), "");

        ConversationComponent->SetTitle("TestTitle");
        ConversationComponent->SetResolved(true);
        csp::common::Vector3 NewCameraPosition(10, 100, 14);
        ConversationComponent->SetConversationCameraPosition(NewCameraPosition);

        EXPECT_EQ(ConversationComponent->GetTitle(), "TestTitle");
        EXPECT_EQ(ConversationComponent->GetResolved(), true);
        EXPECT_EQ(ConversationComponent->GetConversationCameraPosition().X, NewCameraPosition.X);
        EXPECT_EQ(ConversationComponent->GetConversationCameraPosition().Y, NewCameraPosition.Y);
        EXPECT_EQ(ConversationComponent->GetConversationCameraPosition().Z, NewCameraPosition.Z);

        csp::common::String ConversationId;
        csp::common::String MessageId;

        {
            auto [Result] = AWAIT(ConversationComponent, CreateConversation, "TestMessage");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_TRUE(Result.GetValue() != "");

            ConversationId = Result.GetValue();
        }

        {
            auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, "Test");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            MessageId = Result.GetMessageInfo().MessageId;

            EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetMessageInfo, MessageId);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        {
            MessageUpdateParams NewData;
            NewData.NewMessage = "NewTest";
            auto [Result] = AWAIT(ConversationComponent, UpdateMessage, MessageId, NewData);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_NE(Result.GetMessageInfo().EditedTimestamp, "");
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
            EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        {
            MessageUpdateParams NewData;
            SpaceTransform CameraTransformValue(csp::common::Vector3().One(), csp::common::Vector4().One(), csp::common::Vector3().One());
            NewData.NewMessage = "TestMessage1";

            auto [Result] = AWAIT(ConversationComponent, UpdateConversation, NewData);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage1");
            EXPECT_NE(Result.GetConversationInfo().EditedTimestamp, "");
        }

        auto TestMessage = "test123";
        EventBus->ListenNetworkEvent("ConversationSystem:NewMessage",
            [=](bool ok, csp::common::Array<ReplicatedValue> Data)
            {
                EXPECT_TRUE(ok);

                ConversationId == Data[0].GetString();
                std::cerr << "Test Event Received " << ok << std::endl;
            });

        {
            auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            MessageId = Result.GetMessageInfo().MessageId;
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetMessagesFromConversation, nullptr, nullptr);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetTotalCount(), 2);

            EXPECT_EQ(Result.GetMessages()[0].MessageId, MessageId);
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetMessageInfo, MessageId);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetMessageInfo().MessageId, MessageId);
        }

        {
            auto [Result] = AWAIT(ConversationComponent, DeleteMessage, MessageId);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        {
            auto [Result] = AWAIT(ConversationComponent, DeleteConversation);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        // Create object to represent the conversation
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create conversation component
        auto* ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

        SpaceTransform DefaultTransform;

        EXPECT_EQ(ConversationComponent->GetIsVisible(), true);
        EXPECT_EQ(ConversationComponent->GetIsActive(), true);

        EXPECT_EQ(ConversationComponent->GetPosition().X, DefaultTransform.Position.X);
        EXPECT_EQ(ConversationComponent->GetPosition().Y, DefaultTransform.Position.Y);
        EXPECT_EQ(ConversationComponent->GetPosition().Z, DefaultTransform.Position.Z);

        EXPECT_EQ(ConversationComponent->GetRotation().W, DefaultTransform.Rotation.W);
        EXPECT_EQ(ConversationComponent->GetRotation().X, DefaultTransform.Rotation.X);
        EXPECT_EQ(ConversationComponent->GetRotation().Y, DefaultTransform.Rotation.Y);
        EXPECT_EQ(ConversationComponent->GetRotation().Z, DefaultTransform.Rotation.Z);

        CreatedObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        // Setup script
        std::string ConversationScriptText = R"xx(
			var conversation = ThisEntity.getConversationComponents()[0];
			conversation.isVisible = false;
			conversation.isActive = false;
			conversation.position = [1,2,3];
			conversation.rotation = [4,5,6,7];
		)xx";

        CreatedObject->GetScript()->SetScriptSource(ConversationScriptText.c_str());
        CreatedObject->GetScript()->Invoke();

        EntitySystem->ProcessPendingEntityOperations();

        EXPECT_FALSE(ConversationComponent->GetIsVisible());
        EXPECT_FALSE(ConversationComponent->GetIsActive());

        csp::common::Vector3 NewPosition(1, 2, 3);

        EXPECT_EQ(ConversationComponent->GetPosition().X, NewPosition.X);
        EXPECT_EQ(ConversationComponent->GetPosition().Y, NewPosition.Y);
        EXPECT_EQ(ConversationComponent->GetPosition().Z, NewPosition.Z);

        csp::common::Vector4 NewRotation(4, 5, 6, 7);

        EXPECT_EQ(ConversationComponent->GetRotation().W, NewRotation.W);
        EXPECT_EQ(ConversationComponent->GetRotation().X, NewRotation.X);
        EXPECT_EQ(ConversationComponent->GetRotation().Y, NewRotation.Y);
        EXPECT_EQ(ConversationComponent->GetRotation().Z, NewRotation.Z);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    };

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_NUMBER_OF_REPLIES_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentNumberOfRepliesTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        // Create object to represent the conversation
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create conversation component
        auto* ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

        csp::common::String ConversationId;
        csp::common::String MessageId;

        {
            auto [Result] = AWAIT(ConversationComponent, CreateConversation, "TestMessage");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_TRUE(Result.GetValue() != "");

            ConversationId = Result.GetValue();
        }

        // Ensure reply count is 0
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetCount(), 0);
        }

        // Add a reply
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, "TestReply");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            MessageId = Result.GetMessageInfo().MessageId;

            EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        // Ensure reply count is 1
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetCount(), 1);
        }

        // Add another reply
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, "TestReply1");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            MessageId = Result.GetMessageInfo().MessageId;

            EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        // Ensure reply count is 2
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetCount(), 2);
        }

        // Delete the last message
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, DeleteMessage, RequestPredicate, MessageId);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Ensure reply count is 1
        {
            auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetCount(), 1);
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    };

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_DELETE_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentDeleteTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;

    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    // Create object to represent the conversation
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    csp::common::String ConversationId;

    // Create conversation component
    {
        auto* ConversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

        CreatedObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        auto [ConversationResult] = AWAIT(ConversationComponent, CreateConversation, "DefaultConversation");
        EXPECT_EQ(ConversationResult.GetResultCode(), csp::systems::EResultCode::Success);

        ConversationId = ConversationResult.GetValue();
    }

    // Ensure that the conversations asset collection exists
    {
        csp::common::Array<csp::systems::AssetCollection> Collections;
        GetAssetCollectionsByIds(AssetSystem, { ConversationId }, Collections);

        EXPECT_EQ(Collections.Size(), 1);
    }

    // Delete the component to internally call DeleteConversation
    {
        bool CallbackCalled = false;

        AssetSystem->SetAssetDetailBlobChangedCallback(
            [ConversationId, &CallbackCalled](const csp::multiplayer::AssetDetailBlobParams& Params)
            {
                EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Deleted);
                EXPECT_EQ(Params.AssetCollectionId, ConversationId);
                CallbackCalled = true;
            });

        CreatedObject->Destroy([](bool Success) {});

        CreatedObject->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        WaitForCallback(CallbackCalled);

        csp::common::Array<csp::systems::AssetCollection> Collections;
        GetAssetCollectionsByIds(AssetSystem, { ConversationId }, Collections);

        EXPECT_EQ(Collections.Size(), 0);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
    // Log out
    LogOut(UserSystem);
}
#endif

// Tests that all conversation multiplayer events are correctly sent to their componenets,
// and these events are then fired to the caller
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_EVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentEventTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    // Allow us to receive and test our own conversation messages
    auto [FlagSetResult] = AWAIT(Connection, SetAllowSelfMessagingFlag, true);

    // Create object to represent the conversation
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);
    auto* ConversationComponent = (ConversationSpaceComponent*)Object->AddComponent(ComponentType::Conversation);

    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Ensure conversation created event is fired when calling ConversationComponent::CreateConversation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        static constexpr const char* ConversationMessage = "Test Conversation";

        const auto [Result] = AWAIT(ConversationComponent, CreateConversation, ConversationMessage);

        // Due to the way events are registered, we sometimes receive the event before the ConversationId is set,
        // which is needed to correctly register it to the system to receive events.
        // Because of this, we re-register the callback which will internally flush the event buffer.
        // This issue will not exist in a real-world scenario, as multiplayer events aren't received locally
        // and a system is in place to always flush the event buffer after the conversation Id is set from the patch.
        ConversationComponent->SetConversationUpdateCallback(Callback);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::NewMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.IsConversation, true);
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, ConversationMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    csp::common::String FirstMessageId = "";

    // Ensure message created event is fired when calling ConversationComponent::AddMessage
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        static constexpr const char* Message = "Test Message";

        const auto [Result] = AWAIT(ConversationComponent, AddMessage, Message);
        FirstMessageId = Result.GetMessageInfo().MessageId;

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::NewMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.IsConversation, false);
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, Message);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    static constexpr const char* NewConversationMessage = "New Test Conversation";

    // Ensure conversation information event is fired when calling ConversationComponent::UpdateConversation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        MessageUpdateParams NewData;
        NewData.NewMessage = NewConversationMessage;

        const auto [Result] = AWAIT(ConversationComponent, UpdateConversation, NewData);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::ConversationInformation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.IsConversation, true);
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, NewConversationMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(RetrievedParams.MessageInfo.EditedTimestamp, "");
    }

    static constexpr const char* NewMessage = "New Test Message";

    // Ensure message information event is fired when calling ConversationComponent::UpdateMessage
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        MessageUpdateParams NewData;
        NewData.NewMessage = NewMessage;

        const auto [Result] = AWAIT(ConversationComponent, UpdateMessage, FirstMessageId, NewData);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::MessageInformation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.IsConversation, false);
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, NewMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(RetrievedParams.MessageInfo.EditedTimestamp, "");
    }

    // Ensure message deletion event is fired when calling ConversationComponent::DeleteMessage
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        const auto [Result] = AWAIT(ConversationComponent, DeleteMessage, FirstMessageId);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.IsConversation, false);
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, NewMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(RetrievedParams.MessageInfo.EditedTimestamp, "");

        CallbackCalled = true;
    }

    // Ensure conversation deletion event is fired when calling ConversationComponent::DeleteConversation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        const auto [Result] = AWAIT(ConversationComponent, DeleteConversation);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteConversation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.IsConversation, true);
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, NewConversationMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(RetrievedParams.MessageInfo.EditedTimestamp, "");
    }

    // Cleanup
    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

/*
Tests that the CreateConversaiton event is correctly received and processed by other clients.
Due to multiplayer messages being received before the component has a valid component id, we need to ensure that the event is stored and processed
correctly when receiving the component property from a patch, which has been created by the ConversationSpaceComponent::CreateConversation call.
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATIONCOMPONENT_SECOND_CLIENT_EVENT_DELAY_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentSecondClientEventDelayTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Create user
    auto TestUser = CreateTestUser();

    // Log in
    csp::common::String UserId;
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Unlisted, nullptr, nullptr, nullptr, nullptr, Space);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    // Get conversation component created by other client
    SpaceEntity* Entity = nullptr;
    ConversationSpaceComponent* ConversationComponent = nullptr;

    // Create multiplayer test runner to create a conversation
    MultiplayerTestRunnerProcess CreateConversationRunner
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_CONVERSATION)
              .SetSpaceId(Space.Id.c_str())
              .SetLoginEmail(TestUser.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetTimeoutInSeconds(60);

    std::future<void> ReadyForAssertionsFuture = CreateConversationRunner.ReadyForAssertionsFuture();

    // Run the test runner and wait for the entity created callback
    {
        bool CallbackCalled = false;

        EntitySystem->SetEntityCreatedCallback(
            [EntitySystem, &CallbackCalled, &Entity](csp::multiplayer::SpaceEntity* NewEntity)
            {
                Entity = NewEntity;
                CallbackCalled = true;
            });

        // Start other client
        CreateConversationRunner.StartProcess();

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(Entity != nullptr);
    }

    // Wait for the component creation patch
    {
        bool ComponentCreated = false;

        Entity->SetUpdateCallback(
            [&ComponentCreated, &ConversationComponent](
                SpaceEntity* Entity, SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>& Components)
            {
                for (size_t i = 0; i < Components.Size(); ++i)
                {
                    if (Components[i].UpdateType == ComponentUpdateType::Add)
                    {
                        ConversationComponent = static_cast<ConversationSpaceComponent*>(Entity->GetComponent(0));
                        ComponentCreated = true;
                    }
                }
            });

        // We need to wait and update here, as patches require us to process updates
        WaitForCallbackWithUpdate(ComponentCreated, EntitySystem);

        EXPECT_TRUE(ComponentCreated);
        EXPECT_TRUE(ConversationComponent != nullptr);
    }

    // Ensure conversation created callback is called
    {
        bool CallbackCalled = false;

        ConversationComponent->SetConversationUpdateCallback(
            [&CallbackCalled](const csp::multiplayer::ConversationEventParams& Params) { CallbackCalled = true; });

        WaitForCallbackWithUpdate(CallbackCalled, EntitySystem);
        EXPECT_TRUE(CallbackCalled);
    }

    // Just being safe here, so we dont hang forever in case of catastrophe.
    auto Status = ReadyForAssertionsFuture.wait_for(std::chrono::seconds(20));

    if (Status == std::future_status::timeout)
    {
        FAIL() << "CreateAvatar process timed out before it was ready for assertions.";
    }

    // Cleanup
    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

#endif

/*
Tests that other clients can't Delete other clients messages, or edit other clients conversations or messages.
Other clients can still delete other conversations, as components/entities dont have any restrictions.
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATIONCOMPONENT_PERMISSIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentPermissionsTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Create user
    auto TestUser = CreateTestUser();

    // Log in
    csp::common::String UserId;
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create a second test user
    csp::systems::Profile AlternativeTestUser = CreateTestUser();

    uint64_t ConversationObjectId = 0;
    csp::common::String ConversationId;
    csp::common::String MessageId;

    // Add the second test user to the space
    {
        auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeTestUser.Email, true, "", "");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Create object to represent the conversation
        csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);
        ConversationObjectId = Object->GetId();
        auto* ConversationComponent = (ConversationSpaceComponent*)Object->AddComponent(ComponentType::Conversation);

        Object->QueueUpdate();
        EntitySystem->ProcessPendingEntityOperations();

        // Create conversation
        {
            auto [Result] = AWAIT(ConversationComponent, CreateConversation, "TestMessage");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_TRUE(Result.GetValue() != "");

            ConversationId = Result.GetValue();

            Object->QueueUpdate();
            EntitySystem->ProcessPendingEntityOperations();
        }

        // Create message
        {
            auto [Result] = AWAIT(ConversationComponent, AddMessage, "TestMessage");
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            MessageId = Result.GetMessageInfo().MessageId;
        }

        // Logout
        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
        LogOut(UserSystem);
    }

    // Ensure component data has been written to database by chs before entering the space again
    std::this_thread::sleep_for(7s);

    {
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

        WaitForCallbackWithUpdate(EntitiesRetrieved, EntitySystem);

        auto* RetrievedConversationEntity = EntitySystem->FindSpaceEntityById(ConversationObjectId);
        auto* RetrievedConversationComponent = static_cast<ConversationSpaceComponent*>(RetrievedConversationEntity->GetComponent(0));

        static const csp::common::String NoConversationPermissionsErrorLog = "User does not have permission to modify this conversation.";
        static const csp::common::String NoMessagePermissionsErrorLog = "User does not have permission to modify this message.";

        // Attempt to edit the conversation
        {
            bool CallbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&CallbackCalled](const csp::common::String& Message)
                {
                    CallbackCalled = true;
                    EXPECT_EQ(NoConversationPermissionsErrorLog, Message);
                });

            auto [Result] = AWAIT_PRE(RetrievedConversationComponent, UpdateConversation, RequestPredicate, {});
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        // Attempt to edit the message
        {
            bool CallbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&CallbackCalled](const csp::common::String& Message)
                {
                    CallbackCalled = true;
                    EXPECT_EQ(NoMessagePermissionsErrorLog, Message);
                });

            auto [Result] = AWAIT_PRE(RetrievedConversationComponent, UpdateMessage, RequestPredicate, MessageId, {});
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        // Attempt to delete the message
        {
            bool CallbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&CallbackCalled](const csp::common::String& Message)
                {
                    CallbackCalled = true;
                    EXPECT_EQ(NoMessagePermissionsErrorLog, Message);
                });

            auto [Result] = AWAIT_PRE(RetrievedConversationComponent, DeleteMessage, RequestPredicate, MessageId);
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        csp::common::String MessageId2;

        // Ensure we can still add a message
        {
            auto [Result] = AWAIT(RetrievedConversationComponent, AddMessage, "TestMessage2");
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            MessageId2 = Result.GetMessageInfo().MessageId;
        }

        // Ensure we can still edit our own message
        {
            auto [Result] = AWAIT(RetrievedConversationComponent, UpdateMessage, MessageId2, {});
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Ensure we can still delete the conversation
        {
            auto [Result] = AWAIT(RetrievedConversationComponent, DeleteConversation);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }
    }

    // Exit space
    auto [ExitSpaceResult2] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Log out
    LogOut(UserSystem);

    // Log in with the space creator to delete the space
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif