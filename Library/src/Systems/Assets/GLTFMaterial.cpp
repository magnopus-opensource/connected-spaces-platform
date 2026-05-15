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
constexpr int InitialMaterialVersion = 2;
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
static constexpr const char* EmissiveStrength = "emissiveStrength";
static constexpr const char* BaseColorTex = "baseColorTexture";
static constexpr const char* MetallicRoughTex = "metallicRoughnessTexture";
static constexpr const char* NormalTex = "normalTexture";
static constexpr const char* OcclusionTex = "occlusionTexture";
static constexpr const char* EmissiveTex = "emissiveTexture";
}

void ToJson(csp::json::JsonSerializer& serializer, const csp::systems::GLTFMaterial& obj)
{
    serializer.SerializeMember(GLTFMaterialProperties::Name, obj.m_name);
    serializer.SerializeMember(GLTFMaterialProperties::ShaderType, static_cast<uint32_t>(obj.m_type));
    serializer.SerializeMember(GLTFMaterialProperties::Version, obj.m_version);
    serializer.SerializeMember(GLTFMaterialProperties::AlphaMode, static_cast<uint32_t>(obj.m_alphaMode));
    serializer.SerializeMember(GLTFMaterialProperties::AlphaCutoff, obj.m_alphaCutoff);
    serializer.SerializeMember(GLTFMaterialProperties::DoubleSided, obj.m_isDoubleSided);
    serializer.SerializeMember(GLTFMaterialProperties::BaseColorFactor,
        csp::common::Array<float> { obj.m_baseColorFactor.X, obj.m_baseColorFactor.Y, obj.m_baseColorFactor.Z, obj.m_baseColorFactor.W });
    serializer.SerializeMember(GLTFMaterialProperties::MetallicFactor, obj.m_metallicFactor);
    serializer.SerializeMember(GLTFMaterialProperties::RoughnessFactor, obj.m_roughnessFactor);
    serializer.SerializeMember(
        GLTFMaterialProperties::EmissiveFactor, csp::common::Array<float> { obj.m_emissiveFactor.X, obj.m_emissiveFactor.Y, obj.m_emissiveFactor.Z });
    serializer.SerializeMember(GLTFMaterialProperties::EmissiveStrength, obj.m_emissiveStrength);

    if (obj.m_baseColorTexture.IsSet())
        serializer.SerializeMember(GLTFMaterialProperties::BaseColorTex, obj.m_baseColorTexture);
    if (obj.m_metallicRoughnessTexture.IsSet())
        serializer.SerializeMember(GLTFMaterialProperties::MetallicRoughTex, obj.m_metallicRoughnessTexture);
    if (obj.m_normalTexture.IsSet())
        serializer.SerializeMember(GLTFMaterialProperties::NormalTex, obj.m_normalTexture);
    if (obj.m_occlusionTexture.IsSet())
        serializer.SerializeMember(GLTFMaterialProperties::OcclusionTex, obj.m_occlusionTexture);
    if (obj.m_emissiveTexture.IsSet())
        serializer.SerializeMember(GLTFMaterialProperties::EmissiveTex, obj.m_emissiveTexture);
}

void FromJson(const csp::json::JsonDeserializer& deserializer, csp::systems::GLTFMaterial& obj)
{
    deserializer.SafeDeserializeMember(GLTFMaterialProperties::Name, obj.m_name);

    uint32_t shaderType;
    deserializer.SafeDeserializeMember(GLTFMaterialProperties::ShaderType, shaderType);
    obj.m_type = static_cast<csp::systems::EShaderType>(shaderType);

    deserializer.SafeDeserializeMember(GLTFMaterialProperties::Version, obj.m_version);

    uint32_t alphaMode;
    deserializer.SafeDeserializeMember(GLTFMaterialProperties::AlphaMode, alphaMode);
    obj.m_alphaMode = static_cast<csp::systems::EAlphaMode>(alphaMode);

    deserializer.SafeDeserializeMember(GLTFMaterialProperties::AlphaCutoff, obj.m_alphaCutoff);
    deserializer.SafeDeserializeMember(GLTFMaterialProperties::DoubleSided, obj.m_isDoubleSided);

    csp::common::Array<float> baseColorFactorArray;
    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::BaseColorFactor, baseColorFactorArray))
    {
        obj.m_baseColorFactor
            = csp::common::Vector4(baseColorFactorArray[0], baseColorFactorArray[1], baseColorFactorArray[2], baseColorFactorArray[3]);
    }

    deserializer.SafeDeserializeMember(GLTFMaterialProperties::MetallicFactor, obj.m_metallicFactor);
    deserializer.SafeDeserializeMember(GLTFMaterialProperties::RoughnessFactor, obj.m_roughnessFactor);

    csp::common::Array<float> emissiveFactorArray;
    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::EmissiveFactor, emissiveFactorArray))
    {
        obj.m_emissiveFactor = csp::common::Vector3(emissiveFactorArray[0], emissiveFactorArray[1], emissiveFactorArray[2]);
    }

    deserializer.SafeDeserializeMember(GLTFMaterialProperties::EmissiveStrength, obj.m_emissiveStrength);

    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::BaseColorTex, obj.m_baseColorTexture))
    {
        obj.m_baseColorTexture.SetTexture(true);
    }
    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::MetallicRoughTex, obj.m_metallicRoughnessTexture))
    {
        obj.m_metallicRoughnessTexture.SetTexture(true);
    }
    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::NormalTex, obj.m_normalTexture))
    {
        obj.m_normalTexture.SetTexture(true);
    }
    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::OcclusionTex, obj.m_occlusionTexture))
    {
        obj.m_occlusionTexture.SetTexture(true);
    }
    if (deserializer.SafeDeserializeMember(GLTFMaterialProperties::EmissiveTex, obj.m_emissiveTexture))
    {
        obj.m_emissiveTexture.SetTexture(true);
    }
}

