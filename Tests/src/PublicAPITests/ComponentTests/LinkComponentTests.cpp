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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, LinkTests, ExternalLinkComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        // Create custom component
        auto* ExternalLinkComponent = (ExternalLinkSpaceComponent*)CreatedObject->AddComponent(ComponentType::ExternalLink);

        const csp::common::String ExternalLinkName = "MyExternalLink";
        ExternalLinkComponent->SetName(ExternalLinkName);

        EXPECT_EQ(ExternalLinkComponent->GetName(), ExternalLinkName);

        const csp::common::String ExternalLinkUrl = "https://oko.live";
        ExternalLinkComponent->SetLinkUrl(ExternalLinkUrl);

        EXPECT_EQ(ExternalLinkComponent->GetLinkUrl(), ExternalLinkUrl);

        const csp::common::Vector3 Position(123.0f, 456.0f, 789.0f);
        ExternalLinkComponent->SetPosition(Position);

        EXPECT_EQ(ExternalLinkComponent->GetPosition(), Position);

        const csp::common::Vector4 Rotation(1.0f, 2.0f, 3.0f, 4.0f);
        ExternalLinkComponent->SetRotation(Rotation);

        EXPECT_EQ(ExternalLinkComponent->GetRotation(), Rotation);

        const csp::common::Vector3 Scale(123.0f, 456.0f, 789.0f);
        ExternalLinkComponent->SetScale(Scale);

        EXPECT_EQ(ExternalLinkComponent->GetScale(), Scale);

        const csp::common::String DisplayText = "A great link";
        ExternalLinkComponent->SetDisplayText(DisplayText);

        EXPECT_EQ(ExternalLinkComponent->GetDisplayText(), DisplayText);

        bool IsEnabled = false;
        ExternalLinkComponent->SetIsEnabled(IsEnabled);

        EXPECT_EQ(ExternalLinkComponent->GetIsEnabled(), IsEnabled);

        bool IsVisible = false;
        ExternalLinkComponent->SetIsVisible(IsVisible);

        EXPECT_EQ(ExternalLinkComponent->GetIsVisible(), IsVisible);

        bool IsARVisible = false;
        ExternalLinkComponent->SetIsARVisible(IsARVisible);

        EXPECT_EQ(ExternalLinkComponent->GetIsARVisible(), IsARVisible);

        bool IsVirtualVisible = false;
        ExternalLinkComponent->SetIsVirtualVisible(IsVirtualVisible);

        EXPECT_EQ(ExternalLinkComponent->GetIsVirtualVisible(), IsVirtualVisible);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LinkTests, ExternalLinkScriptInterfaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create external link component
    auto* LinkComponent = (ExternalLinkSpaceComponent*)CreatedObject->AddComponent(ComponentType::ExternalLink);

    // Create script component
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(LinkComponent->GetName(), "");
    EXPECT_EQ(LinkComponent->GetLinkUrl(), "");
    EXPECT_EQ(LinkComponent->GetDisplayText(), "");
    EXPECT_EQ(LinkComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(LinkComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(LinkComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(LinkComponent->GetIsEnabled(), true);
    EXPECT_EQ(LinkComponent->GetIsVisible(), true);
    EXPECT_EQ(LinkComponent->GetIsARVisible(), true);
    EXPECT_EQ(LinkComponent->GetIsVirtualVisible(), true);

    // Setup script
    const std::string ExternalLinkScriptText = R"xx(

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

    ScriptComponent->SetScriptSource(ExternalLinkScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    const bool ScriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(ScriptHasErrors);

    EXPECT_EQ(LinkComponent->GetName(), "TestName");
    EXPECT_EQ(LinkComponent->GetLinkUrl(), "http://youtube.com/avideo");
    EXPECT_EQ(LinkComponent->GetDisplayText(), "TestDisplayText");
    EXPECT_EQ(LinkComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(LinkComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(LinkComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(LinkComponent->GetIsEnabled(), false);
    EXPECT_EQ(LinkComponent->GetIsVisible(), false);
    EXPECT_EQ(LinkComponent->GetIsARVisible(), false);
    EXPECT_EQ(LinkComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}