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
#include "Systems/Script/ScriptErrorUtils.h"

#include <charconv>
#include <fmt/format.h>

namespace csp::systems::ScriptErrorUtils
{

std::string ToStdString(JSContext* Ctx, JSValueConst Val)
{
    size_t Len = 0;
    // This call converts any JS value to a string.
    // This call can throw an exception, in which case JS_ToCStringLen returns nullptr and sets a new pending exception.
    // The JSRuntime can only hold a single exception per context (current_exception) and so must be cleared via the
    // call below to JS_GetException().
    const char* Str = JS_ToCStringLen(Ctx, &Len, Val);

    if (Str == nullptr)
    {
        JS_FreeValue(Ctx, JS_GetException(Ctx));
        return "<unprintable value>";
    }

    std::string Result(Str, Len);
    JS_FreeCString(Ctx, Str);
    return Result;
}

JSValue GetPropertySafe(JSContext* Ctx, JSValueConst Obj, const char* Name)
{
    // This call can throw an exception. If it does we must clear that from the JSRuntime which only holds a single exception per context
    // (current_exception) via the call to JS_GetException()
    JSValue Val = JS_GetPropertyStr(Ctx, Obj, Name);

    if (JS_IsException(Val))
    {
        JS_FreeValue(Ctx, JS_GetException(Ctx));
        return JS_UNDEFINED;
    }

    return Val;
}

int32_t GetErrorLineNumber(JSContext* Ctx, JSValueConst Exception, const std::string& Stack, const std::string& FileName)
{
    JSValue LineVal = GetPropertySafe(Ctx, Exception, "lineNumber");
    int32_t Line = 0;
    const bool HasLine = JS_IsNumber(LineVal) && JS_ToInt32(Ctx, &Line, LineVal) == 0 && Line > 0;
    JS_FreeValue(Ctx, LineVal);

    // Syntax errors carry a separate lineNumber property. If HasLine == false this will have been a runtime error and so we must extract the
    // linNumber from the stack trace, eg. "at <anonymous> (Script_ScriptHandlerErrorEntity:7)".
    if (HasLine)
    {
        return Line;
    }

    Line = 0;
    const std::string Offset = FileName + ":";
    const size_t Pos = Stack.find(Offset);

    if (Pos != std::string::npos)
    {
        const char* Begin = Stack.data() + Pos + Offset.size();
        std::from_chars(Begin, Stack.data() + Stack.size(), Line);
    }

    return Line;
}

csp::common::String FormatScriptError(JSContext* Ctx, JSValueConst Exception, const std::string& ScriptName, const std::string& FileName)
{
    // Safely convert the JS value to a string, clearing any exception that may be thrown as a result of this conversion failing.
    const std::string Message = ToStdString(Ctx, Exception);

    std::string Stack;
    // Check that the Exception value really is an Error object as only Error objects get a stack property.
    if (JS_IsError(Ctx, Exception))
    {
        // Get the stack property or discard any thrown exception and return JS_UNDEFINED.
        JSValue StackVal = GetPropertySafe(Ctx, Exception, "stack");

        if (!JS_IsUndefined(StackVal))
        {
            // Stack example with one frame per line:
            //      at <anonymous> (Script_ScriptHandlerErrorEntity:7)
            //      at <anonymous> (Script_ScriptHandlerErrorEntity:1)
            Stack = ToStdString(Ctx, StackVal);
        }

        JS_FreeValue(Ctx, StackVal);
    }

    // Syntax errors carry a separate lineNumber property. Runtime errors do not in which case extract the lineNumber from 
    // the stack trace, eg. "at <anonymous> (Script_ScriptHandlerErrorEntity:7)".
    const int32_t Line = GetErrorLineNumber(Ctx, Exception, Stack, FileName);

    csp::common::String Out = fmt::format("Script error in entity \"{}\"", ScriptName).c_str();

    if (Line > 0)
    {
        Out += fmt::format(" (line {})", Line).c_str();
    }

    Out += fmt::format(": {}", Message).c_str();

    if (!Stack.empty())
    {
        Out += fmt::format("\n{}", Stack).c_str();
    }

    return Out;
}

} // namespace csp::systems
