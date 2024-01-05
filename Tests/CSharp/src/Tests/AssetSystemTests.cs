using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

using Common = Csp.Common;
using Systems = Csp.Systems;
using Multiplayer = Csp.Multiplayer;

using CSharpTests;
using static CSharpTests.TestHelper;


#nullable enable annotations


namespace CSPEngine
{
    static class AssetSystemTests
    {
        static void DeleteAssetCollection(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, bool disposeFoundationResources = true)
        {
            using var result = assetSystem.DeleteAssetCollection(assetCollection).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            LogDebug($"AssetCollection deleted (Id: { assetCollection.Id })");

            if (disposeFoundationResources)
                assetCollection.Dispose();
        }

        public static void CreateAssetCollection(Systems.AssetSystem assetSystem, Systems.Space? space, string parentId, string name, Systems.EAssetCollectionType? type, Common.Array<string>? tags, out Systems.AssetCollection assetCollection, bool disposeFoundationResources = true)
        {
            var AssetCollectionType = type ?? Systems.EAssetCollectionType.DEFAULT;

            using var result = assetSystem.CreateAssetCollection(space?.Id, parentId, name, null, AssetCollectionType, tags).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assetCollection = result.GetAssetCollection();

            LogDebug($"AssetCollection created (Id: { assetCollection.Id }, Name: { assetCollection.Name })");

            var outAssetCollection = assetCollection;

            PushCleanupFunction(() => DeleteAssetCollection(assetSystem, outAssetCollection, disposeFoundationResources));
        }

        static void GetAssetCollections(Systems.AssetSystem assetSystem, Systems.Space space, out Common.Array<Systems.AssetCollection> assetCollections)
        {
            using var result = assetSystem.GetAssetCollectionsByCriteria(space.Id, null, Systems.EAssetCollectionType.DEFAULT, null, null, null, null).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assetCollections = result.GetAssetCollections();
        }

        static void GetAssetCollectionsByCriteria(Systems.AssetSystem assetSystem, Systems.Space space, string parentId, Systems.EAssetCollectionType? type, Common.Array<string>? tags, Common.Array<string>? names, int? ResultsSkipNumber, int? ResultsMaxNumber, out Common.Array<Systems.AssetCollection> assetCollections)
        {
            using var result = assetSystem.GetAssetCollectionsByCriteria(space?.Id, parentId, type, tags, names, ResultsSkipNumber, ResultsMaxNumber).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assetCollections = result.GetAssetCollections();
        }

        static void GetAssetCollectionByName(Systems.AssetSystem assetSystem, String assetCollectionName, out Systems.AssetCollection assetCollection)
        {
            using var result = assetSystem.GetAssetCollectionByName(assetCollectionName).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assetCollection = result.GetAssetCollection();
        }

        static void GetAssetCollectionsByIds(Systems.AssetSystem assetSystem, string[] ids, out Common.Array<Systems.AssetCollection> assetCollections)
        {
            Assert.IsGreaterThan(ids.Length, 0);

            var _ids = ids.ToFoundationArray();

            using var result = assetSystem.GetAssetCollectionsByIds(_ids).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assetCollections = result.GetAssetCollections();
        }

        static void DeleteAsset(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, Systems.Asset asset, bool disposeFoundationResources = true)
        {
            using var result = assetSystem.DeleteAsset(assetCollection, asset).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            LogDebug($"Asset deleted (Id: { asset.Id }, AssetCollection Id: { assetCollection.Id })");

            if (disposeFoundationResources)
                asset.Dispose();
        }

        public static void CreateAsset(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, string name, string? thirdPartyPackagedAssetIdentifier,Systems.EThirdPartyPlatform? thirdPartyPlatform, out Systems.Asset asset, bool disposeFoundationResources = true)
        {
            using var result = assetSystem.CreateAsset(assetCollection, name, thirdPartyPackagedAssetIdentifier,thirdPartyPlatform, Systems.EAssetType.MODEL).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            asset = result.GetAsset();

            LogDebug($"Asset created (Id: { asset.Id }, Name: { asset.Name })");

            var outAsset = asset;

            PushCleanupFunction(() => DeleteAsset(assetSystem, assetCollection, outAsset, disposeFoundationResources));
        }

        static void GetAssetsInCollection(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, out Common.Array<Systems.Asset> assets)
        {
            using var result = assetSystem.GetAssetsInCollection(assetCollection).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assets = result.GetAssets();
        }

        static void GetAssetsByCollectionIds(Systems.AssetSystem assetSystem, string[] ids, out Common.Array<Systems.Asset> assets)
        {
            Assert.AreNotEqual(ids.Length, 0);

            var _ids = ids.ToFoundationArray();

            using var result = assetSystem.GetAssetsByCollectionIds(_ids).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assets = result.GetAssets();
        }

