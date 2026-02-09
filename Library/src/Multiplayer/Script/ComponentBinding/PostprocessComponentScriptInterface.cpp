/*
 * Copyright 2026 Magnopus LLC

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
#include "Multiplayer/Script/ComponentBinding/PostprocessComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/PostprocessComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"

using namespace csp::systems;

namespace csp::multiplayer
{

PostprocessSpaceComponentScriptInterface::PostprocessSpaceComponentScriptInterface(PostprocessSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_VEC3(PostprocessSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(PostprocessSpaceComponent, Rotation);
DEFINE_SCRIPT_PROPERTY_VEC3(PostprocessSpaceComponent, Scale);
DEFINE_SCRIPT_PROPERTY_TYPE(PostprocessSpaceComponent, float, float, ExposureMin);
DEFINE_SCRIPT_PROPERTY_TYPE(PostprocessSpaceComponent, float, float, ExposureMax);
DEFINE_SCRIPT_PROPERTY_TYPE(PostprocessSpaceComponent, bool, bool, IsUnbound);

} // namespace csp::multiplayer
