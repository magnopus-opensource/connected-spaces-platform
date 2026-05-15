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

#include <algorithm>
#include <filesystem>
#include <future>

using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

void CreateMaterial(AssetSystem* assetSystem, const csp::common::String& name, const csp::systems::EShaderType shaderType,
    const csp::common::String& spaceId, const csp::common::Map<csp::common::String, csp::common::String>& metadata,
    const csp::common::Array<csp::common::String>& assetTags, Material** outMaterial,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success)
{
    auto [Result] = AWAIT_PRE(assetSystem, CreateMaterial, RequestPredicate, name, shaderType, spaceId, metadata, assetTags);
    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
    {
        *outMaterial = nullptr;
        return;
    }

    csp::systems::MaterialResult result2 = Result;

    Material* material = result2.GetMaterial();
    EXPECT_EQ(material->GetName(), name);

    *outMaterial = result2.GetMaterial();
}

void UpdateMaterial(AssetSystem* assetSystem, const Material& material)
{
    // Future/Promise required here as Awaitable makes a copy when constructing the tuple.
    std::promise<NullResult> resultPromise;
    std::future<NullResult> resultFuture = resultPromise.get_future();

    NullResultCallback callback = [&resultPromise](NullResult result)
    {
        if (result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);
        resultPromise.set_value(result);
    };

    assetSystem->UpdateMaterial(material, callback);

    resultFuture.wait();
}

void DeleteMaterial(AssetSystem* assetSystem, const Material& material)
{
    // Future/Promise required here as Awaitable makes a copy when constructing the tuple.
    std::promise<NullResult> resultPromise;
    std::future<NullResult> resultFuture = resultPromise.get_future();

    NullResultCallback callback = [&resultPromise](NullResult result)
    {
        EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);
        resultPromise.set_value(result);
    };

    assetSystem->DeleteMaterial(material, callback);

    resultFuture.wait();
}

void GetMaterials(AssetSystem* assetSystem, const csp::common::String& spaceId, csp::common::Array<Material*>& outMaterials,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success)
{
    auto [Result] = AWAIT_PRE(assetSystem, GetMaterials, RequestPredicate, spaceId);
    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
    {
        return;
    }

    outMaterials = *(Result.GetMaterials());
}

