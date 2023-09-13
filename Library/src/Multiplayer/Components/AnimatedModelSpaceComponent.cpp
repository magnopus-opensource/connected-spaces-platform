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

#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/AnimatedModelSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

AnimatedModelSpaceComponent::AnimatedModelSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::AnimatedModel, Parent)
{
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId)]			= "";
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId)] = "";
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::Position)]							= csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation)]							= csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale)]								= csp::common::Vector3 {1, 1, 1};
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback)]					= false;
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying)]							= false;
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible)]							= true;
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex)]					= static_cast<int64_t>(-1);
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible)]						= true;
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef)]			= "";
	Properties[static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster)]					= true;

	SetScriptInterface(CSP_NEW AnimatedModelSpaceComponentScriptInterface(this));
}


/* IExternalResourceComponent */

const csp::common::String& AnimatedModelSpaceComponent::GetExternalResourceAssetId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void AnimatedModelSpaceComponent::SetExternalResourceAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetId), Value);
}


const csp::common::String& AnimatedModelSpaceComponent::GetExternalResourceAssetCollectionId() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void AnimatedModelSpaceComponent::SetExternalResourceAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId), Value);
}


/* ITransformComponent */

const csp::common::Vector3& AnimatedModelSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void AnimatedModelSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Position), Value);
}


const csp::common::Vector4& AnimatedModelSpaceComponent::GetRotation() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector4();
}

void AnimatedModelSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Rotation), Value);
}


const csp::common::Vector3& AnimatedModelSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void AnimatedModelSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::Scale), Value);
}


SpaceTransform AnimatedModelSpaceComponent::GetTransform() const
{
	SpaceTransform Transform;
	Transform.Position = GetPosition();
	Transform.Rotation = GetRotation();
	Transform.Scale	   = GetScale();

	return Transform;
}

void AnimatedModelSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
	SetPosition(InValue.Position);
	SetRotation(InValue.Rotation);
	SetScale(InValue.Scale);
}


bool AnimatedModelSpaceComponent::GetIsLoopPlayback() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void AnimatedModelSpaceComponent::SetIsLoopPlayback(bool Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsLoopPlayback), Value);
}


bool AnimatedModelSpaceComponent::GetIsPlaying() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void AnimatedModelSpaceComponent::SetIsPlaying(bool Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsPlaying), Value);
}


int64_t AnimatedModelSpaceComponent::GetAnimationIndex() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return RepVal.GetInt();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return 0;
}

void AnimatedModelSpaceComponent::SetAnimationIndex(int64_t Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::AnimationIndex), Value);
}


/* IVisibleComponent */

bool AnimatedModelSpaceComponent::GetIsVisible() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void AnimatedModelSpaceComponent::SetIsVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsVisible), Value);
}


bool AnimatedModelSpaceComponent::GetIsARVisible() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void AnimatedModelSpaceComponent::SetIsARVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsARVisible), Value);
}


/* IThirdPartyRefComponent */

const csp::common::String& AnimatedModelSpaceComponent::GetThirdPartyComponentRef() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void AnimatedModelSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::ThirdPartyComponentRef), InValue);
}


/* IShadowCasterComponent */

bool AnimatedModelSpaceComponent::GetIsShadowCaster() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return false;
}

void AnimatedModelSpaceComponent::SetIsShadowCaster(bool Value)
{
	SetProperty(static_cast<uint32_t>(AnimatedModelPropertyKeys::IsShadowCaster), Value);
}

} // namespace csp::multiplayer
