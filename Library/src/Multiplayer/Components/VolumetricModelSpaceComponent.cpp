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

#include "CSP/Multiplayer/Components/VolumetricModelSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/VolumetricModelSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

VolumetricModelSpaceComponent::VolumetricModelSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::VolumetricModel, Parent)
{
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::ExternalResourceAssetId)]			  = "";
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::ExternalResourceAssetCollectionId)] = "";
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::Position)]						  = csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::Rotation)]						  = csp::common::Vector4::Identity();
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::Scale)]							  = csp::common::Vector3::One();
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::IsVisible)]						  = true;
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::IsARVisible)]						  = true;
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::ThirdPartyComponentRef)]			  = "";
	Properties[static_cast<uint32_t>(VolumetricModelPropertyKeys::IsShadowCaster)]					  = true;

	SetScriptInterface(CSP_NEW VolumetricModelSpaceComponentScriptInterface(this));
}


/* IExternalResourceComponent */

const csp::common::String& VolumetricModelSpaceComponent::GetExternalResourceAssetId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::ExternalResourceAssetId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void VolumetricModelSpaceComponent::SetExternalResourceAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::ExternalResourceAssetId), Value);
}


const csp::common::String& VolumetricModelSpaceComponent::GetExternalResourceAssetCollectionId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::ExternalResourceAssetCollectionId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void VolumetricModelSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::ExternalResourceAssetCollectionId), Value);
}


/* ITransformComponent */

const csp::common::Vector3& VolumetricModelSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void VolumetricModelSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::Position), Value);
}


const csp::common::Vector4& VolumetricModelSpaceComponent::GetRotation() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::Rotation));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector4();
}

void VolumetricModelSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::Rotation), Value);
}


const csp::common::Vector3& VolumetricModelSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void VolumetricModelSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::Scale), Value);
}


SpaceTransform VolumetricModelSpaceComponent::GetTransform() const
{
	SpaceTransform Transform;
	Transform.Position = GetPosition();
	Transform.Rotation = GetRotation();
	Transform.Scale	   = GetScale();

	return Transform;
}

void VolumetricModelSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
	SetPosition(InValue.Position);
	SetRotation(InValue.Rotation);
	SetScale(InValue.Scale);
}


/* IVisibleComponent */

bool VolumetricModelSpaceComponent::GetIsVisible() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::IsVisible));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void VolumetricModelSpaceComponent::SetIsVisible(bool InValue)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::IsVisible), InValue);
}


bool VolumetricModelSpaceComponent::GetIsARVisible() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::IsARVisible));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void VolumetricModelSpaceComponent::SetIsARVisible(bool InValue)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::IsARVisible), InValue);
}


const csp::common::String& VolumetricModelSpaceComponent::GetThirdPartyComponentRef() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::ThirdPartyComponentRef));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void VolumetricModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::ThirdPartyComponentRef), InValue);
}


bool VolumetricModelSpaceComponent::GetIsShadowCaster() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::IsShadowCaster));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void VolumetricModelSpaceComponent::SetIsShadowCaster(bool Value)
{
	SetProperty(static_cast<uint32_t>(VolumetricModelPropertyKeys::IsShadowCaster), Value);
}

} // namespace csp::multiplayer
