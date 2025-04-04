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

#include "CSP/Systems/Assets/AlphaVideoMaterial.h"

#include "CSP/Common/Array.h"

#include "Debug/Logging.h"
#include "Json/JsonSerializer.h"

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

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::AlphaVideoMaterial& Obj)
{
    Serializer.SerializeMember(AlphaVideoMaterialProperties::Name, Obj.Name);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::ShaderType, static_cast<uint32_t>(Obj.Type));
    Serializer.SerializeMember(AlphaVideoMaterialProperties::Version, Obj.Version);

    if (Obj.ColorTexture.IsSet())
    {
        Serializer.SerializeMember(AlphaVideoMaterialProperties::ColorTexture, Obj.ColorTexture);
    }

    Serializer.SerializeMember(AlphaVideoMaterialProperties::DoubleSided, Obj.DoubleSided);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::IsEmissive, Obj.IsEmissive);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::ReadAlphaFromChannel, static_cast<uint32_t>(Obj.ReadAlphaFromChannel));
    Serializer.SerializeMember(AlphaVideoMaterialProperties::BlendMode, static_cast<uint32_t>(Obj.BlendMode));
    Serializer.SerializeMember(AlphaVideoMaterialProperties::FresnelFactor, Obj.FresnelFactor);
    Serializer.SerializeMember(
        AlphaVideoMaterialProperties::Tint, csp::common::Array<float> { Obj.MaterialTint.X, Obj.MaterialTint.Y, Obj.MaterialTint.Z });
    Serializer.SerializeMember(AlphaVideoMaterialProperties::EmissiveIntensity, Obj.EmissiveIntensity);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::AlphaFactor, Obj.AlphaFactor);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::AlphaMask, Obj.AlphaMask);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AlphaVideoMaterial& Obj)
{
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::Name, Obj.Name);

    uint32_t ShaderType;
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::ShaderType, ShaderType);
    Obj.Type = static_cast<csp::systems::EShaderType>(ShaderType);

    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::Version, Obj.Version);

    if (Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::ColorTexture, Obj.ColorTexture))
    {
        Obj.ColorTexture.SetTexture(true);
    }

    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::DoubleSided, Obj.DoubleSided);
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::IsEmissive, Obj.IsEmissive);

    uint32_t ReadAlphaFromChannel;
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::ReadAlphaFromChannel, ReadAlphaFromChannel);
    Obj.ReadAlphaFromChannel = static_cast<csp::systems::EColorChannel>(ReadAlphaFromChannel);

    uint32_t BlendMode;
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::BlendMode, BlendMode);
    Obj.BlendMode = static_cast<csp::systems::EBlendMode>(BlendMode);

    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::FresnelFactor, Obj.FresnelFactor);

    csp::common::Array<float> TintArray;
    if (Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::Tint, TintArray))
    {
        Obj.MaterialTint = csp::common::Vector3(TintArray[0], TintArray[1], TintArray[2]);
    }

    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::EmissiveIntensity, Obj.EmissiveIntensity);
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::AlphaFactor, Obj.AlphaFactor);
    Deserializer.SafeDeserializeMember(AlphaVideoMaterialProperties::AlphaMask, Obj.AlphaMask);
}

namespace csp::systems
{

AlphaVideoMaterial::AlphaVideoMaterial(
    const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
    : Material(Name, AssetCollectionId, AssetId, EShaderType::AlphaVideo, 1)
    , ColorTexture()
    , DoubleSided(false)
    , IsEmissive(true)
    , ReadAlphaFromChannel(EColorChannel::A)
    , BlendMode(EBlendMode::Normal)
    , FresnelFactor(0.0f)
    , MaterialTint(1.0f, 1.0f, 1.0f)
    , AlphaFactor(1.0f)
    , EmissiveIntensity(1.0f)
    , AlphaMask(0.02f)
{
    ColorTexture.SetTexture(false);
}

AlphaVideoMaterial::AlphaVideoMaterial()
    : AlphaVideoMaterial("", "", "")
{
}

void AlphaVideoMaterial::SetColorTexture(const TextureInfo& Texture) { ColorTexture = Texture; }

const TextureInfo& AlphaVideoMaterial::GetColorTexture() const { return ColorTexture; }

void AlphaVideoMaterial::SetDoubleSided(bool DoubleSided) { DoubleSided = DoubleSided; }

bool AlphaVideoMaterial::GetDoubleSided() const { return DoubleSided; }

void AlphaVideoMaterial::SetIsEmissive(bool IsEmissive) { IsEmissive = IsEmissive; }

bool AlphaVideoMaterial::GetIsEmissive() const { return IsEmissive; }

void AlphaVideoMaterial::SetReadAlphaFromChannel(EColorChannel ColorChannel) { ReadAlphaFromChannel = ColorChannel; }

EColorChannel AlphaVideoMaterial::GetReadAlphaFromChannel() const { return ReadAlphaFromChannel; }

void AlphaVideoMaterial::SetBlendMode(EBlendMode Mode) { BlendMode = Mode; }

EBlendMode AlphaVideoMaterial::GetBlendMode() const { return BlendMode; }

void AlphaVideoMaterial::SetFresnelFactor(float Factor) { FresnelFactor = Factor; }

float AlphaVideoMaterial::GetFresnelFactor() const { return FresnelFactor; }

void AlphaVideoMaterial::SetTint(const csp::common::Vector3& Tint) { MaterialTint = Tint; }

const csp::common::Vector3& AlphaVideoMaterial::GetTint() const { return MaterialTint; }

void AlphaVideoMaterial::SetAlphaFactor(float Factor) { AlphaFactor = Factor; }

float AlphaVideoMaterial::GetAlphaFactor() const { return AlphaFactor; }

void AlphaVideoMaterial::SetEmissiveIntensity(float Intensity) { EmissiveIntensity = Intensity; }

float AlphaVideoMaterial::GetEmissiveIntensity() const { return EmissiveIntensity; }

void AlphaVideoMaterial::SetAlphaMask(float Mask) { AlphaMask = Mask; }

float AlphaVideoMaterial::GetAlphaMask() const { return AlphaMask; }

} // namespace csp::systems
