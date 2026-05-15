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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

CSP_PUBLIC_TEST(CSPEngine, AudioTests, AudioComponentTest)
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

    // Create audio component
    auto* audioComponent = static_cast<AudioSpaceComponent*>(CreatedObject->AddComponent(ComponentType::Audio));

    // Ensure defaults are set
    EXPECT_EQ(audioComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(audioComponent->GetPlaybackState(), AudioPlaybackState::Reset);
    EXPECT_EQ(audioComponent->GetAudioType(), AudioType::Global);
    EXPECT_EQ(audioComponent->GetAudioAssetId(), "");
    EXPECT_EQ(audioComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(audioComponent->GetAttenuationRadius(), 10.f);
    EXPECT_EQ(audioComponent->GetIsLoopPlayback(), false);
    EXPECT_EQ(audioComponent->GetTimeSincePlay(), 0.f);
    EXPECT_EQ(audioComponent->GetVolume(), 1.f);
    EXPECT_EQ(audioComponent->GetIsEnabled(), true);

    // Set new values
    csp::common::String assetId = "TEST_ASSET_ID";
    csp::common::String assetCollectionId = "TEST_COLLECTION_ID";

    audioComponent->SetPosition(csp::common::Vector3::One());
    audioComponent->SetPlaybackState(AudioPlaybackState::Play);
    audioComponent->SetAudioType(AudioType::Spatial);
    audioComponent->SetAudioAssetId(assetId);
    audioComponent->SetAssetCollectionId(assetCollectionId);
    audioComponent->SetAttenuationRadius(100.f);
    audioComponent->SetIsLoopPlayback(true);
    audioComponent->SetTimeSincePlay(1.f);
    audioComponent->SetVolume(0.5f);
    audioComponent->SetIsEnabled(false);

    // Ensure values are set correctly
    EXPECT_EQ(audioComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(audioComponent->GetPlaybackState(), AudioPlaybackState::Play);
    EXPECT_EQ(audioComponent->GetAudioType(), AudioType::Spatial);
    EXPECT_EQ(audioComponent->GetAudioAssetId(), assetId);
    EXPECT_EQ(audioComponent->GetAssetCollectionId(), assetCollectionId);
    EXPECT_EQ(audioComponent->GetAttenuationRadius(), 100.f);
    EXPECT_EQ(audioComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(audioComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(audioComponent->GetVolume(), 0.5f);
    EXPECT_EQ(audioComponent->GetIsEnabled(), false);

    // Test invalid volume values
    audioComponent->SetVolume(1.5f);
    EXPECT_EQ(audioComponent->GetVolume(), 0.5f);
    audioComponent->SetVolume(-2.5f);
    EXPECT_EQ(audioComponent->GetVolume(), 0.5f);

    // Test boundary volume values
    audioComponent->SetVolume(1.f);
    EXPECT_EQ(audioComponent->GetVolume(), 1.f);
    audioComponent->SetVolume(0.f);
    EXPECT_EQ(audioComponent->GetVolume(), 0.f);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AudioTests, AudioScriptInterfaceTest)
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

    // Create audio component
    auto* audioComponent = (AudioSpaceComponent*)CreatedObject->AddComponent(ComponentType::Audio);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string audioScriptText = R"xx(
	
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
		audio.isEnabled = false;
    )xx";

    CreatedObject->GetScript().SetScriptSource(audioScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    csp::common::String assetId = "TEST_ASSET_ID";
    csp::common::String assetCollectionId = "TEST_COLLECTION_ID";

    EXPECT_EQ(audioComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(audioComponent->GetPlaybackState(), AudioPlaybackState::Play);
    EXPECT_EQ(audioComponent->GetAudioType(), AudioType::Spatial);
    EXPECT_EQ(audioComponent->GetAudioAssetId(), assetId);
    EXPECT_EQ(audioComponent->GetAssetCollectionId(), assetCollectionId);
    EXPECT_EQ(audioComponent->GetAttenuationRadius(), 100.f);
    EXPECT_EQ(audioComponent->GetIsLoopPlayback(), true);
    EXPECT_EQ(audioComponent->GetTimeSincePlay(), 1.f);
    EXPECT_EQ(audioComponent->GetVolume(), 0.75f);
    EXPECT_EQ(audioComponent->GetIsEnabled(), false);

    // Test invalid volume values
    audioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 1.75;
    )xx";
    CreatedObject->GetScript().SetScriptSource(audioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    realtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(audioComponent->GetVolume(), 0.75f);

    audioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = -2.75;
    )xx";
    CreatedObject->GetScript().SetScriptSource(audioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    realtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(audioComponent->GetVolume(), 0.75f);

    // Test boundary volume values
    audioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 1.0;
    )xx";
    CreatedObject->GetScript().SetScriptSource(audioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    realtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(audioComponent->GetVolume(), 1.f);

    audioScriptText = R"xx(
		var audio = ThisEntity.getAudioComponents()[0];
		audio.volume = 0.0;
    )xx";
    CreatedObject->GetScript().SetScriptSource(audioScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    realtimeEngine->ProcessPendingEntityOperations();
    EXPECT_EQ(audioComponent->GetVolume(), 0.f);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}