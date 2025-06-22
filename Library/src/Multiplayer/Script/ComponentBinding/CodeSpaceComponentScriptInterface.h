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

#include "Multiplayer/Script/ComponentScriptInterface.h"
#include "Multiplayer/Script/ComponentBinding/CodeAttributeScriptInterface.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"

#include <string>
#include <variant>
#include <vector>

namespace csp::multiplayer
{

class CodeSpaceComponent;

class CodeSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    DECLARE_SCRIPT_PROPERTY(std::string, ScriptAssetPath);
    CodeSpaceComponentScriptInterface(CodeSpaceComponent* InComponent = nullptr);
    bool HasAttribute(const std::string& Key) const;
    CodeAttributeScriptInterface GetAttribute(const std::string& Key);
    std::vector<std::string> GetAttributeKeys();
};

} // namespace csp::multiplayer
