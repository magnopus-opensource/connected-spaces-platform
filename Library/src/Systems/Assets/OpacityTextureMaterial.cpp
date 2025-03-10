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

    // Alpha Cutoff
    Serializer.SerializeMember("alphaCutoff", Obj.AlphaCutoff);

    // Double Sided
    Serializer.SerializeMember("doubleSided", Obj.DoubleSided);

    // ReadAlphaFromChannel
    Serializer.SerializeMember("readAlphafromChannel", Obj.ReadAlphaFromChannel);

    // Textures
    if (Obj.BaseColorTexture.IsSet())
    {
        Serializer.SerializeMember("baseColorTexture", Obj.BaseColorTexture);
    }
    if (Obj.OpacityTexture.IsSet())
    {
        Serializer.SerializeMember("opacityTexture", Obj.OpacityTexture);
    }
    if (Obj.EmissiveTexture.IsSet())
    {
        Serializer.SerializeMember("emissiveTexture", Obj.EmissiveTexture);
    }
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

    // Alpha Cutoff
    Deserializer.DeserializeMember("alphaCutoff", Obj.AlphaCutoff);

    // Double Sided
    Deserializer.DeserializeMember("doubleSided", Obj.DoubleSided);

    // Textures
    if (Deserializer.HasProperty("baseColorTexture"))
    {
        Deserializer.DeserializeMember("baseColorTexture", Obj.BaseColorTexture);
        Obj.BaseColorTexture.SetTexture(true);
    }
    if (Deserializer.HasProperty("emissiveTexture"))
    {
        Deserializer.DeserializeMember("emissiveTexture", Obj.EmissiveTexture);
        Obj.EmissiveTexture.SetTexture(true);
    }
}

namespace csp::systems
{

void OpacityTextureMaterial::SetBaseColorTexture(const TextureInfo& Texture) { BaseColorTexture = Texture; }

const TextureInfo& OpacityTextureMaterial::GetBaseColorTexture() const { return BaseColorTexture; }

void OpacityTextureMaterial::SetOpacityTexture(const TextureInfo& Texture) { OpacityTexture = Texture; }

const TextureInfo& OpacityTextureMaterial::GetOpacityTexture() const { return OpacityTexture; }

void OpacityTextureMaterial::SetEmissiveTexture(const TextureInfo& Texture) { EmissiveTexture = Texture; }

const TextureInfo& OpacityTextureMaterial::GetEmissiveTexture() const { return EmissiveTexture; }

OpacityTextureMaterial::OpacityTextureMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
    : Material(Name, AssetCollectionId, AssetId)
    , Version(1)
    , AlphaMode(EAlphaMode::Opaque)
    , AlphaCutoff(0.5f)
    , DoubleSided(false)
    , BaseColorTexture()
    , OpacityTexture()
    , EmissiveTexture()
{
    BaseColorTexture.SetTexture(false);
    OpacityTexture.SetTexture(false);
    EmissiveTexture.SetTexture(false);
}

OpacityTextureMaterial::OpacityTextureMaterial()
    : OpacityTextureMaterial("", "", "")
{
}

void OpacityTextureMaterial::SetAlphaMode(EAlphaMode Mode) { AlphaMode = Mode; }

EAlphaMode OpacityTextureMaterial::GetAlphaMode() const { return AlphaMode; }

void OpacityTextureMaterial::SetAlphaCutoff(float Mode) { AlphaCutoff = Mode; }

float OpacityTextureMaterial::GetAlphaCutoff() const { return AlphaCutoff; }

void OpacityTextureMaterial::SetDoubleSided(bool InDoubleSided) { DoubleSided = InDoubleSided; }

bool OpacityTextureMaterial::GetDoubleSided() const { return DoubleSided; }

void OpacityTextureMaterial::SetReadAlphaFromChannel(EColorChannel InReadSetReadAlphaFromChannel) { ReadAlphaFromChannel = InReadSetReadAlphaFromChannel; }

EColorChannel OpacityTextureMaterial::GetReadAlphaFromChannel() const { return ReadAlphaFromChannel; }

const OpacityTextureMaterial& OpacityTextureMaterialResult::GetOpacityTextureMaterial() const { return Material; }

void OpacityTextureMaterialResult::SetOpacityTextureMaterial(const OpacityTextureMaterial& InMaterial) { Material = InMaterial; }

void OpacityTextureMaterialResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { }

const csp::common::Array<OpacityTextureMaterial>& OpacityTextureMaterialsResult::GetOpacityTextureMaterials() const { return Materials; }

void OpacityTextureMaterialsResult::SetOpacityTextureMaterials(const csp::common::Array<OpacityTextureMaterial>& InMaterials) { Materials = InMaterials; }

void OpacityTextureMaterialsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) { }

} // namespace csp::systems
