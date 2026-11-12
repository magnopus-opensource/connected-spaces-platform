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
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/Interfaces/IScriptBinding.h"
#include "CSP/Common/String.h"

#include <functional>
#include <string>
#include <vector>

#include <memory>

namespace csp::common
{
class LogSystem;
}

namespace csp::systems
{
/// @brief Details of an error encountered when running a script.
class CSP_API ScriptErrorData
{
public:
    /// @brief The id of the script context the error occurred in. For entity scripts, this is the id of the owning entity.
    int64_t ContextId = 0;

    /// @brief The name of the script that threw the error. For entity scripts this is the entity name, and for scripts run from a file it is
    /// the file name.
    csp::common::String ScriptName;

    /// @brief The formatted error, including the script name, line number (where known), error text and stack trace.
    /// This is the same text that is written to the log.
    csp::common::String ErrorMessage;
};

/// @brief Callback invoked when a script encounters an error.
typedef std::function<void(const ScriptErrorData& ErrorData)> ScriptErrorCallbackHandler;

/// @brief A JavaScript based scripting system that can be used to create advanced behaviours and interactions between entities in spaces.
class CSP_API ScriptSystem : public csp::common::IJSScriptRunner
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend class ScriptContext;
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Starts up the JavaScript runtime context.
    void Initialise();
    /// @brief Shuts down and deletes the JavaScript runtime context.
    void Shutdown();

    /// @brief A factory function to construct and initialise in a single step.
    ///
    /// At the CSP library level, all access to our `System` classes is through
    /// the `SystemsManager` singleton, which creates and ultimately owns them.
    /// As a general rule, there is only a single instance of each `System` in
    /// the library, all centralised via that manager. As such, currently there
    /// is a pattern to enforce that only the `SystemsManager` can instantiate
    /// each `System` class via private constructors and friendship.
    ///
    /// However, this isn't strictly necessary, and makes testing in isolation difficult.
    /// So, expose a public factory function which at the moment is mainly for
    /// use in tests, but could just as well be used to instantiate the instance
    /// in the `SystemsManager`.
    CSP_NO_EXPORT static std::shared_ptr<ScriptSystem> MakeInitialised(csp::common::LogSystem& LogSystem);

    /// @brief Attempts to execute a script in a given context.
    /// @param ContextId : The context in which to run the script. If the provided context does not exist, the script run will fail.
    /// @param ScriptText : The script to execute.
    /// @return a boolean representing success running the script.
    bool RunScript(int64_t ContextId, const csp::common::String& ScriptText) override;

    /// @brief Attempts to execute a script from a given file path in the given context.
    /// @param ContextId  : The context in which to run the script. If the provided context does not exist, the script run will fail.
    /// @param ScriptFilePath  : The file path of the script to execute.
    /// @return a boolean representing success running the script.
    bool RunScriptFile(int64_t ContextId, const csp::common::String& ScriptFilePath);

    /// @brief Sets a callback to be executed when a script error occurs.
    /// @param Callback csp::systems::ScriptErrorCallbackHandler : the callback to execute when a script error occurs.
    /// This will include a ScriptErrorData struct with the following data:
    /// - ContextId
    /// - ScriptName
    /// - ErrorMessage
    CSP_EVENT void SetScriptErrorCallback(ScriptErrorCallbackHandler Callback);

    // Experimental binding interface (not exposed to wrappergen)
    CSP_START_IGNORE
    bool RunScriptWithErrorLog(int64_t ContextId, const csp::common::String& ScriptText, csp::common::String& OutError) override;
    bool CreateContext(int64_t ContextId, ScriptNameProvider ScriptNameProvider) override;
    bool DestroyContext(int64_t ContextId) override;
    bool BindContext(int64_t ContextId) override;
    bool ResetContext(int64_t ContextId) override;
    bool ExistsInContext(int64_t ContextId, const csp::common::String& ObjectName);
    void* GetContext(int64_t ContextId) override;
    void* GetModule(int64_t ContextId, const csp::common::String& ModuleName) override;
    void RegisterScriptBinding(csp::common::IScriptBinding* ScriptBinding) override;
    void UnregisterScriptBinding(csp::common::IScriptBinding* ScriptBinding) override;
    void SetModuleSource(csp::common::String ModuleUrl, csp::common::String Source) override;
    void AddModuleUrlAlias(const csp::common::String& ModuleUrl, const csp::common::String& ModuleUrlAlias);
    bool GetModuleUrlAlias(const csp::common::String& ModuleUrl, csp::common::String& OutModuleUrlAlias);
    void ClearModuleSource(csp::common::String ModuleUrl) override;
    csp::common::String GetModuleSource(csp::common::String ModuleUrl);
    size_t GetNumImportedModules(int64_t ContextId) const;
    const char* GetImportedModule(int64_t ContextId, size_t Index) const;
    CSP_END_IGNORE

private:
    ScriptSystem() = delete;
    ScriptSystem(csp::common::LogSystem& LogSystem);
    ~ScriptSystem();

    class ScriptRuntime* TheScriptRuntime;
    ScriptErrorCallbackHandler ScriptErrorCallback;

    csp::common::LogSystem& LogSystem;
};

} // namespace csp::systems
