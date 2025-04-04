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

#include "CSP/Systems/Assets/GLTFMaterial.h"

#include "CSP/Common/Array.h"

#include "Debug/Logging.h"
#include "Json/JsonSerializer.h"

namespace
{
struct PropertyNames
{
    static constexpr const char* NAME = "name";
    static constexpr const char* SHADER_TYPE = "shaderType";
    static constexpr const char* VERSION = "version";
    static constexpr const char* ALPHA_MODE = "alphaMode";
    static constexpr const char* ALPHA_CUTOFF = "alphaCutoff";
    static constexpr const char* DOUBLE_SIDED = "doubleSided";
    static constexpr const char* BASE_COLOR_FACTOR = "baseColorFactor";
    static constexpr const char* METALLIC_FACTOR = "metallicFactor";
    static constexpr const char* ROUGHNESS_FACTOR = "roughnessFactor";
    static constexpr const char* EMISSIVE_FACTOR = "emissiveFactor";
    static constexpr const char* BASE_COLOR_TEX = "baseColorTexture";
    static constexpr const char* METALLIC_ROUGH_TEX = "metallicRoughnessTexture";
    static constexpr const char* NORMAL_TEX = "normalTexture";
    static constexpr const char* OCCLUSION_TEX = "occlusionTexture";
    static constexpr const char* EMISSIVE_TEX = "emissiveTexture";
};
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj)
{
    Serializer.SerializeMember(PropertyNames::NAME, Obj.Name);
    Serializer.SerializeMember(PropertyNames::SHADER_TYPE, static_cast<uint32_t>(Obj.Type));
    Serializer.SerializeMember(PropertyNames::VERSION, Obj.Version);
    Serializer.SerializeMember(PropertyNames::ALPHA_MODE, static_cast<uint32_t>(Obj.AlphaMode));
    Serializer.SerializeMember(PropertyNames::ALPHA_CUTOFF, Obj.AlphaCutoff);
    Serializer.SerializeMember(PropertyNames::DOUBLE_SIDED, Obj.DoubleSided);
    Serializer.SerializeMember(PropertyNames::BASE_COLOR_FACTOR,
        csp::common::Array<float> { Obj.BaseColorFactor.X, Obj.BaseColorFactor.Y, Obj.BaseColorFactor.Z, Obj.BaseColorFactor.W });
    Serializer.SerializeMember(PropertyNames::METALLIC_FACTOR, Obj.MetallicFactor);
    Serializer.SerializeMember(PropertyNames::ROUGHNESS_FACTOR, Obj.RoughnessFactor);
    Serializer.SerializeMember(
        PropertyNames::EMISSIVE_FACTOR, csp::common::Array<float> { Obj.EmissiveFactor.X, Obj.EmissiveFactor.Y, Obj.EmissiveFactor.Z });

    if (Obj.BaseColorTexture.IsSet())
        Serializer.SerializeMember(PropertyNames::BASE_COLOR_TEX, Obj.BaseColorTexture);
    if (Obj.MetallicRoughnessTexture.IsSet())
        Serializer.SerializeMember(PropertyNames::METALLIC_ROUGH_TEX, Obj.MetallicRoughnessTexture);
    if (Obj.NormalTexture.IsSet())
        Serializer.SerializeMember(PropertyNames::NORMAL_TEX, Obj.NormalTexture);
    if (Obj.OcclusionTexture.IsSet())
        Serializer.SerializeMember(PropertyNames::OCCLUSION_TEX, Obj.OcclusionTexture);
    if (Obj.EmissiveTexture.IsSet())
        Serializer.SerializeMember(PropertyNames::EMISSIVE_TEX, Obj.EmissiveTexture);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj)
{
    Deserializer.DeserializeMember(PropertyNames::NAME, Obj.Name);

    uint32_t ShaderType;
    Deserializer.DeserializeMember(PropertyNames::SHADER_TYPE, ShaderType);
    Obj.Type = static_cast<csp::systems::EShaderType>(ShaderType);

    Deserializer.DeserializeMember(PropertyNames::VERSION, Obj.Version);

    uint32_t AlphaMode;
    Deserializer.DeserializeMember(PropertyNames::ALPHA_MODE, AlphaMode);
    Obj.AlphaMode = static_cast<csp::systems::EAlphaMode>(AlphaMode);

    Deserializer.DeserializeMember(PropertyNames::ALPHA_CUTOFF, Obj.AlphaCutoff);
    Deserializer.DeserializeMember(PropertyNames::DOUBLE_SIDED, Obj.DoubleSided);

    csp::common::Array<float> BaseColorFactorArray;
    Deserializer.DeserializeMember(PropertyNames::BASE_COLOR_FACTOR, BaseColorFactorArray);
    Obj.BaseColorFactor = csp::common::Vector4(BaseColorFactorArray[0], BaseColorFactorArray[1], BaseColorFactorArray[2], BaseColorFactorArray[3]);

    Deserializer.DeserializeMember(PropertyNames::METALLIC_FACTOR, Obj.MetallicFactor);
    Deserializer.DeserializeMember(PropertyNames::ROUGHNESS_FACTOR, Obj.RoughnessFactor);

    csp::common::Array<float> EmissiveFactorArray;
    Deserializer.DeserializeMember(PropertyNames::EMISSIVE_FACTOR, EmissiveFactorArray);
    Obj.EmissiveFactor = csp::common::Vector3(EmissiveFactorArray[0], EmissiveFactorArray[1], EmissiveFactorArray[2]);

    if (Deserializer.HasProperty(PropertyNames::BASE_COLOR_TEX))
    {
        Deserializer.DeserializeMember(PropertyNames::BASE_COLOR_TEX, Obj.BaseColorTexture);
        Obj.BaseColorTexture.SetTexture(true);
    }
    if (Deserializer.HasProperty(PropertyNames::METALLIC_ROUGH_TEX))
    {
        Deserializer.DeserializeMember(PropertyNames::METALLIC_ROUGH_TEX, Obj.MetallicRoughnessTexture);
        Obj.MetallicRoughnessTexture.SetTexture(true);
    }
    if (Deserializer.HasProperty(PropertyNames::NORMAL_TEX))
    {
        Deserializer.DeserializeMember(PropertyNames::NORMAL_TEX, Obj.NormalTexture);
        Obj.NormalTexture.SetTexture(true);
    }
    if (Deserializer.HasProperty(PropertyNames::OCCLUSION_TEX))
    {
        Deserializer.DeserializeMember(PropertyNames::OCCLUSION_TEX, Obj.OcclusionTexture);
        Obj.OcclusionTexture.SetTexture(true);
    }
    if (Deserializer.HasProperty(PropertyNames::EMISSIVE_TEX))
    {
        Deserializer.DeserializeMember(PropertyNames::EMISSIVE_TEX, Obj.EmissiveTexture);
        Obj.EmissiveTexture.SetTexture(true);
    }
}

