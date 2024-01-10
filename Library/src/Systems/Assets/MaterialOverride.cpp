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

#include "CSP/Systems/Assets/MaterialOverride.h"

#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/MaterialOverrides/MaterialDefinition.h"
#include "Debug/Logging.h"

namespace csp::systems
{

MaterialOverrideAsset::MaterialOverrideAsset() : Definition()
{
}

MaterialDefinition& MaterialOverrideAsset::GetMaterialDefinition()
{
	return Definition;
}

MaterialOverrideAsset& MaterialOverrideResult::GetMaterialOverrideAsset()
{
	return MaterialAsset;
}

const MaterialOverrideAsset& MaterialOverrideResult::GetMaterialOverrideAsset() const
{
	return MaterialAsset;
}

void MaterialOverrideResult::SetMaterialOverrideAsset(const MaterialOverrideAsset& InAsset)
{
	MaterialAsset = InAsset;
}

void MaterialOverrideResult::SetMaterialOverrideAsset(MaterialOverrideAsset&& InAsset)
{
	MaterialAsset = std::move(InAsset);
}

void MaterialOverrideResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);
}

} // namespace csp::systems
