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

namespace csp::systems
{

ScriptContext::ScriptContext(ScriptSystem* inScriptSystem, qjs::Runtime* inRuntime, uint64_t inContextId)
    : m_contextId(inContextId)
    , m_theScriptSystem(inScriptSystem)
    , m_runtime(inRuntime)
{
    Initialise();
}

ScriptContext::~ScriptContext() { Shutdown(); }

void ScriptContext::Initialise()
{
    m_context = new qjs::Context(*m_runtime);

    m_context->moduleLoader = [this](std::string_view filename) -> qjs::Context::ModuleData
    {
        csp::common::String url = csp::common::String(filename.data(), filename.length());

        csp::common::String alias;
        if (m_theScriptSystem->GetModuleUrlAlias(url, alias))
        {
            return qjs::Context::ModuleData { std::nullopt, std::nullopt, alias.c_str() };
        }

        csp::common::String source = m_theScriptSystem->GetModuleSource(url);

        AddImport(url);

        qjs::Context::ModuleData data;

        if (source.IsEmpty())
        {
            CSP_LOG_ERROR_FORMAT("Module %s not found\n", url.c_str());
            return qjs::Context::ModuleData { std::nullopt, std::nullopt, std::nullopt };
        }

        CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Loaded Module: %s\n", url.c_str());

        return qjs::Context::ModuleData { url.c_str(), source.c_str(), std::nullopt };
    };
}

void ScriptContext::Shutdown()
{
    for (auto module : m_modules)
    {
        delete (module.second);
    }

    m_modules.clear();
    m_imports.clear();

    delete (m_context);
}

ScriptModule* ScriptContext::GetModule(const csp::common::String& moduleName)
{
    ModuleMap::iterator it = m_modules.find(moduleName.c_str());

    if (it != m_modules.end())
    {
        return it->second;
    }

    AddModule(moduleName);
    return GetModule(moduleName);
}

void ScriptContext::AddModule(const csp::common::String& moduleName)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "AddModule: %s\n", moduleName.c_str());

    ModuleMap::iterator it = m_modules.find(moduleName.c_str());

    if (it == m_modules.end())
    {
        ScriptModule* theScriptModule = new ScriptModule();
        theScriptModule->ModuleName = moduleName;
        theScriptModule->Module = &m_context->addModule(moduleName.c_str());
        m_modules.insert(ModuleMap::value_type(moduleName.c_str(), theScriptModule));
    }
    else
    {
        CSP_LOG_ERROR_FORMAT("Module %s already exists\n", moduleName.c_str());
    }
}

uint64_t ScriptContext::GetId() const { return m_contextId; }

bool ScriptContext::ExistsInContext(const csp::common::String& objectName)
{
    qjs::Value result = m_context->eval(objectName.c_str());
    bool isExcept = result.isException();
    return !isExcept;
}

void ScriptContext::AddImport(const csp::common::String& url)
{
    bool exists = false;

    for (const auto& import : m_imports)
    {
        if (import == url.c_str())
        {
            exists = true;
            break;
        }
    }

    if (exists)
    {
        m_imports.push_back(url.c_str());
    }
}

size_t ScriptContext::GetNumImportedModules() const { return m_imports.size(); }

const char* ScriptContext::GetImportedModule(size_t index) const
{
    if (index >= m_imports.size())
    {
        return nullptr;
    }

    return m_imports[index].c_str();
}

void ScriptContext::Reset()
{
    // Re-initialise the context ready for new or updated script source
    Shutdown();
    Initialise();
}

} // namespace csp::systems
