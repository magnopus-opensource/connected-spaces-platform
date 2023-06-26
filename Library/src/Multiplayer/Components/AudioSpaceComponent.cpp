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
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h"

namespace
{
constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
constexpr const float DefaultVolume			   = 1.f;
} // namespace

namespace csp::multiplayer
{

AudioSpaceComponent::AudioSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Audio, Parent)
{
	Properties[static_cast<uint32_t>(AudioPropertyKeys::Position)]				 = csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(AudioPropertyKeys::PlaybackState)]			 = static_cast<int64_t>(AudioPlaybackState::Reset);
	Properties[static_cast<uint32_t>(AudioPropertyKeys::AudioType)]				 = static_cast<int64_t>(AudioType::Global);
	Properties[static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId)]			 = "";
	Properties[static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId)]		 = "";
	Properties[static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius)]		 = DefaultAttenuationRadius;
	Properties[static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback)]		 = false;
	Properties[static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay)]			 = 0.f;
	Properties[static_cast<uint32_t>(AudioPropertyKeys::Volume)]				 = DefaultVolume;
	Properties[static_cast<uint32_t>(AudioPropertyKeys::IsEnabled)]				 = true;
	Properties[static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef)] = "";

	SetScriptInterface(CSP_NEW AudioSpaceComponentScriptInterface(this));
}

const csp::common::Vector3& AudioSpaceComponent::GetPosition() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::Position));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void AudioSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Position), Value);
}

AudioPlaybackState AudioSpaceComponent::GetPlaybackState() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::PlaybackState));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<AudioPlaybackState>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return AudioPlaybackState::Pause;
}

void AudioSpaceComponent::SetPlaybackState(AudioPlaybackState Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::PlaybackState), static_cast<int64_t>(Value));
}

AudioType AudioSpaceComponent::GetAudioType() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<AudioType>(RepVal.GetInt());
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return AudioType::Global;
}

void AudioSpaceComponent::SetAudioType(AudioType Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType), static_cast<int64_t>(Value));
}

const csp::common::String& AudioSpaceComponent::GetAudioAssetId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AudioSpaceComponent::SetAudioAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId), Value);
}

const csp::common::String& AudioSpaceComponent::GetAssetCollectionId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AudioSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId), Value);
}

float AudioSpaceComponent::GetAttenuationRadius() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void AudioSpaceComponent::SetAttenuationRadius(float Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius), Value);
}

bool AudioSpaceComponent::GetIsLoopPlayback() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void AudioSpaceComponent::SetIsLoopPlayback(bool Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback), Value);
}

float AudioSpaceComponent::GetTimeSincePlay() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void AudioSpaceComponent::SetTimeSincePlay(float Value)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay), Value);
}

float AudioSpaceComponent::GetVolume() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void AudioSpaceComponent::SetVolume(float Value)
{
	if (Value >= 0.f && Value <= 1.f)
	{
		SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume), Value);
	}
	else
	{
		FOUNDATION_LOG_ERROR_FORMAT("Invalid value for volume (%.2f). Must be from 0.0 to 1.0", Value);
	}
}

bool AudioSpaceComponent::GetIsEnabled() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsEnabled));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void AudioSpaceComponent::SetIsEnabled(bool InValue)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsEnabled), InValue);
}

const csp::common::String& AudioSpaceComponent::GetThirdPartyComponentRef() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	FOUNDATION_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void AudioSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
	SetProperty(static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
