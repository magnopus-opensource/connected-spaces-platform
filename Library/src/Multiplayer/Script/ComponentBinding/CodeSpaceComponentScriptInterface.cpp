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

#include "Multiplayer/Script/ComponentBinding/CodeSpaceComponentScriptInterface.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "CSP/Common/List.h"
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

CodeSpaceComponentScriptInterface::CodeSpaceComponentScriptInterface(CodeSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(CodeSpaceComponent, ScriptAssetPath);

bool CodeSpaceComponentScriptInterface::HasAttribute(const std::string& Key) const
{
    return static_cast<CodeSpaceComponent*>(Component)->HasAttribute(Key.c_str());
}


const csp::multiplayer::CodeAttribute* CodeSpaceComponentScriptInterface::GetAttribute(
    const std::string& Key)
{
    return static_cast<csp::multiplayer::CodeSpaceComponent*>(Component)->GetAttribute(Key.c_str());
}

std::vector<std::string> CodeSpaceComponentScriptInterface::GetAttributeKeys()
{
    std::vector<std::string> ReturnValue;
    csp::common::List<csp::common::String> Keys = static_cast<CodeSpaceComponent*>(Component)->GetAttributeKeys();

    for (size_t i = 0; i < Keys.Size(); ++i)
    {
        ReturnValue.push_back(Keys[i].c_str());
    }

    return ReturnValue;
}

} // namespace csp::multiplayer
