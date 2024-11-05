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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/Assets/TextureInfo.h"

namespace csp::systems
{
class GLTFMaterial;
}

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj);

namespace csp::systems
{

/// @brief Enum representing the shader type of a material. Currently not in use.
enum class EShaderType
{
	Standard
};

/// @brief The alpha mode of a material.
enum class EAlphaMode
{
	Opaque,
	Mask,
	Blend
};

/// @ingroup Asset System
/// @brief Data class which represents a GLTF material.
class CSP_API GLTFMaterial
{
public:
	/// @brief Sets the name of the material
	/// @param ComponentId const csp::common::String&
	void SetName(const csp::common::String& Name);

	/// @brief Gets the name of the material
	/// @return csp::common::String&
	const csp::common::String& GetName() const;

	/// TODO: comment
	void SetAlphaMode(EAlphaMode Mode);
	/// TODO: comment
	EAlphaMode GetAlphaMode() const;

	/// TODO: comment
	void SetAlphaCutoff(float Mode);
	/// TODO: comment
	float GetAlphaCutoff() const;

	/// TODO: comment
	void SetDoubleSided(bool DoubleSided);
	/// TODO: comment
	bool GetDoubleSided() const;

	/// @brief Sets the factor of the base color texture
	/// @param Factor const csp::common::Vector4&
	void SetBaseColorFactor(const csp::common::Vector4& Factor);

	/// @brief Gets the factor of the base color texture
	/// @return csp::common::Vector4&
	const csp::common::Vector4& GetBaseColorFactor() const;

	/// @brief Sets the factor of the base metallic texture
	/// @param Factor float
	void SetMetallicFactor(float Factor);

	/// @brief Gets the factor of the metallic texture
	/// @return float
	float GetMetallicFactor() const;

	/// @brief Sets the factor of the base roughness texture
	/// @param Factor float
	void SetRoughnessFactor(float Factor);

	/// @brief Gets the factor of the roughness texture
	/// @return float
	float GetRoughnessFactor() const;

	/// @brief Sets the factor of the emissive texture
	/// @param Factor const csp::common::Vector3&
	void SetEmissiveFactor(const csp::common::Vector3& Factor);

	/// @brief Gets the factor of the emissive color texture
	/// @return csp::common::Vector3&
	const csp::common::Vector3& GetEmissiveFactor() const;

	/// @brief Sets the base color texture
	/// @param Texture const TextureInfo&
	void SetBaseColorTexture(const TextureInfo& Texture);

	/// @brief Gets the base color texture
	/// @return const TextureInfo&
	const TextureInfo& GetBaseColorTexture() const;

	/// @brief Sets the metallic roughness texture
	/// @param Texture const TextureInfo&
	void SetMetallicRoughnessTexture(const TextureInfo& Texture);

	/// @brief Gets the metallic roughness texture
	/// @return const TextureInfo&
	const TextureInfo& GetMetallicRoughnessTexture() const;

	/// @brief Sets the normal texture
	/// @param Texture const TextureInfo&
	void SetNormalTexture(const TextureInfo& Texture);

	/// @brief Gets the normal texture
	/// @return const TextureInfo&
	const TextureInfo& GetNormalTexture() const;

	/// @brief Sets the occlusion texture
	/// @param Texture const TextureInfo&
	void SetOcclusionTexture(const TextureInfo& Texture);

	/// @brief Gets the occlusion texture
	/// @return const TextureInfo&
	const TextureInfo& GetOcclusionTexture() const;

	/// @brief Sets the emissive texture
	/// @param Texture const TextureInfo&
	void SetEmissiveTexture(const TextureInfo& Texture);

	/// @brief Gets the emissive texture
	/// @return const TextureInfo&
	const TextureInfo& GetEmissiveTexture() const;

	/// @brief Gets the asset collection id for where this material is stored
	/// @return const csp::common::String&
	const csp::common::String& GetAssetCollectionId() const;

	/// @brief Gets the asset id for where this material is stored
	/// @return const csp::common::String&
	const csp::common::String& GetAssetId() const;

	/// @brief Constructor which links the material to an asset
	/// @param AssetCollectionId const csp::common::String& : The asset collection where the material info is stored
	/// @param AssetId const csp::common::String& : The asset where the material info is stored
	GLTFMaterial(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

private:
	csp::common::String Name;
	EShaderType Type;

	int Version;

	EAlphaMode AlphaMode;
	float AlphaCutoff;
	bool DoubleSided;

	csp::common::Vector4 BaseColorFactor;
	float MetallicFactor;
	float RoughnessFactor;
	csp::common::Vector3 EmissiveFactor;

	TextureInfo BaseColorTexture;
	TextureInfo MetallicRoughnessTexture;
	TextureInfo NormalTexture;
	TextureInfo OcclusionTexture;
	TextureInfo EmissiveTexture;

	csp::common::String AssetCollectionId;
	csp::common::String AssetId;

	friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::GLTFMaterial& Obj);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::GLTFMaterial& Obj);
};

} // namespace csp::systems
