/*
 * Copyright 2025 Magnopus LLC

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

#include "Multiplayer/Script/ComponentBinding/ScreenSharingSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/ScreenSharingSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"

using namespace csp::systems;

namespace csp::multiplayer
{

ScreenSharingSpaceComponentScriptInterface::ScreenSharingSpaceComponentScriptInterface(ScreenSharingSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(ScreenSharingSpaceComponent, UserId);
DEFINE_SCRIPT_PROPERTY_STRING(ScreenSharingSpaceComponent, DefaultImageCollectionId);
DEFINE_SCRIPT_PROPERTY_STRING(ScreenSharingSpaceComponent, DefaultImageAssetId);
DEFINE_SCRIPT_PROPERTY_TYPE(ScreenSharingSpaceComponent, float, float, AttenuationRadius);

DEFINE_SCRIPT_PROPERTY_VEC3(ScreenSharingSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(ScreenSharingSpaceComponent, Rotation);
DEFINE_SCRIPT_PROPERTY_VEC3(ScreenSharingSpaceComponent, Scale);

DEFINE_SCRIPT_PROPERTY_TYPE(ScreenSharingSpaceComponent, bool, bool, IsVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(ScreenSharingSpaceComponent, bool, bool, IsARVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(ScreenSharingSpaceComponent, bool, bool, IsVRVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(ScreenSharingSpaceComponent, bool, bool, IsShadowCaster);

} // namespace csp::multiplayer
