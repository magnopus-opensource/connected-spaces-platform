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
#include "Multiplayer/Script/ComponentBinding/AvatarSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

AvatarSpaceComponentScriptInterface::AvatarSpaceComponentScriptInterface(AvatarSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(AvatarSpaceComponent, AvatarId);
DEFINE_SCRIPT_PROPERTY_STRING(AvatarSpaceComponent, UserId);
DEFINE_SCRIPT_PROPERTY_STRING(AvatarSpaceComponent, AgoraUserId);
DEFINE_SCRIPT_PROPERTY_STRING(AvatarSpaceComponent, CustomAvatarUrl);
DEFINE_SCRIPT_PROPERTY_TYPE(AvatarSpaceComponent, csp::multiplayer::AvatarState, int32_t, State);
DEFINE_SCRIPT_PROPERTY_TYPE(AvatarSpaceComponent, int64_t, int32_t, AvatarMeshIndex);
DEFINE_SCRIPT_PROPERTY_TYPE(AvatarSpaceComponent, bool, bool, IsHandIKEnabled);
DEFINE_SCRIPT_PROPERTY_VEC3(AvatarSpaceComponent, TargetHandIKTargetLocation);
DEFINE_SCRIPT_PROPERTY_VEC4(AvatarSpaceComponent, HandRotation);
DEFINE_SCRIPT_PROPERTY_VEC4(AvatarSpaceComponent, HeadRotation);
DEFINE_SCRIPT_PROPERTY_TYPE(AvatarSpaceComponent, float, float, WalkRunBlendPercentage);
DEFINE_SCRIPT_PROPERTY_TYPE(AvatarSpaceComponent, float, float, TorsoTwistAlpha);
DEFINE_SCRIPT_PROPERTY_TYPE(AvatarSpaceComponent, csp::multiplayer::AvatarPlayMode, int32_t, AvatarPlayMode);

} // namespace csp::multiplayer
