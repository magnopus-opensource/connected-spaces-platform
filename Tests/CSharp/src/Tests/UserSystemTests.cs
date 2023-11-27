using System;
using System.IO;
using System.Linq;
using System.Threading;

using Common = Csp.Common;
using Systems = Csp.Systems;
using Services = Csp.Services;

using CSharpTests;
using static CSharpTests.TestHelper;


namespace CSPEngine
{
    public static class ThirdPartyAuthenticationDefines
    {
        public const string StateIdURLParam = "state=";
        public const string ClientIdURLParam = "client_id=";
        public const string ScopeURLParam = "scope=";
        public const string RedirectURLParam = "redirect_uri=";
        public const string InvalidURLParamValue = "N/A";
    }

    static class UserSystemTests
    {
        public static readonly string
            DefaultLoginEmail,
            DefaultLoginPassword,
            AlternativeLoginEmail,
            AlternativeLoginPassword;

        private const string GeneratedTestAccountEmailFormat = "testnopus.pokemon+{0}@magnopus.com";
        private const string GeneratedTestAccountPassword = "3R{d2}3C<x[J7=jU";

        static UserSystemTests()
        {
            if (!File.Exists("test_account_creds.txt"))
                LogFatal("test_account_creds.txt not found! This file must exist and must contain the following information:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>");

            var creds = File.ReadAllText("test_account_creds.txt").Split(' ', '\n').Select(s => s.Trim()).ToArray();

            if (creds.Length != 4)
                LogFatal("test_account_creds.txt must be in the following format:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>");

            DefaultLoginEmail = creds[0];
            DefaultLoginPassword = creds[1];
            AlternativeLoginEmail = creds[2];
            AlternativeLoginPassword = creds[3];
        }

        public static void LogOut(Systems.UserSystem userSystem)
        {
            using var result = userSystem.Logout().Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            LogDebug("Logged out");
        }

        /// <returns>User Id</returns>
        public static string LogIn(Systems.UserSystem userSystem, string email = null, string password = null, Services.EResultCode expectedResult = Services.EResultCode.Success, bool pushCleanupFunction = true)
        {
            email ??= DefaultLoginEmail;
            password ??= DefaultLoginPassword;

            using var result = userSystem.Login("", email, password, null).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, expectedResult);

            using var loginState = result.GetLoginState();
            var userId = loginState.UserId;

            if (resCode == Services.EResultCode.Success)
            {
                if (pushCleanupFunction)
                    PushCleanupFunction(() => LogOut(userSystem));

                LogDebug($"Logged in (UserId: {userId})");
            }

            return userId;
        }

        /// <returns>User Id</returns>
        public static string LogInAsGuest(Systems.UserSystem userSystem, bool pushCleanupFunction = true)
        {
            using var result = userSystem.LoginAsGuest(null).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var loginState = result.GetLoginState();
            var userId = result.GetLoginState().UserId;

            if (resCode == Services.EResultCode.Success)
            {
                if (pushCleanupFunction)
                    PushCleanupFunction(() => LogOut(userSystem));

                LogDebug($"Logged in as guest (UserId: {userId})");
            }

            return userId;
        }

        public static Systems.Profile GetFullProfileByUserId(Systems.UserSystem userSystem, string userId)
        {
            using var result = userSystem.GetProfileByUserId(userId).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            return result.GetProfile();
        }

