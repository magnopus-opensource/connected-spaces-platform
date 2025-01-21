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
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"
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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

#if RUN_ALL_UNIT_TESTS || RUN_VIDEO_TESTS || RUN_VIDEO_PLAYER_COMPONENT_TEST
CSP_PUBLIC_TEST(CSPEngine, VideoTests, VideoPlayerComponentTest)
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

    // Create object to represent the audio
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create audio component
    auto* VideoComponent = static_cast<VideoPlayerSpaceComponent*>(CreatedObject->AddComponent(ComponentType::VideoPlayer));

    // Ensure defaults are set
    EXPECT_EQ(VideoComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(VideoComponent->GetPlaybackState(), VideoPlayerPlaybackState::Reset);
    EXPECT_EQ(VideoComponent->GetVideoAssetURL(), "");
    EXPECT_EQ(VideoComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(VideoComponent->GetAttenuationRadius(), 10.f);
    EXPECT_EQ(VideoComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(VideoComponent->GetTimeSincePlay(), 0.f);
    EXPECT_EQ(VideoComponent->GetIsStateShared(), false);
    EXPECT_EQ(VideoComponent->GetIsAutoPlay(), false);
    EXPECT_EQ(VideoComponent->GetIsAutoResize(), false);
    EXPECT_EQ(VideoComponent->GetCurrentPlayheadPosition(), 0.0f);
    EXPECT_EQ(VideoComponent->GetVideoPlayerSourceType(), VideoPlayerSourceType::AssetSource);
    EXPECT_EQ(VideoComponent->GetIsVisible(), true);
    EXPECT_EQ(VideoComponent->GetMeshComponentId(), 0);
    EXPECT_EQ(VideoComponent->GetIsEnabled(), true);

    auto* ModelComponent = static_cast<VideoPlayerSpaceComponent*>(CreatedObject->AddComponent(ComponentType::AnimatedModel));

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    // Set new values
    csp::common::String AssetId = "TEST_ASSET_ID";
    csp::common::String AssetCollectionId = "TEST_COLLECTION_ID";

    VideoComponent->SetPosition(csp::common::Vector3::One());
    VideoComponent->SetPlaybackState(VideoPlayerPlaybackState::Play);
    VideoComponent->SetVideoAssetURL("http://youtube.com/avideo");
    VideoComponent->SetAssetCollectionId(AssetId);
    VideoComponent->SetAttenuationRadius(100.f);
    VideoComponent->SetIsLoopPlayback(true);
    VideoComponent->SetTimeSincePlay(1.f);
    VideoComponent->SetIsStateShared(true);
    VideoComponent->SetIsAutoPlay(true);
    VideoComponent->SetIsAutoResize(true);
    VideoComponent->SetCurrentPlayheadPosition(1.0f);
    VideoComponent->SetVideoPlayerSourceType(VideoPlayerSourceType::URLSource);
    VideoComponent->SetIsVisible(false);
    VideoComponent->SetMeshComponentId(ModelComponent->GetId());
    VideoComponent->SetIsEnabled(false);

    // Ensure values are set correctly
    EXPECT_EQ(VideoComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(VideoComponent->GetPlaybackState(), VideoPlayerPlaybackState::Play);
    EXPECT_EQ(VideoComponent->GetVideoAssetURL(), "http://youtube.com/avideo");
    EXPECT_EQ(VideoComponent->GetAssetCollectionId(), AssetId);
    EXPECT_EQ(VideoComponent->GetAttenuationRadius(), 100.f);
    EXPECT_EQ(VideoComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(VideoComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(VideoComponent->GetIsStateShared(), true);
    EXPECT_EQ(VideoComponent->GetIsAutoPlay(), true);
    EXPECT_EQ(VideoComponent->GetIsAutoResize(), true);
    EXPECT_EQ(VideoComponent->GetCurrentPlayheadPosition(), 1.0f);
    EXPECT_EQ(VideoComponent->GetVideoPlayerSourceType(), VideoPlayerSourceType::URLSource);
    EXPECT_EQ(VideoComponent->GetIsVisible(), false);
    EXPECT_EQ(VideoComponent->GetMeshComponentId(), ModelComponent->GetId());
    EXPECT_EQ(VideoComponent->GetIsEnabled(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace
