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

class FogSpaceComponent;

class FogSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    FogSpaceComponentScriptInterface(FogSpaceComponent* InComponent = nullptr);

    DECLARE_SCRIPT_PROPERTY(int64_t, FogMode);

    DECLARE_SCRIPT_PROPERTY(Vector3, Position);
    DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);
    DECLARE_SCRIPT_PROPERTY(Vector3, Scale);

    DECLARE_SCRIPT_PROPERTY(float, StartDistance);
    DECLARE_SCRIPT_PROPERTY(float, EndDistance);

    DECLARE_SCRIPT_PROPERTY(Vector3, Color);

    DECLARE_SCRIPT_PROPERTY(float, Density);
    DECLARE_SCRIPT_PROPERTY(float, HeightFalloff);
    DECLARE_SCRIPT_PROPERTY(float, MaxOpacity);

    DECLARE_SCRIPT_PROPERTY(bool, IsVolumetric);

    DECLARE_SCRIPT_PROPERTY(bool, IsVisible);
};

} // namespace csp::multiplayer
