using System.IO;
using Common = Csp.Common;
using Services = Csp.Services;
using Systems = Csp.Systems;
using Web = Csp.Web;
using System.Runtime.InteropServices;

using CSharpTests;
using static CSharpTests.TestHelper;


#nullable enable annotations


namespace CSPEngine
{
    static class SpaceSystemTests
    {
        public static void DeleteSpace(Systems.SpaceSystem spaceSystem, Systems.Space space, bool disposeFoundationResources = true)
        {
            var spaceId = space.Id;

            using var result = spaceSystem.DeleteSpace(space.Id).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            LogDebug($"Space deleted (Id: {spaceId})");

            if (disposeFoundationResources)
                space.Dispose();
        }

        /// <remarks>Automatically deletes and disposes the created space after each test unless otherwise specified.</remarks>
        public static Systems.Space CreateSpace(Systems.SpaceSystem spaceSystem, string name, string description,
            Systems.SpaceAttributes spaceAttributes, Common.Map<string, string>? spaceMetadata, 
            Common.Array<Systems.InviteUserRoleInfo>? inviteUsers, Systems.FileAssetDataSource? thumbnail,
            bool pushCleanupFunction = true, bool disposeFoundationResources = true)
        {
            var testMetadata = spaceMetadata;

            if (testMetadata == null)
            {
                testMetadata = new Common.Map<string, string>
                {
                    ["site"] = "Void"
                };
            }

            using var result = spaceSystem.CreateSpace(name, description, spaceAttributes, inviteUsers, testMetadata, thumbnail).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            var space = result.GetSpace();
            LogDebug($"Space created (Id: {space.Id}, Name: {space.Name})");

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteSpace(spaceSystem, space, disposeFoundationResources));

            return space;
        }

        /// <remarks>Automatically deletes and disposes the created space after each test unless otherwise specified.</remarks>
        public static Systems.Space CreateSpaceWithBuffer(Systems.SpaceSystem spaceSystem, string name, string description,
            Systems.SpaceAttributes spaceAttributes, Common.Map<string, string>? spaceMetadata,
            Common.Array<Systems.InviteUserRoleInfo>? inviteUsers, Systems.BufferAssetDataSource thumbnail,
            bool pushCleanupFunction = true, bool disposeFoundationResources = true)
        {
            var testMetadata = spaceMetadata;

            if (testMetadata == null)
            {
                testMetadata = new Common.Map<string, string>();
                testMetadata["site"] = "Void";
            }

            using var result = spaceSystem.CreateSpaceWithBuffer(name, description, spaceAttributes, inviteUsers, testMetadata, thumbnail).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            var space = result.GetSpace();
            LogDebug($"Space created (Id: {space.Id}, Name: {space.Name})");

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteSpace(spaceSystem, space, disposeFoundationResources));

            return space;
        }

        static Systems.Space GetSpace(Systems.SpaceSystem spaceSystem, string spaceId)
        {
            using var result = spaceSystem.GetSpace(spaceId).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            var space = result.GetSpace();

            return space;
        }

        static Systems.Space[] GetSpacesByIds(Systems.SpaceSystem spaceSystem, string[] spaceIds)
        {
            using var _spaceIds = new Common.Array<string>((ulong)spaceIds.Length);

            for (var i = 0UL; i < _spaceIds.Size(); i++)
                _spaceIds[i] = spaceIds[i];

            using var result = spaceSystem.GetSpacesByIds(_spaceIds).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var _spaces = result.GetSpaces();
            var spaces = new Systems.Space[_spaces.Size()];

            for (var i = 0UL; i < _spaces.Size(); i++)
                spaces[i] = new Systems.Space(_spaces[i]);

            return spaces;
        }

        static void GetSpacesByAttributes(Systems.SpaceSystem spaceSystem, bool? isDiscoverable, bool? requiresInvitation, int? resultsSkipNo, int? resultsMaxNo, out Common.Array<Systems.BasicSpace> spaces)
        {
            using var result = spaceSystem.GetSpacesByAttributes(isDiscoverable, requiresInvitation, resultsSkipNo, resultsMaxNo).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            spaces = result.GetSpaces();
            var spacesTotalCount = result.GetTotalCount();

            if (spaces.Size() > 0)
                Assert.IsGreaterThan(spacesTotalCount, 0);
        }

        static void UpdateSpace(Systems.SpaceSystem spaceSystem, Systems.Space space, string? newName, string? newDescription, Systems.SpaceAttributes? newAttributes, out Systems.BasicSpace updatedSpace)
        {
            var result = spaceSystem.UpdateSpace(space.Id, newName, newDescription, newAttributes).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            updatedSpace = result.GetSpace();
        }

        static void AddSiteInfo(Systems.SpaceSystem spaceSystem, string? name, Systems.Space space, out Systems.Site site)
        {
            var siteName = name ?? "OLY-UNITTEST-SITE-NAME";

            var siteLocation = new Systems.GeoLocation { Latitude = 85.0, Longitude = 175.0 };
            var siteRotation = new Systems.OlyRotation { X = 200.0, Y = 200.0, Z = 200.0, W = 200.0 };
            var siteInfo = new Systems.Site { Name = siteName, Location = siteLocation, Rotation = siteRotation };

            using var result = spaceSystem.AddSiteInfo(space.Id, siteInfo).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            site = result.GetSite();
            LogDebug($"Site created (Id: {site.Id}, Name: {site.Name})");

            var siteCopy = site;
            var spaceCopy = space;
            PushCleanupFunction(() => RemoveSiteInfo(spaceSystem, spaceCopy, siteCopy));
        }

        static void RemoveSiteInfo(Systems.SpaceSystem spaceSystem, Systems.Space space, Systems.Site site)
        {
            using var result = spaceSystem.RemoveSiteInfo(space.Id, site).Result;
            
            var resCode = result.GetResultCode();
            var httpResultCode = result.GetHttpResultCode();
            Assert.AreEqual(resCode, Services.EResultCode.Success);
            Assert.AreEqual(httpResultCode, 204);
            LogDebug($"Site deleted (Id: {site.Id}, Name: {site.Name})");
        }

        static void GetSpaceSites(Systems.SpaceSystem spaceSystem, Systems.Space space, out Common.Array<Systems.Site> sites)
        {
            using var result = spaceSystem.GetSitesInfo(space.Id).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            sites = result.GetSites();
        }

        static void UpdateUserRole(Systems.SpaceSystem spaceSystem, Systems.Space space, Systems.UserRoleInfo newUserRole)
        {
            using var result = spaceSystem.UpdateUserRole(space.Id, newUserRole).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);
            LogDebug($"The user role for UserId {newUserRole.UserId} has been updated to {newUserRole.UserRole} successfully");
        }

        static void GetRoleForSpecificUser(Systems.SpaceSystem spaceSystem, Systems.Space space, string userId, out Systems.UserRoleInfo userRoleInfo)
        {
            using var requestedUserIds = new Common.Array<string>(1);
            requestedUserIds[0] = userId;

            var result = spaceSystem.GetUsersRoles(space.Id, requestedUserIds).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var returnedUserRolesInfo = result.GetUsersRoles();

            Assert.AreEqual(returnedUserRolesInfo.Size(), 1UL);

            // TODO: Remove instantiation here as it is a temp fix to work around elements not being returned by copy
            userRoleInfo = new Systems.UserRoleInfo(returnedUserRolesInfo[0]);
        }

        static void GetUsersRoles(Systems.SpaceSystem spaceSystem, Systems.Space space, Common.Array<string> requestedUserIds, out Common.Array<Systems.UserRoleInfo> usersRoles)
        {
            using var result = spaceSystem.GetUsersRoles(space.Id, requestedUserIds).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            usersRoles = result.GetUsersRoles();
        }

        static void UpdateSpaceMetadata(Systems.SpaceSystem spaceSystem, Systems.Space space, Common.Map<string, string>? newMetadata)
        {
            using var result = spaceSystem.UpdateSpaceMetadata(space.Id, newMetadata ?? new Common.Map<string, string>()).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            LogDebug($"Space metadata have been updated successfully");
        }

        static void GetSpaceMetadata(Systems.SpaceSystem spaceSystem, Systems.Space space, out Common.Map<string, string> spaceMetadata)
        {
            using var result = spaceSystem.GetSpaceMetadata(space.Id).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            spaceMetadata = result.GetMetadata();
        }

        static void GetSpacesMetadata(Systems.SpaceSystem spaceSystem, Common.Array<Systems.Space> spaces, out Common.Map<string, Common.Map<string, string>> spaceMetadata)
        {
            var spaceIds = new Common.Array<string>(spaces.Size());

            for (ulong i = 0; i < spaces.Size(); i++)
            {
                spaceIds[i] = spaces[i].Id;
            }

            using var result = spaceSystem.GetSpacesMetadata(spaceIds).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            spaceMetadata = result.GetMetadata();
        }

        static bool IsUriValid(string uri, string fileName)
        {
            if (!uri.StartsWith("https://world-streaming.magnoboard.com/"))
                return false;

            var posLastSlash = uri.LastIndexOf('/');
            var uriFileName = uri.Substring(posLastSlash + 1, fileName.Length);

            return (fileName == uriFileName);
        }

        static Common.Array<Systems.InviteUserRoleInfo> CreateInviteUsers()
        {
            // Create normal users
            var InviteUser1 = new Systems.InviteUserRoleInfo { UserEmail = "testnopus.pokemon+1@magnopus.com", UserRole = Systems.SpaceUserRole.User };
            var InviteUser2 = new Systems.InviteUserRoleInfo { UserEmail = "testnopus.pokemon+2@magnopus.com", UserRole = Systems.SpaceUserRole.User };

            // Create moderator users
            var ModInviteUser1 = new Systems.InviteUserRoleInfo { UserEmail = "testnopus.pokemon+mod1@magnopus.com", UserRole = Systems.SpaceUserRole.Moderator };
            var ModInviteUser2 = new Systems.InviteUserRoleInfo { UserEmail = "testnopus.pokemon+mod2@magnopus.com", UserRole = Systems.SpaceUserRole.Moderator };

            Systems.InviteUserRoleInfo[] InviteUsers = { InviteUser1, InviteUser2, ModInviteUser1, ModInviteUser2 };
            return InviteUsers.ToFoundationArray<Systems.InviteUserRoleInfo>();
        }


