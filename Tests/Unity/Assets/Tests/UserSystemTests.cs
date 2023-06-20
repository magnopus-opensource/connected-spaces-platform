using System.Collections;
using System.Collections.Generic;

using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

using Common = Olympus.Foundation.Common;
using Systems = Olympus.Foundation.Systems;
using Services = Olympus.Foundation.Services;


namespace Tests
{
    public class UserSystemTests : TestsBase
    {
        public const string DefaultLoginEmail = "olyservice@rewind.co";
        public const string DefaultLoginPassword = "rcWbfc1MU3Yx";
        public const string SecondTestAccountEmail = "olyservice+test@rewind.co";
        public const string SecondTestAccountPassword = "SsLEW@H=b6J2@7#W";
        public const string SecondTestAccountId = "61f42814b159c6ad996c0c67";

        public const string DefaultDeviceId = "0d011257-d49f-4423-9aa0-06d155e560bb";

        public static IEnumerator LogOut(Systems.UserSystem userSystem)
        {
            var task = userSystem.Logout();

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            Debug.Log("Logged out");
        }

        public static IEnumerator LogIn(Result<string> outUserId, Systems.UserSystem userSystem, string email = DefaultLoginEmail, string password = DefaultLoginPassword, Services.EResultCode expectedResult = Services.EResultCode.Success)
        {
            var task = userSystem.Login("", email, password);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();
            outUserId.Value = result.GetLoginState().UserId;

            Assert.AreEqual(resCode, expectedResult);

            if (resCode == Services.EResultCode.Success)
            {
                PushCleanupFunction(() => LogOut(userSystem));
                Debug.Log($"Logged in (UserId: { outUserId.Value })");
            }
        }

        public static IEnumerator LogInAsGuestWithId(Result<string> outUserId, Systems.UserSystem userSystem, string deviceId = DefaultDeviceId, Services.EResultCode expectedResult = Services.EResultCode.Success, bool pushCleanupFunction = true)
        {
            var task = userSystem.LoginAsGuestWithId(deviceId);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();
            outUserId.Value = result.GetLoginState().UserId;

            Assert.AreEqual(resCode, expectedResult);

            if (resCode == Services.EResultCode.Success)
            {
                if (pushCleanupFunction)
                    PushCleanupFunction(() => LogOut(userSystem));

                Debug.Log($"Logged in (UserId: { outUserId.Value })");
            }
        }

        static Dictionary<string, object> ParseJsonObject(string data)
        {
            var json = new Dictionary<string, object>();
            var index = data.IndexOf('{') + 1;

            // Skips whitespace and returns the next character in the string
            char NextChar()
            {
                var c = data[index++];

                while (char.IsWhiteSpace(c))
                    c = data[index++];

                return c;
            }

            // Parses the next string of characters and returns it as the appropriate type
            object ReadValue()
            {
                var c = NextChar();
                object value;

                switch (c)
                {
                    case '{':   // Object
                        {
                            var _start = index - 1;
                            var _scope = 1;

                            while (_scope > 0)
                            {
                                c = data[index++];

                                if (c == '{')
                                    _scope++;
                                else if (c == '}')
                                    _scope--;
                            }

                            var _end = index;
                            var inner = data.Substring(_start, _end - _start);
                            value = ParseJsonObject(inner);
                        }
                        break;
                    case '[':   // Array
                        var list = new List<object>();

                        while (c != ']')
                        {
                            // TODO: FIX THIS! Needs to consume the comma
                            list.Add(ReadValue());
                            c = NextChar();
                        }

                        value = list;
                        break;
                    case '"':   // String
                        {
                            var _start = index;
                            c = data[index++];

                            while (!(c == '"' && data[index - 1] != '\\'))
                                c = data[index++];

                            var _end = index - 1;
                            value = data.Substring(_start, _end - _start);
                        }
                        break;
                    default:
                        {
                            var _start = index - 1;
                            c = data[index++];

                            while (!char.IsWhiteSpace(c))
                            {
                                if (c == ',' || c == '}')
                                {
                                    index--;

                                    break;
                                }

                                c = data[index++];
                            }

                            var _end = index;
                            var inner = data.Substring(_start, _end - _start);

                            if (inner == "true")
                                value = true;
                            else if (inner == "false")
                                value = false;
                            else if (double.TryParse(inner, out var result))
                                value = result;
                            else
                                value = null;
                        }
                        break;
                }

                return value;
            }

            for (; ; )
            {
                var c = NextChar();

                if (c == '}')
                    break;

                if (c == ',')
                    c = NextChar();

                if (c != '"')
                    throw new System.Exception($"Expected '\"'. Got '{c}'.");

                var start = index;
                c = data[index++];

                while (!(c == '"' && data[index - 1] != '\\'))
                    c = data[index++];

                var end = index - 1;
                var key = data.Substring(start, end - start);

                c = NextChar();

                if (c != ':')
                    throw new System.Exception($"Expected ':'. Got '{c}'.");

                json[key] = ReadValue();
            }

            return json;
        }


