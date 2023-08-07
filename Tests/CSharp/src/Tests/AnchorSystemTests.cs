using Common = Csp.Common;
using Services = Csp.Services;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


#nullable enable annotations


namespace CSPEngine
{
    static class AnchorSystemTests
    {
        static Systems.Anchor CreateAnchor(Systems.AnchorSystem anchorSystem, string assetCollectionId, Systems.GeoLocation? location, Common.Map<string, string>? spatialKeyValue, Common.Array<string>? tags, bool pushCleanupFunction = true, bool disposeFoundationResources = true)
        {
            var anchorLocation = location ?? new Systems.GeoLocation() { Latitude = 90.0, Longitude = 180.0 };

            string uniqueThirdPartyAnchorId = GenerateUniqueString("OLY-UNITTEST-ID");

            var anchorPosition = new Systems.OlyAnchorPosition() { X = 100.0, Y = 100.0, Z = 100.0 };
            var anchorRotation = new Systems.OlyRotation() { X = 100.0, Y = 100.0, Z = 100.0, W = 100.0 };
            var anchorSpatialKeyValue = spatialKeyValue ?? new Common.Map<string, string>() { ["TestKey1"] = "TestValue1", ["TestKey2"] = "TestValue2" };

            var anchorTags = tags;

            if (anchorTags == null)
            {
                anchorTags = new Common.Array<string>(2);
                anchorTags[0] = "Test1";
                anchorTags[1] = "Test2";
            }

            var result = anchorSystem.CreateAnchor(Systems.AnchorProvider.GoogleCloudAnchors,
                uniqueThirdPartyAnchorId,
                assetCollectionId,
                anchorLocation,
                anchorPosition,
                anchorRotation,
                anchorSpatialKeyValue,
                anchorTags)
                .Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            var anchor = result.GetAnchor();
            LogDebug($"Anchor Created (Id: {anchor.Id})");

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteSingleAnchor(anchorSystem, anchor, disposeFoundationResources));

            return anchor;
        }

        static Systems.Anchor CreateAnchorInSpace(Systems.AnchorSystem anchorSystem, string spaceId, ulong spaceEntityId, string assetCollectionId, Systems.GeoLocation? location, Common.Map<string, string>? spatialKeyValue, Common.Array<string>? tags, bool pushCleanupFunction = true, bool disposeFoundationResources = true)
        {
            var anchorLocation = location ?? new Systems.GeoLocation() { Latitude = 90.0, Longitude = 180.0 };

            string uniqueThirdPartyAnchorId = GenerateUniqueString("OLY-UNITTEST-ID");

            var anchorPosition = new Systems.OlyAnchorPosition() { X = 100.0, Y = 100.0, Z = 100.0 };
            var anchorRotation = new Systems.OlyRotation() { X = 100.0, Y = 100.0, Z = 100.0, W = 100.0 };
            var anchorSpatialKeyValue = spatialKeyValue ?? new Common.Map<string, string>() { ["TestKey1"] = "TestValue1", ["TestKey2"] = "TestValue2" };

            var anchorTags = tags;

            if (anchorTags == null)
            {
                anchorTags = new Common.Array<string>(2);
                anchorTags[0] = "Test1";
                anchorTags[1] = "Test2";
            }

            var result = anchorSystem.CreateAnchorInSpace(Systems.AnchorProvider.GoogleCloudAnchors,
                uniqueThirdPartyAnchorId,
                spaceId,
                spaceEntityId,
                assetCollectionId,
                anchorLocation,
                anchorPosition,
                anchorRotation,
                anchorSpatialKeyValue,
                anchorTags)
                .Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            var anchor = result.GetAnchor();
            LogDebug($"Anchor Created (Id: {anchor.Id})");

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteSingleAnchor(anchorSystem, anchor, disposeFoundationResources));

