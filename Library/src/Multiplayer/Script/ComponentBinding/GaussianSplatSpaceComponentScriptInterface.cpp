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

#include "Multiplayer/Script/ComponentBinding/GaussianSplatSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"


using namespace csp::systems;


namespace csp::multiplayer
{

GaussianSplatSpaceComponentScriptInterface::GaussianSplatSpaceComponentScriptInterface(GaussianSplatSpaceComponent* InComponent)
	: ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(GaussianSplatSpaceComponent, ExternalResourceAssetId);
DEFINE_SCRIPT_PROPERTY_STRING(GaussianSplatSpaceComponent, ExternalResourceAssetCollectionId);

DEFINE_SCRIPT_PROPERTY_VEC3(GaussianSplatSpaceComponent, Scale);
DEFINE_SCRIPT_PROPERTY_VEC3(GaussianSplatSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(GaussianSplatSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_TYPE(GaussianSplatSpaceComponent, bool, bool, IsVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(GaussianSplatSpaceComponent, bool, bool, IsARVisible);

DEFINE_SCRIPT_PROPERTY_VEC3(GaussianSplatSpaceComponent, Tint);

} // namespace csp::multiplayer
