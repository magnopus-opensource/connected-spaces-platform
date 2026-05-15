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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, PortalTests, UsePortalTest)
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

    // Create space 2
    csp::systems::Space space2;
    CreateDefaultTestSpace(spaceSystem, space);

    csp::common::String portalSpaceId;

    const csp::common::String userName = "Player 1";
    const SpaceTransform userTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    const bool isVisible = true;
    const AvatarState userAvatarState = AvatarState::Idle;
    const csp::common::String userAvatarId = "MyCoolAvatar";
    const AvatarPlayMode userAvatarPlayMode = AvatarPlayMode::Default;

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        const auto loginState = userSystem->GetLoginState();

        auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState,
            userAvatarId, userAvatarPlayMode, LocomotionModel::Grounded);

        // Create object to represent the portal
        csp::common::String objectName = "Object 1";
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create portal component
        auto* portalComponent = (PortalSpaceComponent*)CreatedObject->AddComponent(ComponentType::Portal);
        portalComponent->SetSpaceId(space2.Id);

        portalSpaceId = portalComponent->GetSpaceId();

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    /*
            User would now interact with the portal
    */

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        const auto loginState = userSystem->GetLoginState();

        auto [Avatar] = AWAIT(realtimeEngine.get(), CreateAvatar, userName, loginState.UserId, userTransform, isVisible, userAvatarState,
            userAvatarId, userAvatarPlayMode, LocomotionModel::Grounded);

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);
    DeleteSpace(spaceSystem, space2.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PortalTests, PortalThumbnailTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto filePath = std::filesystem::absolute("assets/OKO.png");

    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.string().c_str();

    // Create space
    csp::systems::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, source, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create object to represent the portal
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create portal component
    auto* portalComponent = (PortalSpaceComponent*)CreatedObject->AddComponent(ComponentType::Portal);

    // Get Thumbnail
    bool hasThumbailResult = false;

    csp::systems::UriResultCallback callback = [&hasThumbailResult](const csp::systems::UriResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            hasThumbailResult = true;
            EXPECT_TRUE(result.GetUri() != "");
        }
    };

    portalComponent->SetSpaceId(space.Id);

    // Get thumbnail using the space id.
    spaceSystem->GetSpaceThumbnail(portalComponent->GetSpaceId(), callback);

    WaitForCallback(hasThumbailResult);
    EXPECT_TRUE(hasThumbailResult);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, PortalTests, PortalScriptInterfaceTest)
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

    // Create object to represent the portal
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create portal component
    auto* portalComponent = (PortalSpaceComponent*)CreatedObject->AddComponent(ComponentType::Portal);

    auto initialPosition = csp::common::Vector3 { 1.1f, 2.2f, 3.3f };
    portalComponent->SetSpaceId("initialTestSpaceId");
    portalComponent->SetIsEnabled(false);
    portalComponent->SetPosition(initialPosition);
    portalComponent->SetRadius(123.123f);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(portalComponent->GetSpaceId(), "initialTestSpaceId");
    EXPECT_EQ(portalComponent->GetIsEnabled(), false);
    EXPECT_FLOAT_EQ(portalComponent->GetPosition().X, initialPosition.X);
    EXPECT_FLOAT_EQ(portalComponent->GetPosition().Y, initialPosition.Y);
    EXPECT_FLOAT_EQ(portalComponent->GetPosition().Z, initialPosition.Z);
    EXPECT_EQ(portalComponent->GetRadius(), 123.123f);

    // Setup script
    std::string portalScriptText = R"xx(
		var portal = ThisEntity.getPortalComponents()[0];
		portal.spaceId = "secondTestSpaceId";
		portal.isEnabled = true;
		portal.position = [4.4, 5.5, 6.6];
		portal.radius = 456.456;
    )xx";

    CreatedObject->GetScript().SetScriptSource(portalScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(portalComponent->GetSpaceId(), "secondTestSpaceId");
    EXPECT_EQ(portalComponent->GetIsEnabled(), true);
    EXPECT_FLOAT_EQ(portalComponent->GetPosition().X, 4.4f);
    EXPECT_FLOAT_EQ(portalComponent->GetPosition().Y, 5.5f);
    EXPECT_FLOAT_EQ(portalComponent->GetPosition().Z, 6.6f);
    EXPECT_FLOAT_EQ(portalComponent->GetRadius(), 456.456f);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}