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
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/AvatarSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

AvatarSpaceComponent::AvatarSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::AvatarData, Parent)
{
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId)]				   = "";
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId)]					   = "";
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::State)]					   = static_cast<int64_t>(AvatarState::Idle);
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarMeshIndex)]			   = static_cast<int64_t>(-1);
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId)]				   = "";
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::CustomAvatarUrl)]			   = "";
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled)]			   = false;
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation)] = csp::common::Vector3 {0.0f, 0.0f, 0.0f};
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation)]			   = csp::common::Vector4 {0.0f, 0.0f, 0.0f, 1.0f};
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation)]			   = csp::common::Vector4 {0.0f, 0.0f, 0.0f, 1.0f};
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage)]	   = 0.0f;
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha)]			   = 0.0f;
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode)]			   = static_cast<int64_t>(AvatarPlayMode::Default);
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection)]		   = csp::common::Vector3 {0.0f, 0.0f, 0.0f};
	Properties[static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel)]			   = static_cast<int64_t>(LocomotionModel::Grounded);

	SetScriptInterface(CSP_NEW AvatarSpaceComponentScriptInterface(this));
}

const csp::common::String& AvatarSpaceComponent::GetAvatarId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AvatarSpaceComponent::SetAvatarId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarId), Value);
}

const csp::common::String& AvatarSpaceComponent::GetUserId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AvatarSpaceComponent::SetUserId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::UserId), Value);
}

AvatarState AvatarSpaceComponent::GetState() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<AvatarState>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return AvatarState::Idle;
}

void AvatarSpaceComponent::SetState(AvatarState Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::State), static_cast<int64_t>(Value));
}

AvatarPlayMode AvatarSpaceComponent::GetAvatarPlayMode() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<AvatarPlayMode>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return AvatarPlayMode::Default;
}

void AvatarSpaceComponent::SetAvatarPlayMode(AvatarPlayMode Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarPlayMode), static_cast<int64_t>(Value));
}

const int64_t AvatarSpaceComponent::GetAvatarMeshIndex() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarMeshIndex));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return RepVal.GetInt();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0;
}

void AvatarSpaceComponent::SetAvatarMeshIndex(const int64_t Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AvatarMeshIndex), Value);
}

const csp::common::String& AvatarSpaceComponent::GetAgoraUserId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AvatarSpaceComponent::SetAgoraUserId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::AgoraUserId), Value);
}

const csp::common::String& AvatarSpaceComponent::GetCustomAvatarUrl() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::CustomAvatarUrl));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AvatarSpaceComponent::SetCustomAvatarUrl(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::CustomAvatarUrl), Value);
}

const bool AvatarSpaceComponent::GetIsHandIKEnabled() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void AvatarSpaceComponent::SetIsHandIKEnabled(bool Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::IsHandIKEnabled), Value);
}

const csp::common::Vector3& AvatarSpaceComponent::GetTargetHandIKTargetLocation() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void AvatarSpaceComponent::SetTargetHandIKTargetLocation(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation), Value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHandRotation() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector4();
}

void AvatarSpaceComponent::SetHandRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HandRotation), Value);
}

const csp::common::Vector4& AvatarSpaceComponent::GetHeadRotation() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector4();
}

void AvatarSpaceComponent::SetHeadRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::HeadRotation), Value);
}

const float AvatarSpaceComponent::GetWalkRunBlendPercentage() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void AvatarSpaceComponent::SetWalkRunBlendPercentage(float Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::WalkRunBlendPercentage), Value);
}

const float AvatarSpaceComponent::GetTorsoTwistAlpha() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void AvatarSpaceComponent::SetTorsoTwistAlpha(float Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::TorsoTwistAlpha), Value);
}

const csp::common::Vector3& csp::multiplayer::AvatarSpaceComponent::GetMovementDirection() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void csp::multiplayer::AvatarSpaceComponent::SetMovementDirection(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::MovementDirection), Value);
}

LocomotionModel AvatarSpaceComponent::GetLocomotionModel() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<LocomotionModel>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return LocomotionModel::Grounded;
}

void AvatarSpaceComponent::SetLocomotionModel(LocomotionModel Value)
{
	SetProperty(static_cast<uint32_t>(AvatarComponentPropertyKeys::LocomotionModel), static_cast<int64_t>(Value));
}

} // namespace csp::multiplayer
