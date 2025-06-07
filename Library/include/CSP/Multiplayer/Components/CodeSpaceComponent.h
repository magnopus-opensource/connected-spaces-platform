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
/// @file CodeSpaceComponent.h
/// @brief Definitions and support for script components.

#pragma once
#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"

namespace csp::multiplayer
{

/// @brief Enumerates the supported scopes of a script.
enum class CodeScopeType
{
    LocalPlayMode = 0,
    LocalEditorMode,
    Server,
    Num
};

/// @brief Enumerates the list of properties that can be replicated for a script component.
enum class CodeComponentPropertyKeys
{
    ScriptAssetPath = 1,
    CodeScopeType,
    Attributes,
    Num
};

/// @ingroup CodeSpaceComponent
/// @brief Data representation of a CodeSpaceComponent.
class CSP_API CodeSpaceComponent : public ComponentBase
{
public:
    /// @brief Constructs the script space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    CodeSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the Path of the Script asset this Script component refers to.
    /// @return The Path of the Script asset this Script component refers to.
    const csp::common::String& GetScriptAssetPath() const;

    /// @brief Sets the Path of the Script asset this Script component refers to.
    /// @param Value The Path of the Script asset this Script component refers to.
    void SetScriptAssetPath(const csp::common::String& Value);

    /// @brief Gets the scope within which this script operates. 
    /// LocalPlayMode: Only executes in playmode, can not write to global state, cannot write to replicated entities
    /// LocalEditorMode: Only executes in editor mode, can not write to global state, cannot write to replicated entities
    /// Server: Executes on the server, can write to global state, can write to replicated entities
    /// @return The scope of this script.
    CodeScopeType GetCodeScopeType() const;

    /// @brief Sets the scope within which this script operates.
    /// LocalPlayMode: Only executes in playmode, can not write to global state, cannot write to replicated entities
    /// LocalEditorMode: Only executes in editor mode, can not write to global state, cannot write to replicated entities
    /// Server: Executes on the server, can write to global state, can write to replicated entities
    /// @param Scope The scope of this script.
    void SetCodeScopeType(CodeScopeType Scope);

        /// @brief Checks if the property with the specified Key exists in the list of replicated properties.
    /// @param Key Uniquely identifies the property for which the check is performed.
    /// @return True if the property with the specified Key exists in the list of replicated properties, false otherwise.
    bool HasAttribute(const csp::common::String& Key) const;

    /// @brief Retrieves the replicated value of the property identified by the specified Key.
    /// @param Key The ID of the property of which the value will be retrieved.
    /// @return The value of the property identified by the provided Key.
    const ReplicatedValue& GetAttribute(const csp::common::String& Key) const;

    /// @brief Sets a custom property by specifying a unique Key and its relative property Value.
    /// @param Key Uniquely identifies this new property.
    /// @param Value The value to store for this new property.
    void SetAttribute(const csp::common::String& Key, const ReplicatedValue& Value);

    /// @brief Removes the specified property by Key.
    /// @param Key The ID of the property that will be removed.
    void RemoveAttribute(const csp::common::String& Key);

    /// @brief Retrieves the list of all the keys of the properties available in the list of replicated values.
    /// @return The list of available property keys.
    csp::common::List<csp::common::String> GetAttributeKeys() const;

    /// @brief Returns the amount of properties currently stored in the list of replicated values.
    /// @return How many properties are currently stored in the list of replicated values of this component.
    int32_t GetNumProperties() const;

    /// @brief Returns the hash of the provided Key.
    /// @param Key The key for which an hash will be produced.
    /// @return The resulting hash for the provided Key.
    uint32_t GetAttributeSubscriptionKey(const csp::common::String& Key) const;

protected:
    void OnRemove() override;
    void AddKey(const csp::common::String& Key);
    void RemoveKey(const csp::common::String& Key);
};

} // namespace csp::multiplayer
