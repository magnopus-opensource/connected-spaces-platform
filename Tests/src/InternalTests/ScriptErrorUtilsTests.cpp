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
#include "TestHelpers.h"

#include "quickjspp.hpp"
#include "gtest/gtest.h"

using namespace csp::systems::ScriptErrorUtils;

namespace
{

// Reads the "stack" property of an Error as a std::string, using the utils under test.
std::string GetStackString(JSContext* Ctx, JSValueConst Exception)
{
    qjs::Value Stack { Ctx, GetPropertySafe(Ctx, Exception, "stack") };
    return JS_IsUndefined(Stack.v) ? std::string {} : ToStdString(Ctx, Stack.v);
}

} // namespace

// Validate that call to ToStdString returns a valid error message string and that the exception is cleared.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, ToStdStringClearException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    // The source must be null-terminated (eval asserts this), so a string literal is fine.
    // The filename becomes the name in stack frames, eg "(Script_MyEntity:3)".
    {
        qjs::Value Result = Context.eval("\n\nnull.notAProperty;", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    // Take the exception off the runtime. qjs::Value frees it when it goes out of scope.
    qjs::Value Exception = Context.getException();
    ASSERT_TRUE(JS_IsError(Context.ctx, Exception.v));

    const std::string Message = ToStdString(Context.ctx, Exception.v);
    EXPECT_NE(std::string(Message.c_str()).find("TypeError: cannot read property 'notAProperty'"), std::string::npos);

    // Check that the exception has been cleared.
    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that ToStdString returns a placeholder when the value can't be converted to a string,
// and that the exception thrown during the conversion is cleared.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, ToStdStringUnprintableValueClearsException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    // An object whose string conversion throws. QuickJS calls toString() and propagates its exception; it doesn't fall back to valueOf().
    qjs::Value Hostile = Context.eval("({ toString() { throw new Error('scriptError'); } })", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(Hostile.v));

    const std::string Message = ToStdString(Context.ctx, Hostile.v);
    EXPECT_EQ(Message, "<unprintable value>");

    // The Error('scriptError') thrown during the conversion must not be left on the runtime.
    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that GetPropertySafe returns the requested property from a thrown Error.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetPropertySafeReturnsProperty)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    {
        qjs::Value Result = Context.eval("throw new TypeError('scriptError');", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    qjs::Value Exception = Context.getException();
    ASSERT_TRUE(JS_IsError(Context.ctx, Exception.v));

    // GetPropertySafe returns a new reference. qjs::Value takes ownership and frees it.
    qjs::Value Message { Context.ctx, GetPropertySafe(Context.ctx, Exception.v, "message") };
    ASSERT_TRUE(JS_IsString(Message.v));
    EXPECT_EQ(ToStdString(Context.ctx, Message.v), "scriptError");

    // A property that doesn't exist comes back as undefined.
    qjs::Value Missing { Context.ctx, GetPropertySafe(Context.ctx, Exception.v, "doesNotExist") };
    EXPECT_TRUE(JS_IsUndefined(Missing.v));

    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that GetPropertySafe returns undefined when reading the property throws,
// and that the exception thrown by the read is cleared.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetPropertySafeThrowingGetterClearsException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    // Evaluated as an expression (no `throw`), so eval returns the object itself.
    qjs::Value Hostile = Context.eval("({ get stack() { throw new Error('scriptError'); } })", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(Hostile.v));

    qjs::Value Stack { Context.ctx, GetPropertySafe(Context.ctx, Hostile.v, "stack") };
    EXPECT_TRUE(JS_IsUndefined(Stack.v));

    // The Error('scriptError') thrown by the getter must not be left on the runtime.
    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that GetPropertySafe handles reading a property of a thrown null (eg `throw null;`).
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetPropertySafeOnNullClearsException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    qjs::Value Prop { Context.ctx, GetPropertySafe(Context.ctx, JS_NULL, "lineNumber") };
    EXPECT_TRUE(JS_IsUndefined(Prop.v));

    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that a syntax error's lineNumber property is used, rather than the line in the stack which is used for runtime errors.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberUsesLineNumberProperty)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    {
        // Syntax error on line 2. The QuickJS parser sets a lineNumber property on the SyntaxError.
        qjs::Value Result = Context.eval("\nlet x = ;", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    qjs::Value Exception = Context.getException();
    ASSERT_TRUE(JS_IsError(Context.ctx, Exception.v));

    // Pass a stack with a different line to show the lineNumber property is used correctly.
    const std::string MisleadingStack = "    at <eval> (Script_MyEntity:99)\n";
    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Exception.v, MisleadingStack, "Script_MyEntity"), 2);

    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that a runtime error, which has no lineNumber property, gets its line from the stack.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberFallsBackToStack)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    {
        qjs::Value Result = Context.eval("\n\nnull.notAProperty;", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    qjs::Value Exception = Context.getException();
    ASSERT_TRUE(JS_IsError(Context.ctx, Exception.v));

    // Stack looks like "    at <eval> (Script_MyEntity:3)\n"
    const std::string Stack = GetStackString(Context.ctx, Exception.v);
    ASSERT_FALSE(Stack.empty());

    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Exception.v, Stack, "Script_MyEntity"), 3);
}

