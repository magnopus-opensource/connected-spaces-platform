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
class IScriptBinding;

class ScriptRuntime
{
public:
    using ContextMap = std::map<int64_t, ScriptContext*>;
    using BindingList = std::list<IScriptBinding*>;
    using ModuleSourceMap = std::map<std::string, std::string>;
    using UrlAliasMap = std::map<std::string, std::string>;

    ScriptRuntime(ScriptSystem* InScriptSystem);
    ~ScriptRuntime();

    bool AddContext(int64_t ContextId);
    bool RemoveContext(int64_t ContextId);
    bool BindContext(int64_t ContextId);
    bool ResetContext(int64_t ContextId);
    ScriptContext* GetContext(int64_t ContextId);

    bool ExistsInContext(int64_t ContextId, const csp::common::String& ObjectName);

    void RegisterScriptBinding(IScriptBinding* ScriptBinding);
    void UnregisterScriptBinding(IScriptBinding* ScriptBinding);

    void BindContext(ScriptContext* Context);
    void ResetContext(ScriptContext* Context);

    void SetModuleSource(csp::common::String ModuleUrl, csp::common::String Source);
    void ClearModuleSource(csp::common::String ModuleUrl);
    csp::common::String GetModuleSource(csp::common::String ModuleUrl);
    void AddModuleUrlAlias(const csp::common::String& ModuleUrl, const csp::common::String& ModuleUrlAlias);
    bool GetModuleUrlAlias(const csp::common::String& ModuleUrl, csp::common::String& OutModuleUrlAlias);

    ScriptSystem* TheScriptSystem;
    qjs::Runtime* Runtime;

    ContextMap Contexts;
    BindingList Bindings;
    ModuleSourceMap Modules;
    UrlAliasMap UrlAliases;
};

} // namespace csp::systems
