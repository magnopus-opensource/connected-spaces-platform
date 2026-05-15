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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/DateTime.h"
#include "MultiplayerTestRunnerProcess.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    // Test defaults
    EXPECT_EQ(conversationComponent->GetConversationId(), "");
    EXPECT_EQ(conversationComponent->GetIsVisible(), true);
    EXPECT_EQ(conversationComponent->GetIsActive(), true);
    EXPECT_TRUE((conversationComponent->GetPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((conversationComponent->GetRotation() == csp::common::Vector4::Identity()));
    EXPECT_EQ(conversationComponent->GetTitle(), "");
    EXPECT_EQ(conversationComponent->GetResolved(), false);
    EXPECT_TRUE((conversationComponent->GetConversationCameraPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((conversationComponent->GetConversationCameraRotation() == csp::common::Vector4::Identity()));

    // Set new properties
    constexpr const char* testConversationId = "TestConversationId";
    const bool testVisible = false;
    const bool testActive = false;
    const csp::common::Vector3 testPosition(1, 2, 3);
    const csp::common::Vector4 testRotation(4, 5, 6, 7);
    constexpr const char* testTitle = "TestTitle";
    const bool testResolved = true;
    const csp::common::Vector3 testConversationCameraPosition(8, 9, 10);
    const csp::common::Vector4 testConversationCameraRotation(11, 12, 13, 14);

    conversationComponent->SetConversationId(testConversationId);
    conversationComponent->SetIsVisible(testVisible);
    conversationComponent->SetIsActive(testActive);
    conversationComponent->SetPosition(testPosition);
    conversationComponent->SetRotation(testRotation);
    conversationComponent->SetTitle(testTitle);
    conversationComponent->SetResolved(testResolved);
    conversationComponent->SetConversationCameraPosition(testConversationCameraPosition);
    conversationComponent->SetConversationCameraRotation(testConversationCameraRotation);

    // Test properties are correctly set/get
    EXPECT_EQ(conversationComponent->GetConversationId(), testConversationId);
    EXPECT_EQ(conversationComponent->GetIsVisible(), testVisible);
    EXPECT_EQ(conversationComponent->GetIsActive(), testActive);
    EXPECT_TRUE((conversationComponent->GetPosition() == testPosition));
    EXPECT_TRUE((conversationComponent->GetRotation() == testRotation));
    EXPECT_EQ(conversationComponent->GetTitle(), testTitle);
    EXPECT_EQ(conversationComponent->GetResolved(), testResolved);
    EXPECT_TRUE((conversationComponent->GetConversationCameraPosition() == testConversationCameraPosition));
    EXPECT_TRUE((conversationComponent->GetConversationCameraRotation() == testConversationCameraRotation));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
    Tests that ConversationSpaceComponents can be sucessfully modified by scripts
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    // Test defaults
    EXPECT_EQ(conversationComponent->GetIsVisible(), true);
    EXPECT_EQ(conversationComponent->GetIsActive(), true);
    EXPECT_TRUE((conversationComponent->GetPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((conversationComponent->GetRotation() == csp::common::Vector4::Identity()));
    EXPECT_EQ(conversationComponent->GetTitle(), "");
    EXPECT_EQ(conversationComponent->GetResolved(), false);
    EXPECT_TRUE((conversationComponent->GetConversationCameraPosition() == csp::common::Vector3::Zero()));
    EXPECT_TRUE((conversationComponent->GetConversationCameraRotation() == csp::common::Vector4::Identity()));

    object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script to set new properties
    std::string conversationScriptText = R"xx(
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

    object->GetScript().SetScriptSource(conversationScriptText.c_str());
    object->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Test scripts sets new properties
    EXPECT_EQ(conversationComponent->GetIsVisible(), false);
    EXPECT_EQ(conversationComponent->GetIsActive(), false);
    EXPECT_TRUE((conversationComponent->GetPosition() == csp::common::Vector3 { 1, 2, 3 }));
    EXPECT_TRUE((conversationComponent->GetRotation() == csp::common::Vector4 { 4, 5, 6, 7 }));
    EXPECT_EQ(conversationComponent->GetTitle(), "TestTitle");
    EXPECT_EQ(conversationComponent->GetResolved(), true);
    EXPECT_TRUE((conversationComponent->GetConversationCameraPosition() == csp::common::Vector3 { 8, 9, 10 }));
    EXPECT_TRUE((conversationComponent->GetConversationCameraRotation() == csp::common::Vector4 { 11, 12, 13, 14 }));

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
    Tests that ConversationSpaceComponents can sucessfully create, update and deleted messages and components.
    Also ensures all callback values are correct.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    csp::common::String conversationId;
    csp::common::String messageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
    {
        static constexpr const char* testMessage = "TestConversation";
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    // Ensure callback values are correct when calling ConversationComponent::CreateMessage
    {
        static constexpr const char* testMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        EXPECT_EQ(info.EditedTimestamp, "");
        EXPECT_EQ(info.UserId, userId);
        EXPECT_EQ(info.Message, testMessage);
        EXPECT_TRUE(info.MessageId != "");

        messageId = info.MessageId;
    }

    csp::common::String lastConversationValue = "";

    // Ensure callback values are correct when calling ConversationComponent::UpdateConversation
    {
        static constexpr const char* testMessage = "TestConversation2";

        MessageUpdateParams newData;
        newData.NewMessage = testMessage;

        auto [Result] = AWAIT(conversationComponent, UpdateConversation, newData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetConversationInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        // Value should now be edited
        EXPECT_TRUE(info.EditedTimestamp != "");
        EXPECT_EQ(info.UserId, userId);
        // Message should be updated with the new value
        EXPECT_EQ(info.Message, testMessage);
        EXPECT_EQ(info.MessageId, "");

        lastConversationValue = info.Message;
    }

    csp::common::String lastMessageValue = "";

    // Ensure callback values are correct when calling ConversationComponent::UpdateMessage
    {
        static constexpr const char* testMessage = "TestMessage2";

        MessageUpdateParams newData;
        newData.NewMessage = testMessage;

        auto [Result] = AWAIT(conversationComponent, UpdateMessage, messageId, newData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        // Value should now be edited
        EXPECT_TRUE(info.EditedTimestamp != "");
        EXPECT_EQ(info.UserId, userId);
        // Message should be updated with the new value
        EXPECT_EQ(info.Message, testMessage);
        EXPECT_EQ(info.MessageId, messageId);

        lastMessageValue = info.Message;
    }

    // Ensure callback values are correct when calling ConversationComponent::GetConversationInfo
    {
        auto [Result] = AWAIT(conversationComponent, GetConversationInfo);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetConversationInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        EXPECT_TRUE(info.EditedTimestamp != "");
        EXPECT_EQ(info.UserId, userId);
        EXPECT_EQ(info.Message, lastConversationValue);
        EXPECT_EQ(info.MessageId, "");
    }

    // Ensure callback values are correct when calling ConversationComponent::GetMessageInfo
    {
        auto [Result] = AWAIT(conversationComponent, GetMessageInfo, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        EXPECT_TRUE(info.EditedTimestamp != "");
        EXPECT_EQ(info.UserId, userId);
        EXPECT_EQ(info.Message, lastMessageValue);
        EXPECT_EQ(info.MessageId, messageId);
    }

    // Ensure callback values are correct when calling ConversationComponent::DeleteMessage
    {
        auto [Result] = AWAIT(conversationComponent, DeleteMessage, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Ensure callback values are correct when calling ConversationComponent::DeleteConversation
    {
        auto [Result] = AWAIT(conversationComponent, DeleteConversation);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests conversation functions can't be used before the component has been initialized
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentPrerequisitesTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    static const csp::common::String noConversationErrorLog = "This component does not have an associated conversation. "
                                                              "Call CreateConversation to create a new conversation for this component";

    // Ensure DeleteConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, DeleteConversation, RequestPredicate);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    csp::common::String messageId;

    // Ensure AddMessage errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, "");

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        messageId = Result.GetMessageInfo().MessageId;
    }

    // Ensure DeleteMessage errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, DeleteMessage, RequestPredicate, messageId);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetMessagesFromConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetConversationInfo errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetConversationInfo, RequestPredicate);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure UpdateConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, UpdateConversation, RequestPredicate, {});

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetMessageInfo errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetMessageInfo, RequestPredicate, messageId);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure UpdateMessage errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, UpdateMessage, RequestPredicate, messageId, {});

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetNumberOfReplies errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    static const csp::common::String alreadyHasConversationErrorLog = "This component already has an associated conversation.";

    // Ensure CreateConversation errors and logs appropriately when a conversation has already been created
    {
        // Create the first conversation
        auto [Result] = AWAIT_PRE(conversationComponent, CreateConversation, RequestPredicate, "");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(alreadyHasConversationErrorLog, message);
            });

        // Attempt to create the second conversation
        auto [Result2] = AWAIT_PRE(conversationComponent, CreateConversation, RequestPredicate, "");

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result2.GetHttpResultCode(), 0);
        EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Exit space
    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
    Ensures the ConversationComponent::GetNumberOfReplies work with a varying amount of messages.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentGetNumberOfRepliesTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create object to represent the conversation
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create conversation component
    auto* conversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

    csp::common::String conversationId;
    csp::common::String messageId1;
    csp::common::String messageId2;

    constexpr const char* testConversationMessage = "TestConversation";
    constexpr const char* testMessage1 = "TestMessage1";
    constexpr const char* testMessage2 = "TestMessage2";

    // Create conversation
    {
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testConversationMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    // Ensure reply count is 0
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 0);
    }

    // Add a reply
    {
        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage1);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        messageId1 = Result.GetMessageInfo().MessageId;
    }

    // Ensure reply count is 1
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 1);
    }

    // Add another reply
    {
        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage2);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        messageId2 = Result.GetMessageInfo().MessageId;
    }

    // Ensure reply count is 2
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 2);
    }

    // Delete the last message
    {
        auto [Result] = AWAIT_PRE(conversationComponent, DeleteMessage, RequestPredicate, messageId2);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Ensure reply count is 1
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetNumberOfReplies, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetCount(), 1);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);
    // Log out
    LogOut(userSystem);
}

