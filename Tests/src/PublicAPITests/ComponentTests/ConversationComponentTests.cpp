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
#include "Common/DateTime.h"
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

/*
    Tests that ConversationSpaceComponents default properties are correct on construction.
    Also tests that the properties are correctly set using setters.
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_PROPERTY_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentPropertyTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);

    // Create conversation component
    auto* ConversationComponent = static_cast<ConversationSpaceComponent*>(Object->AddComponent(ComponentType::Conversation));

    // Test defaults
    EXPECT_EQ(ConversationComponent->GetConversationId(), "");
    EXPECT_EQ(ConversationComponent->GetIsVisible(), true);
    EXPECT_EQ(ConversationComponent->GetIsActive(), true);
    EXPECT_TRUE((ConversationComponent->GetPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((ConversationComponent->GetRotation() == csp::common::Vector4::Identity()));
    EXPECT_EQ(ConversationComponent->GetTitle(), "");
    EXPECT_EQ(ConversationComponent->GetResolved(), false);
    EXPECT_TRUE((ConversationComponent->GetConversationCameraPosition() == csp::common::Vector3::Zero()));

    // Set properties
    constexpr const char* TestConversationId = "TestConversationId";
    const bool TestVisible = false;
    const bool TestActive = false;
    const csp::common::Vector3 TestPosition(1, 2, 3);
    const csp::common::Vector4 TestRotation(4, 5, 6, 7);
    constexpr const char* TestTitle = "TestTitle";
    const bool TestResolved = true;
    const csp::common::Vector3 TestConversationCameraPosition(8, 9, 10);

    ConversationComponent->SetConversationId(TestConversationId);
    ConversationComponent->SetIsVisible(TestVisible);
    ConversationComponent->SetIsActive(TestActive);
    ConversationComponent->SetPosition(TestPosition);
    ConversationComponent->SetRotation(TestRotation);
    ConversationComponent->SetTitle(TestTitle);
    ConversationComponent->SetResolved(TestResolved);
    ConversationComponent->SetConversationCameraPosition(TestConversationCameraPosition);

    // Set new properties
    EXPECT_EQ(ConversationComponent->GetConversationId(), TestConversationId);
    EXPECT_EQ(ConversationComponent->GetIsVisible(), TestVisible);
    EXPECT_EQ(ConversationComponent->GetIsActive(), TestActive);
    EXPECT_TRUE((ConversationComponent->GetPosition() == TestPosition));
    EXPECT_TRUE((ConversationComponent->GetRotation() == TestRotation));
    EXPECT_EQ(ConversationComponent->GetTitle(), TestTitle);
    EXPECT_EQ(ConversationComponent->GetResolved(), TestResolved);
    EXPECT_TRUE((ConversationComponent->GetConversationCameraPosition() == TestConversationCameraPosition));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

/*
    Tests that ConversationSpaceComponents can be sucessfully modified by scripts
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_SCRIPT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);

    // Create conversation component
    auto* ConversationComponent = static_cast<ConversationSpaceComponent*>(Object->AddComponent(ComponentType::Conversation));

    // Test defaults
    EXPECT_EQ(ConversationComponent->GetIsVisible(), true);
    EXPECT_EQ(ConversationComponent->GetIsActive(), true);
    EXPECT_TRUE((ConversationComponent->GetPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((ConversationComponent->GetRotation() == csp::common::Vector4::Identity()));
    EXPECT_EQ(ConversationComponent->GetTitle(), "");
    EXPECT_EQ(ConversationComponent->GetResolved(), false);

    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script
    std::string ConversationScriptText = R"xx(
			var conversation = ThisEntity.getConversationComponents()[0];
			conversation.isVisible = false;
			conversation.isActive = false;
			conversation.position = [1,2,3];
			conversation.rotation = [4,5,6,7];
            conversation.title = "TestTitle";
            conversation.resolved = true;
            conversation.conversationCameraPosition = [8, 9, 10];
		)xx";

    Object->GetScript()->SetScriptSource(ConversationScriptText.c_str());
    Object->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    // Set new properties
    EXPECT_EQ(ConversationComponent->GetIsVisible(), false);
    EXPECT_EQ(ConversationComponent->GetIsActive(), false);
    EXPECT_TRUE((ConversationComponent->GetPosition() == csp::common::Vector3 { 1, 2, 3 }));
    EXPECT_TRUE((ConversationComponent->GetRotation() == csp::common::Vector4 { 4, 5, 6, 7 }));
    EXPECT_EQ(ConversationComponent->GetTitle(), "TestTitle");
    EXPECT_EQ(ConversationComponent->GetResolved(), true);
    EXPECT_TRUE((ConversationComponent->GetConversationCameraPosition() == csp::common::Vector3 { 8, 9, 10 }));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

/*
    Tests that ConversationSpaceComponents can sucessfully create, update and deleted messages and components.
    Also ensures all callback values are correct.
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);

    // Create conversation component
    auto* ConversationComponent = static_cast<ConversationSpaceComponent*>(Object->AddComponent(ComponentType::Conversation));

    csp::common::String ConversationId;
    csp::common::String MessageId;

    // Ensure callback values are correct when calling ConversationComponent::AddMessage
    {
        static constexpr const char* TestMessage = "TestConversation";
        auto [Result] = AWAIT(ConversationComponent, CreateConversation, TestMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        ConversationId = Result.GetValue();
    }

    // Ensure callback values are correct when calling ConversationComponent::CreateMessage
    {
        static constexpr const char* TestMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        EXPECT_EQ(Info.EditedTimestamp, "");
        EXPECT_EQ(Info.UserId, UserId);
        EXPECT_EQ(Info.Message, TestMessage);
        EXPECT_TRUE(Info.MessageId != "");

        MessageId = Info.MessageId;
    }

    csp::common::String LastConversationValue = "";

    // Ensure callback values are correct when calling ConversationComponent::UpdateConversation
    {
        static constexpr const char* TestMessage = "TestConversation2";

        MessageUpdateParams NewData;
        NewData.NewMessage = TestMessage;

        auto [Result] = AWAIT(ConversationComponent, UpdateConversation, NewData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetConversationInfo();

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        // Value should now be edited
        EXPECT_TRUE(Info.EditedTimestamp != "");
        EXPECT_EQ(Info.UserId, UserId);
        // Message should be updated with the new value
        EXPECT_EQ(Info.Message, TestMessage);
        EXPECT_EQ(Info.MessageId, "");

        LastConversationValue = Info.Message;
    }

    csp::common::String LastMessageValue = "";

    // Ensure callback values are correct when calling ConversationComponent::UpdateMessage
    {
        static constexpr const char* TestMessage = "TestMessage2";

        MessageUpdateParams NewData;
        NewData.NewMessage = TestMessage;

        auto [Result] = AWAIT(ConversationComponent, UpdateMessage, MessageId, NewData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        // Value should now be edited
        EXPECT_TRUE(Info.EditedTimestamp != "");
        EXPECT_EQ(Info.UserId, UserId);
        // Message should be updated with the new value
        EXPECT_EQ(Info.Message, TestMessage);
        EXPECT_EQ(Info.MessageId, MessageId);

        LastMessageValue = Info.Message;
    }

    // Ensure callback values are correct when calling ConversationComponent::GetConversationInfo
    {
        auto [Result] = AWAIT(ConversationComponent, GetConversationInfo);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetConversationInfo();

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        EXPECT_TRUE(Info.EditedTimestamp != "");
        EXPECT_EQ(Info.UserId, UserId);
        EXPECT_EQ(Info.Message, LastConversationValue);
        EXPECT_EQ(Info.MessageId, "");
    }

    // Ensure callback values are correct when calling ConversationComponent::GetMessageInfo
    {
        auto [Result] = AWAIT(ConversationComponent, GetMessageInfo, MessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        EXPECT_TRUE(Info.EditedTimestamp != "");
        EXPECT_EQ(Info.UserId, UserId);
        EXPECT_EQ(Info.Message, LastMessageValue);
        EXPECT_EQ(Info.MessageId, MessageId);
    }

    // Ensure callback values are correct when calling ConversationComponent::DeleteMessage
    {
        auto [Result] = AWAIT(ConversationComponent, DeleteMessage, MessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Ensure callback values are correct when calling ConversationComponent::DeleteConversation
    {
        auto [Result] = AWAIT(ConversationComponent, DeleteConversation);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_PREREQUISITES_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentPrerequisitesTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);

    // Create conversation component
    auto* ConversationComponent = static_cast<ConversationSpaceComponent*>(Object->AddComponent(ComponentType::Conversation));

    static const csp::common::String NoConversationErrorLog = "This component does not have an associated conversation. "
                                                              "Call CreateConversation to create a new conversation for this component";

    // Ensure DeleteConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteConversation, RequestPredicate);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    csp::common::String MessageId;

    // Ensure AddMessage errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, "");

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        MessageId = Result.GetMessageInfo().MessageId;
    }

    // Ensure DeleteMessage errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteMessage, RequestPredicate, MessageId);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetMessagesFromConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetConversationInfo errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationInfo, RequestPredicate);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure UpdateConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, UpdateConversation, RequestPredicate, {});

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetMessageInfo errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetMessageInfo, RequestPredicate, MessageId);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure UpdateMessage errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, UpdateMessage, RequestPredicate, MessageId, {});

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetNumberOfReplies errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    static const csp::common::String AlreadyHasConversationErrorLog = "This component does not have an associated conversation. "
                                                                      "Call CreateConversation to create a new conversation for this component";

    // Ensure CreateConversation errors and logs appropriately when a conversation has already been created
    {
        // Create the first conversation
        auto [Result] = AWAIT_PRE(ConversationComponent, CreateConversation, RequestPredicate, "");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(AlreadyHasConversationErrorLog, Message);
            });

        // Attempt to create the second conversation
        auto [Result2] = AWAIT_PRE(ConversationComponent, CreateConversation, RequestPredicate, "");

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result2.GetHttpResultCode(), 0);
        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

/*
    Ensures the ConversationComponent::GetNumberOfReplies work with a varying amount of messages.
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_GETNUMBEROFREPLIES_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentGetNumberOfRepliesTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

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
    csp::common::String MessageId1;
    csp::common::String MessageId2;

    constexpr const char* TestConversationMessage = "TestConversation";
    constexpr const char* TestMessage1 = "TestMessage1";
    constexpr const char* TestMessage2 = "TestMessage2";

    // Create conversation
    {
        auto [Result] = AWAIT(ConversationComponent, CreateConversation, TestConversationMessage);

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
        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage1);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        MessageId1 = Result.GetMessageInfo().MessageId;
    }

    // Ensure reply count is 1
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 1);
    }

    // Add another reply
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage2);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        MessageId2 = Result.GetMessageInfo().MessageId;
    }

    // Ensure reply count is 2
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 2);
    }

    // Delete the last message
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteMessage, RequestPredicate, MessageId2);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Ensure reply count is 1
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 1);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
    // Log out
    LogOut(UserSystem);
}
#endif

/*
    Ensures the ConversationComponent::GetMessagesFromConversation work with a varying amount of messages.
*/
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_GETMESSAGESFROMCONVERSATION_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentGetMessagesFromConversationTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

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
    csp::common::String MessageId1;
    csp::common::String MessageId2;

    constexpr const char* TestConversationMessage = "TestConversation";
    constexpr const char* TestMessage1 = "TestMessage1";
    constexpr const char* TestMessage2 = "TestMessage2";

    // Create conversation
    {
        auto [Result] = AWAIT(ConversationComponent, CreateConversation, TestConversationMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        ConversationId = Result.GetValue();
    }

    // Ensure no messages are found
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 0);
    }

    // Add a reply
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage1);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        MessageId1 = Result.GetMessageInfo().MessageId;
    }

    // Ensure we have our reply
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 1);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessages()[0];

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        EXPECT_EQ(Info.EditedTimestamp, "");
        EXPECT_EQ(Info.UserId, UserId);
        EXPECT_EQ(Info.Message, TestMessage1);
        EXPECT_EQ(Info.MessageId, MessageId1);
    }

    // Add another reply
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage2);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        MessageId2 = Result.GetMessageInfo().MessageId;
    }

    // Ensure we have both replies
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 2);

        const csp::multiplayer::MessageInfo& Info1 = Result.GetMessages()[1];

        EXPECT_EQ(Info1.ConversationId, ConversationId);
        EXPECT_TRUE(Info1.CreatedTimestamp != "");
        EXPECT_EQ(Info1.EditedTimestamp, "");
        EXPECT_EQ(Info1.UserId, UserId);
        EXPECT_EQ(Info1.Message, TestMessage1);
        EXPECT_EQ(Info1.MessageId, MessageId1);

        const csp::multiplayer::MessageInfo& Info2 = Result.GetMessages()[0];

        EXPECT_EQ(Info2.ConversationId, ConversationId);
        EXPECT_TRUE(Info2.CreatedTimestamp != "");
        EXPECT_EQ(Info2.EditedTimestamp, "");
        EXPECT_EQ(Info2.UserId, UserId);
        EXPECT_EQ(Info2.Message, TestMessage2);
        EXPECT_EQ(Info2.MessageId, MessageId2);
    }

    // Delete the first message
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteMessage, RequestPredicate, MessageId1);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Ensure we still have our second message
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 1);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessages()[0];

        EXPECT_EQ(Info.ConversationId, ConversationId);
        EXPECT_TRUE(Info.CreatedTimestamp != "");
        EXPECT_EQ(Info.EditedTimestamp, "");
        EXPECT_EQ(Info.UserId, UserId);
        EXPECT_EQ(Info.Message, TestMessage2);
        EXPECT_EQ(Info.MessageId, MessageId2);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
    // Log out
    LogOut(UserSystem);
}
#endif