        private static void ValidateThirdPartyAuthoriseURL(string authoriseURL, string redirectURL)
        {
            Assert.IsFalse(string.IsNullOrEmpty(authoriseURL));
            Assert.AreNotEqual(authoriseURL, "error");

            var tokens = authoriseURL.Split('&', '?');

            var stateId = ThirdPartyAuthenticationDefines.InvalidURLParamValue;
            var clientId = ThirdPartyAuthenticationDefines.InvalidURLParamValue;
            var scope = ThirdPartyAuthenticationDefines.InvalidURLParamValue;
            var retrievedRedirectURL = ThirdPartyAuthenticationDefines.InvalidURLParamValue;

            foreach (var URLElement in tokens)
            {
                if (URLElement.StartsWith(ThirdPartyAuthenticationDefines.StateIdURLParam))
                {
                    stateId = URLElement.Substring(ThirdPartyAuthenticationDefines.StateIdURLParam.Length);
                    continue;
                }
                else if (URLElement.StartsWith(ThirdPartyAuthenticationDefines.ClientIdURLParam))
                {
                    clientId = URLElement.Substring(ThirdPartyAuthenticationDefines.ClientIdURLParam.Length);
                    continue;
                }
                else if (URLElement.StartsWith(ThirdPartyAuthenticationDefines.ScopeURLParam))
                {
                    scope = URLElement.Substring(ThirdPartyAuthenticationDefines.ScopeURLParam.Length);
                    continue;
                }
                else if (URLElement.StartsWith(ThirdPartyAuthenticationDefines.RedirectURLParam))
                {
                    retrievedRedirectURL = URLElement.Substring(ThirdPartyAuthenticationDefines.RedirectURLParam.Length);
                    continue;
                }
            }

            // validate that the following contain something that potentially makes sense
            Assert.AreNotEqual(stateId, ThirdPartyAuthenticationDefines.InvalidURLParamValue);
            Assert.AreNotEqual(clientId, ThirdPartyAuthenticationDefines.InvalidURLParamValue);
            Assert.AreNotEqual(scope, ThirdPartyAuthenticationDefines.InvalidURLParamValue);
            Assert.AreNotEqual(retrievedRedirectURL, ThirdPartyAuthenticationDefines.InvalidURLParamValue);

            Assert.IsGreaterThan(stateId.Length, 10UL);
            Assert.IsGreaterThan(clientId.Length, 10UL);
            Assert.IsGreaterOrEqualThan(scope.Length, 5UL);
            Assert.AreEqual(retrievedRedirectURL, redirectURL);
        }

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGIN_TEST
        [Test]
        public static void LoginTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = LogIn(userSystem);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_FORGOTPASSWORD_TEST
        [Test]
        public static void ForgotPasswordTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            // Tests passing false for UseTokenChangePasswordUrl
            {
                using var result = userSystem.ForgotPassword("testnopus.pokemon@magnopus.com", null, null, false).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            {
                using var result = userSystem.ForgotPassword("testnopus.pokemon+1@magnopus.com", null, null, false).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            {
                using var result = userSystem.ForgotPassword("email", null, null, false).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Failed);
            }

            // Tests passing true for UseTokenChangePasswordUrl
            {
                using var result = userSystem.ForgotPassword("testnopus.pokemon@magnopus.com", null, null, true).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            {
                using var result = userSystem.ForgotPassword("testnopus.pokemon+1@magnopus.com", null, null, true).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            {
                using var result = userSystem.ForgotPassword("email", null, null, true).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Failed);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGIN_WITH_TOKEN_TEST
        [Test]
        public static void LoginWithTokenTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            string loginToken = "";
            var loginTokenAvailable = false;

            userSystem.OnNewLoginTokenReceived += (s, result) =>
            {
                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                if (result.GetResultCode() == Services.EResultCode.Success)
                {
                    using var tokenInfo = result.GetLoginTokenInfo();
                    loginToken = tokenInfo.RefreshToken;
                    loginTokenAvailable = true;

                    LogDebug($"New Login token {result.GetLoginTokenInfo().RefreshToken} expires at {result.GetLoginTokenInfo().RefreshExpiryTime}");
                }

                result.Dispose();
            };

            // Log in
            var userId = LogIn(userSystem);

            var waitForTestTimeoutCountMs = 0;
            const int waitForTestTimeoutLimitMs = 1000;

            // wait for the login token to become available
            while (loginTokenAvailable == false && waitForTestTimeoutCountMs < waitForTestTimeoutLimitMs)
            {
                Thread.Sleep(50);
                waitForTestTimeoutCountMs += 50;
            }

            using var result = userSystem.LoginWithToken(userId, loginToken).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            loginTokenAvailable = false;
            waitForTestTimeoutCountMs = 0;

            // Wait for the login token to become available
            while (!loginTokenAvailable && waitForTestTimeoutCountMs < waitForTestTimeoutLimitMs)
            {
                Thread.Sleep(50);
                waitForTestTimeoutCountMs += 50;
            }

            using var currentLoginState = userSystem.GetLoginState();

            Assert.AreEqual(currentLoginState.State, Systems.ELoginState.LoggedIn);

            //check that we're successfully logged in to CHS by creating a space
            string testSpaceName = GenerateUniqueString("CSP-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "CSP-UNITTEST-SPACEDESC-REWIND";

            // Create space
            _ = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_EXCHANGEKEY_TEST
        [Test]
        public static void ExchangeKeyTest()
        {
            string key;
            string userId;

            // Use WebClient as HttpClient requires an extra package to be installed
            using var http = new System.Net.WebClient();
            http.Headers[System.Net.HttpRequestHeader.ContentType] = "text/json";
            var response = http.UploadString(CHSEndpointBaseUri + "/mag-user/api/v1/users/login", "{ \"email\": \"olyservice@rewind.co\", \"password\": \"rcWbfc1MU3Yx\", \"deviceId\": \"GoodbyeAndThanksForAllTheFish\" }");
            var json = ParseJsonObject(response);
            userId = json["userId"] as string;
            var authToken = json["accessToken"] as string;

            http.Headers[System.Net.HttpRequestHeader.ContentType] = "text/json";
            http.Headers[System.Net.HttpRequestHeader.Authorization] = "Bearer " + authToken;
            response = http.UploadString(CHSEndpointBaseUri + "/mag-user/api/v1/users/onetimekey", $"{{ \"userId\": \"{userId}\", \"deviceId\": \"GoodbyeAndThanksForAllTheFish\" }}");
            json = ParseJsonObject(response);
            key = json["oneTimeKey"] as string;

            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in with OTP
            {
                using var result = userSystem.ExchangeKey(userId, key).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var loginState = result.GetLoginState();
                var gotUserID = loginState.UserId;

                Assert.AreEqual(userId, gotUserID);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGINERROR_TEST
        [Test]
        public static void LoginErrorTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in with invalid credentials
            _ = LogIn(userSystem, email: "invalidlogin@rewind.co", password: "", expectedResult: Services.EResultCode.Failed);

            // Log in
            _ = LogIn(userSystem);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_TEST
        [Test]
        public static void UpdateDisplayNameTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            var userId = LogIn(userSystem);

            var newDisplayName = GenerateUniqueString("NAME");

            // Update display name
            {
                using var result = userSystem.UpdateUserDisplayName(userId, newDisplayName).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Retrieve user profile and verify display name has been updated
            {
                using var profile = GetFullProfileByUserId(userSystem, userId);

                Assert.AreEqual(profile.UserId, userId);
                Assert.AreEqual(profile.DisplayName, newDisplayName);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_WITH_SPACES_TEST
        [Test]
        public static void UpdateDisplayNameWithSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            var userId = LogIn(userSystem);

            var newDisplayName = GenerateUniqueString("TEST ");

            // Update display name
            {
                using var result = userSystem.UpdateUserDisplayName(userId, newDisplayName).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Retrieve user profile and verify display name has been updated
            {
                using var profile = GetFullProfileByUserId(userSystem, userId);

                Assert.AreEqual(profile.UserId, userId);
                Assert.AreEqual(profile.DisplayName, newDisplayName);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_WITH_SYMBOLS_TEST
        [Test]
        public static void UpdateDisplayNameWithSymbolsTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            var userId = LogIn(userSystem);

            var newDisplayName = GenerateUniqueString("()= - ");

            // Update display name
            {
                using var result = userSystem.UpdateUserDisplayName(userId, newDisplayName).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Retrieve user profile and verify display name has been updated
            {
                using var profile = GetFullProfileByUserId(userSystem, userId);

                Assert.AreEqual(profile.UserId, userId);
                Assert.AreEqual(profile.DisplayName, newDisplayName);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CREATE_USER_TEST
        [Test]
        public static void CreateUserTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out var settingsSystem, out _, out _, out _);

            string uniqueUserName = GenerateUniqueString(GenerateUniqueString("CSP-TEST-NAME"));
            string testDisplayName = "CSP-TEST-DISPLAY";
            string uniqueTestEmail = String.Format(GeneratedTestAccountEmailFormat, GetUniqueHexString());

            string createdUserId;

            // Create new user
            {
                using var result = userSystem.CreateUser(uniqueUserName, testDisplayName, uniqueTestEmail, GeneratedTestAccountPassword, true, true, null, null).Result;
                var resCode = result.GetResultCode();
                var failureReason = result.GetFailureReason();
                var body = result.GetResponseBody();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var createdProfile = result.GetProfile();
                createdUserId = createdProfile.UserId;

                Assert.AreEqual(createdProfile.UserName, uniqueUserName);
                Assert.AreEqual(createdProfile.DisplayName, testDisplayName);
                Assert.AreEqual(createdProfile.Email, uniqueTestEmail);
            }

            _ = LogIn(userSystem);

            // Verify that newsletter preference was set
            {
                using var result = settingsSystem.GetNewsletterStatus(createdUserId).Result;
                var resCode = result.GetResultCode();
                var body = result.GetResponseBody();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.IsTrue(result.GetValue());
            }

            // Retrieve the lite profile
            {
                using var ids = new Common.Array<string>(1);
                ids[0] = createdUserId;

                using var result = userSystem.GetProfilesByUserId(ids).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.AreEqual(result.GetProfiles().Size(), 1UL);

                using var profiles = result.GetProfiles();
                using var profile = profiles[0];

                Assert.AreEqual(profile.UserId, createdUserId);
                Assert.AreEqual(profile.DisplayName, testDisplayName);
            }

            // Retrieve the full profile
            {
                using var fullProfile = GetFullProfileByUserId(userSystem, createdUserId);

                Assert.AreEqual(fullProfile.UserId, createdUserId);
                Assert.AreEqual(fullProfile.UserName, uniqueUserName);
                Assert.AreEqual(fullProfile.DisplayName, testDisplayName);
                Assert.AreEqual(fullProfile.Email, uniqueTestEmail);
            }

            // Delete the created user
            {
                using var result = userSystem.DeleteUser(createdUserId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CREATE_USER_EMPTY_USERNAME_DISPLAYNAME_TEST
        [Test]
        public static void CreateUserEmptyUsernameDisplaynameTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            string uniqueTestEmail = string.Format(GeneratedTestAccountEmailFormat, GetUniqueHexString());

            string createdUserId;

            // Create new user
            {
                using var result = userSystem.CreateUser(null, null, uniqueTestEmail, GeneratedTestAccountPassword, false, true, null, null).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var createdProfile = result.GetProfile();
                createdUserId = createdProfile.UserId;

                Assert.IsTrue(string.IsNullOrEmpty(createdProfile.UserName));
                Assert.IsFalse(string.IsNullOrEmpty(createdProfile.DisplayName));
                Assert.AreEqual(createdProfile.Email, uniqueTestEmail);
            }

            _ = LogIn(userSystem);

            // Retrieve the lite profile
            {
                using var ids = new Common.Array<string>(1);
                ids[0] = createdUserId;

                using var result = userSystem.GetProfilesByUserId(ids).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.AreEqual(result.GetProfiles().Size(), 1UL);

                using var profiles = result.GetProfiles();
                using var profile = profiles[0];

                Assert.AreEqual(profile.UserId, createdUserId);
                Assert.IsFalse(string.IsNullOrEmpty(profile.DisplayName));
            }

            // Retrieve the full profile
            {
                using var fullProfile = GetFullProfileByUserId(userSystem, createdUserId);

                Assert.AreEqual(fullProfile.UserId, createdUserId);
                Assert.IsTrue(string.IsNullOrEmpty(fullProfile.UserName));
                Assert.IsFalse(string.IsNullOrEmpty(fullProfile.DisplayName));
                Assert.AreEqual(fullProfile.Email, uniqueTestEmail);
            }

            // Delete the created user
            {
                using var result = userSystem.DeleteUser(createdUserId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_PING_TEST
        [Test]
        public static void PingTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            using var result = userSystem.Ping().Result;

            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
        }

#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_SUPPORTED_PROVIDERS_TEST
        [Test]
        public static void GetThirdPartySupportedProvidersTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            // Check the FDN supported providers
            using var supportedProviders = userSystem.GetSupportedThirdPartyAuthenticationProviders();
            Assert.AreEqual(supportedProviders.Size(), 3UL);

            bool foundGoogle = false;
            bool foundDiscord = false;
            bool foundApple = false;

            for (uint idx = 0; idx < supportedProviders.Size(); ++idx)
            {
                if (supportedProviders[idx] == Systems.EThirdPartyAuthenticationProviders.Google)
                {
                    foundGoogle = true;
                }
                else if (supportedProviders[idx] == Systems.EThirdPartyAuthenticationProviders.Discord)
                {
                    foundDiscord = true;
                }
                else if (supportedProviders[idx] == Systems.EThirdPartyAuthenticationProviders.Apple)
                {
                    foundApple = true;
                }
                else
                {
                    Assert.IsTrue(false);
                    LogDebug($"Please update this test with this new FDN auth provider: {supportedProviders[idx]}");
                }
            }

            Assert.IsTrue(foundGoogle && foundDiscord && foundApple);
        }
#endif
#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_GOOGLE_TEST
        [Test]
        public static void GetAuthoriseURLForGoogleTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            var redirectURL = "https://odev.magnoverse.space/oauth";

            // Retrieve Authorise URL for Google
            using var result = userSystem.GetThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Google, redirectURL).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            var authoriseURL = result.GetValue();
            ValidateThirdPartyAuthoriseURL(authoriseURL, redirectURL);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_DISCORD_TEST
        [Test]
        public static void GetAuthoriseURLForDiscordTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            var redirectURL = "https://odev.magnoverse.space/oauth";

            // Retrieve Authorise URL for Google
            using var result = userSystem.GetThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Discord, redirectURL).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            var authoriseURL = result.GetValue();
            ValidateThirdPartyAuthoriseURL(authoriseURL, redirectURL);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_APPLE_TEST
        [Test]
        public static void GetAuthoriseURLForAppleTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            var redirectURL = "https://odev.magnoverse.space/oauth";

            // Retrieve Authorise URL for Google
            using var result = userSystem.GetThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Apple, redirectURL).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            var authoriseURL = result.GetValue();
            ValidateThirdPartyAuthoriseURL(authoriseURL, redirectURL);
        }
#endif

        // As the following two tests require manual actions explained inside, they are currently disabled
        // ATM only the WASM tests would be able to have a end-to-end testing flow using Selenium for the URL redirects
#if RUN_USERSYSTEM_GOOGLE_LOGIN_TEST
        [Test] 
        public static void GoogleLogInTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            var redirectURL = "https://odev.magnoverse.space/oauth";
            
            // Retrieve Authorise URL for Google
            using var result = userSystem.GetThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Google, redirectURL).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            var authoriseURL = result.GetValue();
            var tokens = authoriseURL.Split('&');

            var stateId = ThirdPartyAuthenticationDefines.InvalidURLParamValue;

            foreach(var URLElement in tokens)
            {
                if(URLElement.StartsWith(ThirdPartyAuthenticationDefines.StateIdURLParam))
                {
                    stateId = URLElement.Substring(ThirdPartyAuthenticationDefines.StateIdURLParam.Length);
                    break;
                }
            }

            LogDebug($"AuthoriseURL: {authoriseURL}");

            // 1. Set a breakpoint on the next line before reading from the file
            // 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
            // 3. Get the "code" param value from the response URL and drop it in the file below (this file should be next to the Test binary)
            if (!File.Exists("third_party_auth_token.txt"))
                LogFatal("third_party_auth_token.txt not found! This file must exist and must contain the provider authentication code/token");

            var googleToken = File.ReadAllText("third_party_auth_token.txt");
            using var resultLogin = userSystem.LoginToThirdPartyAuthenticationProvider(googleToken, stateId).Result;
            Assert.AreEqual(resultLogin.GetResultCode(), Services.EResultCode.Success);

            using var loginState = resultLogin.GetLoginState();
            var userId = loginState.UserId;
            
            // test that we are in fact logged in
            using var FullProfile = GetFullProfileByUserId(userSystem, userId);
        }
#endif

#if RUN_USERSYSTEM_DISCORD_LOGIN_TEST
        [Test] 
        public static void DiscordLogInTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            var redirectURL = "https://odev.magnoverse.space/oauth";
            
            // Retrieve Authorise URL for Google
            using var result = userSystem.GetThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Discord, redirectURL).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            var authoriseURL = result.GetValue();
            var tokens = authoriseURL.Split('&');

            var stateId = ThirdPartyAuthenticationDefines.InvalidURLParamValue;

            foreach(var URLElement in tokens)
            {
                if(URLElement.StartsWith(ThirdPartyAuthenticationDefines.StateIdURLParam))
                {
                    stateId = URLElement.Substring(ThirdPartyAuthenticationDefines.StateIdURLParam.Length);
                    break;
                }
            }

            LogDebug($"AuthoriseURL: {authoriseURL}");

            // 1. Set a breakpoint on the next line before reading from the file
            // 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
            // 3. Get the "code" param value from the response URL and drop it in the file below (this file should be next to the Test binary)
            if (!File.Exists("third_party_auth_token.txt"))
                LogFatal("third_party_auth_token.txt not found! This file must exist and must contain the provider authentication code/token");

            var discordToken = File.ReadAllText("third_party_auth_token.txt");
            using var resultLogin = userSystem.LoginToThirdPartyAuthenticationProvider(discordToken, stateId).Result;
            Assert.AreEqual(resultLogin.GetResultCode(), Services.EResultCode.Success);

            using var loginState = resultLogin.GetLoginState();
            var userId = loginState.UserId;
            
            // test that we are in fact logged in
            using var FullProfile = GetFullProfileByUserId(userSystem, userId);
        }
#endif

#if RUN_USERSYSTEM_APPLE_LOGIN_TEST
        [Test] 
        public static void AppleLogInTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            var redirectURL = "https://example-app.com/redirect";
            
            // Retrieve Authorise URL for Apple
            using var result = userSystem.GetThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Apple, redirectURL).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            var authoriseURL = result.GetValue();
            var tokens = authoriseURL.Split('&');

            var stateId = ThirdPartyAuthenticationDefines.InvalidURLParamValue;

            foreach(var URLElement in tokens)
            {
                if(URLElement.StartsWith(ThirdPartyAuthenticationDefines.StateIdURLParam))
                {
                    stateId = URLElement.Substring(ThirdPartyAuthenticationDefines.StateIdURLParam.Length);
                    break;
                }
            }

            LogDebug($"AuthoriseURL: {authoriseURL}");

            // 1. Set a breakpoint on the next line before reading from the file
            // 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
            // 3. Get the "code" param value from the response URL and drop it in the file below (this file should be next to the Test binary)
            if (!File.Exists("third_party_auth_token.txt"))
                LogFatal("third_party_auth_token.txt not found! This file must exist and must contain the provider authentication code/token");

            var appleToken = File.ReadAllText("third_party_auth_token.txt");
            using var resultLogin = userSystem.LoginToThirdPartyAuthenticationProvider(appleToken, stateId).Result;
            Assert.AreEqual(resultLogin.GetResultCode(), Services.EResultCode.Success);

            using var loginState = resultLogin.GetLoginState();
            var userId = loginState.UserId;
            
            // test that we are in fact logged in
            using var FullProfile = GetFullProfileByUserId(userSystem, userId);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AGORA_USER_TOKEN_TEST
        [Test]
        public static void GetAgoraUserTokenTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = LogIn(userSystem);

            /*
	          Setup token params 
	          For this test, it's exeptable to use 0 for the UserId and ChannelId
	          This is because the endpoint is just using an algorithm to generate the token
	          So no valid Ids are needed for verification
	        */
            var tokenParams = new Systems.AgoraUserTokenParams();
            tokenParams.AgoraUserId = "0";
            tokenParams.ChannelName = "0";
            tokenParams.Lifespan = 10000;
            tokenParams.ShareAudio = true;
            tokenParams.ShareScreen = false;
            tokenParams.ShareVideo = false;
            tokenParams.ReadOnly = false;

            // Get token
            var result = userSystem.GetAgoraUserToken(tokenParams).Result;

            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            Assert.AreNotEqual(result.GetUserToken(), "");
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GETPROFILEASGUEST_TEST
        [Test]
        public static void GetGuestProfileTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out _);

            string id = LogInAsGuest(userSystem);

            _ = GetFullProfileByUserId(userSystem, id);
        }
#endif
    }
}



