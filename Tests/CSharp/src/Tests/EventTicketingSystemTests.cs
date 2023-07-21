using Common = Csp.Common;
using Services = Csp.Services;
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
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId = "TestVendorEventId";
            string testVendorEventUri = "TestVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var result = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId, testVendorEventUri, true).Result;

            Assert.AreEqual(result.GetResultCode(), Csp.Services.EResultCode.Success);

            var ticketedEvent = result.GetTicketedEvent();

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
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId = "TestVendorEventId";
            string testVendorEventUri = "TestVendorEventUri";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var result = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId, testVendorEventUri, false).Result;

            Assert.AreEqual(result.GetResultCode(), Csp.Services.EResultCode.Success);

            var ticketedEvent = result.GetTicketedEvent();

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
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out var eventTicketingSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";

            string testVendorEventId1 = "TestVendorEventId1";
            string testVendorEventUri1 = "TestVendorEventUri1";

            string testVendorEventId2 = "TestVendorEventId2";
            string testVendorEventUri2 = "TestVendorEventUri2";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var result1 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId1, testVendorEventUri1, true).Result;

            Assert.AreEqual(result1.GetResultCode(), Csp.Services.EResultCode.Success);

            var ticketedEvent1 = result1.GetTicketedEvent();

            Assert.AreEqual(ticketedEvent1.SpaceId, space.Id);
            Assert.AreEqual(ticketedEvent1.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(ticketedEvent1.VendorEventId, testVendorEventId1);
            Assert.AreEqual(ticketedEvent1.VendorEventUri, testVendorEventUri1);
            Assert.IsTrue(ticketedEvent1.IsTicketingActive);

            var result2 = eventTicketingSystem.CreateTicketedEvent(space.Id, Csp.Systems.EventTicketingVendor.Eventbrite, testVendorEventId2, testVendorEventUri2, false).Result;

            Assert.AreEqual(result1.GetResultCode(), Csp.Services.EResultCode.Success);

            var ticketedEvent2 = result2.GetTicketedEvent();

            Assert.AreEqual(ticketedEvent2.SpaceId, space.Id);
            Assert.AreEqual(ticketedEvent2.Vendor, Csp.Systems.EventTicketingVendor.Eventbrite);
            Assert.AreEqual(ticketedEvent2.VendorEventId, testVendorEventId2);
            Assert.AreEqual(ticketedEvent2.VendorEventUri, testVendorEventUri2);
            Assert.IsFalse(ticketedEvent2.IsTicketingActive);

            Assert.AreNotEqual(ticketedEvent1.Id, ticketedEvent2.Id);
        }
#endif
    }
}

