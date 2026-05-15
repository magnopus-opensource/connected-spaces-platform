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
#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"
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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, ReflectionTests, ReflectionComponentTest)
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

    auto* reflectionSpaceComponentInstance = (ReflectionSpaceComponent*)Object->AddComponent(ComponentType::Reflection);

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
    ASSERT_NE(uploadFileData, std::nullopt);

    csp::systems::BufferAssetDataSource bufferSource;
    bufferSource.Buffer = uploadFileData->data();
    bufferSource.BufferLength = uploadFileData->size();

    bufferSource.SetMimeType("image/png");

    printf("Uploading asset data...\n");

    // Upload data
    UploadAssetData(assetSystem, assetCollection, asset, bufferSource, asset.Uri);

    EXPECT_EQ(reflectionSpaceComponentInstance->GetReflectionShape(), ReflectionShape::UnitBox);

    reflectionSpaceComponentInstance->SetAssetCollectionId(asset.AssetCollectionId);
    reflectionSpaceComponentInstance->SetReflectionAssetId(asset.Id);
    reflectionSpaceComponentInstance->SetReflectionShape(ReflectionShape::UnitSphere);

    auto reflectionSpaceComponentKey = reflectionSpaceComponentInstance->GetId();
    auto* storedReflectionSpaceComponent = (ReflectionSpaceComponent*)Object->GetComponent(reflectionSpaceComponentKey);

    EXPECT_EQ(storedReflectionSpaceComponent->GetAssetCollectionId(), asset.AssetCollectionId);
    EXPECT_EQ(storedReflectionSpaceComponent->GetReflectionAssetId(), asset.Id);
    EXPECT_EQ(storedReflectionSpaceComponent->GetReflectionShape(), ReflectionShape::UnitSphere);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
