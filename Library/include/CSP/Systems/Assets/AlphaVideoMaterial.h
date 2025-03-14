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
class AlphaVideoMaterial;
}

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AlphaVideoMaterial& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AlphaVideoMaterial& Obj);

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{


/// @ingroup Asset System
/// @brief Data class which represents a GLTF material.
class CSP_API AlphaVideoMaterial : public Material
{
public:
    // TODO make the common alpha props into an interface
    /// Sets how to alpha value is interpreted
    /// @param Mode EAlphaMode
    void SetAlphaMode(EAlphaMode Mode);

    /// Gets how to alpha value is interpreted
    /// @return EAlphaMode
    EAlphaMode GetAlphaMode() const;
        
    // TODO make the common alpha props into an interface
    /// Sets how to alpha value is blended
    /// @param Mode EAlphaMode
    void SetBlendMode(EBlendMode Mode);

    /// Gets how to alpha value is blended
    /// @return EBlendMode
    EBlendMode GetBlendMode() const;

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

    /// @brief Sets the color texture. The first three components (RGB) MUST be encoded with the sRGB transfer function.
    /// They specify the color of the material.
    /// If the fourth component (A) is present, it represents the linear alpha coverage of the material.
    /// Otherwise, the alpha coverage is equal to 1.0.
    /// The material.blendMode property defines how the alpha is blended when alphaMode is set to BLEND.
    /// The material.readAlphaFromChannel property defines which color channel to read the alpha values from.
    /// The stored texels MUST NOT be premultiplied.
    /// When undefined, the texture MUST be sampled as having 1.0 in all components.
    /// @param Texture const TextureInfo&
    void SetColorTexture(const TextureInfo& Texture);

    /// @brief Gets the color texture
    /// @return const TextureInfo&
    const TextureInfo& GetColorTexture() const;

    /// @brief Sets whether the material is emissive, if not material should be lit by the scene lighting.
    /// @param isEmissive bool
    void SetIsEmissive(bool isEmissive);

    /// @brief Gets whether the material is emissive, if not material should be lit by the scene lighting.
    /// @return bool
    bool GetIsEmissive() const;

    /// @brief Sets the color channel to read the alpha values from.
    /// @param channel EColorChannel
    void SetReadAlphaFromChannel(EColorChannel channel);

    /// @brief Gets the color channel to read the alpha values from
    /// @return EColorChannel
    EColorChannel GetReadAlphaFromChannel() const;

    /// @brief Constructor which links the material to an asset
    /// @param Name const csp::common::String& : The name of the material.
    /// @param AssetCollectionId const csp::common::String& : The asset collection where the material info is stored
    /// @param AssetId const csp::common::String& : The asset where the material info is stored
    AlphaVideoMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

    AlphaVideoMaterial();

private:
    int Version;
    EAlphaMode AlphaMode;
    EBlendMode BlendMode;
    float AlphaCutoff;
    bool DoubleSided;
    EColorChannel ReadAlphaFromChannel;
    bool IsEmissive;
    TextureInfo ColorTexture;

    friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AlphaVideoMaterial& Obj);
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AlphaVideoMaterial& Obj);
};

} // namespace csp::systems
