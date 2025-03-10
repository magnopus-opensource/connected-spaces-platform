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
class OpacityTextureMaterial;
}

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::OpacityTextureMaterial& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::OpacityTextureMaterial& Obj);

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
class CSP_API OpacityTextureMaterial : public Material
{
public:
    // TODO make the common alpha props into an interface
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

    /// @brief Sets the base color texture. The first three components (RGB) MUST be encoded with the sRGB transfer function.
    /// They specify the base color of the material.
    /// If the fourth component (A) is present, it represents the linear alpha coverage of the material.
    /// Otherwise, the alpha coverage is equal to 1.0. The material.alphaMode property specifies how alpha is interpreted.
    /// The stored texels MUST NOT be premultiplied.
    /// When undefined, the texture MUST be sampled as having 1.0 in all components.
    /// @param Texture const TextureInfo&
    void SetOpacityTexture(const TextureInfo& Texture);

    /// @brief Gets the base color texture
    /// @return const TextureInfo&
    const TextureInfo& GetOpacityTexture() const;

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

    /// @brief Sets the color channel to read the alpha values from
    /// @param channel EColorChannel
    void SetReadAlphaFromChannel(EColorChannel channel);

    /// @brief Gets the color channel to read the alpha values from
    /// @return EColorChannel
    EColorChannel GetReadAlphaFromChannel() const;

    /// @brief Constructor which links the material to an asset
    /// @param Name const csp::common::String& : The name of the material.
    /// @param AssetCollectionId const csp::common::String& : The asset collection where the material info is stored
    /// @param AssetId const csp::common::String& : The asset where the material info is stored
    OpacityTextureMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

    OpacityTextureMaterial();

private:
    int Version;

    EAlphaMode AlphaMode;
    float AlphaCutoff;
    bool DoubleSided;
    EColorChannel ReadAlphaFromChannel;
   // bool useDepthFade;
    //bool useFresnel;
    //bool useDistanceFade;

    TextureInfo BaseColorTexture;
    TextureInfo OpacityTexture;
    TextureInfo EmissiveTexture;

    friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::OpacityTextureMaterial& Obj);
    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::OpacityTextureMaterial& Obj);
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download material data.
class CSP_API OpacityTextureMaterialResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the OpacityTextureMaterial from the result.
    const OpacityTextureMaterial& GetOpacityTextureMaterial() const;

    CSP_NO_EXPORT OpacityTextureMaterialResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    OpacityTextureMaterialResult(void*) {};

    void SetOpacityTextureMaterial(const OpacityTextureMaterial& Material);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    OpacityTextureMaterial Material;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download a collection of material data.
class CSP_API OpacityTextureMaterialsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the OpacityTextureMaterial from the result.
    const csp::common::Array<OpacityTextureMaterial>& GetOpacityTextureMaterials() const;

    CSP_NO_EXPORT OpacityTextureMaterialsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    OpacityTextureMaterialsResult(void*) {};

    void SetOpacityTextureMaterials(const csp::common::Array<OpacityTextureMaterial>& Materials);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<OpacityTextureMaterial> Materials;
};

/// @brief Callback containing material data.
/// @param Result OpacityTextureMaterialResult : result class
typedef std::function<void(const OpacityTextureMaterialResult& Result)> OpacityTextureMaterialResultCallback;

/// @brief Callback containing a collection of material data.
/// @param Result Array<OpacityTextureMaterialResult> : result class
typedef std::function<void(const OpacityTextureMaterialsResult& Result)> OpacityTextureMaterialsResultCallback;

} // namespace csp::systems
