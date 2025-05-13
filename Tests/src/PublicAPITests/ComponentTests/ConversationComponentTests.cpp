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
const std::vector<char> PngHeader = { static_cast<char>(0x89), static_cast<char>(0x50), static_cast<char>(0x4E), static_cast<char>(0x47),
    static_cast<char>(0x0D), static_cast<char>(0x0A), static_cast<char>(0x1A), static_cast<char>(0x0A) };

}

/*
    Tests that ConversationSpaceComponents default properties are correct on construction.
    Also tests that the properties are correctly set using setters.
*/
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
    EXPECT_TRUE((ConversationComponent->GetConversationCameraRotation() == csp::common::Vector4::Identity()));

    // Set new properties
    constexpr const char* TestConversationId = "TestConversationId";
    const bool TestVisible = false;
    const bool TestActive = false;
    const csp::common::Vector3 TestPosition(1, 2, 3);
    const csp::common::Vector4 TestRotation(4, 5, 6, 7);
    constexpr const char* TestTitle = "TestTitle";
    const bool TestResolved = true;
    const csp::common::Vector3 TestConversationCameraPosition(8, 9, 10);
    const csp::common::Vector4 TestConversationCameraRotation(11, 12, 13, 14);

    ConversationComponent->SetConversationId(TestConversationId);
    ConversationComponent->SetIsVisible(TestVisible);
    ConversationComponent->SetIsActive(TestActive);
    ConversationComponent->SetPosition(TestPosition);
    ConversationComponent->SetRotation(TestRotation);
    ConversationComponent->SetTitle(TestTitle);
    ConversationComponent->SetResolved(TestResolved);
    ConversationComponent->SetConversationCameraPosition(TestConversationCameraPosition);
    ConversationComponent->SetConversationCameraRotation(TestConversationCameraRotation);

    // Test properties are correctly set/get
    EXPECT_EQ(ConversationComponent->GetConversationId(), TestConversationId);
    EXPECT_EQ(ConversationComponent->GetIsVisible(), TestVisible);
    EXPECT_EQ(ConversationComponent->GetIsActive(), TestActive);
    EXPECT_TRUE((ConversationComponent->GetPosition() == TestPosition));
    EXPECT_TRUE((ConversationComponent->GetRotation() == TestRotation));
    EXPECT_EQ(ConversationComponent->GetTitle(), TestTitle);
    EXPECT_EQ(ConversationComponent->GetResolved(), TestResolved);
    EXPECT_TRUE((ConversationComponent->GetConversationCameraPosition() == TestConversationCameraPosition));
    EXPECT_TRUE((ConversationComponent->GetConversationCameraRotation() == TestConversationCameraRotation));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
    Tests that ConversationSpaceComponents can be sucessfully modified by scripts
