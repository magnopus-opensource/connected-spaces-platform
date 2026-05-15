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
#include "CSP/Multiplayer/Components/ExternalLinkSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <filesystem>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, LinkTests, ExternalLinkComponentTest)
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

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create custom component
        auto* externalLinkComponent = (ExternalLinkSpaceComponent*)CreatedObject->AddComponent(ComponentType::ExternalLink);

        const csp::common::String externalLinkName = "MyExternalLink";
        externalLinkComponent->SetName(externalLinkName);

        EXPECT_EQ(externalLinkComponent->GetName(), externalLinkName);

        const csp::common::String externalLinkUrl = "https://oko.live";
        externalLinkComponent->SetLinkUrl(externalLinkUrl);

        EXPECT_EQ(externalLinkComponent->GetLinkUrl(), externalLinkUrl);

        const csp::common::Vector3 position(123.0f, 456.0f, 789.0f);
        externalLinkComponent->SetPosition(position);

        EXPECT_EQ(externalLinkComponent->GetPosition(), position);

        const csp::common::Vector4 rotation(1.0f, 2.0f, 3.0f, 4.0f);
        externalLinkComponent->SetRotation(rotation);

        EXPECT_EQ(externalLinkComponent->GetRotation(), rotation);

        const csp::common::Vector3 scale(123.0f, 456.0f, 789.0f);
        externalLinkComponent->SetScale(scale);

        EXPECT_EQ(externalLinkComponent->GetScale(), scale);

        const csp::common::String displayText = "A great link";
        externalLinkComponent->SetDisplayText(displayText);

        EXPECT_EQ(externalLinkComponent->GetDisplayText(), displayText);

        bool isEnabled = false;
        externalLinkComponent->SetIsEnabled(isEnabled);

        EXPECT_EQ(externalLinkComponent->GetIsEnabled(), isEnabled);

        bool isVisible = false;
        externalLinkComponent->SetIsVisible(isVisible);

        EXPECT_EQ(externalLinkComponent->GetIsVisible(), isVisible);

        bool isArVisible = false;
        externalLinkComponent->SetIsARVisible(isArVisible);

        EXPECT_EQ(externalLinkComponent->GetIsARVisible(), isArVisible);

        bool isVirtualVisible = false;
        externalLinkComponent->SetIsVirtualVisible(isVirtualVisible);

        EXPECT_EQ(externalLinkComponent->GetIsVirtualVisible(), isVirtualVisible);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LinkTests, ExternalLinkScriptInterfaceTest)
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

    // Create parent entity
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create external link component
    auto* linkComponent = (ExternalLinkSpaceComponent*)CreatedObject->AddComponent(ComponentType::ExternalLink);

    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(linkComponent->GetName(), "");
    EXPECT_EQ(linkComponent->GetLinkUrl(), "");
    EXPECT_EQ(linkComponent->GetDisplayText(), "");
    EXPECT_EQ(linkComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(linkComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(linkComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(linkComponent->GetIsEnabled(), true);
    EXPECT_EQ(linkComponent->GetIsVisible(), true);
    EXPECT_EQ(linkComponent->GetIsARVisible(), true);
    EXPECT_EQ(linkComponent->GetIsVirtualVisible(), true);

    // Setup script
    const std::string externalLinkScriptText = R"xx(

		var link = ThisEntity.getExternalLinkComponents()[0];

        link.name = "TestName";
        link.linkUrl = "http://youtube.com/avideo";
        link.displayText = "TestDisplayText";
        link.position = [1, 1, 1];
        link.scale = [2, 2, 2];
		link.rotation = [1, 1, 1, 1];
        link.isEnabled = false;
		link.isVisible = false;
        link.isARVisible = false;
        link.isVirtualVisible = false;

    )xx";

    scriptComponent->SetScriptSource(externalLinkScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);

    EXPECT_EQ(linkComponent->GetName(), "TestName");
    EXPECT_EQ(linkComponent->GetLinkUrl(), "http://youtube.com/avideo");
    EXPECT_EQ(linkComponent->GetDisplayText(), "TestDisplayText");
    EXPECT_EQ(linkComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(linkComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(linkComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(linkComponent->GetIsEnabled(), false);
    EXPECT_EQ(linkComponent->GetIsVisible(), false);
    EXPECT_EQ(linkComponent->GetIsARVisible(), false);
    EXPECT_EQ(linkComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}