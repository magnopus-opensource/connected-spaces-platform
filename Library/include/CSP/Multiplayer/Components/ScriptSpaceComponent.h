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
    Num
};

/// @ingroup ScriptSpaceComponent
/// @brief Enables custom behavior through scripting.
///
/// This component allows developers to author scripts that control how entities and components behave based on specific conditions or user actions.
/// Scripts can modify entity properties, trigger events, or respond to user inputs.
class CSP_API ScriptSpaceComponent : public ComponentBase
{
public:
    /// @brief Constructs the script space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    ScriptSpaceComponent(SpaceEntity* Parent);

    /// @brief Retrieves the source of the script of this script component.
    /// @return The script source of this script component.
    const csp::common::String& GetScriptSource() const;

    /// @brief Sets the source of the script of this script component.
    /// @param ScriptSource The script source of this script component.
    void SetScriptSource(const csp::common::String& ScriptSource);

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
};

} // namespace csp::multiplayer
