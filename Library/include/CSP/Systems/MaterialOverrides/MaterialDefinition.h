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
#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"

namespace csp::systems
{

/// @brief Enumerates the list of material override parameter types.
enum class EMaterialParameterType
{
	Invalid,
	Int,
	Float,
	Vector2,
	Vector3,
	Texture2d
};

/// @brief Enumerates the list of material override parameter types.
enum class EMaterialFeatures
{
	Invalid,
	Secondary_Normal_Map
};

/// @brief The base class for all Material parameters.
class CSP_API MaterialParameterBase
{

public:
	MaterialParameterBase();
	MaterialParameterBase(csp::common::String Name, EMaterialParameterType Type);

	virtual ~MaterialParameterBase() {};

	/// @brief Get the Name of the Material parameter.
	/// @return const csp::common::string& : The Name of the Material parameter.
	csp::common::String GetParameterName() const
	{
		return ParameterName;
	}

	/// @brief Get the Type of the Material parameter.
	/// @return  EMaterialParameterType : The type of the Material parameter.
	EMaterialParameterType GetParameterType() const
	{
		return ParameterType;
	}

private:
	csp::common::String ParameterName;
	EMaterialParameterType ParameterType;
};

/// @brief The int Material parameter.
class CSP_API MaterialParameterInt : public MaterialParameterBase
{

public:
	MaterialParameterInt();
	MaterialParameterInt(csp::common::String Name);
	MaterialParameterInt(csp::common::String Name, int32_t Value);

	/// @brief Get the Value of the Material parameter.
	/// @return int32_t : The Value of the Material parameter.
	int32_t GetParameterValue() const
	{
		return ParameterValue;
	}

	/// @brief Set the Value of the Material parameter.
	/// @param Value int32_t : The Value of the Material parameter.
	void SetParameterValue(const int32_t Value)
	{
		ParameterValue = Value;
	}

private:
	int32_t ParameterValue;
};

/// @brief The float Material parameter.
class CSP_API MaterialParameterFloat : public MaterialParameterBase
{

public:
	MaterialParameterFloat();
	MaterialParameterFloat(csp::common::String Name);
	MaterialParameterFloat(csp::common::String Name, float Value);

	/// @brief Get the Value of the Material parameter.
	/// @return float : The Value of the Material parameter.
	float GetParameterValue() const
	{
		return ParameterValue;
	}

	/// @brief Set the Value of the Material parameter.
	/// @param Value float : The Value of the Material parameter.
	void SetParameterValue(const float Value)
	{
		ParameterValue = Value;
	}

private:
	float ParameterValue;
};

/// @brief The vector2 Material parameter.
class CSP_API MaterialParameterVector2 : public MaterialParameterBase
{

public:
	MaterialParameterVector2();
	MaterialParameterVector2(csp::common::String Name);
	MaterialParameterVector2(csp::common::String Name, csp::common::Vector2& Value);

	/// @brief Get the Value of the Material parameter.
	/// @return csp::common::Vector2& : The Value of the Material parameter.
	const csp::common::Vector2& GetParameterValue() const
	{
		return ParameterValue;
	}

	/// @brief Set the Value of the Material parameter.
	/// @param Value const csp::common::Vector2& : The Value of the Material parameter.
	void SetParameterValue(const csp::common::Vector2& Value)
	{
		ParameterValue = Value;
	}

private:
	csp::common::Vector2 ParameterValue;
};

/// @brief The vector3 Material parameter.
class CSP_API MaterialParameterVector3 : public MaterialParameterBase
{

public:
	MaterialParameterVector3();
	MaterialParameterVector3(csp::common::String Name);
	MaterialParameterVector3(csp::common::String Name, csp::common::Vector3& Value);

	/// @brief Get the Value of the Material parameter.
	/// @return const csp::common::Vector3& : The Value of the Material parameter.
	const csp::common::Vector3& GetParameterValue() const
	{
		return ParameterValue;
	}

	/// @brief Set the Value of the Material parameter.
	/// @param Value const csp::common::Vector3& : The Value of the Material parameter.
	void SetParameterValue(const csp::common::Vector3& Value)
	{
		ParameterValue = Value;
	}

private:
	csp::common::Vector3 ParameterValue;
};

/// @brief The vector3 Material parameter.
class CSP_API MaterialParameterTexture2d : public MaterialParameterBase
{

public:
	MaterialParameterTexture2d();
	MaterialParameterTexture2d(csp::common::String Name);
	MaterialParameterTexture2d(csp::common::String Name, csp::common::String AssetIdValue, csp::common::String AssetCollectionIdValue);

	/// @brief Get the Asset Id of the Material parameter.
	/// @return const csp::common::String& : The Asset Id of the Material parameter.
	const csp::common::String& GetParameterAssetId() const
	{
		return ParameterAssetIdValue;
	}

	/// @brief Set the Asset Id of the Material parameter.
	/// @param Value const csp::common::String& : The Asset Id of the Material parameter.
	void SetParameterAssetId(const csp::common::String& Value)
	{
		ParameterAssetIdValue = Value;
	}

	/// @brief Get the Asset Collection Id of the Material parameter.
	/// @return const csp::common::String& : The Asset Collection Id of the Material parameter.
	const csp::common::String& GetParameterAssetCollectionId() const
	{
		return ParameterAssetCollectionIdValue;
	}

