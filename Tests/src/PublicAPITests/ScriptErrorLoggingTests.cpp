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

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"

#include "TestHelpers.h"

#include "fmt/format.h"
#include "quickjspp.hpp"
#include "gtest/gtest.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace csp::multiplayer;

namespace
{

// Captures every message sent to the SystemsManager log system for the lifetime of the object.
// Messages are still forwarded to std::cerr so they appear in the test output.
struct LogCapture
{
    LogCapture()
    {
        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto& LogSystem = *SystemsManager.GetLogSystem();

        LogSystem.SetLogCallback(
            [this](csp::common::LogLevel Level, const csp::common::String& Message)
            {
                std::cerr << Message.c_str() << '\n';
                if (Level == csp::common::LogLevel::Error)
                {
                    ErrorMessages.push_back(Message.c_str());
                }
            });
    }

    ~LogCapture()
    {
        auto& SystemsManager = csp::systems::SystemsManager::Get();
        auto& LogSystem = *SystemsManager.GetLogSystem();

        LogSystem.SetLogCallback(nullptr);
    }

    const std::vector<std::string>& GetErrorMessages() const { return ErrorMessages; };

private:
    std::vector<std::string> ErrorMessages;
};

// Checks that an error log contains the script name and expected substrings.
::testing::AssertionResult HasScriptErrorLog(const LogCapture& Capture, std::string_view ScriptName, std::vector<std::string_view> ExpectedSubstrings)
{
    const std::string ExpectedHeader = fmt::format("Script error in \"{}\": ", ScriptName);

    const auto Contains = [](const std::string& Message, std::string_view Substring) { return Message.find(Substring) != std::string::npos; };

    const std::vector<std::string>& ErrorMessages = Capture.GetErrorMessages();

    for (const auto& Message : ErrorMessages)
    {
        if (!Contains(Message, ExpectedHeader))
        {
            continue;
        }

        const bool HasAllSubstrings = std::all_of(ExpectedSubstrings.begin(), ExpectedSubstrings.end(),
            [&Message, &Contains](std::string_view Substring) { return Contains(Message, Substring); });

        if (HasAllSubstrings)
        {
            return ::testing::AssertionSuccess();
        }
    }

    auto Failure = ::testing::AssertionFailure();
    Failure << "No error log contained '" << ExpectedHeader << "'";

    for (const auto& Substring : ExpectedSubstrings)
    {
        Failure << " and '" << Substring << "'";
    }

    Failure << ". Error messages (" << ErrorMessages.size() << "):";

    for (const auto& Message : ErrorMessages)
    {
        Failure << "\n  >>> " << Message;
    }

    return Failure;
}

} // namespace