namespace csp::systems
{

void GLTFMaterial::SetBaseColorFactor(const csp::common::Vector4& Factor) { BaseColorFactor = Factor; }

const csp::common::Vector4& GLTFMaterial::GetBaseColorFactor() const { return BaseColorFactor; }

void GLTFMaterial::SetMetallicFactor(float Factor) { MetallicFactor = Factor; }

float GLTFMaterial::GetMetallicFactor() const { return MetallicFactor; }

void GLTFMaterial::SetRoughnessFactor(float Factor) { RoughnessFactor = Factor; }

float GLTFMaterial::GetRoughnessFactor() const { return RoughnessFactor; }

void GLTFMaterial::SetEmissiveFactor(const csp::common::Vector3& Factor) { EmissiveFactor = Factor; }

const csp::common::Vector3& GLTFMaterial::GetEmissiveFactor() const { return EmissiveFactor; }

void GLTFMaterial::SetBaseColorTexture(const TextureInfo& Texture) { BaseColorTexture = Texture; }

const TextureInfo& GLTFMaterial::GetBaseColorTexture() const { return BaseColorTexture; }

void GLTFMaterial::SetMetallicRoughnessTexture(const TextureInfo& Texture) { MetallicRoughnessTexture = Texture; }

const TextureInfo& GLTFMaterial::GetMetallicRoughnessTexture() const { return MetallicRoughnessTexture; }

void GLTFMaterial::SetNormalTexture(const TextureInfo& Texture) { NormalTexture = Texture; }

void GLTFMaterial::SetOcclusionTexture(const TextureInfo& Texture) { OcclusionTexture = Texture; }

const TextureInfo& GLTFMaterial::GetOcclusionTexture() const { return OcclusionTexture; }

const TextureInfo& GLTFMaterial::GetNormalTexture() const { return NormalTexture; }

void GLTFMaterial::SetEmissiveTexture(const TextureInfo& Texture) { EmissiveTexture = Texture; }

const TextureInfo& GLTFMaterial::GetEmissiveTexture() const { return EmissiveTexture; }

GLTFMaterial::GLTFMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
    : Material(Name, AssetCollectionId, AssetId, csp::systems::EShaderType::Standard, 1)
    , AlphaMode(EAlphaMode::Opaque)
    , AlphaCutoff(0.5f)
    , DoubleSided(false)
    , BaseColorFactor(1.f, 1.f, 1.f, 1.f)
    , MetallicFactor(1.f)
    , RoughnessFactor(1.f)
    , EmissiveFactor(0.f, 0.f, 0.f)
    , BaseColorTexture()
    , MetallicRoughnessTexture()
    , NormalTexture()
    , OcclusionTexture()
    , EmissiveTexture()
{
    BaseColorTexture.SetTexture(false);
    MetallicRoughnessTexture.SetTexture(false);
    NormalTexture.SetTexture(false);
    OcclusionTexture.SetTexture(false);
    EmissiveTexture.SetTexture(false);
}

GLTFMaterial::GLTFMaterial()
    : GLTFMaterial("", "", "")
{
}

void GLTFMaterial::SetAlphaMode(EAlphaMode Mode) { AlphaMode = Mode; }

EAlphaMode GLTFMaterial::GetAlphaMode() const { return AlphaMode; }

void GLTFMaterial::SetAlphaCutoff(float Mode) { AlphaCutoff = Mode; }

float GLTFMaterial::GetAlphaCutoff() const { return AlphaCutoff; }

void GLTFMaterial::SetDoubleSided(bool InDoubleSided) { DoubleSided = InDoubleSided; }

bool GLTFMaterial::GetDoubleSided() const { return DoubleSided; }

} // namespace csp::systems
