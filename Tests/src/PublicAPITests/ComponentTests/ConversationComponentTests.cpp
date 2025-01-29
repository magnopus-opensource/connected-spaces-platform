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

#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

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
            MessageInfo NewData;
            NewData.Message = "NewTest";
            auto [Result] = AWAIT(ConversationComponent, SetMessageInfo, MessageId, NewData);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
            EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
        }

        {
            MessageInfo NewData;
            SpaceTransform CameraTransformValue(csp::common::Vector3().One(), csp::common::Vector4().One(), csp::common::Vector3().One());
            NewData.IsConversation = true;
            NewData.Message = "TestMessage1";

            auto [Result] = AWAIT(ConversationComponent, SetConversationInfo, NewData);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage1");
            EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
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

} // namespace