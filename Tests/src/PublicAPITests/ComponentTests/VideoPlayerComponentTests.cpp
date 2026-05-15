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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, VideoTests, VideoPlayerComponentTest)
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

    // Create video player component
    auto* videoComponent = static_cast<VideoPlayerSpaceComponent*>(CreatedObject->AddComponent(ComponentType::VideoPlayer));

    // Ensure defaults are set
    EXPECT_EQ(videoComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(videoComponent->GetPlaybackState(), VideoPlayerPlaybackState::Reset);
    EXPECT_EQ(videoComponent->GetVideoAssetURL(), "");
    EXPECT_EQ(videoComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(videoComponent->GetAttenuationRadius(), 10.f);
    EXPECT_EQ(videoComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(videoComponent->GetTimeSincePlay(), 0.f);
    EXPECT_EQ(videoComponent->GetIsStateShared(), false);
    EXPECT_EQ(videoComponent->GetIsAutoPlay(), false);
    EXPECT_EQ(videoComponent->GetIsAutoResize(), false);
    EXPECT_EQ(videoComponent->GetCurrentPlayheadPosition(), 0.0f);
    EXPECT_EQ(videoComponent->GetVideoPlayerSourceType(), VideoPlayerSourceType::AssetSource);
    EXPECT_EQ(videoComponent->GetStereoVideoType(), StereoVideoType::None);
    EXPECT_EQ(videoComponent->GetIsVisible(), true);
    EXPECT_EQ(videoComponent->GetIsARVisible(), true);
    EXPECT_EQ(videoComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(videoComponent->GetIsEnabled(), true);
    EXPECT_EQ(videoComponent->GetIsStereoFlipped(), false);
    EXPECT_EQ(videoComponent->GetAudioType(), AudioType::Spatial);
    EXPECT_EQ(videoComponent->GetVolume(), 1.f);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Set new values
    csp::common::String assetId = "TEST_ASSET_ID";
    csp::common::String assetCollectionId = "TEST_COLLECTION_ID";

    videoComponent->SetPosition(csp::common::Vector3::One());
    videoComponent->SetPlaybackState(VideoPlayerPlaybackState::Play);
    videoComponent->SetVideoAssetURL("http://youtube.com/avideo");
    videoComponent->SetAssetCollectionId(assetId);
    videoComponent->SetAttenuationRadius(100.f);
    videoComponent->SetIsLoopPlayback(true);
    videoComponent->SetTimeSincePlay(1.f);
    videoComponent->SetIsStateShared(true);
    videoComponent->SetIsAutoPlay(true);
    videoComponent->SetIsAutoResize(true);
    videoComponent->SetCurrentPlayheadPosition(1.0f);
    videoComponent->SetVideoPlayerSourceType(VideoPlayerSourceType::URLSource);
    videoComponent->SetStereoVideoType(StereoVideoType::SideBySide);
    videoComponent->SetIsVisible(false);
    videoComponent->SetIsARVisible(false);
    videoComponent->SetIsVirtualVisible(false);
    videoComponent->SetIsEnabled(false);
    videoComponent->SetIsStereoFlipped(true);
    videoComponent->SetAudioType(AudioType::Global);
    videoComponent->SetVolume(0.5f);

    // Ensure values are set correctly
    EXPECT_EQ(videoComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(videoComponent->GetPlaybackState(), VideoPlayerPlaybackState::Play);
    EXPECT_EQ(videoComponent->GetVideoAssetURL(), "http://youtube.com/avideo");
    EXPECT_EQ(videoComponent->GetAssetCollectionId(), assetId);
    EXPECT_EQ(videoComponent->GetAttenuationRadius(), 100.f);
    EXPECT_EQ(videoComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(videoComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(videoComponent->GetIsStateShared(), true);
    EXPECT_EQ(videoComponent->GetIsAutoPlay(), true);
    EXPECT_EQ(videoComponent->GetIsAutoResize(), true);
    EXPECT_EQ(videoComponent->GetCurrentPlayheadPosition(), 1.0f);
    EXPECT_EQ(videoComponent->GetVideoPlayerSourceType(), VideoPlayerSourceType::URLSource);
    EXPECT_EQ(videoComponent->GetStereoVideoType(), StereoVideoType::SideBySide);
    EXPECT_EQ(videoComponent->GetIsVisible(), false);
    EXPECT_EQ(videoComponent->GetIsARVisible(), false);
    EXPECT_EQ(videoComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(videoComponent->GetIsEnabled(), false);
    EXPECT_EQ(videoComponent->GetIsStereoFlipped(), true);
    EXPECT_EQ(videoComponent->GetAudioType(), AudioType::Global);
    EXPECT_EQ(videoComponent->GetVolume(), 0.5f);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, VideoTests, VideoPlayerScriptInterfaceTest)
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

    // Create video player component
    auto* videoPlayerComponent = (VideoPlayerSpaceComponent*)CreatedObject->AddComponent(ComponentType::VideoPlayer);

    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(videoPlayerComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(videoPlayerComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(videoPlayerComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(videoPlayerComponent->GetVideoAssetId(), "");
    EXPECT_EQ(videoPlayerComponent->GetVideoAssetURL(), "");
    EXPECT_EQ(videoPlayerComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(videoPlayerComponent->GetIsStateShared(), false);
    EXPECT_EQ(videoPlayerComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(videoPlayerComponent->GetIsAutoResize(), false);
    EXPECT_EQ(videoPlayerComponent->GetPlaybackState(), VideoPlayerPlaybackState::Reset);
    EXPECT_EQ(videoPlayerComponent->GetCurrentPlayheadPosition(), 0.0f);
    EXPECT_EQ(videoPlayerComponent->GetTimeSincePlay(), 0.f);
    EXPECT_EQ(videoPlayerComponent->GetVideoPlayerSourceType(), VideoPlayerSourceType::AssetSource);
    EXPECT_EQ(videoPlayerComponent->GetStereoVideoType(), StereoVideoType::None);
    EXPECT_EQ(videoPlayerComponent->GetIsVisible(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsARVisible(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsEnabled(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsStereoFlipped(), false);
    EXPECT_EQ(videoPlayerComponent->GetAudioType(), AudioType::Spatial);
    EXPECT_EQ(videoPlayerComponent->GetVolume(), 1.f);
    EXPECT_EQ(videoPlayerComponent->GetVolume(), 1.f);
    EXPECT_EQ(videoPlayerComponent->GetIsAutoPlay(), false);
    EXPECT_EQ(videoPlayerComponent->GetAttenuationRadius(), 10.f);

    // Setup script
    const std::string videoPlayerScriptText = R"xx(

		var video = ThisEntity.getVideoPlayerComponents()[0];

        video.position = [1, 1, 1];
        video.scale = [2, 2, 2];
		video.rotation = [1, 1, 1, 1];
        video.videoAssetId = "TestVideoAssetId";
        video.videoAssetURL = "http://youtube.com/avideo"
        video.assetCollectionId = "TestAssetCollectionId"
        video.isStateShared = true;
        video.isLoopPlayback = true;
        video.isAutoResize = true;
        video.playbackState = 2;
        video.currentPlayheadPosition = 1;
        video.timeSincePlay = 1;
        video.videoPlayerSourceType = 0;
        video.stereoVideoType = 1;
        video.isStereoFlipped = true;
		video.isVisible = false;
        video.isARVisible = false;
        video.isVirtualVisible = false;
        video.isEnabled = false;
        video.audioType = 0;
        video.volume = 0.5;
        video.isAutoPlay = true;
        video.attenuationRadius = 22.0;
    )xx";

    scriptComponent->SetScriptSource(videoPlayerScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);

    EXPECT_EQ(videoPlayerComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(videoPlayerComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(videoPlayerComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(videoPlayerComponent->GetVideoAssetId(), "TestVideoAssetId");
    EXPECT_EQ(videoPlayerComponent->GetVideoAssetURL(), "http://youtube.com/avideo");
    EXPECT_EQ(videoPlayerComponent->GetAssetCollectionId(), "TestAssetCollectionId");
    EXPECT_EQ(videoPlayerComponent->GetIsStateShared(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsAutoResize(), true);
    EXPECT_EQ(videoPlayerComponent->GetPlaybackState(), VideoPlayerPlaybackState::Play);
    EXPECT_EQ(videoPlayerComponent->GetCurrentPlayheadPosition(), 1.0f);
    EXPECT_EQ(videoPlayerComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(videoPlayerComponent->GetVideoPlayerSourceType(), VideoPlayerSourceType::URLSource);
    EXPECT_EQ(videoPlayerComponent->GetStereoVideoType(), StereoVideoType::SideBySide);
    EXPECT_EQ(videoPlayerComponent->GetIsStereoFlipped(), true);
    EXPECT_EQ(videoPlayerComponent->GetIsVisible(), false);
    EXPECT_EQ(videoPlayerComponent->GetIsARVisible(), false);
    EXPECT_EQ(videoPlayerComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(videoPlayerComponent->GetIsEnabled(), false);
    EXPECT_EQ(videoPlayerComponent->GetAudioType(), AudioType::Global);
    EXPECT_EQ(videoPlayerComponent->GetVolume(), 0.5f);
    EXPECT_EQ(videoPlayerComponent->GetIsAutoPlay(), true);
    EXPECT_EQ(videoPlayerComponent->GetAttenuationRadius(), 22.f);

    // volume input range validation
    {
        const auto expectedVolume = videoPlayerComponent->GetVolume();
        CreatedObject->GetScript().SetScriptSource(R"(
            const video = ThisEntity.getVideoPlayerComponents()[0];
            video.volume = 1.75;
        )");

        CreatedObject->GetScript().Invoke();
        realtimeEngine->ProcessPendingEntityOperations();

        EXPECT_EQ(videoPlayerComponent->GetVolume(), expectedVolume);
    
        CreatedObject->GetScript().SetScriptSource(R"(
            const video = ThisEntity.getVideoPlayerComponents()[0];
            video.volume = -2.75;
        )");

        CreatedObject->GetScript().Invoke();
        realtimeEngine->ProcessPendingEntityOperations();

        EXPECT_EQ(videoPlayerComponent->GetVolume(), expectedVolume);
    }

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}