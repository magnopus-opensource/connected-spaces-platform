using System.Collections;

using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

using Common = Olympus.Foundation.Common;
using Systems = Olympus.Foundation.Systems;
using Services = Olympus.Foundation.Services;


namespace Tests
{
    public class SpatialSystemTests : TestsBase
    {
        static IEnumerator DeletePointOfInterest(Systems.PointOfInterestSystem poiSystem, Systems.PointOfInterest poi)
        {
            var task = poiSystem.DeletePOI(poi);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            Debug.Log($"POI deleted (Id: { poi.Id }, Name: { poi.Name })");
        }

        static IEnumerator CreatePointOfInterest(Result<Systems.PointOfInterest> outPOI, Systems.PointOfInterestSystem poiSystem, Common.Array<string> tags, double? latitude, double? longitude, Systems.AssetCollection assetCollection)
        {
            string poiTitle = "OLY-UNITTEST-POI-TITLE";
            string poiDescription = "OLY-UNITTEST-POI-DESC-REWIND";
            string poiOwner = "OLY-UNITTEST-OWNER";
            var poiType = Systems.EPointOfInterestType.DEFAULT;

            var poiLocation = new Systems.GeoLocation
            {
                Longitude = longitude ?? 180.0, // Default values for the tests
                Latitude = latitude ?? 90.0
            };

            var poiAssetCollection = new Systems.AssetCollection();

            if (assetCollection != null)
                poiAssetCollection = assetCollection;
            else
                poiAssetCollection.Id = "OLY-UNITTEST-ASSET-COLLECTION-ID"; // For the POI creation only the ID is relevant

            string uniquePOIName = GenerateUniqueString(poiTitle);

            var task = poiSystem.CreatePOI(poiTitle, poiDescription, uniquePOIName, tags, poiType, poiOwner, poiLocation, poiAssetCollection);

            yield return task.RunAsCoroutine();
 
            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outPOI.Value = result.GetPointOfInterest();
            Debug.Log($"POI created (Id: { outPOI.Value.Id }, Name: { outPOI.Value.Name })");
            var _outPOI = outPOI.Value;
            PushCleanupFunction(() => DeletePointOfInterest(poiSystem, _outPOI));
        }

        static IEnumerator GetAssetCollectionFromPOI(Result<Systems.AssetCollection> outAssetCollection, Systems.AssetSystem assetSystem, Systems.PointOfInterest poi)
        {
            var task = assetSystem.GetAssetCollectionById(poi.AssetCollectionId);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outAssetCollection.Value = result.GetAssetCollection();
        }


        [UnityTest, Order(1)]
        public IEnumerator CreatePOITest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out var poiSystem);

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create point of interest
            var poi = new Result<Systems.PointOfInterest>();
            yield return CreatePointOfInterest(poi, poiSystem, null, null, null, null);
        }

        [UnityTest, Order(2)]
        public IEnumerator CreatePOIWithTagsTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out var poiSystem);

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            var Tags = new Common.Array<string>(2);
            Tags[0] = "POITag1";
            Tags[1] = "POITag2";

            // Create point of interest
            var poi = new Result<Systems.PointOfInterest>();
            yield return CreatePointOfInterest(poi, poiSystem, Tags, null, null, null);
        }

        [UnityTest, Order(3)]
        public IEnumerator GetPOIsInAreaTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out var poiSystem);

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            var poiLatitude = 45.0;
            var poiLongitude = 160.0;

            // Create point of interest
            var poi = new Result<Systems.PointOfInterest>();
            yield return CreatePointOfInterest(poi, poiSystem, null, poiLatitude, poiLongitude, null);

            // Search for newly created POI inside circular area
            {
                var searchOrigin = new Systems.GeoLocation
                {
                    Longitude = 160.0,
                    Latitude = 44.0
                };
                var searchRadius = 130000.0;

                var task = poiSystem.GetPOIsInArea(searchOrigin, searchRadius);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                var pois = result.GetPOIs();

                Assert.Greater(pois.Size(), 0);

                bool found = false;

                for (uint idx = 0; idx < pois.Size(); ++idx)
                {
                    if (pois[idx].Name == poi.Value.Name)
                    {
                        found = true;

                        break;
                    }
                }

                Assert.True(found);
            }
        }

        [UnityTest, Order(4)]
        public IEnumerator GetPOIAssetCollectionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out var poiSystem);

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            var testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            var testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            var testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            // Create asset collection
            var assetCollection = new Result<Systems.AssetCollection>();
            yield return AssetSystemTests.CreateAssetCollection(assetCollection, assetSystem, space, testAssetCollectionName);

            // Create point of interest
            var poi = new Result<Systems.PointOfInterest>();
            yield return CreatePointOfInterest(poi, poiSystem, null, null, null, assetCollection);

            // Get asset collection
            var poiAssetCollection = new Result<Systems.AssetCollection>();
            yield return GetAssetCollectionFromPOI(poiAssetCollection, assetSystem, poi);

            Assert.AreEqual(assetCollection.Value.Id, poiAssetCollection.Value.Id);
            Assert.AreEqual(assetCollection.Value.Name, poiAssetCollection.Value.Name);
            Assert.AreEqual(assetCollection.Value.SpaceIds[0], poiAssetCollection.Value.SpaceIds[0]);
        }
    }
}
