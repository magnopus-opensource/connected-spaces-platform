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
#include "AudioSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"

namespace csp::multiplayer
{
csp::multiplayer::AudioSpaceComponentScriptInterface::AudioSpaceComponentScriptInterface(AudioSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_VEC3(AudioSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_TYPE(AudioSpaceComponent, AudioPlaybackState, int64_t, PlaybackState);
DEFINE_SCRIPT_PROPERTY_TYPE(AudioSpaceComponent, AudioType, int64_t, AudioType);
DEFINE_SCRIPT_PROPERTY_STRING(AudioSpaceComponent, AudioAssetId);
DEFINE_SCRIPT_PROPERTY_STRING(AudioSpaceComponent, AssetCollectionId);
DEFINE_SCRIPT_PROPERTY_TYPE(AudioSpaceComponent, float, float, AttenuationRadius);
DEFINE_SCRIPT_PROPERTY_TYPE(AudioSpaceComponent, bool, bool, IsLoopPlayback);
DEFINE_SCRIPT_PROPERTY_TYPE(AudioSpaceComponent, float, float, TimeSincePlay);
DEFINE_SCRIPT_PROPERTY_TYPE(AudioSpaceComponent, float, float, Volume);

} // namespace csp::multiplayer
