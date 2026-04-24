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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"

#include "CSP/Multiplayer/ComponentSchema.h"
#include "Multiplayer/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h"

#include <fmt/format.h>

namespace
{
constexpr const float DefaultAttenuationRadius = 10.f; // Distance in meters
constexpr const float DefaultVolume = 1.f;
}

namespace csp::multiplayer
{

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::VideoPlayer),
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Name_DEPRECATED),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::VideoAssetId),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::VideoAssetURL),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::AssetCollectionId),
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Position),
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Rotation),
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Scale),
            csp::common::Vector3 { 1, 1, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsStateShared),
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsAutoPlay),
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsLoopPlayback),
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsAutoResize),
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::PlaybackState),
            static_cast<int64_t>(0),
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::CurrentPlayheadPosition),
            0.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::TimeSincePlay),
            0.0f,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::AttenuationRadius),
            DefaultAttenuationRadius,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::VideoPlayerSourceType),
            static_cast<int64_t>(VideoPlayerSourceType::AssetSource),
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::StereoVideoType),
            static_cast<int64_t>(StereoVideoType::None),
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsStereoFlipped),
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsVisible),
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsARVisible),
            true,
        },
        {
            static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId),
            static_cast<int64_t>(0),
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsEnabled),
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsVirtualVisible),
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Volume),
            DefaultVolume,
        },
        {
            static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::AudioType),
            static_cast<int64_t>(AudioType::Spatial),
        },
    },
};

VideoPlayerSpaceComponent::VideoPlayerSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new VideoPlayerSpaceComponentScriptInterface(this));
}

const csp::common::String& VideoPlayerSpaceComponent::GetName() const
{
    return GetStringProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Name_DEPRECATED));
}

void VideoPlayerSpaceComponent::SetName(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Name_DEPRECATED), Value);
}

const csp::common::String& VideoPlayerSpaceComponent::GetVideoAssetId() const
{
    return GetStringProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetId));
}

void VideoPlayerSpaceComponent::SetVideoAssetId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetId), Value);
}

const csp::common::String& VideoPlayerSpaceComponent::GetVideoAssetURL() const
{
    return GetStringProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetURL));
}

void VideoPlayerSpaceComponent::SetVideoAssetURL(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetURL), Value);
}

const csp::common::String& VideoPlayerSpaceComponent::GetAssetCollectionId() const
{
    return GetStringProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AssetCollectionId));
}

void VideoPlayerSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AssetCollectionId), Value);
}

/* ITransformComponent */

const csp::common::Vector3& VideoPlayerSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(VideoPlayerPropertyKeys::Position));
}

void VideoPlayerSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Position), Value);
}

const csp::common::Vector4& VideoPlayerSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(VideoPlayerPropertyKeys::Rotation));
}

void VideoPlayerSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& VideoPlayerSpaceComponent::GetScale() const
{
    return GetVector3Property(static_cast<uint32_t>(VideoPlayerPropertyKeys::Scale));
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
    Transform.Scale = GetScale();

    return Transform;
}

void VideoPlayerSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
    SetPosition(InValue.Position);
    SetRotation(InValue.Rotation);
    SetScale(InValue.Scale);
}

bool VideoPlayerSpaceComponent::GetIsStateShared() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStateShared)); }

void VideoPlayerSpaceComponent::SetIsStateShared(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStateShared), Value); }

bool VideoPlayerSpaceComponent::GetIsAutoPlay() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoPlay)); }

void VideoPlayerSpaceComponent::SetIsAutoPlay(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoPlay), Value); }

bool VideoPlayerSpaceComponent::GetIsLoopPlayback() const
{
    return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsLoopPlayback));
}

void VideoPlayerSpaceComponent::SetIsLoopPlayback(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsLoopPlayback), Value); }

bool VideoPlayerSpaceComponent::GetIsAutoResize() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoResize)); }

void VideoPlayerSpaceComponent::SetIsAutoResize(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoResize), Value); }

VideoPlayerPlaybackState VideoPlayerSpaceComponent::GetPlaybackState() const
{
    return static_cast<VideoPlayerPlaybackState>(GetIntegerProperty((uint32_t)VideoPlayerPropertyKeys::PlaybackState));
}

void VideoPlayerSpaceComponent::SetPlaybackState(VideoPlayerPlaybackState Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::PlaybackState), static_cast<int64_t>(Value));
}

float VideoPlayerSpaceComponent::GetCurrentPlayheadPosition() const
{
    return GetFloatProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::CurrentPlayheadPosition));
}

void VideoPlayerSpaceComponent::SetCurrentPlayheadPosition(float Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::CurrentPlayheadPosition), Value);
}

float VideoPlayerSpaceComponent::GetTimeSincePlay() const { return GetFloatProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::TimeSincePlay)); }

void VideoPlayerSpaceComponent::SetTimeSincePlay(float Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::TimeSincePlay), Value); }

VideoPlayerSourceType VideoPlayerSpaceComponent::GetVideoPlayerSourceType() const
{
    return static_cast<VideoPlayerSourceType>(GetIntegerProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType)));
}

void VideoPlayerSpaceComponent::SetVideoPlayerSourceType(VideoPlayerSourceType Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType), static_cast<int64_t>(Value));
}

StereoVideoType VideoPlayerSpaceComponent::GetStereoVideoType() const
{
    return static_cast<StereoVideoType>(GetIntegerProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::StereoVideoType)));
}

void VideoPlayerSpaceComponent::SetStereoVideoType(StereoVideoType Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::StereoVideoType), static_cast<int64_t>(Value));
}

bool VideoPlayerSpaceComponent::GetIsStereoFlipped() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStereoFlipped)); }

void VideoPlayerSpaceComponent::SetIsStereoFlipped(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStereoFlipped), Value); }

/* IVisibleComponent */

bool VideoPlayerSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible)); }

void VideoPlayerSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible), Value); }

bool VideoPlayerSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible)); }

void VideoPlayerSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible), Value); }

bool VideoPlayerSpaceComponent::GetIsVirtualVisible() const
{
    return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVirtualVisible));
}

void VideoPlayerSpaceComponent::SetIsVirtualVisible(bool Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVirtualVisible), Value);
}

/* IAudioControlComponent */

float VideoPlayerSpaceComponent::GetAttenuationRadius() const
{
    return GetFloatProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius));
}

void VideoPlayerSpaceComponent::SetAttenuationRadius(float Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius), Value);
}

AudioType VideoPlayerSpaceComponent::GetAudioType() const
{
    return static_cast<AudioType>(GetIntegerProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AudioType)));
}

void VideoPlayerSpaceComponent::SetAudioType(AudioType Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AudioType), static_cast<int64_t>(Value));
}

float VideoPlayerSpaceComponent::GetVolume() const { return GetFloatProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Volume)); }

void VideoPlayerSpaceComponent::SetVolume(float Value)
{
    if (Value >= 0.f && Value <= 1.f)
    {
        SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::Volume), Value);
    }
    else
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Invalid value for volume ({:.2f}). Must be from 0.0 to 1.0", Value).c_str());
        }
    }
}

/* IEnableableComponent */

bool VideoPlayerSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled)); }

void VideoPlayerSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled), Value); }

} // namespace csp::multiplayer