        static void GetAssetsByCriteria(Systems.AssetSystem assetSystem, string[] assetCollectionIds, string[]? assetIds,
            string[]? assetNames, Common.Array<Systems.EAssetType>? assetTypes, out Common.Array<Systems.Asset> assets)
        {
            var _assetIds = assetIds?.ToFoundationArray();
            var _assetNames = assetNames?.ToFoundationArray();

            using var result = assetSystem.GetAssetsByCriteria(assetCollectionIds.ToFoundationArray(), _assetIds, _assetNames, assetTypes).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            assets = result.GetAssets();
        }

        static void UpdateAssetCollectionMetadata(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, Common.Map<string, string> metaDataIn, out Common.Map<string, string> metaDataOut)
        {
            using var result = assetSystem.UpdateAssetCollectionMetadata(assetCollection, metaDataIn).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            using var updatedAssetCollection = result.GetAssetCollection();

            Assert.AreEqual(updatedAssetCollection.Id, assetCollection.Id);
            Assert.AreEqual(updatedAssetCollection.Name, assetCollection.Name);
            Assert.AreNotEqual(updatedAssetCollection.UpdatedAt, assetCollection.UpdatedAt);

            for (var i = 0UL; i < updatedAssetCollection.Tags.Size(); i++)
                Assert.AreEqual(updatedAssetCollection.Tags[i], assetCollection.Tags[i]);

            metaDataOut = new Common.Map<string, string>(updatedAssetCollection.GetMetadataImmutable());
        }

        public static void UploadAssetData(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, Systems.Asset asset, Systems.AssetDataSource source, out string uriOut)
        {
            var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            uriOut = result.GetUri();
        }

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_CREATEASSETCOLLECTION_TEST
        [Test]
        public static void CreateAssetCollectionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out _);

            // Get asset collections
            GetAssetCollections(assetSystem, space, out var assetCollections);

            Assert.AreEqual(assetCollections.Size(), 1UL);
            Assert.AreEqual(assetCollections[0].Name, testAssetCollectionName);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_CREATEASSETCOLLECTION_NOSPACE_TEST
        [Test]
        public static void CreateAssetCollectionNoSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out _, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create asset collection
            CreateAssetCollection(assetSystem, null, null, testAssetCollectionName, null, null, out _);

            // Get asset collections
            GetAssetCollectionByName(assetSystem, testAssetCollectionName, out var assetCollection);

