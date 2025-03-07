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
#include "Multiplayer/Script/ComponentBinding/ImageSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

ImageSpaceComponentScriptInterface::ImageSpaceComponentScriptInterface(ImageSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(ImageSpaceComponent, Name);
DEFINE_SCRIPT_PROPERTY_STRING(ImageSpaceComponent, ImageAssetId);

DEFINE_SCRIPT_PROPERTY_VEC3(ImageSpaceComponent, Scale);
DEFINE_SCRIPT_PROPERTY_VEC3(ImageSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(ImageSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_TYPE(ImageSpaceComponent, csp::multiplayer::BillboardMode, int32_t, BillboardMode);
DEFINE_SCRIPT_PROPERTY_TYPE(ImageSpaceComponent, csp::multiplayer::DisplayMode, int32_t, DisplayMode);

DEFINE_SCRIPT_PROPERTY_TYPE(ImageSpaceComponent, bool, bool, IsEmissive);
DEFINE_SCRIPT_PROPERTY_TYPE(ImageSpaceComponent, bool, bool, IsVisible);

} // namespace csp::multiplayer