// Validate that the first frame from the scripts own file is used, skipping frames from other files.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberUsesFirstMatchingFrame)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    qjs::Value Obj = Context.newObject();
    const std::string Stack = "    at a (OtherFile:5)\n"
                              "    at b (Script_MyEntity:7)\n"
                              "    at c (Script_MyEntity:1)\n";

    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Obj.v, Stack, "Script_MyEntity"), 7);
}

// Validate that a throwing lineNumber getter falls back to the stack, and its exception is cleared.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberThrowingGetterClearsException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    qjs::Value Hostile = Context.eval("({ get lineNumber() { throw new Error('scriptError'); } })", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(Hostile.v));

    const std::string Stack = "    at <eval> (Script_MyEntity:7)\n";
    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Hostile.v, Stack, "Script_MyEntity"), 7);

    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that a thrown null (no properties, no stack) returns 0 and leaves no pending exception.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberOnNullClearsException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    EXPECT_EQ(GetErrorLineNumber(Context.ctx, JS_NULL, "", "Script_MyEntity"), 0);

    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that a lineNumber that isn't a positive number is ignored in favour of the stack.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberIgnoresInvalidLineNumber)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    const std::string Stack = "    at <eval> (Script_MyEntity:7)\n";

    for (const char* Source : { "({ lineNumber: 0 })", "({ lineNumber: -3 })", "({ lineNumber: 'abc' })" })
    {
        qjs::Value Obj = Context.eval(Source, "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_FALSE(JS_IsException(Obj.v)) << Source;
        EXPECT_EQ(GetErrorLineNumber(Context.ctx, Obj.v, Stack, "Script_MyEntity"), 7) << Source;
    }
}

// Validate that 0 is returned when the stack can't be parsed.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, GetErrorLineNumberUnparseableStackReturnsZero)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    qjs::Value Obj = Context.newObject();

    // Scripts file isn't in the stack.
    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Obj.v, "    at a (OtherFile:5)\n", "Script_MyEntity"), 0);
    // File is present but there's no number after the colon.
    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Obj.v, "    at a (Script_MyEntity:)\n", "Script_MyEntity"), 0);
    // Empty stack.
    EXPECT_EQ(GetErrorLineNumber(Context.ctx, Obj.v, "", "Script_MyEntity"), 0);
}

// Validate that we correctly format an error message.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, FormatsRuntimeError)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    // The source must be null-terminated (eval asserts this), so a string literal is fine.
    // The filename becomes the name in stack frames, eg "(Script_MyEntity:3)".
    {
        qjs::Value Result = Context.eval("\n\nnull.notAProperty;", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    // Take the exception off the runtime. qjs::Value frees it when it goes out of scope.
    qjs::Value Exception = Context.getException();
    ASSERT_TRUE(JS_IsError(Context.ctx, Exception.v));

    const std::string Formatted = FormatScriptError(Context.ctx, Exception.v, "MyEntity", "Script_MyEntity").c_str();

    EXPECT_NE(std::string(Formatted).find("Script error in entity \"MyEntity\" (line 3): TypeError"), std::string::npos);

    // Confirm that FormatScriptError added the stack trace after the one-line message.
    // QuickJS frames are formatted as shown on line 2 below:
    // Script error in entity "MyEntity" (line 3): TypeError: cannot read property 'notAProperty' of null
    //     at<eval>(Script_MyEntity : 3)
    // The frame should point to this script's file and have the same line reported in the header.
    EXPECT_NE(Formatted.find("\n    at "), std::string::npos);
    EXPECT_NE(Formatted.find("(Script_MyEntity:3)"), std::string::npos);

    // Check that the exception has been cleared.
    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}

// Validate that a thrown non-Error value is formatted without a line number or stack.
// For FormatScriptError(), only real Error objects (Error, TypeError, SyntaxError, etc) get a stack property plus a lineNumber for syntax errors.
// A thrown string is just a string and so it gets neither.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, FormatsNonErrorThrow)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    {
        qjs::Value Result = Context.eval("throw 'plain';", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    qjs::Value Exception = Context.getException();
    const csp::common::String Formatted = FormatScriptError(Context.ctx, Exception.v, "MyEntity", "Script_MyEntity");

    EXPECT_EQ(std::string(Formatted.c_str()), "Script error in entity \"MyEntity\": plain");
}

// Validate that a thrown null is formatted and the exceptions raised while inspecting it are cleared.
// Internally ToStdString gives "null", and GetPropertySafe on null throws internally which must be cleared.
CSP_INTERNAL_TEST(CSPEngine, ScriptErrorUtilsTests, FormatsThrownNullClearsException)
{
    qjs::Runtime Runtime;
    qjs::Context Context(Runtime);

    {
        qjs::Value Result = Context.eval("throw null;", "Script_MyEntity", JS_EVAL_TYPE_GLOBAL);
        ASSERT_TRUE(JS_IsException(Result.v));
    }

    qjs::Value Exception = Context.getException();
    ASSERT_TRUE(JS_IsNull(Exception.v));

    const csp::common::String Formatted = FormatScriptError(Context.ctx, Exception.v, "MyEntity", "Script_MyEntity");
    EXPECT_EQ(std::string(Formatted.c_str()), "Script error in entity \"MyEntity\": null");

    qjs::Value Pending = Context.getException();
    EXPECT_TRUE(JS_IsNull(Pending.v));
}
