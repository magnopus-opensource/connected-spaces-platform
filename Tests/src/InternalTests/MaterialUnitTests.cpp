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

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "CSP/Systems/Assets/AlphaVideoMaterial.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "Json/JsonSerializer.h"

using namespace csp::json;
using namespace csp::systems;

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, GLTFMaterialConstructorTest)
{
    constexpr const char* testName = "TestName";
    constexpr const char* testAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testAssetId = "TestAssetId";

    GLTFMaterial material(testName, testAssetCollectionId, testAssetId);

    // Test constructor params
    EXPECT_EQ(material.GetMaterialCollectionId(), testAssetCollectionId);
    EXPECT_EQ(material.GetMaterialId(), testAssetId);

    // Test defaults
    EXPECT_EQ(material.GetName(), testName);
    EXPECT_EQ(material.GetBaseColorFactor(), csp::common::Vector4(1.f, 1.f, 1.f, 1.f));
    EXPECT_EQ(material.GetMetallicFactor(), 1.f);
    EXPECT_EQ(material.GetRoughnessFactor(), 1.f);
    EXPECT_EQ(material.GetEmissiveFactor(), csp::common::Vector3(0.f, 0.f, 0.f));
    EXPECT_EQ(material.GetEmissiveStrength(), 1.f);
    EXPECT_EQ(material.GetAlphaCutoff(), 0.5f);
    EXPECT_EQ(material.GetDoubleSided(), false);

    EXPECT_EQ(material.GetBaseColorTexture().IsSet(), false);
    EXPECT_EQ(material.GetMetallicRoughnessTexture().IsSet(), false);
    EXPECT_EQ(material.GetNormalTexture().IsSet(), false);
    EXPECT_EQ(material.GetOcclusionTexture().IsSet(), false);
    EXPECT_EQ(material.GetEmissiveTexture().IsSet(), false);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, AlphaVideoMaterialConstructorTest)
{
    constexpr const char* testName = "TestName";
    constexpr const char* testAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testAssetId = "TestAssetId";

    AlphaVideoMaterial material(testName, testAssetCollectionId, testAssetId);

    // Test constructor params
    EXPECT_EQ(material.GetMaterialCollectionId(), testAssetCollectionId);
    EXPECT_EQ(material.GetMaterialId(), testAssetId);

    // Test defaults
    EXPECT_EQ(material.GetName(), testName);
    EXPECT_EQ(material.GetDoubleSided(), false);
    EXPECT_EQ(material.GetIsEmissive(), true);
    EXPECT_EQ(material.GetReadAlphaFromChannel(), EColorChannel::A);
    EXPECT_EQ(material.GetBlendMode(), EBlendMode::Normal);
    EXPECT_EQ(material.GetFresnelFactor(), 0.f);
    EXPECT_EQ(material.GetTint(), csp::common::Vector3(1.f, 1.f, 1.f));
    EXPECT_EQ(material.GetAlphaFactor(), 1.f);
    EXPECT_EQ(material.GetEmissiveIntensity(), 1.f);
    EXPECT_EQ(material.GetAlphaMask(), 0.02f);

    EXPECT_EQ(material.GetColorTexture().IsSet(), false);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, GLTFMaterialSetterTest)
{
    constexpr const char* testName = "TestName";
    constexpr const char* testAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testAssetId = "TestAssetId";

    csp::common::Vector4 testBaseColorFactor(0.f, 0.f, 0.f, 0.f);
    float testMetallicFactor = 1.f;
    float testRoughnessFactor = 2.f;
    csp::common::Vector3 testEmissiveFactor(1.f, 1.f, 1.f);
    float testEmissiveStrength = 1.f;
    float testAlphaCutoff = 3.f;
    bool testDoubleSided = true;

    GLTFMaterial material(testName, testAssetCollectionId, testAssetId);

    // Set new values
    material.SetBaseColorFactor(testBaseColorFactor);
    material.SetMetallicFactor(testMetallicFactor);
    material.SetRoughnessFactor(testRoughnessFactor);
    material.SetEmissiveFactor(testEmissiveFactor);
    material.SetEmissiveStrength(testEmissiveStrength);
    material.SetAlphaCutoff(testAlphaCutoff);
    material.SetDoubleSided(testDoubleSided);

    // Test values are set correctly
    EXPECT_EQ(material.GetName(), testName);
    EXPECT_EQ(material.GetBaseColorFactor(), testBaseColorFactor);
    EXPECT_EQ(material.GetMetallicFactor(), testMetallicFactor);
    EXPECT_EQ(material.GetRoughnessFactor(), testRoughnessFactor);
    EXPECT_EQ(material.GetEmissiveFactor(), testEmissiveFactor);
    EXPECT_EQ(material.GetEmissiveStrength(), testEmissiveStrength);
    EXPECT_EQ(material.GetAlphaCutoff(), testAlphaCutoff);
    EXPECT_EQ(material.GetDoubleSided(), testDoubleSided);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, AlphaVideoMaterialSetterTest)
{
    constexpr const char* testName = "TestName";
    constexpr const char* testAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testAssetId = "TestAssetId";

    bool testDoubleSided = true;
    bool testIsEmissive = false;
    EColorChannel testReadAlphaFromChannel = EColorChannel::R;
    EBlendMode testBlendMode = EBlendMode::Additive;
    float testFresnelFactor = 1.f;
    csp::common::Vector3 testTint(0.f, 0.f, 0.f);
    float testAlphaFactor = 0.f;
    float testEmissiveIntensity = 0.f;
    float testAlphaMask = 0.05f;

    AlphaVideoMaterial material(testName, testAssetCollectionId, testAssetId);

    // Set new values
    material.SetDoubleSided(testDoubleSided);
    material.SetIsEmissive(testIsEmissive);
    material.SetReadAlphaFromChannel(testReadAlphaFromChannel);
    material.SetBlendMode(testBlendMode);
    material.SetFresnelFactor(testFresnelFactor);
    material.SetTint(testTint);
    material.SetAlphaFactor(testAlphaFactor);
    material.SetEmissiveIntensity(testEmissiveIntensity);
    material.SetAlphaMask(testAlphaMask);

    // Test values are set correctly
    EXPECT_EQ(material.GetName(), testName);
    EXPECT_EQ(material.GetDoubleSided(), testDoubleSided);
    EXPECT_EQ(material.GetIsEmissive(), testIsEmissive);
    EXPECT_EQ(material.GetReadAlphaFromChannel(), testReadAlphaFromChannel);
    EXPECT_EQ(material.GetBlendMode(), testBlendMode);
    EXPECT_EQ(material.GetFresnelFactor(), testFresnelFactor);
    EXPECT_EQ(material.GetTint(), testTint);
    EXPECT_EQ(material.GetAlphaFactor(), testAlphaFactor);
    EXPECT_EQ(material.GetEmissiveIntensity(), testEmissiveIntensity);
    EXPECT_EQ(material.GetAlphaMask(), testAlphaMask);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, GLTFMaterialJsonSerializationTest)
{
    // Material vars
    constexpr const char* testName = "TestName";
    constexpr const char* testMaterialAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testMaterialAssetId = "TestAssetId";

    csp::common::Vector4 testBaseColorFactor(0.f, 0.f, 0.f, 0.f);
    float testMetallicFactor = 1.f;
    float testRoughnessFactor = 2.f;
    csp::common::Vector3 testEmissiveFactor(1.f, 1.f, 1.f);
    float testEmissiveStrength = 1.f;
    float testAlphaCutoff = 3.f;
    bool testDoubleSided = true;

    GLTFMaterial material(testName, testMaterialAssetCollectionId, testMaterialAssetId);

    // Set new values
    material.SetBaseColorFactor(testBaseColorFactor);
    material.SetMetallicFactor(testMetallicFactor);
    material.SetRoughnessFactor(testRoughnessFactor);
    material.SetEmissiveFactor(testEmissiveFactor);
    material.SetAlphaCutoff(testAlphaCutoff);
    material.SetDoubleSided(testDoubleSided);

    // Base colour texture vars
    const char* testBaseTextureAssetCollectionId = "TestAssetCollectionId";
    const char* testBaseTextureAssetId = "TestAssetId";
    const csp::common::Vector2 testBaseTextureUvOffset(1.f, 1.f);
    float testBaseTextureRotation = 1.f;
    const csp::common::Vector2 testBaseTextureUvScale(2.f, 2.f);
    const int testBaseTextureTexCoord = 2;

    // Metallic texture vars
    const char* testMetallicTextureEntityComponentId = "TestEntityComponentId2";
    const csp::common::Vector2 testMetallicTextureUvOffset(2.f, 2.f);
    float testMetallicTextureRotation = 2.f;
    const csp::common::Vector2 testMetallicTextureUvScale(3.f, 3.f);
    const int testMetallicTextureTexCoord = 3;

    // Normal texture vars
    const char* testNormalTextureAssetCollectionId = "TestAssetCollectionId3";
    const char* testNormalTextureAssetId = "TestAssetId3";
    const csp::common::Vector2 testNormalTextureUvOffset(3.f, 3.f);
    float testNormalTextureRotation = 3.f;
    const csp::common::Vector2 testNormalTextureUvScale(4.f, 4.f);
    const int testNormalTextureTexCoord = 4;

    // Occlusion texture vars
    const char* testOcclusionTextureEntityComponentId = "TestEntityComponentId4";
    const csp::common::Vector2 testOcclusionTextureUvOffset(4.f, 4.f);
    float testOcclusionTextureRotation = 4.f;
    const csp::common::Vector2 testOcclusionTextureUvScale(5.f, 5.f);
    const int testOcclusionTextureTexCoord = 6;

    // Emissive texture vars
    const char* testEmissiveTextureAssetCollectionId = "TestAssetCollectionId4";
    const char* testEmissiveTextureAssetId = "TestAssetId4";
    const csp::common::Vector2 testEmissiveTextureUvOffset(5.f, 5.f);
    float testEmissiveTextureRotation = 5.f;
    const csp::common::Vector2 testEmissiveTextureUvScale(6.f, 6.f);
    const int testEmissiveTextureTexCoord = 7;

    TextureInfo baseColor(testBaseTextureAssetCollectionId, testBaseTextureAssetId);
    baseColor.SetUVOffset(testBaseTextureUvOffset);
    baseColor.SetUVRotation(testBaseTextureRotation);
    baseColor.SetUVScale(testBaseTextureUvScale);
    baseColor.SetTexCoord(testBaseTextureTexCoord);

    TextureInfo metallic(testMetallicTextureEntityComponentId);
    metallic.SetUVOffset(testMetallicTextureUvOffset);
    metallic.SetUVRotation(testMetallicTextureRotation);
    metallic.SetUVScale(testMetallicTextureUvScale);
    metallic.SetTexCoord(testMetallicTextureTexCoord);

    TextureInfo normal(testNormalTextureAssetCollectionId, testNormalTextureAssetId);
    normal.SetUVOffset(testNormalTextureUvOffset);
    normal.SetUVRotation(testNormalTextureRotation);
    normal.SetUVScale(testNormalTextureUvScale);
    normal.SetTexCoord(testNormalTextureTexCoord);

    TextureInfo occlusion(testOcclusionTextureEntityComponentId);
    occlusion.SetUVOffset(testOcclusionTextureUvOffset);
    occlusion.SetUVRotation(testOcclusionTextureRotation);
    occlusion.SetUVScale(testOcclusionTextureUvScale);
    occlusion.SetTexCoord(testOcclusionTextureTexCoord);

    TextureInfo emissive(testEmissiveTextureAssetCollectionId, testEmissiveTextureAssetId);
    emissive.SetUVOffset(testEmissiveTextureUvOffset);
    emissive.SetUVRotation(testEmissiveTextureRotation);
    emissive.SetUVScale(testEmissiveTextureUvScale);
    emissive.SetTexCoord(testEmissiveTextureTexCoord);
    emissive.SetStereoVideoType(csp::multiplayer::StereoVideoType::SideBySide);
    emissive.SetIsStereoFlipped(true);

    material.SetBaseColorTexture(baseColor);
    material.SetMetallicRoughnessTexture(metallic);
    material.SetNormalTexture(normal);
    material.SetOcclusionTexture(occlusion);
    material.SetEmissiveTexture(emissive);

    csp::common::String jsonData = JsonSerializer::Serialize(material);

    GLTFMaterial deserializedMaterial;
    JsonDeserializer::Deserialize(jsonData, deserializedMaterial);

    EXPECT_EQ(deserializedMaterial.GetName(), testName);
    EXPECT_EQ(deserializedMaterial.GetBaseColorFactor(), testBaseColorFactor);
    EXPECT_EQ(deserializedMaterial.GetMetallicFactor(), testMetallicFactor);
    EXPECT_EQ(deserializedMaterial.GetRoughnessFactor(), testRoughnessFactor);
    EXPECT_EQ(deserializedMaterial.GetEmissiveFactor(), testEmissiveFactor);
    EXPECT_EQ(deserializedMaterial.GetEmissiveStrength(), testEmissiveStrength);
    EXPECT_EQ(deserializedMaterial.GetAlphaCutoff(), testAlphaCutoff);
    EXPECT_EQ(deserializedMaterial.GetDoubleSided(), testDoubleSided);

    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetAssetCollectionId(), testBaseTextureAssetCollectionId);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetAssetId(), testBaseTextureAssetId);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetUVOffset(), testBaseTextureUvOffset);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetUVRotation(), testBaseTextureRotation);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetUVScale(), testBaseTextureUvScale);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetTexCoord(), testBaseTextureTexCoord);
    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().GetStereoVideoType(), csp::multiplayer::StereoVideoType::None);

    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetEntityComponentId(), testMetallicTextureEntityComponentId);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetSourceType(), ETextureResourceType::Component);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetUVOffset(), testMetallicTextureUvOffset);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetUVRotation(), testMetallicTextureRotation);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetUVScale(), testMetallicTextureUvScale);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetTexCoord(), testMetallicTextureTexCoord);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().GetStereoVideoType(), csp::multiplayer::StereoVideoType::None);

    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetAssetCollectionId(), testNormalTextureAssetCollectionId);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetAssetId(), testNormalTextureAssetId);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetUVOffset(), testNormalTextureUvOffset);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetUVRotation(), testNormalTextureRotation);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetUVScale(), testNormalTextureUvScale);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetTexCoord(), testNormalTextureTexCoord);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().GetStereoVideoType(), csp::multiplayer::StereoVideoType::None);

    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetEntityComponentId(), testOcclusionTextureEntityComponentId);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetSourceType(), ETextureResourceType::Component);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetUVOffset(), testOcclusionTextureUvOffset);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetUVRotation(), testOcclusionTextureRotation);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetUVScale(), testOcclusionTextureUvScale);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetTexCoord(), testOcclusionTextureTexCoord);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().GetStereoVideoType(), csp::multiplayer::StereoVideoType::None);

    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetAssetCollectionId(), testEmissiveTextureAssetCollectionId);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetAssetId(), testEmissiveTextureAssetId);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetUVOffset(), testEmissiveTextureUvOffset);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetUVRotation(), testEmissiveTextureRotation);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetUVScale(), testEmissiveTextureUvScale);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetTexCoord(), testEmissiveTextureTexCoord);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetStereoVideoType(), csp::multiplayer::StereoVideoType::SideBySide);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().GetIsStereoFlipped(), true);

    EXPECT_EQ(deserializedMaterial.GetBaseColorTexture().IsSet(), true);
    EXPECT_EQ(deserializedMaterial.GetMetallicRoughnessTexture().IsSet(), true);
    EXPECT_EQ(deserializedMaterial.GetNormalTexture().IsSet(), true);
    EXPECT_EQ(deserializedMaterial.GetOcclusionTexture().IsSet(), true);
    EXPECT_EQ(deserializedMaterial.GetEmissiveTexture().IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, AlphaVideoMaterialJsonSerializationTest)
{
    // Material vars
    constexpr const char* testName = "TestName";
    constexpr const char* testMaterialAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testMaterialAssetId = "TestAssetId";

    bool testDoubleSided = true;
    bool testIsEmissive = false;
    EColorChannel testReadAlphaFromChannel = EColorChannel::R;
    EBlendMode testBlendMode = EBlendMode::Additive;
    float testFresnelFactor = 1.f;
    csp::common::Vector3 testTint(0.f, 0.f, 0.f);
    float testAlphaFactor = 0.f;
    float testEmissiveIntensity = 0.f;
    float testAlphaMask = 0.05f;

    AlphaVideoMaterial material(testName, testMaterialAssetCollectionId, testMaterialAssetId);

    // Set new values
    material.SetDoubleSided(testDoubleSided);
    material.SetIsEmissive(testIsEmissive);
    material.SetReadAlphaFromChannel(testReadAlphaFromChannel);
    material.SetBlendMode(testBlendMode);
    material.SetFresnelFactor(testFresnelFactor);
    material.SetTint(testTint);
    material.SetAlphaFactor(testAlphaFactor);
    material.SetEmissiveIntensity(testEmissiveIntensity);
    material.SetAlphaMask(testAlphaMask);

    // Base colour texture vars
    const char* testColorTextureAssetCollectionId = "TestAssetCollectionId";
    const char* testColorTextureAssetId = "TestAssetId";
    const csp::common::Vector2 testColorTextureUvOffset(1.f, 1.f);
    float testColorTextureRotation = 1.f;
    const csp::common::Vector2 testColorTextureUvScale(2.f, 2.f);
    const int testColorTextureTexCoord = 2;

    TextureInfo colorTexture(testColorTextureAssetCollectionId, testColorTextureAssetId);
    colorTexture.SetUVOffset(testColorTextureUvOffset);
    colorTexture.SetUVRotation(testColorTextureRotation);
    colorTexture.SetUVScale(testColorTextureUvScale);
    colorTexture.SetTexCoord(testColorTextureTexCoord);

    material.SetColorTexture(colorTexture);

    csp::common::String jsonData = JsonSerializer::Serialize(material);

    AlphaVideoMaterial deserializedMaterial;
    JsonDeserializer::Deserialize(jsonData, deserializedMaterial);

    EXPECT_EQ(deserializedMaterial.GetName(), testName);
    EXPECT_EQ(deserializedMaterial.GetDoubleSided(), testDoubleSided);
    EXPECT_EQ(deserializedMaterial.GetIsEmissive(), testIsEmissive);
    EXPECT_EQ(deserializedMaterial.GetReadAlphaFromChannel(), testReadAlphaFromChannel);
    EXPECT_EQ(deserializedMaterial.GetBlendMode(), testBlendMode);
    EXPECT_EQ(deserializedMaterial.GetFresnelFactor(), testFresnelFactor);
    EXPECT_EQ(deserializedMaterial.GetTint(), testTint);
    EXPECT_EQ(deserializedMaterial.GetAlphaFactor(), testAlphaFactor);
    EXPECT_EQ(deserializedMaterial.GetEmissiveIntensity(), testEmissiveIntensity);
    EXPECT_EQ(deserializedMaterial.GetAlphaMask(), testAlphaMask);

    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetAssetCollectionId(), testColorTextureAssetCollectionId);
    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetAssetId(), testColorTextureAssetId);
    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetUVOffset(), testColorTextureUvOffset);
    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetUVRotation(), testColorTextureRotation);
    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetUVScale(), testColorTextureUvScale);
    EXPECT_EQ(deserializedMaterial.GetColorTexture().GetTexCoord(), testColorTextureTexCoord);

    EXPECT_EQ(deserializedMaterial.GetColorTexture().IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureInfoDefaultConstructorTest)
{
    TextureInfo texture;

    EXPECT_EQ(texture.GetAssetCollectionId(), "");
    EXPECT_EQ(texture.GetAssetId(), "");
    EXPECT_EQ(texture.GetEntityComponentId(), "");
    EXPECT_EQ(texture.GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(texture.GetUVOffset(), csp::common::Vector2(0.f, 0.f));
    EXPECT_EQ(texture.GetUVRotation(), 0.f);
    EXPECT_EQ(texture.GetUVScale(), csp::common::Vector2(1.f, 1.f));
    EXPECT_EQ(texture.GetTexCoord(), 0);
    EXPECT_EQ(texture.IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureInfoAssetIdConstructorTest)
{
    constexpr const char* testAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* testAssetId = "TestAssetId";

    TextureInfo texture(testAssetCollectionId, testAssetId);

    EXPECT_EQ(texture.GetAssetCollectionId(), testAssetCollectionId);
    EXPECT_EQ(texture.GetAssetId(), testAssetId);
    EXPECT_EQ(texture.GetEntityComponentId(), "");
    EXPECT_EQ(texture.GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(texture.GetUVOffset(), csp::common::Vector2(0.f, 0.f));
    EXPECT_EQ(texture.GetUVRotation(), 0.f);
    EXPECT_EQ(texture.GetUVScale(), csp::common::Vector2(1.f, 1.f));
    EXPECT_EQ(texture.GetTexCoord(), 0);
    EXPECT_EQ(texture.IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureInfoComponentIdConstructorTest)
{
    constexpr const char* testComponentId = "TestComponentId";

    TextureInfo texture(testComponentId);

    EXPECT_EQ(texture.GetAssetCollectionId(), "");
    EXPECT_EQ(texture.GetAssetId(), "");
    EXPECT_EQ(texture.GetEntityComponentId(), testComponentId);
    EXPECT_EQ(texture.GetSourceType(), ETextureResourceType::Component);
    EXPECT_EQ(texture.GetUVOffset(), csp::common::Vector2(0.f, 0.f));
    EXPECT_EQ(texture.GetUVRotation(), 0.f);
    EXPECT_EQ(texture.GetUVScale(), csp::common::Vector2(1.f, 1.f));
    EXPECT_EQ(texture.GetTexCoord(), 0);
    EXPECT_EQ(texture.IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureSetterTest)
{
    const char* testAssetCollectionId = "TestAssetCollectionId";
    const char* testAssetId = "TestAssetId";
    const char* testEntityComponentId = "TestEntityComponentId";
    const csp::common::Vector2 testUvOffset(1.f, 1.f);
    float testRotation = 1.f;
    const csp::common::Vector2 testUvScale(2.f, 2.f);
    const int testTexCoord = 2;

    TextureInfo texture;

    texture.SetCollectionAndAssetId(testAssetCollectionId, testAssetId);
    texture.SetUVOffset(testUvOffset);
    texture.SetUVRotation(testRotation);
    texture.SetUVScale(testUvScale);
    texture.SetTexCoord(testTexCoord);
    texture.SetTexture(false);

    EXPECT_EQ(texture.GetAssetCollectionId(), testAssetCollectionId);
    EXPECT_EQ(texture.GetAssetId(), testAssetId);
    EXPECT_EQ(texture.GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(texture.GetUVOffset(), testUvOffset);
    EXPECT_EQ(texture.GetUVRotation(), testRotation);
    EXPECT_EQ(texture.GetUVScale(), testUvScale);
    EXPECT_EQ(texture.GetTexCoord(), testTexCoord);
    EXPECT_EQ(texture.IsSet(), false);

    texture.SetEntityComponentId(testEntityComponentId);

    // Ensure the component setter is correct
    EXPECT_EQ(texture.GetEntityComponentId(), testEntityComponentId);
    EXPECT_EQ(texture.GetSourceType(), ETextureResourceType::Component);

    // Double-check the Asset setter is working
    texture.SetCollectionAndAssetId(testAssetCollectionId, testAssetId);
    EXPECT_EQ(texture.GetSourceType(), ETextureResourceType::ImageAsset);
}
