using Systems = Csp.Systems;
using Services = Csp.Services;

using CSPEngine;

using static CSharpTests.TestHelper;


namespace CSharpTests
{
    static class ExtensionMethods
    {
        #region Systems.UserSystem

        public static void TestLogOut(this Systems.UserSystem userSystem)
        {
            using var result = userSystem.Logout().Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            LogDebug("Logged out");
        }

        public static string TestLogIn(this Systems.UserSystem userSystem, string email = null, string password = null, Services.EResultCode expectedResult = Services.EResultCode.Success, bool pushCleanupFunction = true)
        {
            email ??= UserSystemTests.DefaultLoginEmail;
            password ??= UserSystemTests.DefaultLoginPassword;

            using var result = userSystem.Login("", email, password, null).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, expectedResult);

            using var loginState = result.GetLoginState();
            var userId = loginState.UserId;

            if (resCode == Services.EResultCode.Success)
            {
                if (pushCleanupFunction)
                    PushCleanupFunction(() => userSystem.TestLogOut());

                LogDebug($"Logged in (UserId: {userId})");
            }

            return userId;
        }

        public static string TestGuestLogIn(this Systems.UserSystem userSystem, bool pushCleanupFunction = true)
        {
            using var result = userSystem.LoginAsGuest(null).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var loginState = result.GetLoginState();
            var userId = result.GetLoginState().UserId;

            if (resCode == Services.EResultCode.Success)
            {
                if (pushCleanupFunction)
                    PushCleanupFunction(() => userSystem.TestLogOut());

                LogDebug($"Logged in as guest (UserId: {userId})");
            }

            return userId;
        }

        #endregion
    }
}
