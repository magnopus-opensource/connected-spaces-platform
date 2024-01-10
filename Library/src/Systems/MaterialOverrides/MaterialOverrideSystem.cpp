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
#include "CSP/Systems/MaterialOverrides/MaterialOverrideSystem.h"

#include "CSP/Systems/MaterialOverrides/MaterialDefinition.h"
#include "CallHelpers.h"
#include "Common/Algorithm.h"
#include "Services/PrototypeService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Web/RemoteFileManager.h"

// StringFormat needs to be here due to clashing headers
#include "CSP/Common/StringFormat.h"


// namespace chs = csp::services::generated::prototypeservice;
//
// constexpr int DEFAULT_SKIP_NUMBER		= 0;
// constexpr int DEFAULT_RESULT_MAX_NUMBER = 100;


namespace csp::systems
{

MaterialOverrideSystem::MaterialOverrideSystem() : SystemBase()
{
}

MaterialOverrideSystem::MaterialOverrideSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
}

MaterialOverrideSystem::~MaterialOverrideSystem()
{
}

void MaterialOverrideSystem::RegisterMaterialOverride(const MaterialDefinition& Definition, NullResultCallback Callback)
{
	// 1. Logic to parse the MaterialDefinition and turn into a json file.
	// 2. Upload json file to CHS.
	// 3. Store json file Asset Id and Asset Collection Id in the material definition.
	// 4. Return true or false.

	csp::common::String MaterialDefinitionJson = Definition.SerialiseToJson();
}

void MaterialOverrideSystem::GenerateMaterialOverride(const csp::common::String& AssetCollectionId,
													  const csp::common::String& AssetId,
													  MaterialOverrideResultCallback Callback)
{
	// 1. Retrieve the Material Definition json file using the provided Asset Id and AssetCollection Id.
	// 2. Parse the json file and create a new MaterialDefinition.
	// 3. Return the MaterialDefinition via the Callback.
}

} // namespace csp::systems
