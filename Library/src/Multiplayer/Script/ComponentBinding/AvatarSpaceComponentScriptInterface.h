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

#include <string>
#include <vector>

namespace csp::multiplayer
{

class AvatarSpaceComponent;

class AvatarSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    AvatarSpaceComponentScriptInterface(AvatarSpaceComponent* InComponent = nullptr);

    DECLARE_SCRIPT_PROPERTY(std::string, AvatarId);
    DECLARE_SCRIPT_PROPERTY(std::string, UserId);
    DECLARE_SCRIPT_PROPERTY(int64_t, State);
    DECLARE_SCRIPT_PROPERTY(int64_t, AvatarMeshIndex);
    DECLARE_SCRIPT_PROPERTY(std::string, AgoraUserId);
    DECLARE_SCRIPT_PROPERTY(std::string, CustomAvatarUrl);
    DECLARE_SCRIPT_PROPERTY(bool, IsHandIKEnabled);
    DECLARE_SCRIPT_PROPERTY(Vector3, TargetHandIKTargetLocation);
    DECLARE_SCRIPT_PROPERTY(Vector3, HandRotation);
    DECLARE_SCRIPT_PROPERTY(Vector3, HeadRotation);
    DECLARE_SCRIPT_PROPERTY(float, WalkRunBlendPercentage);
    DECLARE_SCRIPT_PROPERTY(float, TorsoTwistAlpha);
    DECLARE_SCRIPT_PROPERTY(int64_t, AvatarPlayMode);
};

} // namespace csp::multiplayer
