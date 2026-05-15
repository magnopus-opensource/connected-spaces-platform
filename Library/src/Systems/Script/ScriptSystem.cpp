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
#include "CSP/Systems/Script/ScriptSystem.h"

#include "CSP/CSPFoundation.h"
#include "Debug/Logging.h"
#include "Multiplayer/Script/ScriptHelpers.h"
#include "Systems/Script/ScriptContext.h"
#include "Systems/Script/ScriptRuntime.h"
#include "quickjspp.hpp"

// Enable optional 'os' and 'std' modules for file access etc
// #define SCRIPTS_INCLUDE_STD_LIBS

#if defined(SCRIPTS_INCLUDE_STD_LIBS)
#include "quickjs-libc.h"
#endif

#include <map>
#include <sstream>

// Template specializations for some custom csp types we want to use
template <> struct qjs::js_property_traits<csp::common::String>
{
    static void set_property(JSContext* ctx, JSValue thisObj, csp::common::String str, JSValue value)
    {
        int err = JS_SetPropertyStr(ctx, thisObj, str.c_str(), value);

        if (err < 0)
            throw exception { ctx };
    }

    static JSValue get_property(JSContext* ctx, JSValue thisObj, csp::common::String str) noexcept
    {
        return JS_GetPropertyStr(ctx, thisObj, str.c_str());
    }
};

template <> struct qjs::js_traits<csp::common::String>
{
    static csp::common::String unwrap(JSContext* ctx, JSValueConst v)
    {
        size_t plen;
        const char* ptr = JS_ToCStringLen(ctx, &plen, v);

        if (!ptr)
            throw exception { ctx };

        return csp::common::String(ptr, plen);
    }

    static JSValue wrap(JSContext* ctx, csp::common::String str) noexcept { return JS_NewStringLen(ctx, str.c_str(), str.Length()); }
};

