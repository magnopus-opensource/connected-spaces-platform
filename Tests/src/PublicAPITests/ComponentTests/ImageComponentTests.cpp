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
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, ImageTests, ImageComponentTest)
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

    auto* imageSpaceComponentInstance = (ImageSpaceComponent*)Object->AddComponent(ComponentType::Image);

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

    EXPECT_EQ(imageSpaceComponentInstance->GetBillboardMode(), BillboardMode::Off);
    EXPECT_EQ(imageSpaceComponentInstance->GetDisplayMode(), DisplayMode::DoubleSided);
    EXPECT_EQ(imageSpaceComponentInstance->GetIsARVisible(), true);
    EXPECT_EQ(imageSpaceComponentInstance->GetIsVirtualVisible(), true);
    EXPECT_EQ(imageSpaceComponentInstance->GetIsEmissive(), false);

    imageSpaceComponentInstance->SetAssetCollectionId(asset.AssetCollectionId);
    imageSpaceComponentInstance->SetImageAssetId(asset.Id);
    imageSpaceComponentInstance->SetBillboardMode(BillboardMode::YawLockedBillboard);
    imageSpaceComponentInstance->SetDisplayMode(DisplayMode::SingleSided);
    imageSpaceComponentInstance->SetIsARVisible(false);
    imageSpaceComponentInstance->SetIsVirtualVisible(false);
    imageSpaceComponentInstance->SetIsEmissive(true);

    auto imageSpaceComponentKey = imageSpaceComponentInstance->GetId();
    auto* storedImageSpaceComponent = (ImageSpaceComponent*)Object->GetComponent(imageSpaceComponentKey);

    EXPECT_EQ(storedImageSpaceComponent->GetAssetCollectionId(), asset.AssetCollectionId);
    EXPECT_EQ(storedImageSpaceComponent->GetImageAssetId(), asset.Id);
    EXPECT_EQ(storedImageSpaceComponent->GetBillboardMode(), BillboardMode::YawLockedBillboard);
    EXPECT_EQ(storedImageSpaceComponent->GetDisplayMode(), DisplayMode::SingleSided);
    EXPECT_EQ(storedImageSpaceComponent->GetIsARVisible(), false);
    EXPECT_EQ(storedImageSpaceComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(storedImageSpaceComponent->GetIsEmissive(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ImageTests, ImageScriptInterfaceTest)
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

    // Create object to represent the image
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create image component
    auto* imageComponent = (ImageSpaceComponent*)CreatedObject->AddComponent(ComponentType::Image);
    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(imageComponent->GetName(), "");
    EXPECT_EQ(imageComponent->GetImageAssetId(), "");
    EXPECT_EQ(imageComponent->GetAssetCollectionId(), "");
    EXPECT_EQ(imageComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(imageComponent->GetScale(), csp::common::Vector3::One());
    EXPECT_EQ(imageComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(imageComponent->GetBillboardMode(), BillboardMode::Off);
    EXPECT_EQ(imageComponent->GetDisplayMode(), DisplayMode::DoubleSided);
    EXPECT_EQ(imageComponent->GetIsEmissive(), false);
    EXPECT_EQ(imageComponent->GetIsVisible(), true);
    EXPECT_EQ(imageComponent->GetIsARVisible(), true);
    EXPECT_EQ(imageComponent->GetIsVirtualVisible(), true);

    // Setup script
    const std::string imageScriptText = R"xx(
		var image = ThisEntity.getImageComponents()[0];
        image.name = "TestName";
        image.imageAssetId = "TestImageAssetId";
        image.assetCollectionId = "TestAssetCollectionId";
        image.position = [1, 1, 1];
        image.scale = [2, 2, 2];
		image.rotation = [1, 1, 1, 1];
		image.billboardMode = 1;
        image.displayMode = 2;
		image.isEmissive = true;
        image.isVisible = false;
        image.isARVisible = false;
        image.isVirtualVisible = false;
    )xx";

    scriptComponent->SetScriptSource(imageScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);

    EXPECT_EQ(imageComponent->GetName(), "TestName");
    EXPECT_EQ(imageComponent->GetImageAssetId(), "TestImageAssetId");
    EXPECT_EQ(imageComponent->GetAssetCollectionId(), "TestAssetCollectionId");
    EXPECT_EQ(imageComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(imageComponent->GetScale(), csp::common::Vector3(2, 2, 2));
    EXPECT_EQ(imageComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(imageComponent->GetBillboardMode(), BillboardMode::Billboard);
    EXPECT_EQ(imageComponent->GetDisplayMode(), DisplayMode::DoubleSidedReversed);
    EXPECT_EQ(imageComponent->GetIsEmissive(), true);
    EXPECT_EQ(imageComponent->GetIsVisible(), false);
    EXPECT_EQ(imageComponent->GetIsARVisible(), false);
    EXPECT_EQ(imageComponent->GetIsVirtualVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