namespace csp::systems
{

void GLTFMaterial::SetBaseColorFactor(const csp::common::Vector4& factor) { m_baseColorFactor = factor; }

const csp::common::Vector4& GLTFMaterial::GetBaseColorFactor() const { return m_baseColorFactor; }

void GLTFMaterial::SetMetallicFactor(float factor) { m_metallicFactor = factor; }

float GLTFMaterial::GetMetallicFactor() const { return m_metallicFactor; }

void GLTFMaterial::SetRoughnessFactor(float factor) { m_roughnessFactor = factor; }

float GLTFMaterial::GetRoughnessFactor() const { return m_roughnessFactor; }

void GLTFMaterial::SetEmissiveFactor(const csp::common::Vector3& factor) { m_emissiveFactor = factor; }

const csp::common::Vector3& GLTFMaterial::GetEmissiveFactor() const { return m_emissiveFactor; }

void GLTFMaterial::SetEmissiveStrength(float strength) { m_emissiveStrength = strength; }

float GLTFMaterial::GetEmissiveStrength() const { return m_emissiveStrength; }

void GLTFMaterial::SetBaseColorTexture(const TextureInfo& texture) { m_baseColorTexture = texture; }

const TextureInfo& GLTFMaterial::GetBaseColorTexture() const { return m_baseColorTexture; }

void GLTFMaterial::SetMetallicRoughnessTexture(const TextureInfo& texture) { m_metallicRoughnessTexture = texture; }

const TextureInfo& GLTFMaterial::GetMetallicRoughnessTexture() const { return m_metallicRoughnessTexture; }

void GLTFMaterial::SetNormalTexture(const TextureInfo& texture) { m_normalTexture = texture; }

void GLTFMaterial::SetOcclusionTexture(const TextureInfo& texture) { m_occlusionTexture = texture; }

const TextureInfo& GLTFMaterial::GetOcclusionTexture() const { return m_occlusionTexture; }

const TextureInfo& GLTFMaterial::GetNormalTexture() const { return m_normalTexture; }

void GLTFMaterial::SetEmissiveTexture(const TextureInfo& texture) { m_emissiveTexture = texture; }

const TextureInfo& GLTFMaterial::GetEmissiveTexture() const { return m_emissiveTexture; }

GLTFMaterial::GLTFMaterial(const csp::common::String& name, const csp::common::String& assetCollectionId, const csp::common::String& assetId)
    : Material(name, assetCollectionId, assetId, csp::systems::EShaderType::Standard, InitialMaterialVersion)
    , m_alphaMode(EAlphaMode::Opaque)
    , m_alphaCutoff(0.5f)
    , m_isDoubleSided(false)
    , m_baseColorFactor(1.f, 1.f, 1.f, 1.f)
    , m_metallicFactor(1.f)
    , m_roughnessFactor(1.f)
    , m_emissiveFactor(0.f, 0.f, 0.f)
    , m_emissiveStrength(1.0f)
    , m_baseColorTexture()
    , m_metallicRoughnessTexture()
    , m_normalTexture()
    , m_occlusionTexture()
    , m_emissiveTexture()
{
    m_baseColorTexture.SetTexture(false);
    m_metallicRoughnessTexture.SetTexture(false);
    m_normalTexture.SetTexture(false);
    m_occlusionTexture.SetTexture(false);
    m_emissiveTexture.SetTexture(false);
}

GLTFMaterial::GLTFMaterial()
    : GLTFMaterial("", "", "")
{
}

void GLTFMaterial::SetAlphaMode(EAlphaMode mode) { m_alphaMode = mode; }

EAlphaMode GLTFMaterial::GetAlphaMode() const { return m_alphaMode; }

void GLTFMaterial::SetAlphaCutoff(float mode) { m_alphaCutoff = mode; }

float GLTFMaterial::GetAlphaCutoff() const { return m_alphaCutoff; }

void GLTFMaterial::SetDoubleSided(bool doubleSided) { m_isDoubleSided = doubleSided; }

bool GLTFMaterial::GetDoubleSided() const { return m_isDoubleSided; }

} // namespace csp::systems
