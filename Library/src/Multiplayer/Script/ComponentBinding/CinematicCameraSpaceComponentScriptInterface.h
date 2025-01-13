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

class CinematicCameraSpaceComponent;

class CinematicCameraSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
	CinematicCameraSpaceComponentScriptInterface(CinematicCameraSpaceComponent* InComponent = nullptr);

	float GetFov();

	DECLARE_SCRIPT_PROPERTY(Vector3, Position);
	DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);

	DECLARE_SCRIPT_PROPERTY(float, FocalLength);
	DECLARE_SCRIPT_PROPERTY(float, AspectRatio);

	DECLARE_SCRIPT_PROPERTY(Vector2, SensorSize);

	DECLARE_SCRIPT_PROPERTY(float, NearClip);
	DECLARE_SCRIPT_PROPERTY(float, FarClip);
	DECLARE_SCRIPT_PROPERTY(float, Iso);
	DECLARE_SCRIPT_PROPERTY(float, ShutterSpeed);
	DECLARE_SCRIPT_PROPERTY(float, Aperture);

	DECLARE_SCRIPT_PROPERTY(bool, IsViewerCamera);

	DECLARE_SCRIPT_PROPERTY(bool, IsEnabled);
};

} // namespace csp::multiplayer
