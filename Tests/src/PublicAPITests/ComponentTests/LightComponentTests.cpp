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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, LightTests, LightComponentFieldsTest)
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

    RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String ModelAssetId = "NotARealId";

    auto* LightSpaceComponentInstance = (LightSpaceComponent*)Object->AddComponent(ComponentType::Light);

    // Process component creation
    Object->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& Components = *Object->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

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
    Asset.FileName = "OKO.png";
    Asset.Name = "OKO";
    Asset.Type = csp::systems::EAssetType::IMAGE;

    auto UploadFilePath = std::filesystem::absolute("assets/OKO.png");
    FILE* UploadFile = fopen(UploadFilePath.string().c_str(), "rb");
    uintmax_t UploadFileSize = std::filesystem::file_size(UploadFilePath);
    auto* UploadFileData = new unsigned char[UploadFileSize];
    fread(UploadFileData, UploadFileSize, 1, UploadFile);
    fclose(UploadFile);

    csp::systems::BufferAssetDataSource BufferSource;
    BufferSource.Buffer = UploadFileData;
    BufferSource.BufferLength = UploadFileSize;

    BufferSource.SetMimeType("image/png");

    printf("Uploading asset data...\n");

    // Upload data
    UploadAssetData(AssetSystem, AssetCollection, Asset, BufferSource, Asset.Uri);

    delete[] UploadFileData;

    EXPECT_EQ(LightSpaceComponentInstance->GetLightCookieType(), LightCookieType::NoCookie);
    EXPECT_EQ(LightSpaceComponentInstance->GetLightType(), LightType::Point);
    EXPECT_EQ(LightSpaceComponentInstance->GetLightShadowType(), LightShadowType::None);

    // test values
    const float InnerConeAngle = 10.0f;
    const float OuterConeAngle = 20.0f;
    const float Range = 120.0f;
    const float Intensity = 1000.0f;

    LightSpaceComponentInstance->SetLightCookieType(LightCookieType::ImageCookie);
    LightSpaceComponentInstance->SetLightCookieAssetCollectionId(Asset.AssetCollectionId);
    LightSpaceComponentInstance->SetLightCookieAssetId(Asset.Id);
    LightSpaceComponentInstance->SetLightType(LightType::Spot);
    LightSpaceComponentInstance->SetLightShadowType(LightShadowType::Realtime);
    LightSpaceComponentInstance->SetInnerConeAngle(InnerConeAngle);
    LightSpaceComponentInstance->SetOuterConeAngle(OuterConeAngle);
    LightSpaceComponentInstance->SetRange(Range);
    LightSpaceComponentInstance->SetIntensity(Intensity);

    auto LightSpaceComponentKey = LightSpaceComponentInstance->GetId();
    auto* StoredLightSpaceComponentInstance = (LightSpaceComponent*)Object->GetComponent(LightSpaceComponentKey);

    EXPECT_EQ(StoredLightSpaceComponentInstance->GetLightCookieType(), LightCookieType::ImageCookie);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetLightCookieAssetCollectionId(), Asset.AssetCollectionId);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetLightCookieAssetId(), Asset.Id);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetLightType(), LightType::Spot);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetLightShadowType(), LightShadowType::Realtime);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetInnerConeAngle(), InnerConeAngle);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetOuterConeAngle(), OuterConeAngle);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetRange(), Range);
    EXPECT_EQ(StoredLightSpaceComponentInstance->GetIntensity(), Intensity);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete asset
    DeleteAsset(AssetSystem, AssetCollection, Asset);

    // Delete asset collection
    DeleteAssetCollection(AssetSystem, AssetCollection);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LightTests, ActionHandlerTest)
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

    RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    const csp::common::String ModelAssetId = "NotARealId";

    auto* LightSpaceComponentInstance = (LightSpaceComponent*)Object->AddComponent(ComponentType::Light);

    // Process component creation
    Object->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Check component was created
    auto& Components = *Object->GetComponents();
    EXPECT_EQ(Components.Size(), 1);

    bool ActionCalled = false;
    LightSpaceComponentInstance->RegisterActionHandler(
        "TestAction", [&ActionCalled](ComponentBase*, const csp::common::String, const csp::common::String) { ActionCalled = true; });

    LightSpaceComponentInstance->InvokeAction("TestAction", "TestParam");

    EXPECT_TRUE(ActionCalled);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, LightTests, LightSpaceScriptInterfaceTest)
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

    RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create parent entity
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create light component
    auto* LightComponent = (LightSpaceComponent*)CreatedObject->AddComponent(ComponentType::Light);

    // Create script component
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    EXPECT_EQ(LightComponent->GetLightType(), LightType::Point);
    EXPECT_EQ(LightComponent->GetIntensity(), 5000.f);
    EXPECT_EQ(LightComponent->GetRange(), 1000.f);
    EXPECT_EQ(LightComponent->GetInnerConeAngle(), 0.f);
    EXPECT_EQ(LightComponent->GetOuterConeAngle(), 0.78539816339f);
    EXPECT_EQ(LightComponent->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(LightComponent->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(LightComponent->GetColor(), csp::common::Vector3(255, 255, 255));
    EXPECT_EQ(LightComponent->GetIsVisible(), true);
    EXPECT_EQ(LightComponent->GetIsARVisible(), true);
    EXPECT_EQ(LightComponent->GetIsVRVisible(), true);
    EXPECT_EQ(LightComponent->GetLightCookieAssetId(), "");
    EXPECT_EQ(LightComponent->GetLightCookieType(), LightCookieType::NoCookie);

    // Setup script
    const std::string LightSpaceScriptText = R"xx(

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
        light.isVRVisible = false;
        light.cookieAssetId = "TestLightCookieAssetId";
        light.lightCookieType = 0;

    )xx";

    ScriptComponent->SetScriptSource(LightSpaceScriptText.c_str());
    CreatedObject->GetScript().Invoke();

    RealtimeEngine->ProcessPendingEntityOperations();

    const bool ScriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(ScriptHasErrors);

    EXPECT_EQ(LightComponent->GetLightType(), LightType::Spot);
    EXPECT_EQ(LightComponent->GetIntensity(), 10000.f);
    EXPECT_EQ(LightComponent->GetRange(), 5000.f);
    EXPECT_EQ(LightComponent->GetInnerConeAngle(), 0.78539816339f);
    EXPECT_EQ(LightComponent->GetOuterConeAngle(), 0.15915494309f);
    EXPECT_EQ(LightComponent->GetPosition(), csp::common::Vector3::One());
    EXPECT_EQ(LightComponent->GetRotation(), csp::common::Vector4::One());
    EXPECT_EQ(LightComponent->GetColor(), csp::common::Vector3(0, 0, 0));
    EXPECT_EQ(LightComponent->GetIsVisible(), false);
    EXPECT_EQ(LightComponent->GetIsARVisible(), false);
    EXPECT_EQ(LightComponent->GetIsVRVisible(), false);
    EXPECT_EQ(LightComponent->GetLightCookieAssetId(), "TestLightCookieAssetId");
    EXPECT_EQ(LightComponent->GetLightCookieType(), LightCookieType::ImageCookie);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
