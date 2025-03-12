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

#include "CSP/Systems/Assets/OpacityTextureMaterial.h"

#include "CSP/Common/Array.h"

#include "Json/JsonSerializer.h"

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::OpacityTextureMaterial& Obj)
{
    // Name
    Serializer.SerializeMember("name", Obj.Name);

    // ShaderType
    Serializer.SerializeMember("shaderType", static_cast<uint32_t>(Obj.Type));

    // Version
    Serializer.SerializeMember("version", Obj.Version);

    // Alpha Mode
    Serializer.SerializeMember("alphaMode", static_cast<uint32_t>(Obj.AlphaMode));

    // Blend Mode
    Serializer.SerializeMember("blendMode", static_cast<uint32_t>(Obj.BlendMode));

    // Alpha Cutoff
    Serializer.SerializeMember("alphaCutoff", Obj.AlphaCutoff);

    // Double Sided
    Serializer.SerializeMember("doubleSided", Obj.DoubleSided);

    // ReadAlphaFromChannel
    Serializer.SerializeMember("readAlphafromChannel", static_cast<uint32_t>(Obj.ReadAlphaFromChannel));

    // Textures
    if (Obj.ColorTexture.IsSet())
    {
        Serializer.SerializeMember("colorTexture", Obj.ColorTexture);
    }

    // Double Sided
    Serializer.SerializeMember("isEmissive", Obj.IsEmissive);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::OpacityTextureMaterial& Obj)
{
    // Name
    Deserializer.DeserializeMember("name", Obj.Name);

    // Shader Type
    uint32_t ShaderType;
    Deserializer.DeserializeMember("shaderType", ShaderType);

    Obj.Type = static_cast<csp::systems::EShaderType>(ShaderType);

    // Version
    Deserializer.DeserializeMember("version", Obj.Version);

    // Alpha Mode
    uint32_t AlphaMode;
    Deserializer.DeserializeMember("alphaMode", AlphaMode);
    Obj.AlphaMode = static_cast<csp::systems::EAlphaMode>(AlphaMode);

    // Blend Mode
    uint32_t BlendMode;
    Deserializer.DeserializeMember("blendMode", BlendMode);
    Obj.BlendMode = static_cast<csp::systems::EBlendMode>(BlendMode);

    // Alpha Cutoff
    Deserializer.DeserializeMember("alphaCutoff", Obj.AlphaCutoff);

    // Double Sided
    Deserializer.DeserializeMember("doubleSided", Obj.DoubleSided);

    // Emissive
    Deserializer.DeserializeMember("isEmissive", Obj.IsEmissive);

    // ReadAlphaFromChannel
    uint32_t ReadAlphaFromChannel;
    Deserializer.DeserializeMember("readAlphaFromChannel", ReadAlphaFromChannel);
    Obj.ReadAlphaFromChannel = static_cast<csp::systems::EColorChannel>(ReadAlphaFromChannel);

    // Textures
    if (Deserializer.HasProperty("colorTexture"))
    {
        Deserializer.DeserializeMember("colorTexture", Obj.ColorTexture);
        Obj.ColorTexture.SetTexture(true);
    }
}

namespace csp::systems
{

OpacityTextureMaterial::OpacityTextureMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
    : Material(Name, AssetCollectionId, AssetId)
    , Version(1)
    , AlphaMode(EAlphaMode::Opaque)
    , BlendMode(EBlendMode::Normal)
    , AlphaCutoff(0.5f)
    , DoubleSided(false)
    , IsEmissive(true)
    , ColorTexture()
{
    ColorTexture.SetTexture(false);
}

OpacityTextureMaterial::OpacityTextureMaterial()
    : OpacityTextureMaterial("", "", "")
{
}

void OpacityTextureMaterial::SetAlphaMode(EAlphaMode Mode) { AlphaMode = Mode; }

EAlphaMode OpacityTextureMaterial::GetAlphaMode() const { return AlphaMode; }

void OpacityTextureMaterial::SetBlendMode(EBlendMode Mode) { BlendMode = Mode; }

EBlendMode OpacityTextureMaterial::GetBlendMode() const { return BlendMode; }

void OpacityTextureMaterial::SetAlphaCutoff(float Mode) { AlphaCutoff = Mode; }

float OpacityTextureMaterial::GetAlphaCutoff() const { return AlphaCutoff; }

void OpacityTextureMaterial::SetDoubleSided(bool InDoubleSided) { DoubleSided = InDoubleSided; }

bool OpacityTextureMaterial::GetDoubleSided() const { return DoubleSided; }

void OpacityTextureMaterial::SetIsEmissive(bool InIsEmissive) { IsEmissive = InIsEmissive; }

bool OpacityTextureMaterial::GetIsEmissive() const { return IsEmissive; }

void OpacityTextureMaterial::SetReadAlphaFromChannel(EColorChannel InReadSetReadAlphaFromChannel) { ReadAlphaFromChannel = InReadSetReadAlphaFromChannel; }

EColorChannel OpacityTextureMaterial::GetReadAlphaFromChannel() const { return ReadAlphaFromChannel; }

void OpacityTextureMaterial::SetColorTexture(const TextureInfo& Texture) { ColorTexture = Texture; }

const TextureInfo& OpacityTextureMaterial::GetColorTexture() const { return ColorTexture; }

const OpacityTextureMaterial& OpacityTextureMaterialResult::GetOpacityTextureMaterial() const { return Material; }

void OpacityTextureMaterialResult::SetOpacityTextureMaterial(const OpacityTextureMaterial& InMaterial) { Material = InMaterial; }

void OpacityTextureMaterialResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { }

const csp::common::Array<OpacityTextureMaterial>& OpacityTextureMaterialsResult::GetOpacityTextureMaterials() const { return Materials; }

void OpacityTextureMaterialsResult::SetOpacityTextureMaterials(const csp::common::Array<OpacityTextureMaterial>& InMaterials) { Materials = InMaterials; }

void OpacityTextureMaterialsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { }

} // namespace csp::systems
