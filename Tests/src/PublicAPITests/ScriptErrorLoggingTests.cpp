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
struct RAIILogCapture
{
    RAIILogCapture()
    {
        csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
            [this](csp::common::LogLevel Level, const csp::common::String& Message)
            {
                std::cerr << Message.c_str() << '\n';
                if (Level == csp::common::LogLevel::Error)
                {
                    ErrorMessages.push_back(Message.c_str());
                }
            });
    }

    ~RAIILogCapture() { csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr); }

    const std::vector<std::string>& GetErrorMessages() const { return ErrorMessages; };

private:
    std::vector<std::string> ErrorMessages;
};

// Returns the line number (starting at 1) of the first line in ScriptErrorLog containing SearchStr, or 0 if not found.
size_t LineNumOf(std::string_view ScriptErrorLog, std::string_view SearchStr)
{
    const size_t Pos = ScriptErrorLog.find(SearchStr);

    if (Pos == std::string::npos)
    {
        return 0;
    }

    // Count the number of newline characters before the match.
    return static_cast<size_t>(std::count(ScriptErrorLog.begin(), ScriptErrorLog.begin() + Pos, '\n')) + 1;
}

// Checks that an error log contains the error message written by ScriptErrorUtils::FormatScriptError().
// All tests assume a line number so the error will be:
// - Script error in entity "<entity name>" (line num)
::testing::AssertionResult HasScriptErrorLog(
    const RAIILogCapture& Capture, std::string_view ScriptName, std::string_view ErrorText, size_t ExpectedLine)
{
    const std::string ExpectedError = fmt::format("Script error in entity \"{}\" (line {}): ", ScriptName, ExpectedLine);

    const std::vector<std::string>& ErrorMessages = Capture.GetErrorMessages();

    for (const auto& Message : ErrorMessages)
    {
        if (Message.find(ExpectedError) != std::string::npos && Message.find(ErrorText) != std::string::npos)
        {
            return ::testing::AssertionSuccess();
        }
    }

    auto Failure = ::testing::AssertionFailure();
    Failure << "No error log contained '" << ExpectedError << "' and '" << ErrorText << "'. Error messages (" << ErrorMessages.size() << "):";

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

    const size_t ErrorLine = LineNumOf(ScriptText, "notARealFunction");
    ASSERT_NE(ErrorLine, 0u);

    RAIILogCapture Capture;

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    const bool Succeeded = Entity->GetScript().Invoke();

    EXPECT_FALSE(Succeeded);
    EXPECT_TRUE(Entity->GetScript().HasError());
    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), "not a function", ErrorLine));
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

    const size_t ErrorLine = LineNumOf(ScriptText, "throw new Error");
    ASSERT_NE(ErrorLine, 0u);

    RAIILogCapture Capture;

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    const bool Succeeded = Entity->GetScript().Invoke();

    EXPECT_FALSE(Succeeded);
    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), "Value must not be negative", ErrorLine));
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

    const size_t ErrorLine = LineNumOf(ScriptText, "var broken");
    ASSERT_NE(ErrorLine, 0u);

    RAIILogCapture Capture;

    Entity->GetScript().SetScriptSource(ScriptText.c_str());
    const bool Succeeded = Entity->GetScript().Invoke();

    EXPECT_FALSE(Succeeded);
    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), "SyntaxError", ErrorLine));
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

    const size_t ErrorLine = LineNumOf(ScriptText, "notARealFunction");
    ASSERT_NE(ErrorLine, 0u);

    Entity->GetScript().SetScriptSource(ScriptText.c_str());

    // The script body itself is valid, so the initial invoke should succeed.
    ASSERT_TRUE(Entity->GetScript().Invoke());

    RAIILogCapture Capture;

    Entity->GetScript().PostMessageToScript("testMessage");

    EXPECT_TRUE(HasScriptErrorLog(Capture, EntityName.c_str(), "not a function", ErrorLine));
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

    // Find the failing line from the test file.
    const size_t ErrorLine = 4;
    ASSERT_NE(ErrorLine, 0u);

    {
        RAIILogCapture Capture;

        const bool Succeeded = ScriptSystem.RunScriptFile(ContextId, FilePath.string().c_str());

        EXPECT_FALSE(Succeeded);
        EXPECT_TRUE(HasScriptErrorLog(Capture, "RunScriptFileError.js", "not a function", ErrorLine));
    }

    // The exception must have been consumed. A later, valid script in the same runtime should neither fail nor report it.
    {
        RAIILogCapture Capture;

        EXPECT_TRUE(ScriptSystem.RunScript(ContextId, "const Valid = 1;"));
        EXPECT_TRUE(Capture.GetErrorMessages().empty());
    }

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

    EXPECT_TRUE(ScriptErrorFuture.wait_for(10s) == std::future_status::ready) << "Timed out after 10s waiting for ScriptErrorCallback.";
    const csp::systems::ScriptErrorData Data = ScriptErrorFuture.get();

    EXPECT_EQ(Data.ContextId, Entity->GetId());
    EXPECT_EQ(Data.ScriptName, "ScriptErrorTextEntity");

    const std::string ErrorText = Data.ErrorMessage.c_str();
    EXPECT_NE(ErrorText.find("not a function"), std::string::npos) << "GetErrorText() returned: " << ErrorText;
}
