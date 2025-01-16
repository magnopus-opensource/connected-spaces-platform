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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

#if RUN_ALL_UNIT_TESTS || RUN_TEXT_TESTS || RUN_TEXT_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, TextTests, TextComponentTest)
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Create object to represent the text
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create text component
    auto* TextComponent = static_cast<TextSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Text));

    // Ensure defaults are set
    EXPECT_EQ(TextComponent->GetPosition().X, 0.0f);
    EXPECT_EQ(TextComponent->GetPosition().Y, 0.0f);
    EXPECT_EQ(TextComponent->GetPosition().Z, 0.0f);
    EXPECT_EQ(TextComponent->GetBackgroundColor().X, 0.0f);
    EXPECT_EQ(TextComponent->GetBackgroundColor().Y, 0.0f);
    EXPECT_EQ(TextComponent->GetBackgroundColor().Z, 0.0f);
    EXPECT_EQ(TextComponent->GetBillboardMode(), BillboardMode::Off);
    EXPECT_EQ(TextComponent->GetComponentType(), ComponentType::Text);
    EXPECT_EQ(TextComponent->GetHeight(), 1.0f);
    EXPECT_EQ(TextComponent->GetIsARVisible(), true);
    EXPECT_EQ(TextComponent->GetIsVisible(), true);
    EXPECT_EQ(TextComponent->GetRotation().W, 1);
    EXPECT_EQ(TextComponent->GetRotation().X, 0);
    EXPECT_EQ(TextComponent->GetRotation().Y, 0);
    EXPECT_EQ(TextComponent->GetRotation().Z, 0);
    EXPECT_EQ(TextComponent->GetText(), "");
    EXPECT_EQ(TextComponent->GetTextColor().X, 1.0f);
    EXPECT_EQ(TextComponent->GetTextColor().Y, 1.0f);
    EXPECT_EQ(TextComponent->GetTextColor().Z, 1.0f);
    EXPECT_EQ(TextComponent->GetScale().X, 1.0f);
    EXPECT_EQ(TextComponent->GetScale().Y, 1.0f);
    EXPECT_EQ(TextComponent->GetScale().Z, 1.0f);
    EXPECT_EQ(TextComponent->GetWidth(), 1.0f);

    // Set new values

    TextComponent->SetPosition(csp::common::Vector3::One());
    TextComponent->SetHeight(2.0f);
    TextComponent->SetWidth(2.0f);
    TextComponent->SetBillboardMode(BillboardMode::YawLockedBillboard);
    TextComponent->SetIsARVisible(false);
    TextComponent->SetIsVisible(false);
    TextComponent->SetBackgroundColor(csp::common::Vector3::One());
    TextComponent->SetTextColor(csp::common::Vector3::Zero());
    TextComponent->SetPosition(csp::common::Vector3::One());
    TextComponent->SetRotation(csp::common::Vector4 { 1, 1, 1, 1 });
    TextComponent->SetText("Text");
    TextComponent->SetScale(csp::common::Vector3 { 2.0f, 2.0f, 2.0f });
    TextComponent->SetIsBackgroundVisible(false);

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(TextComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetPosition().Z, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetBackgroundColor().X, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetBackgroundColor().Y, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetBackgroundColor().Z, 1.0f);
    EXPECT_EQ(TextComponent->GetBillboardMode(), BillboardMode::YawLockedBillboard);
    EXPECT_FLOAT_EQ(TextComponent->GetHeight(), 2.0f);
    EXPECT_EQ(TextComponent->GetIsARVisible(), false);
    EXPECT_EQ(TextComponent->GetIsVisible(), false);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(TextComponent->GetText(), "Text");
    EXPECT_FLOAT_EQ(TextComponent->GetTextColor().X, 0.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetTextColor().Y, 0.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetTextColor().Z, 0.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetScale().X, 2.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetScale().Y, 2.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetScale().Z, 2.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetWidth(), 2.0f);
    EXPECT_EQ(TextComponent->GetIsBackgroundVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_TEXT_TESTS || RUN_TEXT_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, TextTests, TextSpaceComponentScriptInterfaceTest)
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

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    // Create object to represent the text
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create text component
    auto* TextComponent = (TextSpaceComponent*)CreatedObject->AddComponent(ComponentType::Text);
    // Create script component
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);
    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Setup script
    std::string TextScriptText = R"xx(
	
		const assetId			= "TEST_ASSET_ID";
		const assetCollectionId = "TEST_COLLECTION_ID";

		var text = ThisEntity.getTextComponents()[0];
		text.position = [1.0,1.0,1.0];
		text.height = 2.0;
		text.width = 2.0;
		text.billboardMode = 2;
		text.isARVisible = false;
		text.isVisible = false;
		text.backgroundColor = [1.0,1.0,1.0];
		text.textColor = [0.0,0.0,0.0];
		text.rotation = [1.0, 1.0, 1.0, 1.0];
		text.text = "Text";
		text.scale = [2.0, 2.0, 2.0];
		text.isBackgroundVisible = false;
    )xx";

    ScriptComponent->SetScriptSource(TextScriptText.c_str());
    CreatedObject->GetScript()->Invoke();
    const bool ScriptHasErrors = CreatedObject->GetScript()->HasError();
    EXPECT_FALSE(ScriptHasErrors);
    EntitySystem->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_FLOAT_EQ(TextComponent->GetPosition().X, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetPosition().Y, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetPosition().Z, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetBackgroundColor().X, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetBackgroundColor().Y, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetBackgroundColor().Z, 1.0f);
    EXPECT_EQ(TextComponent->GetBillboardMode(), BillboardMode::YawLockedBillboard);
    EXPECT_FLOAT_EQ(TextComponent->GetHeight(), 2.0f);
    EXPECT_EQ(TextComponent->GetIsARVisible(), false);
    EXPECT_EQ(TextComponent->GetIsVisible(), false);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().W, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().X, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().Y, 1.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetRotation().Z, 1.0f);
    EXPECT_EQ(TextComponent->GetText(), "Text");
    EXPECT_FLOAT_EQ(TextComponent->GetTextColor().X, 0.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetTextColor().Y, 0.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetTextColor().Z, 0.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetScale().X, 2.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetScale().Y, 2.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetScale().Z, 2.0f);
    EXPECT_FLOAT_EQ(TextComponent->GetWidth(), 2.0f);
    EXPECT_EQ(TextComponent->GetIsBackgroundVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace