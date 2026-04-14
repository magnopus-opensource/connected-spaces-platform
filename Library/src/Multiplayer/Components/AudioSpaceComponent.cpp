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
#include "CSP/Common/Systems/Log/LogSystem.h"

#include "Multiplayer/Component/Schema.h"
#include "Multiplayer/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h"

#include <fmt/format.h>

namespace
{

constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
constexpr const float DefaultVolume = 1.f;

} // namespace

namespace csp::multiplayer
{

const auto Schema = ComponentBase::ComponentSchema {
    ComponentType::Audio,
    std::vector<ComponentBase::ComponentSchema::Property> {
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::Position),
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::PlaybackState),
            static_cast<int64_t>(AudioPlaybackState::Reset),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::AudioType),
            static_cast<int64_t>(AudioType::Global),
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::AudioAssetId),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::AssetCollectionId),
            "",
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::AttenuationRadius),
            DefaultAttenuationRadius,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::IsLoopPlayback),
            false,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::TimeSincePlay),
            0.f,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::Volume),
            DefaultVolume,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::IsEnabled),
            true,
        },
        {
            static_cast<ComponentBase::PropertyKey>(AudioPropertyKeys::ThirdPartyComponentRef),
            "",
        },
    },
};

AudioSpaceComponent::AudioSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new AudioSpaceComponentScriptInterface(this));
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

bool AudioSpaceComponent::GetIsLoopPlayback() const { return GetBooleanProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback)); }

void AudioSpaceComponent::SetIsLoopPlayback(bool Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback), Value); }

float AudioSpaceComponent::GetTimeSincePlay() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay)); }

void AudioSpaceComponent::SetTimeSincePlay(float Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay), Value); }

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

/* IAudioSourceComponent */

float AudioSpaceComponent::GetAttenuationRadius() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius)); }

void AudioSpaceComponent::SetAttenuationRadius(float Value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius), Value); }

AudioType AudioSpaceComponent::GetAudioType() const
{
    return static_cast<AudioType>(GetIntegerProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType)));
}

void AudioSpaceComponent::SetAudioType(AudioType Value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType), static_cast<int64_t>(Value));
}

float AudioSpaceComponent::GetVolume() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume)); }

void AudioSpaceComponent::SetVolume(float Value)
{
    if (Value >= 0.f && Value <= 1.f)
    {
        SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume), Value);
    }
    else
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Invalid value for volume ({:.2f}). Must be from 0.0 to 1.0", Value).c_str());
        }
    }
}

} // namespace csp::multiplayer
