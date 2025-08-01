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
#include "CSP/Multiplayer/Components/PortalSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
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

CSP_PUBLIC_TEST(CSPEngine, PortalTests, UsePortalTest)
{
    SetRandSeed();

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

    // Create space 2
    csp::systems::Space Space2;
    CreateDefaultTestSpace(SpaceSystem, Space);

    csp::common::String PortalSpaceID;

    const csp::common::String UserName = "Player 1";
    const SpaceTransform UserTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    const bool IsVisible = true;
    const AvatarState UserAvatarState = AvatarState::Idle;
    const csp::common::String UserAvatarId = "MyCoolAvatar";
    const AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        const auto LoginState = UserSystem->GetLoginState();

        auto [Avatar]
            = AWAIT(EntitySystem, CreateAvatar, UserName, LoginState, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

        // Create object to represent the portal
        csp::common::String ObjectName = "Object 1";
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

        // Create portal component
        auto* PortalComponent = (PortalSpaceComponent*)CreatedObject->AddComponent(ComponentType::Portal);
        PortalComponent->SetSpaceId(Space2.Id);

        PortalSpaceID = PortalComponent->GetSpaceId();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    /*
            User would now interact with the portal
    */

    {
        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        const auto LoginState = UserSystem->GetLoginState();

        auto [Avatar]
            = AWAIT(EntitySystem, CreateAvatar, UserName, LoginState, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode);

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
    DeleteSpace(SpaceSystem, Space2.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PortalTests, PortalThumbnailTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto FilePath = std::filesystem::absolute("assets/OKO.png");

    csp::systems::FileAssetDataSource Source;
    Source.FilePath = FilePath.string().c_str();

    // Create space
    csp::systems::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, Source, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create object to represent the portal
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create portal component
    auto* PortalComponent = (PortalSpaceComponent*)CreatedObject->AddComponent(ComponentType::Portal);

    // Get Thumbnail
    bool HasThumbailResult = false;

    csp::systems::UriResultCallback Callback = [&HasThumbailResult](const csp::systems::UriResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            HasThumbailResult = true;
            EXPECT_TRUE(Result.GetUri() != "");
        }
    };

    PortalComponent->SetSpaceId(Space.Id);

    // Get thumbnail using the space id.
    SpaceSystem->GetSpaceThumbnail(PortalComponent->GetSpaceId(), Callback);

    WaitForCallback(HasThumbailResult);
    EXPECT_TRUE(HasThumbailResult);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PortalTests, PortalScriptInterfaceTest)
{
    SetRandSeed();

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

    // Create object to represent the portal
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create portal component
    auto* PortalComponent = (PortalSpaceComponent*)CreatedObject->AddComponent(ComponentType::Portal);

    auto InitialPosition = csp::common::Vector3 { 1.1f, 2.2f, 3.3f };
    PortalComponent->SetSpaceId("initialTestSpaceId");
    PortalComponent->SetIsEnabled(false);
    PortalComponent->SetPosition(InitialPosition);
    PortalComponent->SetRadius(123.123f);

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(PortalComponent->GetSpaceId(), "initialTestSpaceId");
    EXPECT_EQ(PortalComponent->GetIsEnabled(), false);
    EXPECT_FLOAT_EQ(PortalComponent->GetPosition().X, InitialPosition.X);
    EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Y, InitialPosition.Y);
    EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Z, InitialPosition.Z);
    EXPECT_EQ(PortalComponent->GetRadius(), 123.123f);

    // Setup script
    std::string PortalScriptText = R"xx(
		var portal = ThisEntity.getPortalComponents()[0];
		portal.spaceId = "secondTestSpaceId";
		portal.isEnabled = true;
		portal.position = [4.4, 5.5, 6.6];
		portal.radius = 456.456;
    )xx";

    CreatedObject->GetScript().SetScriptSource(PortalScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(PortalComponent->GetSpaceId(), "secondTestSpaceId");
    EXPECT_EQ(PortalComponent->GetIsEnabled(), true);
    EXPECT_FLOAT_EQ(PortalComponent->GetPosition().X, 4.4f);
    EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Y, 5.5f);
    EXPECT_FLOAT_EQ(PortalComponent->GetPosition().Z, 6.6f);
    EXPECT_FLOAT_EQ(PortalComponent->GetRadius(), 456.456f);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}