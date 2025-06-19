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

#include "CSP/Common/String.h"

namespace csp::common
{

/**
 * @class IScriptRunner
 * @brief Interface abstraction of an entity capable of running a script.
 *
 * This type enforces an interface allowing a call into the CSP script system.
 * The motivation of this type was initially architectural, representing a dependency break
 * between the existing ScriptSystem and other modules that need to invoke script behaviour.
 * For that reason, there are some interface quirks, particularly the script context ID
 * being specifically relevant to the context provided by the current CSP Script System.
 *
 * CSP's ScriptSystem fulfils this interface, you may pass it into any methods that
 * require IJSScriptRunner.
 */
class CSP_API IJSScriptRunner
{
public:
    /// @brief Virtual destructor.
    virtual ~IJSScriptRunner() = default;

    /**
     * @brief Attempts to execute a script in a given context.
     * @param ContextId The Id of the CSP script context in which to run the script.
     *                  If the provided context does not exist, the script run will fail.
     * @param ScriptText The text of the script to be executed by the javascript engine.
     * @return A boolean representing success running the script.
     */
    virtual bool RunScript(int64_t ContextId, const csp::common::String& ScriptText) = 0;
};
}
