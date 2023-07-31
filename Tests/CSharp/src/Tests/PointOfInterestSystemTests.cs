using Common = Csp.Common;
using Services = Csp.Services;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


namespace CSPEngine
{
    static class PointOfInterestSystemTests
    {
        static void DeletePointOfInterest(Systems.PointOfInterestSystem poiSystem, Systems.PointOfInterest poi, bool disposeFoundationResources = true)
        {
            using var result = poiSystem.DeletePOI(poi).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            LogDebug($"POI deleted (Id: {poi.Id}, Name: {poi.Name})");

            if (disposeFoundationResources)
                poi.Dispose();
        }

        public static void CreatePointOfInterest(Systems.PointOfInterestSystem poiSystem, Common.Array<string> tags,
                                    Systems.GeoLocation location,
                                    Systems.AssetCollection assetCollection, out Systems.PointOfInterest poi, bool disposeFoundationResources = true)
        {
            string poiTitle = "OLY-UNITTEST-POI-TITLE";
            string poiDescription = "OLY-UNITTEST-POI-DESC-REWIND";
            string poiOwner = "OLY-UNITTEST-OWNER";
            var poiType = Systems.EPointOfInterestType.DEFAULT;

            var poiLocation = location ?? new Systems.GeoLocation() { Latitude = 90.0, Longitude = 180.0 };
            var poiAssetCollection = assetCollection ?? new Systems.AssetCollection() { Id = "OLY-UNITTEST-ASSET-COLLECTION-ID" };

            string uniquePOIName = GenerateUniqueString(poiTitle);

            using var result = poiSystem.CreatePOI(poiTitle, poiDescription, uniquePOIName, tags, poiType, poiOwner, poiLocation, poiAssetCollection).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            poi = result.GetPointOfInterest();
            LogDebug($"POI created (Id: {poi.Id}, Name: {poi.Name})");

            var outPoi = poi;

            PushCleanupFunction(() => DeletePointOfInterest(poiSystem, outPoi, disposeFoundationResources));
        }

        static void GetAssetCollectionFromPOI(Systems.AssetSystem assetSystem, Systems.PointOfInterest poi, out Systems.AssetCollection assetCollection)
        {
            using var result = assetSystem.GetAssetCollectionById(poi.AssetCollectionId).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            assetCollection = result.GetAssetCollection();
        }

        static bool AreTestAssetCollectionsEqual(Systems.AssetCollection lhs, Systems.AssetCollection rhs)
            => lhs.Id == rhs.Id
            && lhs.Name == rhs.Name
            && lhs.SpaceIds[0] == rhs.SpaceIds[0];


#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_CREATEPOI_TEST
        [Test]
        public static void CreatePOITest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out var poiSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create POI
            CreatePointOfInterest(poiSystem, null, null, null, out _);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_CREATEPOI_WITH_TAGS_TEST
        [Test]
        public static void CreatePOIWithTagsTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out var poiSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            using var tags = new Common.Array<string>(2);
            tags[0] = "POITag1";
            tags[1] = "POITag2";

            // Create POI
            CreatePointOfInterest(poiSystem, tags, null, null, out _);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_GETPOI_INSIDE_CIRCULAR_AREA_TEST
        [Test]
        public static void GetPOIInsideCircularAreaTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out var poiSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            using var poiLocation = new Systems.GeoLocation() { Latitude = 45.0, Longitude = 160.0 };

            // Create POI
            CreatePointOfInterest(poiSystem, null, poiLocation, null, out var poi);

            // Search for the newly created POI inside a circular area
            using var searchLocationOrigin = new Systems.GeoLocation() { Latitude = 44.0, Longitude = 160.0 };
            var searchRadius = 130000.0;

            // Get POIs
            using var result = poiSystem.GetPOIsInArea(searchLocationOrigin, searchRadius).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            using var poiCollection = result.GetPOIs();

            // We should have at least the POI we've created
            Assert.IsGreaterThan(poiCollection.Size(), 0);

            bool found = false;

            for (var i = 0UL; i < poiCollection.Size(); i++)
            {
                if (poiCollection[i].Name == poi.Name)
                {
                    found = true;

                    break;
                }
            }

            Assert.IsTrue(found);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_POISYSTEM_TESTS || RUN_POISYSTEM_GET_ASSETCOLLECTION_FROM_POI_TEST
        [Test]
        public static void GetAssetCollectionFromPOITest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out var poiSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            // Create asset collection
            AssetSystemTests.CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create POI
            CreatePointOfInterest(poiSystem, null, null, assetCollection, out var poi);

            // Get POI asset collection
            GetAssetCollectionFromPOI(assetSystem, poi, out var retrievedAssetCollection);

            Assert.IsTrue(AreTestAssetCollectionsEqual(assetCollection, retrievedAssetCollection));
        }
#endif
    }
}
