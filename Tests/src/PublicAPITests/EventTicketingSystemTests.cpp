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
#include "CSP/Systems/EventTicketing/EventTicketingSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::services::ResultBase& Result)
{
	return Result.GetResultCode() != csp::services::EResultCode::InProgress;
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETINGSYSTEM_TESTS || RUN_EVENTTICKETINGSYSTEM_DUMMY_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, DummyTest)
{
	auto& SystemsManager	  = csp::systems::SystemsManager::Get();
	auto EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	auto [Result] = AWAIT(EventTicketingSystem, Dummy);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Failed);
}
#endif