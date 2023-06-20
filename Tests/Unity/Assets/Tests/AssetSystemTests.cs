using System.Collections;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

using Common = Olympus.Foundation.Common;
using Services = Olympus.Foundation.Services;
using Systems = Olympus.Foundation.Systems;


namespace Tests
{
    public class AssetSystemTests : TestsBase
    {
        static IEnumerator DeleteAssetCollection(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection)
        {
            var task = assetSystem.DeleteAssetCollection(assetCollection);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            Debug.Log($"AssetCollection deleted (Id: { assetCollection.Id })");
        }

        public static IEnumerator CreateAssetCollection(Result<Systems.AssetCollection> outAssetCollection, Systems.AssetSystem assetSystem, Systems.Space space, string name)
        {
            var task = assetSystem.CreateAssetCollection(space, null, name, null, Systems.EAssetCollectionType.DEFAULT, null);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outAssetCollection.Value = result.GetAssetCollection();
            Debug.Log($"AssetCollection created (Id: { outAssetCollection.Value.Id }, Name: { outAssetCollection.Value.Name })");
            var _outAssetCollection = outAssetCollection.Value;
            PushCleanupFunction(() => DeleteAssetCollection(assetSystem, _outAssetCollection));
        }

        public static IEnumerator GetAssetCollections(Result<Common.Array<Systems.AssetCollection>> outAssetCollections, Systems.AssetSystem assetSystem, Systems.Space space)
        {
            var task = assetSystem.GetAssetCollectionsByCriteria(space, null, Systems.EAssetCollectionType.DEFAULT, null, null, null, null);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outAssetCollections.Value = result.GetAssetCollections();
        }

        static IEnumerator DeleteAsset(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, Systems.Asset asset)
        {
            var task = assetSystem.DeleteAsset(assetCollection, asset);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            Debug.Log($"Asset deleted (Id: { asset.Id }, AssetCollection Id: { assetCollection.Id })");
        }

        public static IEnumerator CreateAsset(Result<Systems.Asset> outAsset, Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, string name)
        {
            var task = assetSystem.CreateAsset(assetCollection, name, null, Systems.EAssetType.MODEL);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outAsset.Value = result.GetAsset();
            Debug.Log($"Asset created (Id: { outAsset.Value.Id }, Name: { outAsset.Value.Name })");
            var _outAsset = outAsset.Value;
            PushCleanupFunction(() => DeleteAsset(assetSystem, assetCollection, _outAsset));
        }

        public static IEnumerator GetAssetsInCollection(Result<Common.Array<Systems.Asset>> outAssets, Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection)
        {
            var task = assetSystem.GetAssetsInCollection(assetCollection);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outAssets.Value = result.GetAssets();
        }


        [UnityTest, Order(1)]
        public IEnumerator CreateAssetCollectionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            // Create asset collection
            var assetCollection = new Result<Systems.AssetCollection>();
            yield return CreateAssetCollection(assetCollection, assetSystem, space, testAssetCollectionName);

            // Get asset collections
            var assetCollections = new Result<Common.Array<Systems.AssetCollection>>();
            yield return GetAssetCollections(assetCollections, assetSystem, space);
            Assert.AreEqual(assetCollections.Value.Size(), 1);
        }

        [UnityTest, Order(2)]
        public IEnumerator CreateAssetTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            // Create asset collection
            var assetCollection = new Result<Systems.AssetCollection>();
            yield return CreateAssetCollection(assetCollection, assetSystem, space, testAssetCollectionName);

            // Create asset
            var asset = new Result<Systems.Asset>();
            yield return CreateAsset(asset, assetSystem, assetCollection, testAssetName);

            // Get assets
            var assets = new Result<Common.Array<Systems.Asset>>();
            yield return GetAssetsInCollection(assets, assetSystem, assetCollection);
            Assert.AreEqual(assets.Value.Size(), 1);
        }

        [UnityTest, Order(3)]
        public IEnumerator UploadAssetTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            // Create asset collection
            var assetCollection = new Result<Systems.AssetCollection>();
            yield return CreateAssetCollection(assetCollection, assetSystem, space, testAssetCollectionName);

            // Create asset
            var asset = new Result<Systems.Asset>();
            yield return CreateAsset(asset, assetSystem, assetCollection, testAssetName);

            asset.Value.Name = "Test.dat";
            var source = new Systems.FileAssetDataSource
            {
                // Unity sets the current working directory for EditMode and PlayMode to the project root, so this works fine
                FilePath = Path.GetFullPath("Assets/test.json")
            };

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => Debug.Log($"Uploading asset... ({e.Progress}%)");
                var task = assetSystem.UploadAssetData(assetCollection, asset, source);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                asset.Value.Uri = result.GetUri();
            }

            byte[] downloadedData;

            // Get data
            {
                var task = assetSystem.DownloadAssetData(asset);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                downloadedData = new byte[result.GetDataLength()];
                Marshal.Copy(result.GetData(), downloadedData, 0, (int)result.GetDataLength());

                var fileData = File.ReadAllBytes(Path.GetFullPath("assets/test.json"));

                Assert.True(downloadedData.SequenceEqual(fileData));
            }
        }
    }
}
