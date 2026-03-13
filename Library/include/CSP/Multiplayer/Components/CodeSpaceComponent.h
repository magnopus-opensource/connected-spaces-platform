/*
 * Copyright 2026 Magnopus LLC

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
#include "CSP/Common/List.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Multiplayer/Components/NgxScriptAttribute.h"

namespace csp::multiplayer
{

/// @brief Controls where and how a CodeSpaceComponent's script is executed.
enum class CodeScopeType
{
    /// Runs on every client. Entity mutations from within the script are local-only
    /// and not replicated. Executes at display rate.
    Local = 0,
    /// Runs on a single designated server client only. Entity mutations are
    /// fully replicated to all clients. There can be only one server runner per component.
    Server = 1,
    /// Like Local, but only executes when the runtime is in Editor mode.
    /// Useful for scene-authoring helpers that should have no effect at runtime.
    Editor = 2,
    Num
};

enum class CodeSpaceComponentPropertyKeys
{
    ScriptAssetPath = 1,
    CodeScopeType = 2,
    Attributes = 3,
    Schema = 4,
    Num
};

class CSP_API CodeSpaceComponent : public ComponentBase
{
public:
    CodeSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    const csp::common::String& GetScriptAssetPath() const;
    void SetScriptAssetPath(const csp::common::String& Value);

    csp::multiplayer::CodeScopeType GetCodeScopeType() const;
    void SetCodeScopeType(csp::multiplayer::CodeScopeType Value);

    bool HasAttribute(const csp::common::String& Key) const;
    bool GetAttribute(const csp::common::String& Key, CodeAttribute& OutAttribute) const;
    void SetAttribute(const csp::common::String& Key, const CodeAttribute& Attribute);
    void RemoveAttribute(const csp::common::String& Key);
    void ClearAttributes();
    csp::common::List<csp::common::String> GetAttributeKeys() const;

    /// @brief Get the raw $schema JSON string as replicated from the script system.
    const csp::common::String& GetSchema() const;

    /// @brief Set the raw $schema JSON string. Typically set by the script runtime
    /// after evaluating a script module's exported attributes declaration.
    void SetSchema(const csp::common::String& SchemaJson);

    /// @brief Parse the $schema and current attribute values into a list of
    /// NgxScriptAttribute objects that combine metadata with live values.
    csp::common::List<NgxScriptAttribute> GetScriptAttributes() const;
};

} // namespace csp::multiplayer
