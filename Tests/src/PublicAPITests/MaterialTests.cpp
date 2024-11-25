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

#include "Awaitable.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

} // namespace

void CreateMaterial(AssetSystem* AssetSystem, const csp::common::String& Name, const csp::common::String& SpaceId, GLTFMaterial& OutMaterial)
{
	auto [Result] = AWAIT_PRE(AssetSystem, CreateMaterial, RequestPredicate, Name, SpaceId);

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

void GetMaterial(AssetSystem* AssetSystem,
				 const csp::common::String& AssetCollectionId,
				 const csp::common::String& AssetId,
				 GLTFMaterial& OutMaterial,
				 csp::systems::EResultCode ExpectedResultCode				   = csp::systems::EResultCode::Success,
				 csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None)
{
	auto [Result] = AWAIT_PRE(AssetSystem, GetMaterial, RequestPredicate, AssetCollectionId, AssetId);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

	OutMaterial = Result.GetGLTFMaterial();
}

#if RUN_ALL_UNIT_TESTS || RUN_MATERIAL_TESTS || RUN_MATERIAL_CREATEMATERIAL_TEST

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateMaterialTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space to associate a material with
	::Space Space;
	CreateDefaultTestSpace(SpaceSystem, Space);

	GLTFMaterial CreatedMaterial;
	CreateMaterial(AssetSystem, "TestMaterial", Space.Id, CreatedMaterial);

	// Cleanup
	DeleteMaterial(AssetSystem, CreatedMaterial);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MATERIAL_TESTS || RUN_MATERIAL_UPDATEMATERIAL_TEST

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, UpdateMaterialTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space to associate a material with
	::Space Space;
	CreateDefaultTestSpace(SpaceSystem, Space);

	// Create a material associated with the space
	GLTFMaterial CreatedMaterial;
	CreateMaterial(AssetSystem, "TestMaterial", Space.Id, CreatedMaterial);

	// Ensure the material can be updated
	EXPECT_NE(CreatedMaterial.GetAlphaCutoff(), 1);

	CreatedMaterial.SetAlphaCutoff(1);
	UpdateMaterial(AssetSystem, CreatedMaterial);

	// Get the material to ensure change have been made
	// More comprehensive material setter/serialization tests exist in:
	// InternalTests/MaterialUnitTests
	GLTFMaterial UpdatedMaterial;
	GetMaterial(AssetSystem, CreatedMaterial.GetAssetCollectionId(), CreatedMaterial.GetAssetId(), UpdatedMaterial);

	EXPECT_EQ(UpdatedMaterial.GetAlphaCutoff(), 1);

	// Cleanup
	DeleteMaterial(AssetSystem, CreatedMaterial);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MATERIAL_TESTS || RUN_MATERIAL_GETEMPTYMATERIALS_TEST

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetEmptyMaterialsTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

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
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MATERIAL_TESTS || RUN_MATERIAL_GETMULTIPLEMATERIALS_TEST

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMultipleMaterialsTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space to search for materials
	::Space Space;
	CreateDefaultTestSpace(SpaceSystem, Space);

	// Create 3 materials
	constexpr const char* TestMaterialName1 = "TestMaterial1";
	GLTFMaterial CreatedMaterial1;
	CreateMaterial(AssetSystem, TestMaterialName1, Space.Id, CreatedMaterial1);

	constexpr const char* TestMaterialName2 = "TestMaterial2";
	GLTFMaterial CreatedMaterial2;
	CreateMaterial(AssetSystem, TestMaterialName2, Space.Id, CreatedMaterial2);

	constexpr const char* TestMaterialName3 = "TestMaterial3";
	GLTFMaterial CreatedMaterial3;
	CreateMaterial(AssetSystem, TestMaterialName3, Space.Id, CreatedMaterial3);

	// Attempt to find the 3 materials that have been created
	csp::common::Array<GLTFMaterial> FoundMaterials;
	GetMaterials(AssetSystem, Space.Id, FoundMaterials);

	EXPECT_EQ(FoundMaterials.Size(), 3);

	// Ensure we found the right materials
	std::vector<csp::common::String> MaterialNames {TestMaterialName1, TestMaterialName2, TestMaterialName3};

	for (int i = 0; i < FoundMaterials.Size(); ++i)
	{
		const csp::common::String& SearchName = FoundMaterials[i].GetName();

		auto Found = std::find_if(std::begin(MaterialNames),
								  std::end(MaterialNames),
								  [&SearchName](const csp::common::String& Name)
								  {
									  return Name == SearchName;
								  });


		EXPECT_TRUE(Found != MaterialNames.end());
	}

	// Cleanup
	DeleteMaterial(AssetSystem, CreatedMaterial1);
	DeleteMaterial(AssetSystem, CreatedMaterial2);
	DeleteMaterial(AssetSystem, CreatedMaterial3);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MATERIAL_TESTS || RUN_MATERIAL_GETMATERIAL_TEST

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMaterialTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space to associate a material with
	::Space Space;
	CreateDefaultTestSpace(SpaceSystem, Space);

	// Create a material associated with the space
	GLTFMaterial CreatedMaterial;
	CreateMaterial(AssetSystem, "TestMaterial", Space.Id, CreatedMaterial);

	// Get the material to ensure it can be found
	GLTFMaterial FoundMaterial;
	GetMaterial(AssetSystem, CreatedMaterial.GetAssetCollectionId(), CreatedMaterial.GetAssetId(), FoundMaterial);

	EXPECT_EQ(FoundMaterial.GetName(), CreatedMaterial.GetName());

	// Cleanup
	DeleteMaterial(AssetSystem, CreatedMaterial);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MATERIAL_TESTS || RUN_MATERIAL_GETINVALIDMATERIAL_TEST

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetInvalidMaterialTest)
{
	SetRandSeed();

	auto& SystemsManager = ::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space to associate a material with
	::Space Space;
	CreateDefaultTestSpace(SpaceSystem, Space);

	// Create a material so we have one in this space
	GLTFMaterial CreatedMaterial;
	CreateMaterial(AssetSystem, "TestMaterial", Space.Id, CreatedMaterial);

	// Attempt to get an invalid material
	GLTFMaterial FoundMaterial;
	GetMaterial(AssetSystem, "InvalidAssetCollectionId", "InvalidAssetId", FoundMaterial, csp::systems::EResultCode::Failed);

	// Cleanup
	DeleteMaterial(AssetSystem, CreatedMaterial);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif