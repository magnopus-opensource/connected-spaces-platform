import { test, assert } from '../test_framework.js';
import { generateUniqueString } from '../test_helpers.js';
import { logIn } from './usersystem_tests_helpers.js';
import { createSpace } from './spacesystem_tests_helpers.js';
import { createAsset, createAssetCollection, getAssetsByCriteria, getAssetCollectionsByCriteria, uploadAssetData } from './assetsystem_tests_helpers.js'
import { jsArrayToCommonArray } from '../conversion_helpers.js';

import { freeBuffer, stringToBuffer, Services, Systems } from '../connected_spaces_platform.js';

async function getLODChain(assetSystem, assetCollection) {
    const result = await assetSystem.getLODChain(assetCollection);
    assert.succeeded(result);

    return result.getLODChain();
}

async function registerAssetToLODChain(assetSystem, assetCollection, asset, LODLevel) {
    const result = await assetSystem.registerAssetToLODChain(assetCollection, asset, LODLevel);
    assert.succeeded(result);
}

test('LODTests', 'GetEmptyLODChainTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';

    const testAssetCollectionName = generateUniqueString('CSP-TESTS-WASM-ASSETCOLLECTION');

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Create asset collection
    const assetCollection = await createAssetCollection(assetSystem, space, null, testAssetCollectionName, Systems.EAssetCollectionType.DEFAULT, null);
    
    // Get LOD chain
    const LODChain = await getLODChain(assetSystem, assetCollection);

    assert.areEqual(LODChain.lODAssets.size(), 0);
});

test('LODTests', 'RegisterAssetsToLODChainTest', async function() {
        const systemsManager = Systems.SystemsManager.get();
        const userSystem = systemsManager.getUserSystem();
        const spaceSystem = systemsManager.getSpaceSystem();
        const assetSystem = systemsManager.getAssetSystem();
    
        const SpaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
        const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';
    
        const testAssetCollectionName = generateUniqueString('CSP-TESTS-WASM-ASSETCOLLECTION');
        const testAsset1Name = generateUniqueString('CSP-TESTS-WASM-ASSET1');
        const testAsset2Name = generateUniqueString('CSP-TESTS-WASM-ASSET2');
        
        // Log in
        await logIn(userSystem);
        
        // Create space
        const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);
    
        // Create asset collection
        const assetCollection = await createAssetCollection(assetSystem, space, null, testAssetCollectionName, Systems.EAssetCollectionType.DEFAULT, null);

        // Create assets
        var asset1 = await createAsset(assetSystem, assetCollection, testAsset1Name, null, null, null);
        var asset2 = await createAsset(assetSystem, assetCollection, testAsset2Name, null, null, null);

        // Register to LOD chain
        await registerAssetToLODChain(assetSystem, assetCollection, asset1, 0);
        await registerAssetToLODChain(assetSystem, assetCollection, asset2, 1);

        // Get LOD chain
        const LODChain = await getLODChain(assetSystem, assetCollection);

        assert.areEqual(LODChain.lODAssets.size(), 2);

        assert.areEqual(LODChain.lODAssets.get(0).level, 0);
        assert.areEqual(LODChain.lODAssets.get(0).asset.id, asset1.id);

        assert.areEqual(LODChain.lODAssets.get(1).level, 1);
        assert.areEqual(LODChain.lODAssets.get(1).asset.id, asset2.id);
});