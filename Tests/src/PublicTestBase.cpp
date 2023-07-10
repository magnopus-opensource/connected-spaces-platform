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

#include "PublicTestBase.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "TestHelpers.h"


void PublicTestBase::SetUp()
{
	::testing::Test::SetUp();

	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	auto* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

	LogSystem->SetSystemLevel(csp::systems::LogLevel::VeryVerbose);

	LogSystem->SetEventCallback(
		[](csp::common::String Message)
		{
			std::cerr << "[ ::EVENT  ] " << Message.c_str() << std::endl;
		});

	LogSystem->SetLogCallback(
		[](csp::common::String Message)
		{
			std::cerr << "[ ::LOG    ] " << Message.c_str() << std::endl;
		});

	LogDebug("Foundation initialized!");
}

void PublicTestBase::TearDown()
{
	::testing::Test::TearDown();

	if (!csp::CSPFoundation::GetIsInitialised())
	{
		LogError("csp::CSPFoundation::Shutdown() already called! Please remove any explicit calls to Initialise() and Shutdown() from this test.");

		return;
	}

	while (!CleanupQueue.empty())
	{
		auto Function = CleanupQueue.top();
		CleanupQueue.pop();

		Function();
	}

	csp::systems::SystemsManager::Get().GetLogSystem()->ClearAllCallbacks();
	csp::CSPFoundation::Shutdown();
	LogDebug("Foundation uninitialized");
}