#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACE_TEST
        [Test]
        public static void CreateSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            Assert.AreEqual(space.Name, testSpaceName);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACE_WITH_BULK_INVITE_TEST
        [Test]
        public static void CreateSpaceWithBulkInviteTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var InviteUsers = CreateInviteUsers();

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, InviteUsers, null);

            Assert.AreEqual(space.Name, testSpaceName);

            using var getInvitesResult = spaceSystem.GetPendingUserInvites(space.Id).Result;
            var resCode = getInvitesResult.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var pendingInvites = getInvitesResult.GetPendingInvitesEmails();

            Assert.AreEqual(pendingInvites.Size(), 4UL);

            for (var i = 0UL; i < pendingInvites.Size(); i++)
                LogDebug($"Pending space invite for email: { pendingInvites[i] }");
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACEWITHBUFFER_TEST
        [Test]
        public static void CreateSpaceWithBufferTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var uploadFilePath = Path.GetFullPath("assets/OKO.png");
            var uploadFileSize = new FileInfo(uploadFilePath).Length;

            var uploadFileData = File.ReadAllBytes(uploadFilePath);

            System.IntPtr uploadFileDataPtr = Marshal.AllocHGlobal((int)uploadFileSize);
            Marshal.Copy(uploadFileData, 0, uploadFileDataPtr, (int)uploadFileSize);

            var source = new Systems.BufferAssetDataSource
            {
                Buffer = uploadFileDataPtr,
                BufferLength = (ulong)uploadFileSize
            };
            source.SetMimeType("image/png");

            // Create space
            var space = CreateSpaceWithBuffer(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, source);

            Assert.AreEqual(space.Name, testSpaceName);

            // Clean up
            Marshal.FreeHGlobal(uploadFileDataPtr);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATESPACEWITHBUFFER_WITH_BULK_INVITE_TEST
        [Test]
        public static void CreateSpaceWithBufferWithBulkInviteTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var uploadFilePath = Path.GetFullPath("assets/OKO.png");
            var uploadFileSize = new FileInfo(uploadFilePath).Length;

            var uploadFileData = File.ReadAllBytes(uploadFilePath);

            System.IntPtr uploadFileDataPtr = Marshal.AllocHGlobal((int)uploadFileSize);
            Marshal.Copy(uploadFileData, 0, uploadFileDataPtr, (int)uploadFileSize);

            var source = new Systems.BufferAssetDataSource
            {
                Buffer = uploadFileDataPtr,
                BufferLength = (ulong)uploadFileSize
            };
            source.SetMimeType("image/png");

            var InviteUsers = CreateInviteUsers();

            // Create space
            var space = CreateSpaceWithBuffer(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, InviteUsers, source);

            Assert.AreEqual(space.Name, testSpaceName);

            using var getInvitesResult = spaceSystem.GetPendingUserInvites(space.Id).Result;
            var resCode = getInvitesResult.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var pendingInvites = getInvitesResult.GetPendingInvitesEmails();

            Assert.AreEqual(pendingInvites.Size(), 4UL);

            for (var i = 0UL; i < pendingInvites.Size(); i++)
                LogDebug($"Pending space invite for email: { pendingInvites[i] }");

            // Clean up
            Marshal.FreeHGlobal(uploadFileDataPtr);
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACEDESCRIPTION_TEST
        [Test]
        public static void UpdateSpaceDescriptionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            string updatedTestSpaceDescription = testSpaceDescription + "-Updated";
            UpdateSpace(spaceSystem, space, null, updatedTestSpaceDescription, null, out var updatedBasicSpace);

            Assert.AreEqual(updatedBasicSpace.Name, space.Name);
            Assert.AreEqual(updatedBasicSpace.Description, updatedTestSpaceDescription);
            Assert.AreEqual(updatedBasicSpace.Attributes, space.Attributes);

            using var updatedSpace = GetSpace(spaceSystem, space.Id);

            Assert.AreEqual(updatedSpace.Name, space.Name);
            Assert.AreEqual(updatedSpace.Description, updatedTestSpaceDescription);
            Assert.AreEqual(updatedSpace.Attributes, space.Attributes);

            // Clean up
            updatedSpace.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACETYPE_TEST
        [Test]
        public static void UpdateSpaceTypeTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var updatedTestSpaceAttributes = Systems.SpaceAttributes.Public;
            UpdateSpace(spaceSystem, space, null, null, updatedTestSpaceAttributes, out var updatedBasicSpace);

            Assert.AreEqual(updatedBasicSpace.Name, space.Name);
            Assert.AreEqual(updatedBasicSpace.Description, space.Description);
            Assert.AreEqual(updatedBasicSpace.Attributes, updatedTestSpaceAttributes);

            using var updatedSpace = GetSpace(spaceSystem, space.Id);

            Assert.AreEqual(updatedSpace.Name, space.Name);
            Assert.AreEqual(updatedSpace.Description, space.Description);
            Assert.AreEqual(updatedSpace.Attributes, updatedTestSpaceAttributes);

            // Clean up
            updatedBasicSpace.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACES_TEST
        [Test]
        public static void GetSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            _ = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Get spaces
            {
                using var result = spaceSystem.GetSpaces().Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var spaces = result.GetSpaces();

                Assert.IsGreaterThan(spaces.Size(), 0);

                var found = false;

                for (uint i = 0; i < spaces.Size(); i++)
                {
                    using var space = spaces[i];

                    if (space.Name == testSpaceName)
                    {
                        found = true;

                        break;
                    }
                }

                Assert.IsTrue(found);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_TEST
        [Test]
        public static void GetSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var resultSpace = GetSpace(spaceSystem, space.Id);

            Assert.AreEqual(resultSpace.Name, space.Name);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACESBYIDS_TEST
        [Test]
        public static void GetSpacesByIdsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testPublicSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testPrivateSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var publicSpace = CreateSpace(spaceSystem, testPublicSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null);
            var privateSpace = CreateSpace(spaceSystem, testPrivateSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var spaceIds = new string[2];
            spaceIds[0] = publicSpace.Id;
            spaceIds[1] = privateSpace.Id;

            var resultSpaces = GetSpacesByIds(spaceSystem, spaceIds);

            Assert.AreEqual(resultSpaces.Length, spaceIds.Length);

            var privateSpaceFound = false;
            var publicSpaceFound = false;

            for (var i = 0; i < resultSpaces.Length; i++)
            {
                if (resultSpaces[i].Name == testPrivateSpaceName)
                    privateSpaceFound = true;
                else if (resultSpaces[i].Name == testPublicSpaceName)
                    publicSpaceFound = true;

                if (privateSpaceFound && publicSpaceFound)
                    break;
            }

            Assert.IsTrue(privateSpaceFound);
            Assert.IsTrue(publicSpaceFound);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPUBLICSPACESASGUEST_TEST
        [Test]
        public static void GetPublicSpacesAsGuestTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            // Log in as default test user to create spaces
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            // Create test spaces
            const int SPACE_COUNT = 3;
            var spaces = new Systems.Space[SPACE_COUNT];

            for (int i = 0; i < SPACE_COUNT; i++)
            {
                spaces[i] = CreateSpace(spaceSystem, GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND"), "OLY-UNITTEST-SPACEDESC-REWIND", Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);
            }

            // Log out and log back in as guest user
            userSystem.TestLogOut();
            _ = userSystem.TestGuestLogIn();

            GetSpacesByAttributes(spaceSystem, true, false, null, null, out var publicSpaces);

            // Make sure that all returned spaces are public
            const ulong SpacesToCheckMaxNo = 50;  // rough limit on how many spaces we can check otherwise the URI that would get created inside GetSpacesByIds would be too long for CHS
            ulong SpacesToCheckNo = System.Math.Min(publicSpaces.Size(), SpacesToCheckMaxNo);

            var publicSpacesIds = new string[SpacesToCheckNo];

            for (var i = 0UL; i < SpacesToCheckNo; i++)
                publicSpacesIds[i] = publicSpaces[i].Id;

            var resultPublicSpaces = GetSpacesByIds(spaceSystem, publicSpacesIds);

            Assert.IsLessOrEqualThan((ulong)resultPublicSpaces.Length, SpacesToCheckNo);

            for (var i = 0; i < resultPublicSpaces.Length; i++)
            {
                Assert.IsTrue(resultPublicSpaces[i].Attributes.HasFlag(Systems.SpaceAttributes.IsDiscoverable));
                Assert.IsFalse(resultPublicSpaces[i].Attributes.HasFlag(Systems.SpaceAttributes.RequiresInvite));
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPUBLICSPACES_TEST
        [Test]
        public static void GetPublicSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testPublicSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testPublicSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null);

            GetSpacesByAttributes(spaceSystem, true, false, null, null, out var publicSpaces);

            Assert.IsGreaterOrEqualThan(publicSpaces.Size(), 1);

            var publicSpaceFound = false;

            for (uint i = 0; i < publicSpaces.Size(); i++)
            {
                using var publicSpace = publicSpaces[i];

                if (publicSpace.Name == testPublicSpaceName)
                {
                    publicSpaceFound = true;
                    break;
                }
            }

            Assert.IsTrue(publicSpaceFound);

            // Make sure that all returned spaces are public
            const ulong SpacesToCheckMaxNo = 50;  // rough limit on how many spaces we can check otherwise the URI that would get created inside GetSpacesByIds would be too long for CHS
            ulong SpacesToCheckNo = System.Math.Min(publicSpaces.Size(), SpacesToCheckMaxNo);

            var publicSpacesIds = new string[SpacesToCheckNo];

            for (var i = 0UL; i < SpacesToCheckNo; i++)
                publicSpacesIds[i] = publicSpaces[i].Id;

            var resultPublicSpaces = GetSpacesByIds(spaceSystem, publicSpacesIds);

            Assert.IsLessOrEqualThan((ulong)resultPublicSpaces.Length, SpacesToCheckNo);

            for (var i = 0; i < resultPublicSpaces.Length; i++)
                Assert.AreEqual(resultPublicSpaces[i].Attributes, Systems.SpaceAttributes.Public);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPRIVATESPACES_TEST
        [Test]
        public static void GetPrivateSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testPrivateSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testPrivateSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            GetSpacesByAttributes(spaceSystem, false, true, null, null, out var privateSpaces);

            Assert.IsTrue(privateSpaces.Size() >= 1);

            var privateSpaceFound = false;

            for (var i = 0UL; i < privateSpaces.Size(); i++)
            {
                using var privateSpace = privateSpaces[i];

                if (privateSpace.Name == testPrivateSpaceName)
                {
                    privateSpaceFound = true;
                    break;
                }
            }

            Assert.IsTrue(privateSpaceFound);

            // Make sure that all returned spaces are private
            const ulong SpacesToCheckMaxNo = 50;  // rough limit on how many spaces we can check otherwise the URI that would get created inside GetSpacesByIds would be too long for CHS
            ulong SpacesToCheckNo = System.Math.Min(privateSpaces.Size(), SpacesToCheckMaxNo);

            var privateSpacesIds = new string[SpacesToCheckNo];

            for (var i = 0UL; i < SpacesToCheckNo; ++i)
                privateSpacesIds[i] = privateSpaces[i].Id;

            var resultPrivateSpaces = GetSpacesByIds(spaceSystem, privateSpacesIds);

            Assert.IsLessOrEqualThan((ulong)resultPrivateSpaces.Length, SpacesToCheckNo);

            for (var i = 0; i < resultPrivateSpaces.Length; i++)
                Assert.AreEqual(resultPrivateSpaces[i].Attributes, Systems.SpaceAttributes.Private);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPAGINATEDPRIVATESPACES_TEST
        [Test]
        public static void GetPaginatedPrivateSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testPrivateSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            _ = CreateSpace(spaceSystem, testPrivateSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var privateSpaceFound = false;

            const int resultsPerPage = 10;
            int skipPreviousResultsNo = 0;

            while (!privateSpaceFound)
            {
                GetSpacesByAttributes(spaceSystem, false, true, skipPreviousResultsNo, resultsPerPage, out var resultSpaces);

                Assert.IsTrue(resultSpaces.Size() <= resultsPerPage);

                for (var i = 0UL; i < resultSpaces.Size(); i++)
                {
                    if (resultSpaces[i].Name == testPrivateSpaceName)
                        privateSpaceFound = true;

                    if (privateSpaceFound)
                        break;
                }

                skipPreviousResultsNo += resultsPerPage;
            }

            Assert.IsTrue(privateSpaceFound);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_JOINPUBLICSPACE_TEST
        [Test]
        public static void JoinPublicSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Login as an admin user in order to be able to create the test space
            var spaceOwnerUserId = userSystem.TestLogIn(pushCleanupFunction: false);

            using var publicSpace = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);

            UserSystemTests.LogOut(userSystem);

            // Log in as a guest user
            var guestUserId = UserSystemTests.LogInAsGuest(userSystem, pushCleanupFunction: false);

            using var result = spaceSystem.AddUserToSpace(publicSpace.Id, guestUserId).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            LogDebug($"User { guestUserId } was added to space { publicSpace.Id }");

            using var updatedPublicSpace = GetSpace(spaceSystem, publicSpace.Id);

            GetUsersRoles(spaceSystem, updatedPublicSpace, updatedPublicSpace.UserIds, out var retrievedUserRoles);

            Assert.AreEqual(retrievedUserRoles.Size(), 2UL);

            for (var i = 0UL; i < retrievedUserRoles.Size(); i++)
            {
                if (retrievedUserRoles[i].UserId == spaceOwnerUserId)
                    Assert.AreEqual(retrievedUserRoles[i].UserRole, Systems.SpaceUserRole.Owner);
                else if (retrievedUserRoles[i].UserId == guestUserId)
                    Assert.AreEqual(retrievedUserRoles[i].UserRole, Systems.SpaceUserRole.User);
                else
                    throw new System.Exception($"Encountered unexpected space user, id: '{ retrievedUserRoles[i].UserId }'");
            }

            UserSystemTests.LogOut(userSystem);

            // Log in as an admin user in order to be able to delete the test space
            _ = UserSystemTests.LogIn(userSystem);
            DeleteSpace(spaceSystem, publicSpace);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ADD_SITE_INFO_TEST
        [Test]
        public static void AddSiteInfoTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            AddSiteInfo(spaceSystem, null, space, out _);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_SITE_INFO_TEST
        [Test]
        public static void GetSiteInfoTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            AddSiteInfo(spaceSystem, "Site1", space, out var siteInfo1);
            AddSiteInfo(spaceSystem, "Site2", space, out var siteInfo2);

            GetSpaceSites(spaceSystem, space, out var sites);

            Assert.AreEqual(sites.Size(), 2UL);

            bool site1Found = false;
            bool site2Found = false;

            for (var i = 0UL; i < sites.Size(); i++)
            {
                if (sites[i].Name == siteInfo1.Name)
                    site1Found = true;
                else if (sites[i].Name == siteInfo2.Name)
                    site2Found = true;

                if (site1Found && site2Found)
                    break;
            }

            Assert.IsTrue(site1Found && site2Found);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_USER_ROLES_TEST
        [Test]
        public static void UpdateUserRolesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in with alt account first to get its ID
            var altUserId = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);
            UserSystemTests.LogOut(userSystem);

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var addUserResult = spaceSystem.AddUserToSpace(space.Id, altUserId).Result;
            var resCode = addUserResult.GetResultCode();
            Assert.AreEqual(resCode, Services.EResultCode.Success);
            space = addUserResult.GetSpace();

            var updatedSecondTestUserRole = new Systems.UserRoleInfo { UserId = altUserId, UserRole = Systems.SpaceUserRole.Moderator };

            // Update and verify user role
            UpdateUserRole(spaceSystem, space, updatedSecondTestUserRole);

            GetUsersRoles(spaceSystem, space, space.UserIds, out var retrievedUserRoles);

            Assert.AreEqual(retrievedUserRoles.Size(), 2UL);

            for (var i = 0UL; i < retrievedUserRoles.Size(); i++)
            {
                if (retrievedUserRoles[i].UserId == userId)
                    Assert.AreEqual(retrievedUserRoles[i].UserRole, Systems.SpaceUserRole.Owner);
                else if (retrievedUserRoles[i].UserId == altUserId)
                    Assert.AreEqual(retrievedUserRoles[i].UserRole, Systems.SpaceUserRole.Moderator);
                else
                    throw new System.Exception($"Encountered unexpected space user, id: '{ retrievedUserRoles[i].UserId }'");
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_GUEST_USER_ROLE_TEST
        [Test]
        public static void UpdateGuestUserRoleTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Login as an admin user in order to be able to create the test space
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            using var publicSpace = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);

            UserSystemTests.LogOut(userSystem);

            // Log in as a guest user
            var guestUserId = UserSystemTests.LogInAsGuest(userSystem, pushCleanupFunction: false);

            using var result = spaceSystem.AddUserToSpace(publicSpace.Id, guestUserId).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            UserSystemTests.LogOut(userSystem);

            // login as an admin user
            var spaceOwnerUserId = userSystem.TestLogIn(pushCleanupFunction: false);

            using var updatedGuestUserRole = new Systems.UserRoleInfo { UserId = guestUserId, UserRole = Systems.SpaceUserRole.Moderator };
            UpdateUserRole(spaceSystem, publicSpace, updatedGuestUserRole);

            GetRoleForSpecificUser(spaceSystem, publicSpace, guestUserId, out var userRoleInfo);

            Assert.AreEqual(userRoleInfo.UserRole, Systems.SpaceUserRole.Moderator);

            DeleteSpace(spaceSystem, publicSpace);
            UserSystemTests.LogOut(userSystem);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_SET_USER_ROLE_ON_INVITE_TEST
        [Test]
        public static void SetUserRoleOnInviteTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in with alt account first to get its ID
            var altUserId = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);
            UserSystemTests.LogOut(userSystem);

            // Login as an admin user in order to be able to create the test space
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            using var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);

            using var result = spaceSystem.InviteToSpace(space.Id, UserSystemTests.AlternativeLoginEmail, true).Result;
            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            GetRoleForSpecificUser(spaceSystem, space, altUserId, out var userRoleInfo);
            Assert.AreEqual(userRoleInfo.UserRole, Systems.SpaceUserRole.Moderator);

            DeleteSpace(spaceSystem, space);
            UserSystemTests.LogOut(userSystem);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACE_METADATA_TEST
        [Test]
        public static void UpdateSpaceMetadataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            using var testSpaceMetadata = new Common.Map<string, string>
            {
                ["site"] = "Void"
            };

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, testSpaceMetadata, null, null);

            GetSpaceMetadata(spaceSystem, space, out var retrievedSpaceMetadata);

            Assert.AreEqual(retrievedSpaceMetadata.Size(), testSpaceMetadata.Size());
            Assert.AreEqual(retrievedSpaceMetadata["site"], "Void");

            testSpaceMetadata["site"] = "MagOffice";

            UpdateSpaceMetadata(spaceSystem, space, testSpaceMetadata);

            GetSpaceMetadata(spaceSystem, space, out retrievedSpaceMetadata);

            Assert.AreEqual(retrievedSpaceMetadata.Size(), testSpaceMetadata.Size());
            Assert.AreEqual(retrievedSpaceMetadata["site"], "MagOffice");
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_SPACES_METADATA_TEST
        [Test]
        public static void GetSpacesMetadataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            using var testSpaceMetadata = new Common.Map<string, string>
            {
                ["site"] = "Void"
            };

            var space1 = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, testSpaceMetadata, null, null);
            var space2 = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, testSpaceMetadata, null, null);

            var spaces = new Common.Array<Systems.Space>(2)
            {
                [0] = space1,
                [1] = space2
            };
            GetSpacesMetadata(spaceSystem, spaces, out var retrievedSpacesMetadata);

            Assert.AreEqual(retrievedSpacesMetadata.Size(), 2UL);

            using var metadata1 = retrievedSpacesMetadata[space1.Id];

            Assert.AreEqual(metadata1.Size(), testSpaceMetadata.Size());
            Assert.AreEqual(metadata1["site"], "Void");

            using var metadata2 = retrievedSpacesMetadata[space2.Id];

            Assert.AreEqual(metadata2.Size(), testSpaceMetadata.Size());
            Assert.AreEqual(metadata2["site"], "Void");
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACE_THUMBNAIL_TEST
        [Test]
        public static void UpdateSpaceThumbnailTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword);

            // create a space without a thumbnail
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            {
                using var result = spaceSystem.GetSpaceThumbnail(space.Id).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseNotFound);
                Assert.AreEqual(result.GetUri(), "");
            }

            {
                var localFileName = "Fox.glb";
                var spaceThumbnail = new Systems.FileAssetDataSource
                {
                    FilePath = Path.GetFullPath("assets/" + localFileName)
                };
                spaceThumbnail.SetMimeType("model/gltf-binary");

                using var result = spaceSystem.UpdateSpaceThumbnail(space.Id, spaceThumbnail).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var resultGetThumbnail = spaceSystem.GetSpaceThumbnail(space.Id).Result;

                Assert.AreEqual(resultGetThumbnail.GetResultCode(), Services.EResultCode.Success);
                Assert.IsTrue(IsUriValid(resultGetThumbnail.GetUri(), localFileName));
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATESPACE_THUMBNAIL_WITH_BUFFER_TEST
        [Test]
        public static void UpdateSpaceThumbnailWithBufferTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword);

            // create a space without a thumbnail
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            {
                using var result = spaceSystem.GetSpaceThumbnail(space.Id).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseNotFound);
                Assert.AreEqual(result.GetUri(), "");
            }

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

                using var result = spaceSystem.UpdateSpaceThumbnailWithBuffer(space.Id, source).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var resultGetThumbnail = spaceSystem.GetSpaceThumbnail(space.Id).Result;

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
#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_CREATE_SPACE_EMPTY_METADATA_TEST
        [Test]
        public static void CreateSpaceWithEmptyMetadataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            var metadata = new Common.Map<string, string>();
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, metadata, null, null);

            GetSpaceMetadata(spaceSystem, space, out var retrievedSpaceMetadata);

            Assert.AreEqual(retrievedSpaceMetadata.Size(), 0UL);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACE_EMPTY_METADATA_TEST
        [Test]
        public static void UpdateSpaceWithEmptyMetadataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            UpdateSpaceMetadata(spaceSystem, space, null);

            GetSpaceMetadata(spaceSystem, space, out var retrievedSpaceMetadata);

            Assert.AreEqual(retrievedSpaceMetadata.Size(), 0UL);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GET_PENDING_INVITES_TEST
        [Test]
        public static void GetPendingUserInvitesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            using var result = spaceSystem.InviteToSpace(space.Id, "testnopus.pokemon@magnopus.com", null).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var getInvitesResult = spaceSystem.GetPendingUserInvites(space.Id).Result;
            resCode = getInvitesResult.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var pendingInvites = getInvitesResult.GetPendingInvitesEmails();

            Assert.AreEqual(pendingInvites.Size(), 1UL);

            for (var i = 0UL; i < pendingInvites.Size(); i++)
                LogDebug($"Pending space invite for email: { pendingInvites[i] }");
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_BULK_INVITE_TO_SPACE_TEST
        [Test]
        public static void BulkInviteToSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = UserSystemTests.LogIn(userSystem);

            var InviteUsers = CreateInviteUsers();

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, InviteUsers, null);

            using var result = spaceSystem.BulkInviteToSpace(space.Id, InviteUsers).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var getInvitesResult = spaceSystem.GetPendingUserInvites(space.Id).Result;
            resCode = getInvitesResult.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var pendingInvites = getInvitesResult.GetPendingInvitesEmails();

            Assert.AreEqual(pendingInvites.Size(), 4UL);

            for (var i = 0UL; i < pendingInvites.Size(); i++)
                LogDebug($"Pending space invite for email: { pendingInvites[i] }");
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETPUBLICSPACEMETADATA_TEST
        [Test]
        public static void GetPublicSpaceMetadataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            using var testSpaceMetadata = new Common.Map<string, string>
            {
                ["site"] = "Void"
            };

            // Log in with default user
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            // Create public space
            using var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, testSpaceMetadata, null, null, pushCleanupFunction: false);

            // Log out with default user and in with alt user
            UserSystemTests.LogOut(userSystem);
            var altUserId = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            // Enter space
            var enterResult = spaceSystem.EnterSpace(space.Id, true).Result;
            Assert.AreEqual(enterResult.GetResultCode(), Services.EResultCode.Success);
            
            // Get metadata for public space
            GetSpaceMetadata(spaceSystem, space, out var retrievedMetadata);

            Assert.AreEqual(retrievedMetadata.Size(), testSpaceMetadata.Size());
            Assert.IsTrue(retrievedMetadata.HasKey("site"));
            Assert.AreEqual(retrievedMetadata["site"], "Void");

            
            // Exit and re-enter space to verify its OK to always add self to public space
            var ok = spaceSystem.ExitSpaceAndDisconnect(enterResult.GetConnection()).Result;
            Assert.IsTrue(ok);
            {
                var enterResult2 = spaceSystem.EnterSpace(space.Id, true).Result;
                Assert.AreEqual(enterResult2.GetResultCode(), Services.EResultCode.Success);
                
                ok = spaceSystem.ExitSpaceAndDisconnect(enterResult.GetConnection()).Result;
                Assert.IsTrue(ok);
            }

            // Log back in with default user so space can be deleted
            UserSystemTests.LogOut(userSystem);
            _ = UserSystemTests.LogIn(userSystem);

            // Delete space
            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_THUMBNAIL_TEST
        [Test]
        public static void GetSpaceThumbnailTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            var localFileName = "test.json";
            var spaceThumbnail = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/" + localFileName)
            };

            // Create a space with a thumbnail
            using var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, spaceThumbnail, pushCleanupFunction: false);

            string initialSpaceThumbnailUri;

            {
                using var result = spaceSystem.GetSpaceThumbnail(space.Id).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                initialSpaceThumbnailUri = result.GetUri();

                Assert.IsTrue(IsUriValid(initialSpaceThumbnailUri, localFileName));
            }

            UserSystemTests.LogOut(userSystem);

            // Check that a user that doesn't belong to the space can retrieve the thumbnail
            _ = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            {
                using var result = spaceSystem.GetSpaceThumbnail(space.Id).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(initialSpaceThumbnailUri, result.GetUri());
            }

            UserSystemTests.LogOut(userSystem);

            _ = UserSystemTests.LogIn(userSystem);
            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GETSPACE_THUMBNAIL_WITH_GUEST_USER_TEST
        [Test]
        public static void GetSpaceThumbnailWithGuestUserTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            var localFileName = "test.json";
            var spaceThumbnail = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/" + localFileName)
            };

            // Create a space with a thumbnail
            using var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, spaceThumbnail, pushCleanupFunction: false);

            UserSystemTests.LogOut(userSystem);

            _ = UserSystemTests.LogInAsGuest(userSystem, pushCleanupFunction: false);

            var updateSpaceThumbnail = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/Fox.glb")
            };
            updateSpaceThumbnail.SetMimeType("model/gltf-binary");

            {
                // A guest shouldn't be able to update the space thumbnail
                using var result = spaceSystem.UpdateSpaceThumbnail(space.Id, updateSpaceThumbnail).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                // but it should be able to retrieve it
                var result = spaceSystem.GetSpaceThumbnail(space.Id).Result;
                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsTrue(IsUriValid(result.GetUri(), localFileName));
            }

            UserSystemTests.LogOut(userSystem);

            _ = UserSystemTests.LogIn(userSystem);

            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACE_GUEST_USER_BANNED_LIST
        [Test]
        public static void UpdateSpaceGuestUserBannedListTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in as a guest user to get a guest id
            var guestUserId = UserSystemTests.LogInAsGuest(userSystem, pushCleanupFunction: false);
            UserSystemTests.LogOut(userSystem);

            // Login as an admin user in order to be able to create the test space
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);

            using var addUserResult = spaceSystem.AddUserToSpace(space.Id, guestUserId).Result;
            var addUserResCode = addUserResult.GetResultCode();
            Assert.AreEqual(addUserResCode, Services.EResultCode.Success);
            space = addUserResult.GetSpace();

            // Add to ban list
            {
                using var result = spaceSystem.AddUserToSpaceBanList(space.Id, guestUserId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var _space = GetSpace(spaceSystem, space.Id);

                Assert.AreEqual(_space.BannedUserIds[0], guestUserId);
            }

            // Remove from ban list
            {
                using var result = spaceSystem.DeleteUserFromSpaceBanList(space.Id, guestUserId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var _space = GetSpace(spaceSystem, space.Id);

                Assert.IsTrue(_space.BannedUserIds.IsEmpty());
            }

            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_UPDATE_SPACE_STANDARD_USER_BANNED_LIST
        [Test]
        public static void UpdateSpaceStandardUserBannedListTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in as a guest user to get a guest id
            var altUserId = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);
            UserSystemTests.LogOut(userSystem);

            // Login as an admin user in order to be able to create the test space
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);

            using var addUserResult = spaceSystem.AddUserToSpace(space.Id, altUserId).Result;
            var addUserResCode = addUserResult.GetResultCode();
            Assert.AreEqual(addUserResCode, Services.EResultCode.Success);
            space = addUserResult.GetSpace();

            // Add to ban list
            {
                using var result = spaceSystem.AddUserToSpaceBanList(space.Id, altUserId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var _space = GetSpace(spaceSystem, space.Id);

                Assert.AreEqual(_space.BannedUserIds[0], altUserId);
            }

            // Remove from ban list
            {
                using var result = spaceSystem.DeleteUserFromSpaceBanList(space.Id, altUserId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                using var _space = GetSpace(spaceSystem, space.Id);

                Assert.IsTrue(_space.BannedUserIds.IsEmpty());
            }

            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_ENTER_SPACE_LIST
        [Test]
        public static void EnterSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Login as an admin user
            _ = userSystem.TestLogIn(pushCleanupFunction: false);
            using var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null, pushCleanupFunction: false);
            // Enter space
            {
                var enterResult = spaceSystem.EnterSpace(space.Id, true).Result;
                Assert.AreEqual(enterResult.GetResultCode(), Services.EResultCode.Success);
                var ok = spaceSystem.ExitSpaceAndDisconnect(enterResult.GetConnection()).Result;
                Assert.IsTrue(ok);
            }

            UserSystemTests.LogOut(userSystem);

            // Log in as a guest user
            var altUserId = userSystem.TestLogIn(email: UserSystemTests.AlternativeLoginEmail, password: UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            var enterResult2 = spaceSystem.EnterSpace(space.Id, true).Result;
            Assert.AreEqual(enterResult2.GetResultCode(), Services.EResultCode.Failed);

            UserSystemTests.LogOut(userSystem);

            _ = userSystem.TestLogIn(pushCleanupFunction: false);
            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_TEST
        [Test]
        public static void GeoLocationTest()
        {

            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var initialLocation = new Systems.GeoLocation();
            initialLocation.Latitude = 1.1;
            initialLocation.Longitude = 2.2;

            var initialOrientation = 90.0f;

            var initialGeoFence = new Common.Array<Systems.GeoLocation>(4);
            var geoFence0 = new Systems.GeoLocation();
            geoFence0.Latitude = 5.5;
            geoFence0.Longitude = 6.6;
            initialGeoFence[0] = geoFence0;
            initialGeoFence[3] = geoFence0;
            var geoFence1 = new Systems.GeoLocation();
            geoFence1.Latitude = 7.7;
            geoFence1.Longitude = 8.8;
            initialGeoFence[1] = geoFence1;
            var geoFence2 = new Systems.GeoLocation();
            geoFence2.Latitude = 9.9;
            geoFence2.Longitude = 10.0;
            initialGeoFence[2] = geoFence2;

            using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, initialLocation, initialOrientation, initialGeoFence).Result;
            Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Success);
            Assert.IsTrue(addGeoResult.HasSpaceGeoLocation());
            Assert.AreEqual(addGeoResult.GetSpaceGeoLocation().Location.Latitude, initialLocation.Latitude);
            Assert.AreEqual(addGeoResult.GetSpaceGeoLocation().Location.Longitude, initialLocation.Longitude);
            Assert.AreEqual(addGeoResult.GetSpaceGeoLocation().Orientation, initialOrientation);
            Assert.AreEqual(addGeoResult.GetSpaceGeoLocation().GeoFence.Size(), initialGeoFence.Size());
            for (var i = 0UL; i < addGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
            {
                Assert.AreEqual(addGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, initialGeoFence[i].Latitude);
                Assert.AreEqual(addGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, initialGeoFence[i].Longitude);
            }

            using var getGeoResult = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.IsTrue(getGeoResult.HasSpaceGeoLocation());
            Assert.AreEqual(getGeoResult.GetSpaceGeoLocation().Location.Latitude, initialLocation.Latitude);
            Assert.AreEqual(getGeoResult.GetSpaceGeoLocation().Location.Longitude, initialLocation.Longitude);
            Assert.AreEqual(getGeoResult.GetSpaceGeoLocation().Orientation, initialOrientation);
            Assert.AreEqual(getGeoResult.GetSpaceGeoLocation().GeoFence.Size(), initialGeoFence.Size());
            for (var i = 0UL; i < getGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
            {
                Assert.AreEqual(getGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, initialGeoFence[i].Latitude);
                Assert.AreEqual(getGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, initialGeoFence[i].Longitude);
            }

            var secondLocation = new Systems.GeoLocation();
            secondLocation.Latitude = 3.3;
            secondLocation.Longitude = 4.4;

            var secondOrientation = 270.0f;

            var secondGeoFence = new Common.Array<Systems.GeoLocation>(4);
            geoFence0.Latitude  = 11.1;
            geoFence0.Longitude = 12.2;
            secondGeoFence[0] = geoFence0;
            secondGeoFence[3] = geoFence0;
            geoFence1.Latitude = 13.3;
            geoFence1.Longitude = 14.4;
            secondGeoFence[1] = geoFence1;
            geoFence2.Latitude = 15.5;
            geoFence2.Longitude = 16.6;
            secondGeoFence[2] = geoFence2;

            using var updateGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, secondLocation, secondOrientation, secondGeoFence).Result;
            Assert.AreEqual(updateGeoResult.GetResultCode(), Services.EResultCode.Success);
            Assert.IsTrue(updateGeoResult.HasSpaceGeoLocation());
            Assert.AreEqual(updateGeoResult.GetSpaceGeoLocation().Location.Latitude, secondLocation.Latitude);
            Assert.AreEqual(updateGeoResult.GetSpaceGeoLocation().Location.Longitude, secondLocation.Longitude);
            Assert.AreEqual(updateGeoResult.GetSpaceGeoLocation().Orientation, secondOrientation);
            Assert.AreEqual(updateGeoResult.GetSpaceGeoLocation().GeoFence.Size(), secondGeoFence.Size());
            for (var i = 0UL; i < updateGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
            {
                Assert.AreEqual(updateGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, secondGeoFence[i].Latitude);
                Assert.AreEqual(updateGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, secondGeoFence[i].Longitude);
            }

            using var getUpdatedGeoResult = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(getUpdatedGeoResult.GetResultCode(), Services.EResultCode.Success);
            Assert.IsTrue(getUpdatedGeoResult.HasSpaceGeoLocation());
            Assert.AreEqual(getUpdatedGeoResult.GetSpaceGeoLocation().Location.Latitude, secondLocation.Latitude);
            Assert.AreEqual(getUpdatedGeoResult.GetSpaceGeoLocation().Location.Longitude, secondLocation.Longitude);
            Assert.AreEqual(getUpdatedGeoResult.GetSpaceGeoLocation().Orientation, secondOrientation);
            Assert.AreEqual(getUpdatedGeoResult.GetSpaceGeoLocation().GeoFence.Size(), secondGeoFence.Size());
            for (var i = 0UL; i < getUpdatedGeoResult.GetSpaceGeoLocation().GeoFence.Size(); ++i)
            {
                Assert.AreEqual(getUpdatedGeoResult.GetSpaceGeoLocation().GeoFence[i].Latitude, secondGeoFence[i].Latitude);
                Assert.AreEqual(getUpdatedGeoResult.GetSpaceGeoLocation().GeoFence[i].Longitude, secondGeoFence[i].Longitude);
            }

            using var deleteGeoResult = spaceSystem.DeleteSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(deleteGeoResult.GetResultCode(), Services.EResultCode.Success);

            using var GetDeletedGeoResult = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(GetDeletedGeoResult.GetResultCode(), Services.EResultCode.Success);
            Assert.IsFalse(GetDeletedGeoResult.HasSpaceGeoLocation());
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_VALIDATION_TEST
        [Test]
        public static void GeoLocationValidationTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var validLocation = new Systems.GeoLocation();
            validLocation.Latitude = 1.1;
            validLocation.Longitude = 2.2;
            
            var invalidLocation = new Systems.GeoLocation();
            invalidLocation.Latitude = 1.1;
            invalidLocation.Longitude = -500.0;

            var validOrientation = 90.0f;
            var invalidOrientation = -90.0f;

            var validGeoFence = new Common.Array<Systems.GeoLocation>(4);
            var shortGeoFence = new Common.Array<Systems.GeoLocation>(2);
            var invalidGeoFence = new Common.Array<Systems.GeoLocation>(4);
            var invalidGeoLocationGeoFence = new Common.Array<Systems.GeoLocation>(4);

            var geoFence0 = new Systems.GeoLocation();
            geoFence0.Latitude = 5.5;
            geoFence0.Longitude = 6.6;
            var geoFence1 = new Systems.GeoLocation();
            geoFence1.Latitude = 7.7;
            geoFence1.Longitude = 8.8;
            var geoFence2 = new Systems.GeoLocation();
            geoFence2.Latitude = 9.9;
            geoFence2.Longitude = 10.0;

            validGeoFence[0] = geoFence0;
            validGeoFence[1] = geoFence1;
            validGeoFence[2] = geoFence2;
            validGeoFence[3] = geoFence0;

            shortGeoFence[0] = geoFence0;
            shortGeoFence[1] = geoFence1;

            invalidGeoFence[0] = geoFence0;
            invalidGeoFence[1] = geoFence1;
            invalidGeoFence[2] = geoFence2;
            invalidGeoFence[3] = geoFence2;

            invalidGeoLocationGeoFence[0] = geoFence0;
            invalidGeoLocationGeoFence[1] = geoFence1;
            invalidGeoLocationGeoFence[2] = invalidLocation;
            invalidGeoLocationGeoFence[3] = geoFence0;

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, invalidLocation, validOrientation, validGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, invalidOrientation, validGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }
            
            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, shortGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }
            
            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, invalidGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, invalidGeoLocationGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            // Actually add a geo location and test again since a different code path is followed when one exists
            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, validGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Success);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, invalidLocation, validOrientation, validGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, invalidOrientation, validGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, shortGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, invalidGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var addGeoResult = spaceSystem.UpdateSpaceGeoLocation(space.Id, validLocation, validOrientation, invalidGeoLocationGeoFence).Result;
                Assert.AreEqual(addGeoResult.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var deleteGeoResult = spaceSystem.DeleteSpaceGeoLocation(space.Id).Result;
                Assert.AreEqual(deleteGeoResult.GetResultCode(), Services.EResultCode.Success);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_WITHOUT_PERMISSION_TEST
        [Test]
        public static void GeoLocationWithoutPermissionTest()
        {

            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = userSystem.TestLogIn(UserSystemTests.DefaultLoginEmail, UserSystemTests.DefaultLoginPassword, pushCleanupFunction: false);

            // Create a space as the primary user
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null, pushCleanupFunction: false);

            var initialLocation = new Systems.GeoLocation();
            initialLocation.Latitude = 1.1;
            initialLocation.Longitude = 2.2;

            var initialOrientation = 90.0f;

            // Switch to the alt user to try and create the geo location
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            using var addGeoResultAsAlt = spaceSystem.UpdateSpaceGeoLocation(space.Id, initialLocation, initialOrientation, null).Result;
            Assert.AreEqual(addGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(addGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);

            using var getGeoResultAsAlt = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(getGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(getGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);

            // Switch back to the primary user to actually create the geo location
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.DefaultLoginEmail, UserSystemTests.DefaultLoginPassword, pushCleanupFunction: false);

            using var addGeoResultAsPrimary = spaceSystem.UpdateSpaceGeoLocation(space.Id, initialLocation, initialOrientation, null).Result;
            Assert.AreEqual(addGeoResultAsPrimary.GetResultCode(), Services.EResultCode.Success);
            Assert.IsTrue(addGeoResultAsPrimary.HasSpaceGeoLocation());
            Assert.AreEqual(addGeoResultAsPrimary.GetSpaceGeoLocation().Location.Latitude, initialLocation.Latitude);
            Assert.AreEqual(addGeoResultAsPrimary.GetSpaceGeoLocation().Location.Longitude, initialLocation.Longitude);
            Assert.AreEqual(addGeoResultAsPrimary.GetSpaceGeoLocation().Orientation, initialOrientation);

            // Switch to the alt user again
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            var secondLocation = new Systems.GeoLocation();
            secondLocation.Latitude = 3.3;
            secondLocation.Longitude = 4.4;

            var secondOrientation = 270.0f;

            // Test they cannot update
            using var updateGeoResultAsAlt = spaceSystem.UpdateSpaceGeoLocation(space.Id, secondLocation, secondOrientation, null).Result;
            Assert.AreEqual(updateGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(updateGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);

            // Test they cannot delete
            using var deleteGeoResultAsAlt = spaceSystem.DeleteSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(deleteGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(deleteGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);

            // Switch back to the primary user to cleanup
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.DefaultLoginEmail, UserSystemTests.DefaultLoginPassword);

            using var deleteGeoResultAsPrimary = spaceSystem.DeleteSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(deleteGeoResultAsPrimary.GetResultCode(), Services.EResultCode.Success);

            using var GetDeletedGeoResultAsPrimary = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(GetDeletedGeoResultAsPrimary.GetResultCode(), Services.EResultCode.Success);
            Assert.IsFalse(GetDeletedGeoResultAsPrimary.HasSpaceGeoLocation());

            DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACESYSTEM_TESTS || RUN_SPACESYSTEM_GEOLOCATION_WITHOUT_PERMISSION_PUBLIC_SPACE_TEST
        [Test]
        public static void GeoLocationWithoutPermissionPublicSpaceTest()
        {

            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            _ = userSystem.TestLogIn(UserSystemTests.DefaultLoginEmail, UserSystemTests.DefaultLoginPassword, pushCleanupFunction: false);

            // Create a space as the primary user
            var space = CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Public, null, null, null, pushCleanupFunction: false);

            var initialLocation = new Systems.GeoLocation();
            initialLocation.Latitude = 1.1;
            initialLocation.Longitude = 2.2;

            var initialOrientation = 90.0f;

            // Switch to the alt user to try and create the geo location
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            using var addGeoResultAsAlt = spaceSystem.UpdateSpaceGeoLocation(space.Id, initialLocation, initialOrientation, null).Result;
            Assert.AreEqual(addGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(addGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);
            
            // Switch back to the primary user to actually create the geo location
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.DefaultLoginEmail, UserSystemTests.DefaultLoginPassword, pushCleanupFunction: false);

            using var addGeoResultAsPrimary = spaceSystem.UpdateSpaceGeoLocation(space.Id, initialLocation, initialOrientation, null).Result;
            Assert.AreEqual(addGeoResultAsPrimary.GetResultCode(), Services.EResultCode.Success);
            Assert.IsTrue(addGeoResultAsPrimary.HasSpaceGeoLocation());
            Assert.AreEqual(addGeoResultAsPrimary.GetSpaceGeoLocation().Location.Latitude, initialLocation.Latitude);
            Assert.AreEqual(addGeoResultAsPrimary.GetSpaceGeoLocation().Location.Longitude, initialLocation.Longitude);
            Assert.AreEqual(addGeoResultAsPrimary.GetSpaceGeoLocation().Orientation, initialOrientation);

            // Switch to the alt user again
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            var secondLocation = new Systems.GeoLocation();
            secondLocation.Latitude = 3.3;
            secondLocation.Longitude = 4.4;
            
            var secondOrientation = 270.0f;

            // Test they cannot update
            using var updateGeoResultAsAlt = spaceSystem.UpdateSpaceGeoLocation(space.Id, secondLocation, secondOrientation, null).Result;
            Assert.AreEqual(updateGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(updateGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);

            // Test they cannot delete
            using var deleteGeoResultAsAlt = spaceSystem.DeleteSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(deleteGeoResultAsAlt.GetResultCode(), Services.EResultCode.Failed);
            Assert.AreEqual(deleteGeoResultAsAlt.GetHttpResultCode(), (ushort)Web.EResponseCodes.ResponseForbidden);

            using var getGeoResultAsAlt = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(getGeoResultAsAlt.GetResultCode(), Services.EResultCode.Success);
            Assert.IsTrue(getGeoResultAsAlt.HasSpaceGeoLocation());
            Assert.AreEqual(getGeoResultAsAlt.GetSpaceGeoLocation().Location.Latitude, initialLocation.Latitude);
            Assert.AreEqual(getGeoResultAsAlt.GetSpaceGeoLocation().Location.Longitude, initialLocation.Longitude);
            Assert.AreEqual(getGeoResultAsAlt.GetSpaceGeoLocation().Orientation, initialOrientation);

            // Switch back to the primary user to cleanup
            UserSystemTests.LogOut(userSystem);
            _ = userSystem.TestLogIn(UserSystemTests.DefaultLoginEmail, UserSystemTests.DefaultLoginPassword);

            using var deleteGeoResultAsPrimary = spaceSystem.DeleteSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(deleteGeoResultAsPrimary.GetResultCode(), Services.EResultCode.Success);

            using var GetDeletedGeoResultAsPrimary = spaceSystem.GetSpaceGeoLocation(space.Id).Result;
            Assert.AreEqual(GetDeletedGeoResultAsPrimary.GetResultCode(), Services.EResultCode.Success);
            Assert.IsFalse(GetDeletedGeoResultAsPrimary.HasSpaceGeoLocation());

            DeleteSpace(spaceSystem, space);
        }
#endif

    }
}
