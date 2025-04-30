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

#include "CSP/Systems/Assets/GLTFMaterial.h"

#include "CSP/Common/Array.h"

#include "Debug/Logging.h"
#include "Json/JsonSerializer.h"

namespace
{
constexpr int InitialMaterialVersion = 1;
}

namespace GLTFMaterialProperties
{
static constexpr const char* Name = "name";
static constexpr const char* ShaderType = "shaderType";
static constexpr const char* Version = "version";
static constexpr const char* AlphaMode = "alphaMode";
static constexpr const char* AlphaCutoff = "alphaCutoff";
static constexpr const char* DoubleSided = "doubleSided";
static constexpr const char* BaseColorFactor = "baseColorFactor";
static constexpr const char* MetallicFactor = "metallicFactor";
static constexpr const char* RoughnessFactor = "roughnessFactor";
static constexpr const char* EmissiveFactor = "emissiveFactor";
static constexpr const char* EmissiveIntensity = "emissiveIntensity";
static constexpr const char* BaseColorTex = "baseColorTexture";
static constexpr const char* MetallicRoughTex = "metallicRoughnessTexture";
static constexpr const char* NormalTex = "normalTexture";
static constexpr const char* OcclusionTex = "occlusionTexture";
static constexpr const char* EmissiveTex = "emissiveTexture";
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj)
{
    Serializer.SerializeMember(GLTFMaterialProperties::Name, Obj.Name);
    Serializer.SerializeMember(GLTFMaterialProperties::ShaderType, static_cast<uint32_t>(Obj.Type));
    Serializer.SerializeMember(GLTFMaterialProperties::Version, Obj.Version);
    Serializer.SerializeMember(GLTFMaterialProperties::AlphaMode, static_cast<uint32_t>(Obj.AlphaMode));
    Serializer.SerializeMember(GLTFMaterialProperties::AlphaCutoff, Obj.AlphaCutoff);
    Serializer.SerializeMember(GLTFMaterialProperties::DoubleSided, Obj.IsDoubleSided);
    Serializer.SerializeMember(GLTFMaterialProperties::BaseColorFactor,
        csp::common::Array<float> { Obj.BaseColorFactor.X, Obj.BaseColorFactor.Y, Obj.BaseColorFactor.Z, Obj.BaseColorFactor.W });
    Serializer.SerializeMember(GLTFMaterialProperties::MetallicFactor, Obj.MetallicFactor);
    Serializer.SerializeMember(GLTFMaterialProperties::RoughnessFactor, Obj.RoughnessFactor);
    Serializer.SerializeMember(
        GLTFMaterialProperties::EmissiveFactor, csp::common::Array<float> { Obj.EmissiveFactor.X, Obj.EmissiveFactor.Y, Obj.EmissiveFactor.Z });
    Serializer.SerializeMember(GLTFMaterialProperties::EmissiveIntensity, Obj.EmissiveIntensity);

    if (Obj.BaseColorTexture.IsSet())
        Serializer.SerializeMember(GLTFMaterialProperties::BaseColorTex, Obj.BaseColorTexture);
    if (Obj.MetallicRoughnessTexture.IsSet())
        Serializer.SerializeMember(GLTFMaterialProperties::MetallicRoughTex, Obj.MetallicRoughnessTexture);
    if (Obj.NormalTexture.IsSet())
        Serializer.SerializeMember(GLTFMaterialProperties::NormalTex, Obj.NormalTexture);
    if (Obj.OcclusionTexture.IsSet())
        Serializer.SerializeMember(GLTFMaterialProperties::OcclusionTex, Obj.OcclusionTexture);
    if (Obj.EmissiveTexture.IsSet())
        Serializer.SerializeMember(GLTFMaterialProperties::EmissiveTex, Obj.EmissiveTexture);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj)
{
    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::Name, Obj.Name);

    uint32_t ShaderType;
    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::ShaderType, ShaderType);
    Obj.Type = static_cast<csp::systems::EShaderType>(ShaderType);

    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::Version, Obj.Version);

    uint32_t AlphaMode;
    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::AlphaMode, AlphaMode);
    Obj.AlphaMode = static_cast<csp::systems::EAlphaMode>(AlphaMode);

    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::AlphaCutoff, Obj.AlphaCutoff);
    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::DoubleSided, Obj.IsDoubleSided);

    csp::common::Array<float> BaseColorFactorArray;
    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::BaseColorFactor, BaseColorFactorArray))
    {
        Obj.BaseColorFactor
            = csp::common::Vector4(BaseColorFactorArray[0], BaseColorFactorArray[1], BaseColorFactorArray[2], BaseColorFactorArray[3]);
    }

    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::MetallicFactor, Obj.MetallicFactor);
    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::RoughnessFactor, Obj.RoughnessFactor);

    csp::common::Array<float> EmissiveFactorArray;
    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::EmissiveFactor, EmissiveFactorArray))
    {
        Obj.EmissiveFactor = csp::common::Vector3(EmissiveFactorArray[0], EmissiveFactorArray[1], EmissiveFactorArray[2]);
    }

    Deserializer.SafeDeserializeMember(GLTFMaterialProperties::EmissiveIntensity, Obj.EmissiveIntensity);

    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::BaseColorTex, Obj.BaseColorTexture))
    {
        Obj.BaseColorTexture.SetTexture(true);
    }
    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::MetallicRoughTex, Obj.MetallicRoughnessTexture))
    {
        Obj.MetallicRoughnessTexture.SetTexture(true);
    }
    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::NormalTex, Obj.NormalTexture))
    {
        Obj.NormalTexture.SetTexture(true);
    }
    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::OcclusionTex, Obj.OcclusionTexture))
    {
        Obj.OcclusionTexture.SetTexture(true);
    }
    if (Deserializer.SafeDeserializeMember(GLTFMaterialProperties::EmissiveTex, Obj.EmissiveTexture))
    {
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

void GLTFMaterial::SetEmissiveIntensity(float Intensity) { EmissiveIntensity = Intensity; }

float GLTFMaterial::GetEmissiveIntensity() const { return EmissiveIntensity; }

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
    : Material(Name, AssetCollectionId, AssetId, csp::systems::EShaderType::Standard, InitialMaterialVersion)
    , AlphaMode(EAlphaMode::Opaque)
    , AlphaCutoff(0.5f)
    , IsDoubleSided(false)
    , BaseColorFactor(1.f, 1.f, 1.f, 1.f)
    , MetallicFactor(1.f)
    , RoughnessFactor(1.f)
    , EmissiveFactor(0.f, 0.f, 0.f)
    , EmissiveIntensity(1.0f)
    ,
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

void GLTFMaterial::SetDoubleSided(bool DoubleSided) { IsDoubleSided = DoubleSided; }

bool GLTFMaterial::GetDoubleSided() const { return IsDoubleSided; }

} // namespace csp::systems
