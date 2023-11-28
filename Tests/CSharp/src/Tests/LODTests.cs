using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

using Common = Csp.Common;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;

namespace CSPEngine
{
    static class LODTests
    {
        static void GetLODChain(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, out Systems.LODChain LODChain)
        {
            using var result = assetSystem.GetLODChain(assetCollection).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);

            LODChain = result.GetLODChain();
        }

        static void RegisterAssetToLODChain(Systems.AssetSystem assetSystem, Systems.AssetCollection assetCollection, Systems.Asset asset, int LODLevel)
        {
            using var result = assetSystem.RegisterAssetToLODChain(assetCollection, asset, LODLevel).Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Systems.EResultCode.Success);
        }

#if RUN_ALL_UNIT_TESTS || RUN_LOD_TESTS || RUN_LOD_GET_EMPTY_LODCHAIN_TEST
         [Test]
        public static void GetEmptyLODChainTest()
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
            Systems.AssetCollection assetCollection;
            AssetSystemTests.CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out assetCollection);

            // Get LOD chain
            Systems.LODChain chain;
            GetLODChain(assetSystem, assetCollection, out chain);

            Assert.AreEqual(chain.LODAssets.Size(), 0u);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOD_TESTS || RUN_LOD_REGISTER_ASSETS_TO_LODCHAIN_TEST
        public static void RegisterAssetsToLODChainTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName1 = GenerateUniqueString("OLY-UNITTEST-ASSET1-REWIND");
            string testAssetName2 = GenerateUniqueString("OLY-UNITTEST-ASSET2-REWIND");

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Create asset collection
            Systems.AssetCollection assetCollection;
            AssetSystemTests.CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out assetCollection);

            // Create assets
            Systems.Asset asset1;
            AssetSystemTests.CreateAsset(assetSystem, assetCollection, testAssetName1, null, null, out asset1);

            Systems.Asset asset2;
            AssetSystemTests.CreateAsset(assetSystem, assetCollection, testAssetName2, null, null, out asset2);

            // Register to LOD chain
            RegisterAssetToLODChain(assetSystem, assetCollection, asset1, 0);
            RegisterAssetToLODChain(assetSystem, assetCollection, asset2, 1);

            // Get LOD chain
            Systems.LODChain chain;
            GetLODChain(assetSystem, assetCollection, out chain);

            Assert.AreEqual(chain.LODAssets.Size(), 2u);

            Assert.AreEqual(chain.LODAssets[0].Level, 0);
            Assert.AreEqual(chain.LODAssets[0].Asset.Id, asset1.Id);

            Assert.AreEqual(chain.LODAssets[1].Level, 1);
            Assert.AreEqual(chain.LODAssets[1].Asset.Id, asset2.Id);
        }
#endif
    }

}