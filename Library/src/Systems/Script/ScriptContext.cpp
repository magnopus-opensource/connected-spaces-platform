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
#include "Systems/Script/ScriptContext.h"

#include "CSP/Systems/Script/ScriptSystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Memory/MemoryManager.h"

namespace csp::systems
{

ScriptContext::ScriptContext(ScriptSystem* InScriptSystem, qjs::Runtime* InRuntime, uint64_t InContextId)
    : ContextId(InContextId)
    , TheScriptSystem(InScriptSystem)
    , Runtime(InRuntime)
{
    Initialise();
}

ScriptContext::~ScriptContext() { Shutdown(); }

void ScriptContext::Initialise()
{
    Context = CSP_NEW qjs::Context(*Runtime);

    Context->moduleLoader = [this](std::string_view filename) -> qjs::Context::ModuleData
    {
        csp::common::String Url = csp::common::String(filename.data(), filename.length());

        csp::common::String Alias;
        if (TheScriptSystem->GetModuleUrlAlias(Url, Alias))
        {
            return qjs::Context::ModuleData { std::nullopt, std::nullopt, Alias.c_str() };
        }

        csp::common::String Source = TheScriptSystem->GetModuleSource(Url);

        AddImport(Url);

        qjs::Context::ModuleData Data;

        if (Source.IsEmpty())
        {
            CSP_LOG_ERROR_FORMAT("Module %s not found\n", Url.c_str());
            return qjs::Context::ModuleData { std::nullopt, std::nullopt, std::nullopt };
        }

        CSP_LOG_FORMAT(LogLevel::Log, "Loaded Module: %s\n", Url.c_str());

        return qjs::Context::ModuleData { Url.c_str(), Source.c_str(), std::nullopt };
    };
}

void ScriptContext::Shutdown()
{
    for (auto Module : Modules)
    {
        CSP_DELETE(Module.second);
    }

    Modules.clear();
    Imports.clear();

    CSP_DELETE(Context);
}

ScriptModule* ScriptContext::GetModule(const csp::common::String& ModuleName)
{
    ModuleMap::iterator It = Modules.find(ModuleName.c_str());

    if (It != Modules.end())
    {
        return It->second;
    }

    AddModule(ModuleName);
    return GetModule(ModuleName);
}

void ScriptContext::AddModule(const csp::common::String& ModuleName)
{
    CSP_LOG_FORMAT(LogLevel::Log, "AddModule: %s\n", ModuleName.c_str());

    ModuleMap::iterator It = Modules.find(ModuleName.c_str());

    if (It == Modules.end())
    {
        ScriptModule* TheScriptModule = CSP_NEW ScriptModule();
        TheScriptModule->ModuleName = ModuleName;
        TheScriptModule->Module = &Context->addModule(ModuleName.c_str());
        Modules.insert(ModuleMap::value_type(ModuleName.c_str(), TheScriptModule));
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Module %s already exists\n", ModuleName.c_str());
    }
}

uint64_t ScriptContext::GetId() const { return ContextId; }

bool ScriptContext::ExistsInContext(const csp::common::String& ObjectName)
{
    qjs::Value Result = Context->eval(ObjectName.c_str());
    bool isExcept = Result.isException();
    return !isExcept;
}

void ScriptContext::AddImport(const csp::common::String& Url)
{
    bool Exists = false;

    for (const auto& Import : Imports)
    {
        if (Import == Url.c_str())
        {
            Exists = true;
            break;
        }
    }

    if (Exists)
    {
        Imports.push_back(Url.c_str());
    }
}

size_t ScriptContext::GetNumImportedModules() const { return Imports.size(); }

const char* ScriptContext::GetImportedModule(size_t Index) const
{
    if (Index >= Imports.size())
    {
        return nullptr;
    }

    return Imports[Index].c_str();
}

void ScriptContext::Reset()
{
    // Re-initialise the context ready for new or updated script source
    Shutdown();
    Initialise();
}

} // namespace csp::systems
