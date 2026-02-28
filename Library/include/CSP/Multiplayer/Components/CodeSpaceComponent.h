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

namespace csp::multiplayer
{

enum class CodeScopeType
{
    Local = 0,
    Owner = 1,
    Num
};

enum class CodeSpaceComponentPropertyKeys
{
    ScriptAssetPath = 1,
    CodeScopeType = 2,
    Attributes = 3,
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
};

} // namespace csp::multiplayer

