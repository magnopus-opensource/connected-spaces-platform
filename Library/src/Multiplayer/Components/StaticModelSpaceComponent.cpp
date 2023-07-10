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
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

StaticModelSpaceComponent::StaticModelSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::StaticModel, Parent)
{
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::ModelAssetId)]		   = "";
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::AssetCollectionId)]	   = "";
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::Position)]			   = csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::Rotation)]			   = csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::Scale)]				   = csp::common::Vector3 {1, 1, 1};
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible)]			   = true;
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible)]			   = true;
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef)] = "";
	Properties[static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster)]		   = true;

	SetScriptInterface(CSP_NEW StaticModelSpaceComponentScriptInterface(this));
}

const csp::common::String& StaticModelSpaceComponent::GetModelAssetId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ModelAssetId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void StaticModelSpaceComponent::SetModelAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ModelAssetId), Value);
}

const csp::common::String& StaticModelSpaceComponent::GetAssetCollectionId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::AssetCollectionId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void StaticModelSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::AssetCollectionId), Value);
}

const csp::common::Vector3& StaticModelSpaceComponent::GetPosition() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Position));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void StaticModelSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Position), Value);
}

const csp::common::Vector4& StaticModelSpaceComponent::GetRotation() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Rotation));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector4();
}

void StaticModelSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& StaticModelSpaceComponent::GetScale() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Scale));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void StaticModelSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::Scale), Value);
}

/* IVisibleComponent */

bool StaticModelSpaceComponent::GetIsVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void StaticModelSpaceComponent::SetIsVisible(bool InValue)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsVisible), InValue);
}

bool StaticModelSpaceComponent::GetIsARVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void StaticModelSpaceComponent::SetIsARVisible(bool InValue)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsARVisible), InValue);
}

const csp::common::String& StaticModelSpaceComponent::GetThirdPartyComponentRef() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void StaticModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::ThirdPartyComponentRef), InValue);
}


bool StaticModelSpaceComponent::GetIsShadowCaster() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void StaticModelSpaceComponent::SetIsShadowCaster(bool Value)
{
	SetProperty(static_cast<uint32_t>(StaticModelPropertyKeys::IsShadowCaster), Value);
}

} // namespace csp::multiplayer
