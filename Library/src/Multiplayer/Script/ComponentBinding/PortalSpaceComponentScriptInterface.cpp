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
#include "Multiplayer/Script/ComponentBinding/PortalSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/PortalSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"

namespace csp::multiplayer
{

csp::multiplayer::PortalSpaceComponentScriptInterface::PortalSpaceComponentScriptInterface(PortalSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(PortalSpaceComponent, SpaceId);
DEFINE_SCRIPT_PROPERTY_TYPE(PortalSpaceComponent, bool, bool, IsEnabled);
DEFINE_SCRIPT_PROPERTY_VEC3(PortalSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_TYPE(PortalSpaceComponent, float, float, Radius);

} // namespace csp::multiplayer
