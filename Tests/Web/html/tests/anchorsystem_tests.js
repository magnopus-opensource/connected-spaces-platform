import { test, assert } from '../test_framework.js';
import { generateUniqueString } from '../test_helpers.js';
import { freeBuffer, Systems, Common, Multiplayer } from '../connected_spaces_platform.js';
import { logIn } from './usersystem_tests_helpers.js';
import { jsArrayToCommonArray } from '../conversion_helpers.js';
import { createSpace } from './spacesystem_tests_helpers.js'
import { createAssetCollection } from './assetsystem_tests_helpers.js';

test('AnchorSystemTests', 'CreateAnchorTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const assetSystem = systemsManager.getAssetSystem();
    const anchorSystem = systemsManager.getAnchorSystem();

    const assetCollectionName = generateUniqueString('CSP-TESTS-WASM-SPACE');

    await logIn(userSystem);

    const assetCollection = await createAssetCollection(assetSystem, null, null, assetCollectionName)

    const anchorLocation = Systems.GeoLocation.create();
    const anchorPosition = Systems.OlyAnchorPosition.create();
    const anchorRotation = Systems.OlyRotation.create();

    const uniqueThirdPartyAnchorId = generateUniqueString('CSP-UNITTEST-ID');

    const anchorResult = await anchorSystem.createAnchor(
        Systems.AnchorProvider.GoogleCloudAnchors, 
        uniqueThirdPartyAnchorId, 
        assetCollection.id, 
        anchorLocation,
        anchorPosition, 
        anchorRotation, 
        null, 
        null);
    
    assert.succeeded(anchorResult);
    const anchor = anchorResult.getAnchor();
    anchorResult.delete();
    assert.areEqual(anchor.thirdPartyAnchorProvider, Systems.AnchorProvider.GoogleCloudAnchors);
    assert.areEqual(anchor.thirdPartyAnchorId, uniqueThirdPartyAnchorId);
    assert.areEqual(anchor.spaceId, "");
    assert.areEqual(anchor.assetCollectionId, assetCollection.id);

    const anchorIds = Common.Array.ofString_number(1);
    anchorIds.set(0, anchor.id);

    const deleteResult = await anchorSystem.deleteAnchors(anchorIds);
    assert.succeeded(deleteResult);
    deleteResult.delete();
    
    anchor.delete();

    anchorLocation.delete();
    anchorPosition.delete();
    anchorRotation.delete(); 
});

test('AnchorSystemTests', 'CreateAnchorInSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();
    const anchorSystem = systemsManager.getAnchorSystem();

    const spaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';
    const assetCollectionName = generateUniqueString('CSP-TESTS-WASM-SPACE');

    await logIn(userSystem);

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});
    await connection.connect();
    await connection.initialiseConnection();

    const transform = Multiplayer.SpaceTransform.create();
    const name = "TestObject";
    const createdObject = await entitySystem.createObject(name, transform);

    const assetCollection = await createAssetCollection(assetSystem, space, null, assetCollectionName)

    const anchorLocation = Systems.GeoLocation.create();
    const anchorPosition = Systems.OlyAnchorPosition.create();
    const anchorRotation = Systems.OlyRotation.create();

    const uniqueThirdPartyAnchorId = generateUniqueString('CSP-UNITTEST-ID');

    const anchorResult = await anchorSystem.createAnchorInSpace(
        Systems.AnchorProvider.GoogleCloudAnchors, 
        uniqueThirdPartyAnchorId, 
        space.id,
        createdObject.getId(),
        assetCollection.id, 
        anchorLocation,
        anchorPosition, 
        anchorRotation, 
        null, 
        null);
    
    assert.succeeded(anchorResult);
    const anchor = anchorResult.getAnchor();
    anchorResult.delete();
    assert.areEqual(anchor.thirdPartyAnchorProvider, Systems.AnchorProvider.GoogleCloudAnchors);
    assert.areEqual(anchor.thirdPartyAnchorId, uniqueThirdPartyAnchorId);
    assert.areEqual(anchor.spaceId, space.id);
    assert.areEqual(anchor.spaceEntityId, createdObject.getId());
    assert.areEqual(anchor.assetCollectionId, assetCollection.id);

    const anchorIds = Common.Array.ofString_number(1);
    anchorIds.set(0, anchor.id);

    const deleteResult = await anchorSystem.deleteAnchors(anchorIds);
    assert.succeeded(deleteResult);
    deleteResult.delete();
    
    anchor.delete();

    anchorLocation.delete();
    anchorPosition.delete();
    anchorRotation.delete(); 

    transform.delete();
    await entitySystem.destroyEntity(createdObject);
    createdObject.delete();
    connection.disconnect();
});

