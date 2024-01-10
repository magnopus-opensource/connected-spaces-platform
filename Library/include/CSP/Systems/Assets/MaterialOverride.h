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
#include "CSP/Common/Map.h"
#include "CSP/Systems/Assets/Asset.h"
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

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download a MaterialDefinition Object.
class CSP_API MaterialDefinitionResult : public csp::systems::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class AssetSystem;

	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	MaterialDefinition& GetMaterialDefinition();
	const MaterialDefinition& GetMaterialDefinition() const;

protected:
	MaterialDefinitionResult() = delete;
	MaterialDefinitionResult(void*) {};

private:
	CSP_NO_EXPORT MaterialDefinitionResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
		: csp::systems::ResultBase(ResCode, HttpResCode) {};

	void SetMaterialDefinition(const MaterialDefinition& InDefinition);
	void SetMaterialDefinition(MaterialDefinition&& InDefinition);

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	MaterialDefinition Definition;
};

/// @brief Callback containing MaterialDefinition Object.
/// @param Result MaterialDefinitionResult : result class
typedef std::function<void(const MaterialDefinitionResult& Result)> MaterialDefinitionResultCallback;


} // namespace csp::systems
