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

#include "CSP/Systems/Assets/LOD.h"

namespace csp::systems
{

csp::common::String CreateLODStyleVar(int LODLevel);
int GetLODLevelFromStylesArray(const csp::common::Array<csp::common::String>& Styles);

LODChain CreateLODChainFromAssets(const csp::common::Array<Asset>& Assets, const csp::common::String& AssetCollectionId);

bool ValidateNewLODLevelForChain(const LODChain& Chain, int LODLevel);

} // namespace csp::systems
