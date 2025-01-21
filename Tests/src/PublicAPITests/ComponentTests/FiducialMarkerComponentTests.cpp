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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

#if RUN_ALL_UNIT_TESTS || RUN_FIDUCIALMARKER_TESTS || RUN_FIDUCIALMARKER_TEST
CSP_PUBLIC_TEST(CSPEngine, FiducialMarkerTests, FiducialMarkerComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
    const char* TestAssetName = "OLY-UNITTEST-ASSET-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    bool AssetDetailBlobChangedCallbackCalled = false;
    csp::common::String CallbackAssetId;

    const csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    auto [Object] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    const csp::common::String ModelAssetId = "NotARealId";

    auto* FiducialMarkerSpaceComponentInstance = (FiducialMarkerSpaceComponent*)Object->AddComponent(ComponentType::FiducialMarker);

    // Process component creation
    Object->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

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

    EXPECT_EQ(FiducialMarkerSpaceComponentInstance->GetIsARVisible(), true);
    EXPECT_EQ(FiducialMarkerSpaceComponentInstance->GetIsVisible(), true);

    FiducialMarkerSpaceComponentInstance->SetAssetCollectionId(Asset.AssetCollectionId);
    FiducialMarkerSpaceComponentInstance->SetMarkerAssetId(Asset.Id);
    FiducialMarkerSpaceComponentInstance->SetIsARVisible(false);

    auto FiducialMarkerSpaceComponentKey = FiducialMarkerSpaceComponentInstance->GetId();
    auto* StoredFiducialMarkerSpaceComponent = (FiducialMarkerSpaceComponent*)Object->GetComponent(FiducialMarkerSpaceComponentKey);

    EXPECT_EQ(StoredFiducialMarkerSpaceComponent->GetAssetCollectionId(), Asset.AssetCollectionId);
    EXPECT_EQ(StoredFiducialMarkerSpaceComponent->GetMarkerAssetId(), Asset.Id);
    EXPECT_EQ(StoredFiducialMarkerSpaceComponent->GetIsARVisible(), false);
    EXPECT_EQ(FiducialMarkerSpaceComponentInstance->GetIsVisible(), true);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_FIDUCIALMARKER_TESTS || RUN_FIDUCIALMARKER_SCRIPT_INTERFACE_TEST
CSP_PUBLIC_TEST(CSPEngine, FiducialMarkerTests, FiducialMarkerScriptInterfaceTest)
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

    // Create object to represent the image
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    // Create image component
    auto* FiducialMarkerComponent = (FiducialMarkerSpaceComponent*)CreatedObject->AddComponent(ComponentType::FiducialMarker);
    // Create script component
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);

    CreatedObject->QueueUpdate();
    EntitySystem->ProcessPendingEntityOperations();

    EXPECT_EQ(FiducialMarkerComponent->GetIsVisible(), true);

    // Setup script
    const std::string FiducialMarkerScriptText = R"xx(
	
		var marker = ThisEntity.getFiducialMarkerComponents()[0];
		
		marker.isVisible = false;
    )xx";

    ScriptComponent->SetScriptSource(FiducialMarkerScriptText.c_str());
    CreatedObject->GetScript()->Invoke();

    EntitySystem->ProcessPendingEntityOperations();

    const bool ScriptHasErrors = CreatedObject->GetScript()->HasError();
    EXPECT_FALSE(ScriptHasErrors);

    EXPECT_EQ(FiducialMarkerComponent->GetIsVisible(), false);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

} // namespace