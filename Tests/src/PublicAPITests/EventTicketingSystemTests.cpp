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

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_CREATETICKETEDEVENT_ACTIVE_TRUE_TEST
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

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_CREATETICKETEDEVENT_ACTIVE_FALSE_TEST
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

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_CREATETICKETEDEVENT_TWICE_TEST
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

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_UPDATETICKETEDEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, UpdateTicketEventTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String InitialVendorEventId  = "InitialVendorEventId";
	csp::common::String InitialVendorEventUri = "InitialVendorEventUri";
	csp::common::String UpdatedVendorEventId  = "UpdatedVendorEventId";
	csp::common::String UpdatedVendorEventUri = "UpdatedVendorEventUri";

	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [CreatedResult] = AWAIT_PRE(EventTicketingSystem,
									 CreateTicketedEvent,
									 RequestPredicate,
									 Space.Id,
									 csp::systems::EventTicketingVendor::Eventbrite,
									 InitialVendorEventId,
									 InitialVendorEventUri,
									 false);

	EXPECT_EQ(CreatedResult.GetResultCode(), csp::services::EResultCode::Success);

	auto CreatedEvent = CreatedResult.GetTicketedEvent();

	EXPECT_EQ(CreatedEvent.SpaceId, Space.Id);
	EXPECT_EQ(CreatedEvent.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(CreatedEvent.VendorEventId, InitialVendorEventId);
	EXPECT_EQ(CreatedEvent.VendorEventUri, InitialVendorEventUri);
	EXPECT_FALSE(CreatedEvent.IsTicketingActive);

	auto [UpdatedResult] = AWAIT_PRE(EventTicketingSystem,
									 UpdateTicketedEvent,
									 RequestPredicate,
									 Space.Id,
									 CreatedEvent.Id,
									 csp::systems::EventTicketingVendor::Eventbrite,
									 UpdatedVendorEventId,
									 UpdatedVendorEventUri,
									 true);

	EXPECT_EQ(UpdatedResult.GetResultCode(), csp::services::EResultCode::Success);

	auto UpdatedEvent = UpdatedResult.GetTicketedEvent();

	EXPECT_EQ(UpdatedEvent.Id, CreatedEvent.Id);
	EXPECT_EQ(UpdatedEvent.SpaceId, Space.Id);
	EXPECT_EQ(UpdatedEvent.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(UpdatedEvent.VendorEventId, UpdatedVendorEventId);
	EXPECT_EQ(UpdatedEvent.VendorEventUri, UpdatedVendorEventUri);
	EXPECT_TRUE(UpdatedEvent.IsTicketingActive);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_UPDATETICKETEDEVENT_BADSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, UpdateTicketEventBadSpaceTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String InitialVendorEventId  = "InitialVendorEventId";
	csp::common::String InitialVendorEventUri = "InitialVendorEventUri";
	csp::common::String UpdatedVendorEventId  = "UpdatedVendorEventId";
	csp::common::String UpdatedVendorEventUri = "UpdatedVendorEventUri";

	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [CreatedResult] = AWAIT_PRE(EventTicketingSystem,
									 CreateTicketedEvent,
									 RequestPredicate,
									 Space.Id,
									 csp::systems::EventTicketingVendor::Eventbrite,
									 InitialVendorEventId,
									 InitialVendorEventUri,
									 false);

	EXPECT_EQ(CreatedResult.GetResultCode(), csp::services::EResultCode::Success);

	auto CreatedEvent = CreatedResult.GetTicketedEvent();

	auto [UpdatedResult] = AWAIT_PRE(EventTicketingSystem,
									 UpdateTicketedEvent,
									 RequestPredicate,
									 "12a345678b9cdd01ef23456a",
									 CreatedEvent.Id,
									 csp::systems::EventTicketingVendor::Eventbrite,
									 UpdatedVendorEventId,
									 UpdatedVendorEventUri,
									 true);

	EXPECT_EQ(UpdatedResult.GetResultCode(), csp::services::EResultCode::Failed);
	EXPECT_EQ(UpdatedResult.GetHttpResultCode(), 404);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_UPDATETICKETEDEVENT_BADEVENTID_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, UpdateTicketEventBadEventIdTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* SpaceSystem		   = SystemsManager.GetSpaceSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String InitialVendorEventId  = "InitialVendorEventId";
	csp::common::String InitialVendorEventUri = "InitialVendorEventUri";
	csp::common::String UpdatedVendorEventId  = "UpdatedVendorEventId";
	csp::common::String UpdatedVendorEventUri = "UpdatedVendorEventUri";

	const char* TestSpaceName		 = "CSP-UNITTEST-SPACE";
	const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [CreatedResult] = AWAIT_PRE(EventTicketingSystem,
									 CreateTicketedEvent,
									 RequestPredicate,
									 Space.Id,
									 csp::systems::EventTicketingVendor::Eventbrite,
									 InitialVendorEventId,
									 InitialVendorEventUri,
									 false);

	EXPECT_EQ(CreatedResult.GetResultCode(), csp::services::EResultCode::Success);

	auto CreatedEvent = CreatedResult.GetTicketedEvent();

	auto [UpdatedResult] = AWAIT_PRE(EventTicketingSystem,
									 UpdateTicketedEvent,
									 RequestPredicate,
									 Space.Id,
									 "12a345678b9cdd01ef23456a",
									 csp::systems::EventTicketingVendor::Eventbrite,
									 UpdatedVendorEventId,
									 UpdatedVendorEventUri,
									 true);

	EXPECT_EQ(UpdatedResult.GetResultCode(), csp::services::EResultCode::Failed);
	EXPECT_EQ(UpdatedResult.GetHttpResultCode(), 404);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_NO_EVENTS_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsNoEventsTest)
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

	auto [Result] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space.Id}, nullptr, nullptr);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	EXPECT_EQ(Result.GetTicketedEvents().Size(), 0);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_ONE_EVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsOneEventTest)
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

	auto [CreateEventResult] = AWAIT_PRE(EventTicketingSystem,
										 CreateTicketedEvent,
										 RequestPredicate,
										 Space.Id,
										 csp::systems::EventTicketingVendor::Eventbrite,
										 TestVendorEventId,
										 TestVendorEventUri,
										 false);
	EXPECT_EQ(CreateEventResult.GetResultCode(), csp::services::EResultCode::Success);

	auto [Result] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space.Id}, nullptr, nullptr);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	EXPECT_EQ(Result.GetTicketedEvents().Size(), 1);

	auto Event = Result.GetTicketedEvents()[0];

	EXPECT_EQ(Event.Id, CreateEventResult.GetTicketedEvent().Id);
	EXPECT_EQ(Event.SpaceId, Space.Id);
	EXPECT_EQ(Event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_EQ(Event.VendorEventId, TestVendorEventId);
	EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri);
	EXPECT_FALSE(Event.IsTicketingActive);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif


