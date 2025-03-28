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

#include "AssetSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>

using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void CreateMaterial(AssetSystem* AssetSystem, const csp::common::String& Name, const csp::common::String& SpaceId,
    const csp::common::Map<csp::common::String, csp::common::String>& Metadata, const csp::common::Array<csp::common::String>& AssetTags,
    GLTFMaterial& OutMaterial)
{
    auto [Result] = AWAIT_PRE(AssetSystem, CreateMaterial, RequestPredicate, Name, SpaceId, Metadata, AssetTags);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const GLTFMaterial& Material = Result.GetGLTFMaterial();
    EXPECT_EQ(Material.GetName(), Name);

    OutMaterial = Result.GetGLTFMaterial();
}

void UpdateMaterial(AssetSystem* AssetSystem, const GLTFMaterial& Material)
{
    auto [Result] = AWAIT_PRE(AssetSystem, UpdateMaterial, RequestPredicate, Material);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void DeleteMaterial(AssetSystem* AssetSystem, const GLTFMaterial& Material)
{
    auto [Result] = AWAIT_PRE(AssetSystem, DeleteMaterial, RequestPredicate, Material);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

void GetMaterials(AssetSystem* AssetSystem, const csp::common::String& SpaceId, csp::common::Array<GLTFMaterial>& OutMaterials)
{
    auto [Result] = AWAIT_PRE(AssetSystem, GetMaterials, RequestPredicate, SpaceId);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    OutMaterials = Result.GetGLTFMaterials();
}

void GetMaterial(AssetSystem* AssetSystem, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId,
    GLTFMaterial& OutMaterial, csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = AWAIT_PRE(AssetSystem, GetMaterial, RequestPredicate, AssetCollectionId, AssetId);
    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

    OutMaterial = Result.GetGLTFMaterial();
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateMaterialTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    GLTFMaterial CreatedMaterial;
    CreateMaterial(AssetSystem, "TestMaterial", Space.Id, {}, {}, CreatedMaterial);

    // Cleanup
    DeleteMaterial(AssetSystem, CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, UpdateMaterialTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create a material associated with the space
    GLTFMaterial CreatedMaterial;
    CreateMaterial(AssetSystem, "TestMaterial", Space.Id, {}, {}, CreatedMaterial);

    // Ensure the material can be updated
    EXPECT_NE(CreatedMaterial.GetAlphaCutoff(), 1);

    CreatedMaterial.SetAlphaCutoff(1);
    UpdateMaterial(AssetSystem, CreatedMaterial);

    // Get the material to ensure change have been made
    // More comprehensive material setter/serialization tests exist in:
    // InternalTests/MaterialUnitTests
    GLTFMaterial UpdatedMaterial;
    GetMaterial(AssetSystem, CreatedMaterial.GetMaterialCollectionId(), CreatedMaterial.GetMaterialId(), UpdatedMaterial);

    EXPECT_EQ(UpdatedMaterial.GetAlphaCutoff(), 1);

    // Cleanup
    DeleteMaterial(AssetSystem, CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetEmptyMaterialsTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to search for materials
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Attempt to find materials in a space none have been created for
    csp::common::Array<GLTFMaterial> FoundMaterials;
    GetMaterials(AssetSystem, Space.Id, FoundMaterials);

    EXPECT_TRUE(FoundMaterials.IsEmpty());

    // Cleanup
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMultipleMaterialsTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to search for materials
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create 3 materials
    constexpr const char* TestMaterialName1 = "TestMaterial1";
    GLTFMaterial CreatedMaterial1;
    CreateMaterial(AssetSystem, TestMaterialName1, Space.Id, {}, {}, CreatedMaterial1);

    constexpr const char* TestMaterialName2 = "TestMaterial2";
    GLTFMaterial CreatedMaterial2;
    CreateMaterial(AssetSystem, TestMaterialName2, Space.Id, {}, {}, CreatedMaterial2);

    constexpr const char* TestMaterialName3 = "TestMaterial3";
    GLTFMaterial CreatedMaterial3;
    CreateMaterial(AssetSystem, TestMaterialName3, Space.Id, {}, {}, CreatedMaterial3);

    // Attempt to find the 3 materials that have been created
    csp::common::Array<GLTFMaterial> FoundMaterials;
    GetMaterials(AssetSystem, Space.Id, FoundMaterials);

    EXPECT_EQ(FoundMaterials.Size(), 3);

    // Ensure we found the right materials
    std::vector<csp::common::String> MaterialNames { TestMaterialName1, TestMaterialName2, TestMaterialName3 };
    std::vector<csp::common::String> MaterialCollectionIds {
        CreatedMaterial1.GetMaterialCollectionId(),
        CreatedMaterial2.GetMaterialCollectionId(),
        CreatedMaterial3.GetMaterialCollectionId(),
    };
    std::vector<csp::common::String> MaterialIds {
        CreatedMaterial1.GetMaterialId(),
        CreatedMaterial2.GetMaterialId(),
        CreatedMaterial3.GetMaterialId(),
    };

    for (int i = 0; i < FoundMaterials.Size(); ++i)
    {
        const csp::common::String& SearchName = FoundMaterials[i].GetName();
        const csp::common::String& SearchCollectionId = FoundMaterials[i].GetMaterialCollectionId();
        const csp::common::String& SearchId = FoundMaterials[i].GetMaterialId();

        auto FoundName = std::find_if(
            std::begin(MaterialNames), std::end(MaterialNames), [&SearchName](const csp::common::String& Name) { return Name == SearchName; });

        EXPECT_TRUE(FoundName != MaterialNames.end());

        auto FoundCollectionId = std::find_if(std::begin(MaterialCollectionIds), std::end(MaterialCollectionIds),
            [&SearchCollectionId](const csp::common::String& CollectionId) { return CollectionId == SearchCollectionId; });

        EXPECT_TRUE(FoundCollectionId != MaterialCollectionIds.end());

        auto FoundId
            = std::find_if(std::begin(MaterialIds), std::end(MaterialIds), [&SearchId](const csp::common::String& Id) { return Id == SearchId; });

        EXPECT_TRUE(FoundId != MaterialIds.end());
    }

    // Cleanup
    DeleteMaterial(AssetSystem, CreatedMaterial1);
    DeleteMaterial(AssetSystem, CreatedMaterial2);
    DeleteMaterial(AssetSystem, CreatedMaterial3);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMaterialTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create a material associated with the space
    GLTFMaterial CreatedMaterial;
    CreateMaterial(AssetSystem, "TestMaterial", Space.Id, {}, {}, CreatedMaterial);

    // Get the material to ensure it can be found
    GLTFMaterial FoundMaterial;
    GetMaterial(AssetSystem, CreatedMaterial.GetMaterialCollectionId(), CreatedMaterial.GetMaterialId(), FoundMaterial);

    EXPECT_EQ(FoundMaterial.GetName(), CreatedMaterial.GetName());
    EXPECT_EQ(FoundMaterial.GetMaterialCollectionId(), CreatedMaterial.GetMaterialCollectionId());
    EXPECT_EQ(FoundMaterial.GetMaterialId(), CreatedMaterial.GetMaterialId());

    // Cleanup
    DeleteMaterial(AssetSystem, CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetInvalidMaterialTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create a material so we have one in this space
    GLTFMaterial CreatedMaterial;
    CreateMaterial(AssetSystem, "TestMaterial", Space.Id, {}, {}, CreatedMaterial);

    // Attempt to get an invalid material
    GLTFMaterial FoundMaterial;
    GetMaterial(AssetSystem, "InvalidAssetCollectionId", "InvalidAssetId", FoundMaterial, csp::systems::EResultCode::Failed);

    // Cleanup
    DeleteMaterial(AssetSystem, CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, DeleteMaterialTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to search for materials
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Create 2 materials
    constexpr const char* TestMaterialName1 = "TestMaterial1";
    GLTFMaterial CreatedMaterial1;
    CreateMaterial(AssetSystem, TestMaterialName1, Space.Id, {}, {}, CreatedMaterial1);

    constexpr const char* TestMaterialName2 = "TestMaterial2";
    GLTFMaterial CreatedMaterial2;
    CreateMaterial(AssetSystem, TestMaterialName2, Space.Id, {}, {}, CreatedMaterial2);

    // Delete first material
    DeleteMaterial(AssetSystem, CreatedMaterial1);

    // Ensure the deletion is correct
    GLTFMaterial DeletedMaterial;
    GetMaterial(AssetSystem, CreatedMaterial1.GetMaterialCollectionId(), CreatedMaterial1.GetMaterialId(), DeletedMaterial,
        csp::systems::EResultCode::Failed);

    // Ensure the other material still exists
    GLTFMaterial RemainingMaterial;
    GetMaterial(AssetSystem, CreatedMaterial2.GetMaterialCollectionId(), CreatedMaterial2.GetMaterialId(), RemainingMaterial);

    // Delete second material
    DeleteMaterial(AssetSystem, CreatedMaterial2);

    // Ensure the final material is deleted
    GetMaterial(AssetSystem, CreatedMaterial2.GetMaterialCollectionId(), CreatedMaterial2.GetMaterialId(), RemainingMaterial,
        csp::systems::EResultCode::Failed);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, MaterialEventTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space so we can get the material events
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    GLTFMaterial CreatedMaterial;

    // Create material and listen for event
    {
        bool CallbackCalled = false;

        auto CB = [&CallbackCalled, &CreatedMaterial](const csp::multiplayer::MaterialChangedParams& Params)
        {
            EXPECT_EQ(Params.MaterialCollectionId, CreatedMaterial.GetMaterialCollectionId());
            EXPECT_EQ(Params.MaterialId, CreatedMaterial.GetMaterialId());

            EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Created);

            CallbackCalled = true;
        };

        AssetSystem->SetMaterialChangedCallback(CB);

        // Create a material associated with the space
        CreateMaterial(AssetSystem, "TestMaterial", Space.Id, {}, {}, CreatedMaterial);
        WaitForCallback(CallbackCalled);

        EXPECT_TRUE(CallbackCalled);
    }

    // Update material and listen for event
    {
        bool CallbackCalled2 = false;

        auto CB = [&CallbackCalled2, &CreatedMaterial](const csp::multiplayer::MaterialChangedParams& Params)
        {
            EXPECT_EQ(Params.MaterialCollectionId, CreatedMaterial.GetMaterialCollectionId());
            EXPECT_EQ(Params.MaterialId, CreatedMaterial.GetMaterialId());

            EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Updated);

            CallbackCalled2 = true;
        };

        AssetSystem->SetMaterialChangedCallback(CB);

        CreatedMaterial.SetAlphaCutoff(1);
        UpdateMaterial(AssetSystem, CreatedMaterial);
        WaitForCallback(CallbackCalled2);

        EXPECT_TRUE(CallbackCalled2);
    }

    // Delete material and listen for event
    {
        bool CallbackCalled3 = false;

        auto CB = [&CallbackCalled3, &CreatedMaterial](const csp::multiplayer::MaterialChangedParams& Params)
        {
            EXPECT_EQ(Params.MaterialCollectionId, CreatedMaterial.GetMaterialCollectionId());
            EXPECT_EQ(Params.MaterialId, CreatedMaterial.GetMaterialId());

            EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Deleted);

            CallbackCalled3 = true;
        };

        AssetSystem->SetMaterialChangedCallback(CB);

        DeleteMaterial(AssetSystem, CreatedMaterial);
        WaitForCallback(CallbackCalled3);

        EXPECT_TRUE(CallbackCalled3);
    }

    // Cleanup
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, MaterialAssetEventTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
    const char* TestAssetName = "OLY-UNITTEST-ASSET-REWIND";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    char UniqueAssetName[256];
    SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space so we can get the material and asset events
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    // Setup Asset callback
    bool AssetDetailBlobChangedCallbackCalled = false;
    csp::common::String CallbackAssetId;

    auto AssetDetailBlobChangedCallback
        = [&AssetDetailBlobChangedCallbackCalled, &CallbackAssetId](const csp::multiplayer::AssetDetailBlobParams& Params)
    {
        if (AssetDetailBlobChangedCallbackCalled)
        {
            return;
        }

        EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Created);
        EXPECT_EQ(Params.AssetType, csp::systems::EAssetType::MODEL);

        CallbackAssetId = Params.AssetId;
        AssetDetailBlobChangedCallbackCalled = true;
    };

    AssetSystem->SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallback);

    GLTFMaterial CreatedMaterial;

    // Create material and listen for event
    bool CallbackCalled = false;

    auto CB = [&CallbackCalled, &CreatedMaterial](const csp::multiplayer::MaterialChangedParams& Params)
    {
        EXPECT_EQ(Params.MaterialCollectionId, CreatedMaterial.GetMaterialCollectionId());
        EXPECT_EQ(Params.MaterialId, CreatedMaterial.GetMaterialId());

        EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Created);

        CallbackCalled = true;
    };

    AssetSystem->SetMaterialChangedCallback(CB);

    // Create asset collection
    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

    // Create asset
    csp::systems::Asset Asset;
    CreateAsset(AssetSystem, AssetCollection, UniqueAssetName, nullptr, nullptr, Asset);

    // Upload data
    auto FilePath = std::filesystem::absolute("assets/test.json");
    csp::systems::FileAssetDataSource Source;
    Source.FilePath = FilePath.u8string().c_str();

    Source.SetMimeType("application/json");

    csp::common::String Uri;
    UploadAssetData(AssetSystem, AssetCollection, Asset, Source, Uri);

    WaitForCallback(AssetDetailBlobChangedCallbackCalled);

    EXPECT_TRUE(AssetDetailBlobChangedCallbackCalled);
    EXPECT_EQ(CallbackAssetId, Asset.Id);

    // Create a material associated with the space
    CreateMaterial(AssetSystem, "TestMaterial", Space.Id, {}, {}, CreatedMaterial);
    WaitForCallback(CallbackCalled);

    EXPECT_TRUE(CallbackCalled);

    // Cleanup
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}