            return anchor;
        }

        static void DeleteSingleAnchor(Systems.AnchorSystem anchorSystem, Systems.Anchor anchor, bool disposeFoundationResources = true)
        {
            DeleteAnchors(anchorSystem, new[] { anchor.Id });

            if (disposeFoundationResources)
                anchor.Dispose();
        }

        static void DeleteAnchors(Systems.AnchorSystem anchorSystem, string[] anchorIDs)
        {
            var _anchorIDs = new Common.Array<string>((ulong)anchorIDs.Length);

            for (var i = 0UL; i < _anchorIDs.Size(); i++)
                _anchorIDs[i] = anchorIDs[i];

            var result = anchorSystem.DeleteAnchors(_anchorIDs).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            foreach (var id in anchorIDs)
                LogDebug($"Anchor Deleted (Id: {id})");
        }

        static Systems.AnchorResolution CreateAnchorResolution(Systems.AnchorSystem anchorSystem, string anchorId, bool disposeFoundationResources = true)
        {
            var successfullyResolved = true;
            var resolveAttempted = 3;
            var resolveTime = 1000;
            var testTag = "TestTag";
            var tags = new Common.Array<string>((ulong)1);
            tags[0] = testTag;

            var result = anchorSystem.CreateAnchorResolution(anchorId, successfullyResolved, resolveAttempted, resolveTime, tags).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            var anchorResolution = result.GetAnchorResolution();

            Assert.AreEqual(anchorResolution.SuccessfullyResolved, successfullyResolved);
            Assert.AreEqual(anchorResolution.ResolveAttempted, resolveAttempted);
            Assert.AreEqual(anchorResolution.ResolveTime, resolveTime);
            Assert.AreEqual(anchorResolution.Tags.Size(), (ulong)1);
            Assert.AreEqual(anchorResolution.Tags[0], testTag);

            return anchorResolution;
        }

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_CREATE_ANCHOR_TEST
        [Test]
        public static void CreateAnchorTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST");

            var createAssetCollectionResult = assetSystem.CreateAssetCollection(null, null, testAssetCollectionName, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection = createAssetCollectionResult.GetAssetCollection();

            // Create anchor
            var anchor = CreateAnchor(anchorSystem, assetCollection.Id, null, null, null);

            Assert.AreEqual(anchor.ThirdPartyAnchorProvider, Systems.AnchorProvider.GoogleCloudAnchors);
            Assert.AreEqual(anchor.SpaceId, "");
            Assert.AreEqual(anchor.SpaceEntityId, 0UL);
            Assert.AreEqual(anchor.AssetCollectionId, assetCollection.Id);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_CREATE_ANCHOR_IN_SPACE_TEST
        [Test]
        public static void CreateAnchorInSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            SpaceSystemTests.EnterSpace(spaceSystem, space.Id);

            var connection = MultiplayerTests.CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Connect to multiplayer service
            {
                var ok = connection.Connect().Result;

                Assert.IsTrue(ok);
            }

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var objectName = "TestObject";
            MultiplayerTests.CreateObject(entitySystem, objectName, out var createdObject, false);

            var createAssetCollectionResult = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection = createAssetCollectionResult.GetAssetCollection();

            // Create anchor
            var anchor = CreateAnchorInSpace(anchorSystem, space.Id, createdObject.GetId(), assetCollection.Id, null, null, null);

            Assert.AreEqual(anchor.ThirdPartyAnchorProvider, Systems.AnchorProvider.GoogleCloudAnchors);
            Assert.AreEqual(anchor.SpaceId, space.Id);
            Assert.AreEqual(anchor.SpaceEntityId, createdObject.GetId());
            Assert.AreEqual(anchor.AssetCollectionId, assetCollection.Id);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_DELETE_MULTIPLE_ANCHORS_TEST
        [Test]
        public static void DeleteMultipleAnchorsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";
            string testAssetCollectionName1 = GenerateUniqueString("OLY-UNITTEST");
            string testAssetCollectionName2 = GenerateUniqueString("OLY-UNITTEST");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            SpaceSystemTests.EnterSpace(spaceSystem, space.Id);

            var connection = MultiplayerTests.CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Connect to multiplayer service
            {
                var ok = connection.Connect().Result;

                Assert.IsTrue(ok);
            }

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var objectName1 = "TestObject1";
            MultiplayerTests.CreateObject(entitySystem, objectName1, out var createdObject1, false);
            var objectName2 = "TestObject2";
            MultiplayerTests.CreateObject(entitySystem, objectName2, out var createdObject2, false);

            var createAssetCollectionResult1 = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName1, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection1 = createAssetCollectionResult1.GetAssetCollection();
            var createAssetCollectionResult2 = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName2, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection2 = createAssetCollectionResult2.GetAssetCollection();

            var createdAnchorIDs = new string[2];

            // We're creating the following two anchors with the test auto cleanup functionality disabled so we can manually delete both of them later
            var anchor1 = CreateAnchorInSpace(anchorSystem, space.Id, createdObject1.GetId(), assetCollection1.Id, null, null, null, false);
            Systems.Anchor anchor2;

            createdAnchorIDs[0] = anchor1.Id;

            try
            {
                // anchor2 = CreateAnchor(anchorSystem, null, null,null,false);
                anchor2 = CreateAnchorInSpace(anchorSystem, space.Id, createdObject2.GetId(), assetCollection2.Id, null, null, null, false);
                createdAnchorIDs[1] = anchor2.Id;
            }
            catch (AssertFailedException e)
            {
                DeleteSingleAnchor(anchorSystem, anchor1);
                anchor1.Dispose();

                throw e;
            }

            // Deleting the anchors manually in order to test the multiple delete API
            DeleteAnchors(anchorSystem, createdAnchorIDs);
            anchor2.Dispose();
            anchor1.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_GET_ANCHORS_INSIDE_CIRCULAR_AREA_TEST
        [Test]
        public static void GetAnchorsInsideCircularAreaTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);
            var spaceIds = new Common.Array<string>(1);
            spaceIds[0] = space.Id;

            SpaceSystemTests.EnterSpace(spaceSystem, space.Id);

            var connection = MultiplayerTests.CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Connect to multiplayer service
            {
                var ok = connection.Connect().Result;

                Assert.IsTrue(ok);
            }

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var objectName = "TestObject";
            MultiplayerTests.CreateObject(entitySystem, objectName, out var createdObject, false);

            var createAssetCollectionResult = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection = createAssetCollectionResult.GetAssetCollection();

            using var anchorLocation = new Systems.GeoLocation() { Latitude = 45.0, Longitude = 160.0 };

            // Create anchor
            var anchor = CreateAnchorInSpace(anchorSystem, space.Id, createdObject.GetId(), assetCollection.Id, anchorLocation, null, null);

            // Search for the newly created anchor inside a circular area
            using var searchLocationOrigin = new Systems.GeoLocation() { Latitude = 44.0, Longitude = 160.0 };
            var searchRadius = 130000.0;

            var tags = new Common.Array<string>(2);
            tags[0] = "Test1";
            tags[1] = "Test2";

            var spacialKeys = new Common.Array<string>(2);
            spacialKeys[0] = "TestKey1";
            spacialKeys[1] = "TestKey2";

            var spacialValues = new Common.Array<string>(2);
            spacialValues[0] = "TestValue1";
            spacialValues[1] = "TestValue2";

            // Get anchors
            Common.Array<Systems.Anchor> anchorCollection;

            {
                using var result = anchorSystem.GetAnchorsInArea(searchLocationOrigin, searchRadius, spacialKeys, spacialValues, tags, true, spaceIds, null, null).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                anchorCollection = result.GetAnchors();
            }

            // We should have at least the anchor we've just created
            Assert.IsGreaterThan(anchorCollection.Size(), 0);

            bool found = false;

            for (uint idx = 0; idx < anchorCollection.Size(); ++idx)
            {
                if (anchorCollection[idx].Id == anchor.Id)
                {
                    var returnSpatialKeyValue = anchorCollection[idx].SpatialKeyValue;

                    Assert.AreEqual(returnSpatialKeyValue.Size(), spacialValues.Size());

                    for (var i = 0UL; i < returnSpatialKeyValue.Size(); i++)
                    {
                        Assert.AreEqual(returnSpatialKeyValue[spacialKeys[i]], spacialValues[i]);
                    }

                    var returnTags = anchorCollection[idx].Tags;

                    Assert.AreEqual(returnTags.Size(), tags.Size());

                    for (var i = 0UL; i < returnTags.Size(); i++)
                    {
                        Assert.AreEqual(returnTags[i], tags[i]);
                    }

                    found = true;

                    break;
                }
            }

            anchorCollection.Dispose();

            Assert.IsTrue(found);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_GETANCHORSINSPACE_TEST
        [Test]
        public static void GetAnchorsInSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";
            string testAssetCollectionName1 = GenerateUniqueString("OLY-UNITTEST");
            string testAssetCollectionName2 = GenerateUniqueString("OLY-UNITTEST");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            SpaceSystemTests.EnterSpace(spaceSystem, space.Id);

            var connection = MultiplayerTests.CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Connect to multiplayer service
            {
                var ok = connection.Connect().Result;

                Assert.IsTrue(ok);
            }

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var objectName1 = "TestObject1";
            MultiplayerTests.CreateObject(entitySystem, objectName1, out var createdObject1, false);
            var objectName2 = "TestObject2";
            MultiplayerTests.CreateObject(entitySystem, objectName2, out var createdObject2, false);

            var createAssetCollectionResult1 = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName1, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection1 = createAssetCollectionResult1.GetAssetCollection();
            var createAssetCollectionResult2 = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName2, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection2 = createAssetCollectionResult2.GetAssetCollection();

            var anchor1 = CreateAnchorInSpace(anchorSystem, space.Id, createdObject1.GetId(), assetCollection1.Id, null, null, null);
            var anchor2 = CreateAnchorInSpace(anchorSystem, space.Id, createdObject2.GetId(), assetCollection2.Id, null, null, null);

            // Get anchors
            Common.Array<Systems.Anchor> anchorCollection;

            {
                using var result = anchorSystem.GetAnchorsInSpace(space.Id, null, null).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                anchorCollection = result.GetAnchors();

                Assert.AreEqual(anchorCollection.Size(), 2UL);
            }

            var anchorsFound = 0;

            for (uint idx = 0; idx < anchorCollection.Size(); ++idx)
            {
                Assert.AreEqual(anchorCollection[idx].SpaceId, space.Id);
                if (anchorCollection[idx].ThirdPartyAnchorId == anchor1.ThirdPartyAnchorId ||
                    anchorCollection[idx].ThirdPartyAnchorId == anchor2.ThirdPartyAnchorId)
                {
                    ++anchorsFound;
                }
            }

            anchorCollection.Dispose();

            Assert.AreEqual(anchorsFound, 2);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_CREATE_ANCHOR_RESOLUTION_TEST
        [Test]
        public static void CreateAnchorResolutionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out var anchorSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST");
            string testSpaceDescription = "OLY-UNITTEST-DESC";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            SpaceSystemTests.EnterSpace(spaceSystem, space.Id);

            var connection = MultiplayerTests.CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Connect to multiplayer service
            {
                var ok = connection.Connect().Result;

                Assert.IsTrue(ok);
            }

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var objectName = "TestObject";
            MultiplayerTests.CreateObject(entitySystem, objectName, out var createdObject, false);

            var createAssetCollectionResult = assetSystem.CreateAssetCollection(space.Id, null, testAssetCollectionName, null, Systems.EAssetCollectionType.DEFAULT, null).Result;
            var assetCollection = createAssetCollectionResult.GetAssetCollection();

            // Create anchor
            var anchor = CreateAnchorInSpace(anchorSystem, space.Id, createdObject.GetId(), assetCollection.Id, null, null, null);

            // Create AnchorResolution
            var anchorResolution = CreateAnchorResolution(anchorSystem, anchor.Id);
        }
#endif
    }
}

