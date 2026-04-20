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
#include "CSP/Multiplayer/Components/AttachmentSpaceComponent.h"
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

} // namespace

CSP_PUBLIC_TEST(CSPEngine, AttachmentTests, AttachmentComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        auto* AttachmentComponent = (AttachmentSpaceComponent*)CreatedObject->AddComponent(ComponentType::Attachment);

        EXPECT_EQ(AttachmentComponent->GetAnchorPath(), "");

        constexpr const char* TestAnchorPath = "/xr/left-hand";
        AttachmentComponent->SetAnchorPath(TestAnchorPath);

        EXPECT_EQ(AttachmentComponent->GetAnchorPath(), TestAnchorPath);

        AttachmentComponent->SetAnchorPath("/xr/right-hand");
        EXPECT_EQ(AttachmentComponent->GetAnchorPath(), "/xr/right-hand");

        AttachmentComponent->SetAnchorPath("");
        EXPECT_EQ(AttachmentComponent->GetAnchorPath(), "");

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AttachmentTests, AttachmentScriptInterfaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    const std::string ScriptText = R"xx(
        var attachment = ThisEntity.addAttachmentComponent();
        attachment.anchorPath = "/xr/left-hand";
    )xx";

    CreatedObject->GetScript().SetScriptSource(ScriptText.c_str());
    const bool ScriptInvoked = CreatedObject->GetScript().Invoke();

    EXPECT_TRUE(ScriptInvoked);
    EXPECT_FALSE(CreatedObject->GetScript().HasError());

    RealtimeEngine->ProcessPendingEntityOperations();

    auto* AttachmentComponent = (AttachmentSpaceComponent*)CreatedObject->FindFirstComponentOfType(ComponentType::Attachment);
    EXPECT_TRUE(AttachmentComponent != nullptr);
    EXPECT_EQ(AttachmentComponent->GetAnchorPath(), "/xr/left-hand");

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AttachmentTests, AttachmentComponentEnterSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::String ObjectName = "Object 1";

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        auto* AttachmentComponent = (AttachmentSpaceComponent*)CreatedObject->AddComponent(ComponentType::Attachment);
        AttachmentComponent->SetAnchorPath("/xr/right-hand");

        CreatedObject->QueueUpdate();
        RealtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    std::this_thread::sleep_for(std::chrono::seconds(7));

    {
        bool EntitiesCreated = false;
        auto EntitiesReadyCallback = [&EntitiesCreated](int /*NumEntitiesFetched*/) { EntitiesCreated = true; };

        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback(EntitiesReadyCallback);

        auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

        WaitForCallbackWithUpdate(EntitiesCreated, RealtimeEngine.get());
        EXPECT_TRUE(EntitiesCreated);

        SpaceEntity* FoundEntity = RealtimeEngine->FindSpaceObject(ObjectName);
        EXPECT_TRUE(FoundEntity != nullptr);

        auto* AttachmentComponent = (AttachmentSpaceComponent*)FoundEntity->GetComponent(0);
        EXPECT_TRUE(AttachmentComponent != nullptr);

        EXPECT_EQ(AttachmentComponent->GetAnchorPath(), "/xr/right-hand");

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