            Assert.AreEqual(assetCollection.Name, testAssetCollectionName);
            Assert.IsTrue(assetCollection.SpaceId.Length > 0);
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_GETASSETCOLLECTIONSBYIDS_TEST
        [Test]
        public static void GetAssetCollectionsByIdsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName1 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetCollectionName2 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collections
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName1, null, null, out var assetCollection1);
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName2, null, null, out var assetCollection2);

            // Get asset collections
            GetAssetCollectionsByIds(assetSystem, new[] { assetCollection1.Id, assetCollection2.Id }, out var assetCollections);

            Assert.AreEqual(assetCollections.Size(), 2UL);

            bool found1 = false, found2 = false;

            for (var i = 0UL; i < assetCollections.Size(); i++)
            {
                var assetCollection = assetCollections[i];

                if (assetCollection.Id == assetCollection1.Id)
                    found1 = true;
                else if (assetCollection.Id == assetCollection2.Id)
                    found2 = true;

                assetCollection.Dispose();

                if (found1 && found2)
                    break;
            }

            Assert.IsTrue(found1 && found2);

            // Clean up
            assetCollections.Dispose();
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_CREATEASSET_TEST
        [Test]
        public static void CreateAssetTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, thirdPartyPackagedAssetIdentifier, null, out _);

            // Get assets
            GetAssetsInCollection(assetSystem, assetCollection, out var assets);

            Assert.AreEqual(assets.Size(), 1UL);
            Assert.AreEqual(assets[0].Name, testAssetName);
            Assert.AreEqual(assets[0].GetThirdPartyPackagedAssetIdentifier(), thirdPartyPackagedAssetIdentifier);
            
            var inboundAsset = assets[0];
            inboundAsset.SetThirdPartyPackagedAssetIdentifier("Test");
            Assert.AreEqual(inboundAsset.GetThirdPartyPackagedAssetIdentifier(), "Test");
            inboundAsset.SetThirdPartyPlatformType(Systems.EThirdPartyPlatform.UNREAL);
            Assert.AreEqual(inboundAsset.GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNREAL);

            var updatedAsset = assetSystem.UpdateAsset(inboundAsset).Result;
            Assert.AreEqual(updatedAsset.GetResultCode(), Systems.EResultCode.Success);

            Assert.AreEqual(updatedAsset.GetAsset().GetThirdPartyPackagedAssetIdentifier(), "Test");
            Assert.AreEqual(updatedAsset.GetAsset().GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNREAL);
            updatedAsset.Dispose();
            inboundAsset.Dispose();
            
            // Clean up
            assets.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_CREATEASSET_NOSPACE_TEST
        [Test]
        public static void CreateAssetNoSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out _, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create asset collection
            CreateAssetCollection(assetSystem, null, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, thirdPartyPackagedAssetIdentifier, null, out _);

            // Get assets
            GetAssetsInCollection(assetSystem, assetCollection, out var assets);

            Assert.AreEqual(assets.Size(), 1UL);
            Assert.AreEqual(assets[0].Name, testAssetName);
            Assert.AreEqual(assets[0].GetThirdPartyPackagedAssetIdentifier(), thirdPartyPackagedAssetIdentifier);
            
            var inboundAsset = assets[0];
            inboundAsset.SetThirdPartyPackagedAssetIdentifier("Test");
            Assert.AreEqual(inboundAsset.GetThirdPartyPackagedAssetIdentifier(), "Test");
            inboundAsset.SetThirdPartyPlatformType(Systems.EThirdPartyPlatform.UNREAL);
            Assert.AreEqual(inboundAsset.GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNREAL);

            var updatedAsset = assetSystem.UpdateAsset(inboundAsset).Result;
            Assert.AreEqual(updatedAsset.GetResultCode(), Systems.EResultCode.Success);

            Assert.AreEqual(updatedAsset.GetAsset().GetThirdPartyPackagedAssetIdentifier(), "Test");
            Assert.AreEqual(updatedAsset.GetAsset().GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNREAL);
            updatedAsset.Dispose();
            inboundAsset.Dispose();
            
            // Clean up
            assets.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_GETASSETSBYCOLLECTIONIDS_TEST
        [Test]
        public static void GetAssetsByCollectionIdsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName1 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetCollectionName2 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName1 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string testAssetName2 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string testAssetName3 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collections
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName1, null, null, out var assetCollection1);
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName2, null, null, out var assetCollection2);

            // Create assets
            CreateAsset(assetSystem, assetCollection1, testAssetName1, null, null, out var asset1);
            CreateAsset(assetSystem, assetCollection1, testAssetName2, null, null, out var asset2);
            CreateAsset(assetSystem, assetCollection2, testAssetName3, null, null, out var asset3);

            // Get assets
            GetAssetsByCollectionIds(assetSystem, new[] { assetCollection1.Id, assetCollection2.Id }, out var assets);

            Assert.AreEqual(assets.Size(), 3UL);

            bool found1 = false, found2 = false, found3 = false;

            for (var i = 0UL; i < assets.Size(); i++)
            {
                var asset = assets[i];

                if (asset.Id == asset1.Id)
                    found1 = true;
                else if (asset.Id == asset2.Id)
                    found2 = true;
                else if (asset.Id == asset3.Id)
                    found3 = true;

                if (found1 && found2 && found3)
                    break;

                asset.Dispose();
            }

            Assert.IsTrue(found1 && found2 && found3);

            // Clean up
            assets.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_GETASSETCOLLECTIONS_BY_DIFFERENT_CRITERIA_TEST
        [Test]
        public static void GetAssetCollectionsByDifferentCriteriaTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName1 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetCollectionName2 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetCollectionName3 = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");

            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var Tags = new Common.Array<string>(1);
            Tags[0] = space.Id;

            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName1, Systems.EAssetCollectionType.SPACE_THUMBNAIL, null, out var assetCollection1);
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName2, Systems.EAssetCollectionType.SPACE_THUMBNAIL, Tags, out var assetCollection2);
            CreateAssetCollection(assetSystem, space, assetCollection1.Id, testAssetCollectionName3, null, null, out var assetCollection3);

            // Search by space
            {
                GetAssetCollectionsByCriteria(assetSystem, space, null, null, null, null, null, null, out var assetCollections);

                Assert.AreEqual(assetCollections.Size(), 4UL);

                assetCollections.Dispose();
            }

            // Search by parentId
            {
                GetAssetCollectionsByCriteria(assetSystem, null, assetCollection1.Id, null, null, null, null, null, out var assetCollections);

                Assert.AreEqual(assetCollections.Size(), 1UL);
                Assert.AreEqual(assetCollections[0].Id, assetCollection3.Id);
                Assert.AreEqual(assetCollections[0].Name, assetCollection3.Name);

                assetCollections.Dispose();
            }

            // Search by tag
            {
                GetAssetCollectionsByCriteria(assetSystem, null, null, null, Tags, null, null, null, out var assetCollections);

                Assert.AreEqual(assetCollections.Size(), 1UL);
                Assert.AreEqual(assetCollections[0].Id, assetCollection2.Id);
                Assert.AreEqual(assetCollections[0].Name, assetCollection2.Name);

                assetCollections.Dispose();
            }

            // Search by names and types
            {
                var names = new Common.Array<string>(2);
                names[0] = testAssetCollectionName1;
                names[1] = testAssetCollectionName2;

                // Search for Default types with these names
                GetAssetCollectionsByCriteria(assetSystem, null, null, Systems.EAssetCollectionType.DEFAULT, null, names, null, null, out var assetCollections1);

                Assert.AreEqual(assetCollections1.Size(), 0UL);

                // Search same names with Space thumbnail type 
                GetAssetCollectionsByCriteria(assetSystem, null, null, Systems.EAssetCollectionType.SPACE_THUMBNAIL, null, names, null, null, out var assetCollections2);

                Assert.AreEqual(assetCollections2.Size(), 2UL);

                bool foundAssetCollection1 = false, foundAssetCollection2 = false;

                for (var i = 0UL; i < assetCollections2.Size(); i++)
                {
                    var currentAsset = assetCollections2[i];

                    if (currentAsset.Id == assetCollection1.Id)
                        foundAssetCollection1 = true;
                    else if (currentAsset.Id == assetCollection2.Id)
                        foundAssetCollection2 = true;

                    currentAsset.Dispose();

                    if (foundAssetCollection1 && foundAssetCollection2)
                        break;
                }

                Assert.IsTrue(foundAssetCollection1 && foundAssetCollection2);

                assetCollections2.Dispose();
                assetCollections1.Dispose();
            }

            // Test pagination
            {
                GetAssetCollectionsByCriteria(assetSystem, space, null, null, null, null, 1, 1, out var assetCollections);

                Assert.AreEqual(assetCollections.Size(), 1UL);

                assetCollections.Dispose();
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_GETASSETS_BY_DIFFERENT_CRITERIA_TEST
        [Test]
        public static void GetAssetsByDifferentCriteriaTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName1 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string testAssetName2 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            CreateAsset(assetSystem, assetCollection, testAssetName1, null, null, out var asset1);
            CreateAsset(assetSystem, assetCollection, testAssetName2, null, null, out var asset2);

            // Search by asset id
            {
                GetAssetsByCriteria(assetSystem, new[] {assetCollection.Id}, new[] { asset1.Id }, null, null, out var assets);

                Assert.AreEqual(assets.Size(), 1UL);
                Assert.AreEqual(assets[0].Id, asset1.Id);
                Assert.AreEqual(assets[0].Name, asset1.Name);

                assets.Dispose();
            }

            // Search by asset name
            {
                GetAssetsByCriteria(assetSystem, new[] {assetCollection.Id}, null, new[] { asset1.Name }, null, out var assets);

                Assert.AreEqual(assets.Size(), 1UL);
                Assert.AreEqual(assets[0].Id, asset1.Id);
                Assert.AreEqual(assets[0].Name, asset1.Name);

                assets.Dispose();
            }

            // Search by asset names and types, both assets are of type Model
            {
                using var assetTypes1 = new Common.Array<Systems.EAssetType>(1);
                assetTypes1[0] = Systems.EAssetType.VIDEO;

                GetAssetsByCriteria(assetSystem, new[] {assetCollection.Id}, null, new[] { asset1.Name, asset2.Name }, assetTypes1, out var assets1);

                Assert.AreEqual(assets1.Size(), 0UL);

                // Next to Model append Video too
                using var assetTypes2 = new Common.Array<Systems.EAssetType>(2);
                assetTypes2[0] = Systems.EAssetType.MODEL;
                assetTypes2[1] = Systems.EAssetType.VIDEO;

                GetAssetsByCriteria(assetSystem, new[] {assetCollection.Id}, null, new[] { asset1.Name, asset2.Name }, assetTypes2, out var assets2);

                Assert.AreEqual(assets2.Size(), 2UL);

                bool foundAsset1 = false, foundAsset2 = false;

                for (var i = 0UL; i < assets2.Size(); i++)
                {
                    var currentAsset = assets2[i];

                    if (currentAsset.Id == asset1.Id)
                        foundAsset1 = true;
                    else if (currentAsset.Id == asset2.Id)
                        foundAsset2 = true;

                    currentAsset.Dispose();

                    if (foundAsset1 && foundAsset2)
                        break;
                }

                Assert.IsTrue(foundAsset1 && foundAsset2);

                assets2.Dispose();
                assets1.Dispose();
            }
        }
