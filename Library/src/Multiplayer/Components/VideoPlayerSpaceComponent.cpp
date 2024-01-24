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
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h"

namespace
{
constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
}

namespace csp::multiplayer
{

VideoPlayerSpaceComponent::VideoPlayerSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::VideoPlayer, Parent)
{
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Name)]					= "";
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetId)]			= "";
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetURL)]			= "";
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::AssetCollectionId)]		= "";
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Position)]				= csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Rotation)]				= csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Scale)]					= csp::common::Vector3 {1, 1, 1};
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStateShared)]			= false;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoPlay)]				= false;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsLoopPlayback)]			= false;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoResize)]			= false;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::PlaybackState)]			= static_cast<int64_t>(0);
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::CurrentPlayheadPosition)] = 0.0f;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::TimeSincePlay)]			= 0.0f;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius)]		= DefaultAttenuationRadius;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType)]	= static_cast<int64_t>(VideoPlayerSourceType::AssetSource);
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible)]				= true;
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible)]				= true;
	Properties[static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId)]			= static_cast<int64_t>(0);
	Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled)]				= true;

	SetScriptInterface(CSP_NEW VideoPlayerSpaceComponentScriptInterface(this));
}

const csp::common::String& VideoPlayerSpaceComponent::GetName() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Name));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void VideoPlayerSpaceComponent::SetName(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Name), Value);
}

const csp::common::String& VideoPlayerSpaceComponent::GetVideoAssetId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void VideoPlayerSpaceComponent::SetVideoAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetId), Value);
}

const csp::common::String& VideoPlayerSpaceComponent::GetVideoAssetURL() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetURL));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void VideoPlayerSpaceComponent::SetVideoAssetURL(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetURL), Value);
}

const csp::common::String& VideoPlayerSpaceComponent::GetAssetCollectionId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AssetCollectionId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void VideoPlayerSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AssetCollectionId), Value);
}


/* ITransformComponent */

const csp::common::Vector3& VideoPlayerSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void VideoPlayerSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Position), Value);
}


const csp::common::Vector4& VideoPlayerSpaceComponent::GetRotation() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Rotation));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector4();
}

void VideoPlayerSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Rotation), Value);
}


const csp::common::Vector3& VideoPlayerSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void VideoPlayerSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Scale), Value);
}


SpaceTransform VideoPlayerSpaceComponent::GetTransform() const
{
	SpaceTransform Transform;
	Transform.Position = GetPosition();
	Transform.Rotation = GetRotation();
	Transform.Scale	   = GetScale();

	return Transform;
}

void VideoPlayerSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
	SetPosition(InValue.Position);
	SetRotation(InValue.Rotation);
	SetScale(InValue.Scale);
}


bool VideoPlayerSpaceComponent::GetIsStateShared() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStateShared));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsStateShared(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStateShared), Value);
}

bool VideoPlayerSpaceComponent::GetIsAutoPlay() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoPlay));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsAutoPlay(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoPlay), Value);
}

bool VideoPlayerSpaceComponent::GetIsLoopPlayback() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsLoopPlayback));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsLoopPlayback(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsLoopPlayback), Value);
}

bool VideoPlayerSpaceComponent::GetIsAutoResize() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoResize));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsAutoResize(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoResize), Value);
}

VideoPlayerPlaybackState VideoPlayerSpaceComponent::GetPlaybackState() const
{
	if (const auto& RepVal = GetProperty((uint32_t) VideoPlayerPropertyKeys::PlaybackState);
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<VideoPlayerPlaybackState>(RepVal.GetInt());
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return VideoPlayerPlaybackState::Reset;
}

void VideoPlayerSpaceComponent::SetPlaybackState(VideoPlayerPlaybackState Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::PlaybackState), static_cast<int64_t>(Value));
}

float VideoPlayerSpaceComponent::GetCurrentPlayheadPosition() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::CurrentPlayheadPosition));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void VideoPlayerSpaceComponent::SetCurrentPlayheadPosition(float Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::CurrentPlayheadPosition), Value);
}

float VideoPlayerSpaceComponent::GetTimeSincePlay() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::TimeSincePlay));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void VideoPlayerSpaceComponent::SetTimeSincePlay(float Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::TimeSincePlay), Value);
}

float VideoPlayerSpaceComponent::GetAttenuationRadius() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void VideoPlayerSpaceComponent::SetAttenuationRadius(float Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius), Value);
}


VideoPlayerSourceType VideoPlayerSpaceComponent::GetVideoPlayerSourceType() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<VideoPlayerSourceType>(RepVal.GetInt());
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return VideoPlayerSourceType::AssetSource;
}

void VideoPlayerSpaceComponent::SetVideoPlayerSourceType(VideoPlayerSourceType Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType), static_cast<int64_t>(Value));
}

uint16_t VideoPlayerSpaceComponent::GetMeshComponentId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		// Component IDs are always created as uint16_t so it is safe cast.
		return static_cast<uint16_t>(RepVal.GetInt());
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0;
}

void VideoPlayerSpaceComponent::SetMeshComponentId(uint16_t Value)
{
	SpaceEntity* Entity				= GetParent();
	SpaceEntitySystem* EntitySystem = Entity->GetSpaceEntitySystem();

	ComponentBase* FoundComponent = EntitySystem->FindComponentById(Value);

	if (FoundComponent == nullptr)
	{
		return;
	}

	SetProperty(static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId), static_cast<int64_t>(Value));
}

/* IVisibleComponent */

bool VideoPlayerSpaceComponent::GetIsVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible), Value);
}

bool VideoPlayerSpaceComponent::GetIsARVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsARVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible), Value);
}

/* IEnableableComponent */

bool VideoPlayerSpaceComponent::GetIsEnabled() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void VideoPlayerSpaceComponent::SetIsEnabled(bool Value)
{
	SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled), Value);
}

} // namespace csp::multiplayer
