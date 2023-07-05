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

#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/CollisionSpaceComponentScriptInterface.h"


namespace
{

constexpr const float DefaultSphereRadius	   = 0.5f;
constexpr const float DefaultCapsuleHalfWidth  = 0.5f;
constexpr const float DefaultCapsuleHalfHeight = 1.f;

} // namespace


namespace csp::multiplayer
{

CollisionSpaceComponent::CollisionSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Collision, Parent)
{
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::Position)]				 = csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::Rotation)]				 = csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::Scale)]					 = csp::common::Vector3 {1, 1, 1};
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::CollisionShape)]		 = static_cast<int64_t>(CollisionShape::Box);
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::CollisionMode)]			 = static_cast<int64_t>(CollisionMode::Collision);
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::CollisionAssetId)]		 = "";
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::AssetCollectionId)]		 = "";
	Properties[static_cast<uint32_t>(CollisionPropertyKeys::ThirdPartyComponentRef)] = "";

	SetScriptInterface(CSP_NEW CollisionSpaceComponentScriptInterface(this));
}

const csp::common::Vector3& CollisionSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void CollisionSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Position), Value);
}

const csp::common::Vector4& CollisionSpaceComponent::GetRotation() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Rotation));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector4();
}

void CollisionSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& CollisionSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void CollisionSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::Scale), Value);
}

CollisionShape CollisionSpaceComponent::GetCollisionShape() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionShape));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<CollisionShape>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return CollisionShape::Box;
}

void CollisionSpaceComponent::SetCollisionShape(CollisionShape collisionShape)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionShape), static_cast<int64_t>(collisionShape));
}

CollisionMode CollisionSpaceComponent::GetCollisionMode() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionMode));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<CollisionMode>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return CollisionMode::Collision;
}

void CollisionSpaceComponent::SetCollisionMode(CollisionMode collisionMode)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionMode), static_cast<int64_t>(collisionMode));
}

const csp::common::String& CollisionSpaceComponent::GetCollisionAssetId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionAssetId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void CollisionSpaceComponent::SetCollisionAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::CollisionAssetId), Value);
}

const csp::common::String& CollisionSpaceComponent::GetAssetCollectionId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::AssetCollectionId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void CollisionSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::AssetCollectionId), Value);
}

const csp::common::Vector3 CollisionSpaceComponent::GetUnscaledBoundingBoxMin()
{
	return csp::common::Vector3(-0.5f, -0.5f, -0.5f);
}

const csp::common::Vector3 CollisionSpaceComponent::GetUnscaledBoundingBoxMax()
{
	return csp::common::Vector3(0.5f, 0.5f, 0.5f);
}

const csp::common::Vector3 CollisionSpaceComponent::GetScaledBoundingBoxMin()
{
	return csp::common::Vector3(-0.5f * GetScale().X, -0.5f * GetScale().Y, -0.5f * GetScale().Z);
}

const csp::common::Vector3 CollisionSpaceComponent::GetScaledBoundingBoxMax()
{
	return csp::common::Vector3(0.5f * GetScale().X, 0.5f * GetScale().Y, 0.5f * GetScale().Z);
}

float CollisionSpaceComponent::GetDefaultSphereRadius()
{
	return DefaultSphereRadius;
}

float CollisionSpaceComponent::GetDefaultCapsuleHalfWidth()
{
	return DefaultCapsuleHalfWidth;
}

float CollisionSpaceComponent::GetDefaultCapsuleHalfHeight()
{
	return DefaultCapsuleHalfHeight;
}

const csp::common::String& CollisionSpaceComponent::GetThirdPartyComponentRef() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(CollisionPropertyKeys::ThirdPartyComponentRef));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void CollisionSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(CollisionPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
