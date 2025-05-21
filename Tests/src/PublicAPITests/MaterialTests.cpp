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
#include <future>

using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void CreateMaterial(AssetSystem* AssetSystem, const csp::common::String& Name, const csp::systems::EShaderType shaderType,
    const csp::common::String& SpaceId, const csp::common::Map<csp::common::String, csp::common::String>& Metadata,
    const csp::common::Array<csp::common::String>& AssetTags, Material** OutMaterial,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success)
{
    auto [Result] = AWAIT_PRE(AssetSystem, CreateMaterial, RequestPredicate, Name, shaderType, SpaceId, Metadata, AssetTags);
    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
    {
        *OutMaterial = nullptr;
        return;
    }

    csp::systems::MaterialResult Result2 = Result;

    Material* Material = Result2.GetMaterial();
    EXPECT_EQ(Material->GetName(), Name);

    *OutMaterial = Result2.GetMaterial();
}

void UpdateMaterial(AssetSystem* AssetSystem, const Material& Material)
{
    // Future/Promise required here as Awaitable makes a copy when constructing the tuple.
    std::promise<NullResult> ResultPromise;
    std::future<NullResult> ResultFuture = ResultPromise.get_future();

    NullResultCallback Callback = [&ResultPromise](NullResult Result)
    {
        if (Result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        ResultPromise.set_value(Result);
    };

    AssetSystem->UpdateMaterial(Material, Callback);

    ResultFuture.wait();
}

void DeleteMaterial(AssetSystem* AssetSystem, const Material& Material)
{
    // Future/Promise required here as Awaitable makes a copy when constructing the tuple.
    std::promise<NullResult> ResultPromise;
    std::future<NullResult> ResultFuture = ResultPromise.get_future();

    NullResultCallback Callback = [&ResultPromise](NullResult Result)
    {
        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        ResultPromise.set_value(Result);
    };

    AssetSystem->DeleteMaterial(Material, Callback);

    ResultFuture.wait();
}

void GetMaterials(AssetSystem* AssetSystem, const csp::common::String& SpaceId, csp::common::Array<Material*>& OutMaterials,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success)
{
    auto [Result] = AWAIT_PRE(AssetSystem, GetMaterials, RequestPredicate, SpaceId);
    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
    {
        return;
    }

    OutMaterials = *(Result.GetMaterials());
}

void GetMaterial(AssetSystem* AssetSystem, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId, Material** OutMaterial,
    csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason /*ExpectedResultFailureCode*/ = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = AWAIT_PRE(AssetSystem, GetMaterial, RequestPredicate, AssetCollectionId, AssetId);
    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
    {
        *OutMaterial = nullptr;
        return;
    }

    csp::systems::MaterialResult Result2 = Result;

    *OutMaterial = Result2.GetMaterial();
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateGLTFMaterialTest)
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

    // Create a standard material associated with the Space
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    GLTFMaterial* CreatedGLTFMaterial = dynamic_cast<GLTFMaterial*>(CreatedMaterial);
    EXPECT_NE(CreatedGLTFMaterial, nullptr);
#endif

    // Cleanup standard material
    DeleteMaterial(AssetSystem, *CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateAlphaVideoMaterialTest)
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

    // Create a alpha video material associated with the Space
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::AlphaVideo, Space.Id, {}, {}, &CreatedMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    AlphaVideoMaterial* CreatedAlphaVideoMaterial = dynamic_cast<AlphaVideoMaterial*>(CreatedMaterial);
    EXPECT_NE(CreatedAlphaVideoMaterial, nullptr);
#endif

    // Cleanup alpha video material
    DeleteMaterial(AssetSystem, *CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateIncorrectMaterialTypeTest)
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

    csp::systems::EShaderType IncorrectShaderType = static_cast<csp::systems::EShaderType>(3);

    // Attempt to create a material with an incorrect type
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", IncorrectShaderType, Space.Id, {}, {}, &CreatedMaterial, csp::systems::EResultCode::Failed);

    EXPECT_EQ(CreatedMaterial, nullptr);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, UpdateGLTFMaterialTest)
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

    // Create a standard material associated with the space
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);
    GLTFMaterial* CreatedGLTFMaterial = static_cast<GLTFMaterial*>(CreatedMaterial);

    // Ensure the material can be updated
    EXPECT_EQ(CreatedGLTFMaterial->GetAlphaCutoff(), 0.5f);

    CreatedGLTFMaterial->SetAlphaCutoff(1.0f);
    UpdateMaterial(AssetSystem, *CreatedMaterial);

    // Get the material to ensure change have been made
    Material* UpdatedMaterial = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialId(), &UpdatedMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    GLTFMaterial* UpdatedMaterialGLTF = dynamic_cast<GLTFMaterial*>(UpdatedMaterial);
    EXPECT_NE(UpdatedMaterialGLTF, nullptr);
    EXPECT_EQ(UpdatedMaterialGLTF->GetAlphaCutoff(), 1.0f);