*/
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
    EXPECT_TRUE((ConversationComponent->GetConversationCameraPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((ConversationComponent->GetConversationCameraRotation() == csp::common::Vector4::Identity()));

    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script to set new properties
    std::string ConversationScriptText = R"xx(
			var conversation = ThisEntity.getConversationComponents()[0];
			conversation.isVisible = false;
			conversation.isActive = false;
			conversation.position = [1,2,3];
			conversation.rotation = [4,5,6,7];
            conversation.title = "TestTitle";
            conversation.resolved = true;
            conversation.conversationCameraPosition = [8, 9, 10];
            conversation.conversationCameraRotation = [11, 12, 13, 14];
		)xx";

    Object->GetScript().SetScriptSource(ConversationScriptText.c_str());
    Object->GetScript().Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    // Test scripts sets new properties
    EXPECT_EQ(ConversationComponent->GetIsVisible(), false);
    EXPECT_EQ(ConversationComponent->GetIsActive(), false);
    EXPECT_TRUE((ConversationComponent->GetPosition() == csp::common::Vector3 { 1, 2, 3 }));
    EXPECT_TRUE((ConversationComponent->GetRotation() == csp::common::Vector4 { 4, 5, 6, 7 }));
    EXPECT_EQ(ConversationComponent->GetTitle(), "TestTitle");
    EXPECT_EQ(ConversationComponent->GetResolved(), true);
    EXPECT_TRUE((ConversationComponent->GetConversationCameraPosition() == csp::common::Vector3 { 8, 9, 10 }));
    EXPECT_TRUE((ConversationComponent->GetConversationCameraRotation() == csp::common::Vector4 { 11, 12, 13, 14 }));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
    Tests that ConversationSpaceComponents can sucessfully create, update and deleted messages and components.
    Also ensures all callback values are correct.
*/
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

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
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

/*
Tests conversation functions can't be used before the component has been initialized
*/
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

/*
    Ensures the ConversationComponent::GetNumberOfReplies work with a varying amount of messages.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentGetNumberOfRepliesTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

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

/*
    Ensures the ConversationComponent::GetMessagesFromConversation work with a varying amount of messages.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentGetMessagesFromConversationTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

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

/*
    Ensures that when deleting the ConversationComponent, it internally calls DeleteConversation
*/
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

        CreatedObject->Destroy([](bool /*Success*/) {});

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

/*
Tests that all conversation multiplayer events are correctly sent to their components
*/
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

/*
Tests that the CreateConversaiton event is correctly received and processed by other clients.
Due to multiplayer messages being received before the component has a valid component id, we need to ensure that the event is stored and processed
correctly when receiving the component property from a patch, which has been created by the ConversationSpaceComponent::CreateConversation call.
*/
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ConversationTests, ConversationComponentSecondClientEventDelayTest)
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
              .SetEndpoint(EndpointBaseURI())
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

/*
Tests that other clients can't Delete other clients messages, or edit other clients conversations or messages.
Other clients can still delete other conversations, as components/entities dont have any restrictions.
*/
CSP_PUBLIC_TEST(DISABLED_CSPEngine, ConversationTests, ConversationComponentPermissionsTest)
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

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

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
                    if (Message == NoConversationPermissionsErrorLog)
                    {
                        CallbackCalled = true;
                    }
                });

            auto [Result] = AWAIT_PRE(RetrievedConversationComponent, UpdateConversation, RequestPredicate, {});

            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
            EXPECT_TRUE(CallbackCalled);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        // Attempt to edit the message
        {
            bool CallbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&CallbackCalled](const csp::common::String& Message)
                {
                    if (Message == NoMessagePermissionsErrorLog)
                    {
                        CallbackCalled = true;
                    }
                });

            auto [Result] = AWAIT_PRE(RetrievedConversationComponent, UpdateMessage, RequestPredicate, MessageId, {});
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
            EXPECT_TRUE(CallbackCalled);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        // Attempt to delete the message
        {
            bool CallbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&CallbackCalled](const csp::common::String& Message)
                {
                    if (Message == NoMessagePermissionsErrorLog)
                    {
                        CallbackCalled = true;
                    }
                });

            auto [Result] = AWAIT_PRE(RetrievedConversationComponent, DeleteMessage, RequestPredicate, MessageId);
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
            EXPECT_TRUE(CallbackCalled);

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