void GetMaterial(AssetSystem* assetSystem, const csp::common::String& assetCollectionId, const csp::common::String& assetId, Material** outMaterial,
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason /*ExpectedResultFailureCode*/ = csp::systems::ERequestFailureReason::None)
{
    auto [Result] = AWAIT_PRE(assetSystem, GetMaterial, RequestPredicate, assetCollectionId, assetId);
    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
    {
        *outMaterial = nullptr;
        return;
    }

    csp::systems::MaterialResult result2 = Result;

    *outMaterial = result2.GetMaterial();
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateGLTFMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create a standard material associated with the Space
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    GLTFMaterial* createdGltfMaterial = dynamic_cast<GLTFMaterial*>(createdMaterial);
    EXPECT_NE(createdGltfMaterial, nullptr);
#endif

    // Cleanup standard material
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateAlphaVideoMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create a alpha video material associated with the Space
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::AlphaVideo, space.Id, {}, {}, &createdMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    AlphaVideoMaterial* createdAlphaVideoMaterial = dynamic_cast<AlphaVideoMaterial*>(createdMaterial);
    EXPECT_NE(createdAlphaVideoMaterial, nullptr);
#endif

    // Cleanup alpha video material
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, CreateIncorrectMaterialTypeTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    csp::systems::EShaderType incorrectShaderType = static_cast<csp::systems::EShaderType>(3);

    // Attempt to create a material with an incorrect type
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", incorrectShaderType, space.Id, {}, {}, &createdMaterial, csp::systems::EResultCode::Failed);

    EXPECT_EQ(createdMaterial, nullptr);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, UpdateGLTFMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create a standard material associated with the space
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);
    GLTFMaterial* createdGltfMaterial = static_cast<GLTFMaterial*>(createdMaterial);

    // Ensure the material can be updated
    EXPECT_EQ(createdGltfMaterial->GetAlphaCutoff(), 0.5f);

    createdGltfMaterial->SetAlphaCutoff(1.0f);
    UpdateMaterial(assetSystem, *createdMaterial);

    // Get the material to ensure change have been made
    Material* updatedMaterial = nullptr;
    GetMaterial(assetSystem, createdMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialId(), &updatedMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    GLTFMaterial* updatedMaterialGltf = dynamic_cast<GLTFMaterial*>(updatedMaterial);
    EXPECT_NE(updatedMaterialGltf, nullptr);
    EXPECT_EQ(updatedMaterialGltf->GetAlphaCutoff(), 1.0f);
#endif

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, UpdateAlphaVideoMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create a alpha video material associated with the space
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::AlphaVideo, space.Id, {}, {}, &createdMaterial);
    AlphaVideoMaterial* createdAlphaVideoMaterial = static_cast<AlphaVideoMaterial*>(createdMaterial);

    // Ensure the material can be updated
    EXPECT_EQ(createdAlphaVideoMaterial->GetAlphaFactor(), 1.0f);

    createdAlphaVideoMaterial->SetAlphaFactor(0.5f);
    UpdateMaterial(assetSystem, *createdMaterial);
    EXPECT_EQ(createdAlphaVideoMaterial->GetAlphaFactor(), 0.5f);

    // Get the material to ensure change have been made
    Material* updatedMaterial = nullptr;
    GetMaterial(assetSystem, createdMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialId(), &updatedMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    AlphaVideoMaterial* updatedMaterialAlphaVideo = dynamic_cast<AlphaVideoMaterial*>(updatedMaterial);
    EXPECT_NE(updatedMaterialAlphaVideo, nullptr);
    EXPECT_EQ(updatedMaterialAlphaVideo->GetAlphaFactor(), 0.5f);
#endif

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetEmptyMaterialsTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to search for materials
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Attempt to find materials in a Space none have been created for
    csp::common::Array<Material*> foundMaterials;
    GetMaterials(assetSystem, space.Id, foundMaterials);

    EXPECT_TRUE(foundMaterials.IsEmpty());

    // Cleanup
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMultipleMaterialsTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to search for materials
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create 4 materials associated with the space - 2 alpha video, 2 standard
    constexpr const char* testAlphaVideoMaterialName1 = "TestAlphaVideoMaterial1";
    Material* createdMaterial1 = nullptr;
    CreateMaterial(assetSystem, testAlphaVideoMaterialName1, csp::systems::EShaderType::AlphaVideo, space.Id, {}, {}, &createdMaterial1);

    constexpr const char* testStandardMaterialName1 = "TestStandardMaterial1";
    Material* createdMaterial2 = nullptr;
    CreateMaterial(assetSystem, testStandardMaterialName1, csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial2);

    constexpr const char* testAlphaVideoMaterialName2 = "TestAlphaVideoMaterial2";
    Material* createdMaterial3 = nullptr;
    CreateMaterial(assetSystem, testAlphaVideoMaterialName2, csp::systems::EShaderType::AlphaVideo, space.Id, {}, {}, &createdMaterial3);

    constexpr const char* testStandardMaterialName2 = "TestStandardMaterial2";
    Material* createdMaterial4 = nullptr;
    CreateMaterial(assetSystem, testStandardMaterialName2, csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial4);

    // Attempt to find the 4 materials that have been created
    csp::common::Array<Material*> foundMaterials;
    GetMaterials(assetSystem, space.Id, foundMaterials);

    EXPECT_EQ(foundMaterials.Size(), 4);

    // Ensure we found the right materials
    std::vector<csp::common::String> materialNames { testAlphaVideoMaterialName1, testStandardMaterialName1, testAlphaVideoMaterialName2,
        testStandardMaterialName2 };
    std::vector<csp::common::String> materialCollectionIds {
        createdMaterial1->GetMaterialCollectionId(),
        createdMaterial2->GetMaterialCollectionId(),
        createdMaterial3->GetMaterialCollectionId(),
        createdMaterial4->GetMaterialCollectionId(),
    };
    std::vector<csp::common::String> materialIds {
        createdMaterial1->GetMaterialId(),
        createdMaterial2->GetMaterialId(),
        createdMaterial3->GetMaterialId(),
        createdMaterial4->GetMaterialId(),
    };

    for (size_t i = 0; i < foundMaterials.Size(); ++i)
    {
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
        if (foundMaterials[i]->GetShaderType() == EShaderType::AlphaVideo)
        {
            AlphaVideoMaterial* foundMaterialAlphaVideo = dynamic_cast<AlphaVideoMaterial*>(foundMaterials[i]);
            EXPECT_NE(foundMaterialAlphaVideo, nullptr);
        }
        else if (foundMaterials[i]->GetShaderType() == EShaderType::Standard)
        {
            GLTFMaterial* foundMaterialGltf = dynamic_cast<GLTFMaterial*>(foundMaterials[i]);
            EXPECT_NE(foundMaterialGltf, nullptr);
        }
#endif

        const csp::common::String& searchName = foundMaterials[i]->GetName();
        const csp::common::String& searchCollectionId = foundMaterials[i]->GetMaterialCollectionId();
        const csp::common::String& searchId = foundMaterials[i]->GetMaterialId();

        auto foundName = std::find_if(
            std::begin(materialNames), std::end(materialNames), [&searchName](const csp::common::String& name) { return name == searchName; });

        EXPECT_TRUE(foundName != materialNames.end());

        auto foundCollectionId = std::find_if(std::begin(materialCollectionIds), std::end(materialCollectionIds),
            [&searchCollectionId](const csp::common::String& collectionId) { return collectionId == searchCollectionId; });

        EXPECT_TRUE(foundCollectionId != materialCollectionIds.end());

        auto foundId
            = std::find_if(std::begin(materialIds), std::end(materialIds), [&searchId](const csp::common::String& id) { return id == searchId; });

        EXPECT_TRUE(foundId != materialIds.end());
    }

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial1);
    DeleteMaterial(assetSystem, *createdMaterial2);
    DeleteMaterial(assetSystem, *createdMaterial3);
    DeleteMaterial(assetSystem, *createdMaterial4);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

