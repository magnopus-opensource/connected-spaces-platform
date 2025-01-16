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

VideoPlayerSpaceComponent::VideoPlayerSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::VideoPlayer, Parent)
{
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Name_DEPRECATED)] = "";
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetId)] = "";
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoAssetURL)] = "";
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::AssetCollectionId)] = "";
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::Scale)] = csp::common::Vector3 { 1, 1, 1 };
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsStateShared)] = false;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoPlay)] = false;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsLoopPlayback)] = false;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsAutoResize)] = false;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::PlaybackState)] = static_cast<int64_t>(0);
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::CurrentPlayheadPosition)] = 0.0f;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::TimeSincePlay)] = 0.0f;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius)] = DefaultAttenuationRadius;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType)] = static_cast<int64_t>(VideoPlayerSourceType::AssetSource);
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible)] = true;
    Properties[static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId)] = static_cast<int64_t>(0);
    Properties[static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled)] = true;

    SetScriptInterface(CSP_NEW VideoPlayerSpaceComponentScriptInterface(this));
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

float VideoPlayerSpaceComponent::GetAttenuationRadius() const
{
    return GetFloatProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius));
}

void VideoPlayerSpaceComponent::SetAttenuationRadius(float Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::AttenuationRadius), Value);
}

VideoPlayerSourceType VideoPlayerSpaceComponent::GetVideoPlayerSourceType() const
{
    return static_cast<VideoPlayerSourceType>(GetIntegerProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType)));
}

void VideoPlayerSpaceComponent::SetVideoPlayerSourceType(VideoPlayerSourceType Value)
{
    SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::VideoPlayerSourceType), static_cast<int64_t>(Value));
}

uint16_t VideoPlayerSpaceComponent::GetMeshComponentId() const
{
    return GetIntegerProperty(static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId));
}

void VideoPlayerSpaceComponent::SetMeshComponentId(uint16_t Value)
{
    SpaceEntity* Entity = GetParent();
    SpaceEntitySystem* EntitySystem = Entity->GetSpaceEntitySystem();

    ComponentBase* FoundComponent = EntitySystem->FindComponentById(Value);

    if (FoundComponent == nullptr)
    {
        return;
    }

    SetProperty(static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId), static_cast<int64_t>(Value));
}

/* IVisibleComponent */

bool VideoPlayerSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible)); }

void VideoPlayerSpaceComponent::SetIsVisible(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsVisible), Value); }

bool VideoPlayerSpaceComponent::GetIsARVisible() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible)); }

void VideoPlayerSpaceComponent::SetIsARVisible(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsARVisible), Value); }

/* IEnableableComponent */

bool VideoPlayerSpaceComponent::GetIsEnabled() const { return GetBooleanProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled)); }

void VideoPlayerSpaceComponent::SetIsEnabled(bool Value) { SetProperty(static_cast<uint32_t>(VideoPlayerPropertyKeys::IsEnabled), Value); }

} // namespace csp::multiplayer
