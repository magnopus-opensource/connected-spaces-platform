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
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, TextTests, TextComponentTest)
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

    // Create object to represent the text
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create text component
    auto* textComponent = static_cast<TextSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Text));

    // Ensure defaults are set
    EXPECT_EQ(textComponent->GetPosition().X, 0.0f);
    EXPECT_EQ(textComponent->GetPosition().Y, 0.0f);
    EXPECT_EQ(textComponent->GetPosition().Z, 0.0f);
    EXPECT_EQ(textComponent->GetBackgroundColor().X, 0.0f);
    EXPECT_EQ(textComponent->GetBackgroundColor().Y, 0.0f);
    EXPECT_EQ(textComponent->GetBackgroundColor().Z, 0.0f);
    EXPECT_EQ(textComponent->GetBillboardMode(), BillboardMode::Off);
    EXPECT_EQ(textComponent->GetComponentType(), ComponentType::Text);
    EXPECT_EQ(textComponent->GetHeight(), 1.0f);
    EXPECT_EQ(textComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(textComponent->GetIsARVisible(), true);
    EXPECT_EQ(textComponent->GetIsVisible(), true);
    EXPECT_EQ(textComponent->GetRotation().W, 1);
    EXPECT_EQ(textComponent->GetRotation().X, 0);
    EXPECT_EQ(textComponent->GetRotation().Y, 0);
    EXPECT_EQ(textComponent->GetRotation().Z, 0);
    EXPECT_EQ(textComponent->GetText(), "");
    EXPECT_EQ(textComponent->GetTextColor().X, 1.0f);
    EXPECT_EQ(textComponent->GetTextColor().Y, 1.0f);
    EXPECT_EQ(textComponent->GetTextColor().Z, 1.0f);
    EXPECT_EQ(textComponent->GetScale().X, 1.0f);
    EXPECT_EQ(textComponent->GetScale().Y, 1.0f);
    EXPECT_EQ(textComponent->GetScale().Z, 1.0f);
    EXPECT_EQ(textComponent->GetWidth(), 1.0f);

    // Set new values
    textComponent->SetPosition(csp::common::Vector3::One());
    textComponent->SetHeight(2.0f);
    textComponent->SetWidth(2.0f);
    textComponent->SetBillboardMode(BillboardMode::YawLockedBillboard);
    textComponent->SetIsVirtualVisible(false);
    textComponent->SetIsARVisible(false);
    textComponent->SetIsVisible(false);
    textComponent->SetBackgroundColor(csp::common::Vector3::One());
    textComponent->SetTextColor(csp::common::Vector3::Zero());
    textComponent->SetPosition(csp::common::Vector3::One());
    textComponent->SetRotation(csp::common::Vector4 { 1, 1, 1, 1 });
    textComponent->SetText("Text");
    textComponent->SetScale(csp::common::Vector3 { 2.0f, 2.0f, 2.0f });
    textComponent->SetIsBackgroundVisible(false);

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(textComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetPosition().Z, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetBackgroundColor().X, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetBackgroundColor().Y, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetBackgroundColor().Z, 1.0f);
    EXPECT_EQ(textComponent->GetBillboardMode(), BillboardMode::YawLockedBillboard);
    EXPECT_FLOAT_EQ(textComponent->GetHeight(), 2.0f);
    EXPECT_EQ(textComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(textComponent->GetIsARVisible(), false);
    EXPECT_EQ(textComponent->GetIsVisible(), false);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(textComponent->GetText(), "Text");
    EXPECT_FLOAT_EQ(textComponent->GetTextColor().X, 0.0f);
    EXPECT_FLOAT_EQ(textComponent->GetTextColor().Y, 0.0f);
    EXPECT_FLOAT_EQ(textComponent->GetTextColor().Z, 0.0f);
    EXPECT_FLOAT_EQ(textComponent->GetScale().X, 2.0f);
    EXPECT_FLOAT_EQ(textComponent->GetScale().Y, 2.0f);
    EXPECT_FLOAT_EQ(textComponent->GetScale().Z, 2.0f);
    EXPECT_FLOAT_EQ(textComponent->GetWidth(), 2.0f);
    EXPECT_EQ(textComponent->GetIsBackgroundVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, TextTests, TextSpaceComponentScriptInterfaceTest)
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

    // Create object to represent the text
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create text component
    auto* textComponent = (TextSpaceComponent*)CreatedObject->AddComponent(ComponentType::Text);
    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);
    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string textScriptText = R"xx(
	
		const assetId			= "TEST_ASSET_ID";
		const assetCollectionId = "TEST_COLLECTION_ID";

		var text = ThisEntity.getTextComponents()[0];
        text.text = "Text";
		text.position = [1.0,1.0,1.0];
        text.scale = [2.0, 2.0, 2.0];
        text.rotation = [1.0, 1.0, 1.0, 1.0];
        text.textColor = [0.0,0.0,0.0];
        text.backgroundColor = [1.0,1.0,1.0];
        text.isBackgroundVisible = false;
        text.width = 2.0;
		text.height = 2.0;
		text.billboardMode = 2;
		text.isVisible = false;
		text.isARVisible = false;
		text.isVirtualVisible = false;

    )xx";

    scriptComponent->SetScriptSource(textScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);
    realtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_EQ(textComponent->GetText(), "Text");

    EXPECT_FLOAT_EQ(textComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetPosition().Z, 1.0f);

    EXPECT_FLOAT_EQ(textComponent->GetScale().X, 2.0f);
    EXPECT_FLOAT_EQ(textComponent->GetScale().Y, 2.0f);
    EXPECT_FLOAT_EQ(textComponent->GetScale().Z, 2.0f);

    EXPECT_FLOAT_EQ(textComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetRotation().Z, 1.0f);

    EXPECT_FLOAT_EQ(textComponent->GetTextColor().X, 0.0f);
    EXPECT_FLOAT_EQ(textComponent->GetTextColor().Y, 0.0f);
    EXPECT_FLOAT_EQ(textComponent->GetTextColor().Z, 0.0f);

    EXPECT_FLOAT_EQ(textComponent->GetBackgroundColor().X, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetBackgroundColor().Y, 1.0f);
    EXPECT_FLOAT_EQ(textComponent->GetBackgroundColor().Z, 1.0f);

    EXPECT_EQ(textComponent->GetIsBackgroundVisible(), false);

    EXPECT_FLOAT_EQ(textComponent->GetWidth(), 2.0f);

    EXPECT_FLOAT_EQ(textComponent->GetHeight(), 2.0f);

    EXPECT_EQ(textComponent->GetBillboardMode(), BillboardMode::YawLockedBillboard);

    EXPECT_EQ(textComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(textComponent->GetIsARVisible(), false);
    EXPECT_EQ(textComponent->GetIsVisible(), false);

    EXPECT_EQ(textComponent->GetIsBackgroundVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}