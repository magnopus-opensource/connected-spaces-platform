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
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/Assets/TextureInfo.h"

namespace csp::systems
{
/// @brief Enum representing the shader type of a material. Currently not in use.
enum class EShaderType
{
	Standard
};

/// @ingroup Asset System
/// @brief Base class for a material.
class CSP_API Material
{
public:
	/// @brief Sets the user-defined name of the material
	/// @param Name const csp::common::String&
	void SetName(const csp::common::String& Name);

	/// @brief Gets the user-defined name of the material
	/// @return csp::common::String&
	const csp::common::String& GetName() const;

	/// @brief Gets the asset collection id for where this material is stored
	/// @return const csp::common::String&
	const csp::common::String& GetAssetCollectionId() const;

	/// @brief Gets the asset id for where this material is stored
	/// @return const csp::common::String&
	const csp::common::String& GetAssetId() const;

	/// @brief Constructor which links the material to an asset
	/// @param AssetCollectionId const csp::common::String& : The asset collection where the material info is stored
	/// @param AssetId const csp::common::String& : The asset where the material info is stored
	Material(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);
	Material() = default;

protected:
	csp::common::String Name;
	EShaderType Type;

	csp::common::String AssetCollectionId;
	csp::common::String AssetId;
};

} // namespace csp::systems
