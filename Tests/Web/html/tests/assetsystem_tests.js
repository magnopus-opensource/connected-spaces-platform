import { test, assert } from '../test_framework.js';
import { generateUniqueString } from '../test_helpers.js';
import { logIn } from './usersystem_tests_helpers.js';
import { createSpace } from './spacesystem_tests_helpers.js';
import { createAsset, createAssetCollection, getAssetsByCriteria, getAssetCollectionsByCriteria, uploadAssetData } from './assetsystem_tests_helpers.js'
import { jsArrayToCommonArray } from '../conversion_helpers.js';

import { freeBuffer, stringToBuffer, Services, Systems } from '../olympus_foundation.js';


test('AssetSystemTests', 'GetAssetCollectionsByDifferentCriteriaTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const testAssetCollectionName1 = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetCollectionName2 = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetCollectionName3 = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');

    await logIn(userSystem);

    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const tags = [space.id];

    // Create asset collections
    const assetCollection1 = await createAssetCollection(assetSystem, space, null, testAssetCollectionName1, Systems.EAssetCollectionType.SPACE_THUMBNAIL, null)
    const assetCollection2 = await createAssetCollection(assetSystem, space, null, testAssetCollectionName2, Systems.EAssetCollectionType.SPACE_THUMBNAIL, tags)
    const assetCollection3 = await createAssetCollection(assetSystem, space, assetCollection1.id, testAssetCollectionName3, null, null)

    // Search by space
    {
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, space, null, null, null, null, null, null);
        assert.areEqual(assetCollections.length, 4);
    }

    // Search by parentId
    {
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, null, assetCollection1.id, null, null, null, null, null);
        assert.areEqual(assetCollections.length, 1);
        assert.areEqual(assetCollections[0].name, assetCollection3.name);
        assert.areEqual(assetCollections[0].id, assetCollection3.id);
    }

    // Search by names
    {
        const names = [testAssetCollectionName1, testAssetCollectionName2];
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, null, null, null, null, names, null, null);
        assert.areEqual(assetCollections.length, 2);
    
        let foundAssetCollection1 = false;
        let foundAssetCollection2 = false;

        for (let assetCollection of assetCollections) {
            if (assetCollection.name === assetCollection1.name)
                foundAssetCollection1 = true;
            else if (assetCollection.name === assetCollection2.name)
                foundAssetCollection2 = true;
        }
        
        assert.isTrue(foundAssetCollection1 && foundAssetCollection2);
    }
    
    // Test pagination
    {
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, space, null, null, null, null, 1, 1);
        assert.areEqual(assetCollections.length, 1);
    }

    // Search by tags
    {
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, null, null, null, tags, null, null, null);

        assert.areEqual(assetCollections.length, 1);

        const assetCollection = assetCollections[0];

        assert.areEqual(assetCollection.name, assetCollection2.name);
        assert.areEqual(assetCollection.id, assetCollection2.id);
    }
    
    // Search by names and types
    const names = [testAssetCollectionName1, testAssetCollectionName2];

    // Search default types with these names
    {
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, null, null, Systems.EAssetCollectionType.DEFAULT, null, names, null, null);

        assert.areEqual(assetCollections.length, 0);
    }

    // Search for space thumbnails with these names
    {
        const assetCollections = await getAssetCollectionsByCriteria(assetSystem, null, null, Systems.EAssetCollectionType.SPACE_THUMBNAIL, null, names, null, null);
        assert.areEqual(assetCollections.length, 2);

        let foundAssetCollection1 = false;
        let foundAssetCollection2 = false;

        for (let assetCollection of assetCollections) {
            if (assetCollection.name === assetCollection1.name)
                foundAssetCollection1 = true;
            else if (assetCollection.name === assetCollection2.name)
                foundAssetCollection2 = true;
            
            if (foundAssetCollection1 && foundAssetCollection2)
                break;
        }
        assert.isTrue(foundAssetCollection1 && foundAssetCollection2);
    }    
});