#endif

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, UpdateAlphaVideoMaterialTest)
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

    // Create a alpha video material associated with the space
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::AlphaVideo, Space.Id, {}, {}, &CreatedMaterial);
    AlphaVideoMaterial* CreatedAlphaVideoMaterial = static_cast<AlphaVideoMaterial*>(CreatedMaterial);

    // Ensure the material can be updated
    EXPECT_EQ(CreatedAlphaVideoMaterial->GetAlphaFactor(), 1.0f);

    CreatedAlphaVideoMaterial->SetAlphaFactor(0.5f);
    UpdateMaterial(AssetSystem, *CreatedMaterial);
    EXPECT_EQ(CreatedAlphaVideoMaterial->GetAlphaFactor(), 0.5f);

    // Get the material to ensure change have been made
    Material* UpdatedMaterial = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialId(), &UpdatedMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    AlphaVideoMaterial* UpdatedMaterialAlphaVideo = dynamic_cast<AlphaVideoMaterial*>(UpdatedMaterial);
    EXPECT_NE(UpdatedMaterialAlphaVideo, nullptr);
    EXPECT_EQ(UpdatedMaterialAlphaVideo->GetAlphaFactor(), 0.5f);
#endif

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

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

    // Attempt to find materials in a Space none have been created for
    csp::common::Array<Material*> FoundMaterials;
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

    // Create 4 materials associated with the space - 2 alpha video, 2 standard
    constexpr const char* TestAlphaVideoMaterialName1 = "TestAlphaVideoMaterial1";
    Material* CreatedMaterial1 = nullptr;
    CreateMaterial(AssetSystem, TestAlphaVideoMaterialName1, csp::systems::EShaderType::AlphaVideo, Space.Id, {}, {}, &CreatedMaterial1);

    constexpr const char* TestStandardMaterialName1 = "TestStandardMaterial1";
    Material* CreatedMaterial2 = nullptr;
    CreateMaterial(AssetSystem, TestStandardMaterialName1, csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial2);

    constexpr const char* TestAlphaVideoMaterialName2 = "TestAlphaVideoMaterial2";
    Material* CreatedMaterial3 = nullptr;
    CreateMaterial(AssetSystem, TestAlphaVideoMaterialName2, csp::systems::EShaderType::AlphaVideo, Space.Id, {}, {}, &CreatedMaterial3);

    constexpr const char* TestStandardMaterialName2 = "TestStandardMaterial2";
    Material* CreatedMaterial4 = nullptr;
    CreateMaterial(AssetSystem, TestStandardMaterialName2, csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial4);

    // Attempt to find the 4 materials that have been created
    csp::common::Array<Material*> FoundMaterials;
    GetMaterials(AssetSystem, Space.Id, FoundMaterials);

    EXPECT_EQ(FoundMaterials.Size(), 4);

    // Ensure we found the right materials
    std::vector<csp::common::String> MaterialNames { TestAlphaVideoMaterialName1, TestStandardMaterialName1, TestAlphaVideoMaterialName2,
        TestStandardMaterialName2 };
    std::vector<csp::common::String> MaterialCollectionIds {
        CreatedMaterial1->GetMaterialCollectionId(),
        CreatedMaterial2->GetMaterialCollectionId(),
        CreatedMaterial3->GetMaterialCollectionId(),
        CreatedMaterial4->GetMaterialCollectionId(),
    };
    std::vector<csp::common::String> MaterialIds {
        CreatedMaterial1->GetMaterialId(),
        CreatedMaterial2->GetMaterialId(),
        CreatedMaterial3->GetMaterialId(),
        CreatedMaterial4->GetMaterialId(),
    };

    for (size_t i = 0; i < FoundMaterials.Size(); ++i)
    {
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
        if (FoundMaterials[i]->GetShaderType() == EShaderType::AlphaVideo)
        {
            AlphaVideoMaterial* FoundMaterialAlphaVideo = dynamic_cast<AlphaVideoMaterial*>(FoundMaterials[i]);
            EXPECT_NE(FoundMaterialAlphaVideo, nullptr);
        }
        else if (FoundMaterials[i]->GetShaderType() == EShaderType::Standard)
        {
            GLTFMaterial* FoundMaterialGLTF = dynamic_cast<GLTFMaterial*>(FoundMaterials[i]);
            EXPECT_NE(FoundMaterialGLTF, nullptr);
        }
#endif

        const csp::common::String& SearchName = FoundMaterials[i]->GetName();
        const csp::common::String& SearchCollectionId = FoundMaterials[i]->GetMaterialCollectionId();
        const csp::common::String& SearchId = FoundMaterials[i]->GetMaterialId();

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
    DeleteMaterial(AssetSystem, *CreatedMaterial1);
    DeleteMaterial(AssetSystem, *CreatedMaterial2);
    DeleteMaterial(AssetSystem, *CreatedMaterial3);
    DeleteMaterial(AssetSystem, *CreatedMaterial4);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetGLTFMaterialTest)
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

    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);

    // Get the material
    Material* FoundMaterial = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialId(), &FoundMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    GLTFMaterial* FoundGLTFMaterial = dynamic_cast<GLTFMaterial*>(FoundMaterial);
    EXPECT_NE(FoundGLTFMaterial, nullptr);