#endif

        // disabled as for some reason it was causing other C# tests failures ONLY on TC, this will be investigated in a separate ticket
#if false
#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_GETASSETS_FROM_MULTIPLE_ASSET_COLLECTIONS_TEST
        [Test]
        public static void GetAssetsFromMultipleAssetCollectionsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testFirstAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testSecondAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName1 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string testAssetName2 = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            _ = UserSystemTests.LogIn(userSystem);

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            CreateAssetCollection(assetSystem, space, null, testFirstAssetCollectionName, null, null, out var firstAssetCollection);
            CreateAssetCollection(assetSystem, space, null, testSecondAssetCollectionName, null, null, out var secondAssetCollection);

            CreateAsset(assetSystem, firstAssetCollection, testAssetName1, null, out var asset1);
            CreateAsset(assetSystem, secondAssetCollection, testAssetName2, null, out var asset2);

            // Try to search but don't specify any asset collection Ids, only add one Asset Id though
            {
                string[] emptyAssetCollectionIds = {};
                using var result = assetSystem.GetAssetsByCriteria(emptyAssetCollectionIds.ToFoundationArray(), null, null, null).Result;
                Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Failed);
            }

            // Search by both asset collection Ids at the same time
            {
                GetAssetsByCriteria(assetSystem, new[] {firstAssetCollection.Id, secondAssetCollection.Id}, null, null, null, out var assets);

                Assert.AreEqual(assets.Size(), 2UL);

                bool foundAsset1 = false, foundAsset2 = false;

                for (var i = 0UL; i < assets.Size(); i++)
                {
                    var currentAsset = assets[i];

                    if (currentAsset.Id == asset1.Id)
                        foundAsset1 = true;
                    else if (currentAsset.Id == asset2.Id)
                        foundAsset2 = true;

                    currentAsset.Dispose();

                    if (foundAsset1 && foundAsset2)
                        break;
                }

                Assert.IsTrue(foundAsset1 && foundAsset2);

                assets.Dispose();
            }

            // Search by both asset collection Ids and only one Asset Id
            {
                GetAssetsByCriteria(assetSystem, new[] {firstAssetCollection.Id, secondAssetCollection.Id}, new[] { asset2.Id }, null, null, out var assets);

                Assert.AreEqual(assets.Size(), 1UL);
                Assert.AreEqual(assets[0].Id, asset2.Id);
                Assert.AreEqual(assets[0].Name, asset2.Name);

                assets.Dispose();
            }
        }
