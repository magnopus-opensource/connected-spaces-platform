using Common = Csp.Common;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


#nullable enable annotations


namespace CSPEngine
{
    static class EventTicketingSystemTests
    {
#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_CREATETICKETEDEVENT_ACTIVE_TRUE_TEST
        [Test]
        public static void CreateTicketedEventActiveTrueTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId = "TestVendorEventId";
            string testVendorEventUri = "TestVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var result = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId, testVendorEventUri, true).Result;

            Assert.AreEqual(result.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvent = result.GetTicketedEvent();

            Assert.AreEqual(ticketedEvent.SpaceId, space.Id);
            Assert.AreEqual(ticketedEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(ticketedEvent.VendorEventId, testVendorEventId);
            Assert.AreEqual(ticketedEvent.VendorEventUri, testVendorEventUri);
            Assert.IsTrue(ticketedEvent.IsTicketingActive);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_CREATETICKETEDEVENT_ACTIVE_FALSE_TEST
        [Test]
        public static void CreateTicketedEventActiveFalseTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId = "TestVendorEventId";
            string testVendorEventUri = "TestVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var result = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId, testVendorEventUri, false).Result;

            Assert.AreEqual(result.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvent = result.GetTicketedEvent();

            Assert.AreEqual(ticketedEvent.SpaceId, space.Id);
            Assert.AreEqual(ticketedEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(ticketedEvent.VendorEventId, testVendorEventId);
            Assert.AreEqual(ticketedEvent.VendorEventUri, testVendorEventUri);
            Assert.IsFalse(ticketedEvent.IsTicketingActive);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_CREATETICKETEDEVENT_TWICE_TEST
        [Test]
        public static void CreateTicketedEventTwiceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId1 = "TestVendorEventId1";
            string testVendorEventUri1 = "TestVendorEventUri1";

            string testVendorEventId2 = "TestVendorEventId2";
            string testVendorEventUri2 = "TestVendorEventUri2";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var result1 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId1, testVendorEventUri1, true).Result;

            Assert.AreEqual(result1.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvent1 = result1.GetTicketedEvent();

            Assert.AreEqual(ticketedEvent1.SpaceId, space.Id);
            Assert.AreEqual(ticketedEvent1.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(ticketedEvent1.VendorEventId, testVendorEventId1);
            Assert.AreEqual(ticketedEvent1.VendorEventUri, testVendorEventUri1);
            Assert.IsTrue(ticketedEvent1.IsTicketingActive);

            using var result2 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId2, testVendorEventUri2, false).Result;

            Assert.AreEqual(result2.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvent2 = result2.GetTicketedEvent();

            Assert.AreEqual(ticketedEvent2.SpaceId, space.Id);
            Assert.AreEqual(ticketedEvent2.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(ticketedEvent2.VendorEventId, testVendorEventId2);
            Assert.AreEqual(ticketedEvent2.VendorEventUri, testVendorEventUri2);
            Assert.IsFalse(ticketedEvent2.IsTicketingActive);

            Assert.AreNotEqual(ticketedEvent1.Id, ticketedEvent2.Id);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_UPDATETICKETEDEVENT_TEST
        [Test]
        public static void UpdateTicketedEventTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string initialVendorEventId = "InitialVendorEventId";
            string initialVendorEventUri = "InitialVendorEventUri";

            string updatedVendorEventId = "UpdatedVendorEventId";
            string updatedVendorEventUri = "UpdatedVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, initialVendorEventId, initialVendorEventUri, false).Result;
            Assert.AreEqual(createResult.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var createdEvent = createResult.GetTicketedEvent();

            Assert.AreEqual(createdEvent.SpaceId, space.Id);
            Assert.AreEqual(createdEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(createdEvent.VendorEventId, initialVendorEventId);
            Assert.AreEqual(createdEvent.VendorEventUri, initialVendorEventUri);
            Assert.IsFalse(createdEvent.IsTicketingActive);

            using var updateResult = eventTicketingSystem.UpdateTicketedEvent(space.Id, createdEvent.Id, Csp.Systems.EventTicketingVendor.Eventbrite, updatedVendorEventId, updatedVendorEventUri, true).Result;

            Assert.AreEqual(updateResult.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var updatedEvent = updateResult.GetTicketedEvent();

            Assert.AreEqual(updatedEvent.Id, createdEvent.Id);
            Assert.AreEqual(updatedEvent.SpaceId, space.Id);
            Assert.AreEqual(updatedEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(updatedEvent.VendorEventId, updatedVendorEventId);
            Assert.AreEqual(updatedEvent.VendorEventUri, updatedVendorEventUri);
            Assert.IsTrue(updatedEvent.IsTicketingActive);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_UPDATETICKETEDEVENT_BADSPACE_TEST
        [Test]
        public static void UpdateTicketedEventBadSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string initialVendorEventId = "InitialVendorEventId";
            string initialVendorEventUri = "InitialVendorEventUri";

            string updatedVendorEventId = "UpdatedVendorEventId";
            string updatedVendorEventUri = "UpdatedVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, initialVendorEventId, initialVendorEventUri, false).Result;
            Assert.AreEqual(createResult.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var createdEvent = createResult.GetTicketedEvent();

            Assert.AreEqual(createdEvent.SpaceId, space.Id);
            Assert.AreEqual(createdEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(createdEvent.VendorEventId, initialVendorEventId);
            Assert.AreEqual(createdEvent.VendorEventUri, initialVendorEventUri);
            Assert.IsFalse(createdEvent.IsTicketingActive);

            using var updateResult = eventTicketingSystem.UpdateTicketedEvent("12a345678b9cdd01ef23456a", createdEvent.Id, Csp.Systems.EventTicketingVendor.Eventbrite, updatedVendorEventId, updatedVendorEventUri, true).Result;

            Assert.AreEqual(updateResult.GetResultCode(), Csp.Systems.EResultCode.Failed);
            Assert.AreEqual(updateResult.GetHttpResultCode(), 404);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_UPDATETICKETEDEVENT_BADEVENTID_TEST
        [Test]
        public static void UpdateTicketedEventBadEventIdTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string initialVendorEventId = "InitialVendorEventId";
            string initialVendorEventUri = "InitialVendorEventUri";

            string updatedVendorEventId = "UpdatedVendorEventId";
            string updatedVendorEventUri = "UpdatedVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, initialVendorEventId, initialVendorEventUri, false).Result;
            Assert.AreEqual(createResult.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var createdEvent = createResult.GetTicketedEvent();

            Assert.AreEqual(createdEvent.SpaceId, space.Id);
            Assert.AreEqual(createdEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(createdEvent.VendorEventId, initialVendorEventId);
            Assert.AreEqual(createdEvent.VendorEventUri, initialVendorEventUri);
            Assert.IsFalse(createdEvent.IsTicketingActive);

            using var updateResult = eventTicketingSystem.UpdateTicketedEvent(space.Id, "12a345678b9cdd01ef23456a", Csp.Systems.EventTicketingVendor.Eventbrite, updatedVendorEventId, updatedVendorEventUri, true).Result;

            Assert.AreEqual(updateResult.GetResultCode(), Csp.Systems.EResultCode.Failed);
            Assert.AreEqual(updateResult.GetHttpResultCode(), 404);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_NO_EVENTS_TES
        [Test]
        public static void GetTicketedEventsNoEventsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var spaceIds = new Csp.Common.Array<string>(1);
            spaceIds[0] = space.Id;

            using var result = eventTicketingSystem.GetTicketedEvents(spaceIds, null, null).Result;

            Assert.AreEqual(result.GetResultCode(), Csp.Systems.EResultCode.Success);
            Assert.AreEqual(result.GetTicketedEvents().Size(), 0UL);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_ONE_EVENT_TEST
        [Test]
        public static void GetTicketedEventsOneEventTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId = "TestVendorEventId";
            string testVendorEventUri = "TestVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId, testVendorEventUri, true).Result;

            Assert.AreEqual(createResult.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent = createResult.GetTicketedEvent();
           
            using var spaceIds = new Csp.Common.Array<string>(1);
            spaceIds[0] = space.Id;

            using var getResult = eventTicketingSystem.GetTicketedEvents(spaceIds, null, null).Result;
            Assert.AreEqual(getResult.GetResultCode(), Csp.Systems.EResultCode.Success);
            
            using var ticketedEvents = getResult.GetTicketedEvents();

            Assert.AreEqual(ticketedEvents.Size(), 1UL);

            using var getEvent = ticketedEvents[0];

            Assert.AreEqual(getEvent.Id, createdEvent.Id);
            Assert.AreEqual(getEvent.SpaceId, space.Id);
            Assert.AreEqual(getEvent.Vendor, createdEvent.Vendor);
            Assert.AreEqual(getEvent.VendorEventId, createdEvent.VendorEventId);
            Assert.AreEqual(getEvent.VendorEventUri, createdEvent.VendorEventUri);
            Assert.AreEqual(getEvent.IsTicketingActive, createdEvent.IsTicketingActive);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_TWO_EVENTS_SAME_SPACE_TEST
        [Test]
        public static void GetTicketedEventsTwoEventsSameSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId1 = "TestVendorEventId1";
            string testVendorEventUri1 = "TestVendorEventUri1";

            string testVendorEventId2 = "TestVendorEventId2";
            string testVendorEventUri2 = "TestVendorEventUri2";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult1 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId1, testVendorEventUri1, true).Result;
            Assert.AreEqual(createResult1.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent1 = createResult1.GetTicketedEvent();

            using var createdResult2 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId2, testVendorEventUri2, false).Result;
            Assert.AreEqual(createdResult2.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent2 = createdResult2.GetTicketedEvent();

            using var spaceIds = new Csp.Common.Array<string>(1);
            spaceIds[0] = space.Id;

            using var getResult = eventTicketingSystem.GetTicketedEvents(spaceIds, null, null).Result;
            Assert.AreEqual(getResult.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvents = getResult.GetTicketedEvents();

            Assert.AreEqual(ticketedEvents.Size(), 2UL);

            bool foundFirst = false;
            bool foundSecond = false;
            bool foundUnexpected = false;

            for (var i = 0UL; i < ticketedEvents.Size(); ++i)
            {
                using var getEvent = ticketedEvents[i];
                Assert.AreEqual(getEvent.SpaceId, space.Id);
                Assert.AreEqual(getEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);

                if (getEvent.Id == createdEvent1.Id)
                {
                    Assert.AreEqual(getEvent.VendorEventId, testVendorEventId1);
                    Assert.AreEqual(getEvent.VendorEventUri, testVendorEventUri1);
                    Assert.IsTrue(getEvent.IsTicketingActive);
                    foundFirst = true;
                }
                else if (getEvent.Id == createdEvent2.Id)
                {
                    Assert.AreEqual(getEvent.VendorEventId, testVendorEventId2);
                    Assert.AreEqual(getEvent.VendorEventUri, testVendorEventUri2);
                    Assert.IsFalse(getEvent.IsTicketingActive);
                    foundSecond = true;
                }
                else
                {
                    foundUnexpected = true;
                }

            }

            Assert.IsTrue(foundFirst);
            Assert.IsTrue(foundSecond);
            Assert.IsFalse(foundUnexpected);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_TWO_EVENTS_TWO_SPACES_TEST
        [Test]
        public static void GetTicketedEventsTwoEventsTwoSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName1 = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceName2 = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId1 = "TestVendorEventId1";
            string testVendorEventUri1 = "TestVendorEventUri1";

            string testVendorEventId2 = "TestVendorEventId2";
            string testVendorEventUri2 = "TestVendorEventUri2";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space1 = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName1, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);
            var space2 = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName2, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult1 = eventTicketingSystem.CreateTicketedEvent(space1.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId1, testVendorEventUri1, true).Result;
            Assert.AreEqual(createResult1.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent1 = createResult1.GetTicketedEvent();

            using var createdResult2 = eventTicketingSystem.CreateTicketedEvent(space2.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId2, testVendorEventUri2, false).Result;
            Assert.AreEqual(createdResult2.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent2 = createdResult2.GetTicketedEvent();

            using var spaceIds = new Csp.Common.Array<string>(2);
            spaceIds[0] = space1.Id;
            spaceIds[1] = space2.Id;

            using var getResult = eventTicketingSystem.GetTicketedEvents(spaceIds, null, null).Result;
            Assert.AreEqual(getResult.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvents = getResult.GetTicketedEvents();

            Assert.AreEqual(ticketedEvents.Size(), 2UL);

            bool foundFirst = false;
            bool foundSecond = false;
            bool foundUnexpected = false;

            for (var i = 0UL; i < ticketedEvents.Size(); ++i)
            {
                using var getEvent = ticketedEvents[i];
                Assert.AreEqual(getEvent.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);

                if (getEvent.Id == createdEvent1.Id)
                {
                    Assert.AreEqual(getEvent.SpaceId, space1.Id);
                    Assert.AreEqual(getEvent.VendorEventId, testVendorEventId1);
                    Assert.AreEqual(getEvent.VendorEventUri, testVendorEventUri1);
                    Assert.IsTrue(getEvent.IsTicketingActive);
                    foundFirst = true;
                }
                else if (getEvent.Id == createdEvent2.Id)
                {
                    Assert.AreEqual(getEvent.SpaceId, space2.Id);
                    Assert.AreEqual(getEvent.VendorEventId, testVendorEventId2);
                    Assert.AreEqual(getEvent.VendorEventUri, testVendorEventUri2);
                    Assert.IsFalse(getEvent.IsTicketingActive);
                    foundSecond = true;
                }
                else
                {
                    foundUnexpected = true;
                }

            }

            Assert.IsTrue(foundFirst);
            Assert.IsTrue(foundSecond);
            Assert.IsFalse(foundUnexpected);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_EVENTTICKETING_TESTS || RUN_EVENTTICKETING_GETTICKETEDEVENTS_PAGINATION_TEST
        [Test]
        public static void GetTicketedEventsPaginationTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId1 = "TestVendorEventId1";
            string testVendorEventUri1 = "TestVendorEventUri1";

            string testVendorEventId2 = "TestVendorEventId2";
            string testVendorEventUri2 = "TestVendorEventUri2";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var createResult1 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId1, testVendorEventUri1, true).Result;
            Assert.AreEqual(createResult1.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent1 = createResult1.GetTicketedEvent();

            using var createdResult2 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId2, testVendorEventUri2, false).Result;
            Assert.AreEqual(createdResult2.GetResultCode(), Csp.Systems.EResultCode.Success);
            using var createdEvent2 = createdResult2.GetTicketedEvent();

            bool foundFirst = false;
            bool foundSecond = false;
            bool foundUnexpected = false;

            using var spaceIds = new Csp.Common.Array<string>(1);
            spaceIds[0] = space.Id;

            using var getResult1 = eventTicketingSystem.GetTicketedEvents(spaceIds, 0, 1).Result;
            Assert.AreEqual(getResult1.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvents1 = getResult1.GetTicketedEvents();

            Assert.AreEqual(ticketedEvents1.Size(), 1UL);


            using var getEvent1 = ticketedEvents1[0];

            if (getEvent1.Id == createdEvent1.Id)
            {
                foundFirst = true;
            }
            else if (getEvent1.Id == createdEvent2.Id)
            {
                foundSecond = true;
            }
            else
            {
                foundUnexpected = true;
            }

            using var getResult2 = eventTicketingSystem.GetTicketedEvents(spaceIds, 1, 1).Result;
            Assert.AreEqual(getResult2.GetResultCode(), Csp.Systems.EResultCode.Success);

            using var ticketedEvents2 = getResult2.GetTicketedEvents();

            Assert.AreEqual(ticketedEvents2.Size(), 1UL);


            using var getEvent2 = ticketedEvents2[0];

            if (getEvent2.Id == createdEvent1.Id)
            {
                foundFirst = true;
            }
            else if (getEvent2.Id == createdEvent2.Id)
            {
                foundSecond = true;
            }
            else
            {
                foundUnexpected = true;
            }

            Assert.IsTrue(foundFirst);
            Assert.IsTrue(foundSecond);
            Assert.IsFalse(foundUnexpected);
        }
#endif

    }
}