/*
Tests that annotations can be correctly get, set and deleted
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentCreateAnnotationTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);

    // Create conversation component
    auto* ConversationComponent = static_cast<ConversationSpaceComponent*>(Object->AddComponent(ComponentType::Conversation));

    csp::common::String ConversationId;
    csp::common::String MessageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
    {
        static constexpr const char* TestMessage = "TestConversation";
        auto [Result] = AWAIT(ConversationComponent, CreateConversation, TestMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        ConversationId = Result.GetValue();
    }

    static const csp::common::Vector3 TestConversationAuthorCameraPosition { 1.f, 2.f, 3.f };
    static const csp::common::Vector4 TestConversationAuthorCameraRotation { 4.f, 5.f, 6.f, 7.f };
    static constexpr const uint16_t TestConversationFov = 90;

    std::vector<char> TestAnnotationDataV = PngHeader;
    TestAnnotationDataV.push_back('1');
    std::vector<char> TestAnnotationThumbnailDataV = PngHeader;
    TestAnnotationDataV.push_back('2');

    // Create annotation attached to conversation
    {
        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = TestConversationAuthorCameraPosition;
        Data.AuthorCameraRotation = TestConversationAuthorCameraRotation;
        Data.VerticalFov = TestConversationFov;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationDataV.data();
        AnnotationBufferData.BufferLength = TestAnnotationDataV.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestAnnotationThumbnailDataV.data();
        AnnotationThumbnailBufferData.BufferLength = TestAnnotationThumbnailDataV.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            ConversationComponent, SetConversationAnnotation, RequestPredicate, Data, AnnotationBufferData, AnnotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& RetrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((RetrievedData.GetAuthorCameraPosition() == TestConversationAuthorCameraPosition));
        EXPECT_TRUE((RetrievedData.GetAuthorCameraRotation() == TestConversationAuthorCameraRotation));
        EXPECT_EQ(RetrievedData.GetVerticalFov(), TestConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> DataV(ResultData, ResultData + strlen(ResultData));
        EXPECT_EQ(TestAnnotationDataV, DataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> ThumbnailDataV(ResultAnnotationData, ResultAnnotationData + strlen(ResultAnnotationData));
        EXPECT_EQ(TestAnnotationThumbnailDataV, ThumbnailDataV);
    }

    // Get conversation annotation and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& RetrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((RetrievedData.GetAuthorCameraPosition() == TestConversationAuthorCameraPosition));
        EXPECT_TRUE((RetrievedData.GetAuthorCameraRotation() == TestConversationAuthorCameraRotation));
        EXPECT_EQ(RetrievedData.GetVerticalFov(), TestConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> DataV(ResultData, ResultData + strlen(ResultData));
        EXPECT_EQ(TestAnnotationDataV, DataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> ThumbnailDataV(ResultAnnotationData, ResultAnnotationData + strlen(ResultAnnotationData));
        EXPECT_EQ(TestAnnotationThumbnailDataV, ThumbnailDataV);
    }

    // Delete conversation annotation and ensure collection and assets are deleted
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Attempt to get the deleted conversation annotation
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                if (Message == "Message asset collection doesn't contain annotation data.")
                {
                    CallbackCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        EXPECT_TRUE(CallbackCalled);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure callback values are correct when calling ConversationComponent::CreateMessage
    {
        static constexpr const char* TestMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();
        MessageId = Info.MessageId;
    }

    static const csp::common::Vector3 TestAuthorCameraPosition { 8.f, 9.f, 10.f };
    static const csp::common::Vector4 TestAuthorCameraRotation { 11.f, 12.f, 13.f, 14.f };
    static constexpr const uint16_t TestFov = 100;

    std::vector<char> TestAnnotationData2V = PngHeader;
    TestAnnotationData2V.push_back('3');
    std::vector<char> TestAnnotationThumbnailData2V = PngHeader;
    TestAnnotationData2V.push_back('4');

    // Create annotation attached to message
    {
        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = TestAuthorCameraPosition;
        Data.AuthorCameraRotation = TestAuthorCameraRotation;
        Data.VerticalFov = TestFov;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationData2V.data();
        AnnotationBufferData.BufferLength = TestAnnotationData2V.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestAnnotationThumbnailData2V.data();
        AnnotationThumbnailBufferData.BufferLength = TestAnnotationThumbnailData2V.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(ConversationComponent, SetAnnotation, RequestPredicate, MessageId, Data, AnnotationBufferData, AnnotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& RetrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((RetrievedData.GetAuthorCameraPosition() == TestAuthorCameraPosition));
        EXPECT_TRUE((RetrievedData.GetAuthorCameraRotation() == TestAuthorCameraRotation));
        EXPECT_EQ(RetrievedData.GetVerticalFov(), TestFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> DataV(ResultData, ResultData + strlen(ResultData));
        EXPECT_EQ(TestAnnotationData2V, DataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> ThumbnailDataV(ResultAnnotationData, ResultAnnotationData + strlen(ResultAnnotationData));
        EXPECT_EQ(TestAnnotationThumbnailData2V, ThumbnailDataV);
    }

    // Get annotation and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotation, RequestPredicate, MessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& RetrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((RetrievedData.GetAuthorCameraPosition() == TestAuthorCameraPosition));
        EXPECT_TRUE((RetrievedData.GetAuthorCameraRotation() == TestAuthorCameraRotation));
        EXPECT_EQ(RetrievedData.GetVerticalFov(), TestFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> DataV(ResultData, ResultData + strlen(ResultData));
        EXPECT_EQ(TestAnnotationData2V, DataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> ThumbnailData2V(ResultAnnotationData, ResultAnnotationData + strlen(ResultAnnotationData));
        EXPECT_EQ(TestAnnotationThumbnailData2V, ThumbnailData2V);
    }

    // Delete annotation and ensure collection and assets are deleted
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteAnnotation, RequestPredicate, MessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Attempt to get the deleted annotation
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                if (Message == "Message asset collection doesn't contain annotation data.")
                {
                    CallbackCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotation, RequestPredicate, MessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        EXPECT_TRUE(CallbackCalled);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
Tests that all annotation multiplayer events are correctly sent to their componenets,
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationEventTest)
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

    static constexpr const char* ConversationMessage = "Test Conversation";

    // Create conversation
    {
        // We test conversation creation in another test.
        // However, we still need this callback to flush the original creation event
        bool CallbackCalled = false;

        const auto Callback = [&CallbackCalled](const csp::multiplayer::ConversationEventParams& /*Params*/) { CallbackCalled = true; };
        ConversationComponent->SetConversationUpdateCallback(Callback);

        const auto [Result] = AWAIT(ConversationComponent, CreateConversation, ConversationMessage);

        ConversationComponent->SetConversationUpdateCallback(Callback);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);
    }

    // Ensure conversation annotation set event is fired when calling ConversationComponent::SetConversationAnnotation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        std::vector<char> TestAnnotationDataV = PngHeader;
        TestAnnotationDataV.push_back('1');
        std::vector<char> TestAnnotationThumbnailDataV = PngHeader;
        TestAnnotationDataV.push_back('2');

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 1.f, 2.f, 3.f };
        Data.AuthorCameraRotation = { 4.f, 5.f, 6.f, 7.f };
        Data.VerticalFov = 90;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationDataV.data();
        AnnotationBufferData.BufferLength = TestAnnotationDataV.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestAnnotationThumbnailDataV.data();
        AnnotationThumbnailBufferData.BufferLength = TestAnnotationThumbnailDataV.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            ConversationComponent, SetConversationAnnotation, RequestPredicate, Data, AnnotationBufferData, AnnotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::SetConversationAnnotation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, ConversationMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    // Ensure annotation delete event is fired when calling ConversationComponent::DeleteConversationAnnotation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteConversationAnnotation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, ConversationMessage);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    csp::common::String FirstMessageId = "";
    static constexpr const char* Message = "Test Message";

    // Ensure message created event is fired when calling ConversationComponent::AddMessage
    {
        bool CallbackCalled = false;

        const auto Callback = [&CallbackCalled](const csp::multiplayer::ConversationEventParams& /*Params*/) { CallbackCalled = true; };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        const auto [Result] = AWAIT(ConversationComponent, AddMessage, Message);
        FirstMessageId = Result.GetMessageInfo().MessageId;

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);
    }

    // Ensure annotation set event is fired when calling ConversationComponent::SetAnnotation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        std::vector<char> TestAnnotationDataV = PngHeader;
        TestAnnotationDataV.push_back('1');
        std::vector<char> TestAnnotationThumbnailDataV = PngHeader;
        TestAnnotationDataV.push_back('2');

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 1.f, 2.f, 3.f };
        Data.AuthorCameraRotation = { 4.f, 5.f, 6.f, 7.f };
        Data.VerticalFov = 90;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationDataV.data();
        AnnotationBufferData.BufferLength = TestAnnotationDataV.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestAnnotationThumbnailDataV.data();
        AnnotationThumbnailBufferData.BufferLength = TestAnnotationThumbnailDataV.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            ConversationComponent, SetAnnotation, RequestPredicate, FirstMessageId, Data, AnnotationBufferData, AnnotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(CallbackCalled);
        EXPECT_TRUE(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::SetAnnotation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, Message);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    // Ensure anntation delete event is fired when calling ConversationComponent::DeleteAnnotation
    {
        csp::multiplayer::ConversationEventParams RetrievedParams;
        bool CallbackCalled = false;

        const auto Callback = [&RetrievedParams, &CallbackCalled](const csp::multiplayer::ConversationEventParams& Params)
        {
            RetrievedParams = Params;
            CallbackCalled = true;
        };

        ConversationComponent->SetConversationUpdateCallback(Callback);

        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteAnnotation, RequestPredicate, FirstMessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(CallbackCalled);

        EXPECT_EQ(RetrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteAnnotation);
        EXPECT_EQ(RetrievedParams.MessageInfo.ConversationId, ConversationComponent->GetConversationId());
        EXPECT_EQ(RetrievedParams.MessageInfo.UserId, UserId);
        EXPECT_EQ(RetrievedParams.MessageInfo.Message, Message);
        EXPECT_EQ(RetrievedParams.MessageInfo.MessageId, FirstMessageId);
        EXPECT_NE(RetrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
Tests that annotations can be correctly overwritten
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentOverwriteAnnotationTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object = CreateTestObject(EntitySystem);

    // Create conversation component
    auto* ConversationComponent = static_cast<ConversationSpaceComponent*>(Object->AddComponent(ComponentType::Conversation));

    csp::common::String ConversationId;
    csp::common::String MessageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
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
        MessageId = Info.MessageId;
    }

    // Create annotation attached to message
    {
        static const csp::common::Vector3 TestAuthorCameraPosition { 1.f, 2.f, 3.f };
        static const csp::common::Vector4 TestAuthorCameraRotation { 4.f, 5.f, 6.f, 7.f };
        static constexpr const uint16_t TestFov = 90;
        std::vector<char> TestAnnotationDataV = PngHeader;
        TestAnnotationDataV.push_back('1');
        std::vector<char> TestAnnotationThumbnailDataV = PngHeader;
        TestAnnotationDataV.push_back('2');

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = TestAuthorCameraPosition;
        Data.AuthorCameraRotation = TestAuthorCameraRotation;
        Data.VerticalFov = TestFov;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationDataV.data();
        AnnotationBufferData.BufferLength = TestAnnotationDataV.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestAnnotationThumbnailDataV.data();
        AnnotationThumbnailBufferData.BufferLength = TestAnnotationThumbnailDataV.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(ConversationComponent, SetAnnotation, RequestPredicate, MessageId, Data, AnnotationBufferData, AnnotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Overwrite annotation
    {
        static const csp::common::Vector3 TestAuthorCameraPosition { 8.f, 9.f, 10.f };
        static const csp::common::Vector4 TestAuthorCameraRotation { 11.f, 12.f, 13.f, 14.f };
        static constexpr const uint16_t TestFov = 100;
        bool AssetOverwriteLogCalled = false;

        std::vector<char> TestAnnotationDataV = PngHeader;
        TestAnnotationDataV.push_back('3');
        std::vector<char> TestAnnotationThumbnailDataV = PngHeader;
        TestAnnotationDataV.push_back('4');

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&AssetOverwriteLogCalled](const csp::common::String& Message)
            {
                if (Message == "ConversationSystemInternal::SetAnnotation, asset already exists, so not creating")
                {
                    AssetOverwriteLogCalled = true;
                }
            });

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = TestAuthorCameraPosition;
        Data.AuthorCameraRotation = TestAuthorCameraRotation;
        Data.VerticalFov = TestFov;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationDataV.data();
        AnnotationBufferData.BufferLength = TestAnnotationDataV.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestAnnotationThumbnailDataV.data();
        AnnotationThumbnailBufferData.BufferLength = TestAnnotationThumbnailDataV.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(ConversationComponent, SetAnnotation, RequestPredicate, MessageId, Data, AnnotationBufferData, AnnotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_TRUE(AssetOverwriteLogCalled);

        const csp::multiplayer::AnnotationData& RetrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((RetrievedData.GetAuthorCameraPosition() == TestAuthorCameraPosition));
        EXPECT_TRUE((RetrievedData.GetAuthorCameraRotation() == TestAuthorCameraRotation));
        EXPECT_EQ(RetrievedData.GetVerticalFov(), TestFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> DataV(ResultData, ResultData + strlen(ResultData));
        EXPECT_EQ(TestAnnotationDataV, DataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* ResultThumbnailData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> ThumbnailDataV(ResultThumbnailData, ResultThumbnailData + strlen(ResultThumbnailData));
        EXPECT_EQ(TestAnnotationThumbnailDataV, ThumbnailDataV);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
Tests annotation functions can't be used before the component has been initialized
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationsPrerequisitesTest)
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

    // Ensure GetAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotation, RequestPredicate, "");

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetConversationAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetConversationAnnotation, RequestPredicate);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure SetAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        Data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        Data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = "";
        AnnotationBufferData.BufferLength = 0;
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = "";
        AnnotationThumbnailBufferData.BufferLength = 0;
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(ConversationComponent, SetAnnotation, RequestPredicate, "", Data, AnnotationBufferData, AnnotationThumbnailBufferData);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure SetConversationAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        Data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        Data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = "";
        AnnotationBufferData.BufferLength = 0;
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = "";
        AnnotationThumbnailBufferData.BufferLength = 0;
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            ConversationComponent, SetConversationAnnotation, RequestPredicate, Data, AnnotationBufferData, AnnotationThumbnailBufferData);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure DeleteAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteAnnotation, RequestPredicate, "");

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure DeleteConversationAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, DeleteConversationAnnotation, RequestPredicate);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetAnnotationThumbnailsForConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool CallbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&CallbackCalled](const csp::common::String& Message)
            {
                CallbackCalled = true;
                EXPECT_EQ(NoConversationErrorLog, Message);
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);

        EXPECT_TRUE(CallbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
Tests annotations cant be attached to invalid messages
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationInvalidMessageTest)
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

    // Create the conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, CreateConversation, RequestPredicate, "TestMessage");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Try and get an annotation on an invalid message on a valid conversation
    {
        bool LogCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&LogCalled](const csp::common::String& Message)
            {
                if (Message == "Failed to get message asset collection.")
                {
                    LogCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotation, RequestPredicate, "");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        EXPECT_TRUE(LogCalled);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Try and set an annotation on an invalid message on a valid message
    {
        bool LogCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&LogCalled](const csp::common::String& Message)
            {
                if (Message == "Failed to get message asset collection.")
                {
                    LogCalled = true;
                }
            });

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        Data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        Data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = "";
        AnnotationBufferData.BufferLength = 0;
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = "";
        AnnotationThumbnailBufferData.BufferLength = 0;
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(ConversationComponent, SetAnnotation, RequestPredicate, "", Data, AnnotationBufferData, AnnotationThumbnailBufferData);

        EXPECT_TRUE(LogCalled);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
