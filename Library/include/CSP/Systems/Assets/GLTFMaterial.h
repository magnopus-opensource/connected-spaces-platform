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
#pragma once

#include "CSP/Common/Array.h"
#include "CSP/Systems/WebService.h"
#include "Material.h"

namespace csp::systems
{
class GLTFMaterial;
}

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj);

namespace csp::systems
{

/// @ingroup Asset System
/// @brief Data class which represents a GLTF material.
class CSP_API GLTFMaterial : public Material
{
public:
    /// @brief Sets the alpha mode which determines how the alpha value is interpreted.
    /// @param Mode EAlphaMode : The alpha mode to set.
    void SetAlphaMode(EAlphaMode Mode);

    /// @brief Gets the alpha mode which determines how the alpha value is interpreted.
    /// @return EAlphaMode : The alpha mode.
    EAlphaMode GetAlphaMode() const;

    /// @brief Sets the alpha cutoff value.
    /// @details When alphaMode is set to MASK the alphaCutoff property specifies the cutoff threshold.
    /// If the alpha value is greater than or equal to the alphaCutoff value then it is rendered as fully opaque,
    /// otherwise, it is rendered as fully transparent. The alphaCutoff value is ignored for other modes.
    /// @param Cutoff float : The alpha cutoff value to set.
    void SetAlphaCutoff(float Cutoff);

    /// @brief Gets the alpha cutoff value.
    /// @return float : The alpha cutoff value.
    float GetAlphaCutoff() const;

    /// @brief Sets the doubleSided property which specifies whether the material is double sided.
    /// @details When this value is false, back - face culling is enabled, i.e., only front-facing triangles are rendered.
    /// When this value is true, back - face culling is disabled and double sided lighting is enabled.
    /// @param DoubleSided bool : The double sided value to set.
    void SetDoubleSided(bool DoubleSided);

    /// @brief Gets the double sided value.
    /// @return bool : The double sided value.
    bool GetDoubleSided() const;

    /// @brief Sets the factors for the base color of the material.
    /// @details This value defines linear multipliers for the sampled texels of the base color texture.
    /// @param Factor const csp::common::Vector4& : The base color factor to set.
    void SetBaseColorFactor(const csp::common::Vector4& Factor);

    /// @brief Gets the factor of the base color texture.
    /// @return csp::common::Vector4& : The base color factor.
    const csp::common::Vector4& GetBaseColorFactor() const;

    /// @brief Sets the factor for the metalness of the material.
    /// @details This value defines a linear multiplier for the sampled metalness values of the metallic-roughness texture.
    /// @param Factor float : The metallic factor to set.
    void SetMetallicFactor(float Factor);

    /// @brief Gets the factor of the metallic texture.
    /// @return float : The metallic factor.
    float GetMetallicFactor() const;

    /// @brief Sets the factor for the roughness of the material.
    /// @details This value defines a linear multiplier for the sampled roughness values of the metallic-roughness texture.
    /// @param Factor float : The roughness factor to set.
    void SetRoughnessFactor(float Factor);

    /// @brief Gets the factor of the roughness texture.
    /// @return float : The roughness factor.
    float GetRoughnessFactor() const;

    /// @brief Sets factors for the emissive color of the material.
    /// @details This value defines linear multipliers for the sampled texels of the emissive texture.
    /// @param Factor const csp::common::Vector3& : The emissive factor to set.
    void SetEmissiveFactor(const csp::common::Vector3& Factor);

    /// @brief Gets the factor of the emissive color texture.
    /// @return csp::common::Vector3& : The emissive factor.
    const csp::common::Vector3& GetEmissiveFactor() const;

    /// @brief Set the emissive strength.
    /// @param Strength float : The emissive strength.
    void SetEmissiveStrength(float Strength);

    /// @brief Get the emissive strength.
    /// @return float : The emissive strength.
    float GetEmissiveStrength() const;

