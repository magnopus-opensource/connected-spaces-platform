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

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETINGSYSTEM_TESTS || RUN_EVENTTICKETINGSYSTEM_CREATETICKETEDEVENT_ACTIVE_TRUE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventActiveTrueTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String TestVendorEventId  = "TestVendorEventId";
	csp::common::String TestVendorEventUri = "TestVendorEventUri";

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
										   TestVendorEventId,
										   TestVendorEventUri,
										   true);

	EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::services::EResultCode::Success);

	auto Event = TicketedEventResult.GetTicketedEvent();

	EXPECT_EQ(Event.SpaceId, Space.Id);
	EXPECT_EQ(Event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(Event.VendorEventId, TestVendorEventId);
	EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri);
	EXPECT_TRUE(Event.IsTicketingActive);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETINGSYSTEM_TESTS || RUN_EVENTTICKETINGSYSTEM_CREATETICKETEDEVENT_ACTIVE_FALSE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventActiveFalseTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String TestVendorEventId  = "TestVendorEventId";
	csp::common::String TestVendorEventUri = "TestVendorEventUri";

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
										   TestVendorEventId,
										   TestVendorEventUri,
										   false);

	EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::services::EResultCode::Success);

	auto Event = TicketedEventResult.GetTicketedEvent();

	EXPECT_EQ(Event.SpaceId, Space.Id);
	EXPECT_EQ(Event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(Event.VendorEventId, TestVendorEventId);
	EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri);
	EXPECT_FALSE(Event.IsTicketingActive);

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

	csp::common::String TestVendorEventId1	= "TestVendorEventId1";
	csp::common::String TestVendorEventUri1 = "TestVendorEventUri1";

	csp::common::String TestVendorEventId2	= "TestVendorEventId2";
	csp::common::String TestVendorEventUri2 = "TestVendorEventUri2";

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
											TestVendorEventId1,
											TestVendorEventUri1,
											true);

	EXPECT_EQ(TicketedEventResult1.GetResultCode(), csp::services::EResultCode::Success);

	auto Event1 = TicketedEventResult1.GetTicketedEvent();

	EXPECT_EQ(Event1.SpaceId, Space.Id);
	EXPECT_EQ(Event1.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(Event1.VendorEventId, TestVendorEventId1);
	EXPECT_EQ(Event1.VendorEventUri, TestVendorEventUri1);
	EXPECT_TRUE(Event1.IsTicketingActive);

	auto [TicketedEventResult2] = AWAIT_PRE(EventTicketingSystem,
											CreateTicketedEvent,
											RequestPredicate,
											Space.Id,
											csp::systems::EventTicketingVendor::Eventbrite,
											TestVendorEventId2,
											TestVendorEventUri2,
											false);

	EXPECT_EQ(TicketedEventResult1.GetResultCode(), csp::services::EResultCode::Success);

	auto Event2 = TicketedEventResult2.GetTicketedEvent();

	EXPECT_EQ(Event2.SpaceId, Space.Id);
	EXPECT_EQ(Event2.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(Event2.VendorEventId, TestVendorEventId2);
	EXPECT_EQ(Event2.VendorEventUri, TestVendorEventUri2);
	EXPECT_FALSE(Event2.IsTicketingActive);

	EXPECT_NE(Event1.Id, Event2.Id);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif
