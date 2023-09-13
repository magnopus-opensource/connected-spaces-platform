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

#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ReflectionSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

ReflectionSpaceComponent::ReflectionSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Reflection, Parent)
{
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::Name)]					  = "";
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId)]	  = "";
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId)]	  = "";
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::Position)]				  = csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::Scale)]				  = csp::common::Vector3::One();
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape)]		  = static_cast<int64_t>(ReflectionShape::UnitBox);
	Properties[static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef)] = "";

	SetScriptInterface(CSP_NEW ReflectionSpaceComponentScriptInterface(this));
}

const csp::common::String& ReflectionSpaceComponent::GetReflectionAssetId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void ReflectionSpaceComponent::SetReflectionAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionAssetId), Value);
}

const csp::common::String& ReflectionSpaceComponent::GetAssetCollectionId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void ReflectionSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::AssetCollectionId), Value);
}

const csp::common::String& ReflectionSpaceComponent::GetName() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Name));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void ReflectionSpaceComponent::SetName(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Name), Value);
}

const csp::common::Vector3& ReflectionSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void ReflectionSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Position), Value);
}

const csp::common::Vector4& ReflectionSpaceComponent::GetRotation() const
{
	static csp::common::Vector4 Rotation(0, 0, 0, 1);

	return Rotation;
}

const csp::common::Vector3& ReflectionSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void ReflectionSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::Scale), Value);
}

ReflectionShape ReflectionSpaceComponent::GetReflectionShape() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<ReflectionShape>(RepVal.GetInt());
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReflectionShape::UnitBox;
}

void ReflectionSpaceComponent::SetReflectionShape(ReflectionShape Value)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ReflectionShape), static_cast<int64_t>(Value));
}

const csp::common::String& ReflectionSpaceComponent::GetThirdPartyComponentRef() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void ReflectionSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(ReflectionPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
