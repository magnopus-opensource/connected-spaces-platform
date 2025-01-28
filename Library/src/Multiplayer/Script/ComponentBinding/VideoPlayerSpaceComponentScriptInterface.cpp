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
#include "Multiplayer/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

VideoPlayerSpaceComponentScriptInterface::VideoPlayerSpaceComponentScriptInterface(VideoPlayerSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING_ADAPTNAME(VideoPlayerSpaceComponent, Name, ComponentName);

DEFINE_SCRIPT_PROPERTY_VEC3(VideoPlayerSpaceComponent, Scale);
DEFINE_SCRIPT_PROPERTY_VEC3(VideoPlayerSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(VideoPlayerSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_STRING(VideoPlayerSpaceComponent, VideoAssetId);
DEFINE_SCRIPT_PROPERTY_STRING(VideoPlayerSpaceComponent, VideoAssetURL);
DEFINE_SCRIPT_PROPERTY_STRING(VideoPlayerSpaceComponent, AssetCollectionId);

DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, bool, bool, IsStateShared);
DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, bool, bool, IsAutoPlay);
DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, bool, bool, IsLoopPlayback);
DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, bool, bool, IsAutoResize);
DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, float, float, AttenuationRadius);

DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, csp::multiplayer::VideoPlayerPlaybackState, int64_t, PlaybackState);
DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, float, float, CurrentPlayheadPosition);
DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, float, float, TimeSincePlay);

DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, csp::multiplayer::VideoPlayerSourceType, int64_t, VideoPlayerSourceType);

DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, bool, bool, IsVisible);

DEFINE_SCRIPT_PROPERTY_TYPE(VideoPlayerSpaceComponent, bool, bool, IsEnabled);

} // namespace csp::multiplayer