test('AnchorSystemTests', 'GetAnchorsInSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();
    const anchorSystem = systemsManager.getAnchorSystem();

    const spaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';
    const assetCollectionName1 = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const assetCollectionName2 = generateUniqueString('CSP-TESTS-WASM-SPACE');

    await logIn(userSystem);

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});
    await connection.connect();
    await connection.initialiseConnection();

    const transform = Multiplayer.SpaceTransform.create();
    const name1 = "TestObject1";
    const createdObject1 = await entitySystem.createObject(name1, transform);
    const name2 = "TestObject2";
    const createdObject2 = await entitySystem.createObject(name2, transform);

    const assetCollection1 = await createAssetCollection(assetSystem, space, null, assetCollectionName1)
    const assetCollection2 = await createAssetCollection(assetSystem, space, null, assetCollectionName2)

    const anchorLocation = Systems.GeoLocation.create();
    const anchorPosition = Systems.OlyAnchorPosition.create();
    const anchorRotation = Systems.OlyRotation.create();

    const uniqueThirdPartyAnchorId1 = generateUniqueString('CSP-UNITTEST-ID');
    const uniqueThirdPartyAnchorId2 = generateUniqueString('CSP-UNITTEST-ID');

    const anchorResult1 = await anchorSystem.createAnchorInSpace(
        Systems.AnchorProvider.GoogleCloudAnchors, 
        uniqueThirdPartyAnchorId1, 
        space.id,
        createdObject1.getId(),
        assetCollection1.id, 
        anchorLocation,
        anchorPosition, 
        anchorRotation, 
        null, 
        null);
    
    assert.succeeded(anchorResult1);
    const anchor1 = anchorResult1.getAnchor();
    anchorResult1.delete();
    assert.areEqual(anchor1.thirdPartyAnchorProvider, Systems.AnchorProvider.GoogleCloudAnchors);
    assert.areEqual(anchor1.thirdPartyAnchorId, uniqueThirdPartyAnchorId1);
    assert.areEqual(anchor1.spaceId, space.id);
    assert.areEqual(anchor1.spaceEntityId, createdObject1.getId());
    assert.areEqual(anchor1.assetCollectionId, assetCollection1.id);

    const anchorResult2 = await anchorSystem.createAnchorInSpace(
        Systems.AnchorProvider.GoogleCloudAnchors, 
        uniqueThirdPartyAnchorId2, 
        space.id,
        createdObject2.getId(),
        assetCollection2.id, 
        anchorLocation,
        anchorPosition, 
        anchorRotation, 
        null, 
        null);
    
    assert.succeeded(anchorResult2);
    const anchor2 = anchorResult2.getAnchor();
    anchorResult2.delete();
    assert.areEqual(anchor2.thirdPartyAnchorProvider, Systems.AnchorProvider.GoogleCloudAnchors);
    assert.areEqual(anchor2.thirdPartyAnchorId, uniqueThirdPartyAnchorId2);
    assert.areEqual(anchor2.spaceId, space.id);
    assert.areEqual(anchor2.spaceEntityId, createdObject2.getId());
    assert.areEqual(anchor2.assetCollectionId, assetCollection2.id);

    const getAnchorsResult = await anchorSystem.getAnchorsInSpace(space.id, null, null);
    assert.succeeded(getAnchorsResult);
    const anchorsInSpace = getAnchorsResult.getAnchors();
    getAnchorsResult.delete();

    assert.areEqual(anchorsInSpace.size(), 2);
    let anchorsFound = 0;
    for (let i = 0; i < anchorsInSpace.size(); i++) {
        assert.areEqual(anchorsInSpace.get(i).spaceId, space.id);
        if (anchorsInSpace.get(i).thirdPartyAnchorId == anchor1.thirdPartyAnchorId 
            || anchorsInSpace.get(i).thirdPartyAnchorId == anchor2.thirdPartyAnchorId) {
            anchorsFound++;
        }
    }    
    assert.areEqual(anchorsFound, 2);
    
    const anchorIds = Common.Array.ofString_number(2);
    anchorIds.set(0, anchor1.id);
    anchorIds.set(1, anchor2.id);

    const deleteResult = await anchorSystem.deleteAnchors(anchorIds);
    assert.succeeded(deleteResult);
    deleteResult.delete();
    
    anchor1.delete();
    anchor2.delete();

    anchorLocation.delete();
    anchorPosition.delete();
    anchorRotation.delete(); 

    transform.delete();
    await entitySystem.destroyEntity(createdObject1);
    createdObject1.delete();
    await entitySystem.destroyEntity(createdObject2);
    createdObject2.delete();
    connection.disconnect();
});