#endif
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_EXTERNALURIASSET_TEST
        [Test]
        public static void ExternalUriAssetTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";
            string testExternalUri = "https://github.com/KhronosGroup/glTF-Sample-Models/raw/master/2.0/Duck/glTF-Binary/Duck.glb";
            string testExternalMimetype = "model/gltf-binary";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, thirdPartyPackagedAssetIdentifier, null, out _);

            // Get assets
            {
                GetAssetsInCollection(assetSystem, assetCollection, out var assets);

                Assert.AreEqual(assets.Size(), 1UL);

                using var asset = assets[0];

                Assert.AreEqual(asset.Name, testAssetName);
                Assert.AreEqual(asset.Uri, "");
                Assert.AreEqual(asset.MimeType, "");

                using var modifiedAsset = new Systems.Asset(asset)
                {
                    ExternalMimeType = testExternalMimetype,
                    ExternalUri = testExternalUri
                };

                using var result = assetSystem.UpdateAsset(modifiedAsset).Result;
                var resCode = result.GetResultCode();

                using var updatedAsset = result.GetAsset();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);
                Assert.AreEqual(updatedAsset.Name, testAssetName);
                Assert.AreEqual(updatedAsset.Uri, testExternalUri);
                Assert.AreEqual(updatedAsset.MimeType, testExternalMimetype);

                assets.Dispose();
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPLOADASSET_AS_FILE_TEST
        [Test]
        public static void UploadAssetAsFileTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/test.json")
            };
            var FileNoMimeType = "";
            var FileMimeType = "application/json";

            // Upload data with no mime type
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");

                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                asset.Uri = result.GetUri();
            }

            // Get asset to check default mime type
            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;

                Assert.AreNotEqual(result.GetAsset().MimeType, FileNoMimeType);
                Assert.AreEqual(result.GetAsset().MimeType, "application/octet-stream");
            }

            source.SetMimeType(FileMimeType);

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");
                
                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                asset.Uri = result.GetUri();
            }

            // Get asset to check correct mime type
            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;

                Assert.AreEqual(result.GetAsset().MimeType, FileMimeType);
            }

            byte[] downloadedData;

            // Get data
            {
                using var result = assetSystem.DownloadAssetData(asset).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                downloadedData = new byte[result.GetDataLength()];
                Marshal.Copy(result.GetData(), downloadedData, 0, (int)result.GetDataLength());

                var fileData = File.ReadAllBytes(Path.GetFullPath("assets/test.json"));

                Assert.IsTrue(downloadedData.SequenceEqual(fileData));
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPLOADASSET_AS_FILE_NOSPACE_TEST
        [Test]
        public static void UploadAssetAsFileNoSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out _, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create asset collection
            CreateAssetCollection(assetSystem, null, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/test.json")
            };
            var FileNoMimeType = "";
            var FileMimeType = "application/json";

            // Upload data with no mime type
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");

                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                asset.Uri = result.GetUri();
            }

            // Get asset to check default mime type
            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;

                Assert.AreNotEqual(result.GetAsset().MimeType, FileNoMimeType);
                Assert.AreEqual(result.GetAsset().MimeType, "application/octet-stream");
            }

            source.SetMimeType(FileMimeType);

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");
                
                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                asset.Uri = result.GetUri();
            }

            // Get asset to check correct mime type
            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;

                Assert.AreEqual(result.GetAsset().MimeType, FileMimeType);
            }

            byte[] downloadedData;

            // Get data
            {
                using var result = assetSystem.DownloadAssetData(asset).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                downloadedData = new byte[result.GetDataLength()];
                Marshal.Copy(result.GetData(), downloadedData, 0, (int)result.GetDataLength());

                var fileData = File.ReadAllBytes(Path.GetFullPath("assets/test.json"));

                Assert.IsTrue(downloadedData.SequenceEqual(fileData));
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPLOADASSET_AS_BUFFER_TEST
        [Test]
        public static void UploadAssetAsBufferTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            asset.FileName = "test.json";

            var uploadFilePath = Path.GetFullPath("assets/test.json");
            var uploadFileSize = new FileInfo(uploadFilePath).Length;

            var uploadFileData = File.ReadAllBytes(uploadFilePath);

            IntPtr uploadFileDataPtr = Marshal.AllocHGlobal((int)uploadFileSize);
            Marshal.Copy(uploadFileData, 0, uploadFileDataPtr, (int)uploadFileSize);

            var source = new Systems.BufferAssetDataSource
            {
                Buffer = uploadFileDataPtr,
                BufferLength = (ulong)uploadFileSize
            };

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");
                
                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                asset.Uri = result.GetUri();
            }

            Marshal.FreeHGlobal(uploadFileDataPtr);

            byte[] downloadedData;

            // Get data
            {
                using var result = assetSystem.DownloadAssetData(asset).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                downloadedData = new byte[result.GetDataLength()];
                Marshal.Copy(result.GetData(), downloadedData, 0, (int)result.GetDataLength());

                Assert.IsTrue(downloadedData.SequenceEqual(uploadFileData));
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPDATEASSETDATA_TEST
        [Test]
        public static void UpdateAssetDataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/test.json")
            };

            var initialFileUri = "";

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");

                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                initialFileUri = result.GetUri();
            }

            // Call Update Asset
            {
                asset.Name = "TestName";

                using var result = assetSystem.UpdateAsset(asset).Result;
                var resCode = result.GetResultCode();
                var resBody = result.GetResponseBody();
                LogDebug(resBody);

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

            }

            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                using var updatedAsset = result.GetAsset();

                Assert.AreEqual(asset.Id, updatedAsset.Id);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPDATEASSETDATA_AS_FILE_TEST
        [Test]
        public static void UpdateAssetAsFileTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/test.json")
            };

            var initialFileUri = "";

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");
                
                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                initialFileUri = result.GetUri();
            }

            // Replace the file with a new one
            {
                source = new Systems.FileAssetDataSource
                {
                    FilePath = Path.GetFullPath("assets/test2.json")
                };

                LogDebug("Uploading new asset data...");

                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                Assert.AreNotEqual(initialFileUri, result.GetUri());
            }

            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                using var updatedAsset = result.GetAsset();

                Assert.AreEqual(asset.Id, updatedAsset.Id);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPDATEASSETDATA_AS_BUFFER_TEST
        [Test]
        public static void UpdateAssetAsBufferTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/test.json")
            };

            var initialFileUri = "";

            // Upload data
            {
                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");
                
                using var result = assetSystem.UploadAssetData(assetCollection, asset, source).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                initialFileUri = result.GetUri();
            }

            // Replace the file with a new one taken from a buffer
            {
                asset.FileName = "test2.json";

                var updatedFilePath = Path.GetFullPath("assets/test2.json");
                var updatedFileSize = new FileInfo(updatedFilePath).Length;

                var updatedFileData = File.ReadAllBytes(updatedFilePath);

                IntPtr updatedFileDataPtr = Marshal.AllocHGlobal((int)updatedFileSize);
                Marshal.Copy(updatedFileData, 0, updatedFileDataPtr, (int)updatedFileSize);

                var bufferSource = new Systems.BufferAssetDataSource
                {
                    Buffer = updatedFileDataPtr,
                    BufferLength = (ulong)updatedFileSize
                };

                LogDebug("Uploading new asset data...");

                using var result = assetSystem.UploadAssetData(assetCollection, asset, bufferSource).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                Assert.AreNotEqual(initialFileUri, result.GetUri());

                Marshal.FreeHGlobal(updatedFileDataPtr);
            }

            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                using var updatedAsset = result.GetAsset();

                Assert.AreEqual(asset.Id, updatedAsset.Id);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_UPDATEASSETMETADATA_TEST
        [Test]
        public static void UpdateAssetCollectionMetadataTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            using var metadataIn = new Common.Map<string, string>();
            metadataIn[testSpaceName] = testSpaceName;

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testSpaceName, null, null, out var assetCollection);

            UpdateAssetCollectionMetadata(assetSystem, assetCollection, metadataIn, out var metaDataOut);

            Assert.AreEqual(metadataIn[testSpaceName], metaDataOut[testSpaceName]);

            metaDataOut.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_GETASSETDATASIZE_TEST
        [Test]
        public static void GetAssetDataSizeTest()
        {
            GetFoundationSystems(out var userSystem, out _, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET");

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create asset collection
            CreateAssetCollection(assetSystem, null, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out var asset);

            // Upload data
            asset.FileName = "asimplejsonfile.json";
            var assetData = "{ \"some_value\": 42 }";
            var assetDataBytes = System.Text.Encoding.UTF8.GetBytes(assetData);
            var buf = Marshal.AllocHGlobal(assetDataBytes.Length);
            Marshal.Copy(assetDataBytes, 0, buf, assetDataBytes.Length);

            using var source = new Systems.BufferAssetDataSource
            {
                Buffer = buf,
                BufferLength = (ulong)assetDataBytes.Length
            };
            source.SetMimeType("application/json");

            LogDebug("Uploading asset data...");

            UploadAssetData(assetSystem, assetCollection, asset, source, out _);

            // Get updated asset
            Systems.Asset updatedAsset;

            {
                using var result = assetSystem.GetAssetById(assetCollection.Id, asset.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);

                updatedAsset = result.GetAsset();

                Assert.AreEqual(asset.Id, updatedAsset.Id);
            }

            // Get asset data size
            {
                using var result = assetSystem.GetAssetDataSize(updatedAsset).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);
                Assert.AreEqual((int)result.GetValue(), assetDataBytes.Length);
            }

            Marshal.FreeHGlobal(buf);
            updatedAsset.Dispose();
        }
