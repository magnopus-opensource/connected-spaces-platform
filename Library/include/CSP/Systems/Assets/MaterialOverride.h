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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/MaterialOverrides/MaterialDefinition.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @ingroup Material Override System
/// @brief A MaterialOverrideAsset represents an asset contains a MaterialDeinition object.
class CSP_API MaterialOverrideAsset
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	// template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	friend class AssetSystem;
	CSP_END_IGNORE
	/** @endcond */

public:
	MaterialOverrideAsset();
	~MaterialOverrideAsset() {};

	MaterialDefinition& GetMaterialDefinition();

private:
	MaterialDefinition Definition;
};


/// @ingroup Material Override System
/// @brief Data class used to contain information when attempting to download a MaterialOverride Asset.
class CSP_API MaterialOverrideResult : public csp::systems::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class AssetSystem;

	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	MaterialOverrideAsset& GetMaterialOverrideAsset();
	const MaterialOverrideAsset& GetMaterialOverrideAsset() const;

protected:
	MaterialOverrideResult() = delete;
	MaterialOverrideResult(void*) {};

private:
	CSP_NO_EXPORT MaterialOverrideResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode) : csp::systems::ResultBase(ResCode, HttpResCode) {};

	void SetMaterialOverrideAsset(const MaterialOverrideAsset& InAsset);
	void SetMaterialOverrideAsset(MaterialOverrideAsset&& InAsset);

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	MaterialOverrideAsset MaterialAsset;
};

/// @brief Callback containing MaterialOverride Asset.
/// @param Result MaterialOverrideResult : result class
typedef std::function<void(const MaterialOverrideResult& Result)> MaterialOverrideResultCallback;


} // namespace csp::systems