#endif

    EXPECT_EQ(FoundMaterial->GetShaderType(), CreatedMaterial->GetShaderType());
    EXPECT_EQ(FoundMaterial->GetName(), CreatedMaterial->GetName());
    EXPECT_EQ(FoundMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialCollectionId());
    EXPECT_EQ(FoundMaterial->GetMaterialId(), CreatedMaterial->GetMaterialId());

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetAlphaVideoMaterialTest)
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

    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::AlphaVideo, Space.Id, {}, {}, &CreatedMaterial);

    // Get the material
    Material* FoundMaterial = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialId(), &FoundMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    AlphaVideoMaterial* FoundAlphaVideoMaterial = dynamic_cast<AlphaVideoMaterial*>(FoundMaterial);
    EXPECT_NE(FoundAlphaVideoMaterial, nullptr);
#endif

    EXPECT_EQ(FoundMaterial->GetShaderType(), CreatedMaterial->GetShaderType());
    EXPECT_EQ(FoundMaterial->GetName(), CreatedMaterial->GetName());
    EXPECT_EQ(FoundMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialCollectionId());
    EXPECT_EQ(FoundMaterial->GetMaterialId(), CreatedMaterial->GetMaterialId());

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMaterialWithIncorrectShaderTypeTest)
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

    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);

    auto [Result1] = AWAIT_PRE(AssetSystem, GetAssetCollectionById, RequestPredicate, CreatedMaterial->GetMaterialCollectionId());
    EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

    AssetCollection OutAssetCollection = Result1.GetAssetCollection();

    // Create metadata for MaterialCollection with an invalid shader type
    csp::common::Map<csp::common::String, csp::common::String> InMetaData;
    InMetaData["ShaderType"] = "InvalidShaderType";

    auto [Result2] = AWAIT_PRE(AssetSystem, UpdateAssetCollectionMetadata, RequestPredicate, OutAssetCollection, InMetaData, nullptr);
    EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the material
    Material* FoundMaterial = nullptr;
    GetMaterial(
        AssetSystem, CreatedMaterial->GetMaterialCollectionId(), CreatedMaterial->GetMaterialId(), &FoundMaterial, csp::systems::EResultCode::Failed);

    EXPECT_EQ(FoundMaterial, nullptr);

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMaterialsWithIncorrectShaderTypeTest)
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

    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);

    auto [Result1] = AWAIT_PRE(AssetSystem, GetAssetCollectionById, RequestPredicate, CreatedMaterial->GetMaterialCollectionId());
    EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

    AssetCollection OutAssetCollection = Result1.GetAssetCollection();

    // Create metadata for MaterialCollection with an invalid shader type
    csp::common::Map<csp::common::String, csp::common::String> InMetaData;
    InMetaData["ShaderType"] = "InvalidShaderType";

    auto [Result2] = AWAIT_PRE(AssetSystem, UpdateAssetCollectionMetadata, RequestPredicate, OutAssetCollection, InMetaData, nullptr);
    EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the material using the GetMaterials method
    csp::common::Array<Material*> FoundMaterials;
    GetMaterials(AssetSystem, Space.Id, FoundMaterials, csp::systems::EResultCode::Failed);

    EXPECT_EQ(FoundMaterials.Size(), 0);

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

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
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, "TestMaterial", csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);

    // Attempt to get an invalid material
    Material* FoundMaterial = nullptr;
    GetMaterial(AssetSystem, "InvalidAssetCollectionId", "InvalidAssetId", &FoundMaterial, csp::systems::EResultCode::Failed);

    // Cleanup
    DeleteMaterial(AssetSystem, *CreatedMaterial);

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

    // Create 2 materials associated with the space
    constexpr const char* TestMaterialName1 = "TestStandardMaterial";
    Material* CreatedMaterial1 = nullptr;
    CreateMaterial(AssetSystem, TestMaterialName1, csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial1);

    constexpr const char* TestMaterialName2 = "TestAlphaVideoMaterial";
    Material* CreatedMaterial2 = nullptr;
    CreateMaterial(AssetSystem, TestMaterialName2, csp::systems::EShaderType::AlphaVideo, Space.Id, {}, {}, &CreatedMaterial2);

    // Delete first material
    DeleteMaterial(AssetSystem, *CreatedMaterial1);

    // Ensure the deletion has worked
    Material* DeletedMaterial1 = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial1->GetMaterialCollectionId(), CreatedMaterial1->GetMaterialId(), &DeletedMaterial1,
        csp::systems::EResultCode::Failed);

    // Make sure we can still get the second material
    Material* RemainingMaterial = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial2->GetMaterialCollectionId(), CreatedMaterial2->GetMaterialId(), &RemainingMaterial);

    // Delete second material
    DeleteMaterial(AssetSystem, *CreatedMaterial2);

    // Ensure the second material is deleted
    Material* DeletedMaterial2 = nullptr;
    GetMaterial(AssetSystem, CreatedMaterial2->GetMaterialCollectionId(), CreatedMaterial2->GetMaterialId(), &DeletedMaterial2,
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

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space to associate a material with
    ::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    // Enter space so we can get the material events
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    constexpr const char* TestMaterialName1 = "TestMaterial1";
    GLTFMaterial* CreatedGLTFMaterial = nullptr;

    // Create material and listen for event
    {
        bool CallbackCalled = false;

        auto CB = [&CallbackCalled, &CreatedGLTFMaterial](const csp::multiplayer::MaterialChangedParams& Params)
        {
            EXPECT_EQ(Params.MaterialCollectionId, CreatedGLTFMaterial->GetMaterialCollectionId());
            EXPECT_EQ(Params.MaterialId, CreatedGLTFMaterial->GetMaterialId());

            EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Created);

            CallbackCalled = true;
        };

        AssetSystem->SetMaterialChangedCallback(CB);

        // Create a material associated with the space
        Material* CreatedMaterial = nullptr;
        CreateMaterial(AssetSystem, TestMaterialName1, csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);
        CreatedGLTFMaterial = static_cast<GLTFMaterial*>(CreatedMaterial);

        WaitForCallback(CallbackCalled);

        EXPECT_TRUE(CallbackCalled);
    }

    // Update material and listen for event
    {
        bool CallbackCalled2 = false;

        auto CB = [&CallbackCalled2, &CreatedGLTFMaterial](const csp::multiplayer::MaterialChangedParams& Params)
        {
            EXPECT_EQ(Params.MaterialCollectionId, CreatedGLTFMaterial->GetMaterialCollectionId());
            EXPECT_EQ(Params.MaterialId, CreatedGLTFMaterial->GetMaterialId());

            EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Updated);

            CallbackCalled2 = true;
        };

        AssetSystem->SetMaterialChangedCallback(CB);

        CreatedGLTFMaterial->SetAlphaCutoff(1);
        UpdateMaterial(AssetSystem, *CreatedGLTFMaterial);
        WaitForCallback(CallbackCalled2);

        EXPECT_TRUE(CallbackCalled2);
    }

    // Delete material and listen for event
    {
        bool CallbackCalled3 = false;

        auto CB = [&CallbackCalled3, &CreatedGLTFMaterial](const csp::multiplayer::MaterialChangedParams& Params)
        {
            EXPECT_EQ(Params.MaterialCollectionId, CreatedGLTFMaterial->GetMaterialCollectionId());
            EXPECT_EQ(Params.MaterialId, CreatedGLTFMaterial->GetMaterialId());

            EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Deleted);

            CallbackCalled3 = true;
        };

        AssetSystem->SetMaterialChangedCallback(CB);

        DeleteMaterial(AssetSystem, *CreatedGLTFMaterial);
        WaitForCallback(CallbackCalled3);

        EXPECT_TRUE(CallbackCalled3);
    }

    // Cleanup
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