test('AnchorSystemTests', 'CreateAnchorResolutionTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();
    const anchorSystem = systemsManager.getAnchorSystem();

    const spaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';
    const assetCollectionName = generateUniqueString('CSP-TESTS-WASM-SPACE');

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});
    await connection.connect();
    await connection.initialiseConnection();
        
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    const assetCollection = await createAssetCollection(assetSystem, space, null, assetCollectionName)

    // Create anchor
    const uniqueThirdPartyAnchorId = generateUniqueString('CSP-UNITTEST-ID');
    var anchorLocation = Systems.GeoLocation.create();
    const anchorPosition = Systems.OlyAnchorPosition.create();
    const anchorRotation = Systems.OlyRotation.create();

    const anchorSpatialKeyValue = Common.Map.ofStringAndString();
    anchorSpatialKeyValue.set('TestKey1', 'TestValue1');
    anchorSpatialKeyValue.set('TestKey2', 'TestValue2');

    var testTag = "TestTag";
    var tags = [];
    tags[0] = testTag;

    const anchorResult = await anchorSystem.createAnchorInSpace(
        Systems.AnchorProvider.GoogleCloudAnchors, 
        uniqueThirdPartyAnchorId, 
        space.id,
        createdObject.getId(),
        assetCollection.id, 
        anchorLocation,
        anchorPosition, 
        anchorRotation, 
        anchorSpatialKeyValue, 
        jsArrayToCommonArray(tags, String));
    
    assert.areEqual(anchorResult.getAnchor().tags.size(), 1);

    assert.succeeded(anchorResult);

    const anchorId = anchorResult.getAnchor().id;

    anchorResult.delete();

    var successfullyResolved = true;
    var resolveAttempted = 3;
    var resolveTime = 1000;

    const anchorResolutionResult = await anchorSystem.createAnchorResolution(anchorId, successfullyResolved, resolveAttempted, resolveTime, jsArrayToCommonArray(tags, String));
    const anchorResolution = anchorResolutionResult.getAnchorResolution();

    anchorResolutionResult.delete();

    assert.areEqual(anchorResolution.successfullyResolved, successfullyResolved);
    assert.areEqual(anchorResolution.resolveAttempted, resolveAttempted);
    assert.areEqual(anchorResolution.resolveTime, resolveTime);
    assert.areEqual(anchorResolution.tags.size(), 1);
    assert.areEqual(anchorResolution.tags.get(0), testTag);

    anchorResolution.delete();
    anchorLocation.delete();
    anchorPosition.delete();
    anchorRotation.delete();
    anchorSpatialKeyValue.delete();

    transform.delete();
    await entitySystem.destroyEntity(createdObject);
    createdObject.delete();
    connection.disconnect();
});