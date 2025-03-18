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

#include "Json/JsonSerializer.h"
#include "Debug/Logging.h"

struct AlphaVideoMaterialProperties
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
};

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
    Serializer.SerializeMember(AlphaVideoMaterialProperties::Tint, 
        csp::common::Array<float> { Obj.Tint.X, Obj.Tint.Y, Obj.Tint.Z });
    Serializer.SerializeMember(AlphaVideoMaterialProperties::EmissiveIntensity, Obj.EmissiveIntensity);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::AlphaFactor, Obj.AlphaFactor);
    Serializer.SerializeMember(AlphaVideoMaterialProperties::AlphaMask, Obj.AlphaMask);
}

// TODO review why/if this is needed, without it a missing property causes a crash
namespace
{
    template<typename T>
    bool SafeDeserialize(const csp::json::JsonDeserializer& Deserializer, const char* PropertyName, T& OutValue)
    {
        if (Deserializer.HasProperty(PropertyName))
        {
            Deserializer.DeserializeMember(PropertyName, OutValue);
            return true;
        }
        return false;
    }

    template<typename EnumType>
    bool SafeDeserializeEnum(const csp::json::JsonDeserializer& Deserializer, const char* PropertyName, EnumType& OutValue)
    {
        if (Deserializer.HasProperty(PropertyName))
        {
            uint32_t RawValue;
            Deserializer.DeserializeMember(PropertyName, RawValue);
            OutValue = static_cast<EnumType>(RawValue);
            return true;
        }
        return false;
    }
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::AlphaVideoMaterial& Obj)
{
    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::Name, Obj.Name);
    SafeDeserializeEnum(Deserializer, AlphaVideoMaterialProperties::ShaderType, Obj.Type);
    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::Version, Obj.Version);

    if (SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::ColorTexture, Obj.ColorTexture))
    {
        Obj.ColorTexture.SetTexture(true);
    }

    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::DoubleSided, Obj.DoubleSided);
    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::IsEmissive, Obj.IsEmissive);
    SafeDeserializeEnum(Deserializer, AlphaVideoMaterialProperties::ReadAlphaFromChannel, Obj.ReadAlphaFromChannel);
    SafeDeserializeEnum(Deserializer, AlphaVideoMaterialProperties::BlendMode, Obj.BlendMode);
    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::FresnelFactor, Obj.FresnelFactor);

    csp::common::Array<float> TintArray;
    if (SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::Tint, TintArray))
    {
        Obj.Tint = csp::common::Vector3(TintArray[0], TintArray[1], TintArray[2]);
    }

    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::EmissiveIntensity, Obj.EmissiveIntensity);
    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::AlphaFactor, Obj.AlphaFactor);
    SafeDeserialize(Deserializer, AlphaVideoMaterialProperties::AlphaMask, Obj.AlphaMask);
}

namespace csp::systems
{

AlphaVideoMaterial::AlphaVideoMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
    : Material(Name, AssetCollectionId, AssetId, EShaderType::AlphaVideo, 1)
    , ColorTexture()
    , DoubleSided(false)
    , IsEmissive(true)
    , ReadAlphaFromChannel(EColorChannel::A)
    , BlendMode(EBlendMode::Normal)
    , FresnelFactor(0.0f)
    , Tint(1.0f, 1.0f, 1.0f)
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

void AlphaVideoMaterial::SetDoubleSided(bool InDoubleSided) { DoubleSided = InDoubleSided; }

bool AlphaVideoMaterial::GetDoubleSided() const { return DoubleSided; }

void AlphaVideoMaterial::SetIsEmissive(bool InIsEmissive) { IsEmissive = InIsEmissive; }

bool AlphaVideoMaterial::GetIsEmissive() const { return IsEmissive; }

void AlphaVideoMaterial::SetReadAlphaFromChannel(EColorChannel InReadSetReadAlphaFromChannel) { ReadAlphaFromChannel = InReadSetReadAlphaFromChannel; }

EColorChannel AlphaVideoMaterial::GetReadAlphaFromChannel() const { return ReadAlphaFromChannel; }

void AlphaVideoMaterial::SetBlendMode(EBlendMode Mode) { BlendMode = Mode; }

EBlendMode AlphaVideoMaterial::GetBlendMode() const { return BlendMode; }

void AlphaVideoMaterial::SetFresnelFactor(float factor) { FresnelFactor = factor; }

float AlphaVideoMaterial::GetFresnelFactor() const { return FresnelFactor; }

void AlphaVideoMaterial::SetTint(const csp::common::Vector3& factor) { Tint = factor; }

const csp::common::Vector3& AlphaVideoMaterial::GetTint() const { return Tint; }

void AlphaVideoMaterial::SetAlphaFactor(float factor) { AlphaFactor = factor; }

float AlphaVideoMaterial::GetAlphaFactor() const { return AlphaFactor; }

void AlphaVideoMaterial::SetEmissiveIntensity(float factor) { EmissiveIntensity = factor; }

float AlphaVideoMaterial::GetEmissiveIntensity() const { return EmissiveIntensity; }

void AlphaVideoMaterial::SetAlphaMask(float mask) { AlphaMask = mask; }

float AlphaVideoMaterial::GetAlphaMask() const { return AlphaMask; }



} // namespace csp::systems