/*
    Ensures that when deleting the ConversationComponent, it internally calls DeleteConversation
*/
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

/*
Tests that all conversation multiplayer events are correctly sent to their componenets,
*/
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

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::NewConversation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
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
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, Message);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

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

        static constexpr const char* NewMessage = "New Test Conversation";
        MessageUpdateParams NewData;
        NewData.NewMessage = NewMessage;

        const auto [Result] = AWAIT(ConversationComponent, UpdateConversation, NewData);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::ConversationInformation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, NewMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(RetrievedParams.MessageInfo.EditedTimestamp, "");
    }

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

        static constexpr const char* NewMessage = "New Test Message";
        MessageUpdateParams NewData;
        NewData.NewMessage = NewMessage;

        const auto [Result] = AWAIT(ConversationComponent, UpdateMessage, FirstMessageId, NewData);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::MessageInformation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
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
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_EQ(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.EditedTimestamp, "");

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
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_EQ(RetrievedParams.MessageInfo.EditedTimestamp, "");
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
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

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

    csp::multiplayer::MessageInfo ReceivedInfo;

    // Ensure conversation created callback is called
    {
        csp::multiplayer::ConversationEventParams ReceivedParams;
        bool CallbackCalled = false;

        ConversationComponent->SetConversationUpdateCallback(
            [&CallbackCalled, &ReceivedParams, &ReceivedInfo](const csp::multiplayer::ConversationEventParams& Params)
            {
                ReceivedParams = Params;
                ReceivedInfo = Params.MessageInfo;
                CallbackCalled = true;
            });

        WaitForCallbackWithUpdate(CallbackCalled, EntitySystem);
        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(ReceivedParams.MessageType, csp::multiplayer::ConversationEventType::NewConversation);

        ConversationComponent->SetConversationUpdateCallback(nullptr);
    }

    // Ensure we can get the information about the conversation
    {
        auto [Result] = AWAIT(ConversationComponent, GetConversationInfo);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetConversationInfo();

        EXPECT_EQ(Info.ConversationId, ReceivedInfo.ConversationId);
        // Currently commenting out until CHS fix an inconsistancy with the CreatedTimestamp

        // Currently converting to milliseconds to get around chs rounding error inconsistency
        csp::common::DateTime InfoCreatedTime(Info.CreatedTimestamp);
        auto CreatedDuration = InfoCreatedTime.GetTimePoint().time_since_epoch();
        auto CreatedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(CreatedDuration);

        csp::common::DateTime ReceivedInfoCreatedTime(ReceivedInfo.CreatedTimestamp);
        auto ReceivedDuration = ReceivedInfoCreatedTime.GetTimePoint().time_since_epoch();
        auto ReceivedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(ReceivedDuration);

        EXPECT_EQ(CreatedMilliseconds, ReceivedMilliseconds);

        EXPECT_EQ(Info.EditedTimestamp, ReceivedInfo.EditedTimestamp);
        EXPECT_EQ(Info.UserId, ReceivedInfo.UserId);
        EXPECT_EQ(Info.Message, ReceivedInfo.Message);
        EXPECT_EQ(Info.MessageId, ReceivedInfo.MessageId);
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