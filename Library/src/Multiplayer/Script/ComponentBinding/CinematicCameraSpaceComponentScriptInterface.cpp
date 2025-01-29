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
#include "Multiplayer/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/CinematicCameraSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

CinematicCameraSpaceComponentScriptInterface::CinematicCameraSpaceComponentScriptInterface(CinematicCameraSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

float CinematicCameraSpaceComponentScriptInterface::GetFov() { return static_cast<CinematicCameraSpaceComponent*>(Component)->GetFov(); }

DEFINE_SCRIPT_PROPERTY_VEC3(CinematicCameraSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(CinematicCameraSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, FocalLength);
DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, AspectRatio);

DEFINE_SCRIPT_PROPERTY_VEC2(CinematicCameraSpaceComponent, SensorSize);

DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, NearClip);
DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, FarClip);
DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, Iso);

DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, ShutterSpeed);
DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, float, float, Aperture);

DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, bool, bool, IsViewerCamera);

DEFINE_SCRIPT_PROPERTY_TYPE(CinematicCameraSpaceComponent, bool, bool, IsEnabled);

} // namespace csp::multiplayer
