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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/EventTicketing/EventTicketingSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::systems;

// Dummy EventBrite-formatted data used throughout the tests.
static const csp::common::String TestVendorEventId = "123456789123";
static const csp::common::String TestVendorEventUri = "https://www.eventbrite.com/e/csp-test-event-tickets-123456789123";

static const csp::common::String AlternativeTestVendorEventId = "234567891234";
static const csp::common::String AlternativeTestVendorEventUri = "https://www.eventbrite.com/e/csp-test-event-tickets-234567891234";

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventActiveTrueTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [TicketedEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);

    EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto event = TicketedEventResult.GetTicketedEvent();

    EXPECT_EQ(event.SpaceId, space.Id);
    EXPECT_EQ(event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(event.VendorEventId, TestVendorEventId);
    EXPECT_EQ(event.VendorEventUri, TestVendorEventUri);
    EXPECT_TRUE(event.IsTicketingActive);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventActiveFalseTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [TicketedEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, false);

    EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto event = TicketedEventResult.GetTicketedEvent();

    EXPECT_EQ(event.SpaceId, space.Id);
    EXPECT_EQ(event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(event.VendorEventId, TestVendorEventId);
    EXPECT_EQ(event.VendorEventUri, TestVendorEventUri);
    EXPECT_FALSE(event.IsTicketingActive);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, CreateTicketedEventTwiceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [TicketedEventResult1] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);

    EXPECT_EQ(TicketedEventResult1.GetResultCode(), csp::systems::EResultCode::Success);

    auto event1 = TicketedEventResult1.GetTicketedEvent();

    EXPECT_EQ(event1.SpaceId, space.Id);
    EXPECT_EQ(event1.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(event1.VendorEventId, TestVendorEventId);
    EXPECT_EQ(event1.VendorEventUri, TestVendorEventUri);
    EXPECT_TRUE(event1.IsTicketingActive);

    auto [TicketedEventResult2] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, false);

    EXPECT_EQ(TicketedEventResult1.GetResultCode(), csp::systems::EResultCode::Success);

    auto event2 = TicketedEventResult2.GetTicketedEvent();

    EXPECT_EQ(event2.SpaceId, space.Id);
    EXPECT_EQ(event2.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(event2.VendorEventId, AlternativeTestVendorEventId);
    EXPECT_EQ(event2.VendorEventUri, AlternativeTestVendorEventUri);
    EXPECT_FALSE(event2.IsTicketingActive);

    EXPECT_NE(event1.Id, event2.Id);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, UpdateTicketEventTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreatedResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, false);

    EXPECT_EQ(CreatedResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdEvent = CreatedResult.GetTicketedEvent();

    EXPECT_EQ(createdEvent.SpaceId, space.Id);
    EXPECT_EQ(createdEvent.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(createdEvent.VendorEventId, TestVendorEventId);
    EXPECT_EQ(createdEvent.VendorEventUri, TestVendorEventUri);
    EXPECT_FALSE(createdEvent.IsTicketingActive);

    auto [UpdatedResult] = AWAIT_PRE(eventTicketingSystem, UpdateTicketedEvent, RequestPredicate, space.Id, createdEvent.Id,
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, true);

    EXPECT_EQ(UpdatedResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto updatedEvent = UpdatedResult.GetTicketedEvent();

    EXPECT_EQ(updatedEvent.Id, createdEvent.Id);
    EXPECT_EQ(updatedEvent.SpaceId, space.Id);
    EXPECT_EQ(updatedEvent.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(updatedEvent.VendorEventId, AlternativeTestVendorEventId);
    EXPECT_EQ(updatedEvent.VendorEventUri, AlternativeTestVendorEventUri);
    EXPECT_TRUE(updatedEvent.IsTicketingActive);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, UpdateTicketEventBadSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreatedResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, false);

    EXPECT_EQ(CreatedResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdEvent = CreatedResult.GetTicketedEvent();

    auto [UpdatedResult] = AWAIT_PRE(eventTicketingSystem, UpdateTicketedEvent, RequestPredicate, "12a345678b9cdd01ef23456a", createdEvent.Id,
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, true);

    EXPECT_EQ(UpdatedResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdatedResult.GetHttpResultCode(), 404);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, UpdateTicketEventBadEventIdTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreatedResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, false);

    EXPECT_EQ(CreatedResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto createdEvent = CreatedResult.GetTicketedEvent();

    auto [UpdatedResult] = AWAIT_PRE(eventTicketingSystem, UpdateTicketedEvent, RequestPredicate, space.Id, "12a345678b9cdd01ef23456a",
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, true);

    EXPECT_EQ(UpdatedResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(UpdatedResult.GetHttpResultCode(), 404);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsNoEventsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [Result] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space.Id }, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetTicketedEvents().Size(), 0);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsOneEventTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreateEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, false);
    EXPECT_EQ(CreateEventResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space.Id }, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetTicketedEvents().Size(), 1);

    auto event = Result.GetTicketedEvents()[0];

    EXPECT_EQ(event.Id, CreateEventResult.GetTicketedEvent().Id);
    EXPECT_EQ(event.SpaceId, space.Id);
    EXPECT_EQ(event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(event.VendorEventId, TestVendorEventId);
    EXPECT_EQ(event.VendorEventUri, TestVendorEventUri);
    EXPECT_FALSE(event.IsTicketingActive);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetIsSpaceTicketedTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreateEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);
    EXPECT_EQ(CreateEventResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result] = AWAIT_PRE(eventTicketingSystem, GetIsSpaceTicketed, RequestPredicate, space.Id);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_TRUE(Result.GetIsTicketedEvent());

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetIsSpaceTicketedFailureTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    // Test for IsTicketedEvent prior to creating an event, ensure it returns false
    auto [Result] = AWAIT_PRE(eventTicketingSystem, GetIsSpaceTicketed, RequestPredicate, space.Id);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_FALSE(Result.GetIsTicketedEvent());

    auto [CreateEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, false);
    EXPECT_EQ(CreateEventResult.GetResultCode(), csp::systems::EResultCode::Success);

    // Test for IsTicketedEvent post creating an event, but with ticketing disabled, ensure it returns false
    auto [SecondResult] = AWAIT_PRE(eventTicketingSystem, GetIsSpaceTicketed, RequestPredicate, space.Id);

    EXPECT_EQ(SecondResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_FALSE(SecondResult.GetIsTicketedEvent());

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsTwoEventsSameSpaceTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreateEventResult1] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);
    EXPECT_EQ(CreateEventResult1.GetResultCode(), csp::systems::EResultCode::Success);
    auto event1 = CreateEventResult1.GetTicketedEvent();

    auto [CreateEventResult2] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, false);
    EXPECT_EQ(CreateEventResult2.GetResultCode(), csp::systems::EResultCode::Success);
    auto event2 = CreateEventResult2.GetTicketedEvent();

    auto [Result] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space.Id }, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto events = Result.GetTicketedEvents();

    EXPECT_EQ(Result.GetTicketedEvents().Size(), 2);

    bool foundFirst = false;
    bool foundSecond = false;
    bool foundUnexpected = false;

    for (size_t i = 0; i < events.Size(); ++i)
    {
        auto event = Result.GetTicketedEvents()[i];

        EXPECT_EQ(event.SpaceId, space.Id);
        EXPECT_EQ(event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);

        if (event.Id == event1.Id)
        {
            EXPECT_EQ(event.VendorEventId, TestVendorEventId);
            EXPECT_EQ(event.VendorEventUri, TestVendorEventUri);
            EXPECT_TRUE(event.IsTicketingActive);
            foundFirst = true;
        }
        else if (event.Id == event2.Id)
        {
            EXPECT_EQ(event.VendorEventId, AlternativeTestVendorEventId);
            EXPECT_EQ(event.VendorEventUri, AlternativeTestVendorEventUri);
            EXPECT_FALSE(event.IsTicketingActive);
            foundSecond = true;
        }
        else
        {
            foundUnexpected = true;
        }
    }

    EXPECT_TRUE(foundFirst);
    EXPECT_TRUE(foundSecond);
    EXPECT_FALSE(foundUnexpected);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsTwoEventsTwoSpacesTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName1[256];
    SPRINTF(uniqueSpaceName1, "%s-%s", testSpaceName, GetUniqueString().c_str());
    char uniqueSpaceName2[256];
    SPRINTF(uniqueSpaceName2, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space1;
    CreateSpace(
        spaceSystem, uniqueSpaceName1, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space1);
    csp::systems::Space space2;
    CreateSpace(
        spaceSystem, uniqueSpaceName2, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space2);

    auto [CreateEventResult1] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space1.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);
    EXPECT_EQ(CreateEventResult1.GetResultCode(), csp::systems::EResultCode::Success);
    auto event1 = CreateEventResult1.GetTicketedEvent();

    auto [CreateEventResult2] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space2.Id,
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, false);
    EXPECT_EQ(CreateEventResult2.GetResultCode(), csp::systems::EResultCode::Success);
    auto event2 = CreateEventResult2.GetTicketedEvent();

    auto [Result] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space1.Id, space2.Id }, nullptr, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto events = Result.GetTicketedEvents();

    EXPECT_EQ(Result.GetTicketedEvents().Size(), 2);

    bool foundFirst = false;
    bool foundSecond = false;
    bool foundUnexpected = false;

    for (size_t i = 0; i < events.Size(); ++i)
    {
        auto event = Result.GetTicketedEvents()[i];

        EXPECT_EQ(event.Vendor, csp::systems::EventTicketingVendor::Eventbrite);

        if (event.Id == event1.Id)
        {
            EXPECT_EQ(event.SpaceId, space1.Id);
            EXPECT_EQ(event.VendorEventId, TestVendorEventId);
            EXPECT_EQ(event.VendorEventUri, TestVendorEventUri);
            EXPECT_TRUE(event.IsTicketingActive);
            foundFirst = true;
        }
        else if (event.Id == event2.Id)
        {
            EXPECT_EQ(event.SpaceId, space2.Id);
            EXPECT_EQ(event.VendorEventId, AlternativeTestVendorEventId);
            EXPECT_EQ(event.VendorEventUri, AlternativeTestVendorEventUri);
            EXPECT_FALSE(event.IsTicketingActive);
            foundSecond = true;
        }
        else
        {
            foundUnexpected = true;
        }
    }

    EXPECT_TRUE(foundFirst);
    EXPECT_TRUE(foundSecond);
    EXPECT_FALSE(foundUnexpected);

    DeleteSpace(spaceSystem, space1.Id);
    DeleteSpace(spaceSystem, space2.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetTicketedEventsPaginationTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [CreateEventResult1] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);
    EXPECT_EQ(CreateEventResult1.GetResultCode(), csp::systems::EResultCode::Success);
    auto event1 = CreateEventResult1.GetTicketedEvent();

    auto [CreateEventResult2] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, AlternativeTestVendorEventId, AlternativeTestVendorEventUri, false);
    EXPECT_EQ(CreateEventResult2.GetResultCode(), csp::systems::EResultCode::Success);
    auto event2 = CreateEventResult2.GetTicketedEvent();

    bool foundFirst = false;
    bool foundSecond = false;
    bool foundUnexpected = false;

    auto [GetResult1] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space.Id }, 0, 1);

    EXPECT_EQ(GetResult1.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetResult1.GetTicketedEvents().Size(), 1);

    auto getEvent1 = GetResult1.GetTicketedEvents()[0];

    if (getEvent1.Id == event1.Id)
    {
        foundFirst = true;
    }
    else if (getEvent1.Id == event2.Id)
    {
        foundSecond = true;
    }
    else
    {
        foundUnexpected = true;
    }

    auto [GetResult2] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space.Id }, 1, 1);

    EXPECT_EQ(GetResult2.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetResult2.GetTicketedEvents().Size(), 1);

    auto getEvent2 = GetResult2.GetTicketedEvents()[0];

    if (getEvent2.Id == event1.Id)
    {
        foundFirst = true;
    }
    else if (getEvent2.Id == event2.Id)
    {
        foundSecond = true;
    }
    else
    {
        foundUnexpected = true;
    }

    EXPECT_TRUE(foundFirst);
    EXPECT_TRUE(foundSecond);
    EXPECT_FALSE(foundUnexpected);

    auto [GetResult3] = AWAIT_PRE(eventTicketingSystem, GetTicketedEvents, RequestPredicate, { space.Id }, 2, 1);

    EXPECT_EQ(GetResult3.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(GetResult3.GetTicketedEvents().Size(), 0);

    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetVendorAuthorizeInfoTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [TicketedEventVendorAuthInfoResult]
        = AWAIT_PRE(eventTicketingSystem, GetVendorAuthorizeInfo, RequestPredicate, csp::systems::EventTicketingVendor::Eventbrite, userId);

    EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto vendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

    EXPECT_EQ(vendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_NE(vendorAuthInfo.ClientId, "");
    EXPECT_NE(vendorAuthInfo.AuthorizeEndpoint, "");
    EXPECT_NE(vendorAuthInfo.OAuthRedirectUrl, "");

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, EventTicketingSystemTests, GetVendorAuthorizeInfoBadDataTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // 1. Invalid vendor test
    {
        auto [TicketedEventVendorAuthInfoResult]
            = AWAIT_PRE(eventTicketingSystem, GetVendorAuthorizeInfo, RequestPredicate, csp::systems::EventTicketingVendor::Unknown, userId);

        // specifying an unknown vendor when attempting to get auth info should return a fail and an empty vendor auth info object
        EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::systems::EResultCode::Failed);

        const auto vendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

        EXPECT_EQ(vendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Unknown);
        EXPECT_EQ(vendorAuthInfo.ClientId, "");
        EXPECT_EQ(vendorAuthInfo.AuthorizeEndpoint, "");
        EXPECT_EQ(vendorAuthInfo.OAuthRedirectUrl, "");
    }

    // 2. Invalid user ID
    {
        auto [TicketedEventVendorAuthInfoResult] = AWAIT_PRE(
            eventTicketingSystem, GetVendorAuthorizeInfo, RequestPredicate, csp::systems::EventTicketingVendor::Eventbrite, "n0taR3alC1ien7");

        // specifying an unknown user ID when attempting to get auth info should return a fail and an empty vendor auth info object
        EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::systems::EResultCode::Failed);

        const auto vendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

        EXPECT_EQ(vendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Unknown);
        EXPECT_EQ(vendorAuthInfo.ClientId, "");
        EXPECT_EQ(vendorAuthInfo.AuthorizeEndpoint, "");
        EXPECT_EQ(vendorAuthInfo.OAuthRedirectUrl, "");
    }

    // 3. Invalid vendor ID and user ID
    {
        auto [TicketedEventVendorAuthInfoResult] = AWAIT_PRE(
            eventTicketingSystem, GetVendorAuthorizeInfo, RequestPredicate, csp::systems::EventTicketingVendor::Unknown, "n0taR3alC1ien7");

        // specifying an unknown vendor ID and user ID when attempting to get auth info should return a fail and an empty vendor auth info object
        EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::systems::EResultCode::Failed);

        const auto vendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

        EXPECT_EQ(vendorAuthInfo.Vendor, csp::systems::EventTicketingVendor::Unknown);
        EXPECT_EQ(vendorAuthInfo.ClientId, "");
        EXPECT_EQ(vendorAuthInfo.AuthorizeEndpoint, "");
        EXPECT_EQ(vendorAuthInfo.OAuthRedirectUrl, "");
    }

    LogOut(userSystem);
}

