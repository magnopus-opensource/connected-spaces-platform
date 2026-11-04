/*
 * Copyright 2026 Magnopus LLC

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

#include "quickjs.h"
#include "CSP/Common/String.h"

#include <string>

namespace csp::systems::ScriptErrorUtils
{

std::string ToStdString(JSContext* Ctx, JSValueConst Val);

// Utility function to safely read a property without throwing. Returns undefined if it is missing or the read itself throws
// (e.g. reading a property of a thrown null).
// Safely clears any exception thrown from the read.
JSValue GetPropertySafe(JSContext* Ctx, JSValueConst Obj, const char* Name);

// Utility function to safely extract the lineNumber property.
// Syntax errors carry a separate lineNumber property whereas runtime errors only have the line captured in the stack trace,
// eg. "at <anonymous> (MyScript:7)".
// For runtime errors extract the lineNumber from the first stack frame.
int32_t GetErrorLineNumber(JSContext* Ctx, JSValueConst Exception, const std::string& Stack, const std::string& FileName);

// Utility function to safely construct a one line log from the exception QuickJS has thrown.
// Operations to extract elements such as the 'message' and 'stack' can throw exceptions. The JSRuntime exception slot
// is shared by every context which could result in another script error reporting an old exception instead of its own.
// This function will ensure pending exceptions are safely discarded.
// 
// Example formatted error:
// Script error in entity "ScriptHandlerErrorEntity" (line 7): TypeError: not a function
//    at <anonymous> (Script_ScriptHandlerErrorEntity:7)
//    at <anonymous> (Script_ScriptHandlerErrorEntity:1)
csp::common::String FormatScriptError(JSContext* Ctx, JSValueConst Exception, const std::string& ScriptName, const std::string& FileName);

} // namespace csp::systems::ScriptErrorUtils
