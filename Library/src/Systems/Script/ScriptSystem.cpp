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
#include "Systems/Script/ScriptErrorUtils.h"
#include "Systems/Script/ScriptRuntime.h"
#include "quickjspp.hpp"

// Enable optional 'os' and 'std' modules for file access etc
// #define SCRIPTS_INCLUDE_STD_LIBS

#if defined(SCRIPTS_INCLUDE_STD_LIBS)
#include "quickjs-libc.h"
#endif

#include <filesystem>
#include <fmt/format.h>
#include <map>
#include <sstream>

namespace
{

// Takes the exception from the shared slot on the JSRuntime (rt->current_exception), formats it into a one line error log and clears it.
// qjs::Value frees the exception on destruction.
csp::common::String ConsumeAndLogScriptException(csp::common::LogSystem& LogSystem, qjs::Context& Context, const std::string& ScriptName, const std::string& FileName)
{
    qjs::Value Exception = Context.getException();

    // Utility function to safely construct a one line log from the exception QuickJS has thrown.
    // Operations to extract elements such as the 'message' and 'stack' can throw exceptions so this fuction ensures they are safely discarded.
    const csp::common::String ErrorText = csp::systems::ScriptErrorUtils::FormatScriptError(Context.ctx, Exception.v, ScriptName, FileName);
    LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorText.c_str());

    return ErrorText;
}

} // namespace

// Template specializations for some custom csp types we want to use
template <> struct qjs::js_property_traits<csp::common::String>
{
    static void set_property(JSContext* ctx, JSValue this_obj, csp::common::String str, JSValue value)
    {
        int err = JS_SetPropertyStr(ctx, this_obj, str.c_str(), value);

        if (err < 0)
            throw exception { ctx };
    }

    static JSValue get_property(JSContext* ctx, JSValue this_obj, csp::common::String str) noexcept
    {
        return JS_GetPropertyStr(ctx, this_obj, str.c_str());
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

std::shared_ptr<ScriptSystem> ScriptSystem::MakeInitialised(csp::common::LogSystem& LogSystem)
{
    auto Instance = std::shared_ptr<ScriptSystem>(new ScriptSystem(LogSystem), [](ScriptSystem* Ptr) { delete Ptr; });

    Instance->Initialise();

    return Instance;
}

ScriptSystem::ScriptSystem(csp::common::LogSystem& LogSystem)
    : csp::common::IJSScriptRunner()
    , TheScriptRuntime(nullptr)
    , LogSystem(LogSystem)
{
}

ScriptSystem::~ScriptSystem() { Shutdown(); }

void ScriptSystem::Initialise()
{
    if (TheScriptRuntime != nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "ScriptSystem::Initialise already called\n");
        return;
    }

    TheScriptRuntime = new ScriptRuntime(this);

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
    if (TheScriptRuntime != nullptr)
    {
        delete (TheScriptRuntime);
        TheScriptRuntime = nullptr;
    }
}

bool ScriptSystem::RunScript(int64_t ContextId, const csp::common::String& ScriptText)
{
    csp::common::String Error;

    return RunScriptWithErrorLog(ContextId, ScriptText, Error);
}

bool ScriptSystem::RunScriptWithErrorLog(int64_t ContextId, const csp::common::String& ScriptText, csp::common::String& OutError)
{
    ScriptContext* TheScriptContext = TheScriptRuntime->GetContext(ContextId);
    if (TheScriptContext == nullptr)
    {
        OutError = fmt::format("Unable to run script: no script context exists with id {}", ContextId).c_str();
        LogSystem.LogMsg(csp::common::LogLevel::Error, OutError);

        return false;
    }

    csp::common::String ScriptName = TheScriptContext->ScriptNameProvider();
    if (ScriptName.IsEmpty())
    {
        ScriptName = "<unknown script>";
    }

    csp::common::String FileName = "Script_" + ScriptName;

    qjs::Value Result = TheScriptContext->Context->eval(ScriptText.c_str(), FileName.c_str(), JS_EVAL_TYPE_MODULE);
    if (Result.isException())
    {
        OutError = ConsumeAndLogScriptException(LogSystem, *TheScriptContext->Context, ScriptName.c_str(), FileName.c_str());

        if (ScriptErrorCallback)
        {
            ScriptErrorCallback(ScriptErrorData { ContextId, ScriptName.c_str(), OutError });
        }

        return false;
    }

    return true;
}

bool ScriptSystem::RunScriptFile(int64_t ContextId, const csp::common::String& ScriptFilePath)
{
    LogSystem.LogMsg(csp::common::LogLevel::Verbose,
        fmt::format("RunScriptFile: {}\n", ScriptFilePath.c_str()).c_str());

    ScriptContext* TheScriptContext = TheScriptRuntime->GetContext(ContextId);
    if (TheScriptContext == nullptr)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("Unable to run script file {0}: no script context exists with id {1}", ScriptFilePath.c_str(), ContextId)
                .c_str());

        return false;
    }

    // evalFile uses the path as the eval filename, so stack frames reference the full path.
    // If an exception is thrown, ConsumeAndLogScriptException() -> FormatScriptError() below will try to extract the linenumber from the exception
    // which will require the full path.
    const std::string FileName = ScriptFilePath.c_str();
    const std::string ScriptName = std::filesystem::path(FileName).filename().string();

    qjs::Value Result = TheScriptContext->Context->evalFile(ScriptFilePath.c_str(), JS_EVAL_TYPE_MODULE);
    if (Result.isException())
    {
        csp::common::String OutError = ConsumeAndLogScriptException(LogSystem, *TheScriptContext->Context, ScriptName, FileName);

        if (ScriptErrorCallback)
        {
            ScriptErrorCallback(ScriptErrorData { ContextId, ScriptName.c_str(), OutError });
        }

        return false;
    }

    return true;
}