// This test currently requires manual steps and will be reviewed as part of OF-1535.
/*
 * This test is disabled by default as it requires human interaction but is provided as a means to test ticket redemption.
 *
 * To run this test we need an actual Eventbrite event and ticket and we need to pause halfway through. The tickets
 * must be created with different users.
 *
 * Create the Eventbrite event with an account using an email.
 *
 * Get a ticket for that event with an account using a different email.
 *
 * When you have those fill in the values for TestVendorEventId, TestVendorEventUri and TestVendorTicketId.
 *
 * When running the test add a breakpoint after the line that logs VendorAuthInfo.AuthorizeEndpoint but before
 * the line that runs EventTicketingSystem.SubmitEventTicket. In a browser, ensure that you are logged in as the
 * event creator and copy/paste the auth endpoint and click Allow when it loads. Once this is done, continue with
 * the test.
 *
 * When done testing, make sure to delete the event in Eventbrite.
 */
CSP_PUBLIC_TEST(DISABLED_CSPEngine, EventTicketingSystemTests, SubmitEventTicketTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    csp::common::String testVendorTicketId = "7307631489";

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in as attendee just to get their UserId and log out again
    csp::common::String eventAttendeeUserId;
    csp::systems::Profile eventAttendee = CreateTestUser();
    LogIn(userSystem, eventAttendeeUserId, eventAttendee.Email, GeneratedTestAccountPassword);
    LogOut(userSystem);

    // Log in as the creator
    csp::common::String eventCreatorUserId;
    csp::systems::Profile eventCreator = CreateTestUser();
    LogIn(userSystem, eventCreatorUserId, eventCreator.Email, GeneratedTestAccountPassword);

    csp::systems::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

    // Add the attendee to the space
    auto [AddUserToSpaceResult] = AWAIT_PRE(spaceSystem, AddUserToSpace, RequestPredicate, space.Id, eventAttendeeUserId);
    EXPECT_EQ(AddUserToSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);

    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "SpaceId: %s", space.Id.c_str());
    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "CreatorUserId: %s", eventCreatorUserId.c_str());
    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "AttendeeUserId: %s", eventAttendeeUserId.c_str());

    auto [TicketedEventVendorAuthInfoResult] = AWAIT_PRE(
        eventTicketingSystem, GetVendorAuthorizeInfo, RequestPredicate, csp::systems::EventTicketingVendor::Eventbrite, eventCreatorUserId);
    EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto vendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "Login to Eventbrite as the event creator and paste the following URL into your browser: %s",
        vendorAuthInfo.AuthorizeEndpoint.c_str());

    auto [TicketedEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);

    EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto event = TicketedEventResult.GetTicketedEvent();

    // Log out as the creator
    LogOut(userSystem);

    // Log in as the attendee
    LogIn(userSystem, eventAttendeeUserId, eventAttendee.Email, GeneratedTestAccountPassword);

    auto [SubmitEventTicketResult] = AWAIT_PRE(eventTicketingSystem, SubmitEventTicket, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, testVendorTicketId, nullptr);

    EXPECT_EQ(SubmitEventTicketResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto submittedEventTicket = SubmitEventTicketResult.GetEventTicket();

    EXPECT_EQ(submittedEventTicket.SpaceId, space.Id);
    EXPECT_EQ(submittedEventTicket.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(submittedEventTicket.VendorEventId, TestVendorEventId);
    EXPECT_EQ(submittedEventTicket.VendorTicketId, testVendorTicketId);
    EXPECT_EQ(submittedEventTicket.Status, csp::systems::TicketStatus::Redeemed);
    EXPECT_EQ(submittedEventTicket.UserId, eventAttendeeUserId);
    EXPECT_EQ(submittedEventTicket.Email, eventAttendee.Email);

    // Log out as the attendee
    LogOut(userSystem);

    LogIn(userSystem, eventCreatorUserId, eventCreator.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}

// This test currently requires manual steps and will be reviewed as part of OF-1535.
/*
 * This test is disabled by default and works the same as the previous test with one difference in that the ticket
 * is submitted by the superuser on behalf of the alternative user.
 */
CSP_PUBLIC_TEST(DISABLED_CSPEngine, EventTicketingSystemTests, SubmitEventTicketOnBehalfOfTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();
    auto* eventTicketingSystem = systemsManager.GetEventTicketingSystem();

    csp::common::String testVendorTicketId = "7307701069";

    const char* testSpaceName = "CSP-UNITTEST-SPACE";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in as attendee just to get their UserId and log out again
    csp::common::String eventAttendeeUserId;
    csp::systems::Profile eventAttendee = CreateTestUser();
    LogIn(userSystem, eventAttendeeUserId, eventAttendee.Email, GeneratedTestAccountPassword);
    LogOut(userSystem);

    // Log in as the creator
    csp::common::String eventCreatorUserId;
    csp::systems::Profile eventCreator = CreateTestUser();
    LogIn(userSystem, eventCreatorUserId, eventCreator.Email, GeneratedTestAccountPassword);

    csp::systems::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Public, nullptr, nullptr, nullptr, nullptr, space);

    // Add the attendee to the space
    auto [AddUserToSpaceResult] = AWAIT_PRE(spaceSystem, AddUserToSpace, RequestPredicate, space.Id, eventAttendeeUserId);
    EXPECT_EQ(AddUserToSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);

    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "SpaceId: %s", space.Id.c_str());
    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "CreatorUserId: %s", eventCreatorUserId.c_str());
    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "AttendeeUserId: %s", eventAttendeeUserId.c_str());

    auto [TicketedEventVendorAuthInfoResult] = AWAIT_PRE(
        eventTicketingSystem, GetVendorAuthorizeInfo, RequestPredicate, csp::systems::EventTicketingVendor::Eventbrite, eventCreatorUserId);
    EXPECT_EQ(TicketedEventVendorAuthInfoResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto vendorAuthInfo = TicketedEventVendorAuthInfoResult.GetVendorAuthInfo();

    CSP_LOG_FORMAT(csp::common::LogLevel::Display, "Login to Eventbrite as the event creator and paste the following URL into your browser: %s",
        vendorAuthInfo.AuthorizeEndpoint.c_str());

    auto [TicketedEventResult] = AWAIT_PRE(eventTicketingSystem, CreateTicketedEvent, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, TestVendorEventUri, true);

    EXPECT_EQ(TicketedEventResult.GetResultCode(), csp::systems::EResultCode::Success);
    auto event = TicketedEventResult.GetTicketedEvent();

    auto [SubmitEventTicketResult] = AWAIT_PRE(eventTicketingSystem, SubmitEventTicket, RequestPredicate, space.Id,
        csp::systems::EventTicketingVendor::Eventbrite, TestVendorEventId, testVendorTicketId, eventAttendeeUserId);

    EXPECT_EQ(SubmitEventTicketResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto submittedEventTicket = SubmitEventTicketResult.GetEventTicket();

    EXPECT_EQ(submittedEventTicket.SpaceId, space.Id);
    EXPECT_EQ(submittedEventTicket.Vendor, csp::systems::EventTicketingVendor::Eventbrite);
    EXPECT_EQ(submittedEventTicket.VendorEventId, TestVendorEventId);
    EXPECT_EQ(submittedEventTicket.VendorTicketId, testVendorTicketId);
    EXPECT_EQ(submittedEventTicket.Status, csp::systems::TicketStatus::Redeemed);
    EXPECT_EQ(submittedEventTicket.UserId, eventAttendeeUserId);
    EXPECT_EQ(submittedEventTicket.Email, eventAttendee.Email);

    // Log out as the attendee
    LogOut(userSystem);

    LogIn(userSystem, eventCreatorUserId, eventCreator.Email, GeneratedTestAccountPassword);
    DeleteSpace(spaceSystem, space.Id);
    LogOut(userSystem);
}
