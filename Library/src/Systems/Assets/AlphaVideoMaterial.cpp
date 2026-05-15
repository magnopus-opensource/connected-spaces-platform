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

#include "CSP/Systems/Assets/AlphaVideoMaterial.h"

#include "CSP/Common/Array.h"

#include "Debug/Logging.h"
#include "Json/JsonSerializer.h"

namespace
{
constexpr int InitialMaterialVersion = 1;
}

namespace AlphaVideoMaterialProperties
{
static constexpr const char* Name = "name";
static constexpr const char* ShaderType = "shaderType";
static constexpr const char* Version = "version";
static constexpr const char* ColorTexture = "colorTexture";
static constexpr const char* DoubleSided = "doubleSided";
static constexpr const char* IsEmissive = "isEmissive";
static constexpr const char* ReadAlphaFromChannel = "readAlphaFromChannel";
static constexpr const char* BlendMode = "blendMode";
static constexpr const char* FresnelFactor = "fresnelFactor";
static constexpr const char* Tint = "tint";
static constexpr const char* EmissiveIntensity = "emissiveIntensity";
static constexpr const char* AlphaFactor = "alphaFactor";
static constexpr const char* AlphaMask = "alphaMask";
}

void ToJson(csp::json::JsonSerializer& serializer, const csp::systems::AlphaVideoMaterial& obj)
{
    serializer.SerializeMember(AlphaVideoMaterialProperties::Name, obj.m_name);
    serializer.SerializeMember(AlphaVideoMaterialProperties::ShaderType, static_cast<uint32_t>(obj.m_type));
    serializer.SerializeMember(AlphaVideoMaterialProperties::Version, obj.m_version);

    if (obj.m_colorTexture.IsSet())
    {
        serializer.SerializeMember(AlphaVideoMaterialProperties::ColorTexture, obj.m_colorTexture);
    }

    serializer.SerializeMember(AlphaVideoMaterialProperties::DoubleSided, obj.m_isDoubleSided);
    serializer.SerializeMember(AlphaVideoMaterialProperties::IsEmissive, obj.m_isEmissive);
    serializer.SerializeMember(AlphaVideoMaterialProperties::ReadAlphaFromChannel, static_cast<uint32_t>(obj.m_readAlphaFromChannel));
    serializer.SerializeMember(AlphaVideoMaterialProperties::BlendMode, static_cast<uint32_t>(obj.m_blendMode));
    serializer.SerializeMember(AlphaVideoMaterialProperties::FresnelFactor, obj.m_fresnelFactor);
    serializer.SerializeMember(
        AlphaVideoMaterialProperties::Tint, csp::common::Array<float> { obj.m_materialTint.X, obj.m_materialTint.Y, obj.m_materialTint.Z });
    serializer.SerializeMember(AlphaVideoMaterialProperties::EmissiveIntensity, obj.m_emissiveIntensity);
    serializer.SerializeMember(AlphaVideoMaterialProperties::AlphaFactor, obj.m_alphaFactor);
    serializer.SerializeMember(AlphaVideoMaterialProperties::AlphaMask, obj.m_alphaMask);
}

void FromJson(const csp::json::JsonDeserializer& deserializer, csp::systems::AlphaVideoMaterial& obj)
{
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::Name, obj.m_name);

    uint32_t shaderType;
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::ShaderType, shaderType);
    obj.m_type = static_cast<csp::systems::EShaderType>(shaderType);

    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::Version, obj.m_version);

    if (deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::ColorTexture, obj.m_colorTexture))
    {
        obj.m_colorTexture.SetTexture(true);
    }

    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::DoubleSided, obj.m_isDoubleSided);
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::IsEmissive, obj.m_isEmissive);

    uint32_t readAlphaFromChannel;
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::ReadAlphaFromChannel, readAlphaFromChannel);
    obj.m_readAlphaFromChannel = static_cast<csp::systems::EColorChannel>(readAlphaFromChannel);

    uint32_t blendMode;
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::BlendMode, blendMode);
    obj.m_blendMode = static_cast<csp::systems::EBlendMode>(blendMode);

    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::FresnelFactor, obj.m_fresnelFactor);

    csp::common::Array<float> tintArray;
    if (deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::Tint, tintArray))
    {
        obj.m_materialTint = csp::common::Vector3(tintArray[0], tintArray[1], tintArray[2]);
    }

    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::EmissiveIntensity, obj.m_emissiveIntensity);
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::AlphaFactor, obj.m_alphaFactor);
    deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::AlphaMask, obj.m_alphaMask);
}

namespace csp::systems
{

AlphaVideoMaterial::AlphaVideoMaterial(
    const csp::common::String& name, const csp::common::String& assetCollectionId, const csp::common::String& assetId)
    : Material(name, assetCollectionId, assetId, EShaderType::AlphaVideo, InitialMaterialVersion)
    , m_colorTexture()
    , m_isDoubleSided(false)
    , m_isEmissive(true)
    , m_readAlphaFromChannel(EColorChannel::A)
    , m_blendMode(EBlendMode::Normal)
    , m_fresnelFactor(0.0f)
    , m_materialTint(1.0f, 1.0f, 1.0f)
    , m_alphaFactor(1.0f)
    , m_emissiveIntensity(1.0f)
    , m_alphaMask(0.02f)
{
    m_colorTexture.SetTexture(false);
}

AlphaVideoMaterial::AlphaVideoMaterial()
    : AlphaVideoMaterial("", "", "")
{
}

void AlphaVideoMaterial::SetColorTexture(const TextureInfo& texture) { m_colorTexture = texture; }

const TextureInfo& AlphaVideoMaterial::GetColorTexture() const { return m_colorTexture; }

void AlphaVideoMaterial::SetDoubleSided(bool doubleSided) { m_isDoubleSided = doubleSided; }

bool AlphaVideoMaterial::GetDoubleSided() const { return m_isDoubleSided; }

void AlphaVideoMaterial::SetIsEmissive(bool emissive) { m_isEmissive = emissive; }

bool AlphaVideoMaterial::GetIsEmissive() const { return m_isEmissive; }

void AlphaVideoMaterial::SetReadAlphaFromChannel(EColorChannel colorChannel) { m_readAlphaFromChannel = colorChannel; }

EColorChannel AlphaVideoMaterial::GetReadAlphaFromChannel() const { return m_readAlphaFromChannel; }

void AlphaVideoMaterial::SetBlendMode(EBlendMode mode) { m_blendMode = mode; }

EBlendMode AlphaVideoMaterial::GetBlendMode() const { return m_blendMode; }

void AlphaVideoMaterial::SetFresnelFactor(float factor) { m_fresnelFactor = factor; }

float AlphaVideoMaterial::GetFresnelFactor() const { return m_fresnelFactor; }

void AlphaVideoMaterial::SetTint(const csp::common::Vector3& tint) { m_materialTint = tint; }

const csp::common::Vector3& AlphaVideoMaterial::GetTint() const { return m_materialTint; }

void AlphaVideoMaterial::SetAlphaFactor(float factor) { m_alphaFactor = factor; }

float AlphaVideoMaterial::GetAlphaFactor() const { return m_alphaFactor; }

void AlphaVideoMaterial::SetEmissiveIntensity(float intensity) { m_emissiveIntensity = intensity; }

float AlphaVideoMaterial::GetEmissiveIntensity() const { return m_emissiveIntensity; }

void AlphaVideoMaterial::SetAlphaMask(float mask) { m_alphaMask = mask; }

float AlphaVideoMaterial::GetAlphaMask() const { return m_alphaMask; }

} // namespace csp::systems