/*
 * This test is concerned with ensuring `AssetSystem::GetMaterials` is robust to the different ways in which the
 * downloading of materials can fail, and documents that our design is such that if one download fails that
 * the others are still run to completion.
 *
 * To that end, we inject a mock web client, which is setup to handle all the different API requests that are made by
 * `AssetSystem::GetMaterials`. As such, we can have quite fine grained control over the responses, to exercise the
 * different failure cases. This is one test vs individual tests for the different failure cases, so that we ensure
 * there are multiple threads at play (for targets other than WASM each request runs on a thread from the thread pool)
 * which can help shake out any thread safety issues in the implementation.
 */
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, MaterialTestsWithMocks, GetMultipleMaterialsSomeFail)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* assetSystem = systemsManager.GetAssetSystem();

    auto authContext = TestAuthContext();
    m_webClientMock->WebClient::SetAuthContext(authContext);

    EXPECT_CALL(*m_webClientMock, SendRequest)
        .WillRepeatedly([mock = m_webClientMock](auto&&... args) { mock->WebClient::SendRequest(std::forward<decltype(args)>(args)...); });

    EXPECT_CALL(*m_webClientMock, Send)
        .WillRepeatedly(
            [](csp::web::HttpRequest& request)
            {
                const auto verb = request.GetVerb();
                const auto uri = request.GetUri();

                ASSERT_EQ(verb, csp::web::ERequestVerb::GET);

                auto& response = request.GetMutableResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseNotFound);

                const auto rawUri = csp::common::String(uri.GetAsString());

                // `/mag-prototype/api/v1/prototypes`: Endpoint hit for requests sent by `AssetSystem::FindAssetCollections`
                if (rawUri.Contains("/mag-prototype/api/v1/prototypes?GroupIds=TestSpaceId"))
                {
                    response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);

                    response.GetMutablePayload().SetContent(R"([
                        {
                            "id": "TestPrototypeIdStandard1",
                            "metadata": {
                                "ShaderType": "Standard"
                            }
                        },
                        {
                            "id": "TestPrototypeIdUnsupportedShaderType",
                            "metadata": {
                                "ShaderType": "FutureTypeThatOldClientsDoNotSupport"
                            }
                        },
                        {
                            "id": "TestPrototypeIdInvalidContent",
                            "metadata": {
                                "ShaderType": "Standard"
                            }
                        },
                        {
                            "id": "TestPrototypeIdMissing",
                            "metadata": {
                                "ShaderType": "Standard"
                            }
                        },
                        {
                            "id": "TestPrototypeIdStandard2",
                            "metadata": {
                                "ShaderType": "Standard"
                            }
                        },
                        {
                            "id": "TestPrototypeIdStandard3",
                            "metadata": {
                                "ShaderType": "Standard"
                            }
                        }
                    ])");
                }
                // `/mag-prototype/api/v1/prototypes/asset-details`: Endpoint hit for requests sent by `AssetSystem::GetAssetsByCriteria`
                else if (rawUri.Contains("/mag-prototype/api/v1/prototypes/asset-details"))
                {
                    ASSERT_TRUE(rawUri.Contains("AssetTypes=Material"));
                    ASSERT_TRUE(rawUri.Contains("PrototypeIds=TestPrototypeIdStandard"));
                    ASSERT_TRUE(rawUri.Contains("PrototypeIds=TestPrototypeIdUnsupportedShaderType"));
                    ASSERT_TRUE(rawUri.Contains("PrototypeIds=TestPrototypeIdInvalidContent"));
                    ASSERT_TRUE(rawUri.Contains("PrototypeIds=TestPrototypeIdMissing"));

                    response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);

                    response.GetMutablePayload().SetContent(R"([
                        {
                            "id": "TestIdStandard1",
                            "prototypeId": "TestPrototypeIdStandard1",
                            "uri": "https://example.com/standard1.json"
                        },
                        {
                            "id": "TestIdUnsupportedShaderType",
                            "prototypeId": "TestPrototypeIdUnsupportedShaderType",
                            "uri": "https://example.com/unsupported.json"
                        },
                        {
                            "id": "TestIdInvalidContent",
                            "prototypeId": "TestPrototypeIdInvalidContent",
                            "uri": "https://example.com/invalid.json"
                        },
                        {
                            "id": "TestIdMissing",
                            "prototypeId": "TestPrototypeIdMissing",
                            "uri": "https://example.com/missing.json"
                        },
                        {
                            "id": "TestIdStandard2",
                            "prototypeId": "TestPrototypeIdStandard2",
                            "uri": "https://example.com/standard2.json"
                        },
                        {
                            "id": "TestIdStandard3",
                            "prototypeId": "TestPrototypeIdStandard3",
                            "uri": "https://example.com/standard3.json"
                        }
                    ])");
                }
                // Handle requests for individual `Asset::Uri`s
                else if (rawUri == "https://example.com/standard1.json" || rawUri == "https://example.com/standard2.json"
                    || rawUri == "https://example.com/standard3.json")
                {
                    response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                    response.GetMutablePayload().SetContent("{}");
                }
                else if (rawUri == "https://example.com/unsupported.json")
                {
                    response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                    response.GetMutablePayload().SetContent(R"({"unsupported": 42})");
                }
                else if (rawUri == "https://example.com/invalid.json")
                {
                    response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                    response.GetMutablePayload().SetContent("invalid");
                }
                else if (rawUri == "https://example.com/missing.json")
                {
                    response.SetResponseCode(csp::web::EResponseCodes::ResponseNotFound);
                }
            });

    const auto spaceId = csp::common::String("TestSpaceId");

    using OwnedMaterials = std::vector<std::unique_ptr<Material>>;

    const auto foundMaterials = [&]() -> OwnedMaterials
    {
        auto foundMaterials = csp::common::Array<Material*>();
        GetMaterials(assetSystem, spaceId, foundMaterials);

        const auto adopt = [](const csp::common::Array<Material*>& unownedMaterials) -> OwnedMaterials
        {
            auto owned = OwnedMaterials();
            owned.reserve(unownedMaterials.Size());

            for (auto* unowned : unownedMaterials)
            {
                owned.emplace_back(unowned);
            }

            return owned;
        };

        return adopt(foundMaterials);
    }();

    EXPECT_EQ(foundMaterials.size(), 3);

    const auto containsMaterial = [](const auto& range, csp::common::String materialId)
    {
        return std::find_if(
                   std::begin(range), std::end(range), [&materialId](const auto& material) { return material->GetMaterialId() == materialId; })
            != std::end(range);
    };

    EXPECT_TRUE(containsMaterial(foundMaterials, "TestIdStandard1"));
    EXPECT_TRUE(containsMaterial(foundMaterials, "TestIdStandard2"));
    EXPECT_TRUE(containsMaterial(foundMaterials, "TestIdStandard3"));
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetGLTFMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);

    // Get the material
    Material* foundMaterial = nullptr;
    GetMaterial(assetSystem, createdMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialId(), &foundMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    GLTFMaterial* foundGltfMaterial = dynamic_cast<GLTFMaterial*>(foundMaterial);
    EXPECT_NE(foundGltfMaterial, nullptr);
#endif

    EXPECT_EQ(foundMaterial->GetShaderType(), createdMaterial->GetShaderType());
    EXPECT_EQ(foundMaterial->GetName(), createdMaterial->GetName());
    EXPECT_EQ(foundMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialCollectionId());
    EXPECT_EQ(foundMaterial->GetMaterialId(), createdMaterial->GetMaterialId());

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetAlphaVideoMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::AlphaVideo, space.Id, {}, {}, &createdMaterial);

    // Get the material
    Material* foundMaterial = nullptr;
    GetMaterial(assetSystem, createdMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialId(), &foundMaterial);
