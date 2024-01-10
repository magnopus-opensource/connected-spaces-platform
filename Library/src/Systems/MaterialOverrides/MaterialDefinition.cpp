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

#include "CSP/Systems/MaterialOverrides/MaterialDefinition.h"

#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "Debug/Logging.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace
{
csp::common::String ConvertParameterTypeToString(csp::systems::EMaterialParameterType ParameterType)
{
	if (ParameterType == csp::systems::EMaterialParameterType::Int)
		return "int";
	else if (ParameterType == csp::systems::EMaterialParameterType::Float)
		return "float";
	else if (ParameterType == csp::systems::EMaterialParameterType::Vector2)
		return "vector2";
	else if (ParameterType == csp::systems::EMaterialParameterType::Vector3)
		return "vector3";
	else if (ParameterType == csp::systems::EMaterialParameterType::Texture2d)
		return "texture2d";
	else
	{
		assert(false && "Unsupported Parameter Type!");
		return "invalid";
	}
}

csp::common::String ConvertFeatureNamesToString(csp::systems::EMaterialFeatures MaterialFeature)
{
	if (MaterialFeature == csp::systems::EMaterialFeatures::Secondary_Normal_Map)
		return "secondary_normal_map";
	else
	{
		assert(false && "Unsupported Feature name!");
		return "invalid";
	}
}
} // namespace