#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_TWO_EVENTS_SAME_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsTwoEventsSameSpaceTest)
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

	auto [CreateEventResult1] = AWAIT_PRE(EventTicketingSystem,
										  CreateTicketedEvent,
										  RequestPredicate,
										  Space.Id,
										  csp::systems::EventTicketingVendor::Eventbrite,
										  TestVendorEventId1,
										  TestVendorEventUri1,
										  true);
	EXPECT_EQ(CreateEventResult1.GetResultCode(), csp::services::EResultCode::Success);
	auto Event1 = CreateEventResult1.GetTicketedEvent();

	auto [CreateEventResult2] = AWAIT_PRE(EventTicketingSystem,
										  CreateTicketedEvent,
										  RequestPredicate,
										  Space.Id,
										  csp::systems::EventTicketingVendor::Eventbrite,
										  TestVendorEventId2,
										  TestVendorEventUri2,
										  false);
	EXPECT_EQ(CreateEventResult2.GetResultCode(), csp::services::EResultCode::Success);
	auto Event2 = CreateEventResult2.GetTicketedEvent();

	auto [Result] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space.Id}, nullptr, nullptr);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	auto Events = Result.GetTicketedEvents();

	EXPECT_EQ(Result.GetTicketedEvents().Size(), 2);

	bool FoundFirst		 = false;
	bool FoundSecond	 = false;
	bool FoundUnexpected = false;

	for (auto i = 0; i < Events.Size(); ++i)
	{
		auto Event = Result.GetTicketedEvents()[i];

		EXPECT_EQ(Event.SpaceId, Space.Id);
		EXPECT_EQ(Event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);

		if (Event.Id == Event1.Id)
		{
			EXPECT_EQ(Event.VendorEventId, TestVendorEventId1);
			EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri1);
			EXPECT_TRUE(Event.IsTicketingActive);
			FoundFirst = true;
		}
		else if (Event.Id == Event2.Id)
		{
			EXPECT_EQ(Event.VendorEventId, TestVendorEventId2);
			EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri2);
			EXPECT_FALSE(Event.IsTicketingActive);
			FoundSecond = true;
		}
		else
		{
			FoundUnexpected = true;
		}
	}

	EXPECT_TRUE(FoundFirst);
	EXPECT_TRUE(FoundSecond);
	EXPECT_FALSE(FoundUnexpected);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_TWO_EVENTS_TWO_SPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsTwoEventsTwoSpacesTest)
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

	char UniqueSpaceName1[256];
	SPRINTF(UniqueSpaceName1, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());
	char UniqueSpaceName2[256];
	SPRINTF(UniqueSpaceName2, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	csp::systems::Space Space1;
	CreateSpace(SpaceSystem, UniqueSpaceName1, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space1);
	csp::systems::Space Space2;
	CreateSpace(SpaceSystem, UniqueSpaceName2, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space2);

	auto [CreateEventResult1] = AWAIT_PRE(EventTicketingSystem,
										  CreateTicketedEvent,
										  RequestPredicate,
										  Space1.Id,
										  csp::systems::EventTicketingVendor::Eventbrite,
										  TestVendorEventId1,
										  TestVendorEventUri1,
										  true);
	EXPECT_EQ(CreateEventResult1.GetResultCode(), csp::services::EResultCode::Success);
	auto Event1 = CreateEventResult1.GetTicketedEvent();

	auto [CreateEventResult2] = AWAIT_PRE(EventTicketingSystem,
										  CreateTicketedEvent,
										  RequestPredicate,
										  Space2.Id,
										  csp::systems::EventTicketingVendor::Eventbrite,
										  TestVendorEventId2,
										  TestVendorEventUri2,
										  false);
	EXPECT_EQ(CreateEventResult2.GetResultCode(), csp::services::EResultCode::Success);
	auto Event2 = CreateEventResult2.GetTicketedEvent();

	auto [Result] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space1.Id, Space2.Id}, nullptr, nullptr);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	auto Events = Result.GetTicketedEvents();

	EXPECT_EQ(Result.GetTicketedEvents().Size(), 2);

	bool FoundFirst		 = false;
	bool FoundSecond	 = false;
	bool FoundUnexpected = false;

	for (auto i = 0; i < Events.Size(); ++i)
	{
		auto Event = Result.GetTicketedEvents()[i];

		EXPECT_EQ(Event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);

		if (Event.Id == Event1.Id)
		{
			EXPECT_EQ(Event.SpaceId, Space1.Id);
			EXPECT_EQ(Event.VendorEventId, TestVendorEventId1);
			EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri1);
			EXPECT_TRUE(Event.IsTicketingActive);
			FoundFirst = true;
		}
		else if (Event.Id == Event2.Id)
		{
			EXPECT_EQ(Event.SpaceId, Space2.Id);
			EXPECT_EQ(Event.VendorEventId, TestVendorEventId2);
			EXPECT_EQ(Event.VendorEventUri, TestVendorEventUri2);
			EXPECT_FALSE(Event.IsTicketingActive);
			FoundSecond = true;
		}
		else
		{
			FoundUnexpected = true;
		}
	}

	EXPECT_TRUE(FoundFirst);
	EXPECT_TRUE(FoundSecond);
	EXPECT_FALSE(FoundUnexpected);

	DeleteSpace(SpaceSystem, Space1.Id);
	DeleteSpace(SpaceSystem, Space2.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_PAGINATION_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsPaginationTest)
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

	auto [CreateEventResult1] = AWAIT_PRE(EventTicketingSystem,
										  CreateTicketedEvent,
										  RequestPredicate,
										  Space.Id,
										  csp::systems::EventTicketingVendor::Eventbrite,
										  TestVendorEventId1,
										  TestVendorEventUri1,
										  true);
	EXPECT_EQ(CreateEventResult1.GetResultCode(), csp::services::EResultCode::Success);
	auto Event1 = CreateEventResult1.GetTicketedEvent();

	auto [CreateEventResult2] = AWAIT_PRE(EventTicketingSystem,
										  CreateTicketedEvent,
										  RequestPredicate,
										  Space.Id,
										  csp::systems::EventTicketingVendor::Eventbrite,
										  TestVendorEventId2,
										  TestVendorEventUri2,
										  false);
	EXPECT_EQ(CreateEventResult2.GetResultCode(), csp::services::EResultCode::Success);
	auto Event2 = CreateEventResult2.GetTicketedEvent();

	bool FoundFirst		 = false;
	bool FoundSecond	 = false;
	bool FoundUnexpected = false;

	auto [GetResult1] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space.Id}, 0, 1);

	EXPECT_EQ(GetResult1.GetResultCode(), csp::services::EResultCode::Success);
	EXPECT_EQ(GetResult1.GetTicketedEvents().Size(), 1);

	auto GetEvent1 = GetResult1.GetTicketedEvents()[0];

	if (GetEvent1.Id == Event1.Id)
	{
		FoundFirst = true;
	}
	else if (GetEvent1.Id == Event2.Id)
	{
		FoundSecond = true;
	}
	else
	{
		FoundUnexpected = true;
	}

	auto [GetResult2] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space.Id}, 1, 1);

	EXPECT_EQ(GetResult2.GetResultCode(), csp::services::EResultCode::Success);
	EXPECT_EQ(GetResult2.GetTicketedEvents().Size(), 1);

	auto GetEvent2 = GetResult2.GetTicketedEvents()[0];

	if (GetEvent2.Id == Event1.Id)
	{
		FoundFirst = true;
	}
	else if (GetEvent2.Id == Event2.Id)
	{
		FoundSecond = true;
	}
	else
	{
		FoundUnexpected = true;
	}

	EXPECT_TRUE(FoundFirst);
	EXPECT_TRUE(FoundSecond);
	EXPECT_FALSE(FoundUnexpected);

	auto [GetResult3] = AWAIT_PRE(EventTicketingSystem, GetTicketedEvents, RequestPredicate, {Space.Id}, 2, 1);

	EXPECT_EQ(GetResult3.GetResultCode(), csp::services::EResultCode::Success);
	EXPECT_EQ(GetResult3.GetTicketedEvents().Size(), 0);

	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETVENDORAUTHORISEINFO_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetVendorAuthoriseInfoTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	auto [TicketedEventVendorAuthInfoResult]
		= AWAIT_PRE(EventTicketingSystem, GetVendorAuthoriseInfo, RequestPredicate, csp::systems::EventTicketingVendor::Eventbrite, UserId);

	EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::services::EResultCode::Success);

	auto VendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

	EXPECT_EQ(VendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
	EXPECT_NE(VendorAuthInfo.ClientId, "");
	EXPECT_NE(VendorAuthInfo.AuthorizeEndpoint, "");
	EXPECT_NE(VendorAuthInfo.OAuthRedirectUrl, "");

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETVENDORAUTHORISEINFO_BADDATA_TEST
CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetVendorAuthoriseInfoBadDataTest)
{
	SetRandSeed();

	auto& SystemsManager	   = csp::systems::SystemsManager::Get();
	auto* UserSystem		   = SystemsManager.GetUserSystem();
	auto* EventTicketingSystem = SystemsManager.GetEventTicketingSystem();

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// 1. Invalid vendor test
	{
		auto [TicketedEventVendorAuthInfoResult]
			= AWAIT_PRE(EventTicketingSystem, GetVendorAuthoriseInfo, RequestPredicate, csp::systems::EventTicketingVendor::Unknown, UserId);

		// specifying an unknown vendor when attempting to get auth info should return a fail and an empty vendor auth info object
		EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::services::EResultCode::Failed);

		const auto VendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

		EXPECT_EQ(VendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Unknown);
		EXPECT_EQ(VendorAuthInfo.ClientId, "");
		EXPECT_EQ(VendorAuthInfo.AuthorizeEndpoint, "");
		EXPECT_EQ(VendorAuthInfo.OAuthRedirectUrl, "");
	}

	// 2. Invalid user ID
	{
		auto [TicketedEventVendorAuthInfoResult] = AWAIT_PRE(EventTicketingSystem,
															 GetVendorAuthoriseInfo,
															 RequestPredicate,
															 csp::systems::EventTicketingVendor::Eventbrite,
															 "n0taR3alC1ien7");

		// specifying an unknown user ID when attempting to get auth info should return a fail and an empty vendor auth info object
		EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::services::EResultCode::Failed);

		const auto VendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

		EXPECT_EQ(VendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Unknown);
		EXPECT_EQ(VendorAuthInfo.ClientId, "");
		EXPECT_EQ(VendorAuthInfo.AuthorizeEndpoint, "");
		EXPECT_EQ(VendorAuthInfo.OAuthRedirectUrl, "");
	}

	// 3. Invalid vendor ID and user ID
	{
		auto [TicketedEventVendorAuthInfoResult] = AWAIT_PRE(EventTicketingSystem,
															 GetVendorAuthoriseInfo,
															 RequestPredicate,
															 csp::systems::EventTicketingVendor::Unknown,
															 "n0taR3alC1ien7");

		// specifying an unknown vendor ID and user ID when attempting to get auth info should return a fail and an empty vendor auth info object
		EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::services::EResultCode::Failed);

		const auto VendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

		EXPECT_EQ(VendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Unknown);
		EXPECT_EQ(VendorAuthInfo.ClientId, "");
		EXPECT_EQ(VendorAuthInfo.AuthorizeEndpoint, "");
		EXPECT_EQ(VendorAuthInfo.OAuthRedirectUrl, "");
	}

	LogOut(UserSystem);
}
#endif