/*
    A runtime error thrown at the top level of an entity script, while the script is first invoked.
    This is the simplest case: EntityScript::Invoke -> ScriptSystem::RunScript.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RuntimeErrorInScriptBodyIsReported)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    OfflineRealtimeEngine Engine { *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    csp::common::String EntityName = "ScriptBodyErrorEntity";
    SpaceEntity* Entity = nullptr;
    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    ASSERT_NE(Entity, nullptr);

    const std::string ScriptText = R"xx(
        var entities = TheEntitySystem.getEntities();
        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

        ThisEntity.notARealFunction();
    )xx";

    LogCapture Capture;

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    const bool Succeeded = Entity->GetScript().Invoke();

    EXPECT_FALSE(Succeeded);
    EXPECT_TRUE(Entity->GetScript().HasError());
    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), { "not a function" }));
}

/*
    An explicit `throw` from user code. Checks that the user's own error message makes it into the log.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, ThrownErrorInScriptBodyIsReported)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    OfflineRealtimeEngine Engine { *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName = "ScriptThrowEntity";
    SpaceEntity* Entity = nullptr;
    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    ASSERT_NE(Entity, nullptr);

    const std::string ScriptText = R"xx(
        function validate(value) {
            if (value < 0) {
                throw new Error("Value must not be negative");
            }
        }

        validate(-1);
    )xx";

    LogCapture Capture;

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    const bool Succeeded = Entity->GetScript().Invoke();

    EXPECT_FALSE(Succeeded);
    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), { "Value must not be negative" }));
}

/*
    A syntax error. QuickJS reports these at parse time with a different stack format (no function frames).
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, SyntaxErrorInScriptIsReported)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    OfflineRealtimeEngine Engine { *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName = "ScriptSyntaxErrorEntity";
    SpaceEntity* Entity = nullptr;
    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    ASSERT_NE(Entity, nullptr);

    const std::string ScriptText = R"xx(
        var entities = TheEntitySystem.getEntities();

        var broken = ;
    )xx";

    LogCapture Capture;

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    const bool Succeeded = Entity->GetScript().Invoke();

    EXPECT_FALSE(Succeeded);
    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), { "SyntaxError" }));
}

/*
    The scenario from the ticket: an error thrown inside a message handler (e.g. onTick, onClick).
    The handler is called via generated code from EntityScript::PostMessageToScript, which would produce a
    stack of two anonymous <eval> frames:
        at <anonymous> (<eval>:N)   <- inside the handler, in the entity's script
        at <anonymous> (<eval>:1)   <- the generated "handler('message','params')" call
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RuntimeErrorInMessageHandlerIsReported)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    OfflineRealtimeEngine Engine { *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName = "ScriptHandlerErrorEntity";
    SpaceEntity* Entity = nullptr;
    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    ASSERT_NE(Entity, nullptr);

    const std::string ScriptText = R"xx(
        var entities = TheEntitySystem.getEntities();
        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

        globalThis.onTestMessage = (_evtName, _params) => {
            var entity = entities[entityIndex];
            entity.notARealFunction();
        }

        ThisEntity.subscribeToMessage("testMessage", "onTestMessage");
    )xx";

    Entity->GetScript().SetScriptSource(ScriptText.c_str());

    // The script body itself is valid, so the initial invoke should succeed.
    ASSERT_TRUE(Entity->GetScript().Invoke());

    LogCapture Capture;

    Entity->GetScript().PostMessageToScript("testMessage");

    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), { "not a function" }));
}

/*
    EntityScript::GetErrorText is documented as returning the text of the last error, but previously always
    returned "Unknown Error" for script failures because the exception was consumed inside quickjspp.hpp.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, GetErrorTextReturnsScriptError)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    OfflineRealtimeEngine Engine { *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName = "ScriptErrorTextEntity";
    SpaceEntity* Entity = nullptr;
    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    ASSERT_NE(Entity, nullptr);

    const std::string ScriptText = R"xx(
        ThisEntity.notARealFunction();
    )xx";

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    ASSERT_FALSE(Entity->GetScript().Invoke());

    const std::string ErrorText = Entity->GetScript().GetErrorText().c_str();
    EXPECT_NE(ErrorText.find("not a function"), std::string::npos) << "GetErrorText() returned: " << ErrorText;
}

/*
    A runtime error thrown when running a script from a file. This bypasses EntityScript entirely:
    ScriptSystem::RunScriptFile -> qjs::Context::evalFile. The script is named after the file.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RuntimeErrorInScriptFileIsReported)
{
    auto& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();

    constexpr int64_t ContextId = INT64_MAX;
    ASSERT_TRUE(ScriptSystem.CreateContext(ContextId, []() { return csp::common::String("RunScriptFileError.js"); }));

    // Assets are copied next to the test executable, and loaded relative to the working directory like the other asset tests.
    const auto FilePath = std::filesystem::absolute("assets/RunScriptFileError.js");
    ASSERT_TRUE(std::filesystem::exists(FilePath)) << "Test script not found at " << FilePath.string();

    {
        LogCapture Capture;

        EXPECT_FALSE(ScriptSystem.RunScriptFile(ContextId, FilePath.string().c_str()));
        EXPECT_TRUE(HasScriptErrorLog(Capture, "RunScriptFileError.js", { "not a function" }));
    }

    // Confirm that the exception was correctly cleared.
    {
        auto* Context = static_cast<qjs::Context*>(ScriptSystem.GetContext(ContextId));
        qjs::Value Exception = Context->getException();
        EXPECT_TRUE(JS_IsNull(Exception.v)) << "A QuickJS exception was left pending on the runtime after a failed script run.";
    }

    // Now throw another exception.
    {
        LogCapture Capture;

        EXPECT_FALSE(ScriptSystem.RunScriptFile(ContextId, FilePath.string().c_str()));
        EXPECT_TRUE(HasScriptErrorLog(Capture, "RunScriptFileError.js", { "not a function" }));

        // Confirm that a second different failure is reported as its own error, not the previous one.
        EXPECT_FALSE(ScriptSystem.RunScript(ContextId, "\nthrow new Error(\"Second failure\");"));

        const auto& Errors = Capture.GetErrorMessages();
        ASSERT_EQ(Errors.size(), 2u);
        EXPECT_NE(Errors[1].find("Second failure"), std::string::npos);
        EXPECT_EQ(Errors[1].find("not a function"), std::string::npos);
    }

    // The exception must have been consumed. A later, valid script in the same runtime should neither fail nor report it.
    {
        LogCapture Capture;

        EXPECT_TRUE(ScriptSystem.RunScript(ContextId, "const Valid = 1;"));
        EXPECT_TRUE(Capture.GetErrorMessages().empty());
    }
}

/*
    Ensures that calls to run a script file with no path specified fail correctly.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RunScriptFileWithEmptyPathFails)
{
    auto& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();
    constexpr int64_t ContextId = INT64_MAX;
    ASSERT_TRUE(ScriptSystem.CreateContext(ContextId, nullptr));

    LogCapture Capture;
    EXPECT_FALSE(ScriptSystem.RunScriptFile(ContextId, ""));
    EXPECT_FALSE(Capture.GetErrorMessages().empty());

    ScriptSystem.DestroyContext(ContextId);
}

/*
    Ensures that calls to run a script file with an incorrect path specified fail correctly.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RunScriptFileWithMissingFileFails)
{
    auto& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();
    constexpr int64_t ContextId = INT64_MAX;
    ASSERT_TRUE(ScriptSystem.CreateContext(ContextId, nullptr));

    LogCapture Capture;
    EXPECT_FALSE(ScriptSystem.RunScriptFile(ContextId, "assets/DoesNotExist.js"));
    EXPECT_FALSE(Capture.GetErrorMessages().empty());

    ScriptSystem.DestroyContext(ContextId);
}

/*
    A runtime error thrown inside a function exported by an imported module, called from the script.
    The first stack frame is in the module, followed by the call site in the importing script:
        at throwFromModule (RuntimeErrorModule:N)
        at <anonymous> (Script_ModuleRuntimeErrorScript:M)
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RuntimeErrorInImportedModuleFunctionIsReported)
{
    auto& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();

    constexpr int64_t ContextId = INT64_MAX;
    const std::string ScriptName = "ModuleRuntimeErrorScript";
    ASSERT_TRUE(ScriptSystem.CreateContext(ContextId, [ScriptName]() { return csp::common::String(ScriptName.c_str()); }));

    const std::string ModuleName = "RuntimeErrorModule";
    const std::string ModuleText = R"xx(
        export function throwFromModule() {
            throw new Error("Thrown from imported module");
        }
    )xx";

    ScriptSystem.SetModuleSource(ModuleName.c_str(), ModuleText.c_str());

    // Pad the script so the call site line differs from the module's error line.
    const std::string ScriptText = fmt::format(R"xx(
        import {{ throwFromModule }} from "{}";

        var padding1 = 1;
        var padding2 = 2;

        throwFromModule();
    )xx",
        ModuleName);

    LogCapture Capture;

    // Match the module stack frames for runtime and syntax errors: "(RuntimeErrorModule:3)" or "at SyntaxErrorModule:6".
    auto ModuleFrame = fmt::format("{}:", ModuleName);

    EXPECT_FALSE(ScriptSystem.RunScript(ContextId, ScriptText.c_str()));
    EXPECT_TRUE(HasScriptErrorLog(Capture, ScriptName, { ModuleFrame, "Thrown from imported module" }));

    ScriptSystem.ClearModuleSource(ModuleName.c_str());
    ScriptSystem.DestroyContext(ContextId);
}

/*
    A runtime error thrown by an imported module's top-level code while it is being evaluated as part of the import.
    No stack frame belongs to the importing script, so the location can only come from the module's own frame.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RuntimeErrorInImportedModuleTopLevelIsReported)
{
    auto& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();

    constexpr int64_t ContextId = INT64_MAX;
    const std::string ScriptName = "ModuleTopLevelErrorScript";
    ASSERT_TRUE(ScriptSystem.CreateContext(ContextId, [ScriptName]() { return csp::common::String(ScriptName.c_str()); }));

    const std::string ModuleName = "TopLevelErrorModule";
    const std::string ModuleText = R"xx(
        export const value = 1;

        throw new Error("Thrown from module top level");
    )xx";

    ScriptSystem.SetModuleSource(ModuleName.c_str(), ModuleText.c_str());

    const std::string ScriptText = fmt::format(R"xx(
        import {{ value }} from "{}";
    )xx",
        ModuleName);

    LogCapture Capture;

    // Match the module stack frames for runtime and syntax errors: "(RuntimeErrorModule:3)" or "at SyntaxErrorModule:6".
    auto ModuleFrame = fmt::format("{}:", ModuleName);

    EXPECT_FALSE(ScriptSystem.RunScript(ContextId, ScriptText.c_str()));
    EXPECT_TRUE(HasScriptErrorLog(Capture, ScriptName, { ModuleFrame, "Thrown from module top level" }));

    ScriptSystem.ClearModuleSource(ModuleName.c_str());
    ScriptSystem.DestroyContext(ContextId);
}

/*
    The imported module does not have an error, instead the error is thrown in the importing script itself after a successful import.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, RuntimeErrorInScriptWithImportIsReportedAgainstScript)
{
    auto& ScriptSystem = *csp::systems::SystemsManager::Get().GetScriptSystem();

    constexpr int64_t ContextId = INT64_MAX;
    const std::string ScriptName = "ScriptWithImportErrorScript";
    ASSERT_TRUE(ScriptSystem.CreateContext(ContextId, [ScriptName]() { return csp::common::String(ScriptName.c_str()); }));

    const std::string ModuleName = "ValidModule";
    const std::string ModuleText = R"xx(
        export function add(a, b) {
            return a + b;
        }
    )xx";

    ScriptSystem.SetModuleSource(ModuleName.c_str(), ModuleText.c_str());

    const std::string ScriptText = fmt::format(R"xx(
        import {{ add }} from "{}";

        var total = add(1, 2);
        throw new Error("Thrown from importing script");
    )xx",
        ModuleName);

    LogCapture Capture;

    EXPECT_FALSE(ScriptSystem.RunScript(ContextId, ScriptText.c_str()));
    EXPECT_TRUE(HasScriptErrorLog(Capture, ScriptName, { "Thrown from importing script" }));

    ScriptSystem.ClearModuleSource(ModuleName.c_str());
    ScriptSystem.DestroyContext(ContextId);
}

/*
    Ensures that the ScriptErrorCallback is successfully fired when registered if a script error occurs.
    Callback contains a ScriptErrorData struct with the ContextId, ScriptName and formatted ErrorMessage.
*/
CSP_PUBLIC_TEST(CSPEngine, ScriptErrorLoggingTests, ScriptErrorCallbackFiredTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& ScriptSystem = *SystemsManager.GetScriptSystem();

    OfflineRealtimeEngine Engine { *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    std::promise<csp::systems::ScriptErrorData> ScriptErrorPromise;
    auto ScriptErrorFuture = ScriptErrorPromise.get_future();

    auto ScriptErrorCallback = [&ScriptErrorPromise](csp::systems::ScriptErrorData Data) { ScriptErrorPromise.set_value(Data); };
    ScriptSystem.SetScriptErrorCallback(ScriptErrorCallback);

    const csp::common::String EntityName = "ScriptErrorTextEntity";
    SpaceEntity* Entity = nullptr;
    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    ASSERT_NE(Entity, nullptr);

    const std::string ScriptText = R"xx(
        ThisEntity.notARealFunction();
    )xx";

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    ASSERT_FALSE(Entity->GetScript().Invoke());

    ASSERT_TRUE(ScriptErrorFuture.wait_for(10s) == std::future_status::ready) << "Timed out after 10s waiting for ScriptErrorCallback.";
    const csp::systems::ScriptErrorData Data = ScriptErrorFuture.get();

    EXPECT_EQ(Data.ContextId, Entity->GetId());
    EXPECT_EQ(Data.ScriptName, "ScriptErrorTextEntity");

    const std::string ErrorText = Data.ErrorMessage.c_str();
    EXPECT_NE(ErrorText.find("not a function"), std::string::npos) << "GetErrorText() returned: " << ErrorText;
}
