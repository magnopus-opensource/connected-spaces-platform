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

class HotspotSpaceComponent;

class HotspotSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    HotspotSpaceComponentScriptInterface(HotspotSpaceComponent* InComponent = nullptr);

    std::string GetUniqueComponentId();

    DECLARE_SCRIPT_PROPERTY(Vector3, Position);
    DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);
    DECLARE_SCRIPT_PROPERTY(std::string, Name);
    DECLARE_SCRIPT_PROPERTY(bool, IsTeleportPoint);
    DECLARE_SCRIPT_PROPERTY(bool, IsSpawnPoint);
    DECLARE_SCRIPT_PROPERTY(bool, IsVisible);
    DECLARE_SCRIPT_PROPERTY(bool, IsARVisible);
};

} // namespace csp::multiplayer