// Guarding use of dynamic_cast to avoid having to enable RTTI for WASM.
#if defined CSP_WINDOWS
    AlphaVideoMaterial* foundAlphaVideoMaterial = dynamic_cast<AlphaVideoMaterial*>(foundMaterial);
    EXPECT_NE(foundAlphaVideoMaterial, nullptr);
#endif

    EXPECT_EQ(foundMaterial->GetShaderType(), createdMaterial->GetShaderType());
    EXPECT_EQ(foundMaterial->GetName(), createdMaterial->GetName());
    EXPECT_EQ(foundMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialCollectionId());
    EXPECT_EQ(foundMaterial->GetMaterialId(), createdMaterial->GetMaterialId());

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMaterialWithIncorrectShaderTypeTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);

    auto [Result1] = AWAIT_PRE(assetSystem, GetAssetCollectionById, RequestPredicate, createdMaterial->GetMaterialCollectionId());
    EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

    AssetCollection outAssetCollection = Result1.GetAssetCollection();

    // Create metadata for MaterialCollection with an invalid shader type
    csp::common::Map<csp::common::String, csp::common::String> inMetaData;
    inMetaData["ShaderType"] = "InvalidShaderType";

    auto [Result2] = AWAIT_PRE(assetSystem, UpdateAssetCollectionMetadata, RequestPredicate, outAssetCollection, inMetaData, nullptr);
    EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the material
    Material* foundMaterial = nullptr;
    GetMaterial(
        assetSystem, createdMaterial->GetMaterialCollectionId(), createdMaterial->GetMaterialId(), &foundMaterial, csp::systems::EResultCode::Failed);

    EXPECT_EQ(foundMaterial, nullptr);

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetMaterialsWithIncorrectShaderTypeTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);

    auto [Result1] = AWAIT_PRE(assetSystem, GetAssetCollectionById, RequestPredicate, createdMaterial->GetMaterialCollectionId());
    EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

    AssetCollection outAssetCollection = Result1.GetAssetCollection();

    // Create metadata for MaterialCollection with an invalid shader type
    csp::common::Map<csp::common::String, csp::common::String> inMetaData;
    inMetaData["ShaderType"] = "InvalidShaderType";

    auto [Result2] = AWAIT_PRE(assetSystem, UpdateAssetCollectionMetadata, RequestPredicate, outAssetCollection, inMetaData, nullptr);
    EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

    // Get the material using the GetMaterials method
    csp::common::Array<Material*> foundMaterials;
    GetMaterials(assetSystem, space.Id, foundMaterials, csp::systems::EResultCode::Failed);

    EXPECT_EQ(foundMaterials.Size(), 0);

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, GetInvalidMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create a material so we have one in this space
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, "TestMaterial", csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);

    // Attempt to get an invalid material
    Material* foundMaterial = nullptr;
    GetMaterial(assetSystem, "InvalidAssetCollectionId", "InvalidAssetId", &foundMaterial, csp::systems::EResultCode::Failed);

    // Cleanup
    DeleteMaterial(assetSystem, *createdMaterial);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, DeleteMaterialTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to search for materials
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    // Create 2 materials associated with the space
    constexpr const char* testMaterialName1 = "TestStandardMaterial";
    Material* createdMaterial1 = nullptr;
    CreateMaterial(assetSystem, testMaterialName1, csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial1);

    constexpr const char* testMaterialName2 = "TestAlphaVideoMaterial";
    Material* createdMaterial2 = nullptr;
    CreateMaterial(assetSystem, testMaterialName2, csp::systems::EShaderType::AlphaVideo, space.Id, {}, {}, &createdMaterial2);

    // Delete first material
    DeleteMaterial(assetSystem, *createdMaterial1);

    // Ensure the deletion has worked
    Material* deletedMaterial1 = nullptr;
    GetMaterial(assetSystem, createdMaterial1->GetMaterialCollectionId(), createdMaterial1->GetMaterialId(), &deletedMaterial1,
        csp::systems::EResultCode::Failed);

    // Make sure we can still get the second material
    Material* remainingMaterial = nullptr;
    GetMaterial(assetSystem, createdMaterial2->GetMaterialCollectionId(), createdMaterial2->GetMaterialId(), &remainingMaterial);

    // Delete second material
    DeleteMaterial(assetSystem, *createdMaterial2);

    // Ensure the second material is deleted
    Material* deletedMaterial2 = nullptr;
    GetMaterial(assetSystem, createdMaterial2->GetMaterialCollectionId(), createdMaterial2->GetMaterialId(), &deletedMaterial2,
        csp::systems::EResultCode::Failed);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, MaterialEventTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space so we can get the material events
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    constexpr const char* testMaterialName1 = "TestMaterial1";
    GLTFMaterial* createdGltfMaterial = nullptr;

    // Create material and listen for event
    {
        bool callbackCalled = false;

        csp::common::String materialCollectionId;
        csp::common::String materialId;

        auto cb = [&callbackCalled, &materialCollectionId, &materialId](const csp::common::MaterialChangedParams& params)
        {
            materialCollectionId = params.MaterialCollectionId;
            materialId = params.MaterialId;

            EXPECT_EQ(params.ChangeType, csp::common::EAssetChangeType::Created);
            callbackCalled = true;
        };

        assetSystem->SetMaterialChangedCallback(cb);

        // Create a material associated with the space
        Material* createdMaterial = nullptr;
        CreateMaterial(assetSystem, testMaterialName1, csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);
        createdGltfMaterial = static_cast<GLTFMaterial*>(createdMaterial);

        WaitForCallback(callbackCalled);
        EXPECT_TRUE(callbackCalled);

        // Do the check here where we know it exists
        EXPECT_EQ(materialCollectionId, createdGltfMaterial->GetMaterialCollectionId());
        EXPECT_EQ(materialId, createdGltfMaterial->GetMaterialId());
    }

    // Update material and listen for event
    {
        bool callbackCalled2 = false;

        auto cb = [&callbackCalled2, &createdGltfMaterial](const csp::common::MaterialChangedParams& params)
        {
            EXPECT_EQ(params.MaterialCollectionId, createdGltfMaterial->GetMaterialCollectionId());
            EXPECT_EQ(params.MaterialId, createdGltfMaterial->GetMaterialId());

            EXPECT_EQ(params.ChangeType, csp::common::EAssetChangeType::Updated);

            callbackCalled2 = true;
        };

        assetSystem->SetMaterialChangedCallback(cb);

        createdGltfMaterial->SetAlphaCutoff(1);
        UpdateMaterial(assetSystem, *createdGltfMaterial);
        WaitForCallback(callbackCalled2);

        EXPECT_TRUE(callbackCalled2);
    }

    // Delete material and listen for event
    {
        bool callbackCalled3 = false;

        auto cb = [&callbackCalled3, &createdGltfMaterial](const csp::common::MaterialChangedParams& params)
        {
            EXPECT_EQ(params.MaterialCollectionId, createdGltfMaterial->GetMaterialCollectionId());
            EXPECT_EQ(params.MaterialId, createdGltfMaterial->GetMaterialId());

            EXPECT_EQ(params.ChangeType, csp::common::EAssetChangeType::Deleted);

            callbackCalled3 = true;
        };

        assetSystem->SetMaterialChangedCallback(cb);

        DeleteMaterial(assetSystem, *createdGltfMaterial);
        WaitForCallback(callbackCalled3);

        EXPECT_TRUE(callbackCalled3);
    }

    // Cleanup
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, MaterialTests, MaterialAssetEventTest)
{
    SetRandSeed();

    auto& systemsManager = ::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* assetSystem = systemsManager.GetAssetSystem();

    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION";
    const char* testAssetName = "CSP-UNITTEST-ASSET";

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, GetUniqueString().c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space to associate a material with
    ::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space so we can get the material and asset events
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    constexpr const char* testMaterialName1 = "TestMaterial1";
    GLTFMaterial* createdGltfMaterial = nullptr;

    // Create material and listen for event
    bool callbackCalled = false;

    csp::common::String materialCollectionId;
    csp::common::String materialId;

    auto cb = [&callbackCalled, &materialCollectionId, &materialId](const csp::common::MaterialChangedParams& params)
    {
        materialCollectionId = params.MaterialCollectionId;
        materialId = params.MaterialId;

        EXPECT_EQ(params.ChangeType, csp::common::EAssetChangeType::Created);
        callbackCalled = true;
    };

    assetSystem->SetMaterialChangedCallback(cb);

    // Create a material associated with the space
    Material* createdMaterial = nullptr;
    CreateMaterial(assetSystem, testMaterialName1, csp::systems::EShaderType::Standard, space.Id, {}, {}, &createdMaterial);
    createdGltfMaterial = static_cast<GLTFMaterial*>(createdMaterial);

    WaitForCallback(callbackCalled);
    EXPECT_TRUE(callbackCalled);

    // Do the check here where we know it exists
    EXPECT_EQ(materialCollectionId, createdGltfMaterial->GetMaterialCollectionId());
    EXPECT_EQ(materialId, createdGltfMaterial->GetMaterialId());

    // Cleanup
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}
