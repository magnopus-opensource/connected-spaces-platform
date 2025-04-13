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
class AlphaVideoMaterial;
}

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AlphaVideoMaterial& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AlphaVideoMaterial& Obj);

namespace csp::systems
{

/// @ingroup Asset System
/// @brief Data class which represents an Alpha Video material.
class CSP_API AlphaVideoMaterial : public Material
{
public:
    /// @brief Sets the color texture.
    /// @details The first three components (RGB) MUST be encoded with the sRGB transfer function. They specify the color of the material.
    /// If the fourth component (A) is present, it represents the linear alpha coverage of the material.
    /// Otherwise, the alpha coverage is equal to 1.0.
    /// The material.blendMode property defines how the alpha is blended when alphaMode is set to BLEND.
    /// The material.readAlphaFromChannel property defines which color channel to read the alpha values from.
    /// The stored texels MUST NOT be premultiplied.
    /// @param Texture const TextureInfo& : The material color texture.
    /// @pre If the fourth component (A) is undefined, the texture MUST be sampled as having 1.0 in all components.
    void SetColorTexture(const TextureInfo& Texture);

    /// @brief Gets the color texture.
    /// @return const TextureInfo& : The material color texture.
    const TextureInfo& GetColorTexture() const;

    /// @brief Sets the doubleSided property which specifies whether the material is double sided.
    /// @details When this value is false, back-face culling is enabled, i.e., only front-facing triangles are rendered.
    /// When this value is true, back-face culling is disabled and double sided lighting is enabled.
    /// @param DoubleSided bool : The double sided value.
    void SetDoubleSided(bool DoubleSided);

    /// @brief Gets the double sided value.
    /// @return bool : The double sided value.
    bool GetDoubleSided() const;

    /// @brief Sets whether the material is emissive, if not material should be lit by the scene lighting.
    /// @param IsEmissive bool : The emissive value.
    void SetIsEmissive(bool IsEmissive);

    /// @brief Gets whether the material is emissive, if not material should be lit by the scene lighting.
    /// @return bool
    bool GetIsEmissive() const;

    /// @brief Sets the color channel to read the alpha values from.
    /// @param ColorChannel EColorChannel : The color channel to read the alpha values from.
    void SetReadAlphaFromChannel(EColorChannel ColorChannel);

    /// @brief Gets the color channel to read the alpha values from.
    /// @return EColorChannel : The color channel to read the alpha values from.
    EColorChannel GetReadAlphaFromChannel() const;

    /// @brief Sets the mode to use for alpha blending.
    /// @param Mode EBlendMode : The mode to use for alpha blending.
    void SetBlendMode(EBlendMode Mode);

    /// @brief Gets the alpha blend mode.
    /// @return EBlendMode : The alpha blend mode.
    EBlendMode GetBlendMode() const;

    /// @brief Sets the fresnel factor.
    /// @param Factor float : The fresnel factor.
    void SetFresnelFactor(float Factor);

    /// @brief Gets the fresnel factor.
    /// @return float : The fresnel factor.
    float GetFresnelFactor() const;

    /// @brief Set the Material tint.
    /// @param Tint const csp::common::Vector3& : The tint value.
    void SetTint(const csp::common::Vector3& Tint);

    /// @brief Get the Material tint.
    /// @return const csp::common::Vector3& GetTint() : The tint value.
    const csp::common::Vector3& GetTint() const;

    /// @brief Set the alpha factor.
    /// @param Factor float : The alpha factor.
    void SetAlphaFactor(float Factor);

    /// @brief Get the alpha factor.
    /// @return float : The alpha factor.
    float GetAlphaFactor() const;

    /// @brief Set the emissive intensity.
    /// @param Intensity float : The emissive intensity.
    void SetEmissiveIntensity(float Intensity);

    /// @brief Get the emissive intensity.
    /// @return float : The emissive intensity.
    float GetEmissiveIntensity() const;

    /// @brief Set the alpha mask.
    /// @param Mask float : The alpha mask.
    void SetAlphaMask(float Mask);

    /// @brief Get the alpha mask.
    /// @return float : The alpha mask.
    float GetAlphaMask() const;

    /// @brief Constructor which links the material to an asset.
    /// @param Name const csp::common::String& : The name of the material.
    /// @param MaterialCollectionId const csp::common::String& : The asset collection which holds the associated material asset.
    /// @param MaterialId const csp::common::String& : The asset where the material info is stored.
    AlphaVideoMaterial(const csp::common::String& Name, const csp::common::String& MaterialCollectionId, const csp::common::String& MaterialId);

    virtual ~AlphaVideoMaterial() = default;
    AlphaVideoMaterial();

private:
    TextureInfo ColorTexture;
    bool IsDoubleSided;
    bool IsEmissive;
    EColorChannel ReadAlphaFromChannel;
    EBlendMode BlendMode;
    float FresnelFactor;
    csp::common::Vector3 MaterialTint;
    float AlphaFactor;
    float EmissiveIntensity;
    float AlphaMask;

    friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AlphaVideoMaterial& Obj);
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AlphaVideoMaterial& Obj);
};

} // namespace csp::systems