/*
    Ensures the ConversationComponent::GetMessagesFromConversation work with a varying amount of messages.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentGetMessagesFromConversationTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create object to represent the conversation
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create conversation component
    auto* conversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

    csp::common::String conversationId;
    csp::common::String messageId1;
    csp::common::String messageId2;

    constexpr const char* testConversationMessage = "TestConversation";
    constexpr const char* testMessage1 = "TestMessage1";
    constexpr const char* testMessage2 = "TestMessage2";

    // Create conversation
    {
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testConversationMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    // Ensure no messages are found
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 0);
    }

    // Add a reply
    {
        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage1);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        messageId1 = Result.GetMessageInfo().MessageId;
    }

    // Ensure we have our reply
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 1);

        const csp::multiplayer::MessageInfo& info = Result.GetMessages()[0];

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        EXPECT_EQ(info.EditedTimestamp, "");
        EXPECT_EQ(info.UserId, userId);
        EXPECT_EQ(info.Message, testMessage1);
        EXPECT_EQ(info.MessageId, messageId1);
    }

    // Add another reply
    {
        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage2);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        messageId2 = Result.GetMessageInfo().MessageId;
    }

    // Ensure we have both replies
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 2);

        const csp::multiplayer::MessageInfo& info1 = Result.GetMessages()[1];

        EXPECT_EQ(info1.ConversationId, conversationId);
        EXPECT_TRUE(info1.CreatedTimestamp != "");
        EXPECT_EQ(info1.EditedTimestamp, "");
        EXPECT_EQ(info1.UserId, userId);
        EXPECT_EQ(info1.Message, testMessage1);
        EXPECT_EQ(info1.MessageId, messageId1);

        const csp::multiplayer::MessageInfo& info2 = Result.GetMessages()[0];

        EXPECT_EQ(info2.ConversationId, conversationId);
        EXPECT_TRUE(info2.CreatedTimestamp != "");
        EXPECT_EQ(info2.EditedTimestamp, "");
        EXPECT_EQ(info2.UserId, userId);
        EXPECT_EQ(info2.Message, testMessage2);
        EXPECT_EQ(info2.MessageId, messageId2);
    }

    // Delete the first message
    {
        auto [Result] = AWAIT_PRE(conversationComponent, DeleteMessage, RequestPredicate, messageId1);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Ensure we still have our second message
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetMessagesFromConversation, RequestPredicate, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetMessages().Size(), 1);

        const csp::multiplayer::MessageInfo& info = Result.GetMessages()[0];

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        EXPECT_EQ(info.EditedTimestamp, "");
        EXPECT_EQ(info.UserId, userId);
        EXPECT_EQ(info.Message, testMessage2);
        EXPECT_EQ(info.MessageId, messageId2);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);
    // Log out
    LogOut(userSystem);
}

/*
    Ensures that when deleting the ConversationComponent, it internally calls DeleteConversation
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentDeleteTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    auto* assetSystem = systemsManager.GetAssetSystem();

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

    // Create object to represent the conversation
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    csp::common::String conversationId;

    // Create conversation component
    {
        auto* conversationComponent = (ConversationSpaceComponent*)CreatedObject->AddComponent(ComponentType::Conversation);

        CreatedObject->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        auto [ConversationResult] = AWAIT(conversationComponent, CreateConversation, "DefaultConversation");
        EXPECT_EQ(ConversationResult.GetResultCode(), csp::systems::EResultCode::Success);

        conversationId = ConversationResult.GetValue();
    }

    // Ensure that the conversations asset collection exists
    {
        csp::common::Array<csp::systems::AssetCollection> collections;
        GetAssetCollectionsByIds(assetSystem, { conversationId }, collections);

        EXPECT_EQ(collections.Size(), 1);
    }

    // Delete the component to internally call DeleteConversation
    {
        bool callbackCalled = false;

        assetSystem->SetAssetDetailBlobChangedCallback(
            [conversationId, &callbackCalled](const csp::common::AssetDetailBlobChangedNetworkEventData& networkEventData)
            {
                EXPECT_EQ(networkEventData.ChangeType, csp::common::EAssetChangeType::Deleted);
                EXPECT_EQ(networkEventData.AssetCollectionId, conversationId);
                callbackCalled = true;
            });

        CreatedObject->Destroy([](bool /*Success*/) {});

        CreatedObject->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        WaitForCallback(callbackCalled);

        csp::common::Array<csp::systems::AssetCollection> collections;
        GetAssetCollectionsByIds(assetSystem, { conversationId }, collections);

        EXPECT_EQ(collections.Size(), 0);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);
    // Log out
    LogOut(userSystem);
}

