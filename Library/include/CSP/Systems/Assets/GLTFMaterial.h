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

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @brief Defines how to alpha value is interpreted
/// The alpha value is taken from the fourth component of the base color for metallic-roughness material model
enum class EAlphaMode
{
    Opaque,
    Mask,
    Blend
};

/// @ingroup Asset System
/// @brief Data class which represents a GLTF material.
class CSP_API GLTFMaterial : public Material
{
public:
    /// Sets how to alpha value is interpreted
    /// @param Mode EAlphaMode
    void SetAlphaMode(EAlphaMode Mode);

    /// Gets how to alpha value is interpreted
    /// @return EAlphaMode
    EAlphaMode GetAlphaMode() const;

    /// @brief Sets the alpha cutoff value
    /// When alphaMode is set to MASK the alphaCutoff property specifies the cutoff threshold.
    /// If the alpha value is greater than or equal to the alphaCutoff value then it is rendered as fully opaque,
    /// otherwise, it is rendered as fully transparent. alphaCutoff value is ignored for other modes.
    /// @param Cutoff float
    void SetAlphaCutoff(float Cutoff);

    /// Gets the alpha cutoff value
    /// @return float
    float GetAlphaCutoff() const;

    /// @brief Sets the doubleSided property which specifies whether the material is double sided.
    /// When this value is false, back - face culling is enabled, i.e., only front-facing triangles are rendered.
    /// When this value is true, back - face culling is disabled and double sided lighting is enabled.
    /// @param DoubleSided bool
    void SetDoubleSided(bool DoubleSided);

    /// @brief Gets the double sided value
    /// @return bool
    bool GetDoubleSided() const;

    /// @brief Sets the factors for the base color of the material.
    /// This value defines linear multipliers for the sampled texels of the base color texture.
    /// @param Factor const csp::common::Vector4&
    void SetBaseColorFactor(const csp::common::Vector4& Factor);

    /// @brief Gets the factor of the base color texture
    /// @return csp::common::Vector4&
    const csp::common::Vector4& GetBaseColorFactor() const;

    /// @brief Sets the factor for the metalness of the material.
    /// This value defines a linear multiplier for the sampled metalness values of the metallic-roughness texture.
    /// @param Factor float
    void SetMetallicFactor(float Factor);

    /// @brief Gets the factor of the metallic texture
    /// @return float
    float GetMetallicFactor() const;

    /// @brief Sets the factor for the roughness of the material.
    /// This value defines a linear multiplier for the sampled roughness values of the metallic-roughness texture.
    /// @param Factor float
    void SetRoughnessFactor(float Factor);

    /// @brief Gets the factor of the roughness texture
    /// @return float
    float GetRoughnessFactor() const;

    /// @brief Sets factors for the emissive color of the material.
    /// This value defines linear multipliers for the sampled texels of the emissive texture.
    /// @param Factor const csp::common::Vector3&
    void SetEmissiveFactor(const csp::common::Vector3& Factor);

    /// @brief Gets the factor of the emissive color texture
    /// @return csp::common::Vector3&
    const csp::common::Vector3& GetEmissiveFactor() const;

    /// @brief Sets the base color texture. The first three components (RGB) MUST be encoded with the sRGB transfer function.
    /// They specify the base color of the material.
    /// If the fourth component (A) is present, it represents the linear alpha coverage of the material.
    /// Otherwise, the alpha coverage is equal to 1.0. The material.alphaMode property specifies how alpha is interpreted.
    /// The stored texels MUST NOT be premultiplied.
    /// When undefined, the texture MUST be sampled as having 1.0 in all components.
    /// @param Texture const TextureInfo&
    void SetBaseColorTexture(const TextureInfo& Texture);

    /// @brief Gets the base color texture
    /// @return const TextureInfo&
    const TextureInfo& GetBaseColorTexture() const;

    /// @brief Sets the metallic-roughness texture.
    /// The metalness values are sampled from the B channel.
    /// The roughness values are sampled from the G channel.
    /// These values MUST be encoded with a linear transfer function.
    /// If other channels are present (R or A), they MUST be ignored for metallic-roughness calculations.
    /// When undefined, the texture MUST be sampled as having 1.0 in G and B components.
    /// @param Texture const TextureInfo&
    void SetMetallicRoughnessTexture(const TextureInfo& Texture);

