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
#include "Memory/Memory.h"

#include <rapidjson/document.h>
//#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace
{
csp::systems::EMaterialParameterSetName ConvertParameterNameStringToEnum(const csp::common::String& ParameterName)
{
	if (ParameterName == "BaseColor")
		return csp::systems::EMaterialParameterSetName::BaseColor;
	else if (ParameterName == "Normal")
		return csp::systems::EMaterialParameterSetName::Normal;
	else if (ParameterName == "Emissive")
		return csp::systems::EMaterialParameterSetName::Emissive;
	else if (ParameterName == "EmissiveMultiplier")
		return csp::systems::EMaterialParameterSetName::EmissiveMultiplier;
	else if (ParameterName == "Opacity")
		return csp::systems::EMaterialParameterSetName::Opacity;
	else if (ParameterName == "AO")
		return csp::systems::EMaterialParameterSetName::AO;
	else if (ParameterName == "UVOffset")
		return csp::systems::EMaterialParameterSetName::UVOffset;
	else if (ParameterName == "UVScale")
		return csp::systems::EMaterialParameterSetName::UVScale;
	else
	{
		assert(false && "Unsupported Parameter Name!");
		return csp::systems::EMaterialParameterSetName::Invalid;
	}
}

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

csp::systems::EMaterialParameterType ConvertParameterStringToType(csp::common::String ParameterType)
{
	if (ParameterType == "int")
		return csp::systems::EMaterialParameterType::Int;
	else if (ParameterType == "float")
		return csp::systems::EMaterialParameterType::Float;
	else if (ParameterType == "vector2")
		return csp::systems::EMaterialParameterType::Vector2;
	else if (ParameterType == "vector3")
		return csp::systems::EMaterialParameterType::Vector3;
	else if (ParameterType == "texture2d")
		return csp::systems::EMaterialParameterType::Texture2d;
	else
	{
		assert(false && "Unsupported Parameter Type!");
		return csp::systems::EMaterialParameterType::Invalid;
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

csp::systems::EMaterialFeatures ConvertFeatureNameStringToEnum(const csp::common::String& FeatureName)
{
	if (FeatureName == "secondary_normal_map")
		return csp::systems::EMaterialFeatures::Secondary_Normal_Map;
	else
	{
		assert(false && "Unsupported Feature name!");
		return csp::systems::EMaterialFeatures::Invalid;
	}
}

} // namespace

namespace csp::systems
{

void SetIntMaterialParameter(MaterialDefinition& OutMaterialDefinition,
							 EMaterialParameterSetName MaterialParameterName,
							 const csp::common::String& AtttributeParameterName,
							 const rapidjson::Value::ConstValueIterator& MatPropItr)
{
	CSP_LOG_ERROR_FORMAT("Function SetIntMaterialParameter not implement yet: %s.", AtttributeParameterName);
}

void SetFloatMaterialParameter(MaterialDefinition& OutMaterialDefinition,
							   EMaterialParameterSetName MaterialParameterName,
							   const csp::common::String& AtttributeParameterName,
							   const rapidjson::Value::ConstValueIterator& MatPropItr)
{
	const rapidjson::Value::ConstMemberIterator paramValueAttribute = MatPropItr->FindMember("parameterValue");

	if (paramValueAttribute == MatPropItr->MemberEnd() || !paramValueAttribute->value.IsFloat())
	{
		CSP_LOG_ERROR_FORMAT("Float parameter value for property invalid: %s.", AtttributeParameterName);
	}

	switch (MaterialParameterName)
	{
		case EMaterialParameterSetName::EmissiveMultiplier:
			OutMaterialDefinition.GetEmissiveMultiplier().SetParameterValue(paramValueAttribute->value.GetFloat());
			break;
		default:
			CSP_LOG_ERROR_FORMAT("The property name specified does not match the property type: %s.", AtttributeParameterName);
			break;
	}
}

void SetVector2MaterialParameter(MaterialDefinition& OutMaterialDefinition,
								 EMaterialParameterSetName MaterialParameterName,
								 const csp::common::String& AtttributeParameterName,
								 const rapidjson::Value::ConstValueIterator& MatPropItr)
{
	const rapidjson::Value::ConstMemberIterator paramValueAttribute = MatPropItr->FindMember("parameterValue");

	if (paramValueAttribute == MatPropItr->MemberEnd() || !paramValueAttribute->value.IsArray())
	{
		CSP_LOG_ERROR_FORMAT("Vector2 parameter value for property invalid: %s.", AtttributeParameterName);
	}

	float _X;
	float _Y;

	for (rapidjson::Value::ConstValueIterator Vec2PropItr = paramValueAttribute->value.Begin(); Vec2PropItr != paramValueAttribute->value.End();
		 ++Vec2PropItr)
	{
		const rapidjson::Value::ConstMemberIterator XAttribute = Vec2PropItr->FindMember("X");
		if (XAttribute != Vec2PropItr->MemberEnd() && XAttribute->value.IsFloat())
		{
			_X = XAttribute->value.GetFloat();
		}

		const rapidjson::Value::ConstMemberIterator YAttribute = Vec2PropItr->FindMember("Y");
		if (YAttribute != Vec2PropItr->MemberEnd() && YAttribute->value.IsFloat())
		{
			_Y = YAttribute->value.GetFloat();
		}
	}

	switch (MaterialParameterName)
	{
		case EMaterialParameterSetName::UVOffset:
			OutMaterialDefinition.GetUVOffset().SetParameterValue({_X, _Y});
			break;
		case EMaterialParameterSetName::UVScale:
			OutMaterialDefinition.GetUVScale().SetParameterValue({_X, _Y});
			break;
		default:
			CSP_LOG_ERROR_FORMAT("The property name specified does not match the property type: %s.", AtttributeParameterName);
			break;
	}
}

void SetVector3MaterialParameter(MaterialDefinition& OutMaterialDefinition,
								 EMaterialParameterSetName MaterialParameterName,
								 const csp::common::String& AtttributeParameterName,
								 const rapidjson::Value::ConstValueIterator& MatPropItr)
{
	CSP_LOG_ERROR_FORMAT("Function SetVector3MaterialParameter not implement yet: %s.", AtttributeParameterName);
}

void SetTexture2dMaterialParameter(MaterialDefinition& OutMaterialDefinition,
								   EMaterialParameterSetName MaterialParameterName,
								   const csp::common::String& AtttributeParameterName,
								   const rapidjson::Value::ConstValueIterator& MatPropItr)
{
	const rapidjson::Value::ConstMemberIterator paramAssetCollectionAttribute = MatPropItr->FindMember("assetCollectionId");
	const rapidjson::Value::ConstMemberIterator paramAssetAttribute			  = MatPropItr->FindMember("assetId");

	if (paramAssetCollectionAttribute == MatPropItr->MemberEnd() || paramAssetAttribute == MatPropItr->MemberEnd()
		|| !paramAssetCollectionAttribute->value.IsString() || !paramAssetAttribute->value.IsString())
	{
		CSP_LOG_ERROR_FORMAT("AssetCollection ID and/or Asset Id for texture2d property invalid: %s.", AtttributeParameterName);
	}

	switch (MaterialParameterName)
	{
		case EMaterialParameterSetName::BaseColor:
			OutMaterialDefinition.GetBaseColor().SetParameterAssetCollectionId(paramAssetCollectionAttribute->value.GetString());
			OutMaterialDefinition.GetBaseColor().SetParameterAssetId(paramAssetAttribute->value.GetString());
			break;
		case EMaterialParameterSetName::Normal:
			OutMaterialDefinition.GetNormal().SetParameterAssetCollectionId(paramAssetCollectionAttribute->value.GetString());
			OutMaterialDefinition.GetNormal().SetParameterAssetId(paramAssetAttribute->value.GetString());
			break;
		case EMaterialParameterSetName::Emissive:
			OutMaterialDefinition.GetEmissive().SetParameterAssetCollectionId(paramAssetCollectionAttribute->value.GetString());
			OutMaterialDefinition.GetEmissive().SetParameterAssetId(paramAssetAttribute->value.GetString());
			break;
		case EMaterialParameterSetName::Opacity:
			OutMaterialDefinition.GetOpacity().SetParameterAssetCollectionId(paramAssetCollectionAttribute->value.GetString());
			OutMaterialDefinition.GetOpacity().SetParameterAssetId(paramAssetAttribute->value.GetString());
			break;
		case EMaterialParameterSetName::AO:
			OutMaterialDefinition.GetAO().SetParameterAssetCollectionId(paramAssetCollectionAttribute->value.GetString());
			OutMaterialDefinition.GetAO().SetParameterAssetId(paramAssetAttribute->value.GetString());
			break;
		default:
			CSP_LOG_ERROR_FORMAT("The property name specified does not match the property type: %s.", AtttributeParameterName);
			break;
	}
}

void SetMaterialPropertyValues(MaterialDefinition& OutMaterialDefinition,
							   const EMaterialParameterType& MaterialParameterType,
							   const rapidjson::Value::ConstValueIterator& MatPropItr)
{
	const rapidjson::Value::ConstMemberIterator paramNameAttribute = MatPropItr->FindMember("parameterName");
	if (paramNameAttribute == MatPropItr->MemberEnd() || !paramNameAttribute->value.IsString())
	{
		CSP_LOG_ERROR_MSG("Invalid json format for Material property.");
	}

	csp::common::String AtttributeParameterName		= paramNameAttribute->value.GetString();
	EMaterialParameterSetName MaterialParameterName = ConvertParameterNameStringToEnum(AtttributeParameterName);

	switch (MaterialParameterType)
	{
		case EMaterialParameterType::Int:
			SetIntMaterialParameter(OutMaterialDefinition, MaterialParameterName, AtttributeParameterName, MatPropItr);
			break;
		case EMaterialParameterType::Float:
			SetFloatMaterialParameter(OutMaterialDefinition, MaterialParameterName, AtttributeParameterName, MatPropItr);
			break;
		case EMaterialParameterType::Vector2:
			SetVector2MaterialParameter(OutMaterialDefinition, MaterialParameterName, AtttributeParameterName, MatPropItr);
			break;
		case EMaterialParameterType::Vector3:
			SetVector3MaterialParameter(OutMaterialDefinition, MaterialParameterName, AtttributeParameterName, MatPropItr);
			break;
		case EMaterialParameterType::Texture2d:
			SetTexture2dMaterialParameter(OutMaterialDefinition, MaterialParameterName, AtttributeParameterName, MatPropItr);
			break;
		default:
			CSP_LOG_ERROR_FORMAT("Specified Material property does not have a valid type: %s.", AtttributeParameterName);
			break;
	}
}

void SetMaterialFeatureValues(MaterialDefinition& OutMaterialDefinition,
							  const csp::common::String& FeatureName,
							  const rapidjson::Value::ConstValueIterator& MatFeatItr)
{
	EMaterialFeatures MaterialFeature = ConvertFeatureNameStringToEnum(FeatureName);

	const rapidjson::Value::ConstMemberIterator featParamsAttribute = MatFeatItr->FindMember("featureParameters");

	if (featParamsAttribute == MatFeatItr->MemberEnd() || !featParamsAttribute->value.IsArray())
	{
		CSP_LOG_ERROR_FORMAT("Invalid json format for Material feature: %s.", FeatureName);
	}

	if (MaterialFeature == EMaterialFeatures::Secondary_Normal_Map)
	{
		MaterialFeatureSecondaryNormal SecondaryNormalFeature;

		csp::common::String ParamName;

		for (rapidjson::Value::ConstValueIterator FeatParamItr = featParamsAttribute->value.Begin(); FeatParamItr != featParamsAttribute->value.End();
			 ++FeatParamItr)
		{
			const rapidjson::Value::ConstMemberIterator paramNameAttribute = FeatParamItr->FindMember("parameterName");
			csp::common::String ParameterName							   = paramNameAttribute->value.GetString();

			if (paramNameAttribute != FeatParamItr->MemberEnd() && paramNameAttribute->value.IsString())
			{
				const rapidjson::Value::ConstMemberIterator paramValueAttribute = FeatParamItr->FindMember("parameterValue");

				if (paramValueAttribute == FeatParamItr->MemberEnd() || !paramValueAttribute->value.IsArray())
				{
					CSP_LOG_ERROR_MSG("Invalide type for parameter Array.");
				}

				float _X;
				float _Y;

				for (rapidjson::Value::ConstValueIterator Vec2PropItr = paramValueAttribute->value.Begin();
					 Vec2PropItr != paramValueAttribute->value.End();
					 ++Vec2PropItr)
				{
					const rapidjson::Value::ConstMemberIterator XAttribute = Vec2PropItr->FindMember("X");
					if (XAttribute != Vec2PropItr->MemberEnd() && XAttribute->value.IsFloat())
					{
						_X = XAttribute->value.GetFloat();
					}

					const rapidjson::Value::ConstMemberIterator YAttribute = Vec2PropItr->FindMember("Y");
					if (YAttribute != Vec2PropItr->MemberEnd() && YAttribute->value.IsFloat())
					{
						_Y = YAttribute->value.GetFloat();
					}
				}

				if (ParameterName == "UVOffset")
				{
					SecondaryNormalFeature.GetParameterUVOffset().SetParameterValue({_X, _Y});
				}
				else if (ParameterName == "UVScale")
				{
					SecondaryNormalFeature.GetParameterUVScale().SetParameterValue({_X, _Y});
				}
			}
		}
	}
}

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

MaterialFeatureBase::MaterialFeatureBase() : FeatureName("")
{
}

MaterialFeatureBase::MaterialFeatureBase(csp::common::String Name) : FeatureName(Name)
{
}

MaterialFeatureSecondaryNormal::MaterialFeatureSecondaryNormal()
	: MaterialFeatureBase("secondary_normal_map"), UVOffsetParameter(), UVScaleParameter()
{
}

static MaterialFeatureBase InvalidParameters = MaterialFeatureBase();


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
	MaterialFeatures = CSP_NEW csp::common::Map<EMaterialFeatures, MaterialFeatureBase>();
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
	MaterialFeatures = CSP_NEW csp::common::Map<EMaterialFeatures, MaterialFeatureBase>();
}

MaterialDefinition::MaterialDefinition(const MaterialDefinition& Other) : MaterialDefinition()
{
	*this = Other;
}

MaterialDefinition::~MaterialDefinition()
{
	CSP_DELETE(MaterialFeatures);
}

MaterialDefinition& MaterialDefinition::operator=(const MaterialDefinition& Other)
{
	MaterialName	   = Other.MaterialName;
	DefinitionVersion  = Other.DefinitionVersion;
	BaseColor		   = Other.BaseColor;
	Normal			   = Other.Normal;
	Emissive		   = Other.Emissive;
	EmissiveMultiplier = Other.EmissiveMultiplier;
	Opacity			   = Other.Opacity;
	AO				   = Other.AO;
	UVOffset		   = Other.UVOffset;
	UVScale			   = Other.UVScale;

	*MaterialFeatures = *Other.MaterialFeatures;

	return *this;
}

MaterialFeatureBase MaterialDefinition::GetFeatureParameters(EMaterialFeatures Feature) const
{
	if (MaterialFeatures->HasKey(Feature))
	{
		return (*MaterialFeatures)[Feature];
	}

	CSP_LOG_ERROR_FORMAT("Specified Material feature has not been added: %s.", ConvertFeatureNamesToString(Feature));

	return InvalidParameters;
}

void MaterialDefinition::SetFeatureParameters(EMaterialFeatures FeatureName, MaterialFeatureBase Feature)
{
	(*MaterialFeatures)[FeatureName] = Feature;
}

void MaterialDefinition::RemoveFeature(EMaterialFeatures Feature)
{
	(*MaterialFeatures).Remove(Feature);
}

int32_t MaterialDefinition::GetFeatureCount() const
{
	return static_cast<int32_t>(MaterialFeatures->Size());
}

bool MaterialDefinition::IsFeatureDefined(EMaterialFeatures Feature) const
{
	return MaterialFeatures->HasKey(Feature);
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
	StringBufferWriter.String("assetId");
	StringBufferWriter.String(Emissive.GetParameterAssetId());
	StringBufferWriter.String("assetCollectionId");
	StringBufferWriter.String(Emissive.GetParameterAssetCollectionId());
	StringBufferWriter.EndObject(4);

	// Parameter 4: Emissive Multiplier
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(EmissiveMultiplier.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(EmissiveMultiplier.GetParameterType()));
	StringBufferWriter.String("parameterValue");
	StringBufferWriter.Double(EmissiveMultiplier.GetParameterValue());
	StringBufferWriter.EndObject(3);

	// Parameter 5: Opacity
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

	// Parameter 6: AO
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

	// Parameter 7: UVOffset
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(UVOffset.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(UVOffset.GetParameterType()));
	StringBufferWriter.String("parameterValue");
	StringBufferWriter.StartArray();
	// Begin parameter X object
	StringBufferWriter.StartObject();
	StringBufferWriter.String("X");
	StringBufferWriter.Double(UVOffset.GetParameterValue().X);
	// End parameter X object
	StringBufferWriter.EndObject(1);
	// Start parameter Y object
	StringBufferWriter.StartObject();
	StringBufferWriter.String("Y");
	StringBufferWriter.Double(UVOffset.GetParameterValue().Y);
	// End parameter X object
	StringBufferWriter.EndObject(1);
	StringBufferWriter.EndArray(2);
	StringBufferWriter.EndObject(3);

	// Parameter 8: UVScale
	StringBufferWriter.StartObject();
	StringBufferWriter.String("parameterName");
	StringBufferWriter.String(UVScale.GetParameterName());
	StringBufferWriter.String("parameterType");
	StringBufferWriter.String(ConvertParameterTypeToString(UVScale.GetParameterType()));
	StringBufferWriter.String("parameterValue");
	StringBufferWriter.StartArray();
	// Begin parameter X object
	StringBufferWriter.StartObject();
	StringBufferWriter.String("X");
	StringBufferWriter.Double(UVScale.GetParameterValue().X);
	// End parameter X object
	StringBufferWriter.EndObject(1);
	// Start parameter Y object
	StringBufferWriter.StartObject();
	StringBufferWriter.String("Y");
	StringBufferWriter.Double(UVScale.GetParameterValue().Y);
	// End parameter X object
	StringBufferWriter.EndObject(1);
	StringBufferWriter.EndArray(2);
	StringBufferWriter.EndObject(3);

	// End material parameter array
	StringBufferWriter.EndArray(8);

	if (IsFeatureDefined(EMaterialFeatures::Secondary_Normal_Map))
	{
		MaterialFeatureSecondaryNormal* SecondaryNormalFeature
			= (MaterialFeatureSecondaryNormal*) &(MaterialFeatures->operator[](EMaterialFeatures::Secondary_Normal_Map));
		MaterialParameterVector2& UVOffsetParam = SecondaryNormalFeature->GetParameterUVOffset();
		MaterialParameterVector2& UVScaleParam	= SecondaryNormalFeature->GetParameterUVScale();

		// Begin material features array
		StringBufferWriter.String("materialFeatures");
		StringBufferWriter.StartArray();

		// Begin feature object
		StringBufferWriter.StartObject();

		StringBufferWriter.String("featureName");
		StringBufferWriter.String("secondary_normal_map");

		StringBufferWriter.String("featureParameters");
		// Begin feature parameters array
		StringBufferWriter.StartArray();

		// Begin UVOffset Feature parameter object
		StringBufferWriter.StartObject();
		StringBufferWriter.String("parameterName");
		StringBufferWriter.String("UVOffset");
		StringBufferWriter.String("parameterType");
		StringBufferWriter.String("vector2");
		StringBufferWriter.String("parameterValue");
		// Begin UVOffset Array
		StringBufferWriter.StartArray();
		// X element
		StringBufferWriter.StartObject();
		StringBufferWriter.String("X");
		StringBufferWriter.Double(UVOffsetParam.GetParameterValue().X);
		StringBufferWriter.EndObject(1);
		StringBufferWriter.StartObject();
		StringBufferWriter.String("Y");
		StringBufferWriter.Double(UVOffsetParam.GetParameterValue().Y);
		StringBufferWriter.EndObject(1);
		// End UVOffset Array
		StringBufferWriter.EndArray(2);
		// End feature parameter object
		StringBufferWriter.EndObject(3);

		// Begin UVScale Feature parameter object
		StringBufferWriter.StartObject();
		StringBufferWriter.String("parameterName");
		StringBufferWriter.String("UVScale");
		StringBufferWriter.String("parameterType");
		StringBufferWriter.String("vector2");
		StringBufferWriter.String("parameterValue");
		// Begin UVOffset Array
		StringBufferWriter.StartArray();
		// X element
		StringBufferWriter.StartObject();
		StringBufferWriter.String("X");
		StringBufferWriter.Double(UVScaleParam.GetParameterValue().X);
		StringBufferWriter.EndObject(1);
		StringBufferWriter.StartObject();
		StringBufferWriter.String("Y");
		StringBufferWriter.Double(UVScaleParam.GetParameterValue().Y);
		StringBufferWriter.EndObject(1);
		// End UVOffset Array
		StringBufferWriter.EndArray(2);
		// End feature parameter object
		StringBufferWriter.EndObject(3);

		// End feature parameters array
		StringBufferWriter.EndArray(2);

		// End feature object
		StringBufferWriter.EndObject(2);

		// End material features array
		StringBufferWriter.EndArray(1);
	}

	StringBufferWriter.EndObject();

	csp::common::String JsonString(InternalStringBuffer.GetString());
	return JsonString;
}

bool MaterialDefinition::DeserialiseFromJson(const csp::common::String& Json)
{
	if (Json.IsEmpty())
	{
		return false;
	}

	// Check string is valid json.
	rapidjson::Document doc;
	if (doc.Parse(Json.c_str()).HasParseError())
	{
		const rapidjson::ParseErrorCode ParseError = doc.GetParseError();
		CSP_LOG_FORMAT(LogLevel::Error, "Failed to deserialise a material definition JSON. Error code: %i", static_cast<int>(ParseError));

		return false;
	}

	MaterialName	  = doc["materialName"].GetString();
	DefinitionVersion = doc["definitionVersion"].GetString();

	// Get the material properties array.
	const rapidjson::Value& materialPropertyAttributes = doc["materialProperties"];
	if (!materialPropertyAttributes.IsArray())
	{
		return false;
	}

	for (rapidjson::Value::ConstValueIterator MatPropItr = materialPropertyAttributes.Begin(); MatPropItr != materialPropertyAttributes.End();
		 ++MatPropItr)
	{
		const rapidjson::Value::ConstMemberIterator currentAttribute = MatPropItr->FindMember("parameterType");

		if (currentAttribute != MatPropItr->MemberEnd() && currentAttribute->value.IsString())
		{
			csp::systems::EMaterialParameterType MaterialParameterType = ConvertParameterStringToType(currentAttribute->value.GetString());

			SetMaterialPropertyValues(*this, MaterialParameterType, MatPropItr);
		}
	}

	if (doc.HasMember("materialFeatures") == false)
	{
		CSP_LOG_MSG(LogLevel::Log, "No material features found.");
		return false;
	}

	const rapidjson::Value& materialPropertyFeatures = doc["materialFeatures"];
	if (!materialPropertyFeatures.IsArray())
	{
		CSP_LOG_MSG(LogLevel::Log, "No material features found.");
		return false;
	}

	for (rapidjson::Value::ConstValueIterator MatFeatItr = materialPropertyFeatures.Begin(); MatFeatItr != materialPropertyFeatures.End();
		 ++MatFeatItr)
	{
		const rapidjson::Value::ConstMemberIterator currentFeatureAttribute = MatFeatItr->FindMember("featureName");

		if (currentFeatureAttribute != MatFeatItr->MemberEnd() && currentFeatureAttribute->value.IsString())
		{
			csp::common::String FeatureName = currentFeatureAttribute->value.GetString();

			SetMaterialFeatureValues(*this, FeatureName, MatFeatItr);
		}
	}

	return true;
}

} // namespace csp::systems