    /// @brief Sets the base color texture.
    /// @details The first three components (RGB) MUST be encoded with the sRGB transfer function. They specify the base color of the material.
    /// If the fourth component (A) is present, it represents the linear alpha coverage of the material.
    /// Otherwise, the alpha coverage is equal to 1.0. The material.alphaMode property specifies how alpha is interpreted.
    /// The stored texels MUST NOT be premultiplied.
    /// When undefined, the texture MUST be sampled as having 1.0 in all components.
    /// @param Texture const TextureInfo& : The base color texture to set.
    void SetBaseColorTexture(const TextureInfo& Texture);

    /// @brief Gets the base color texture.
    /// @return const TextureInfo& : The base color texture.
    const TextureInfo& GetBaseColorTexture() const;

    /// @brief Sets the metallic-roughness texture.
    /// @details The metalness values are sampled from the B channel.
    /// The roughness values are sampled from the G channel.
    /// These values MUST be encoded with a linear transfer function.
    /// If other channels are present (R or A), they MUST be ignored for metallic-roughness calculations.
    /// When undefined, the texture MUST be sampled as having 1.0 in G and B components.
    /// @param Texture const TextureInfo& : The metallic-roughness texture to set.
    void SetMetallicRoughnessTexture(const TextureInfo& Texture);

    /// @brief Gets the metallic-roughness texture.
    /// @return const TextureInfo& : The metallic-roughness texture.
    const TextureInfo& GetMetallicRoughnessTexture() const;

    /// @brief Sets the tangent space normal texture.
    /// @details The texture encodes RGB components with linear transfer function.
    /// Each texel represents the XYZ components of a normal vector in tangent space.
    /// The normal vectors use the convention +X is right and +Y is up. +Z points toward the viewer.
    /// If a fourth component (A) is present, it MUST be ignored. When undefined, the material does not have a tangent space normal texture.
    /// @param Texture const TextureInfo& : The normal texture to set.
    void SetNormalTexture(const TextureInfo& Texture);

    /// @brief Gets the tangent space normal texture.
    /// @return const TextureInfo& : The normal texture.
    const TextureInfo& GetNormalTexture() const;

    /// @brief Sets the occlusion texture.
    /// @details The occlusion values are linearly sampled from the R channel.
    /// Higher values indicate areas that receive full indirect lighting and lower values indicate no indirect lighting.
    /// If other channels are present (GBA), they MUST be ignored for occlusion calculations.
    /// When undefined, the material does not have an occlusion texture.
    /// @param Texture const TextureInfo& : The occlusion texture to set.
    void SetOcclusionTexture(const TextureInfo& Texture);

    /// @brief Gets the occlusion texture.
    /// @return const TextureInfo& : The occlusion texture.
    const TextureInfo& GetOcclusionTexture() const;

    /// @brief Sets the emissive texture.
    /// @details It controls the color and intensity of the light being emitted by the material.
    /// This texture contains RGB components encoded with the sRGB transfer function.
    /// If a fourth component (A) is present, it MUST be ignored.
    /// When undefined, the texture MUST be sampled as having 1.0 in RGB components.
    /// @param Texture const TextureInfo& : The emissive texture to set.
    void SetEmissiveTexture(const TextureInfo& Texture);

    /// @brief Gets the emissive texture.
    /// @return const TextureInfo& : The emissive texture.
    const TextureInfo& GetEmissiveTexture() const;

    /// @brief Constructor which links the material to an asset.
    /// @param Name const csp::common::String& : The name of the material.
    /// @param MaterialCollectionId const csp::common::String& : The asset collection which holds the associated material asset.
    /// @param MaterialId const csp::common::String& : The asset where the material info is stored.
    GLTFMaterial(const csp::common::String& Name, const csp::common::String& MaterialCollectionId, const csp::common::String& MaterialId);

    virtual ~GLTFMaterial() = default;

    GLTFMaterial();

private:
    EAlphaMode AlphaMode;
    float AlphaCutoff;
    bool IsDoubleSided;

    csp::common::Vector4 BaseColorFactor;
    float MetallicFactor;
    float RoughnessFactor;
    csp::common::Vector3 EmissiveFactor;
    float EmissiveStrength;

    TextureInfo BaseColorTexture;
    TextureInfo MetallicRoughnessTexture;
    TextureInfo NormalTexture;
    TextureInfo OcclusionTexture;
    TextureInfo EmissiveTexture;

    friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj);
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj);
};

} // namespace csp::systems
