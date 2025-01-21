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
#ifndef SKIP_INTERNAL_TESTS

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "Json/JsonSerializer.h"

using namespace csp::json;
using namespace csp::systems;

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, MaterialConstructorTest)
{
    constexpr const char* TestName = "TestName";
    constexpr const char* TestAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* TestAssetId = "TestAssetId";

    GLTFMaterial Material(TestName, TestAssetCollectionId, TestAssetId);

    // Test constructor params
    EXPECT_EQ(Material.GetMaterialCollectionId(), TestAssetCollectionId);
    EXPECT_EQ(Material.GetMaterialId(), TestAssetId);

    // Test defaults
    EXPECT_EQ(Material.GetName(), TestName);
    EXPECT_EQ(Material.GetBaseColorFactor(), csp::common::Vector4(1.f, 1.f, 1.f, 1.f));
    EXPECT_EQ(Material.GetMetallicFactor(), 1.f);
    EXPECT_EQ(Material.GetRoughnessFactor(), 1.f);
    EXPECT_EQ(Material.GetEmissiveFactor(), csp::common::Vector3(0.f, 0.f, 0.f));
    EXPECT_EQ(Material.GetAlphaCutoff(), 0.5f);
    EXPECT_EQ(Material.GetDoubleSided(), false);

    EXPECT_EQ(Material.GetBaseColorTexture().IsSet(), false);
    EXPECT_EQ(Material.GetMetallicRoughnessTexture().IsSet(), false);
    EXPECT_EQ(Material.GetNormalTexture().IsSet(), false);
    EXPECT_EQ(Material.GetOcclusionTexture().IsSet(), false);
    EXPECT_EQ(Material.GetEmissiveTexture().IsSet(), false);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, MaterialSetterTest)
{
    constexpr const char* TestName = "TestName";
    constexpr const char* TestAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* TestAssetId = "TestAssetId";

    csp::common::Vector4 TestBaseColorFactor(0.f, 0.f, 0.f, 0.f);
    float TestMetallicFactor = 1.f;
    float TestRoughnessFactor = 2.f;
    csp::common::Vector3 TestEmissiveFactor(1.f, 1.f, 1.f);
    float TestAlphaCutoff = 3.f;
    bool TestDoubleSided = true;

    GLTFMaterial Material(TestName, TestAssetCollectionId, TestAssetId);

    // Set new values
    Material.SetBaseColorFactor(TestBaseColorFactor);
    Material.SetMetallicFactor(TestMetallicFactor);
    Material.SetRoughnessFactor(TestRoughnessFactor);
    Material.SetEmissiveFactor(TestEmissiveFactor);
    Material.SetAlphaCutoff(TestAlphaCutoff);
    Material.SetDoubleSided(TestDoubleSided);

    // Test values are set correctly
    EXPECT_EQ(Material.GetName(), TestName);
    EXPECT_EQ(Material.GetBaseColorFactor(), TestBaseColorFactor);
    EXPECT_EQ(Material.GetMetallicFactor(), TestMetallicFactor);
    EXPECT_EQ(Material.GetRoughnessFactor(), TestRoughnessFactor);
    EXPECT_EQ(Material.GetEmissiveFactor(), TestEmissiveFactor);
    EXPECT_EQ(Material.GetAlphaCutoff(), TestAlphaCutoff);
    EXPECT_EQ(Material.GetDoubleSided(), TestDoubleSided);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, MaterialJsonSerializationTest)
{
    // Material vars
    constexpr const char* TestName = "TestName";
    constexpr const char* TestMaterialAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* TestMaterialAssetId = "TestAssetId";

    csp::common::Vector4 TestBaseColorFactor(0.f, 0.f, 0.f, 0.f);
    float TestMetallicFactor = 1.f;
    float TestRoughnessFactor = 2.f;
    csp::common::Vector3 TestEmissiveFactor(1.f, 1.f, 1.f);
    float TestAlphaCutoff = 3.f;
    bool TestDoubleSided = true;

    GLTFMaterial Material(TestName, TestMaterialAssetCollectionId, TestMaterialAssetId);

    // Set new values
    Material.SetBaseColorFactor(TestBaseColorFactor);
    Material.SetMetallicFactor(TestMetallicFactor);
    Material.SetRoughnessFactor(TestRoughnessFactor);
    Material.SetEmissiveFactor(TestEmissiveFactor);
    Material.SetAlphaCutoff(TestAlphaCutoff);
    Material.SetDoubleSided(TestDoubleSided);

    // Base colour texture vars
    const char* TestBaseTextureAssetCollectionId = "TestAssetCollectionId";
    const char* TestBaseTextureAssetId = "TestAssetId";
    const csp::common::Vector2 TestBaseTextureUVOffset(1.f, 1.f);
    float TestBaseTextureRotation = 1.f;
    const csp::common::Vector2 TestBaseTextureUVScale(2.f, 2.f);
    const int TestBaseTextureTexCoord = 2;

    // Metallic texture vars
    const char* TestMetallicTextureEntityComponentId = "TestEntityComponentId2";
    const csp::common::Vector2 TestMetallicTextureUVOffset(2.f, 2.f);
    float TestMetallicTextureRotation = 2.f;
    const csp::common::Vector2 TestMetallicTextureUVScale(3.f, 3.f);
    const int TestMetallicTextureTexCoord = 3;

    // Normal texture vars
    const char* TestNormalTextureAssetCollectionId = "TestAssetCollectionId3";
    const char* TestNormalTextureAssetId = "TestAssetId3";
    const csp::common::Vector2 TestNormalTextureUVOffset(3.f, 3.f);
    float TestNormalTextureRotation = 3.f;
    const csp::common::Vector2 TestNormalTextureUVScale(4.f, 4.f);
    const int TestNormalTextureTexCoord = 4;

    // Occlusion texture vars
    const char* TestOcclusionTextureEntityComponentId = "TestEntityComponentId4";
    const csp::common::Vector2 TestOcclusionTextureUVOffset(4.f, 4.f);
    float TestOcclusionTextureRotation = 4.f;
    const csp::common::Vector2 TestOcclusionTextureUVScale(5.f, 5.f);
    const int TestOcclusionTextureTexCoord = 6;

    // Emissive texture vars
    const char* TestEmissiveTextureAssetCollectionId = "TestAssetCollectionId4";
    const char* TestEmissiveTextureAssetId = "TestAssetId4";
    const csp::common::Vector2 TestEmissiveTextureUVOffset(5.f, 5.f);
    float TestEmissiveTextureRotation = 5.f;
    const csp::common::Vector2 TestEmissiveTextureUVScale(6.f, 6.f);
    const int TestEmissiveTextureTexCoord = 7;

    TextureInfo BaseColor(TestBaseTextureAssetCollectionId, TestBaseTextureAssetId);
    BaseColor.SetUVOffset(TestBaseTextureUVOffset);
    BaseColor.SetUVRotation(TestBaseTextureRotation);
    BaseColor.SetUVScale(TestBaseTextureUVScale);
    BaseColor.SetTexCoord(TestBaseTextureTexCoord);

    TextureInfo Metallic(TestMetallicTextureEntityComponentId);
    Metallic.SetUVOffset(TestMetallicTextureUVOffset);
    Metallic.SetUVRotation(TestMetallicTextureRotation);
    Metallic.SetUVScale(TestMetallicTextureUVScale);
    Metallic.SetTexCoord(TestMetallicTextureTexCoord);

    TextureInfo Normal(TestNormalTextureAssetCollectionId, TestNormalTextureAssetId);
    Normal.SetUVOffset(TestNormalTextureUVOffset);
    Normal.SetUVRotation(TestNormalTextureRotation);
    Normal.SetUVScale(TestNormalTextureUVScale);
    Normal.SetTexCoord(TestNormalTextureTexCoord);

    TextureInfo Occlusion(TestOcclusionTextureEntityComponentId);
    Occlusion.SetUVOffset(TestOcclusionTextureUVOffset);
    Occlusion.SetUVRotation(TestOcclusionTextureRotation);
    Occlusion.SetUVScale(TestOcclusionTextureUVScale);
    Occlusion.SetTexCoord(TestOcclusionTextureTexCoord);

    TextureInfo Emissive(TestEmissiveTextureAssetCollectionId, TestEmissiveTextureAssetId);
    Emissive.SetUVOffset(TestEmissiveTextureUVOffset);
    Emissive.SetUVRotation(TestEmissiveTextureRotation);
    Emissive.SetUVScale(TestEmissiveTextureUVScale);
    Emissive.SetTexCoord(TestEmissiveTextureTexCoord);

    Material.SetBaseColorTexture(BaseColor);
    Material.SetMetallicRoughnessTexture(Metallic);
    Material.SetNormalTexture(Normal);
    Material.SetOcclusionTexture(Occlusion);
    Material.SetEmissiveTexture(Emissive);

    csp::common::String JsonData = JsonSerializer::Serialize(Material);

    GLTFMaterial DeserializedMaterial;
    JsonDeserializer::Deserialize(JsonData, DeserializedMaterial);

    EXPECT_EQ(DeserializedMaterial.GetName(), TestName);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorFactor(), TestBaseColorFactor);
    EXPECT_EQ(DeserializedMaterial.GetMetallicFactor(), TestMetallicFactor);
    EXPECT_EQ(DeserializedMaterial.GetRoughnessFactor(), TestRoughnessFactor);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveFactor(), TestEmissiveFactor);
    EXPECT_EQ(DeserializedMaterial.GetAlphaCutoff(), TestAlphaCutoff);
    EXPECT_EQ(DeserializedMaterial.GetDoubleSided(), TestDoubleSided);

    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetAssetCollectionId(), TestBaseTextureAssetCollectionId);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetAssetId(), TestBaseTextureAssetId);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetUVOffset(), TestBaseTextureUVOffset);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetUVRotation(), TestBaseTextureRotation);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetUVScale(), TestBaseTextureUVScale);
    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().GetTexCoord(), TestBaseTextureTexCoord);

    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().GetEntityComponentId(), TestMetallicTextureEntityComponentId);
    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().GetSourceType(), ETextureResourceType::Component);
    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().GetUVOffset(), TestMetallicTextureUVOffset);
    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().GetUVRotation(), TestMetallicTextureRotation);
    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().GetUVScale(), TestMetallicTextureUVScale);
    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().GetTexCoord(), TestMetallicTextureTexCoord);

    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetAssetCollectionId(), TestNormalTextureAssetCollectionId);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetAssetId(), TestNormalTextureAssetId);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetUVOffset(), TestNormalTextureUVOffset);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetUVRotation(), TestNormalTextureRotation);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetUVScale(), TestNormalTextureUVScale);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().GetTexCoord(), TestNormalTextureTexCoord);

    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().GetEntityComponentId(), TestOcclusionTextureEntityComponentId);
    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().GetSourceType(), ETextureResourceType::Component);
    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().GetUVOffset(), TestOcclusionTextureUVOffset);
    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().GetUVRotation(), TestOcclusionTextureRotation);
    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().GetUVScale(), TestOcclusionTextureUVScale);
    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().GetTexCoord(), TestOcclusionTextureTexCoord);

    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetAssetCollectionId(), TestEmissiveTextureAssetCollectionId);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetAssetId(), TestEmissiveTextureAssetId);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetUVOffset(), TestEmissiveTextureUVOffset);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetUVRotation(), TestEmissiveTextureRotation);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetUVScale(), TestEmissiveTextureUVScale);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().GetTexCoord(), TestEmissiveTextureTexCoord);

    EXPECT_EQ(DeserializedMaterial.GetBaseColorTexture().IsSet(), true);
    EXPECT_EQ(DeserializedMaterial.GetMetallicRoughnessTexture().IsSet(), true);
    EXPECT_EQ(DeserializedMaterial.GetNormalTexture().IsSet(), true);
    EXPECT_EQ(DeserializedMaterial.GetOcclusionTexture().IsSet(), true);
    EXPECT_EQ(DeserializedMaterial.GetEmissiveTexture().IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureInfoDefaultConstructorTest)
{
    TextureInfo Texture;

    EXPECT_EQ(Texture.GetAssetCollectionId(), "");
    EXPECT_EQ(Texture.GetAssetId(), "");
    EXPECT_EQ(Texture.GetEntityComponentId(), "");
    EXPECT_EQ(Texture.GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(Texture.GetUVOffset(), csp::common::Vector2(0.f, 0.f));
    EXPECT_EQ(Texture.GetUVRotation(), 0.f);
    EXPECT_EQ(Texture.GetUVScale(), csp::common::Vector2(1.f, 1.f));
    EXPECT_EQ(Texture.GetTexCoord(), 0);
    EXPECT_EQ(Texture.IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureInfoAssetIdConstructorTest)
{
    constexpr const char* TestAssetCollectionId = "TestAssetCollectionId";
    constexpr const char* TestAssetId = "TestAssetId";

    TextureInfo Texture(TestAssetCollectionId, TestAssetId);

    EXPECT_EQ(Texture.GetAssetCollectionId(), TestAssetCollectionId);
    EXPECT_EQ(Texture.GetAssetId(), TestAssetId);
    EXPECT_EQ(Texture.GetEntityComponentId(), "");
    EXPECT_EQ(Texture.GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(Texture.GetUVOffset(), csp::common::Vector2(0.f, 0.f));
    EXPECT_EQ(Texture.GetUVRotation(), 0.f);
    EXPECT_EQ(Texture.GetUVScale(), csp::common::Vector2(1.f, 1.f));
    EXPECT_EQ(Texture.GetTexCoord(), 0);
    EXPECT_EQ(Texture.IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureInfoComponentIdConstructorTest)
{
    constexpr const char* TestComponentId = "TestComponentId";

    TextureInfo Texture(TestComponentId);

    EXPECT_EQ(Texture.GetAssetCollectionId(), "");
    EXPECT_EQ(Texture.GetAssetId(), "");
    EXPECT_EQ(Texture.GetEntityComponentId(), TestComponentId);
    EXPECT_EQ(Texture.GetSourceType(), ETextureResourceType::Component);
    EXPECT_EQ(Texture.GetUVOffset(), csp::common::Vector2(0.f, 0.f));
    EXPECT_EQ(Texture.GetUVRotation(), 0.f);
    EXPECT_EQ(Texture.GetUVScale(), csp::common::Vector2(1.f, 1.f));
    EXPECT_EQ(Texture.GetTexCoord(), 0);
    EXPECT_EQ(Texture.IsSet(), true);
}

CSP_INTERNAL_TEST(CSPEngine, MaterialUnitTests, TextureSetterTest)
{
    const char* TestAssetCollectionId = "TestAssetCollectionId";
    const char* TestAssetId = "TestAssetId";
    const char* TestEntityComponentId = "TestEntityComponentId";
    const csp::common::Vector2 TestUVOffset(1.f, 1.f);
    float TestRotation = 1.f;
    const csp::common::Vector2 TestUVScale(2.f, 2.f);
    const int TestTexCoord = 2;

    TextureInfo Texture;

    Texture.SetCollectionAndAssetId(TestAssetCollectionId, TestAssetId);
    Texture.SetUVOffset(TestUVOffset);
    Texture.SetUVRotation(TestRotation);
    Texture.SetUVScale(TestUVScale);
    Texture.SetTexCoord(TestTexCoord);
    Texture.SetTexture(false);

    EXPECT_EQ(Texture.GetAssetCollectionId(), TestAssetCollectionId);
    EXPECT_EQ(Texture.GetAssetId(), TestAssetId);
    EXPECT_EQ(Texture.GetSourceType(), ETextureResourceType::ImageAsset);
    EXPECT_EQ(Texture.GetUVOffset(), TestUVOffset);
    EXPECT_EQ(Texture.GetUVRotation(), TestRotation);
    EXPECT_EQ(Texture.GetUVScale(), TestUVScale);
    EXPECT_EQ(Texture.GetTexCoord(), TestTexCoord);
    EXPECT_EQ(Texture.IsSet(), false);

    Texture.SetEntityComponentId(TestEntityComponentId);

    // Ensure the component setter is correct
    EXPECT_EQ(Texture.GetEntityComponentId(), TestEntityComponentId);
    EXPECT_EQ(Texture.GetSourceType(), ETextureResourceType::Component);

    // Double-check the Asset setter is working
    Texture.SetCollectionAndAssetId(TestAssetCollectionId, TestAssetId);
    EXPECT_EQ(Texture.GetSourceType(), ETextureResourceType::ImageAsset);
}
#endif