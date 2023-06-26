using Services = Csp.Services;

using CSharpTests;
using static CSharpTests.TestHelper;

namespace CSPEngine
{
    static class MaintenanceSystemTests
    {

#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_GETMAINTENANCEINFO_TEST
        [Test]
        public static void GetMaintenanceInfoTest()
        {
            GetFoundationSystems(out _, out _, out _, out _, out _, out _, out _, out var maintenanceSystem);

            using var result = maintenanceSystem.GetMaintenanceInfo().Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            using var MaintenanceInfoResponses = result.GetMaintenanceInfoResponses();
            Assert.AreEqual(MaintenanceInfoResponses.Size(), 1UL);

            Assert.AreEqual(result.GetMaintenanceInfoResponses()[0].Description, "Example downtime for a Saturday at 2am PST");
            Assert.AreEqual(result.GetMaintenanceInfoResponses()[0].StartDateTimestamp, "2022-04-30T02:00:00+0000");
            Assert.AreEqual(result.GetMaintenanceInfoResponses()[0].EndDateTimestamp, "2022-04-30T03:00:00+0000");

        }
#endif
        
#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_GETINSIDEMAINTENANCEINFO_TEST
        [Test]
        public static void GetInsideMaintenanceInfoTest()
        {
            GetFoundationSystems(out _, out _, out _, out _, out _, out _, out _, out var maintenanceSystem);

            using var result = maintenanceSystem.IsInsideMaintenanceWindow().Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            using var InsideMaintenanceInfoResponse = result.GetInsideMaintenanceInfo();
            Assert.IsFalse(InsideMaintenanceInfoResponse.IsInsideMaintenanceWindow);

            Assert.AreEqual(InsideMaintenanceInfoResponse.Description, "Example downtime for a Saturday at 2am PST");
            Assert.AreEqual(InsideMaintenanceInfoResponse.StartDateTimestamp, "2022-04-30T02:00:00+0000");
            Assert.AreEqual(InsideMaintenanceInfoResponse.EndDateTimestamp, "2022-04-30T03:00:00+0000");

        }
#endif
    }
}