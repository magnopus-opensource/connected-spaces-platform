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

#include "CSP/Systems/Assets/Material.h"

#include "CSP/Common/Array.h"

#include "Json/JsonSerializer.h"

namespace csp::systems
{

Material::Material(const csp::common::String& InAssetCollectionId, const csp::common::String& InAssetId)
	: AssetCollectionId(InAssetCollectionId), AssetId(InAssetId), Name(""), Type(EShaderType::Standard)
{
}

void Material::SetName(const csp::common::String& InName)
{
	Name = InName;
}

const csp::common::String& Material::GetName() const
{
	return Name;
}

const csp::common::String& Material::GetAssetCollectionId() const
{
	return AssetCollectionId;
}

const csp::common::String& Material::GetAssetId() const
{
	return AssetId;
}

} // namespace csp::systems