    /// @brief Gets the metallic-roughness texture
    /// @return const TextureInfo&
    const TextureInfo& GetMetallicRoughnessTexture() const;

    /// @brief Sets the tangent space normal texture.
    /// The texture encodes RGB components with linear transfer function.
    /// Each texel represents the XYZ components of a normal vector in tangent space.
    /// The normal vectors use the convention +X is right and +Y is up. +Z points toward the viewer.
    /// If a fourth component (A) is present, it MUST be ignored. When undefined, the material does not have a tangent space normal texture.
    /// @param Texture const TextureInfo&
    void SetNormalTexture(const TextureInfo& Texture);

    /// @brief Gets the tangent space normal texture.
    /// @return const TextureInfo&
    const TextureInfo& GetNormalTexture() const;

    /// @brief Sets the occlusion texture.
    /// The occlusion values are linearly sampled from the R channel.
    /// Higher values indicate areas that receive full indirect lighting and lower values indicate no indirect lighting.
    /// If other channels are present (GBA), they MUST be ignored for occlusion calculations.
    /// When undefined, the material does not have an occlusion texture.
    /// @param Texture const TextureInfo&
    void SetOcclusionTexture(const TextureInfo& Texture);

    /// @brief Gets the occlusion texture
    /// @return const TextureInfo&
    const TextureInfo& GetOcclusionTexture() const;

    /// @brief Sets the emissive texture.
    /// It controls the color and intensity of the light being emitted by the material.
    /// This texture contains RGB components encoded with the sRGB transfer function.
    /// If a fourth component (A) is present, it MUST be ignored.
    /// When undefined, the texture MUST be sampled as having 1.0 in RGB components.
    /// @param Texture const TextureInfo&
    void SetEmissiveTexture(const TextureInfo& Texture);

    /// @brief Gets the emissive texture
    /// @return const TextureInfo&
    const TextureInfo& GetEmissiveTexture() const;

    /// @brief Constructor which links the material to an asset
    /// @param Name const csp::common::String& : The name of the material.
    /// @param AssetCollectionId const csp::common::String& : The asset collection where the material info is stored
    /// @param AssetId const csp::common::String& : The asset where the material info is stored
    GLTFMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

    GLTFMaterial();

private:
    int Version;

    EAlphaMode AlphaMode;
    float AlphaCutoff;
    bool DoubleSided;

    csp::common::Vector4 BaseColorFactor;
    float MetallicFactor;
    float RoughnessFactor;
    csp::common::Vector3 EmissiveFactor;

    TextureInfo BaseColorTexture;
    TextureInfo MetallicRoughnessTexture;
    TextureInfo NormalTexture;
    TextureInfo OcclusionTexture;
    TextureInfo EmissiveTexture;

    friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj);
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj);
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download material data.
class CSP_API GLTFMaterialResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the GLTFMaterial from the result.
    const GLTFMaterial& GetGLTFMaterial() const;

    CSP_NO_EXPORT GLTFMaterialResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    GLTFMaterialResult(void*) {};

    void SetGLTFMaterial(const GLTFMaterial& Material);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    GLTFMaterial Material;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download a collection of material data.
class CSP_API GLTFMaterialsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the GLTFMaterial from the result.
    const csp::common::Array<GLTFMaterial>& GetGLTFMaterials() const;

    CSP_NO_EXPORT GLTFMaterialsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    GLTFMaterialsResult(void*) {};

    void SetGLTFMaterials(const csp::common::Array<GLTFMaterial>& Materials);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<GLTFMaterial> Materials;
};

/// @brief Callback containing material data.
/// @param Result GLTFMaterialResult : result class
typedef std::function<void(const GLTFMaterialResult& Result)> GLTFMaterialResultCallback;

/// @brief Callback containing a collection of material data.
/// @param Result Array<GLTFMaterialResult> : result class
typedef std::function<void(const GLTFMaterialsResult& Result)> GLTFMaterialsResultCallback;

} // namespace csp::systems
