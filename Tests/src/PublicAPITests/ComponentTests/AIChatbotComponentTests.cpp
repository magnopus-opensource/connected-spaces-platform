/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Multiplayer/Components/AIChatbotComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, AIChatbotTests, AIChatbotSpaceComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* TestAssetName = "CSP-UNITTEST-ASSET-MAG";

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

    // Create parent Space Entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create ai chatbot component
    auto* AIChatbotComponent = static_cast<AIChatbotSpaceComponent*>(CreatedObject->AddComponent(ComponentType::AIChatbot));

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    char UniqueAssetName[256];
    SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

    // Create asset collection
    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

    // Create asset
    csp::systems::Asset Asset;
    CreateAsset(AssetSystem, AssetCollection, UniqueAssetName, nullptr, nullptr, Asset);
    Asset.FileName = "Guardrails.txt";
    Asset.Name = "Guardrails";
    Asset.Type = csp::systems::EAssetType::TEXT;

    auto UploadFilePath = std::filesystem::absolute("assets/Guardrails.txt");
    FILE* UploadFile = fopen(UploadFilePath.string().c_str(), "rb");
    uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
    auto* UploadFileData = new unsigned char[UploadFileSize];
    fread(UploadFileData, UploadFileSize, 1, UploadFile);
    fclose(UploadFile);

    csp::systems::BufferAssetDataSource BufferSource;
    BufferSource.Buffer = UploadFileData;
    BufferSource.BufferLength = UploadFileSize;

    BufferSource.SetMimeType("text/plain");

    printf("Uploading asset data...\n");

    // Upload data
    UploadAssetData(AssetSystem, AssetCollection, Asset, BufferSource, Asset.Uri);

    delete[] UploadFileData;

    // Ensure defaults are set
    EXPECT_EQ(AIChatbotComponent->GetPosition(), csp::common::Vector3::Zero());

    EXPECT_EQ(AIChatbotComponent->GetVoice(), "");

    EXPECT_EQ(AIChatbotComponent->GetGuardrailAssetCollectionId(), "");

    EXPECT_EQ(AIChatbotComponent->GetVisualState(), AIChatbotVisualState::Waiting);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Set new values
    csp::common::String Voice = "Zephyr";

    AIChatbotComponent->SetPosition(csp::common::Vector3::One());

    AIChatbotComponent->SetVoice(Voice);

    AIChatbotComponent->SetGuardrailAssetCollectionId(Asset.AssetCollectionId);

    AIChatbotComponent->SetVisualState(AIChatbotVisualState::Listening);

    // Ensure values are set correctly
    EXPECT_EQ(AIChatbotComponent->GetPosition(), csp::common::Vector3::One());

    EXPECT_EQ(AIChatbotComponent->GetGuardrailAssetCollectionId(), Asset.AssetCollectionId);

    EXPECT_EQ(AIChatbotComponent->GetVoice(), Voice);

    EXPECT_EQ(AIChatbotComponent->GetVisualState(), AIChatbotVisualState::Listening);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AIChatbotTests, AIChatbotSpaceComponentScriptTest)
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

    // Create parent Space Entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create ai chatbot component
    auto* AIChatbotComponent = (AIChatbotSpaceComponent*)CreatedObject->AddComponent(ComponentType::AIChatbot);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    const std::string ScreenSharingScriptText = R"xx(
		var component = ThisEntity.getAIChatbotComponents()[0];

		component.position = [1, 1, 1];
        component.voice = "Zephyr";
        component.guardrailAssetCollectionId = "TEST_GUARDRAIL_ASSET_COLLECTION_ID";
        component.visualState = 1;
    )xx";

    CreatedObject->GetScript().SetScriptSource(ScreenSharingScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_EQ(AIChatbotComponent->GetPosition(), csp::common::Vector3::One());

    EXPECT_EQ(AIChatbotComponent->GetVoice(), "Zephyr");

    EXPECT_EQ(AIChatbotComponent->GetGuardrailAssetCollectionId(), "TEST_GUARDRAIL_ASSET_COLLECTION_ID");

    EXPECT_EQ(AIChatbotComponent->GetVisualState(), AIChatbotVisualState::Listening);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