// This test is to be fixed as part of OF-1651.
CSP_PUBLIC_TEST(DISABLED_CSPEngine, MaterialTests, MaterialAssetEventTest)
{
    SetRandSeed();

    auto& SystemsManager = ::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION";
    const char* TestAssetName = "CSP-UNITTEST-ASSET";

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

    constexpr const char* TestMaterialName1 = "TestMaterial1";
    GLTFMaterial* CreatedGLTFMaterial = nullptr;

    // Create material and listen for event
    bool CallbackCalled = false;

    auto CB = [&CallbackCalled, &CreatedGLTFMaterial](const csp::multiplayer::MaterialChangedParams& Params)
    {
        EXPECT_EQ(Params.MaterialCollectionId, CreatedGLTFMaterial->GetMaterialCollectionId());
        EXPECT_EQ(Params.MaterialId, CreatedGLTFMaterial->GetMaterialId());

        EXPECT_EQ(Params.ChangeType, csp::multiplayer::EAssetChangeType::Created);

        CallbackCalled = true;
    };

    AssetSystem->SetMaterialChangedCallback(CB);

    // Create a material associated with the space
    Material* CreatedMaterial = nullptr;
    CreateMaterial(AssetSystem, TestMaterialName1, csp::systems::EShaderType::Standard, Space.Id, {}, {}, &CreatedMaterial);
    CreatedGLTFMaterial = static_cast<GLTFMaterial*>(CreatedMaterial);

    WaitForCallback(CallbackCalled);

    EXPECT_TRUE(CallbackCalled);

    // Cleanup
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
