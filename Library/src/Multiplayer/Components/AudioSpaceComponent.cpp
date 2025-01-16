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
constexpr const float DefaultVolume = 1.f;

} // namespace

namespace csp::multiplayer
{

AudioSpaceComponent::AudioSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Audio, Parent)
{
    Properties[static_cast<uint32_t>(AudioPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(AudioPropertyKeys::PlaybackState)] = static_cast<int64_t>(AudioPlaybackState::Reset);
    Properties[static_cast<uint32_t>(AudioPropertyKeys::AudioType)] = static_cast<int64_t>(AudioType::Global);
    Properties[static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId)] = "";
    Properties[static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius)] = DefaultAttenuationRadius;
    Properties[static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback)] = false;
    Properties[static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay)] = 0.f;
    Properties[static_cast<uint32_t>(AudioPropertyKeys::Volume)] = DefaultVolume;
    Properties[static_cast<uint32_t>(AudioPropertyKeys::IsEnabled)] = true;
    Properties[static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef)] = "";

    SetScriptInterface(CSP_NEW AudioSpaceComponentScriptInterface(this));
}

const csp::common::Vector3& AudioSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AudioPropertyKeys::Position));
}

void AudioSpaceComponent::SetPosition(const csp::common::Vector3& Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Position), Value); }

AudioPlaybackState AudioSpaceComponent::GetPlaybackState() const
{
    return static_cast<AudioPlaybackState>(GetIntegerProperty(static_cast<uint32_t>(AudioPropertyKeys::PlaybackState)));
}

void AudioSpaceComponent::SetPlaybackState(AudioPlaybackState Value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::PlaybackState), static_cast<int64_t>(Value));
}

AudioType AudioSpaceComponent::GetAudioType() const
{
    return static_cast<AudioType>(GetIntegerProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType)));
}

void AudioSpaceComponent::SetAudioType(AudioType Value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType), static_cast<int64_t>(Value));
}

const csp::common::String& AudioSpaceComponent::GetAudioAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId));
}

void AudioSpaceComponent::SetAudioAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId), Value);
}

const csp::common::String& AudioSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId));
}

void AudioSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId), Value);
}

float AudioSpaceComponent::GetAttenuationRadius() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius)); }

void AudioSpaceComponent::SetAttenuationRadius(float Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius), Value); }

bool AudioSpaceComponent::GetIsLoopPlayback() const { return GetBooleanProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback)); }

void AudioSpaceComponent::SetIsLoopPlayback(bool Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback), Value); }

float AudioSpaceComponent::GetTimeSincePlay() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay)); }

void AudioSpaceComponent::SetTimeSincePlay(float Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay), Value); }

float AudioSpaceComponent::GetVolume() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume)); }

void AudioSpaceComponent::SetVolume(float Value)
{
    if (Value >= 0.f && Value <= 1.f)
    {
        SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume), Value);
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Invalid value for volume (%.2f). Must be from 0.0 to 1.0", Value);
    }
}

bool AudioSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(AudioPropertyKeys::IsEnabled)); }

void AudioSpaceComponent::SetIsEnabled(bool InValue) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsEnabled), InValue); }

const csp::common::String& AudioSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef));
}

void AudioSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& InValue)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef), InValue);
}

} // namespace csp::multiplayer
