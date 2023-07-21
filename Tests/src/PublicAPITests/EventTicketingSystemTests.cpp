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

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETINGSYSTEM_TESTS || RUN_EVENTTICKETINGSYSTEM_CREATETICKETEDEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [TicketedEventResult] = AWAIT_PRE(EventTicketingSystem,
										   CreateTicketedEvent,
										   RequestPredicate,
										   Space.Id,
										   csp::systems::EventTicketingVendor::Eventbrite,
										   "TestEventId",
										   "TestEventUri",
										   true);

	EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::services::EResultCode::Success);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETINGSYSTEM_TESTS || RUN_EVENTTICKETINGSYSTEM_CREATETICKETEDEVENT_TWICE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventTwiceTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [TicketedEventResult1] = AWAIT_PRE(EventTicketingSystem,
											CreateTicketedEvent,
											RequestPredicate,
											Space.Id,
											csp::systems::EventTicketingVendor::Eventbrite,
											"TestEventId1",
											"TestEventUri1",
											true);

	EXPECT_EQ(TicketedEventResult1.GetResultCode(), csp::services::EResultCode::Success);

	auto [TicketedEventResult2] = AWAIT_PRE(EventTicketingSystem,
											CreateTicketedEvent,
											RequestPredicate,
											Space.Id,
											csp::systems::EventTicketingVendor::Eventbrite,
											"TestEventId2",
											"TestEventUri2",
											true);

	EXPECT_EQ(TicketedEventResult1.GetResultCode(), csp::services::EResultCode::Success);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif
