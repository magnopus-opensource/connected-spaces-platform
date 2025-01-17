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

#include "Json/JsonSerializer.h"

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj)
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

	// Base Color Factor
	Serializer.SerializeMember(
		"baseColorFactor",
		csp::common::Array<float> {Obj.BaseColorFactor.X, Obj.BaseColorFactor.Y, Obj.BaseColorFactor.Z, Obj.BaseColorFactor.W});

	// Metallic Factor
	Serializer.SerializeMember("metallicFactor", Obj.MetallicFactor);

	// Roughness Factor
	Serializer.SerializeMember("roughnessFactor", Obj.RoughnessFactor);

	// Emissive Factor
	Serializer.SerializeMember("emissiveFactor", csp::common::Array<float> {Obj.EmissiveFactor.X, Obj.EmissiveFactor.Y, Obj.EmissiveFactor.Z});

	// Textures
	if (Obj.BaseColorTexture.IsSet())
	{
		Serializer.SerializeMember("baseColorTexture", Obj.BaseColorTexture);
	}
	if (Obj.MetallicRoughnessTexture.IsSet())
	{
		Serializer.SerializeMember("metallicRoughnessTexture", Obj.MetallicRoughnessTexture);
	}
	if (Obj.NormalTexture.IsSet())
	{
		Serializer.SerializeMember("normalTexture", Obj.NormalTexture);
	}
	if (Obj.OcclusionTexture.IsSet())
	{
		Serializer.SerializeMember("occlusionTexture", Obj.OcclusionTexture);
	}
	if (Obj.EmissiveTexture.IsSet())
	{
		Serializer.SerializeMember("emissiveTexture", Obj.EmissiveTexture);
	}
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj)
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

	// Base Color Factor
	csp::common::Array<float> BaseColorFactorArray;
	Deserializer.DeserializeMember("baseColorFactor", BaseColorFactorArray);

	Obj.BaseColorFactor = csp::common::Vector4(BaseColorFactorArray[0], BaseColorFactorArray[1], BaseColorFactorArray[2], BaseColorFactorArray[3]);

	// Metallic Factor
	Deserializer.DeserializeMember("metallicFactor", Obj.MetallicFactor);

	// Roughness Factor
	Deserializer.DeserializeMember("roughnessFactor", Obj.RoughnessFactor);

	// Emissive Factor
	csp::common::Array<float> EmissiveFactorArray;
	Deserializer.DeserializeMember("emissiveFactor", EmissiveFactorArray);

	Obj.EmissiveFactor = csp::common::Vector3(EmissiveFactorArray[0], EmissiveFactorArray[1], EmissiveFactorArray[2]);

	// Textures
	if (Deserializer.HasProperty("baseColorTexture"))
	{
		Deserializer.DeserializeMember("baseColorTexture", Obj.BaseColorTexture);
		Obj.BaseColorTexture.SetTexture(true);
	}
	if (Deserializer.HasProperty("metallicRoughnessTexture"))
	{
		Deserializer.DeserializeMember("metallicRoughnessTexture", Obj.MetallicRoughnessTexture);
		Obj.MetallicRoughnessTexture.SetTexture(true);
	}
	if (Deserializer.HasProperty("normalTexture"))
	{
		Deserializer.DeserializeMember("normalTexture", Obj.NormalTexture);
		Obj.NormalTexture.SetTexture(true);
	}
	if (Deserializer.HasProperty("occlusionTexture"))
	{
		Deserializer.DeserializeMember("occlusionTexture", Obj.OcclusionTexture);
		Obj.OcclusionTexture.SetTexture(true);
	}
	if (Deserializer.HasProperty("emissiveTexture"))
	{
		Deserializer.DeserializeMember("emissiveTexture", Obj.EmissiveTexture);
		Obj.EmissiveTexture.SetTexture(true);
	}
}