void ScriptSystem::SetScriptErrorCallback(ScriptErrorCallbackHandler Callback)
{
    if (ScriptErrorCallback)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "ScriptErrorCallback has already been set.Previous callback overwritten.");
    }

    ScriptErrorCallback = std::move(Callback);
}

bool ScriptSystem::CreateContext(int64_t ContextId, ScriptNameProvider ScriptNameProvider)
{
    return TheScriptRuntime->AddContext(ContextId, ScriptNameProvider);
}

bool ScriptSystem::DestroyContext(int64_t ContextId) { return TheScriptRuntime->RemoveContext(ContextId); }

bool ScriptSystem::BindContext(int64_t ContextId) { return TheScriptRuntime->BindContext(ContextId); }

bool ScriptSystem::ResetContext(int64_t ContextId) { return TheScriptRuntime->ResetContext(ContextId); }

bool ScriptSystem::ExistsInContext(int64_t ContextId, const csp::common::String& ObjectName)
{
    return TheScriptRuntime->ExistsInContext(ContextId, ObjectName);
}

void* ScriptSystem::GetContext(int64_t ContextId) { return (void*)TheScriptRuntime->GetContext(ContextId)->Context; }

void* ScriptSystem::GetModule(int64_t ContextId, const csp::common::String& ModuleName)
{
    ScriptContext* TheScriptContext = TheScriptRuntime->GetContext(ContextId);
    return (void*)TheScriptContext->GetModule(ModuleName)->Module;
}

void ScriptSystem::RegisterScriptBinding(csp::common::IScriptBinding* ScriptBinding) { TheScriptRuntime->RegisterScriptBinding(ScriptBinding); }

void ScriptSystem::UnregisterScriptBinding(csp::common::IScriptBinding* ScriptBinding) { TheScriptRuntime->UnregisterScriptBinding(ScriptBinding); }

void ScriptSystem::SetModuleSource(csp::common::String ModuleUrl, csp::common::String Source)
{
    TheScriptRuntime->SetModuleSource(ModuleUrl, Source);
}

void ScriptSystem::AddModuleUrlAlias(const csp::common::String& ModuleUrl, const csp::common::String& ModuleUrlAlias)
{
    TheScriptRuntime->AddModuleUrlAlias(ModuleUrl, ModuleUrlAlias);
}

void ScriptSystem::ClearModuleSource(csp::common::String ModuleUrl) { TheScriptRuntime->ClearModuleSource(ModuleUrl); }

csp::common::String ScriptSystem::GetModuleSource(csp::common::String ModuleUrl) { return TheScriptRuntime->GetModuleSource(ModuleUrl); }

bool ScriptSystem::GetModuleUrlAlias(const csp::common::String& ModuleUrl, csp::common::String& OutModuleUrlAlias)
{
    return TheScriptRuntime->GetModuleUrlAlias(ModuleUrl, OutModuleUrlAlias);
}

size_t ScriptSystem::GetNumImportedModules(int64_t ContextId) const { return TheScriptRuntime->GetContext(ContextId)->GetNumImportedModules(); }

const char* ScriptSystem::GetImportedModule(int64_t ContextId, size_t Index) const
{
    return TheScriptRuntime->GetContext(ContextId)->GetImportedModule(Index);
}

} // namespace csp::systems