#endif
#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_THIRDPARTYPACKAGEDASSETIDENTIFIER_TEST
        [Test]
        public static void thirdPartyPackagedAssetIdentifierTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");
            string thirdPartyPackagedAssetIdentifierAlphanumeric = "^[a-zA-Z0-9][a-zA-Z0-9/\\s]/*^\\s]*";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var assetCollection);

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, null, null, out _);

            // Get assets
            GetAssetsInCollection(assetSystem, assetCollection, out var assets);
            Assert.AreEqual(assets.Size(), 1UL);
            Assert.AreEqual(assets[0].Name, testAssetName);
            Assert.AreEqual(assets[0].GetThirdPartyPackagedAssetIdentifier(), "");
            Assert.AreEqual(assets[0].GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.NONE);
            
            var inboundAsset = assets[0];
            inboundAsset.SetThirdPartyPackagedAssetIdentifier("Test");
            Assert.AreEqual(inboundAsset.GetThirdPartyPackagedAssetIdentifier(), "Test");
            inboundAsset.SetThirdPartyPlatformType(Systems.EThirdPartyPlatform.UNREAL);
            Assert.AreEqual(inboundAsset.GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNREAL);
            inboundAsset.Dispose();

            // Create asset
            CreateAsset(assetSystem, assetCollection, testAssetName, thirdPartyPackagedAssetIdentifierAlphanumeric, Systems.EThirdPartyPlatform.UNITY, out _);

            // Get assets
            GetAssetsInCollection(assetSystem, assetCollection, out assets);
            
            Assert.AreEqual(assets.Size(), 2UL);
            Assert.AreEqual(assets[0].Name, testAssetName);
            Assert.AreEqual(assets[0].GetThirdPartyPackagedAssetIdentifier(), thirdPartyPackagedAssetIdentifierAlphanumeric);
            Assert.AreEqual(assets[0].GetThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNITY);

        }