namespace csp::systems
{

std::shared_ptr<ScriptSystem> ScriptSystem::MakeInitialised()
{
    auto instance = std::shared_ptr<ScriptSystem>(new ScriptSystem(), [](ScriptSystem* ptr) {
        delete ptr;
    });

    instance->Initialise();

    return instance;
}

ScriptSystem::ScriptSystem()
    : csp::common::IJSScriptRunner()
    , m_theScriptRuntime(nullptr)
{
}

ScriptSystem::~ScriptSystem() { Shutdown(); }

void ScriptSystem::Initialise()
{
    if (m_theScriptRuntime != nullptr)
    {
        CSP_LOG_ERROR_MSG("ScriptSystem::Initialise already called\n");
        return;
    }

    m_theScriptRuntime = new ScriptRuntime(this);

#if defined(SCRIPTS_INCLUDE_STD_LIBS)
    js_std_init_handlers(TheScriptRuntime->Runtime->rt);
    JS_SetModuleLoaderFunc(TheScriptRuntime->Runtime->rt, nullptr, js_module_loader, nullptr);
    js_std_add_helpers(TheScriptRuntime->Context->ctx, 0, nullptr);

    js_init_module_std(TheScriptRuntime->Context->ctx, "std");
    js_init_module_os(TheScriptRuntime->Context->ctx, "os");
#else
// @todo WASM build may need a custom module loader
//	JS_SetModuleLoaderFunc(TheScriptRuntime->Runtime->rt, nullptr, js_module_loader, nullptr);
#endif

    // Define a module name alias to be used when importing a module by name in a script
    AddModuleUrlAlias(OLD_SCRIPT_NAMESPACE, SCRIPT_NAMESPACE);
}

void ScriptSystem::Shutdown()
{
    if (m_theScriptRuntime != nullptr)
    {
        delete (m_theScriptRuntime);
        m_theScriptRuntime = nullptr;
    }
}

bool ScriptSystem::RunScript(int64_t contextId, const csp::common::String& scriptText)
{
    // CSP_LOG_FORMAT(LogLevel::Verbose, "RunScript: %s\n", ScriptText.c_str());

    ScriptContext* theScriptContext = m_theScriptRuntime->GetContext(contextId);
    if (theScriptContext == nullptr)
    {
        return false;
    }

    qjs::Value result = theScriptContext->m_context->eval(scriptText.c_str(), "<eval>", JS_EVAL_TYPE_MODULE);
    bool hasErrors = result.isException();
    return !hasErrors;
}

bool ScriptSystem::RunScriptFile(int64_t contextId, const csp::common::String& scriptFilePath)
{
    CSP_LOG_FORMAT(common::LogLevel::Verbose, "RunScriptFile: %s\n", scriptFilePath.c_str());

    ScriptContext* theScriptContext = m_theScriptRuntime->GetContext(contextId);
    if (theScriptContext == nullptr)
    {
        return false;
    }

    qjs::Value result = theScriptContext->m_context->evalFile(scriptFilePath.c_str(), JS_EVAL_TYPE_MODULE);
    bool hasErrors = result.isException();
    return !hasErrors;
}

bool ScriptSystem::CreateContext(int64_t contextId) { return m_theScriptRuntime->AddContext(contextId); }

bool ScriptSystem::DestroyContext(int64_t contextId) { return m_theScriptRuntime->RemoveContext(contextId); }

bool ScriptSystem::BindContext(int64_t contextId) { return m_theScriptRuntime->BindContext(contextId); }

bool ScriptSystem::ResetContext(int64_t contextId) { return m_theScriptRuntime->ResetContext(contextId); }

bool ScriptSystem::ExistsInContext(int64_t contextId, const csp::common::String& objectName)
{
    return m_theScriptRuntime->ExistsInContext(contextId, objectName);
}

void* ScriptSystem::GetContext(int64_t contextId) { return (void*)m_theScriptRuntime->GetContext(contextId)->m_context; }

void* ScriptSystem::GetModule(int64_t contextId, const csp::common::String& moduleName)
{
    ScriptContext* theScriptContext = m_theScriptRuntime->GetContext(contextId);
    return (void*)theScriptContext->GetModule(moduleName)->Module;
}

void ScriptSystem::RegisterScriptBinding(csp::common::IScriptBinding* scriptBinding) { m_theScriptRuntime->RegisterScriptBinding(scriptBinding); }

void ScriptSystem::UnregisterScriptBinding(csp::common::IScriptBinding* scriptBinding) { m_theScriptRuntime->UnregisterScriptBinding(scriptBinding); }

void ScriptSystem::SetModuleSource(csp::common::String moduleUrl, csp::common::String source)
{
    m_theScriptRuntime->SetModuleSource(moduleUrl, source);
}

void ScriptSystem::AddModuleUrlAlias(const csp::common::String& moduleUrl, const csp::common::String& moduleUrlAlias)
{
    m_theScriptRuntime->AddModuleUrlAlias(moduleUrl, moduleUrlAlias);
}

void ScriptSystem::ClearModuleSource(csp::common::String moduleUrl) { m_theScriptRuntime->ClearModuleSource(moduleUrl); }

csp::common::String ScriptSystem::GetModuleSource(csp::common::String moduleUrl) { return m_theScriptRuntime->GetModuleSource(moduleUrl); }

bool ScriptSystem::GetModuleUrlAlias(const csp::common::String& moduleUrl, csp::common::String& outModuleUrlAlias)
{
    return m_theScriptRuntime->GetModuleUrlAlias(moduleUrl, outModuleUrlAlias);
}

size_t ScriptSystem::GetNumImportedModules(int64_t contextId) const { return m_theScriptRuntime->GetContext(contextId)->GetNumImportedModules(); }

const char* ScriptSystem::GetImportedModule(int64_t contextId, size_t index) const
{
    return m_theScriptRuntime->GetContext(contextId)->GetImportedModule(index);
}

} // namespace csp::systems