        [UnityTest, Order(1)]
        public IEnumerator LoginTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _);

            Debug.Log(Olympus.Foundation.OlympusFoundation.GetVersion());

            // Log in
            var userId = new Result<string>();
            yield return LogIn(userId, userSystem);
        }

        [UnityTest, Order(2)]
        public IEnumerator LoginWithTokenTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);
            string loginToken = "";

            userSystem.OnNewLoginTokenReceived += (s, result) =>
            {
                if (result.GetResultCode() == Services.EResultCode.Success)
                {
                    loginToken = result.GetLoginTokenInfo().RefreshToken;

                    Debug.Log($"New Login token {result.GetLoginTokenInfo().RefreshToken} expires at {result.GetLoginTokenInfo().RefreshExpiryTime}");
                }
            };

            // Log in
            var userId = new Result<string>();
            yield return LogIn(userId, userSystem);

            {
                var task = userSystem.LoginWithToken(userId, loginToken);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();
                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            var currentLoginState = userSystem.GetLoginState();
            Assert.AreEqual(currentLoginState.State, Systems.ELoginState.LoggedIn);

            //check that we're successfully logged in to CHS by creating a space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);
        }

        [UnityTest, Order(3)]
        public IEnumerator ExchangeKeyTest()
        {
            string key;
            string userId;

            // Use WebClient as HttpClient requires an extra package to be installed
            using (var http = new System.Net.WebClient())
            {
                http.Headers[System.Net.HttpRequestHeader.ContentType] = "text/json";
                var response = http.UploadString(chsEndpointBaseUri + "/mag-user/api/v1/users/login", "{ \"email\": \"olyservice@rewind.co\", \"password\": \"rcWbfc1MU3Yx\", \"deviceId\": \"GoodbyeAndThanksForAllTheFish\" }");
                var json = ParseJsonObject(response);
                userId = json["userId"] as string;
                var authToken = json["accessToken"] as string;

                http.Headers[System.Net.HttpRequestHeader.ContentType] = "text/json";
                http.Headers[System.Net.HttpRequestHeader.Authorization] = "Bearer " + authToken;
                response = http.UploadString(chsEndpointBaseUri + "/mag-user/api/v1/users/onetimekey", $"{{ \"userId\": \"{userId}\", \"deviceId\": \"GoodbyeAndThanksForAllTheFish\" }}");
                json = ParseJsonObject(response);
                key = json["oneTimeKey"] as string;
            }

            GetFoundationSystems(out var userSystem, out _, out _, out _);

            // Log in with OTP
            {
                var task = userSystem.ExchangeKey(userId, key);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                var gotUserID = result.GetLoginState().UserId;

                Assert.AreEqual(userId, gotUserID);
            }
        }

        [UnityTest, Order(4)]
        public IEnumerator GetProfileTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _);

            // Log in
            var userId = new Result<string>();
            yield return LogIn(userId, userSystem);

            // Get profile
            {
                var task = userSystem.GetProfileByUserId(userId);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.AreEqual(result.GetProfile().Email, "olyservice@rewind.co");
            }
        }

        [UnityTest, Order(5)]
        public IEnumerator GetLiteProfilesTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _);

            // Log in
            var userId = new Result<string>();
            yield return LogIn(userId, userSystem);

            // Get profiles
            {
                var userIds = new Common.Array<string>(1);
                userIds[0] = userId;
                var result = userSystem.GetProfilesByUserId(userIds).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.AreEqual(result.GetProfiles().Size(), 1);
                Assert.AreEqual(result.GetProfiles()[0].UserId, userId.Value);
            }
        }

        [UnityTest, Order(6)]
        public IEnumerator LoginErrorTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _);

            // Log in with invalid credentials
            var userId = new Result<string>();
            yield return LogIn(userId, userSystem, email: "invalidlogin@rewind.co", password: "", expectedResult: Services.EResultCode.Failed);
        }
    }
}