Tests annotations cant be attached to messages of a different conversation
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationIncorrectConversationTest)
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

    // Create the conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, CreateConversation, RequestPredicate, "TestMessage");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Create object to hold component
    csp::multiplayer::SpaceEntity* Object2 = CreateTestObject(EntitySystem, "Object2");

    // Create conversation component
    auto* ConversationComponent2 = static_cast<ConversationSpaceComponent*>(Object2->AddComponent(ComponentType::Conversation));

    // Create the second conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent2, CreateConversation, RequestPredicate, "TestMessage2");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    csp::common::String MessageId;

    // Create a message on the first conversation
    {
        static constexpr const char* TestMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();
        MessageId = Info.MessageId;
    }

    // Attempt to add an annotation to the message using the second conversation
    {
        bool LogCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&LogCalled](const csp::common::String& Message)
            {
                if (Message == "Given message doesn't exist on the conversation.")
                {
                    LogCalled = true;
                }
            });

        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        Data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        Data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = "";
        AnnotationBufferData.BufferLength = 0;
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = "";
        AnnotationThumbnailBufferData.BufferLength = 0;
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            ConversationComponent2, SetAnnotation, RequestPredicate, MessageId, Data, AnnotationBufferData, AnnotationThumbnailBufferData);

        EXPECT_TRUE(LogCalled);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Attempt to get an annotation on the message using the second conversation
    {
        bool LogCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&LogCalled](const csp::common::String& Message)
            {
                if (Message == "Given message doesn't exist on the conversation.")
                {
                    LogCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(ConversationComponent2, GetAnnotation, RequestPredicate, MessageId);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Attempt to delete an annotation on the message using the second conversation
    {
        bool LogCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&LogCalled](const csp::common::String& Message)
            {
                if (Message == "Given message doesn't exist on the conversation.")
                {
                    LogCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(ConversationComponent2, DeleteAnnotation, RequestPredicate, MessageId);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

/*
Tests annotation thumbnails can be retrieved
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationThumbnailsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

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

    // Create the conversation
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, CreateConversation, RequestPredicate, "TestMessage");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    csp::common::String MessageId;
    csp::common::String MessageId2;

    // Create a message on the first conversation
    {
        static constexpr const char* TestMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();
        MessageId = Info.MessageId;
    }

    // Create a message on the first conversation
    {
        static constexpr const char* TestMessage = "TestMessage2";

        auto [Result] = AWAIT_PRE(ConversationComponent, AddMessage, RequestPredicate, TestMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& Info = Result.GetMessageInfo();
        MessageId2 = Info.MessageId;
    }

    // Try and get thumbnails before any have been added
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetTotalCount(), 0);
    }

    std::vector<char> TestAnnotationDataV = PngHeader;
    TestAnnotationDataV.push_back('1');

    std::vector<char> TestAnnotationData2V = PngHeader;
    TestAnnotationData2V.push_back('2');

    std::vector<char> TestThumbnailDataV = PngHeader;
    TestThumbnailDataV.push_back('3');

    std::vector<char> TestThumbnailData2V = PngHeader;
    TestThumbnailData2V.push_back('4');

    // Add annotation to the first message
    {
        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        Data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        Data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationDataV.data();
        AnnotationBufferData.BufferLength = TestAnnotationDataV.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestThumbnailDataV.data();
        AnnotationThumbnailBufferData.BufferLength = TestThumbnailDataV.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(ConversationComponent, SetAnnotation, RequestPredicate, MessageId, Data, AnnotationBufferData, AnnotationThumbnailBufferData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Try and get thumbnails after one has been added
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetTotalCount(), 1);

        // Ensure the data matches
        auto [DownloadAnnotationResult]
            = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAssetsMap()[MessageId]);
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* Data = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> DataV(Data, Data + strlen(Data));
        EXPECT_EQ(TestThumbnailDataV, DataV);
    }

    // Add annotation to the second message
    {
        AnnotationUpdateParams Data;
        Data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        Data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        Data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource AnnotationBufferData;
        AnnotationBufferData.Buffer = TestAnnotationData2V.data();
        AnnotationBufferData.BufferLength = TestAnnotationData2V.size();
        AnnotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource AnnotationThumbnailBufferData;
        AnnotationThumbnailBufferData.Buffer = TestThumbnailData2V.data();
        AnnotationThumbnailBufferData.BufferLength = TestThumbnailData2V.size();
        AnnotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            ConversationComponent, SetAnnotation, RequestPredicate, MessageId2, Data, AnnotationBufferData, AnnotationThumbnailBufferData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Try and get thumbnails after two have been added
    {
        auto [Result] = AWAIT_PRE(ConversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetTotalCount(), 2);

        // Ensure the data matches
        auto [DownloadAnnotationResult1]
            = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAssetsMap()[MessageId]);
        EXPECT_EQ(DownloadAnnotationResult1.GetResultCode(), csp::systems::EResultCode::Success);

        const char* Data = static_cast<const char*>(DownloadAnnotationResult1.GetData());
        std::vector<char> DataV(Data, Data + strlen(Data));
        EXPECT_EQ(TestThumbnailDataV, DataV);

        auto [DownloadAnnotationResult2]
            = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAssetsMap()[MessageId2]);
        EXPECT_EQ(DownloadAnnotationResult2.GetResultCode(), csp::systems::EResultCode::Success);

        const char* Data2 = static_cast<const char*>(DownloadAnnotationResult2.GetData());
        std::vector<char> DataV2(Data2, Data2 + strlen(Data2));
        EXPECT_EQ(TestThumbnailData2V, DataV2);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}