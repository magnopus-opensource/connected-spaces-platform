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
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

CSP_PUBLIC_TEST(CSPEngine, AudioTests, AudioComponentTest)
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

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create audio component
    auto* AudioComponent = static_cast<AudioSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Audio));

    // Ensure defaults are set
    EXPECT_EQ(AudioComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(AudioComponent->GetPlaybackState(), AudioPlaybackState::Reset);
    EXPECT_EQ(AudioComponent->GetAudioType(), AudioType::Global);
    EXPECT_EQ(AudioComponent->GetAudioAssetId(), "");
    EXPECT_EQ(AudioComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(AudioComponent->GetAttenuationRadius(), 10.f);
    EXPECT_EQ(AudioComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(AudioComponent->GetTimeSincePlay(), 0.f);
    EXPECT_EQ(AudioComponent->GetVolume(), 1.f);
    EXPECT_EQ(AudioComponent->GetIsEnabled(), true);

    // Set new values
    csp::common::String AssetId = "TEST_ASSET_ID";
    csp::common::String AssetCollectionId = "TEST_COLLECTION_ID";

    AudioComponent->SetPosition(csp::common::Vector3::One());
    AudioComponent->SetPlaybackState(AudioPlaybackState::Play);
    AudioComponent->SetAudioType(AudioType::Spatial);
    AudioComponent->SetAudioAssetId(AssetId);
    AudioComponent->SetAssetCollectionId(AssetCollectionId);
    AudioComponent->SetAttenuationRadius(100.f);
    AudioComponent->SetIsLoopPlayback(true);
    AudioComponent->SetTimeSincePlay(1.f);
    AudioComponent->SetVolume(0.5f);
    AudioComponent->SetIsEnabled(false);

    // Ensure values are set correctly
    EXPECT_EQ(AudioComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(AudioComponent->GetPlaybackState(), AudioPlaybackState::Play);
    EXPECT_EQ(AudioComponent->GetAudioType(), AudioType::Spatial);
    EXPECT_EQ(AudioComponent->GetAudioAssetId(), AssetId);
    EXPECT_EQ(AudioComponent->GetAssetCollectionId(), AssetCollectionId);
    EXPECT_EQ(AudioComponent->GetAttenuationRadius(), 100.f);
    EXPECT_EQ(AudioComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(AudioComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(AudioComponent->GetVolume(), 0.5f);
    EXPECT_EQ(AudioComponent->GetIsEnabled(), false);

    // Test invalid volume values
    AudioComponent->SetVolume(1.5f);
    EXPECT_EQ(AudioComponent->GetVolume(), 0.5f);
    AudioComponent->SetVolume(-2.5f);
    EXPECT_EQ(AudioComponent->GetVolume(), 0.5f);

    // Test boundary volume values
    AudioComponent->SetVolume(1.f);
    EXPECT_EQ(AudioComponent->GetVolume(), 1.f);
    AudioComponent->SetVolume(0.f);
    EXPECT_EQ(AudioComponent->GetVolume(), 0.f);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AudioTests, AudioScriptInterfaceTest)
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

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create audio component
    auto* AudioComponent = (AudioSpaceComponent*)CreatedObject->AddComponent(ComponentType::Audio);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string AudioScriptText = R"xx(
	
		const assetId			= "TEST_ASSET_ID";
		const assetCollectionId = "TEST_COLLECTION_ID";

		var audio = ThisEntity.getAudioComponents()[0];
		audio.position = [1,1,1];
		audio.playbackState = 2;
		audio.audioType = 1;
		audio.audioAssetId = assetId;
		audio.assetCollectionId = assetCollectionId;
		audio.attenuationRadius = 100;
		audio.isLoopPlayback = true;
		audio.timeSincePlay = 1;
		audio.volume = 0.75;
    )xx";

    CreatedObject->GetScript().SetScriptSource(AudioScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    csp::common::String AssetId = "TEST_ASSET_ID";
    csp::common::String AssetCollectionId = "TEST_COLLECTION_ID";

    EXPECT_EQ(AudioComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(AudioComponent->GetPlaybackState(), AudioPlaybackState::Play);
    EXPECT_EQ(AudioComponent->GetAudioType(), AudioType::Spatial);
    EXPECT_EQ(AudioComponent->GetAudioAssetId(), AssetId);
    EXPECT_EQ(AudioComponent->GetAssetCollectionId(), AssetCollectionId);
    EXPECT_EQ(AudioComponent->GetAttenuationRadius(), 100.f);
    EXPECT_EQ(AudioComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(AudioComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(AudioComponent->GetVolume(), 0.75f);

    // Test invalid volume values
    AudioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 1.75;
    )xx";
    CreatedObject->GetScript().Invoke();
    RealtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(AudioComponent->GetVolume(), 0.75f);

    AudioScriptText = R"xx(M
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = -2.75;
    )xx";
    CreatedObject->GetScript().SetScriptSource(AudioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    RealtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(AudioComponent->GetVolume(), 0.75f);

    // Test boundary volume values
    AudioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 1.0;
    )xx";
    CreatedObject->GetScript().SetScriptSource(AudioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    RealtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(AudioComponent->GetVolume(), 1.f);

    AudioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 0.0;
    )xx";
    CreatedObject->GetScript().SetScriptSource(AudioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    RealtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(AudioComponent->GetVolume(), 0.f);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}