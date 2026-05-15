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
#include "CSP/Multiplayer/Components/FiducialMarkerSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, FiducialMarkerTests, FiducialMarkerComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";
    const char* testAssetName = "CSP-UNITTEST-ASSET-MAG";

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

    csp::common::String callbackAssetId;

    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String modelAssetId = "NotARealId";

    auto* fiducialMarkerSpaceComponentInstance = (FiducialMarkerSpaceComponent*)Object->AddComponent(ComponentType::FiducialMarker);

    // Process component creation
    Object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& components = *Object->GetComponents();
    EXPECT_EQ(components.Size(), 1);

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    asset.FileName = "OKO.png";
    asset.Name = "OKO";
    asset.Type = csp::systems::EAssetType::IMAGE;

    auto uploadFileData = OpenFile("assets/OKO.png");
    ASSERT_TRUE(uploadFileData.has_value());

    csp::systems::BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData->data();
    bufferSource.BufferLength = uploadFileData->size();

    bufferSource.SetMimeType("image/png");

    printf("Uploading asset data...\n");

    // Upload data
    UploadAssetData(assetSystem, assetCollection, asset, bufferSource, asset.Uri);

    EXPECT_EQ(fiducialMarkerSpaceComponentInstance->GetIsVirtualVisible(), true);
    EXPECT_EQ(fiducialMarkerSpaceComponentInstance->GetIsARVisible(), true);
    EXPECT_EQ(fiducialMarkerSpaceComponentInstance->GetIsVisible(), true);

    fiducialMarkerSpaceComponentInstance->SetAssetCollectionId(asset.AssetCollectionId);
    fiducialMarkerSpaceComponentInstance->SetMarkerAssetId(asset.Id);
    fiducialMarkerSpaceComponentInstance->SetIsVirtualVisible(false);
    fiducialMarkerSpaceComponentInstance->SetIsARVisible(false);

    auto fiducialMarkerSpaceComponentKey = fiducialMarkerSpaceComponentInstance->GetId();
    auto* storedFiducialMarkerSpaceComponent = (FiducialMarkerSpaceComponent*)Object->GetComponent(fiducialMarkerSpaceComponentKey);

    EXPECT_EQ(storedFiducialMarkerSpaceComponent->GetAssetCollectionId(), asset.AssetCollectionId);
    EXPECT_EQ(storedFiducialMarkerSpaceComponent->GetMarkerAssetId(), asset.Id);
    EXPECT_EQ(storedFiducialMarkerSpaceComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(storedFiducialMarkerSpaceComponent->GetIsARVisible(), false);
    EXPECT_EQ(fiducialMarkerSpaceComponentInstance->GetIsVisible(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, FiducialMarkerTests, FiducialMarkerScriptInterfaceTest)
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

    // Create fiducial marker component
    auto* fiducialMarkerComponent = (FiducialMarkerSpaceComponent*)CreatedObject->AddComponent(ComponentType::FiducialMarker);

    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(fiducialMarkerComponent->GetIsVisible(), true);
    EXPECT_EQ(fiducialMarkerComponent->GetMarkerAssetId(), "");
    EXPECT_EQ(fiducialMarkerComponent->GetAssetCollectionId(), "");

    // Setup script
    const std::string fiducialMarkerScriptText = R"xx(
		var marker = ThisEntity.getFiducialMarkerComponents()[0];
		marker.name = "Updated_FiducialMarkerScriptName";
        marker.position = [1, 1, 1];
        marker.scale = [2, 2, 2];
		marker.rotation = [1, 1, 1, 1];
		marker.isVisible = false;
        marker.isARVisible = false;
        marker.isVirtualVisible = false;
        marker.markerAssetId = "TestAssetId";
        marker.assetCollectionId = "TestAssetCollectionId";
    )xx";

    scriptComponent->SetScriptSource(fiducialMarkerScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);

    EXPECT_EQ(fiducialMarkerComponent->GetName(), "Updated_FiducialMarkerScriptName");
    EXPECT_EQ(fiducialMarkerComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(fiducialMarkerComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(fiducialMarkerComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(fiducialMarkerComponent->GetIsVisible(), false);
    EXPECT_EQ(fiducialMarkerComponent->GetIsARVisible(), false);
    EXPECT_EQ(fiducialMarkerComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(fiducialMarkerComponent->GetMarkerAssetId(), "TestAssetId");
    EXPECT_EQ(fiducialMarkerComponent->GetAssetCollectionId(), "TestAssetCollectionId");

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
