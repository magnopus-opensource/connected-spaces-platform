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
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/LightSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

LightSpaceComponent::LightSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Light, Parent)
{
	Properties[static_cast<uint32_t>(LightPropertyKeys::LightType)]					   = static_cast<int64_t>(LightType::Point);
	Properties[static_cast<uint32_t>(LightPropertyKeys::Color)]						   = csp::common::Vector3 {255, 255, 255};
	Properties[static_cast<uint32_t>(LightPropertyKeys::Intensity)]					   = 5000.0f;
	Properties[static_cast<uint32_t>(LightPropertyKeys::Range)]						   = 1000.0f;
	Properties[static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle)]			   = 0.0f;
	Properties[static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle)]			   = 0.78539816339f; // Pi / 4
	Properties[static_cast<uint32_t>(LightPropertyKeys::Position)]					   = csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(LightPropertyKeys::Rotation)]					   = csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(LightPropertyKeys::IsVisible)]					   = true;
	Properties[static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId)]		   = "";
	Properties[static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId)] = "";
	Properties[static_cast<uint32_t>(LightPropertyKeys::LightCookieType)]			   = static_cast<int64_t>(LightCookieType::NoCookie);
	Properties[static_cast<uint32_t>(LightPropertyKeys::IsARVisible)]				   = true;

	SetScriptInterface(CSP_NEW LightSpaceComponentScriptInterface(this));
}

LightType LightSpaceComponent::GetLightType() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::LightType));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<LightType>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return LightType::Directional;
}

void LightSpaceComponent::SetLightType(LightType Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightType), static_cast<int64_t>(Value));
}

const csp::common::Vector3& LightSpaceComponent::GetColor() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::Color));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void LightSpaceComponent::SetColor(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::Color), Value);
}

float LightSpaceComponent::GetIntensity() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::Intensity));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void LightSpaceComponent::SetIntensity(float Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::Intensity), Value);
}

float LightSpaceComponent::GetRange() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::Range));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void LightSpaceComponent::SetRange(float Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::Range), Value);
}

float LightSpaceComponent::GetInnerConeAngle() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void LightSpaceComponent::SetInnerConeAngle(float Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::InnerConeAngle), Value);
}

float LightSpaceComponent::GetOuterConeAngle() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void LightSpaceComponent::SetOuterConeAngle(float Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::OuterConeAngle), Value);
}

const csp::common::Vector3& LightSpaceComponent::GetPosition() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::Position));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void LightSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::Position), Value);
}

const csp::common::Vector4& LightSpaceComponent::GetRotation() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::Rotation));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector4();
}

void LightSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::Rotation), Value);
}

/* IVisibleComponent */

bool LightSpaceComponent::GetIsVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::IsVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void LightSpaceComponent::SetIsVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsVisible), Value);
}

bool LightSpaceComponent::GetIsARVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::IsARVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void LightSpaceComponent::SetIsARVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::IsARVisible), Value);
}

const csp::common::String& LightSpaceComponent::GetLightCookieAssetId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void LightSpaceComponent::SetLightCookieAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetId), Value);
}

const csp::common::String& LightSpaceComponent::GetLightCookieAssetCollectionId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void LightSpaceComponent::SetLightCookieAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieAssetCollectionId), Value);
}

LightCookieType LightSpaceComponent::GetLightCookieType() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieType));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<LightCookieType>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return LightCookieType::NoCookie;
}

void LightSpaceComponent::SetLightCookieType(LightCookieType Value)
{
	SetProperty(static_cast<uint32_t>(LightPropertyKeys::LightCookieType), static_cast<int64_t>(Value));
}

} // namespace csp::multiplayer
