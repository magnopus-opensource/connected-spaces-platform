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

#include "CSP/Common/Interfaces/IScriptBinding.h"
#include "CSP/Common/String.h"

#include <list>
#include <map>
#include <string>

namespace qjs
{
class Runtime;
}

namespace csp::systems
{

class ScriptSystem;
class ScriptContext;

class ScriptRuntime
{
public:
    using ContextMap = std::map<int64_t, ScriptContext*>;
    using BindingList = std::list<csp::common::IScriptBinding*>;
    using ModuleSourceMap = std::map<std::string, std::string>;
    using UrlAliasMap = std::map<std::string, std::string>;

    ScriptRuntime(ScriptSystem* inScriptSystem);
    ~ScriptRuntime();

    bool AddContext(int64_t contextId);
    bool RemoveContext(int64_t contextId);
    bool BindContext(int64_t contextId);
    bool ResetContext(int64_t contextId);
    ScriptContext* GetContext(int64_t contextId);

    bool ExistsInContext(int64_t contextId, const csp::common::String& objectName);

    void RegisterScriptBinding(csp::common::IScriptBinding* scriptBinding);
    void UnregisterScriptBinding(csp::common::IScriptBinding* scriptBinding);

    void BindContext(ScriptContext* context);
    void ResetContext(ScriptContext* context);

    void SetModuleSource(csp::common::String moduleUrl, csp::common::String source);
    void ClearModuleSource(csp::common::String moduleUrl);
    csp::common::String GetModuleSource(csp::common::String moduleUrl);
    void AddModuleUrlAlias(const csp::common::String& moduleUrl, const csp::common::String& moduleUrlAlias);
    bool GetModuleUrlAlias(const csp::common::String& moduleUrl, csp::common::String& outModuleUrlAlias);

    ScriptSystem* TheScriptSystem;
    qjs::Runtime* Runtime;

    ContextMap Contexts;
    BindingList Bindings;
    ModuleSourceMap Modules;
    UrlAliasMap UrlAliases;
};

} // namespace csp::systems
