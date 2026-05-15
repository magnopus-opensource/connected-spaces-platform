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
#include "Systems/Script/ScriptRuntime.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "Debug/Logging.h"
#include "Systems/Script/ScriptContext.h"
#include "quickjspp.hpp"

#if defined(DEBUG) && defined(CSP_WINDOWS)
// In windows debug all script modules to be loaded from files
#define CSP_SCRIPT_ALLOW_FILE_MODULES
#endif

namespace csp::systems
{

#if defined(CSP_SCRIPT_ALLOW_FILE_MODULES)
inline std::optional<std::string> ReadScriptModuleFile(std::filesystem::path const& filepath)
{
    if (!std::filesystem::exists(filepath))
        return std::nullopt;
    std::ifstream f(filepath, std::ios::in | std::ios::binary);
    if (!f.is_open())
        return std::nullopt;
    std::stringstream sstream;
    sstream << f.rdbuf();
    return sstream.str();
}
#endif

ScriptRuntime::ScriptRuntime(ScriptSystem* inScriptSystem)
    : TheScriptSystem(inScriptSystem)
    , Runtime(new qjs::Runtime())
{
}

ScriptRuntime::~ScriptRuntime()
{
    for (auto context : Contexts)
    {
        delete (context.second);
    }

    delete (Runtime);
}

bool ScriptRuntime::AddContext(int64_t contextId)
{
    ContextMap::iterator it = Contexts.find(contextId);

    if (it == Contexts.end())
    {
        ScriptContext* theScriptContext = new ScriptContext(TheScriptSystem, Runtime, contextId);
        Contexts.insert(ContextMap::value_type(contextId, theScriptContext));
        return true;
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Context %lld already exists\n", contextId);
    }

    return false;
}

bool ScriptRuntime::RemoveContext(int64_t contextId)
{
    ContextMap::iterator it = Contexts.find(contextId);

    if (it != Contexts.end())
    {
        delete (it->second);
        Contexts.erase(it);
        return true;
    }

    return false;
}

ScriptContext* ScriptRuntime::GetContext(int64_t contextId)
{
    ContextMap::iterator it = Contexts.find(contextId);

    if (it != Contexts.end())
    {
        return it->second;
    }

    return nullptr;
}

bool ScriptRuntime::BindContext(int64_t contextId)
{
    ScriptContext* context = GetContext(contextId);
    if (context)
    {
        BindContext(context);
        return true;
    }

    return false;
}

bool ScriptRuntime::ResetContext(int64_t contextId)
{
    ScriptContext* context = GetContext(contextId);
    if (context)
    {
        ResetContext(context);
        return true;
    }

    return false;
}

bool ScriptRuntime::ExistsInContext(int64_t contextId, const csp::common::String& objectName)
{
    ScriptContext* context = GetContext(contextId);
    if (context)
    {
        return context->ExistsInContext(objectName);
    }

    return false;
}

void ScriptRuntime::RegisterScriptBinding(csp::common::IScriptBinding* scriptBinding) { Bindings.push_back(scriptBinding); }

void ScriptRuntime::UnregisterScriptBinding(csp::common::IScriptBinding* scriptBinding) { Bindings.remove(scriptBinding); }

void ScriptRuntime::BindContext(ScriptContext* context)
{
    for (auto binding : Bindings)
    {
        binding->Bind(context->GetId(), *TheScriptSystem);
    }
}

void ScriptRuntime::ResetContext(ScriptContext* context) { context->Reset(); }

void ScriptRuntime::SetModuleSource(csp::common::String moduleUrl, csp::common::String source)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "ScriptRuntime::SetModuleSource %s\n", moduleUrl.c_str());
    Modules[moduleUrl.c_str()] = source.c_str();
}

void ScriptRuntime::AddModuleUrlAlias(const csp::common::String& moduleUrl, const csp::common::String& moduleUrlAlias)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "AddModuleUrlAlias: %s-%s\n", moduleUrl.c_str(), moduleUrlAlias.c_str());

    UrlAliasMap::iterator it = UrlAliases.find(moduleUrl.c_str());

    if (it == UrlAliases.end())
    {
        UrlAliases.insert(UrlAliasMap::value_type(moduleUrl.c_str(), moduleUrlAlias.c_str()));
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Module alias %s-%s already exists\n", moduleUrl.c_str(), moduleUrlAlias.c_str());
    }
}

bool ScriptRuntime::GetModuleUrlAlias(const csp::common::String& moduleUrl, csp::common::String& outModuleUrlAlias)
{
    bool foundAlias = false;

    UrlAliasMap::iterator it = UrlAliases.find(moduleUrl.c_str());

    if (it == UrlAliases.end())
    {
        outModuleUrlAlias = moduleUrl;
    }
    else
    {
        outModuleUrlAlias = it->second.c_str();
        foundAlias = true;
    }

    return foundAlias;
}

void ScriptRuntime::ClearModuleSource(csp::common::String moduleUrl) { Modules.erase(moduleUrl.c_str()); }

csp::common::String ScriptRuntime::GetModuleSource(csp::common::String moduleUrl)
{
    ModuleSourceMap::const_iterator moduleIt = Modules.find(moduleUrl.c_str());

    if (moduleIt != Modules.end())
    {
        return csp::common::String(moduleIt->second.c_str());
    }
#if defined(CSP_SCRIPT_ALLOW_FILE_MODULES)
    else
    {
        std::optional<std::string> ModuleSource = ReadScriptModuleFile(moduleUrl.c_str());

        if (ModuleSource.has_value())
        {
            return csp::common::String(ModuleSource->c_str());
        }
    }
#endif

    return csp::common::String();
}

} // namespace csp::systems
