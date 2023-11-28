using Systems = Csp.Systems;

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
            GetFoundationSystems(out _, out _, out _, out _, out _, out _, out _, out var maintenanceSystem, out _, out _);

            using var result = maintenanceSystem.GetMaintenanceInfo().Result;
            Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Success);

        }
#endif
        
#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_GETINSIDEMAINTENANCEINFO_TEST
        [Test]
        public static void GetInsideMaintenanceInfoTest()
        {
            GetFoundationSystems(out _, out _, out _, out _, out _, out _, out _, out var maintenanceSystem, out _, out _);

            using var result = maintenanceSystem.GetMaintenanceInfo().Result;
            Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Success);

            using var LatestMaintenanceInfo = result.GetLatestMaintenanceInfo();
            Assert.IsFalse(LatestMaintenanceInfo.IsInsideWindow());
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MAINTENANCESYSTEM_TESTS || RUN_MAINTENANCESYSTEM_GET_LATEST_MAINTENANCEWINDOW_TEST
        [Test]
        public static void GetLatestMaintenanceWindowInfoTest()
        {
            GetFoundationSystems(out _, out _, out _, out _, out _, out _, out _, out var maintenanceSystem, out _, out _);

            using var result = maintenanceSystem.GetMaintenanceInfo().Result;
            Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Success);

            using var latestMaintenanceInfo = result.GetLatestMaintenanceInfo();
	        if (result.HasAnyMaintenanceWindows())
	        {
		        // if any windows were retrieved, then we should expect these fields to all be filled
		        Assert.AreNotEqual(latestMaintenanceInfo.Description, "");
		        Assert.AreNotEqual(latestMaintenanceInfo.StartDateTimestamp, "");
		        Assert.AreNotEqual(latestMaintenanceInfo.EndDateTimestamp, "");
	        }
	        else
	        {
		        // if no windows were retrieved, we should expect to have gotten the default window back when asking for the latest one
		        Assert.IsFalse(latestMaintenanceInfo.IsInsideWindow());
		        Assert.AreEqual(latestMaintenanceInfo.Description, result.GetDefaultMaintenanceInfo().Description);
		        Assert.AreEqual(latestMaintenanceInfo.StartDateTimestamp, result.GetDefaultMaintenanceInfo().StartDateTimestamp);
		        Assert.AreEqual(latestMaintenanceInfo.EndDateTimestamp, result.GetDefaultMaintenanceInfo().EndDateTimestamp);
	        }
        }
#endif
    }
}