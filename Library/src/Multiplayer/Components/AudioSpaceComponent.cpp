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

#include "CSP/Multiplayer/ComponentSchema.h"
#include "Multiplayer/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h"

#include <fmt/format.h>

namespace
{

constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
constexpr const float DefaultVolume = 1.f;

} // namespace

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Audio),
    "Audio",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::PlaybackState),
            "playbackState",
            static_cast<int64_t>(AudioPlaybackState::Reset),
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AudioType),
            "audioType",
            static_cast<int64_t>(AudioType::Global),
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AudioAssetId),
            "audioAssetId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AssetCollectionId),
            "assetCollectionId",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AttenuationRadius),
            "attenuationRadius",
            DefaultAttenuationRadius,
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::IsLoopPlayback),
            "isLoopPlayback",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::TimeSincePlay),
            "timeSincePlay",
            0.f,
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::Volume),
            {}, // not exposed to scripting via schema: we can't express value ranges (min, max) in schemas yet, so manually bind
            DefaultVolume,
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::IsEnabled),
            "isEnabled",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::ThirdPartyComponentRef),
            {}, // not exposed to scripting
            "",
        },
    },
};

const ComponentSchema& AudioSpaceComponent::GetSchema() { return Schema; }

AudioSpaceComponent::AudioSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
    SetScriptInterface(new AudioSpaceComponentScriptInterface(this));
}

const csp::common::Vector3& AudioSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AudioPropertyKeys::Position));
}

void AudioSpaceComponent::SetPosition(const csp::common::Vector3& value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Position), value); }

AudioPlaybackState AudioSpaceComponent::GetPlaybackState() const
{
    return static_cast<AudioPlaybackState>(GetIntegerProperty(static_cast<uint32_t>(AudioPropertyKeys::PlaybackState)));
}

void AudioSpaceComponent::SetPlaybackState(AudioPlaybackState value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::PlaybackState), static_cast<int64_t>(value));
}

const csp::common::String& AudioSpaceComponent::GetAudioAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId));
}

void AudioSpaceComponent::SetAudioAssetId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioAssetId), value);
}

const csp::common::String& AudioSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId));
}

void AudioSpaceComponent::SetAssetCollectionId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AssetCollectionId), value);
}

bool AudioSpaceComponent::GetIsLoopPlayback() const { return GetBooleanProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback)); }

void AudioSpaceComponent::SetIsLoopPlayback(bool value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsLoopPlayback), value); }

float AudioSpaceComponent::GetTimeSincePlay() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay)); }

void AudioSpaceComponent::SetTimeSincePlay(float value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::TimeSincePlay), value); }

bool AudioSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(AudioPropertyKeys::IsEnabled)); }

void AudioSpaceComponent::SetIsEnabled(bool inValue) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::IsEnabled), inValue); }

const csp::common::String& AudioSpaceComponent::GetThirdPartyComponentRef() const
{
    return GetStringProperty(static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef));
}

void AudioSpaceComponent::SetThirdPartyComponentRef(const csp::common::String& inValue)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::ThirdPartyComponentRef), inValue);
}

/* IAudioControlComponent */

float AudioSpaceComponent::GetAttenuationRadius() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius)); }

void AudioSpaceComponent::SetAttenuationRadius(float value) { SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AttenuationRadius), value); }

AudioType AudioSpaceComponent::GetAudioType() const
{
    return static_cast<AudioType>(GetIntegerProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType)));
}

void AudioSpaceComponent::SetAudioType(AudioType value)
{
    SetProperty(static_cast<uint32_t>(AudioPropertyKeys::AudioType), static_cast<int64_t>(value));
}

float AudioSpaceComponent::GetVolume() const { return GetFloatProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume)); }

void AudioSpaceComponent::SetVolume(float value)
{
    if (value >= 0.f && value <= 1.f)
    {
        SetProperty(static_cast<uint32_t>(AudioPropertyKeys::Volume), value);
    }
    else
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Invalid value for volume ({:.2f}). Must be from 0.0 to 1.0", value).c_str());
        }
    }
}

} // namespace csp::multiplayer
