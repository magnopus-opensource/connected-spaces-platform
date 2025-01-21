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
#include "Multiplayer/Script/ComponentBinding/TextSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/TextSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

TextSpaceComponentScriptInterface::TextSpaceComponentScriptInterface(TextSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(TextSpaceComponent, Text);

DEFINE_SCRIPT_PROPERTY_VEC3(TextSpaceComponent, Scale);
DEFINE_SCRIPT_PROPERTY_VEC3(TextSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(TextSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_VEC3(TextSpaceComponent, TextColor);
DEFINE_SCRIPT_PROPERTY_VEC3(TextSpaceComponent, BackgroundColor);

DEFINE_SCRIPT_PROPERTY_TYPE(TextSpaceComponent, bool, bool, IsBackgroundVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(TextSpaceComponent, BillboardMode, int64_t, BillboardMode);
DEFINE_SCRIPT_PROPERTY_TYPE(TextSpaceComponent, float, uint32_t, Width);
DEFINE_SCRIPT_PROPERTY_TYPE(TextSpaceComponent, float, uint32_t, Height);
DEFINE_SCRIPT_PROPERTY_TYPE(TextSpaceComponent, bool, bool, IsVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(TextSpaceComponent, bool, bool, IsARVisible);

} // namespace csp::multiplayer
