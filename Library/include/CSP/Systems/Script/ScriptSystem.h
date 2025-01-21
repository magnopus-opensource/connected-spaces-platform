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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"

#include <functional>
#include <string>
#include <vector>

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

class IScriptBinding
{
public:
    virtual ~IScriptBinding() = default;
    virtual void Bind(int64_t ContextId, class ScriptSystem* InScriptSystem) = 0;
};

/// @brief A JavaScript based scripting system that can be used to create advanced behaviours and interactions between entities in spaces.
class CSP_API ScriptSystem
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend class ScriptContext;
    friend void csp::memory::Delete<ScriptSystem>(ScriptSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Starts up the JavaScript runtime context.
    void Initialise();
    /// @brief Shuts down and deletes the JavaScript runtime context.
    void Shutdown();

    /// @brief Attempts to execute a script in a given context.
    /// @param ContextId : The context in which to run the script. If the provided context does not exist, the script run will fail.
    /// @param ScriptText : The script to execute.
    /// @return a boolean representing success running the script.
    bool RunScript(int64_t ContextId, const csp::common::String& ScriptText);
    /// @brief Attempts to execute a script from a given file path in the given context.
    /// @param ContextId  : The context in which to run the script. If the provided context does not exist, the script run will fail.
    /// @param ScriptFilePath  : The file path of the script to execute.
    /// @return a boolean representing success running the script.
    bool RunScriptFile(int64_t ContextId, const csp::common::String& ScriptFilePath);

    // Experimental binding interface (not exposed to wrappergen)
    CSP_START_IGNORE
    bool CreateContext(int64_t ContextId);
    bool DestroyContext(int64_t ContextId);
    bool BindContext(int64_t ContextId);
    bool ResetContext(int64_t ContextId);
    bool ExistsInContext(int64_t ContextId, const csp::common::String& ObjectName);
    void* GetContext(int64_t ContextId);
    void* GetModule(int64_t ContextId, const csp::common::String& ModuleName);
    void RegisterScriptBinding(IScriptBinding* ScriptBinding);
    void UnregisterScriptBinding(IScriptBinding* ScriptBinding);
    void SetModuleSource(csp::common::String ModuleUrl, csp::common::String Source);
    void AddModuleUrlAlias(const csp::common::String& ModuleUrl, const csp::common::String& ModuleUrlAlias);
    bool GetModuleUrlAlias(const csp::common::String& ModuleUrl, csp::common::String& OutModuleUrlAlias);
    void ClearModuleSource(csp::common::String ModuleUrl);
    csp::common::String GetModuleSource(csp::common::String ModuleUrl);
    size_t GetNumImportedModules(int64_t ContextId) const;
    const char* GetImportedModule(int64_t ContextId, size_t Index) const;
    CSP_END_IGNORE

private:
    ScriptSystem();
    ~ScriptSystem();

    class ScriptRuntime* TheScriptRuntime;
};

} // namespace csp::systems