test('AssetSystemTests', 'GetAssetsFromMultipleAssetCollectionsTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const testAssetCollectionName1 = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetCollectionName2 = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');

    const testAssetName1 = generateUniqueString('OLY-TESTS-WASM-ASSET');
    const testAssetName2 = generateUniqueString('OLY-TESTS-WASM-ASSET');

    await logIn(userSystem);

    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Create asset collections
    const assetCollection1 = await createAssetCollection(assetSystem, space, null, testAssetCollectionName1, Systems.EAssetCollectionType.DEFAULT, null)
    const assetCollection2 = await createAssetCollection(assetSystem, space, null, testAssetCollectionName2, Systems.EAssetCollectionType.DEFAULT, null)

    const asset1 = await createAsset(assetSystem, assetCollection1, testAssetName1, null);
    const asset2 = await createAsset(assetSystem, assetCollection2, testAssetName2, null);

    // Try to search but don't specify any asset collection Ids, only add one Asset Id though
    {
        const assetCollectionIds = [];
        const result = await assetSystem.getAssetsByCriteria(jsArrayToCommonArray(assetCollectionIds, String), null, null, null);
        assert.succeeded(result, Services.EResultCode.Failed);
    }

    // Search by both asset collection Ids at the same time
    {
        const assetCollectionIds = [assetCollection1.id, assetCollection2.id];
        const assets = await getAssetsByCriteria(assetSystem, assetCollectionIds);
        assert.areEqual(assets.length, 2);
        
        let foundAsset1 = false;
        let foundAsset2 = false;

        for (let asset of assets) {
            if (asset.id === asset1.id)
                foundAsset1 = true;
            else if (asset.id === asset2.id)
                foundAsset2 = true;
        }

        assert.isTrue(foundAsset1 && foundAsset2);
    }

    // Search by both asset collection Ids and only one Asset Id
    {
        const assetCollectionIds = [assetCollection1.id, assetCollection2.id];
        const assetIds = [asset2.id];
        const assets = await getAssetsByCriteria(assetSystem, assetCollectionIds, assetIds);
        
        assert.areEqual(assets.length, 1);
        assert.areEqual(assets[0].name, asset2.name);
        assert.areEqual(assets[0].id, asset2.id);
    }
});


test('AssetSystemTests', 'GetAssetDataSizeTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const assetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const assetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    await logIn(userSystem);

    // Create asset collection
    const assetCollection = await createAssetCollection(assetSystem, null, null, assetCollectionName);

    // Create asset
    const asset = await createAsset(assetSystem, assetCollection, assetName);

    // Upload data
    asset.fileName = 'asimplejsonfile.json';
    const assetData = '{ "some_value": 42 }';
    const [dataBuf, dataLen] = stringToBuffer(assetData);
    let source = Systems.BufferAssetDataSource.create();
    source.buffer = dataBuf;
    source.bufferLength = dataLen;
    source.setMimeType('application/json');

    console.log('Uploading asset data...');

    await uploadAssetData(assetSystem, assetCollection, asset, source);
    source.delete();

    // Get updated asset
    let updatedAsset;

    {
        const result = await assetSystem.getAssetById(assetCollection.id, asset.id);

        assert.succeeded(result);

        updatedAsset = result.getAsset();
        result.delete();

        assert.areEqual(asset.id, updatedAsset.id);
    }

    {
        console.log("Downloading asset data...");
        const result = await assetSystem.downloadAssetData(updatedAsset);

        assert.succeeded(result);
        
        console.log("Done!");
    }

    // Get asset data size
    {
        console.log(updatedAsset.uri);
        const result = await assetSystem.getAssetDataSize(updatedAsset);

        assert.succeeded(result);

        updatedAsset.delete();

        assert.areEqual(Number(result.getValue()), dataLen);

        result.delete();
    }

    freeBuffer(dataBuf);
});

test('AssetSystemTests', 'ThirdPartyPackagedAssetIdentifierTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const assetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const assetName = generateUniqueString('OLY-TESTS-WASM-ASSET');
    const thirdPartyPackagedAssetIdentifier = "OKO interoperable assets Test";

    await logIn(userSystem);

    // Create asset collection
    const assetCollection = await createAssetCollection(assetSystem, null, null, assetCollectionName);

    // Create asset
    var asset = await createAsset(assetSystem, assetCollection, assetName);

    assert.areEqual(asset.getThirdPartyPackagedAssetIdentifier(), "");
    assert.areEqual(asset.getThirdPartyPlatformType(), Systems.EThirdPartyPlatform.NONE);

    asset.setThirdPartyPackagedAssetIdentifier("Test");
    assert.areEqual(asset.getThirdPartyPackagedAssetIdentifier(), "Test");

    asset.setThirdPartyPlatformType(Systems.EThirdPartyPlatform.UNREAL)
    asset.setThirdPartyPackagedAssetIdentifier("")
    const Assetupdated = await assetSystem.updateAsset(asset);
    assert.areEqual(Assetupdated.getAsset().getThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNREAL);
    assert.areEqual(Assetupdated.getAsset().getThirdPartyPackagedAssetIdentifier(), "");

    // Create asset
    const asset2 = await createAsset(assetSystem, assetCollection, assetName,thirdPartyPackagedAssetIdentifier, Systems.EThirdPartyPlatform.UNITY);

    assert.areEqual(asset2.getThirdPartyPackagedAssetIdentifier(), thirdPartyPackagedAssetIdentifier);
    assert.areEqual(asset2.getThirdPartyPlatformType(), Systems.EThirdPartyPlatform.UNITY);

});