	/// @brief Set the Asset Collection Id of the Material parameter.
	/// @param Value const csp::common::String& : The Asset Collection Id of the Material parameter.
	void SetParameterAssetCollectionId(const csp::common::String& Value)
	{
		ParameterAssetCollectionIdValue = Value;
	}

private:
	csp::common::String ParameterAssetIdValue;
	csp::common::String ParameterAssetCollectionIdValue;
};


class CSP_API MaterialDefinition
{
	/** @cond DO_NOT_DOCUMENT */
	friend class AssetSystem;
	/** @endcond */

public:
	MaterialDefinition();
	MaterialDefinition(const csp::common::String& Name);
	MaterialDefinition(const MaterialDefinition& Other);
	~MaterialDefinition();
	MaterialDefinition& operator=(const MaterialDefinition& Other);

	/// @brief Get the name of the Material.
	/// @return const csp::common::String& : The name of the Material.
	const csp::common::String& GetMaterialName() const
	{
		return MaterialName;
	}

	/// @brief Set the name of the Material.
	/// @param Name const csp::common::String& : The name of the Material.
	void SetMaterialName(const csp::common::String& Name)
	{
		MaterialName = Name;
	}

	/// @brief Get the Material definition version.
	/// @return const csp::common::String& : The definition version of the Material.
	const csp::common::String& GetMaterialDefinitionVersion() const
	{
		return DefinitionVersion;
	}

	/// @brief Get the Material BaseColor parameter.
	/// @return MaterialParameterTexture2d& : The Material BaseColor parameter.
	MaterialParameterTexture2d& GetBaseColor()
	{
		return BaseColor;
	}

	/// @brief Get the Material Normal parameter.
	/// @return MaterialParameterTexture2d& : The Material Normal parameter.
	MaterialParameterTexture2d& GetNormal()
	{
		return Normal;
	}

	/// @brief Get the Material Emissive parameter.
	/// @return MaterialParameterTexture2d& : The Material Emissive parameter.
	MaterialParameterTexture2d& GetEmissive()
	{
		return Emissive;
	}

	/// @brief Get the Material Emissive multiplier parameter.
	/// @return MaterialParameterFloat& : The Material Emissive multiplier parameter.
	MaterialParameterFloat& GetEmissiveMultiplier()
	{
		return EmissiveMultiplier;
	}

	/// @brief Get the Material Opacity parameter.
	/// @return MaterialParameterTexture2d& : The Material Opacity parameter.
	MaterialParameterTexture2d& GetOpacity()
	{
		return Opacity;
	}

	/// @brief Get the Material AO parameter.
	/// @return MaterialParameterTexture2d& : The Material AO parameter.
	MaterialParameterTexture2d& GetAO()
	{
		return AO;
	}

	/// @brief Get the Material UVOffset parameter.
	/// @return MaterialParameterVector2& : The Material UVOffset parameter.
	MaterialParameterVector2& GetUVOffset()
	{
		return UVOffset;
	}

	/// @brief Get the Material UVScale parameter.
	/// @return MaterialParameterVector2& : The Material UVScale parameter.
	MaterialParameterVector2& GetUVScale()
	{
		return UVScale;
	}

	/// @brief Get the Material feature parameters.
	/// @param Feature EMaterialFeatures : The name of the feature.
	/// @return const csp::common::Array<MaterialParameterBase>& : Array of Material feature parameters.
	csp::common::Array<MaterialParameterBase> GetFeatureParameters(EMaterialFeatures Feature) const;

	/// @brief Set the Material parameters for a feature.
	/// @param Feature EMaterialFeatures : The name of the feature.
	/// @param FeatureParameters csp::common::Array<MaterialParameterBase> : The array of feature parameters.
	/// @return const csp::common::Array<MaterialParameterBase>& : Array of Material feature parameters.
	void SetFeatureParameters(EMaterialFeatures Feature, csp::common::Array<MaterialParameterBase> FeatureParameters);

	/// @brief Remove specified Material feature.
	/// @param Feature EMaterialFeatures : The name of the feature.
	void RemoveFeature(EMaterialFeatures Feature);

	/// @brief Get the number of Material features.
	/// @return int32_t : Number of Material features.
	int32_t GetFeatureCount() const;

	/// @brief Verify if a Material feature has been defined.
	/// @param Feature EMaterialFeatures : The name of the feature.
	/// @return bool : Result of check.
	bool IsFeatureDefined(EMaterialFeatures Feature) const;

	/// @brief Serialising the MaterialDefinition object to a json string.
	/// @return csp::common::String : Serialised json string.
	csp::common::String SerialiseToJson() const;

private:
	bool DeserialiseFromJson(const csp::common::String& json);

	csp::common::String MaterialName;
	csp::common::String DefinitionVersion;

	MaterialParameterTexture2d BaseColor;
	MaterialParameterTexture2d Normal;
	MaterialParameterTexture2d Emissive;
	MaterialParameterFloat EmissiveMultiplier;
	MaterialParameterTexture2d Opacity;
	MaterialParameterTexture2d AO;
	MaterialParameterVector2 UVOffset;
	MaterialParameterVector2 UVScale;

	csp::common::Map<EMaterialFeatures, csp::common::Array<MaterialParameterBase>>* MaterialFeatures;
};


} // namespace csp::systems