namespace csp::systems
{

MaterialParameterBase::MaterialParameterBase() : ParameterName(""), ParameterType(EMaterialParameterType::Invalid)
{
}

MaterialParameterBase::MaterialParameterBase(csp::common::String Name, EMaterialParameterType Type) : ParameterName(Name), ParameterType(Type)
{
}

MaterialParameterInt::MaterialParameterInt() : MaterialParameterBase("", EMaterialParameterType::Int), ParameterValue(0)
{
}

MaterialParameterInt::MaterialParameterInt(csp::common::String Name) : MaterialParameterBase(Name, EMaterialParameterType::Int), ParameterValue(0)
{
}

MaterialParameterInt::MaterialParameterInt(csp::common::String Name, int32_t Value)
	: MaterialParameterBase(Name, EMaterialParameterType::Int), ParameterValue(Value)
{
}

MaterialParameterFloat::MaterialParameterFloat() : MaterialParameterBase("", EMaterialParameterType::Float), ParameterValue(0.0)
{
}

MaterialParameterFloat::MaterialParameterFloat(csp::common::String Name)
	: MaterialParameterBase(Name, EMaterialParameterType::Float), ParameterValue(0.0)
{
}

MaterialParameterFloat::MaterialParameterFloat(csp::common::String Name, float Value)
	: MaterialParameterBase(Name, EMaterialParameterType::Float), ParameterValue(Value)
{
}

MaterialParameterVector2::MaterialParameterVector2() : MaterialParameterBase("", EMaterialParameterType::Vector2), ParameterValue({0, 0})
{
}

MaterialParameterVector2::MaterialParameterVector2(csp::common::String Name)
	: MaterialParameterBase(Name, EMaterialParameterType::Vector2), ParameterValue({0, 0})
{
}

MaterialParameterVector2::MaterialParameterVector2(csp::common::String Name, csp::common::Vector2& Value)
	: MaterialParameterBase(Name, EMaterialParameterType::Vector2), ParameterValue(Value)
{
}

MaterialParameterVector3::MaterialParameterVector3() : MaterialParameterBase("", EMaterialParameterType::Vector3), ParameterValue({0, 0, 0})
{
}

MaterialParameterVector3::MaterialParameterVector3(csp::common::String Name)
	: MaterialParameterBase(Name, EMaterialParameterType::Vector3), ParameterValue({0, 0, 0})
{
}

MaterialParameterVector3::MaterialParameterVector3(csp::common::String Name, csp::common::Vector3& Value)
	: MaterialParameterBase(Name, EMaterialParameterType::Vector3), ParameterValue(Value)
{
}

MaterialParameterTexture2d::MaterialParameterTexture2d()
	: MaterialParameterBase("", EMaterialParameterType::Texture2d), ParameterAssetIdValue(""), ParameterAssetCollectionIdValue("")
{
}

MaterialParameterTexture2d::MaterialParameterTexture2d(csp::common::String Name)
	: MaterialParameterBase(Name, EMaterialParameterType::Texture2d), ParameterAssetIdValue(""), ParameterAssetCollectionIdValue("")
{
}

MaterialParameterTexture2d::MaterialParameterTexture2d(csp::common::String Name,
													   csp::common::String AssetIdValue,
													   csp::common::String AssetCollectionIdValue)
	: MaterialParameterBase(Name, EMaterialParameterType::Texture2d)
	, ParameterAssetIdValue(AssetIdValue)
	, ParameterAssetCollectionIdValue(AssetCollectionIdValue)
{
}

static const csp::common::Array<MaterialParameterBase> InvalidParameters = csp::common::Array<MaterialParameterBase>();


MaterialDefinition::MaterialDefinition()
	: MaterialName("")
	, DefinitionVersion("0.0.1")
	, BaseColor("BaseColor")
	, Normal("Normal")
	, Emissive("Emissive")
	, EmissiveMultiplier("EmissiveMultiplier")
	, Opacity("Opacity")
	, AO("AO")
	, UVOffset("UVOffset")
	, UVScale("UVScale")
{
}

MaterialDefinition::MaterialDefinition(const csp::common::String& Name)
	: MaterialName(Name)
	, DefinitionVersion("0.0.1")
	, BaseColor("BaseColor")
	, Normal("Normal")
	, Emissive("Emissive")
	, EmissiveMultiplier("EmissiveMultiplier")
	, Opacity("Opacity")
	, AO("AO")
	, UVOffset("UVOffset")
	, UVScale("UVScale")
{
}

csp::common::Array<MaterialParameterBase> MaterialDefinition::GetFeatureParameters(EMaterialFeatures Feature) const
{
	if (MaterialFeatures.HasKey(Feature))
	{
		return MaterialFeatures[Feature];
	}

	CSP_LOG_ERROR_FORMAT("Specified Material feature has not been added.");

	return InvalidParameters;
}

void MaterialDefinition::SetFeatureParameters(EMaterialFeatures Feature, csp::common::Array<MaterialParameterBase> FeatureParameters)
{
	MaterialFeatures[Feature] = FeatureParameters;
}

void MaterialDefinition::RemoveFeature(EMaterialFeatures Feature)
{
	MaterialFeatures.Remove(Feature);
}

int32_t MaterialDefinition::GetFeatureCount() const
{
	return static_cast<int32_t>(MaterialFeatures.Size());
}

bool MaterialDefinition::IsFeatureDefined(EMaterialFeatures Feature) const
{
	return MaterialFeatures.HasKey(Feature);
}

csp::common::String MaterialDefinition::SerialiseToJson() const
{
	rapidjson::StringBuffer InternalStringBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> StringBufferWriter(InternalStringBuffer);

	StringBufferWriter.StartObject();

	StringBufferWriter.String("materialName");
	StringBufferWriter.String(MaterialName);

	StringBufferWriter.String("definitionVersion");
	StringBufferWriter.String(DefinitionVersion);

	// Begin material parameter array
	StringBufferWriter.String("materialProperties");
	StringBufferWriter.StartArray();

	// Parameter 1: BaseColor
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(BaseColor.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(BaseColor.GetParameterType()));
	StringBufferWriter.String("assetId");
	StringBufferWriter.String(BaseColor.GetParameterAssetId());
	StringBufferWriter.String("assetCollectionId");
	StringBufferWriter.String(BaseColor.GetParameterAssetCollectionId());
	StringBufferWriter.EndObject(4);

	// Parameter 2: Normal
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(Normal.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(Normal.GetParameterType()));
	StringBufferWriter.String("assetId");
	StringBufferWriter.String(Normal.GetParameterAssetId());
	StringBufferWriter.String("assetCollectionId");
	StringBufferWriter.String(Normal.GetParameterAssetCollectionId());
	StringBufferWriter.EndObject(4);

	// Parameter 3: Emissive
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(Emissive.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(Emissive.GetParameterType()));
	StringBufferWriter.String(EmissiveMultiplier.GetParameterName());
	StringBufferWriter.Double(EmissiveMultiplier.GetParameterValue());
	StringBufferWriter.String("assetId");
	StringBufferWriter.String(Emissive.GetParameterAssetId());
	StringBufferWriter.String("assetCollectionId");
	StringBufferWriter.String(Emissive.GetParameterAssetCollectionId());
	StringBufferWriter.EndObject(5);

	// Parameter 4: Opacity
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(Opacity.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(Opacity.GetParameterType()));
	StringBufferWriter.String("assetId");
	StringBufferWriter.String(Opacity.GetParameterAssetId());
	StringBufferWriter.String("assetCollectionId");
	StringBufferWriter.String(Opacity.GetParameterAssetCollectionId());
	StringBufferWriter.EndObject(4);

	// Parameter 5: AO
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(AO.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(AO.GetParameterType()));
	StringBufferWriter.String("assetId");
	StringBufferWriter.String(AO.GetParameterAssetId());
	StringBufferWriter.String("assetCollectionId");
	StringBufferWriter.String(AO.GetParameterAssetCollectionId());
	StringBufferWriter.EndObject(4);

	// Parameter 6: UVOffset
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(UVOffset.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(UVOffset.GetParameterType()));
	StringBufferWriter.String("parameterValue");
	StringBufferWriter.StartArray();
	StringBufferWriter.String("X");
	StringBufferWriter.Double(UVOffset.GetParameterValue().X);
	StringBufferWriter.String("Y");
	StringBufferWriter.Double(UVOffset.GetParameterValue().Y);
	StringBufferWriter.EndArray(2);
	StringBufferWriter.EndObject(3);

	// Parameter 7: UVScale
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(UVScale.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(UVScale.GetParameterType()));
	StringBufferWriter.String("parameterValue");
	StringBufferWriter.StartArray();
	StringBufferWriter.String("X");
	StringBufferWriter.Double(UVScale.GetParameterValue().X);
	StringBufferWriter.String("Y");
	StringBufferWriter.Double(UVScale.GetParameterValue().Y);
	StringBufferWriter.EndArray(2);
	StringBufferWriter.EndObject(3);

	StringBufferWriter.EndArray(7);
	// End material parameter array

	if (GetFeatureCount() > 0)
	{
		// Begin material features array
		StringBufferWriter.String("materialFeatures");
		StringBufferWriter.StartArray();

		// Begin feature object
		StringBufferWriter.StartObject();
		StringBufferWriter.String("featureName");
		StringBufferWriter.String(ConvertFeatureNamesToString(EMaterialFeatures::Secondary_Normal_Map));

		const csp::common::Array<MaterialParameterBase>& FeatureParameters = GetFeatureParameters(EMaterialFeatures::Secondary_Normal_Map);

		StringBufferWriter.String("featureParameters");
		// Begin feature parameters array
		StringBufferWriter.StartArray();

		for (size_t i = 0; i < FeatureParameters.Size(); ++i)
		{
			MaterialParameterBase ParameterBase = FeatureParameters.operator[](i);

			// Begin feature parameter object
			StringBufferWriter.StartObject();
			StringBufferWriter.String("parameterName");
			StringBufferWriter.String(ParameterBase.GetParameterName());

			if (ParameterBase.GetParameterType() == EMaterialParameterType::Vector2)
			{
				StringBufferWriter.String("parameterType");
				StringBufferWriter.String(ConvertParameterTypeToString(ParameterBase.GetParameterType()));

				const MaterialParameterVector2* Vector2Parameter = (MaterialParameterVector2*) &ParameterBase;

				StringBufferWriter.String("parameterValue");
				StringBufferWriter.StartArray();
				StringBufferWriter.String("X");
				StringBufferWriter.Double(Vector2Parameter->GetParameterValue().X);
				StringBufferWriter.String("Y");
				StringBufferWriter.Double(Vector2Parameter->GetParameterValue().Y);
				StringBufferWriter.EndArray(2);
			}

			// End feature parameter object
			StringBufferWriter.EndObject(3);
		}
		// End feature parameters array
		StringBufferWriter.EndArray(2);

		// End feature object
		StringBufferWriter.EndObject(GetFeatureCount());

		// End material features array
		StringBufferWriter.EndArray(GetFeatureCount());
	}

	StringBufferWriter.EndObject();

	csp::common::String JsonString(InternalStringBuffer.GetString());
	return JsonString;
}

bool MaterialDefinition::DeserialiseFromJson(const csp::common::String& json)
{
	return true;
}


} // namespace csp::systems
