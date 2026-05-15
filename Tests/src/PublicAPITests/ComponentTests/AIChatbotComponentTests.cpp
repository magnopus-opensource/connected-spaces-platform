/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Multiplayer/Components/AIChatbotComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, AIChatbotTests, AIChatbotSpaceComponentTest)
{
    SetRandSeed();

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

    // Create parent Space Entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create ai chatbot component
    auto* aiChatbotComponent = static_cast<AIChatbotSpaceComponent*>(CreatedObject->AddComponent(ComponentType::AIChatbot));

    // Ensure defaults are set
    EXPECT_EQ(aiChatbotComponent->GetPosition(), csp::common::Vector3::Zero());

    EXPECT_EQ(aiChatbotComponent->GetVoice(), "");

    EXPECT_EQ(aiChatbotComponent->GetGuardrailAssetCollectionId(), "");

    EXPECT_EQ(aiChatbotComponent->GetVisualState(), AIChatbotVisualState::Waiting);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Set new values
    csp::common::String voice = "Zephyr";
    csp::common::String guardrailAssetCollectionId = "TEST_GUARDRAIL_ASSET_COLLECTION_ID";

    aiChatbotComponent->SetPosition(csp::common::Vector3::One());

    aiChatbotComponent->SetVoice(voice);

    aiChatbotComponent->SetGuardrailAssetCollectionId(guardrailAssetCollectionId);

    aiChatbotComponent->SetVisualState(AIChatbotVisualState::Listening);

    // Ensure values are set correctly
    EXPECT_EQ(aiChatbotComponent->GetPosition(), csp::common::Vector3::One());

    EXPECT_EQ(aiChatbotComponent->GetGuardrailAssetCollectionId(), guardrailAssetCollectionId);

    EXPECT_EQ(aiChatbotComponent->GetVoice(), voice);

    EXPECT_EQ(aiChatbotComponent->GetVisualState(), AIChatbotVisualState::Listening);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AIChatbotTests, AIChatbotSpaceComponentScriptTest)
{
    SetRandSeed();

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

    // Create parent Space Entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create ai chatbot component
    auto* aiChatbotComponent = (AIChatbotSpaceComponent*)CreatedObject->AddComponent(ComponentType::AIChatbot);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string screenSharingScriptText = R"xx(
		var component = ThisEntity.getAIChatbotComponents()[0];

		component.position = [1, 1, 1];
        component.voice = "Zephyr";
        component.guardrailAssetCollectionId = "TEST_GUARDRAIL_ASSET_COLLECTION_ID";
        component.visualState = 1;
    )xx";

    CreatedObject->GetScript().SetScriptSource(screenSharingScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_EQ(aiChatbotComponent->GetPosition(), csp::common::Vector3::One());

    EXPECT_EQ(aiChatbotComponent->GetVoice(), "Zephyr");

    EXPECT_EQ(aiChatbotComponent->GetGuardrailAssetCollectionId(), "TEST_GUARDRAIL_ASSET_COLLECTION_ID");

    EXPECT_EQ(aiChatbotComponent->GetVisualState(), AIChatbotVisualState::Listening);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
