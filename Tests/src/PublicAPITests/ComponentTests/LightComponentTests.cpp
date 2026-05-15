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
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
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

CSP_PUBLIC_TEST(CSPEngine, LightTests, LightComponentFieldsTest)
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

    auto* lightSpaceComponentInstance = (LightSpaceComponent*)Object->AddComponent(ComponentType::Light);

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

    EXPECT_EQ(lightSpaceComponentInstance->GetLightCookieType(), LightCookieType::NoCookie);
    EXPECT_EQ(lightSpaceComponentInstance->GetLightType(), LightType::Point);
    EXPECT_EQ(lightSpaceComponentInstance->GetLightShadowType(), LightShadowType::None);

    // test values
    const float innerConeAngle = 10.0f;
    const float outerConeAngle = 20.0f;
    const float range = 120.0f;
    const float intensity = 1000.0f;

    lightSpaceComponentInstance->SetLightCookieType(LightCookieType::ImageCookie);
    lightSpaceComponentInstance->SetLightCookieAssetCollectionId(asset.AssetCollectionId);
    lightSpaceComponentInstance->SetLightCookieAssetId(asset.Id);
    lightSpaceComponentInstance->SetLightType(LightType::Spot);
    lightSpaceComponentInstance->SetLightShadowType(LightShadowType::Realtime);
    lightSpaceComponentInstance->SetInnerConeAngle(innerConeAngle);
    lightSpaceComponentInstance->SetOuterConeAngle(outerConeAngle);
    lightSpaceComponentInstance->SetRange(range);
    lightSpaceComponentInstance->SetIntensity(intensity);

    auto lightSpaceComponentKey = lightSpaceComponentInstance->GetId();
    auto* storedLightSpaceComponentInstance = (LightSpaceComponent*)Object->GetComponent(lightSpaceComponentKey);

    EXPECT_EQ(storedLightSpaceComponentInstance->GetLightCookieType(), LightCookieType::ImageCookie);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetLightCookieAssetCollectionId(), asset.AssetCollectionId);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetLightCookieAssetId(), asset.Id);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetLightType(), LightType::Spot);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetLightShadowType(), LightShadowType::Realtime);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetInnerConeAngle(), innerConeAngle);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetOuterConeAngle(), outerConeAngle);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetRange(), range);
    EXPECT_EQ(storedLightSpaceComponentInstance->GetIntensity(), intensity);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LightTests, ActionHandlerTest)
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

    csp::common::String callbackAssetId;

    const csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String modelAssetId = "NotARealId";

    auto* lightSpaceComponentInstance = (LightSpaceComponent*)Object->AddComponent(ComponentType::Light);

    // Process component creation
    Object->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& components = *Object->GetComponents();
    EXPECT_EQ(components.Size(), 1);

    bool actionCalled = false;
    lightSpaceComponentInstance->RegisterActionHandler(
        "TestAction", [&actionCalled](ComponentBase*, const csp::common::String, const csp::common::String) { actionCalled = true; });

    lightSpaceComponentInstance->InvokeAction("TestAction", "TestParam");

    EXPECT_TRUE(actionCalled);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LightTests, LightSpaceScriptInterfaceTest)
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

    // Create light component
    auto* lightComponent = (LightSpaceComponent*)CreatedObject->AddComponent(ComponentType::Light);

    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(lightComponent->GetLightType(), LightType::Point);
    EXPECT_EQ(lightComponent->GetIntensity(), 5000.f);
    EXPECT_EQ(lightComponent->GetRange(), 1000.f);
    EXPECT_EQ(lightComponent->GetInnerConeAngle(), 0.f);
    EXPECT_EQ(lightComponent->GetOuterConeAngle(), 0.78539816339f);
    EXPECT_EQ(lightComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(lightComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(lightComponent->GetColor(), csp::common::Vector3(255, 255, 255));
    EXPECT_EQ(lightComponent->GetIsVisible(), true);
    EXPECT_EQ(lightComponent->GetIsARVisible(), true);
    EXPECT_EQ(lightComponent->GetIsVirtualVisible(), true);
    EXPECT_EQ(lightComponent->GetLightCookieAssetId(), "");
    EXPECT_EQ(lightComponent->GetLightCookieAssetCollectionId(), "");
    EXPECT_EQ(lightComponent->GetLightCookieType(), LightCookieType::NoCookie);

    // Setup script
    const std::string lightSpaceScriptText = R"xx(

		var light = ThisEntity.getLightComponents()[0];
        
        light.lightType = 2;
        light.Intensity = 10000;
        light.range = 5000;
        light.innerConeAngle = 0.78539816339;
        light.outerConeAngle = 0.15915494309;
        light.position = [1, 1, 1];
		light.rotation = [1, 1, 1, 1];
        light.color = [0, 0, 0];
		light.isVisible = false;
        light.isARVisible = false;
        light.isVirtualVisible = false;
        light.cookieAssetId = "TestLightCookieAssetId";
        light.cookieAssetCollectionId = "TestLightCookieAssetCollectionId";
        light.lightCookieType = 0;

    )xx";

    scriptComponent->SetScriptSource(lightSpaceScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    realtimeEngine->ProcessPendingEntityOperations();

    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);

    EXPECT_EQ(lightComponent->GetLightType(), LightType::Spot);
    EXPECT_EQ(lightComponent->GetIntensity(), 10000.f);
    EXPECT_EQ(lightComponent->GetRange(), 5000.f);
    EXPECT_EQ(lightComponent->GetInnerConeAngle(), 0.78539816339f);
    EXPECT_EQ(lightComponent->GetOuterConeAngle(), 0.15915494309f);
    EXPECT_EQ(lightComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(lightComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(lightComponent->GetColor(), csp::common::Vector3(0, 0, 0));
    EXPECT_EQ(lightComponent->GetIsVisible(), false);
    EXPECT_EQ(lightComponent->GetIsARVisible(), false);
    EXPECT_EQ(lightComponent->GetIsVirtualVisible(), false);
    EXPECT_EQ(lightComponent->GetLightCookieAssetId(), "TestLightCookieAssetId");
    EXPECT_EQ(lightComponent->GetLightCookieAssetCollectionId(), "TestLightCookieAssetCollectionId");
    EXPECT_EQ(lightComponent->GetLightCookieType(), LightCookieType::ImageCookie);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