#endif
#if RUN_ALL_UNIT_TESTS || RUN_ASSETSYSTEM_TESTS || RUN_ASSETSYSTEM_COPY_ASSET_COLLECTION_TEST
        [Test]
        public static void copyAssetCollectionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string sourceSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string destSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var sourceAssetCollection = new Systems.AssetCollection();

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create 'source' space and asset collection
            var sourceSpace = SpaceSystemTests.CreateSpace(spaceSystem, sourceSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);
            {
                CreateAssetCollection(assetSystem, sourceSpace, null, testAssetCollectionName, null, null, out sourceAssetCollection);
                CreateAsset(assetSystem, sourceAssetCollection, testAssetName, null, null, out var sourceAsset);

                var fileSource = new Systems.FileAssetDataSource
                {
                    FilePath = Path.GetFullPath("assets/test.json")
                };
                fileSource.SetMimeType("application/json");

                assetSystem.UploadAssetDataOnProgress += (s, e) => LogDebug($"Uploading asset... ({e.Progress}%)");
                using var result = assetSystem.UploadAssetData(sourceAssetCollection, sourceAsset, fileSource).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Systems.EResultCode.Success);
            }

            // Create 'dest' space and invoke the copy
            var destSpace = SpaceSystemTests.CreateSpace(spaceSystem, destSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);
            var destAssetCollections = new Common.Array<Systems.AssetCollection>();
            {
                Common.Array<Systems.AssetCollection> sourceAssetCollections = new Common.Array<Systems.AssetCollection>(1);
                sourceAssetCollections[0] = sourceAssetCollection;

                using var result = assetSystem.CopyAssetCollectionsToSpace(sourceAssetCollections, destSpace.Id, false).Result;
                
                Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Success);
                destAssetCollections = result.GetAssetCollections();
            }

            // Validate the copied asset collection and its data
	        {
                Assert.AreEqual(destAssetCollections.Size(), 1UL);
		        Assert.AreNotEqual(destAssetCollections[0].Id, sourceAssetCollection.Id);
		        Assert.AreEqual(destAssetCollections[0].SpaceId, destSpace.Id);
		        Assert.AreEqual(destAssetCollections[0].Type, sourceAssetCollection.Type);
		        Assert.AreEqual(destAssetCollections[0].Tags.Size(), 1UL);

                GetAssetsInCollection(assetSystem, destAssetCollections[0], out var destAssets);
                Assert.AreEqual(destAssets.Size(), 1UL);

                // Get the copied data and compare it with our source
                using var result = assetSystem.DownloadAssetData(destAssets[0]).Result;
                Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Success);

                byte[] downloadedData = new byte[result.GetDataLength()];
                Marshal.Copy(result.GetData(), downloadedData, 0, (int)result.GetDataLength());

                var fileData = File.ReadAllBytes(Path.GetFullPath("assets/test.json"));
                Assert.IsTrue(downloadedData.SequenceEqual(fileData));
            }

            // Validating that we must have at least one asset collection to copy
	        {
		        Common.Array<Systems.AssetCollection> sourceAssetCollections = new Common.Array<Systems.AssetCollection>(1);
                using var result = assetSystem.CopyAssetCollectionsToSpace(sourceAssetCollections, destSpace.Id, false).Result;
                Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Failed);
	        }

            // Validating we cannot perform a copy if the asset has no space ID
            {
                Common.Array<Systems.AssetCollection> assetCollections = new Common.Array<Systems.AssetCollection>(1);
                assetCollections[0] = new Systems.AssetCollection();

                using var result = assetSystem.CopyAssetCollectionsToSpace(assetCollections, destSpace.Id, false).Result;
                Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Failed);
            }

            // Validating we cannot perform a copy of assets that belong to different spaces
            {
                var firstSpaceAssetCollection = new Systems.AssetCollection();
                firstSpaceAssetCollection.SpaceId = "123456";

                var secondSpaceAssetCollection = new Systems.AssetCollection();
                secondSpaceAssetCollection.SpaceId = "456789";

                Common.Array<Systems.AssetCollection> assetCollections = new Common.Array<Systems.AssetCollection>(2);
                assetCollections[0] = firstSpaceAssetCollection;
                assetCollections[1] = secondSpaceAssetCollection;

                using var result = assetSystem.CopyAssetCollectionsToSpace(assetCollections, destSpace.Id, false).Result;
                Assert.AreEqual(result.GetResultCode(), Systems.EResultCode.Failed);
            }
        }
#endif
    }
}
