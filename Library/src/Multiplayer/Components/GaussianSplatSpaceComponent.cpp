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

#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/GaussianSplatSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

GaussianSplatSpaceComponent::GaussianSplatSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::GaussianSplat, Parent)
{
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetId)]			  = "";
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId)] = "";
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::Position)]						  = csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::Rotation)]						  = csp::common::Vector4::Identity();
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::Scale)]							  = csp::common::Vector3::One();
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVisible)]						  = true;
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::IsARVisible)]						  = true;
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::ThirdPartyComponentRef)]			  = "";
	Properties[static_cast<uint32_t>(GaussianSplatPropertyKeys::IsShadowCaster)]					  = true;

	SetScriptInterface(CSP_NEW GaussianSplatSpaceComponentScriptInterface(this));
}


/* IExternalResourceComponent */

const csp::common::String& GaussianSplatSpaceComponent::GetExternalResourceAssetId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void GaussianSplatSpaceComponent::SetExternalResourceAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetId), Value);
}


const csp::common::String& GaussianSplatSpaceComponent::GetExternalResourceAssetCollectionId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void GaussianSplatSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId), Value);
}


/* ITransformComponent */

const csp::common::Vector3& GaussianSplatSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void GaussianSplatSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Position), Value);
}


const csp::common::Vector4& GaussianSplatSpaceComponent::GetRotation() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Rotation));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector4();
}

void GaussianSplatSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Rotation), Value);
}


const csp::common::Vector3& GaussianSplatSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void GaussianSplatSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::Scale), Value);
}


SpaceTransform GaussianSplatSpaceComponent::GetTransform() const
{
	SpaceTransform Transform;
	Transform.Position = GetPosition();
	Transform.Rotation = GetRotation();
	Transform.Scale	   = GetScale();

	return Transform;
}

void GaussianSplatSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
	SetPosition(InValue.Position);
	SetRotation(InValue.Rotation);
	SetScale(InValue.Scale);
}


/* IVisibleComponent */

bool GaussianSplatSpaceComponent::GetIsVisible() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVisible));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void GaussianSplatSpaceComponent::SetIsVisible(bool InValue)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsVisible), InValue);
}


bool GaussianSplatSpaceComponent::GetIsARVisible() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsARVisible));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void GaussianSplatSpaceComponent::SetIsARVisible(bool InValue)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsARVisible), InValue);
}


const csp::common::String& GaussianSplatSpaceComponent::GetThirdPartyComponentRef() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ThirdPartyComponentRef));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void GaussianSplatSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::ThirdPartyComponentRef), InValue);
}


bool GaussianSplatSpaceComponent::GetIsShadowCaster() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsShadowCaster));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void GaussianSplatSpaceComponent::SetIsShadowCaster(bool Value)
{
	SetProperty(static_cast<uint32_t>(GaussianSplatPropertyKeys::IsShadowCaster), Value);
}

} // namespace csp::multiplayer
