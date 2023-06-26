using System.IO;
using System.Runtime.InteropServices;
using Common = Csp.Common;
using Services = Csp.Services;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


namespace CSPEngine
{
    static class SettingsSystemTests
    {
#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_BLOCKSPACE_TEST
        [Test]
        public static void BlockSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out var settingsSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Clear blocked spaces
            {
                using var result = settingsSystem.ClearBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Block space
            {
                using var result = settingsSystem.AddBlockedSpace(userId, space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Get blocked spaces
            {
                using var result = settingsSystem.GetBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var blockedSpaces = result.GetValue();

                Assert.AreEqual(blockedSpaces.Size(), 1UL);
                Assert.AreEqual(blockedSpaces[0], space.Id);
            }

            // Clear blocked spaces
            {
                using var result = settingsSystem.ClearBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_UNBLOCKSPACE_TEST
        [Test]
        public static void UnblockSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out var settingsSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Clear blocked spaces
            {
                using var result = settingsSystem.ClearBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Block space
            {
                using var result = settingsSystem.AddBlockedSpace(userId, space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Get blocked spaces
            {
                using var result = settingsSystem.GetBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var blockedSpaces = result.GetValue();

                Assert.AreEqual(blockedSpaces.Size(), 1UL);
                Assert.AreEqual(blockedSpaces[0], space.Id);
            }

            // Remove Blocked Spaces
            {
                using var result = settingsSystem.RemoveBlockedSpace(userId, space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            // Get blocked spaces
            {
                using var result = settingsSystem.GetBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var blockedSpaces = result.GetValue();

                Assert.AreEqual(blockedSpaces.Size(), 0UL);
            }

            // Clear blocked spaces
            {
                using var result = settingsSystem.ClearBlockedSpaces(userId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }
        }
#endif
#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_GETAVATAR_PORTRAIT_TEST
        [Test]
        public static void GetAvatarPortraitTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out var settingsSystem, out _);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            var userId = userSystem.TestLogIn(pushCleanupFunction: false);

            var localFileName = "OKO.png";
            var AvatarPortrait = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/" + localFileName)
            };
            {
                using var result = settingsSystem.UpdateAvatarPortrait(userId, AvatarPortrait).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }
            {
                using var result = settingsSystem.GetAvatarPortrait(userId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsTrue(result.GetUri().Contains(localFileName));
            }

            UserSystemTests.LogOut(userSystem);
        }
#endif
#if RUN_ALL_UNIT_TESTS || RUN_SETTINGSSYSTEM_TESTS || RUN_SETTINGSSYSTEM_UPDATEAVATAR_PORTRAIT_WITH_BUFFER_TEST
        [Test]
        public static void UpdateAvatarPortraitWithBufferTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out var settingsSystem, out _);
            var userId = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword);

            {
                var uploadFilePath = Path.GetFullPath("assets/OKO.png");
                var uploadFileSize = new System.IO.FileInfo(uploadFilePath).Length;

                var uploadFileData = File.ReadAllBytes(uploadFilePath);

                System.IntPtr uploadFileDataPtr = Marshal.AllocHGlobal((int)uploadFileSize);
                Marshal.Copy(uploadFileData, 0, uploadFileDataPtr, (int)uploadFileSize);

                var source = new Systems.BufferAssetDataSource
                {
                    Buffer = uploadFileDataPtr,
                    BufferLength = (ulong)uploadFileSize
                };
                source.SetMimeType("image/png");

                using var result = settingsSystem.UpdateAvatarPortraitWithBuffer(userId, source).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var resultGetThumbnail = settingsSystem.GetAvatarPortrait(userId).Result;

                Assert.AreEqual(resultGetThumbnail.GetResultCode(), Services.EResultCode.Success);

                var downloadedAsset = new Systems.Asset
                {
                    Uri = resultGetThumbnail.GetUri()
                };

                using var downloadResult = assetSystem.DownloadAssetData(downloadedAsset).Result;

                Assert.AreEqual((ulong)uploadFileSize, downloadResult.GetDataLength());
            }
        }
#endif

    }
}
