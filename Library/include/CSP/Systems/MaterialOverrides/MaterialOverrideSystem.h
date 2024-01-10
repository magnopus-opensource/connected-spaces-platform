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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/CancellationToken.h"
#include "CSP/Common/Optional.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/LOD.h"
#include "CSP/Systems/Assets/MaterialOverride.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemBase.h"


namespace csp::systems
{

class MaterialDefinition;

/// @ingroup Asset System
/// @brief Public facing system that allows uploading/downloading and creation of assets.
class CSP_API CSP_NO_DISPOSE MaterialOverrideSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */

public:
	~MaterialOverrideSystem();

	/// @brief Registers a Material Definition.
	/// @param MaterialName csp::common::String& : The Material name.
	/// @param Callback NullResultCallback : Callback when asynchronous task finishes.
	CSP_ASYNC_RESULT void RegisterMaterialOverride(const MaterialDefinition& Definition, NullResultCallback Callback);

	/// @brief Generates a new MaterialDefinition from a MaterialOverride asset.
	/// @param AssetCollectionId const csp::common::String& : The Asset Collection Id for the MaterialOverride asset.
	/// @param AssetId const csp::common::String& : The Asset Id for the MaterialOverride asset.
	/// @param Callback MaterialOverrideAssetResultCallback : Callback when asynchronous task finishes containing the new MaterialDefinition.
	CSP_ASYNC_RESULT void GenerateMaterialOverride(const csp::common::String& AssetCollectionId,
												   const csp::common::String& AssetId,
												   MaterialOverrideResultCallback Callback);


private:
	MaterialOverrideSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT MaterialOverrideSystem(csp::web::WebClient* InWebClient);
};

} // namespace csp::systems
