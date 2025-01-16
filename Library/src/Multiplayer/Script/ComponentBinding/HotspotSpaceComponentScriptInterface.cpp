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
#include "Multiplayer/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

namespace csp::multiplayer
{

HotspotSpaceComponentScriptInterface::HotspotSpaceComponentScriptInterface(HotspotSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

std::string HotspotSpaceComponentScriptInterface::GetUniqueComponentId()
{
    return static_cast<HotspotSpaceComponent*>(Component)->GetUniqueComponentId().c_str();
}

DEFINE_SCRIPT_PROPERTY_VEC3(HotspotSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(HotspotSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_STRING(HotspotSpaceComponent, Name);

DEFINE_SCRIPT_PROPERTY_TYPE(HotspotSpaceComponent, bool, bool, IsTeleportPoint);
DEFINE_SCRIPT_PROPERTY_TYPE(HotspotSpaceComponent, bool, bool, IsSpawnPoint);

DEFINE_SCRIPT_PROPERTY_TYPE(HotspotSpaceComponent, bool, bool, IsVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(HotspotSpaceComponent, bool, bool, IsARVisible);

} // namespace csp::multiplayer