namespace csp::systems
{

void GLTFMaterial::SetBaseColorFactor(const csp::common::Vector4& Factor)
{
	BaseColorFactor = Factor;
}

const csp::common::Vector4& GLTFMaterial::GetBaseColorFactor() const
{
	return BaseColorFactor;
}

void GLTFMaterial::SetMetallicFactor(float Factor)
{
	MetallicFactor = Factor;
}

float GLTFMaterial::GetMetallicFactor() const
{
	return MetallicFactor;
}

void GLTFMaterial::SetRoughnessFactor(float Factor)
{
	RoughnessFactor = Factor;
}

float GLTFMaterial::GetRoughnessFactor() const
{
	return RoughnessFactor;
}

void GLTFMaterial::SetEmissiveFactor(const csp::common::Vector3& Factor)
{
	EmissiveFactor = Factor;
}

const csp::common::Vector3& GLTFMaterial::GetEmissiveFactor() const
{
	return EmissiveFactor;
}

void GLTFMaterial::SetBaseColorTexture(const TextureInfo& Texture)
{
	BaseColorTexture = Texture;
}

const TextureInfo& GLTFMaterial::GetBaseColorTexture() const
{
	return BaseColorTexture;
}

void GLTFMaterial::SetMetallicRoughnessTexture(const TextureInfo& Texture)
{
	MetallicRoughnessTexture = Texture;
}

const TextureInfo& GLTFMaterial::GetMetallicRoughnessTexture() const
{
	return MetallicRoughnessTexture;
}

void GLTFMaterial::SetNormalTexture(const TextureInfo& Texture)
{
	NormalTexture = Texture;
}

void GLTFMaterial::SetOcclusionTexture(const TextureInfo& Texture)
{
	OcclusionTexture = Texture;
}

const TextureInfo& GLTFMaterial::GetOcclusionTexture() const
{
	return OcclusionTexture;
}

const TextureInfo& GLTFMaterial::GetNormalTexture() const
{
	return NormalTexture;
}

void GLTFMaterial::SetEmissiveTexture(const TextureInfo& Texture)
{
	EmissiveTexture = Texture;
}

const TextureInfo& GLTFMaterial::GetEmissiveTexture() const
{
	return EmissiveTexture;
}

GLTFMaterial::GLTFMaterial(const csp::common::String& Name, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
	: Material(Name, AssetCollectionId, AssetId)
	, BaseColorFactor(1.f, 1.f, 1.f, 1.f)
	, MetallicFactor(1.f)
	, RoughnessFactor(1.f)
	, EmissiveFactor(0.f, 0.f, 0.f)
	, BaseColorTexture()
	, MetallicRoughnessTexture()
	, NormalTexture()
	, OcclusionTexture()
	, EmissiveTexture()
	, AlphaMode(EAlphaMode::Opaque)
	, AlphaCutoff(0.5f)
	, DoubleSided(false)
	, Version(1)
{
	BaseColorTexture.SetTexture(false);
	MetallicRoughnessTexture.SetTexture(false);
	NormalTexture.SetTexture(false);
	OcclusionTexture.SetTexture(false);
	EmissiveTexture.SetTexture(false);
}

GLTFMaterial::GLTFMaterial() : GLTFMaterial("", "", "")
{
}

void GLTFMaterial::SetAlphaMode(EAlphaMode Mode)
{
	AlphaMode = Mode;
}

EAlphaMode GLTFMaterial::GetAlphaMode() const
{
	return AlphaMode;
}

void GLTFMaterial::SetAlphaCutoff(float Mode)
{
	AlphaCutoff = Mode;
}

float GLTFMaterial::GetAlphaCutoff() const
{
	return AlphaCutoff;
}

void GLTFMaterial::SetDoubleSided(bool InDoubleSided)
{
	DoubleSided = InDoubleSided;
}

bool GLTFMaterial::GetDoubleSided() const
{
	return DoubleSided;
}

const GLTFMaterial& GLTFMaterialResult::GetGLTFMaterial() const
{
	return Material;
}

void GLTFMaterialResult::SetGLTFMaterial(const GLTFMaterial& InMaterial)
{
	Material = InMaterial;
}

void GLTFMaterialResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
}

const csp::common::Array<GLTFMaterial>& GLTFMaterialsResult::GetGLTFMaterials() const
{
	return Materials;
}

void GLTFMaterialsResult::SetGLTFMaterials(const csp::common::Array<GLTFMaterial>& InMaterials)
{
	Materials = InMaterials;
}

void GLTFMaterialsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
}

} // namespace csp::systems
