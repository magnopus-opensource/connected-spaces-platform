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
#include "Multiplayer/Script/ComponentBinding/FogSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/FogSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

FogSpaceComponentScriptInterface::FogSpaceComponentScriptInterface(FogSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, FogMode, int32_t, FogMode);

DEFINE_SCRIPT_PROPERTY_VEC3(FogSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(FogSpaceComponent, Rotation);
DEFINE_SCRIPT_PROPERTY_VEC3(FogSpaceComponent, Scale);

DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, float, float, StartDistance);
DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, float, float, EndDistance);

DEFINE_SCRIPT_PROPERTY_VEC3(FogSpaceComponent, Color);

DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, float, float, Density);
DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, float, float, HeightFalloff);
DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, float, float, MaxOpacity);

DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, bool, bool, IsVolumetric);

DEFINE_SCRIPT_PROPERTY_TYPE(FogSpaceComponent, bool, bool, IsVisible);

} // namespace csp::multiplayer
