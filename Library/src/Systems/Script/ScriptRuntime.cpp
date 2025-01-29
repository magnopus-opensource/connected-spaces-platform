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
#include "Memory/Memory.h"
#include "Memory/MemoryManager.h"
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

ScriptRuntime::ScriptRuntime(ScriptSystem* InScriptSystem)
    : TheScriptSystem(InScriptSystem)
    , Runtime(CSP_NEW qjs::Runtime())
{
}

ScriptRuntime::~ScriptRuntime()
{
    for (auto Context : Contexts)
    {
        CSP_DELETE(Context.second);
    }

    CSP_DELETE(Runtime);
}

bool ScriptRuntime::AddContext(int64_t ContextId)
{
    ContextMap::iterator It = Contexts.find(ContextId);

    if (It == Contexts.end())
    {
        ScriptContext* TheScriptContext = CSP_NEW ScriptContext(TheScriptSystem, Runtime, ContextId);
        Contexts.insert(ContextMap::value_type(ContextId, TheScriptContext));
        return true;
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Context %lld already exists\n", ContextId);
    }

    return false;
}

bool ScriptRuntime::RemoveContext(int64_t ContextId)
{
    ContextMap::iterator It = Contexts.find(ContextId);

    if (It != Contexts.end())
    {
        CSP_DELETE(It->second);
        Contexts.erase(It);
        return true;
    }

    return false;
}

ScriptContext* ScriptRuntime::GetContext(int64_t ContextId)
{
    ContextMap::iterator It = Contexts.find(ContextId);

    if (It != Contexts.end())
    {
        return It->second;
    }

    return nullptr;
}

bool ScriptRuntime::BindContext(int64_t ContextId)
{
    ScriptContext* Context = GetContext(ContextId);
    if (Context)
    {
        BindContext(Context);
        return true;
    }

    return false;
}

bool ScriptRuntime::ResetContext(int64_t ContextId)
{
    ScriptContext* Context = GetContext(ContextId);
    if (Context)
    {
        ResetContext(Context);
        return true;
    }

    return false;
}

bool ScriptRuntime::ExistsInContext(int64_t ContextId, const csp::common::String& ObjectName)
{
    ScriptContext* Context = GetContext(ContextId);
    if (Context)
    {
        return Context->ExistsInContext(ObjectName);
    }

    return false;
}

void ScriptRuntime::RegisterScriptBinding(IScriptBinding* ScriptBinding) { Bindings.push_back(ScriptBinding); }

void ScriptRuntime::UnregisterScriptBinding(IScriptBinding* ScriptBinding) { Bindings.remove(ScriptBinding); }

void ScriptRuntime::BindContext(ScriptContext* Context)
{
    for (auto Binding : Bindings)
    {
        Binding->Bind(Context->GetId(), TheScriptSystem);
    }
}

void ScriptRuntime::ResetContext(ScriptContext* Context) { Context->Reset(); }

void ScriptRuntime::SetModuleSource(csp::common::String ModuleUrl, csp::common::String Source)
{
    CSP_LOG_FORMAT(LogLevel::Log, "ScriptRuntime::SetModuleSource %s\n", ModuleUrl.c_str());
    Modules[ModuleUrl.c_str()] = Source.c_str();
}

void ScriptRuntime::AddModuleUrlAlias(const csp::common::String& ModuleUrl, const csp::common::String& ModuleUrlAlias)
{
    CSP_LOG_FORMAT(LogLevel::Log, "AddModuleUrlAlias: %s-%s\n", ModuleUrl.c_str(), ModuleUrlAlias.c_str());

    UrlAliasMap::iterator It = UrlAliases.find(ModuleUrl.c_str());

    if (It == UrlAliases.end())
    {
        UrlAliases.insert(UrlAliasMap::value_type(ModuleUrl.c_str(), ModuleUrlAlias.c_str()));
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Module alias %s-%s already exists\n", ModuleUrl.c_str(), ModuleUrlAlias.c_str());
    }
}

bool ScriptRuntime::GetModuleUrlAlias(const csp::common::String& ModuleUrl, csp::common::String& OutModuleUrlAlias)
{
    bool FoundAlias = false;

    UrlAliasMap::iterator It = UrlAliases.find(ModuleUrl.c_str());

    if (It == UrlAliases.end())
    {
        OutModuleUrlAlias = ModuleUrl;
    }
    else
    {
        OutModuleUrlAlias = It->second.c_str();
        FoundAlias = true;
    }

    return FoundAlias;
}

void ScriptRuntime::ClearModuleSource(csp::common::String ModuleUrl) { Modules.erase(ModuleUrl.c_str()); }

csp::common::String ScriptRuntime::GetModuleSource(csp::common::String ModuleUrl)
{
    ModuleSourceMap::const_iterator ModuleIt = Modules.find(ModuleUrl.c_str());

    if (ModuleIt != Modules.end())
    {
        return csp::common::String(ModuleIt->second.c_str());
    }
#if defined(CSP_SCRIPT_ALLOW_FILE_MODULES)
    else
    {
        std::optional<std::string> ModuleSource = ReadScriptModuleFile(ModuleUrl.c_str());

        if (ModuleSource.has_value())
        {
            return csp::common::String(ModuleSource->c_str());
        }
    }
#endif

    return csp::common::String();
}

} // namespace csp::systems
