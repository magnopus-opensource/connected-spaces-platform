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
#pragma once

#include "Multiplayer/Script/ComponentScriptInterface.h"

namespace csp::multiplayer
{
class AudioSpaceComponent;

class AudioSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    AudioSpaceComponentScriptInterface(AudioSpaceComponent* InComponent = nullptr);

    DECLARE_SCRIPT_PROPERTY(Vector3, Position);
    DECLARE_SCRIPT_PROPERTY(int64_t, PlaybackState);
    DECLARE_SCRIPT_PROPERTY(int64_t, AudioType);
    DECLARE_SCRIPT_PROPERTY(std::string, AudioAssetId);
    DECLARE_SCRIPT_PROPERTY(std::string, AssetCollectionId);
    DECLARE_SCRIPT_PROPERTY(float, AttenuationRadius);
    DECLARE_SCRIPT_PROPERTY(bool, IsLoopPlayback);
    DECLARE_SCRIPT_PROPERTY(float, TimeSincePlay);
    DECLARE_SCRIPT_PROPERTY(float, Volume);
};

} // namespace csp::multiplayer
