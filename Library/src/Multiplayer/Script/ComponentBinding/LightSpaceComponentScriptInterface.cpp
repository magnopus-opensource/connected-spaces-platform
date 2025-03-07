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
#include "Multiplayer/Script/ComponentBinding/LightSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/LightSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

LightSpaceComponentScriptInterface::LightSpaceComponentScriptInterface(LightSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, csp::multiplayer::LightType, int32_t, LightType);

DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, float, float, Intensity);
DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, float, float, Range);
DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, float, float, InnerConeAngle);
DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, float, float, OuterConeAngle);

DEFINE_SCRIPT_PROPERTY_VEC3(LightSpaceComponent, Color);
DEFINE_SCRIPT_PROPERTY_VEC3(LightSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(LightSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, bool, bool, IsVisible);

DEFINE_SCRIPT_PROPERTY_STRING(LightSpaceComponent, LightCookieAssetId);
DEFINE_SCRIPT_PROPERTY_TYPE(LightSpaceComponent, csp::multiplayer::LightCookieType, int32_t, LightCookieType);

} // namespace csp::multiplayer
