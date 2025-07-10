/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Common/String.h"

namespace csp::common
{

class IScriptBinding;

/**
 * @class IScriptRunner
 * @brief Interface abstraction of an entity capable of running a script.
 *
 * This type enforces an interface allowing a call into the CSP script system.
 * The motivation of this type was initially architectural, representing a dependency break
 * between the existing ScriptSystem and other modules that need to invoke script behaviour,
 * (primarily for what is becoming the RealtimeEngine module).
 * For that reason, there are some interface quirks, particularly the script context ID
 * being specifically relevant to the context provided by the current CSP Script System.
 *
 * CSP's ScriptSystem fulfils this interface, you may pass it into any methods that
 * require IJSScriptRunner.
 *
 * @important This type is not a true interface, instead having throwing default
 * implementations. This is due to wrapper generator constraints, hopefully temporary ones.
 */

// We do not want a dependency on the STL just to do this generator-hack-workaround. Use a custom type.
class CSP_API InvalidInterfaceUseError
{
public:
    csp::common::String msg;
    InvalidInterfaceUseError(const csp::common::String& msg)
        : msg(msg)
    {
    }
};

class CSP_API IJSScriptRunner
{
public:
    /// @brief Virtual destructor.
    virtual ~IJSScriptRunner() = default;

    /**
     * @brief Attempts to execute a script in a given context.
     * @param ContextId int64_t : The Id of the CSP script context in which to run the script.
     *                  If the provided context does not exist, the script run will fail.
     * @param ScriptText String& : The text of the script to be executed by the javascript engine.
     * @return Whether the script was successfully run.
     */
    virtual bool RunScript(int64_t ContextId, const String& ScriptText) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /**
     * @brief Register a binding object with the script runner. The script runner should store this for use.
     * @param ScriptBinding IScriptBinding* : Object capable of binding a script. The binding is eventually used to call back into IJSScriptRunner.
     */
    CSP_NO_EXPORT virtual void RegisterScriptBinding(IScriptBinding* ScriptBinding)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /**
     * @brief Unregister a binding object with the script runner.
     * @param ScriptBinding IScriptBinding : Object capable of binding a script.
     * @pre ScriptBinding has been registered via RegisterScriptBinding
     */
    CSP_NO_EXPORT virtual void UnregisterScriptBinding(IScriptBinding* ScriptBinding)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /**
     * @brief Perform the script binding on any bindings registered via RegisterScriptBinding.
     * @param ContextId int64_t : The Id of the CSP script context in which to run the script.
     *                  If the provided context does not exist, the script bind will fail.
     * @return Whether the context was successfully bound.
     */
    CSP_NO_EXPORT virtual bool BindContext(int64_t ContextId) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /**
     * @brief Reset the script context. This will likely shutdown and re-initialize any modules in the context.
     * @param ContextId int64_t : The Id of the CSP script context in which to run the script.
     *                  If the provided context does not exist, the script reset will fail.
     * @return Whether the context was successfully reset.
     */
    CSP_NO_EXPORT virtual bool ResetContext(int64_t ContextId) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /**
     * @brief Get the script context object.
     * @param ContextId int64_t : The Id of the CSP script context in which to run the script.
     * @return The script context object. This specific type of this is implementation defined. However, if using CSP's ScriptSystem at time of
     * writing, it will be a csp::systems::ScriptContext*, which you should cast to. Returns nullptr if the provided context does not exist.
     */
    CSP_NO_EXPORT virtual void* GetContext(int64_t ContextId) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /**
     * @brief Get a script module object within a context.
     * @param ContextId int64_t : The Id of the CSP script context in which to run the script.
     * @param ModuleName String& : Name of the module to return.
     * @return The script module. This specific type of this is implementation defined. However, if using CSP's ScriptSystem at time of
     * writing, it will be a csp::systems::ScriptModule*, which you should cast to. Returns nullptr if the specified module does not exist in the
     * context.
     */
    CSP_NO_EXPORT virtual void* GetModule(int64_t ContextId, const String& ModuleName)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /**
     * @brief Create a new context with specified ID.
     * @param ContextId int64_t : The Id of the context to create, must be unique to other contexts.
     * @return Whether the context was created successfully.
     */
    CSP_NO_EXPORT virtual bool CreateContext(int64_t ContextId) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /**
     * @brief Destroy a pre-existing context with specified ID.
     * @param ContextId int64_t : The Id of the context to destroy.
     * @return Whether the context was destroyed successfully.
     */
    CSP_NO_EXPORT virtual bool DestroyContext(int64_t ContextId) { throw InvalidInterfaceUseError("Illegal use of \"abstract\" type."); }

    /**
     * @brief Set the javascript source code of a particular module
     * @param ModuleUrl String : The URL of the module. This is an arbitrary accessor. It may be a good idea to use something like the scenegraph
     * path of an entity when setting up a module on any particular entity.
     * @param Source String: The javascript source code to set.
     * @pre ModuleURL must be unique.
     */
    CSP_NO_EXPORT virtual void SetModuleSource(csp::common::String ModuleUrl, csp::common::String Source)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

    /**
     * @brief Clear the javascript source code of a particular module, leaving it empty.
     * @param ModuleUrl String : The URL of the module to clear.
     * @pre ModuleURL must already exist via setting it with SetModuleSource.
     */
    CSP_NO_EXPORT virtual void ClearModuleSource(csp::common::String ModuleUrl)
    {
        throw InvalidInterfaceUseError("Illegal use of \"abstract\" type.");
    }

protected:
    IJSScriptRunner() = default;
};
}