/*
Tests that all conversation multiplayer events are correctly sent to their components
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentEventTest)
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

    // Allow us to receive and test our own conversation messages
    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    // Create object to represent the conversation
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());
    auto* conversationComponent = (ConversationSpaceComponent*)object->AddComponent(ComponentType::Conversation);

    object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Ensure conversation created event is fired when calling ConversationComponent::CreateConversation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        static constexpr const char* conversationMessage = "Test Conversation";

        const auto [Result] = AWAIT(conversationComponent, CreateConversation, conversationMessage);

        // Due to the way events are registered, we sometimes receive the event before the ConversationId is set,
        // which is needed to correctly register it to the system to receive events.
        // Because of this, we re-register the callback which will internally flush the event buffer.
        // This issue will not exist in a real-world scenario, as multiplayer events aren't received locally
        // and a system is in place to always flush the event buffer after the conversation Id is set from the patch.
        conversationComponent->SetConversationUpdateCallback(callback);

        WaitForCallback(callbackCalled);
        ASSERT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::NewConversation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, conversationMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    csp::common::String firstMessageId = "";

    // Ensure message created event is fired when calling ConversationComponent::AddMessage
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        static constexpr const char* message = "Test Message";

        const auto [Result] = AWAIT(conversationComponent, AddMessage, message);
        firstMessageId = Result.GetMessageInfo().MessageId;

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::NewMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, message);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, firstMessageId);
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    static constexpr const char* newConversationMessage = "New Test Conversation";

    // Ensure conversation information event is fired when calling ConversationComponent::UpdateConversation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        MessageUpdateParams newData;
        newData.NewMessage = newConversationMessage;

        const auto [Result] = AWAIT(conversationComponent, UpdateConversation, newData);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::ConversationInformation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, newConversationMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(retrievedParams.MessageInfo.EditedTimestamp, "");
    }

    static constexpr const char* newMessage = "New Test Message";

    // Ensure message information event is fired when calling ConversationComponent::UpdateMessage
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        MessageUpdateParams newData;
        newData.NewMessage = newMessage;

        const auto [Result] = AWAIT(conversationComponent, UpdateMessage, firstMessageId, newData);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::MessageInformation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, newMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, firstMessageId);
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(retrievedParams.MessageInfo.EditedTimestamp, "");
    }

    // Ensure message deletion event is fired when calling ConversationComponent::DeleteMessage
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        const auto [Result] = AWAIT(conversationComponent, DeleteMessage, firstMessageId);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, newMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, firstMessageId);
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(retrievedParams.MessageInfo.EditedTimestamp, "");

        callbackCalled = true;
    }

    // Ensure conversation deletion event is fired when calling ConversationComponent::DeleteConversation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        const auto [Result] = AWAIT(conversationComponent, DeleteConversation);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteConversation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, newConversationMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
        EXPECT_NE(retrievedParams.MessageInfo.EditedTimestamp, "");
    }

    // Cleanup
    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
Tests that the CreateConversaiton event is correctly received and processed by other clients.
Due to multiplayer messages being received before the component has a valid component id, we need to ensure that the event is stored and processed
correctly when receiving the component property from a patch, which has been created by the ConversationSpaceComponent::CreateConversation call.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentSecondClientEventDelayTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Create users
    auto testUser = CreateTestUser();
    auto testUser2 = CreateTestUser();

    // Log in
    csp::common::String userId;
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space space;
    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::systems::InviteUserRoleInfo inviteUser;
    inviteUser.UserEmail = testUser2.Email;
    inviteUser.UserRole = csp::systems::SpaceUserRole::Moderator;
    csp::systems::InviteUserRoleInfoCollection inviteUsers;
    inviteUsers.InviteUserRoleInfos = { inviteUser };

    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, inviteUsers, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Get conversation component created by other client
    SpaceEntity* entity = nullptr;
    ConversationSpaceComponent* conversationComponent = nullptr;

    // Create multiplayer test runner to create a conversation
    MultiplayerTestRunnerProcess createConversationRunner
        = MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_CONVERSATION)
              .SetSpaceId(space.Id.c_str())
              .SetLoginEmail(testUser2.Email.c_str())
              .SetPassword(GeneratedTestAccountPassword)
              .SetEndpoint(EndpointBaseURI())
              .SetTimeoutInSeconds(60);

    std::future<void> readyForAssertionsFuture = createConversationRunner.ReadyForAssertionsFuture();

    // Run the test runner and wait for the entity created callback
    {
        bool callbackCalled = false;

        realtimeEngine->SetRemoteEntityCreatedCallback(
            [&callbackCalled, &entity](csp::multiplayer::SpaceEntity* newEntity)
            {
                if (newEntity->GetName() == "TestObject")
                {
                    entity = newEntity;
                    callbackCalled = true;
                }
            });

        // Start other client
        createConversationRunner.StartProcess();

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(entity != nullptr);
    }

    // Wait for the component creation patch
    {
        bool componentCreated = false;

        entity->SetUpdateCallback(
            [&componentCreated, &conversationComponent](
                SpaceEntity* entity, SpaceEntityUpdateFlags, csp::common::Array<ComponentUpdateInfo>& components)
            {
                if (entity->GetName() == "TestObject")
                {
                    for (size_t i = 0; i < components.Size(); ++i)
                    {
                        if (components[i].UpdateType == ComponentUpdateType::Add)
                        {
                            conversationComponent = static_cast<ConversationSpaceComponent*>(entity->GetComponent(0));
                            componentCreated = true;
                        }
                    }
                }
            });

        // We need to wait and update here, as patches require us to process updates
        WaitForCallbackWithUpdate(componentCreated, realtimeEngine.get());

        EXPECT_TRUE(componentCreated);
        EXPECT_TRUE(conversationComponent != nullptr);
    }

    csp::multiplayer::MessageInfo receivedInfo;

    // Ensure conversation created callback is called
    {
        csp::common::ConversationNetworkEventData receivedParams;
        bool callbackCalled = false;

        conversationComponent->SetConversationUpdateCallback(
            [&callbackCalled, &receivedParams, &receivedInfo](const csp::common::ConversationNetworkEventData& params)
            {
                receivedParams = params;
                receivedInfo = params.MessageInfo;
                callbackCalled = true;
            });

        WaitForCallbackWithUpdate(callbackCalled, realtimeEngine.get());
        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(receivedParams.MessageType, csp::multiplayer::ConversationEventType::NewConversation);

        conversationComponent->SetConversationUpdateCallback(nullptr);
    }

    // Ensure we can get the information about the conversation
    {
        auto [Result] = AWAIT(conversationComponent, GetConversationInfo);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetConversationInfo();

        EXPECT_EQ(info.ConversationId, receivedInfo.ConversationId);

        // Currently converting to milliseconds to get around chs rounding error inconsistency
        csp::common::DateTime infoCreatedTime(info.CreatedTimestamp);
        auto createdDuration = infoCreatedTime.GetTimePoint().time_since_epoch();
        auto createdMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(createdDuration);

        csp::common::DateTime receivedInfoCreatedTime(receivedInfo.CreatedTimestamp);
        auto receivedDuration = receivedInfoCreatedTime.GetTimePoint().time_since_epoch();
        auto receivedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(receivedDuration);

        EXPECT_EQ(createdMilliseconds, receivedMilliseconds);

        EXPECT_EQ(info.EditedTimestamp, receivedInfo.EditedTimestamp);
        EXPECT_EQ(info.UserId, receivedInfo.UserId);
        EXPECT_EQ(info.Message, receivedInfo.Message);
        EXPECT_EQ(info.MessageId, receivedInfo.MessageId);
    }

    // Just being safe here, so we dont hang forever in case of catastrophe.
    auto status = readyForAssertionsFuture.wait_for(std::chrono::seconds(20));

    if (status == std::future_status::timeout)
    {
        FAIL() << "CreateAvatar process timed out before it was ready for assertions.";
    }

    // Cleanup
    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
Tests that other clients can't Delete other clients messages, or edit other clients conversations or messages.
Other clients can still delete other conversations, as components/entities dont have any restrictions.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentPermissionsTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Create user
    auto testUser = CreateTestUser();

    // Log in
    csp::common::String userId;
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create a second test user
    csp::systems::Profile alternativeTestUser = CreateTestUser();

    uint64_t conversationObjectId = 0;
    csp::common::String messageId;

    bool entitiesCreated = false;

    auto entitiesReadyCallback = [&entitiesCreated](int /*NumEntitiesFetched*/) { entitiesCreated = true; };

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback(entitiesReadyCallback);
    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Add the second test user to the space
    {
        auto [Result] = AWAIT_PRE(spaceSystem, InviteToSpace, RequestPredicate, space.Id, alternativeTestUser.Email, true, "", "");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    {
        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Ensure patch rate limiting is off, as we're sending patches in quick succession.
        realtimeEngine->SetEntityPatchRateLimitEnabled(false);

        // Create object to represent the conversation
        csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());
        conversationObjectId = object->GetId();
        auto* conversationComponent = (ConversationSpaceComponent*)object->AddComponent(ComponentType::Conversation);

        object->QueueUpdate();
        realtimeEngine->ProcessPendingEntityOperations();

        // Create conversation
        {
            auto [Result] = AWAIT(conversationComponent, CreateConversation, "TestMessage");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_TRUE(Result.GetValue() != "");

            object->QueueUpdate();
            realtimeEngine->ProcessPendingEntityOperations();
        }

        // Create message
        {
            auto [Result] = AWAIT(conversationComponent, AddMessage, "TestMessage");
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            messageId = Result.GetMessageInfo().MessageId;
        }

        // Logout
        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
        LogOut(userSystem);
    }

    // Ensure component data has been written to database by chs before entering the space again
    std::this_thread::sleep_for(7s);

    {
        // Log in with the second account
        csp::common::String secondTestUserId;
        LogIn(userSystem, secondTestUserId, alternativeTestUser.Email, GeneratedTestAccountPassword);

        entitiesCreated = false;

        auto [EnterResult2] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(entitiesCreated, realtimeEngine.get());

        auto* retrievedConversationEntity = realtimeEngine->FindSpaceEntityById(conversationObjectId);
        auto* retrievedConversationComponent = static_cast<ConversationSpaceComponent*>(retrievedConversationEntity->GetComponent(0));

        static const csp::common::String noConversationPermissionsErrorLog = "User does not have permission to modify this conversation.";
        static const csp::common::String noMessagePermissionsErrorLog = "User does not have permission to modify this message.";

        // Attempt to edit the conversation
        {
            bool callbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
                {
                    if (message == noConversationPermissionsErrorLog)
                    {
                        callbackCalled = true;
                    }
                });

            auto [Result] = AWAIT_PRE(retrievedConversationComponent, UpdateConversation, RequestPredicate, {});

            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
            EXPECT_TRUE(callbackCalled);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        // Attempt to edit the message
        {
            bool callbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
                {
                    if (message == noMessagePermissionsErrorLog)
                    {
                        callbackCalled = true;
                    }
                });

            auto [Result] = AWAIT_PRE(retrievedConversationComponent, UpdateMessage, RequestPredicate, messageId, {});
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
            EXPECT_TRUE(callbackCalled);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        // Attempt to delete the message
        {
            bool callbackCalled = false;

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
                [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
                {
                    if (message == noMessagePermissionsErrorLog)
                    {
                        callbackCalled = true;
                    }
                });

            auto [Result] = AWAIT_PRE(retrievedConversationComponent, DeleteMessage, RequestPredicate, messageId);
            EXPECT_EQ(Result.GetHttpResultCode(), 0);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
            EXPECT_TRUE(callbackCalled);

            csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
        }

        csp::common::String messageId2;

        // Ensure we can still add a message
        {
            auto [Result] = AWAIT(retrievedConversationComponent, AddMessage, "TestMessage2");
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

            messageId2 = Result.GetMessageInfo().MessageId;
        }

        // Ensure we can still edit our own message
        {
            auto [Result] = AWAIT(retrievedConversationComponent, UpdateMessage, messageId2, {});
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }

        // Ensure we can still delete the conversation
        {
            auto [Result] = AWAIT(retrievedConversationComponent, DeleteConversation);
            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        }
    }

    // Exit space
    auto [ExitSpaceResult2] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Log out
    LogOut(userSystem);

    // Log in with the space creator to delete the space
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests that annotations can be correctly get, set and deleted
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentCreateAnnotationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    csp::common::String conversationId;
    csp::common::String messageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
    {
        static constexpr const char* testMessage = "TestConversation";
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    static const csp::common::Vector3 testConversationAuthorCameraPosition { 1.f, 2.f, 3.f };
    static const csp::common::Vector4 testConversationAuthorCameraRotation { 4.f, 5.f, 6.f, 7.f };
    static constexpr const float testConversationFov = 90.f;

    std::vector<char> testAnnotationDataV = PngHeader;
    testAnnotationDataV.push_back('1');
    std::vector<char> testAnnotationThumbnailDataV = PngHeader;
    testAnnotationDataV.push_back('2');

    // Create annotation attached to conversation
    {
        AnnotationUpdateParams data;
        data.AuthorCameraPosition = testConversationAuthorCameraPosition;
        data.AuthorCameraRotation = testConversationAuthorCameraRotation;
        data.VerticalFov = testConversationFov;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent, SetConversationAnnotation, RequestPredicate, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testConversationAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testConversationAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationDataV, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailDataV, thumbnailDataV);
    }

    // Get conversation annotation and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testConversationAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testConversationAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationDataV, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailDataV, thumbnailDataV);
    }

    // Delete conversation annotation and ensure collection and assets are deleted
    {
        auto [Result] = AWAIT_PRE(conversationComponent, DeleteConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Attempt to get the deleted conversation annotation
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Message asset collection doesn't contain annotation data.")
                {
                    callbackCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        EXPECT_TRUE(callbackCalled);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure callback values are correct when calling ConversationComponent::CreateMessage
    {
        static constexpr const char* testMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();
        messageId = info.MessageId;
    }

    static const csp::common::Vector3 testAuthorCameraPosition { 8.f, 9.f, 10.f };
    static const csp::common::Vector4 testAuthorCameraRotation { 11.f, 12.f, 13.f, 14.f };
    static constexpr const float testFov = 100.f;

    std::vector<char> testAnnotationData2V = PngHeader;
    testAnnotationData2V.push_back('3');
    std::vector<char> testAnnotationThumbnailData2V = PngHeader;
    testAnnotationData2V.push_back('4');

    // Create annotation attached to message
    {
        AnnotationUpdateParams data;
        data.AuthorCameraPosition = testAuthorCameraPosition;
        data.AuthorCameraRotation = testAuthorCameraRotation;
        data.VerticalFov = testFov;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationData2V.data();
        annotationBufferData.BufferLength = testAnnotationData2V.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailData2V.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailData2V.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, messageId, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationData2V, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailData2V, thumbnailDataV);
    }

    // Get annotation and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotation, RequestPredicate, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationData2V, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailData2V(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailData2V, thumbnailData2V);
    }

    // Delete annotation and ensure collection and assets are deleted
    {
        auto [Result] = AWAIT_PRE(conversationComponent, DeleteAnnotation, RequestPredicate, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Attempt to get the deleted annotation
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Message asset collection doesn't contain annotation data.")
                {
                    callbackCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotation, RequestPredicate, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        EXPECT_TRUE(callbackCalled);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests that all annotation multiplayer events are correctly sent to their componenets,
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationEventTest)
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

    // Allow us to receive and test our own conversation messages
    auto [FlagSetResult] = AWAIT(connection, SetAllowSelfMessagingFlag, true);

    // Create object to represent the conversation
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());
    auto* conversationComponent = (ConversationSpaceComponent*)object->AddComponent(ComponentType::Conversation);

    object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    static constexpr const char* conversationMessage = "Test Conversation";

    // Create conversation
    {
        // We test conversation creation in another test.
        // However, we still need this callback to flush the original creation event
        bool callbackCalled = false;

        const auto callback = [&callbackCalled](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled = true; };
        conversationComponent->SetConversationUpdateCallback(callback);

        const auto [Result] = AWAIT(conversationComponent, CreateConversation, conversationMessage);

        conversationComponent->SetConversationUpdateCallback(callback);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);
    }

    // Ensure conversation annotation set event is fired when calling ConversationComponent::SetConversationAnnotation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        std::vector<char> testAnnotationDataV = PngHeader;
        testAnnotationDataV.push_back('1');
        std::vector<char> testAnnotationThumbnailDataV = PngHeader;
        testAnnotationDataV.push_back('2');

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 1.f, 2.f, 3.f };
        data.AuthorCameraRotation = { 4.f, 5.f, 6.f, 7.f };
        data.VerticalFov = 90;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent, SetConversationAnnotation, RequestPredicate, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::SetConversationAnnotation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, conversationMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    // Ensure annotation delete event is fired when calling ConversationComponent::DeleteConversationAnnotation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        auto [Result] = AWAIT_PRE(conversationComponent, DeleteConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteConversationAnnotation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, conversationMessage);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, "");
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    csp::common::String firstMessageId = "";
    static constexpr const char* message = "Test Message";

    // Ensure message created event is fired when calling ConversationComponent::AddMessage
    {
        bool callbackCalled = false;

        const auto callback = [&callbackCalled](const csp::common::ConversationNetworkEventData& /*Params*/) { callbackCalled = true; };

        conversationComponent->SetConversationUpdateCallback(callback);

        const auto [Result] = AWAIT(conversationComponent, AddMessage, message);
        firstMessageId = Result.GetMessageInfo().MessageId;

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);
    }

    // Ensure annotation set event is fired when calling ConversationComponent::SetAnnotation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        std::vector<char> testAnnotationDataV = PngHeader;
        testAnnotationDataV.push_back('1');
        std::vector<char> testAnnotationThumbnailDataV = PngHeader;
        testAnnotationDataV.push_back('2');

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 1.f, 2.f, 3.f };
        data.AuthorCameraRotation = { 4.f, 5.f, 6.f, 7.f };
        data.VerticalFov = 90;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent, SetAnnotation, RequestPredicate, firstMessageId, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::SetAnnotation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, message);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, firstMessageId);
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    // Ensure anntation delete event is fired when calling ConversationComponent::DeleteAnnotation
    {
        csp::common::ConversationNetworkEventData retrievedParams;
        bool callbackCalled = false;

        const auto callback = [&retrievedParams, &callbackCalled](const csp::common::ConversationNetworkEventData& params)
        {
            retrievedParams = params;
            callbackCalled = true;
        };

        conversationComponent->SetConversationUpdateCallback(callback);

        auto [Result] = AWAIT_PRE(conversationComponent, DeleteAnnotation, RequestPredicate, firstMessageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallback(callbackCalled);

        EXPECT_EQ(retrievedParams.MessageType, csp::multiplayer::ConversationEventType::DeleteAnnotation);
        EXPECT_EQ(retrievedParams.MessageInfo.ConversationId, conversationComponent->GetConversationId());
        EXPECT_EQ(retrievedParams.MessageInfo.UserId, userId);
        EXPECT_EQ(retrievedParams.MessageInfo.Message, message);
        EXPECT_EQ(retrievedParams.MessageInfo.MessageId, firstMessageId);
        EXPECT_NE(retrievedParams.MessageInfo.CreatedTimestamp, "");
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests that annotations can be correctly overwritten
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentOverwriteAnnotationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    csp::common::String conversationId;
    csp::common::String messageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
    {
        static constexpr const char* testMessage = "TestConversation";
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    // Ensure callback values are correct when calling ConversationComponent::CreateMessage
    {
        static constexpr const char* testMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();
        messageId = info.MessageId;
    }

    // Create annotation attached to message
    {
        static const csp::common::Vector3 testAuthorCameraPosition { 1.f, 2.f, 3.f };
        static const csp::common::Vector4 testAuthorCameraRotation { 4.f, 5.f, 6.f, 7.f };
        static constexpr const float testFov = 90.f;
        std::vector<char> testAnnotationDataV = PngHeader;
        testAnnotationDataV.push_back('1');
        std::vector<char> testAnnotationThumbnailDataV = PngHeader;
        testAnnotationDataV.push_back('2');

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = testAuthorCameraPosition;
        data.AuthorCameraRotation = testAuthorCameraRotation;
        data.VerticalFov = testFov;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, messageId, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Overwrite annotation
    {
        static const csp::common::Vector3 testAuthorCameraPosition { 8.f, 9.f, 10.f };
        static const csp::common::Vector4 testAuthorCameraRotation { 11.f, 12.f, 13.f, 14.f };
        static constexpr const float testFov = 100.f;
        bool assetOverwriteLogCalled = false;

        std::vector<char> testAnnotationDataV = PngHeader;
        testAnnotationDataV.push_back('3');
        std::vector<char> testAnnotationThumbnailDataV = PngHeader;
        testAnnotationDataV.push_back('4');

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&assetOverwriteLogCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "ConversationSystemInternal::SetAnnotation, asset already exists, so not creating")
                {
                    assetOverwriteLogCalled = true;
                }
            });

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = testAuthorCameraPosition;
        data.AuthorCameraRotation = testAuthorCameraRotation;
        data.VerticalFov = testFov;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, messageId, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        EXPECT_TRUE(assetOverwriteLogCalled);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationDataV, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultThumbnailData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultThumbnailData, resultThumbnailData + strlen(resultThumbnailData));
        EXPECT_EQ(testAnnotationThumbnailDataV, thumbnailDataV);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests annotation functions can't be used before the component has been initialized
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationsPrerequisitesTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    static const csp::common::String noConversationErrorLog = "This component does not have an associated conversation. "
                                                              "Call CreateConversation to create a new conversation for this component";

    // Ensure GetAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotation, RequestPredicate, "");

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetConversationAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetConversationAnnotation, RequestPredicate);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure SetAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = nullptr;
        annotationBufferData.BufferLength = 0;
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = nullptr;
        annotationThumbnailBufferData.BufferLength = 0;
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, "", data, annotationBufferData, annotationThumbnailBufferData);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure SetConversationAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = nullptr;
        annotationBufferData.BufferLength = 0;
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = nullptr;
        annotationThumbnailBufferData.BufferLength = 0;
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent, SetConversationAnnotation, RequestPredicate, data, annotationBufferData, annotationThumbnailBufferData);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure DeleteAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, DeleteAnnotation, RequestPredicate, "");

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure DeleteConversationAnnotation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, DeleteConversationAnnotation, RequestPredicate);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Ensure GetAnnotationThumbnailsForConversation errors and logs appropriately when a conversation hasn't been created
    {
        bool callbackCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&callbackCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                callbackCalled = true;
                EXPECT_EQ(noConversationErrorLog, message);
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests annotations cant be attached to invalid messages
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationInvalidMessageTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    // Create the conversation
    {
        auto [Result] = AWAIT_PRE(conversationComponent, CreateConversation, RequestPredicate, "TestMessage");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Try and get an annotation on an invalid message on a valid conversation
    {
        bool logCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&logCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Failed to get message asset collection.")
                {
                    logCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotation, RequestPredicate, "");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        EXPECT_TRUE(logCalled);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Try and set an annotation on an invalid message on a valid message
    {
        bool logCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&logCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Failed to get message asset collection.")
                {
                    logCalled = true;
                }
            });

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = nullptr;
        annotationBufferData.BufferLength = 0;
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = nullptr;
        annotationThumbnailBufferData.BufferLength = 0;
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, "", data, annotationBufferData, annotationThumbnailBufferData);

        EXPECT_TRUE(logCalled);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests annotations cant be attached to messages of a different conversation
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationIncorrectConversationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    // Create the conversation
    {
        auto [Result] = AWAIT_PRE(conversationComponent, CreateConversation, RequestPredicate, "TestMessage");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object2 = CreateTestObject(realtimeEngine.get(), "Object2");

    // Create conversation component
    auto* conversationComponent2 = static_cast<ConversationSpaceComponent*>(object2->AddComponent(ComponentType::Conversation));

    // Create the second conversation
    {
        auto [Result] = AWAIT_PRE(conversationComponent2, CreateConversation, RequestPredicate, "TestMessage2");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    csp::common::String messageId;

    // Create a message on the first conversation
    {
        static constexpr const char* testMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();
        messageId = info.MessageId;
    }

    // Attempt to add an annotation to the message using the second conversation
    {
        bool logCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&logCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Given message doesn't exist on the conversation.")
                {
                    logCalled = true;
                }
            });

        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = nullptr;
        annotationBufferData.BufferLength = 0;
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = nullptr;
        annotationThumbnailBufferData.BufferLength = 0;
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent2, SetAnnotation, RequestPredicate, messageId, data, annotationBufferData, annotationThumbnailBufferData);

        EXPECT_TRUE(logCalled);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Attempt to get an annotation on the message using the second conversation
    {
        bool logCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&logCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Given message doesn't exist on the conversation.")
                {
                    logCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(conversationComponent2, GetAnnotation, RequestPredicate, messageId);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Attempt to delete an annotation on the message using the second conversation
    {
        bool logCalled = false;

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [&logCalled](csp::common::LogLevel, const csp::common::String& message)
            {
                if (message == "Given message doesn't exist on the conversation.")
                {
                    logCalled = true;
                }
            });

        auto [Result] = AWAIT_PRE(conversationComponent2, DeleteAnnotation, RequestPredicate, messageId);

        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
Tests annotation thumbnails can be retrieved
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentAnnotationThumbnailsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    // Create the conversation
    {
        auto [Result] = AWAIT_PRE(conversationComponent, CreateConversation, RequestPredicate, "TestMessage");
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    csp::common::String messageId;
    csp::common::String messageId2;

    // Create a message on the first conversation
    {
        static constexpr const char* testMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();
        messageId = info.MessageId;
    }

    // Create a message on the first conversation
    {
        static constexpr const char* testMessage = "TestMessage2";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();
        messageId2 = info.MessageId;
    }

    // Try and get thumbnails before any have been added
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetTotalCount(), 0);
    }

    std::vector<char> testAnnotationDataV = PngHeader;
    testAnnotationDataV.push_back('1');

    std::vector<char> testAnnotationData2V = PngHeader;
    testAnnotationData2V.push_back('2');

    std::vector<char> testThumbnailDataV = PngHeader;
    testThumbnailDataV.push_back('3');

    std::vector<char> testThumbnailData2V = PngHeader;
    testThumbnailData2V.push_back('4');

    // Add annotation to the first message
    {
        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, messageId, data, annotationBufferData, annotationThumbnailBufferData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Try and get thumbnails after one has been added
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetTotalCount(), 1);

        // Ensure the data matches
        auto [DownloadAnnotationResult]
            = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAssetsMap()[messageId]);
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* data = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(data, data + strlen(data));
        EXPECT_EQ(testThumbnailDataV, dataV);
    }

    // Add annotation to the second message
    {
        AnnotationUpdateParams data;
        data.AuthorCameraPosition = { 0.f, 0.f, 0.f };
        data.AuthorCameraRotation = { 0.f, 0.f, 0.f, 0.f };
        data.VerticalFov = 0;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationData2V.data();
        annotationBufferData.BufferLength = testAnnotationData2V.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testThumbnailData2V.data();
        annotationThumbnailBufferData.BufferLength = testThumbnailData2V.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent, SetAnnotation, RequestPredicate, messageId2, data, annotationBufferData, annotationThumbnailBufferData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Try and get thumbnails after two have been added
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotationThumbnailsForConversation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetTotalCount(), 2);

        // Ensure the data matches
        auto [DownloadAnnotationResult1]
            = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAssetsMap()[messageId]);
        EXPECT_EQ(DownloadAnnotationResult1.GetResultCode(), csp::systems::EResultCode::Success);

        const char* data = static_cast<const char*>(DownloadAnnotationResult1.GetData());
        std::vector<char> dataV(data, data + strlen(data));
        EXPECT_EQ(testThumbnailDataV, dataV);

        auto [DownloadAnnotationResult2]
            = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAssetsMap()[messageId2]);
        EXPECT_EQ(DownloadAnnotationResult2.GetResultCode(), csp::systems::EResultCode::Success);

        const char* data2 = static_cast<const char*>(DownloadAnnotationResult2.GetData());
        std::vector<char> dataV2(data2, data2 + strlen(data2));
        EXPECT_EQ(testThumbnailData2V, dataV2);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
 * Tests that the annotation for a conversation can be retrieved after updating the conversation.
 */
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentUpdateConversationGetAnnotationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    csp::common::String conversationId;
    csp::common::String messageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
    {
        static constexpr const char* testMessage = "TestConversation";
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    static const csp::common::Vector3 testConversationAuthorCameraPosition { 1.f, 2.f, 3.f };
    static const csp::common::Vector4 testConversationAuthorCameraRotation { 4.f, 5.f, 6.f, 7.f };
    static constexpr const float testConversationFov = 90.f;

    std::vector<char> testAnnotationDataV = PngHeader;
    testAnnotationDataV.push_back('1');
    std::vector<char> testAnnotationThumbnailDataV = PngHeader;
    testAnnotationDataV.push_back('2');

    // Create annotation attached to conversation
    {
        AnnotationUpdateParams data;
        data.AuthorCameraPosition = testConversationAuthorCameraPosition;
        data.AuthorCameraRotation = testConversationAuthorCameraRotation;
        data.VerticalFov = testConversationFov;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationDataV.data();
        annotationBufferData.BufferLength = testAnnotationDataV.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailDataV.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailDataV.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result] = AWAIT_PRE(
            conversationComponent, SetConversationAnnotation, RequestPredicate, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testConversationAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testConversationAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationDataV, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailDataV, thumbnailDataV);
    }

    // Get conversation annotation and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testConversationAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testConversationAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationDataV, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailDataV, thumbnailDataV);
    }

    // Update the conversation
    {
        static constexpr const char* testMessage = "TestConversation2";

        MessageUpdateParams newData;
        newData.NewMessage = testMessage;

        auto [Result] = AWAIT(conversationComponent, UpdateConversation, newData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetConversationInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");

        // Value should now be edited
        EXPECT_TRUE(info.EditedTimestamp != "");
        EXPECT_EQ(info.UserId, userId);

        // Message should be updated with the new value
        EXPECT_EQ(info.Message, testMessage);
        EXPECT_EQ(info.MessageId, "");
    }

    // Get conversation annotation again and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetConversationAnnotation, RequestPredicate);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testConversationAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testConversationAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testConversationFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationDataV, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailDataV, thumbnailDataV);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
 * Tests that the annotation for a message can be retrieved after updating the message.
 */
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentUpdateMessageGetAnnotationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    csp::common::String conversationId;
    csp::common::String messageId;

    // Ensure callback values are correct when calling ConversationComponent::CreateConversation
    {
        static constexpr const char* testMessage = "TestConversation";
        auto [Result] = AWAIT(conversationComponent, CreateConversation, testMessage);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_TRUE(Result.GetValue() != "");

        conversationId = Result.GetValue();
    }

    static const csp::common::Vector3 testConversationAuthorCameraPosition { 1.f, 2.f, 3.f };
    static const csp::common::Vector4 testConversationAuthorCameraRotation { 4.f, 5.f, 6.f, 7.f };
    static constexpr const float testConversationFov = 90.f;

    std::vector<char> testAnnotationDataV = PngHeader;
    testAnnotationDataV.push_back('1');
    std::vector<char> testAnnotationThumbnailDataV = PngHeader;
    testAnnotationDataV.push_back('2');

    // Ensure callback values are correct when calling ConversationComponent::CreateMessage
    {
        static constexpr const char* testMessage = "TestMessage";

        auto [Result] = AWAIT_PRE(conversationComponent, AddMessage, RequestPredicate, testMessage);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();
        messageId = info.MessageId;
    }

    static const csp::common::Vector3 testAuthorCameraPosition { 8.f, 9.f, 10.f };
    static const csp::common::Vector4 testAuthorCameraRotation { 11.f, 12.f, 13.f, 14.f };
    static constexpr const float testFov = 100.f;

    std::vector<char> testAnnotationData2V = PngHeader;
    testAnnotationData2V.push_back('3');
    std::vector<char> testAnnotationThumbnailData2V = PngHeader;
    testAnnotationData2V.push_back('4');

    // Create annotation attached to message
    {
        AnnotationUpdateParams data;
        data.AuthorCameraPosition = testAuthorCameraPosition;
        data.AuthorCameraRotation = testAuthorCameraRotation;
        data.VerticalFov = testFov;

        csp::systems::BufferAssetDataSource annotationBufferData;
        annotationBufferData.Buffer = testAnnotationData2V.data();
        annotationBufferData.BufferLength = testAnnotationData2V.size();
        annotationBufferData.SetMimeType("image/png");

        csp::systems::BufferAssetDataSource annotationThumbnailBufferData;
        annotationThumbnailBufferData.Buffer = testAnnotationThumbnailData2V.data();
        annotationThumbnailBufferData.BufferLength = testAnnotationThumbnailData2V.size();
        annotationThumbnailBufferData.SetMimeType("image/png");

        auto [Result]
            = AWAIT_PRE(conversationComponent, SetAnnotation, RequestPredicate, messageId, data, annotationBufferData, annotationThumbnailBufferData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationData2V, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailDataV(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailData2V, thumbnailDataV);
    }

    // Get annotation and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotation, RequestPredicate, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationData2V, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailData2V(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailData2V, thumbnailData2V);
    }

    // Ensure callback values are correct when calling ConversationComponent::UpdateMessage
    {
        static constexpr const char* testMessage = "TestMessage2";

        MessageUpdateParams newData;
        newData.NewMessage = testMessage;

        auto [Result] = AWAIT(conversationComponent, UpdateMessage, messageId, newData);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::MessageInfo& info = Result.GetMessageInfo();

        EXPECT_EQ(info.ConversationId, conversationId);
        EXPECT_TRUE(info.CreatedTimestamp != "");
        // Value should now be edited
        EXPECT_TRUE(info.EditedTimestamp != "");
        EXPECT_EQ(info.UserId, userId);
        // Message should be updated with the new value
        EXPECT_EQ(info.Message, testMessage);
        EXPECT_EQ(info.MessageId, messageId);
    }

    // Get annotation again and ensure data still matches
    {
        auto [Result] = AWAIT_PRE(conversationComponent, GetAnnotation, RequestPredicate, messageId);
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const csp::multiplayer::AnnotationData& retrievedData = Result.GetAnnotationData();

        EXPECT_TRUE((retrievedData.AuthorCameraPosition == testAuthorCameraPosition));
        EXPECT_TRUE((retrievedData.AuthorCameraRotation == testAuthorCameraRotation));
        EXPECT_EQ(retrievedData.VerticalFov, testFov);

        auto [DownloadAnnotationResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationAsset());
        EXPECT_EQ(DownloadAnnotationResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultData = static_cast<const char*>(DownloadAnnotationResult.GetData());
        std::vector<char> dataV(resultData, resultData + strlen(resultData));
        EXPECT_EQ(testAnnotationData2V, dataV);

        auto [DownloadAnnotationThumbnailResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, Result.GetAnnotationThumbnailAsset());
        EXPECT_EQ(DownloadAnnotationThumbnailResult.GetResultCode(), csp::systems::EResultCode::Success);

        const char* resultAnnotationData = static_cast<const char*>(DownloadAnnotationThumbnailResult.GetData());
        std::vector<char> thumbnailData2V(resultAnnotationData, resultAnnotationData + strlen(resultAnnotationData));
        EXPECT_EQ(testAnnotationThumbnailData2V, thumbnailData2V);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

/*
    Tests that when using an offline engine, ConversationComponents, which are dependent on the network
    event bus, don't crash, and at least emit somewhat helpful errors.
    Conversations being dependent on the NetworkEventBus like this is another unfortunate exception
    to them being "systems", they're probably better expressed as multiplayer components.
*/
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentCreateConversationOfflineTest)
{
    // We can't really test more than this one because other conversation functions are guarded by a valid
    // conversation ID, which isn't possible to make without a SignalR connection.
    // HOWEVER, if we are to enable loading conversations via a scene description, then we must test attempting
    // to update it without a connection, as we're essentially creating read-only functionality.

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    systemsManager.GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Error);

    // Login without a signalR connection
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, false, true);

    std::unique_ptr<csp::multiplayer::OfflineRealtimeEngine> realtimeEngine { systemsManager.MakeOfflineRealtimeEngine() };

    // Create object to hold component
    csp::multiplayer::SpaceEntity* object = CreateTestObject(realtimeEngine.get());

    // Create conversation component
    auto* conversationComponent = static_cast<ConversationSpaceComponent*>(object->AddComponent(ComponentType::Conversation));

    RAIIMockLogger mockLogger {};
    const csp::common::String lockErrorMsg = "Create Conversation: SignalR connection error: NotConnected";
    const csp::common::String disconnectErrorMsg = "Error disconnecting MultiplayerConnection: NotConnected";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, lockErrorMsg)).Times(1);
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, disconnectErrorMsg)).Times(1);

    static constexpr const char* testMessage = "TestConversation";
    auto [Result] = AWAIT(conversationComponent, CreateConversation, testMessage);

    // Log out
    LogOut(userSystem);
}
