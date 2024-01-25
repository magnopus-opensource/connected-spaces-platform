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

/// @file ScriptSpaceComponent.h
/// @brief Definitions and support for script components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "Interfaces/IExternalResourceComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the supported scopes of a script.
enum class ScriptScope
{
	Local = 0,
	Owner,
	Num
};


/// @brief Enumerates the list of properties that can be replicated for a script component.
enum class ScriptComponentPropertyKeys
{
	ScriptSource = 1,
	OwnerId,
	ScriptScope,
	ExternalResourceAssetId,
	ExternalResourceAssetCollectionId,
	Num
};


/// @ingroup ScriptSpaceComponent
/// @brief Data representation of a ScriptSpaceComponent.
class CSP_API ScriptSpaceComponent : public ComponentBase, public IExternalResourceComponent
{
	CSP_START_IGNORE
	/** @cond DO_NOT_DOCUMENT */
	friend class EntityScript;
	/** @endcond */
	CSP_END_IGNORE

public:
	/// @brief Constructs the script space component, and associates it with the specified Parent space entity.
	/// @param Parent The Space entity that owns this component.
	ScriptSpaceComponent(SpaceEntity* Parent);

	/// \addtogroup IExternalResourceComponent
	/// @{
	/// @brief Get the ID for the asset used to store the Script source for this component.
	/// @note To retrieve this component's Script source, both the Asset ID and the Asset Collection ID are required.
	/// @return csp::common::String : The Script source asset ID.
	const csp::common::String& GetExternalResourceAssetId() const override;

	/// @brief Set the ID for the asset used to store the Script source for this component.
	/// @note To retrieve this component's Script source, both the Asset ID and the Asset Collection ID are required.
	/// @param csp::common::String : The Script source asset ID.
	void SetExternalResourceAssetId(const csp::common::String& Value) override;

	/// @brief Gets the ID of the asset collection associated with this component.
	/// @note To retrieve this component's Script source, both the Asset ID and the Asset Collection ID are required.
	/// @return The ID of the asset collection associated with this component.
	const csp::common::String& GetExternalResourceAssetCollectionId() const override;

	/// @brief Sets the ID of the asset collection associated with this component.
	/// @note To retrieve this component's Script source, both the Asset ID and the Asset Collection ID are required.
	/// @param Value The ID of the asset collection associated with this component.
	void SetExternalResourceAssetCollectionId(const csp::common::String& Value) override;
	/// @}

	/* clang-format off */
	[[deprecated("The Script Component now uses an Asset to store the Script source. Please use the associated Get/SetExternalResourceAssetId and Get/SetExternalResourceAssetCollectionId methods.")]]
	/// @brief Retrieves the source of the script of this script component.
	/// @return The script source of this script component.
	const csp::common::String& GetScriptSource() const;
    
	[[deprecated("The Script Component now uses an Asset to store the Script source. Please use the associated Get/SetExternalResourceAssetId and Get/SetExternalResourceAssetCollectionId methods.")]]
	/// @brief Sets the source of the script of this script component.
	/// @param ScriptSource The script source of this script component.
	void SetScriptSource(const csp::common::String& ScriptSource);
	/* clang-format on */

	/// @brief Returns whether the Script source is prototype backed or not.
	/// This check is present to support backwards compatibility.
	/// @return bool : Whether Script source is prototype backed.
	bool GetIsPrototypeBacked() const
	{
		return IsPrototypeBacked;
	}

	/// @brief Gets the ID of the owner of this script component.
	/// @return The ID of the owner of this script.
	int64_t GetOwnerId() const;

	/// @brief Sets the ID of the owner of this script component.
	/// @param OwnerId The ID of the owner of this script.
	void SetOwnerId(int64_t OwnerId);

	/// @brief Gets the scope within which this script operates.
	/// @return The scope of this script.
	ScriptScope GetScriptScope() const;

	/// @brief Sets the scope within which this script operates.
	/// @param Scope The scope of this script.
	void SetScriptScope(ScriptScope Scope);

protected:
	void SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value) override;
	void OnRemove() override;

private:
	void SetComponentScriptSource(const csp::common::String& Value);

	// Required to support backwards compatibility of ScriptComponents that store their source via a replicated property.
	bool IsPrototypeBacked;
	csp::common::String ScriptSource;
};

} // namespace csp::multiplayer
