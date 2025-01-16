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
    const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

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
        EXPECT_EQ(ConversationComponent->GetDate(), "");
        EXPECT_EQ(ConversationComponent->GetNumberOfReplies(), 0);

        ConversationComponent->SetTitle("TestTitle");
        ConversationComponent->SetDate("02-01-1972");
        ConversationComponent->SetNumberOfReplies(2);

        EXPECT_EQ(ConversationComponent->GetTitle(), "TestTitle");
        EXPECT_EQ(ConversationComponent->GetDate(), "02-01-1972");
        EXPECT_EQ(ConversationComponent->GetNumberOfReplies(), 2);

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

            MessageId = Result.GetMessageInfo().Id;

            EXPECT_EQ(Result.GetMessageInfo().Edited, false);
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetMessageInfo, MessageId);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetMessageInfo().Edited, false);
        }

        {
            MessageInfo NewData;
            NewData.Message = "NewTest";
            auto [Result] = AWAIT(ConversationComponent, SetMessageInfo, MessageId, NewData);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetMessageInfo().Edited, true);
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserID, UserId);
            EXPECT_EQ(Result.GetConversationInfo().UserDisplayName, UserDisplayName);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
            EXPECT_FALSE(Result.GetConversationInfo().Edited);
            EXPECT_FALSE(Result.GetConversationInfo().Resolved);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.X, DefaultTransform.Position.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Y, DefaultTransform.Position.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Z, DefaultTransform.Position.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.W, DefaultTransform.Rotation.W);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.X, DefaultTransform.Rotation.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Y, DefaultTransform.Rotation.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Z, DefaultTransform.Rotation.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.X, DefaultTransform.Scale.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Y, DefaultTransform.Scale.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Z, DefaultTransform.Scale.Z);
        }

        {
            ConversationInfo NewData;
            SpaceTransform CameraTransformValue(csp::common::Vector3().One(), csp::common::Vector4().One(), csp::common::Vector3().One());
            NewData.Resolved = true;
            NewData.CameraPosition = CameraTransformValue;
            NewData.Message = "TestMessage1";

            auto [Result] = AWAIT(ConversationComponent, SetConversationInfo, NewData);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserID, UserId);
            EXPECT_EQ(Result.GetConversationInfo().UserDisplayName, UserDisplayName);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage1");
            EXPECT_TRUE(Result.GetConversationInfo().Edited);
            EXPECT_TRUE(Result.GetConversationInfo().Resolved);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.X, CameraTransformValue.Position.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Y, CameraTransformValue.Position.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Z, CameraTransformValue.Position.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.W, CameraTransformValue.Rotation.W);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.X, CameraTransformValue.Rotation.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Y, CameraTransformValue.Rotation.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Z, CameraTransformValue.Rotation.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.X, CameraTransformValue.Scale.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Y, CameraTransformValue.Scale.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Z, CameraTransformValue.Scale.Z);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage1");
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

            MessageId = Result.GetMessageInfo().Id;
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetAllMessages);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetTotalCount(), 2);

            EXPECT_EQ(Result.GetMessages()[0].Id, MessageId);
        }

        {
            auto [Result] = AWAIT(ConversationComponent, GetMessage, MessageId);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetMessageInfo().Id, MessageId);
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

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATION_TESTS || RUN_CONVERSATION_COMPONENT_MOVE_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationTests, ConversationComponentMoveTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    const char* TestSpaceName2 = "OLY-UNITTEST-SPACE-REWIND-2";
    const char* TestSpaceDescription2 = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);
    const auto UserDisplayName = GetFullProfileByUserId(UserSystem, UserId).DisplayName;

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

        csp::common::String ObjectName1 = "Object 1";
        csp::common::String ObjectName2 = "Object 2";

        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

        auto [CreatedObject1] = AWAIT(EntitySystem, CreateObject, ObjectName1, ObjectTransform);
        auto [CreatedObject2] = AWAIT(EntitySystem, CreateObject, ObjectName2, ObjectTransform);

        // Create conversation component
        auto* ConversationComponent1 = (ConversationSpaceComponent*)CreatedObject1->AddComponent(ComponentType::Conversation);
        auto* ConversationComponent2 = (ConversationSpaceComponent*)CreatedObject2->AddComponent(ComponentType::Conversation);

        csp::common::String ConversationId;
        csp::common::String MessageId;

        {
            auto [Result] = AWAIT(ConversationComponent1, CreateConversation, "TestMessage");

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_TRUE(Result.GetValue() != "");

            ConversationId = Result.GetValue();
        }

        SpaceTransform DefaultTransform = SpaceTransform();

        {
            auto [Result] = AWAIT(ConversationComponent1, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserID, UserId);
            EXPECT_EQ(Result.GetConversationInfo().UserDisplayName, UserDisplayName);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
            EXPECT_FALSE(Result.GetConversationInfo().Edited);
            EXPECT_FALSE(Result.GetConversationInfo().Resolved);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.X, DefaultTransform.Position.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Y, DefaultTransform.Position.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Z, DefaultTransform.Position.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.W, DefaultTransform.Rotation.W);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.X, DefaultTransform.Rotation.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Y, DefaultTransform.Rotation.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Z, DefaultTransform.Rotation.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.X, DefaultTransform.Scale.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Y, DefaultTransform.Scale.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Z, DefaultTransform.Scale.Z);
        }

        {
            auto [Result] = AWAIT(ConversationComponent2, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        }

        {
            auto Result = ConversationComponent2->MoveConversationFromComponent(*ConversationComponent1);

            EXPECT_TRUE(Result);
        }

        {
            auto [Result] = AWAIT(ConversationComponent1, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        }

        {
            auto [Result] = AWAIT(ConversationComponent2, GetConversationInfo);

            EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
            EXPECT_EQ(Result.GetConversationInfo().UserID, UserId);
            EXPECT_EQ(Result.GetConversationInfo().UserDisplayName, UserDisplayName);
            EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
            EXPECT_FALSE(Result.GetConversationInfo().Edited);
            EXPECT_FALSE(Result.GetConversationInfo().Resolved);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.X, DefaultTransform.Position.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Y, DefaultTransform.Position.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Position.Z, DefaultTransform.Position.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.W, DefaultTransform.Rotation.W);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.X, DefaultTransform.Rotation.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Y, DefaultTransform.Rotation.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Rotation.Z, DefaultTransform.Rotation.Z);

            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.X, DefaultTransform.Scale.X);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Y, DefaultTransform.Scale.Y);
            EXPECT_EQ(Result.GetConversationInfo().CameraPosition.Scale.Z, DefaultTransform.Scale.Z);
        }

        {
            auto [Result] = AWAIT(ConversationComponent2, DeleteConversation);

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

} // namespace