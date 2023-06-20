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
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "TestHelpers.h"
#include "quickjspp.hpp"

#include "gtest/gtest.h"
#include <atomic>

#if RUN_ALL_UNIT_TESTS || RUN_SCRIPTSYSTEM_TESTS || RUN_SCRIPTSYSTEM_SCRIPT_BINDING_TEST
CSP_PUBLIC_TEST(CSPEngine, ScriptSystemTests, ScriptBindingTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto& ScriptSystem	 = *SystemsManager.GetScriptSystem();

	std::string TestMessage;

	ScriptSystem.Initialise();

	auto Fn = [&TestMessage](const char* Str)
	{
		TestMessage = Str;
		FOUNDATION_LOG_MSG(csp::systems::LogLevel::Log, Str);
		std::cout << Str << "\n";
	};

	constexpr int ContextId = 0;

	ScriptSystem.CreateContext(ContextId);

	qjs::Context* Context		 = (qjs::Context*) ScriptSystem.GetContext(ContextId);
	qjs::Context::Module* Module = (qjs::Context::Module*) ScriptSystem.GetModule(ContextId, "CSPTest");

	Module->function("RunFunction", Fn);

	std::string ScriptText = R"xx(

        import * as CSPTest from "CSPTest";
        CSPTest.RunFunction('Hello Test');

        globalThis.onCallback = function()
        {   
            CSPTest.RunFunction('Hello Callback');
        }

    )xx";

	bool NoScriptErrors = ScriptSystem.RunScript(ContextId, ScriptText.c_str());

	EXPECT_TRUE(NoScriptErrors);
	EXPECT_TRUE(TestMessage == "Hello Test");

	ScriptSystem.RunScript(ContextId, "onCallback()");

	EXPECT_TRUE(TestMessage == "Hello Callback");

	ScriptSystem.DestroyContext(ContextId);
	ScriptSystem.Shutdown();
}
#endif
