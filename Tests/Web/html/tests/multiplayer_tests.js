import { test, assert } from '../test_framework.js';
import { sleep, generateUniqueString, CHS_ENDPOINT_BASE_URI } from '../test_helpers.js';
import { DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, getProfileByUserId, logIn, logInAsGuest, logOut } from './usersystem_tests_helpers.js'
import { createSpace, deleteSpace, getSpace, getSpacesByIds, updateSpace } from './spacesystem_tests_helpers.js'
import { createAsset, createAssetCollection, createBufferAssetDataSource, uploadAssetData } from './assetsystem_tests_helpers.js';

import { freeBuffer, OlympusFoundation, Multiplayer, Services, Systems, Common } from '../olympus_foundation.js';
import { commonArrayToJSArray } from '../conversion_helpers.js';


/*
Test requires user interaction. So commented out

test('MultiplayerTests', 'ConnectionInterruptTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);

    let interrupted = false;
    let disconnected = false;

    connection.setNetworkInterruptionCallback( (message) => { interrupted = true; console.log("interruption: " + message); });
    connection.setDisconnectionCallback( (message) => { disconnected = true; console.log("disconnection: " + message); });

    let ok = await connection.connect();

    assert.areEqual(ok, 1);

    ok = await connection.initialiseConnection();

    assert.areEqual(ok, 1);

    const entitySystem = connection.getSpaceEntitySystem();

    const userName = "Player 1";
    const transform = Multiplayer.SpaceTransform.create();
    const avatarState = Multiplayer.AvatarState.Idle;
    const avatarId = "MyCoolAvatar";
    const avatarPlayMode = Multiplayer.AvatarPlayMode.Default;

    entitySystem.setEntityCreatedCallback((entity) => { });

    const avatar = await entitySystem.createAvatar(userName, transform, avatarState, avatarId, avatarPlayMode);

    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    console.log("Interrupt connection here");

    while(!interrupted && testTime < 60){
        await sleep(50);
        
        avatar.setPosition(Common.Vector3.create_x_y_z(Math.random() * 100, Math.random() * 100, Math.random() * 100));

        entitySystem.sendEntityUpdate(avatar);

        current = Date.now();
        testTime = (current - start) / 1000;

        OlympusFoundation.tick();
    }

    assert.isTrue(interrupted);

           await spaceSystem.exitSpaceAndDisconnect(connection ,(Ok)=>{
        assert.isTrue(Ok);
    });


    assert.isTrue(disconnected);
});
*/

test('MultiplayerTests', 'ManualConnectionTest', async function() {

    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});
    
    // Connect
    {
        const ok = await connection.connect();
    
        assert.isTrue(ok);
    }

    // Initialise connection
    {
        const ok = await connection.initialiseConnection();
    
        assert.isTrue(ok);
    }
    
    // Create object to represent the portal
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Cleanup
    await connection.disconnect();
    connection.delete();
});

test('MultiplayerTests', 'RunScriptTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    
    assert.succeeded(enterResult);

    const connection  = enterResult.getConnection();
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});

    const scriptText = `
        var entities = TheEntitySystem.getEntities();
        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

        globalThis.onTick = () =>
        {
            var model = entities[entityIndex].getAnimatedModelComponents()[0];
            model.position = [10, 10, 10];
        }

        ThisEntity.subscribeToMessage("entityTick", "onTick");
    `;

    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const entity = await entitySystem.createObject(name, transform);

    assert.isTrue(entity.pointerIsValid());

    transform.delete();

    let component = entity.addComponent(Multiplayer.ComponentType.AnimatedModel);
    const animatedModelComponent = Multiplayer.StaticModelSpaceComponent.fromComponentBase(component);

    component = entity.addComponent(Multiplayer.ComponentType.ScriptData);
    const scriptComponent = Multiplayer.ScriptSpaceComponent.fromComponentBase(component);

    entity.queueUpdate();
    entitySystem.processPendingEntityOperations();

    scriptComponent.setScriptSource(scriptText);
    entity.getScript().invoke();

    OlympusFoundation.tick();

    const err = entity.getScript().hasError();

    assert.isFalse(err);

    const pos = animatedModelComponent.getPosition();

    assert.areEqual(pos.x, 10.0);
    assert.areEqual(pos.y, 10.0);
    assert.areEqual(pos.z, 10.0);

    pos.delete();

    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    
    assert.isTrue(ok);connection.delete();
});


test('MultiplayerTests', 'UsePortalTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const spaceName2 = generateUniqueString('OLY-TESTS-WASM-SPACE-2');
    const spaceDescription2 = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);
    const space2 = await createSpace(spaceSystem, spaceName2, spaceDescription2, Systems.SpaceAttributes.Private, null, null, null, false);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Ensure we're in the first space
    const currentSpace = spaceSystem.getCurrentSpace();
    assert.areEqual(currentSpace.id, space.id);

    currentSpace.delete();

    // Create avatar
    const userName = "Player 1";
    const transform = Multiplayer.SpaceTransform.create();
    const avatarState = Multiplayer.AvatarState.Idle;
    const avatarId = "MyCoolAvatar";
    const avatarPlayMode = Multiplayer.AvatarPlayMode.Default;
    
    {
        let avatar = await entitySystem.createAvatar(userName, transform, avatarState, avatarId, avatarPlayMode);

        assert.isTrue(avatar.pointerIsValid());
    }

    // Create object to represent the portal
    const name = "TestObject";
    const objectTransform = Multiplayer.SpaceTransform.create();

    const entity = await entitySystem.createObject(name, objectTransform);

    // Create portal component
    const component = entity.addComponent(Multiplayer.ComponentType.Portal);
    const portalComponent = Multiplayer.PortalSpaceComponent.fromComponentBase(component);
    portalComponent.setSpaceId(space2.id);

    entitySystem.processPendingEntityOperations();

    /*
        User would now interact with the portal
    */

    const testId = space.id;
    const portalSpaceId = portalComponent.getSpaceId();

    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);

    const spaces = await getSpacesByIds(spaceSystem, [portalSpaceId]);

    assert.isGreaterThan(spaces.length, 0);

    const portalSpace = spaces[0];

    const enterResult2 = await spaceSystem.enterSpace(portalSpace.id, true);
    assert.succeeded(enterResult2);
    connection  = enterResult2.getConnection();
    entitySystem = connection.getSpaceEntitySystem();
    entitySystem.setEntityCreatedCallback((e) => {});
    enterResult2.delete();

    // Ensure we're in the second space
    const updatedCurrentSpace = spaceSystem.getCurrentSpace();
    assert.areEqual(updatedCurrentSpace.id, portalSpace.id);

    {
        const avatar = await entitySystem.createAvatar(userName, transform, avatarState, avatarId, avatarPlayMode);

        assert.isTrue(avatar.pointerIsValid());
    }

    entitySystem.processPendingEntityOperations();

    // Cleanup
    
var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();

    await deleteSpace(spaceSystem, space);
    await deleteSpace(spaceSystem, space2);

    await logOut(userSystem);
});


test('MultiplayerTests', 'ImageScriptInterfaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    const enterResult = await spaceSystem.enterSpace(space.id, true);

    assert.succeeded(enterResult);

    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});
    // Create object to represent the image
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create image component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Image);
    const imageComponent = Multiplayer.ImageSpaceComponent.fromComponentBase(component);
    
    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    assert.areEqual(imageComponent.getIsVisible(), true);
    assert.areEqual(imageComponent.getIsEmissive(), false);
    assert.areEqual(imageComponent.getBillboardMode(), Multiplayer.BillboardMode.Off);
    assert.areEqual(imageComponent.getDisplayMode(), Multiplayer.DisplayMode.DoubleSided);

    // Setup script
    const scriptText = `
        var image = ThisEntity.getImageComponents()[0];
        image.position = [2, 1, 3];
        image.isVisible = false;
        image.isEmissive = true;
        image.displayMode = 2;
        image.billboardMode = 1;
    `;

    const script = createdObject.getScript();
    script.setScriptSource(scriptText);
    script.invoke();

    entitySystem.processPendingEntityOperations();

    assert.areEqual(imageComponent.getIsVisible(), false);
    assert.areEqual(imageComponent.getIsEmissive(), true);
    assert.areEqual(imageComponent.getBillboardMode(), Multiplayer.BillboardMode.Billboard);
    assert.areEqual(imageComponent.getDisplayMode(), Multiplayer.DisplayMode.DoubleSidedReversed);

    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();

    await deleteSpace(spaceSystem, space);

    await logOut(userSystem);
});


test('MultiplayerTests', 'PortalScriptInterfaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});
    // Create object to represent the portal
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create portal component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Portal);
    const portalComponent = Multiplayer.PortalSpaceComponent.fromComponentBase(component);

    const initialPosition = Common.Vector3.create_x_y_z(1.0, 2.0, 3.0);

    portalComponent.setSpaceId("initialTestSpaceId");
    portalComponent.setIsEnabled(false);
    portalComponent.setPosition(initialPosition);
    portalComponent.setRadius(123.0);

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    assert.areEqual(portalComponent.getSpaceId(), "initialTestSpaceId");
    assert.areEqual(portalComponent.getIsEnabled(), false);
    assert.areApproximatelyEqual(portalComponent.getPosition().x, initialPosition.x);
    assert.areApproximatelyEqual(portalComponent.getPosition().y, initialPosition.y);
    assert.areApproximatelyEqual(portalComponent.getPosition().z, initialPosition.z);
    assert.areApproximatelyEqual(portalComponent.getRadius(), 123.0);

    initialPosition.delete();

    // Setup script
    const scriptText = `
        var portal = ThisEntity.getPortalComponents()[0];
        portal.spaceId = "secondTestSpaceId";
        portal.isEnabled = true;
        portal.position = [4.0, 5.0, 6.0];
        portal.radius = 456.0;
`;

    const script = createdObject.getScript();
    script.setScriptSource(scriptText);
    script.invoke();

    entitySystem.processPendingEntityOperations();

    assert.areEqual(portalComponent.getSpaceId(), "secondTestSpaceId");
    assert.areEqual(portalComponent.getIsEnabled(), true);
    assert.areApproximatelyEqual(portalComponent.getPosition().x, 4.0);
    assert.areApproximatelyEqual(portalComponent.getPosition().y, 5.0);
    assert.areApproximatelyEqual(portalComponent.getPosition().z, 6.0);
    assert.areApproximatelyEqual(portalComponent.getRadius(), 456.0);

    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();

    await deleteSpace(spaceSystem, space);

    await logOut(userSystem);
});


test('MultiplayerTests', 'PortalThumbnailTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    // Update Thumbnail
    {
        var thumbnail = await createBufferAssetDataSource("/assets/dog.png", "image/png");

        const result = await spaceSystem.updateSpaceThumbnailWithBuffer(space.id, thumbnail);

        assert.succeeded(result);

        freeBuffer(thumbnail.buffer);
        thumbnail.delete();
        result.delete();
    }

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});
    // Create object to represent the portal
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create portal component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Portal);
    const portalComponent = Multiplayer.PortalSpaceComponent.fromComponentBase(component);

    portalComponent.setSpaceId(space.id);
    const result = await portalComponent.getSpaceThumbnail();

    assert.succeeded(result);
    assert.areNotEqual(result.getUri(), "");

    result.delete();

    // Cleanup
var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();

    await deleteSpace(spaceSystem, space);
 
    await logOut(userSystem);
}
);


test('MultiplayerTests', 'DeleteMultipleEntitiesTest', async function() {
    // Test for OB-1046
    // If the rate limiter hasn't processed all PendingOutgoingUpdates after SpaceEntity deletion it will crash when trying to process them

    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    // Connect
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create 3 seperate objects to ensure there is too many updates for the rate limiter to process in one tick
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);
    createdObject.addComponent(Multiplayer.ComponentType.Image);

    const createdObject2 = await entitySystem.createObject(name, transform);
    createdObject2.addComponent(Multiplayer.ComponentType.Image);

    const createdObject3 = await entitySystem.createObject(name, transform);
    createdObject2.addComponent(Multiplayer.ComponentType.Image);

    transform.delete();

    // Destroy Entites
    entitySystem.destroyEntity(createdObject);
    entitySystem.destroyEntity(createdObject2);
    entitySystem.destroyEntity(createdObject3);
    
    OlympusFoundation.tick();

    // Cleanup
var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();

    await deleteSpace(spaceSystem, space);
 
    await logOut(userSystem);
}
);


test('MultiplayerTests', 'AssetProcessedCallbackTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';
    const testAssetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup Asset callback
    let assetDetailBlobChangedCallbackCalled = false;
    let callbackAssetId = "";

    connection.setAssetDetailBlobChangedCallback((p) => {
        if (assetDetailBlobChangedCallbackCalled)
            return;

        assert.areEqual(p.changeType, Multiplayer.EAssetChangeType.Created);
        assert.areEqual(p.assetType, Systems.EAssetType.MODEL);

        callbackAssetId = p.assetId;
        assetDetailBlobChangedCallbackCalled = true;
    });

    // Create asset collection
    let collection = await createAssetCollection(assetSystem, space, null, testAssetCollectionName, Systems.EAssetCollectionType.SPACE_THUMBNAIL, null);

    const uploadFilename = "/assets/Test.json";

    // Create asset
    let asset = await createAsset(assetSystem, collection, testAssetName, null);
    asset.fileName = uploadFilename;

    // Upload data
    const source = await createBufferAssetDataSource(uploadFilename, "application/json");
    await uploadAssetData(assetSystem, collection, asset, source);

    source.delete();
   
    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!assetDetailBlobChangedCallbackCalled && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(assetDetailBlobChangedCallbackCalled);
    assert.areEqual(callbackAssetId, asset.id);

    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});


test('MultiplayerTests', 'DeleteScriptTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup script
    const scriptText = `
        var entities = TheEntitySystem.getEntities();
        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

        globalThis.onTick = () => {
            var entity = entities[entityIndex];
            entity.position = [10, 10, 10];
        }

        ThisEntity.subscribeToMessage("entityTick", "onTick");
    `;

    // Create object
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);
    
     // Create script
    const component = createdObject.addComponent(Multiplayer.ComponentType.ScriptData);
    const scriptComponent = Multiplayer.ScriptSpaceComponent.fromComponentBase(component);

    scriptComponent.setScriptSource(scriptText);
    createdObject.getScript().invoke();

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Ensure position is set to 0
    {
        var position = createdObject.getPosition();

        assert.areEqual(position.x, 0);
        assert.areEqual(position.y, 0);
        assert.areEqual(position.z, 0);

        position.delete();
    }

    // Delete script component
    createdObject.removeComponent(scriptComponent.getId());

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Tick to attempt to call scripts tick event
    OlympusFoundation.tick();

    // Ensure position is still set to 0
    {
        const position = createdObject.getPosition();

        assert.areEqual(position.x, 0);
        assert.areEqual(position.y, 0);
        assert.areEqual(position.z, 0);

        position.delete();
    }
    
    // Cleanup
var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'DeleteAndChangeComponentTest', async function() {
    // Test for: OB-864
    // Second script deletion test adds a second component to the object with the script

    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup script
    const scriptText = `
        var entities = TheEntitySystem.getEntities();
        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

        globalThis.onTick = () => {
            var entity = entities[entityIndex];
            entity.position = [10, 10, 10];
        }

        ThisEntity.subscribeToMessage("entityTick", "onTick");
    `;

    // Create object
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    // Create animated model component
    var component = createdObject.addComponent(Multiplayer.ComponentType.AnimatedModel);
    const animatedComponent = Multiplayer.AnimatedModelSpaceComponent.fromComponentBase(component);
    component.delete();
    
     // Create script
    component = createdObject.addComponent(Multiplayer.ComponentType.ScriptData);
    const scriptComponent = Multiplayer.ScriptSpaceComponent.fromComponentBase(component);
    component.delete();
    scriptComponent.setScriptSource(scriptText);
    createdObject.getScript().invoke();

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Make a component update
    animatedComponent.setPosition(Common.Vector3.create_x_y_z(1, 1, 1));

    // Delete script component
    createdObject.removeComponent(scriptComponent.getId());
    
    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();
    createdObject.delete();
    // Ensure entity update doesn't crash
    OlympusFoundation.tick();
    
    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'AddSecondScriptTest', async function() {
    // Test for OB-1407

    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup script
    const scriptText = `
        var entities = TheEntitySystem.getEntities();
        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

        globalThis.onTick = () => {
            var entity = entities[entityIndex];
            entity.position = [1, 1, 1];
        }

        ThisEntity.subscribeToMessage("entityTick", "onTick");
    `;

    // Create object
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);
    
     // Create script
    var component = createdObject.addComponent(Multiplayer.ComponentType.ScriptData);
    var scriptComponent = Multiplayer.ScriptSpaceComponent.fromComponentBase(component);

    scriptComponent.setScriptSource(scriptText);
    createdObject.getScript().invoke();

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Delete script component
    createdObject.removeComponent(scriptComponent.getId());

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    OlympusFoundation.tick();

    // Ensure position is set to 0
    {
        const position = createdObject.getPosition();

        assert.areEqual(position.x, 0);
        assert.areEqual(position.y, 0);
        assert.areEqual(position.z, 0);

        position.delete();
    }

    // Re-add script component
    component = createdObject.addComponent(Multiplayer.ComponentType.ScriptData);
    scriptComponent = Multiplayer.ScriptSpaceComponent.fromComponentBase(component);

    scriptComponent.setScriptSource(scriptText);
    createdObject.getScript().invoke();

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Ensure re-bound script works
    OlympusFoundation.tick();

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    {
        const position = createdObject.getPosition();

        assert.areEqual(position.x, 1);
        assert.areEqual(position.y, 1);
        assert.areEqual(position.z, 1);

        position.delete();
    }
    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
   assert.isTrue(ok);
   connection.delete();
});



test('MultiplayerTests', 'ConversationSpaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    const userId = await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);
    const userProfile = await getProfileByUserId(userSystem, userId);
    const userDisplayName = userProfile.displayName;
    userProfile.delete();
    
    
    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create Conversation component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Conversation);
    const ConversationComponent = Multiplayer.ConversationSpaceComponent.fromComponentBase(component);
    var conversationId;
    var messageId;
    var testMessage = "Test123";
    {
        const result = await ConversationComponent.createConversation("TestMessage");
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();

    }

    // Get Conversation Info
    {
        const result = await ConversationComponent.getConversationInfo()
        assert.succeeded(result);
        assert.areEqual(result.getConversationInfo().conversationId, conversationId);
        assert.areEqual(result.getConversationInfo().userID, userId);
        assert.areEqual(result.getConversationInfo().userDisplayName, userDisplayName);
        assert.areEqual(result.getConversationInfo().message,"TestMessage");
        assert.isFalse(result.getConversationInfo().edited);
        assert.isFalse(result.getConversationInfo().resolved);
                    
        var defaultValue = Multiplayer.SpaceTransform.create();
        assert.areEqual(result.getConversationInfo().cameraPosition.position.x,defaultValue.position.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.y,defaultValue.position.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.z,defaultValue.position.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.w,defaultValue.rotation.w);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.x,defaultValue.rotation.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.y,defaultValue.rotation.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.z,defaultValue.rotation.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.scale.x,defaultValue.scale.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.y,defaultValue.scale.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.z,defaultValue.scale.z);
        
        defaultValue.delete();
        result.delete();
    }

    {
        const result = await ConversationComponent.addMessage(testMessage);
        assert.succeeded(result);
        assert.areEqual(testMessage,result.getMessageInfo().message);
        messageId = result.getMessageInfo().id;
        result.delete();

    }
    {
        const result = await ConversationComponent.getMessage(messageId);
        assert.succeeded(result);
        assert.areEqual(testMessage,result.getMessageInfo().message);
        assert.areEqual(messageId,result.getMessageInfo().id);
        result.delete();

    }
    {
        const result = await ConversationComponent.getAllMessages();
        assert.succeeded(result);
        assert.areEqual(testMessage, result.getMessages().get(0).message);
        result.delete();
    }
    {
        const result = await ConversationComponent.deleteMessage(messageId);
        assert.succeeded(result);
        result.delete();

    }
    {
        const result = await ConversationComponent.deleteConversation();
        assert.succeeded(result);
        result.delete();

    }
    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);

    connection.delete();

    await deleteSpace(spaceSystem, space);

    await logOut(userSystem);
});

test('MultiplayerTests', 'AudioComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the audio component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create audio component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Audio);
    const audioComponent = Multiplayer.AudioSpaceComponent.fromComponentBase(component);

    // Ensure defaults are set
    {
        const pos = audioComponent.getPosition();

        assert.areEqual(pos.x, 0);
        assert.areEqual(pos.y, 0);
        assert.areEqual(pos.z, 0);
        assert.areEqual(audioComponent.getPlaybackState(), Multiplayer.AudioPlaybackState.Reset);
        assert.areEqual(audioComponent.getAudioType(), Multiplayer.AudioType.Global);
        assert.areEqual(audioComponent.getAudioAssetId(), "");
        assert.areEqual(audioComponent.getAssetCollectionId(), "");
        assert.areEqual(audioComponent.getAttenuationRadius(), 10);
        assert.areEqual(audioComponent.getIsLoopPlayback(), false);
        assert.areEqual(audioComponent.getTimeSincePlay(), 0);
        assert.areEqual(audioComponent.getVolume(), 1);
        assert.areEqual(audioComponent.getIsEnabled(), true);

        pos.delete();
    }

    // Set new values
    {
        const assetId = "TEST_ASSET_ID";
        const assetCollectionId = "TEST_COLLECTION_ID";

        audioComponent.setPosition(Common.Vector3.create_x_y_z(1, 1, 1));
        audioComponent.setPlaybackState(Multiplayer.AudioPlaybackState.Play);
        audioComponent.setAudioType(Multiplayer.AudioType.Spatial);
        audioComponent.setAudioAssetId(assetId);
        audioComponent.setAssetCollectionId(assetCollectionId);
        audioComponent.setAttenuationRadius(100);
        audioComponent.setIsLoopPlayback(true);
        audioComponent.setTimeSincePlay(1);
        audioComponent.setVolume(0.25);
        audioComponent.setIsEnabled(false);
  
        // Ensure values are set correctly
        const pos = audioComponent.getPosition();

        assert.areEqual(pos.x, 1);
        assert.areEqual(pos.y, 1);
        assert.areEqual(pos.z, 1);
        assert.areEqual(audioComponent.getPlaybackState(), Multiplayer.AudioPlaybackState.Play);
        assert.areEqual(audioComponent.getAudioType(), Multiplayer.AudioType.Spatial);
        assert.areEqual(audioComponent.getAudioAssetId(), assetId);
        assert.areEqual(audioComponent.getAssetCollectionId(), assetCollectionId);
        assert.areEqual(audioComponent.getAttenuationRadius(), 100);
        assert.areEqual(audioComponent.getIsLoopPlayback(), true);
        assert.areEqual(audioComponent.getTimeSincePlay(), 1);
        assert.areEqual(audioComponent.getVolume(), 0.25);
        assert.areEqual(audioComponent.getIsEnabled(), false);

        pos.delete();
    }

    // Test invalid volume values
    {
        audioComponent.setVolume(1.25);
        assert.areEqual(audioComponent.getVolume(), 0.25);

        audioComponent.setVolume(-2.25);
        assert.areEqual(audioComponent.getVolume(), 0.25);
    }

    // Test boundary volume values
    {
        audioComponent.setVolume(1.0);
        assert.areEqual(audioComponent.getVolume(), 1.0);

        audioComponent.setVolume(0.0);
        assert.areEqual(audioComponent.getVolume(), 0.0);
    }
    createdObject.delete();
    component.delete()
    audioComponent.delete();
    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'VideoPlayerComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the audio component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create audio component
    const component = createdObject.addComponent(Multiplayer.ComponentType.VideoPlayer);
    const videoComponent = Multiplayer.VideoPlayerSpaceComponent.fromComponentBase(component);
    
    // Ensure defaults are set
    {
        const pos = videoComponent.getPosition();
        
        assert.areEqual(videoComponent.getPlaybackState(), Multiplayer.VideoPlayerPlaybackState.Reset);
        assert.areEqual(videoComponent.getVideoAssetURL(), "");
        assert.areEqual(videoComponent.getAssetCollectionId(), "");
        assert.areEqual(videoComponent.getAttenuationRadius(), 10);
        assert.areEqual(videoComponent.getIsLoopPlayback(), false);
        assert.areEqual(videoComponent.getTimeSincePlay(), 0);
        assert.areEqual(videoComponent.getIsStateShared(), false);
        assert.areEqual(videoComponent.getIsAutoPlay(), false);
        assert.areEqual(videoComponent.getIsAutoResize(), false);
        assert.areEqual(videoComponent.getCurrentPlayheadPosition(), 0);
        assert.areEqual(videoComponent.getVideoPlayerSourceType(), Multiplayer.VideoPlayerSourceType.AssetSource);
        assert.areEqual(videoComponent.getIsVisible(), true);

        pos.delete();
    }
    
    // Set new values
    {
        const assetId             = "TEST_ASSET_ID";
        const assetCollectionId  = "TEST_COLLECTION_ID";

        videoComponent.setPosition(Common.Vector3.create_x_y_z(1, 1, 1));
        videoComponent.setPlaybackState(Multiplayer.VideoPlayerPlaybackState.Play);
        videoComponent.setVideoAssetURL("http://youtube.com/avideo");
        videoComponent.setVideoAssetId(assetId);
        videoComponent.setAssetCollectionId(assetCollectionId);
        videoComponent.setAttenuationRadius(100);
        videoComponent.setIsLoopPlayback(true);
        videoComponent.setTimeSincePlay(1);
        videoComponent.setIsStateShared(true);
        videoComponent.setIsAutoPlay(true);
        videoComponent.setIsAutoResize(true);
        videoComponent.setCurrentPlayheadPosition(1);
        videoComponent.setVideoPlayerSourceType(Multiplayer.VideoPlayerSourceType.URLSource);
        videoComponent.setIsVisible(false);

        // Ensure values are set correctly
        const pos = videoComponent.getPosition();
        
        // Ensure values are set correctly
        assert.areEqual(videoComponent.getPlaybackState(), Multiplayer.VideoPlayerPlaybackState.Play);
        assert.areEqual(videoComponent.getVideoAssetURL(), "http://youtube.com/avideo");
        assert.areEqual(videoComponent.getVideoAssetId(), assetId);
        assert.areEqual(videoComponent.getAssetCollectionId(), assetCollectionId);
        assert.areEqual(videoComponent.getAttenuationRadius(), 100);
        assert.areEqual(videoComponent.getIsLoopPlayback(), true);
        assert.areEqual(videoComponent.getTimeSincePlay(), 1);
        assert.areEqual(videoComponent.getIsStateShared(), true);
        assert.areEqual(videoComponent.getIsAutoPlay(), true);
        assert.areEqual(videoComponent.getIsAutoResize(), true);
        assert.areEqual(videoComponent.getCurrentPlayheadPosition(), 1);
        assert.areEqual(videoComponent.getVideoPlayerSourceType(), Multiplayer.VideoPlayerSourceType.URLSource);
        assert.areEqual(videoComponent.getIsVisible(), false);

        pos.delete();
    }
    
    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
})

const approximatelyEqual = (v1, v2, epsilon = 0.001) => Math.abs(v1 - v2) < epsilon;

test('MultiplayerTests', 'LightComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();
 
    entitySystem.setEntityCreatedCallback((e) => {});
    
    // Connect
    {
        const ok = await connection.connect();
    
        assert.isTrue(ok);
    }
 
    // Initialise connection
    {
        const ok = await connection.initialiseConnection();
    
        assert.isTrue(ok);
    }

    // Create object to represent the audio component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create audio component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Light);
    const lightComponent = Multiplayer.LightSpaceComponent.fromComponentBase(component);
    
    // Ensure defaults are set
    {
        const pos = lightComponent.getPosition();
        
        assert.areEqual(lightComponent.getLightCookieType(), Multiplayer.LightCookieType.NoCookie);
        assert.areEqual(lightComponent.getLightType(), Multiplayer.LightType.Point);
        assert.areEqual(lightComponent.getLightShadowType(), Multiplayer.LightShadowType.None);
        assert.areEqual(lightComponent.getInnerConeAngle(), 0.0);
        assert.isTrue(approximatelyEqual(lightComponent.getOuterConeAngle(),  0.78539816339, 0.0000001));
        assert.areEqual(lightComponent.getRange(), 1000.0);
        assert.areEqual(lightComponent.getIntensity(), 5000.0);

        pos.delete();
    }
    
    // Set new values
    {
        const assetId             = "TEST_ASSET_ID";
        const assetCollectionId  = "TEST_COLLECTION_ID";

        // test values
        const InnerConeAngle = 10.0;
        const OuterConeAngle = 20.0;
        const Range		   = 120.0;
        const Intensity	   = 1000.0;
        
        lightComponent.setPosition(Common.Vector3.create_x_y_z(1, 1, 1));
        lightComponent.setLightCookieAssetId(assetId);
        lightComponent.setLightCookieAssetCollectionId(assetCollectionId);
        lightComponent.setLightCookieType(Multiplayer.LightCookieType.ImageCookie);
        lightComponent.setLightType(Multiplayer.LightType.Spot);
        lightComponent.setLightShadowType(Multiplayer.LightType.Realtime);
        lightComponent.setInnerConeAngle(InnerConeAngle);
        lightComponent.setOuterConeAngle(OuterConeAngle);
        lightComponent.setRange(Range);
        lightComponent.setIntensity(Intensity);

        // Ensure values are set correctly
        const pos = lightComponent.getPosition();
        
        // Ensure values are set correctly
        assert.areEqual(lightComponent.getLightCookieAssetCollectionId(), assetCollectionId);
        assert.areEqual(lightComponent.getLightCookieAssetId(), assetId);
        assert.areEqual(lightComponent.getLightCookieType(), Multiplayer.LightCookieType.ImageCookie);
        assert.areEqual(lightComponent.getLightType(), Multiplayer.LightType.Spot);
        assert.areEqual(lightComponent.getLightShadowType(), Multiplayer.LightType.Realtime);
        assert.areEqual(lightComponent.getInnerConeAngle(), InnerConeAngle);
        assert.areEqual(lightComponent.getOuterConeAngle(), OuterConeAngle);
        assert.areEqual(lightComponent.getRange(), Range);
        assert.areEqual(lightComponent.getIntensity(), Intensity);

        pos.delete();
    }
    
    // Cleanup
    await connection.disconnect();
    connection.delete();
})

test('MultiplayerTests', 'ImageComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();
 
    entitySystem.setEntityCreatedCallback((e) => {});
    
    // Connect
    {
        const ok = await connection.connect();
    
        assert.isTrue(ok);
    }
 
    // Initialise connection
    {
        const ok = await connection.initialiseConnection();
    
        assert.isTrue(ok);
    }

    // Create object to represent the audio component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create audio component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Image);
    const imageComponent = Multiplayer.ImageSpaceComponent.fromComponentBase(component);
    
    // Ensure defaults are set
    {
        const pos = imageComponent.getPosition();
        
        assert.areEqual(imageComponent.getBillboardMode(), Multiplayer.BillboardMode.Off);
        assert.areEqual(imageComponent.getDisplayMode(), Multiplayer.DisplayMode.DoubleSided);
        assert.areEqual(imageComponent.getIsARVisible(), true);
        assert.areEqual(imageComponent.getIsEmissive(), false);

        pos.delete();
    }
    
    // Set new values
    {
        const assetId             = "TEST_ASSET_ID";
        const assetCollectionId  = "TEST_COLLECTION_ID";

        // test values
        const InnerConeAngle = 10.0;
        const OuterConeAngle = 20.0;
        const Range		   = 120.0;
        const Intensity	   = 1000.0;
        
        imageComponent.setPosition(Common.Vector3.create_x_y_z(1, 1, 1));
        imageComponent.setImageAssetId(assetId);
        imageComponent.setAssetCollectionId(assetCollectionId);
        imageComponent.setBillboardMode(Multiplayer.BillboardMode.Billboard);
        imageComponent.setDisplayMode(Multiplayer.DisplayMode.DoubleSidedReversed);
        imageComponent.setIsARVisible(false);
        imageComponent.setIsEmissive(true);

        // Ensure values are set correctly
        const pos = imageComponent.getPosition();
        
        // Ensure values are set correctly
        assert.areEqual(imageComponent.getImageAssetId(), assetId);
        assert.areEqual(imageComponent.getAssetCollectionId(), assetCollectionId);
        assert.areEqual(imageComponent.getBillboardMode(), Multiplayer.BillboardMode.Billboard);
        assert.areEqual(imageComponent.getDisplayMode(), Multiplayer.DisplayMode.DoubleSidedReversed);
        assert.areEqual(imageComponent.getIsARVisible(), false);
        assert.areEqual(imageComponent.getIsEmissive(), true);

        pos.delete();
    }
    
    // Cleanup
    await connection.disconnect();
    connection.delete();
})

test('MultiplayerTests', 'CollisionComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();
 
    entitySystem.setEntityCreatedCallback((e) => {});
    
    // Connect
    {
        var ok = await connection.connect();
    
        assert.isTrue(ok);
    }
 
    // Initialise connection
    {
        var ok = await connection.initialiseConnection();
    
        assert.isTrue(ok);
    }

    // Create object to represent the collision component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create collision component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Collision);
    const collisionComponent = Multiplayer.CollisionSpaceComponent.fromComponentBase(component);
    
    // Ensure defaults are set
    {
        const pos = collisionComponent.getPosition();
        assert.areEqual(pos.x, 0);
        assert.areEqual(pos.y, 0);
        assert.areEqual(pos.z, 0);

        const scale = collisionComponent.getScale();
        assert.areEqual(scale.x, 1);
        assert.areEqual(scale.y, 1);
        assert.areEqual(scale.z, 1);

        const unscaledBoundingMin = collisionComponent.getUnscaledBoundingBoxMin();
        const unscaledBoundingMax = collisionComponent.getUnscaledBoundingBoxMax();
        const scaledBoundingMin = collisionComponent.getScaledBoundingBoxMin();
        const scaledBoundingMax = collisionComponent.getScaledBoundingBoxMax();

        // Ensure defaults are set
        assert.areEqual(unscaledBoundingMin.x, -0.5);
        assert.areEqual(unscaledBoundingMin.y, -0.5);
        assert.areEqual(unscaledBoundingMin.z, -0.5);
        assert.areEqual(unscaledBoundingMax.x, 0.5);
        assert.areEqual(unscaledBoundingMax.y, 0.5);
        assert.areEqual(unscaledBoundingMax.z, 0.5);
        assert.areEqual(scaledBoundingMin.x, -0.5);
        assert.areEqual(scaledBoundingMin.y, -0.5);
        assert.areEqual(scaledBoundingMin.z, -0.5);
        assert.areEqual(scaledBoundingMax.x, 0.5);
        assert.areEqual(scaledBoundingMax.y, 0.5);
        assert.areEqual(scaledBoundingMax.z, 0.5);

        assert.areEqual(collisionComponent.getCollisionMode(), Multiplayer.CollisionMode.Collision);
        assert.areEqual(collisionComponent.getCollisionShape(), Multiplayer.CollisionShape.Box);
        assert.areEqual(collisionComponent.getCollisionAssetId(), "");
        assert.areEqual(collisionComponent.getAssetCollectionId(), "");

        pos.delete();
        scale.delete();
        unscaledBoundingMin.delete();
        unscaledBoundingMax.delete();
        scaledBoundingMin.delete();
        scaledBoundingMax.delete();
    }
    
    // Set new values
    {
        const assetId             = "TEST_ASSET_ID";
        const assetCollectionId  = "TEST_COLLECTION_ID";

        collisionComponent.setPosition(Common.Vector3.create_x_y_z(1, 1, 1));
        collisionComponent.setScale(Common.Vector3.create_x_y_z(2, 2, 2));
        collisionComponent.setCollisionMode(Multiplayer.CollisionMode.Trigger);
        collisionComponent.setCollisionShape(Multiplayer.CollisionShape.Mesh);
        collisionComponent.setCollisionAssetId("TestAssetID");
        collisionComponent.setAssetCollectionId("TestAssetCollectionID");

        // Ensure values are set correctly
        const pos = collisionComponent.getPosition();
        assert.areEqual(pos.x, 1);
        assert.areEqual(pos.y, 1);
        assert.areEqual(pos.z, 1);

        const scale = collisionComponent.getScale();
        assert.areEqual(scale.x, 2);
        assert.areEqual(scale.y, 2);
        assert.areEqual(scale.z, 2);
        
        // Ensure values are set correctly
        const unscaledBoundingMin = collisionComponent.getUnscaledBoundingBoxMin();
        const unscaledBoundingMax = collisionComponent.getUnscaledBoundingBoxMax();
        const scaledBoundingMin = collisionComponent.getScaledBoundingBoxMin();
        const scaledBoundingMax = collisionComponent.getScaledBoundingBoxMax();

        assert.areEqual(unscaledBoundingMin.x, -0.5);
        assert.areEqual(unscaledBoundingMin.y, -0.5);
        assert.areEqual(unscaledBoundingMin.z, -0.5);
        assert.areEqual(unscaledBoundingMax.x, 0.5);
        assert.areEqual(unscaledBoundingMax.y, 0.5);
        assert.areEqual(unscaledBoundingMax.z, 0.5);
        assert.areEqual(scaledBoundingMin.x, -1);
        assert.areEqual(scaledBoundingMin.y, -1);
        assert.areEqual(scaledBoundingMin.z, -1);
        assert.areEqual(scaledBoundingMax.x, 1);
        assert.areEqual(scaledBoundingMax.y, 1);
        assert.areEqual(scaledBoundingMax.z, 1);
        assert.areEqual(collisionComponent.getCollisionMode(), Multiplayer.CollisionMode.Trigger);
        assert.areEqual(collisionComponent.getCollisionShape(), Multiplayer.CollisionShape.Mesh);
        assert.areEqual(collisionComponent.getCollisionAssetId(), "TestAssetID");
        assert.areEqual(collisionComponent.getAssetCollectionId(), "TestAssetCollectionID");

        var defaultSphereRadius = Multiplayer.CollisionSpaceComponent.getDefaultSphereRadius()
        var defaultCapsuleHalfWidth = Multiplayer.CollisionSpaceComponent.getDefaultCapsuleHalfWidth();
        var defaultCapsuleHalfHeight = Multiplayer.CollisionSpaceComponent.getDefaultCapsuleHalfHeight();

        assert.areEqual(defaultSphereRadius, 0.5);
        assert.areEqual(defaultCapsuleHalfWidth, 0.5);
        assert.areEqual(defaultCapsuleHalfHeight, 1.0);

        pos.delete();
        scale.delete();
        unscaledBoundingMin.delete();
        unscaledBoundingMax.delete();
        scaledBoundingMin.delete();
        scaledBoundingMax.delete();
    }
    
    // Cleanup
    await connection.disconnect();
    connection.delete();
})

test('MultiplayerTests', 'AudioScriptInterfaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the audio
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create audio component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Audio);
    const audioComponent = Multiplayer.AudioSpaceComponent.fromComponentBase(component);

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Setup script
    let scriptText = `

        const assetId            = "TEST_ASSET_ID";
        const assetCollectionId = "TEST_COLLECTION_ID";

        var audio = ThisEntity.getAudioComponents()[0];
        audio.position = [1, 1, 1];
        audio.playbackState = 2;
        audio.audioType = 1;
        audio.audioAssetId = assetId;
        audio.assetCollectionId = assetCollectionId;
        audio.attenuationRadius = 100;
        audio.isLoopPlayback = true;
        audio.timeSincePlay = 1;
        audio.volume = 0.75;

    `;

    const script = createdObject.getScript();
    script.setScriptSource(scriptText);
    script.invoke();

    entitySystem.processPendingEntityOperations();

    // Ensure values are set correctly
    const assetId = "TEST_ASSET_ID";
    const assetCollectionId = "TEST_COLLECTION_ID";

    const pos = audioComponent.getPosition();

    assert.areEqual(pos.x, 1);
    assert.areEqual(pos.y, 1);
    assert.areEqual(pos.z, 1);
    assert.areEqual(audioComponent.getPlaybackState(), Multiplayer.AudioPlaybackState.Play);
    assert.areEqual(audioComponent.getAudioType(), Multiplayer.AudioType.Spatial);
    assert.areEqual(audioComponent.getAudioAssetId(), assetId);
    assert.areEqual(audioComponent.getAssetCollectionId(), assetCollectionId);
    assert.areEqual(audioComponent.getAttenuationRadius(), 100);
    assert.areEqual(audioComponent.getIsLoopPlayback(), true);
    assert.areEqual(audioComponent.getTimeSincePlay(), 1);
    assert.areEqual(audioComponent.getVolume(), 0.75);
    component.delete();
    audioComponent.delete();
    createdObject.delete();



    // Test invalid volume values
    scriptText = `var audio = ThisEntity.getAudioComponents()[0]; audio.volume = 1.75;`;
    script.setScriptSource(scriptText);
    script.invoke();
    entitySystem.processPendingEntityOperations();
    entitySystem.registerEntityScriptAsModule
    assert.areEqual(audioComponent.getVolume(), 0.75);

    scriptText = `var audio = ThisEntity.getAudioComponents()[0]; audio.volume = -2.75;`;
    script.setScriptSource(scriptText);
    script.invoke();
    entitySystem.processPendingEntityOperations();
    assert.areEqual(audioComponent.getVolume(), 0.75);

    // Test boundary volume values
    scriptText = `var audio = ThisEntity.getAudioComponents()[0]; audio.volume = 1.0;`;
    script.setScriptSource(scriptText);
    script.invoke();
    entitySystem.processPendingEntityOperations();
    assert.areEqual(audioComponent.getVolume(), 1.0);

    scriptText = `var audio = ThisEntity.getAudioComponents()[0]; audio.volume = 0.0;`;
    script.setScriptSource(scriptText);
    script.invoke();
    entitySystem.processPendingEntityOperations();
    assert.areEqual(audioComponent.getVolume(), 0.0);

    // Cleanup
    pos.delete();
    
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'SplineComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();
     // Log in
     await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

     // Create space
     const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the spline component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create spline component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Spline);
    const splineComponent = Multiplayer.SplineSpaceComponent.fromComponentBase(component);
    createdObject.delete();

    var waypoints = Common.List.ofoly_common_Vector3();  //[[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
    waypoints.append(Common.Vector3.create_x_y_z(0, 0, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 1000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 2000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 3000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 4000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 5000, 0));
    // Set Resolution values
    {
        splineComponent.setWaypoints(waypoints);
        assert.areEqual(splineComponent.getWaypoints().get(5).x, waypoints.get(5).x);
        assert.areEqual(splineComponent.getWaypoints().get(5).y, waypoints.get(5).y);
        assert.areEqual(splineComponent.getWaypoints().get(5).z, waypoints.get(5).z);
    }

    {
        // get spline point
        var result = splineComponent.getLocationAlongSpline(1);

        // check final spline point matches waypoint
        assert.areEqual(result.x,waypoints.get(waypoints.size()-1).x);
        assert.areEqual(result.y,waypoints.get(waypoints.size()-1).y);
        assert.areEqual(result.z,waypoints.get(waypoints.size()-1).z);
        result.delete();
    }
    waypoints.delete();
    component.delete();
    splineComponent.delete();
    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'SplineScriptInterfaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the spline
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create spline component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Spline);
    const splineComponent = Multiplayer.SplineSpaceComponent.fromComponentBase(component);

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    var waypoints = Common.List.ofoly_common_Vector3();  //[[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
    waypoints.append(Common.Vector3.create_x_y_z(0, 0, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 1000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 2000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 3000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 4000, 0));
    waypoints.append(Common.Vector3.create_x_y_z(0, 5000, 0));

    // Setup script
    const scriptText = `
    var spline = ThisEntity.getSplineComponents()[0];
        
    var waypoints = [[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
    spline.setWaypoints(waypoints);
    var positionResult = spline.getLocationAlongSpline(1);
    `;

    const script = createdObject.getScript();
    script.setScriptSource(scriptText);
    script.invoke();

    entitySystem.processPendingEntityOperations();

    assert.areEqual(splineComponent.getWaypoints().get(5).x, waypoints.get(5).x);
    assert.areEqual(splineComponent.getWaypoints().get(5).y, waypoints.get(5).y);
    assert.areEqual(splineComponent.getWaypoints().get(5).z, waypoints.get(5).z);
    
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'ConversationComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

     // Log in
     const userId = await logIn(userSystem);
     const userProfile = await getProfileByUserId(userSystem, userId);
     const userDisplayName = userProfile.displayName;
     userProfile.delete();
     
    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

     // Create space
     const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
     const enterResult = await spaceSystem.enterSpace(space.id, true);
     assert.succeeded(enterResult);
     var connection  = enterResult.getConnection();
     var entitySystem = connection.getSpaceEntitySystem();
     enterResult.delete();
     entitySystem.setEntityCreatedCallback((e) => {});
    // Create object to represent the conversation component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    // Setup Asset callback
    let ConversationNewMessageCallbackCalled = false;
    let conversationId = "";
     let messageId = "";
    connection.setConversationSystemCallback((p) => {
    if (ConversationNewMessageCallbackCalled)
        return;
    assert.areEqual(Multiplayer.ConversationMessageType.NewMessage, p.messageType);
    assert.areEqual(conversationId, p.messageValue);

    ConversationNewMessageCallbackCalled = true;
    });

    // Create Conversation component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Conversation);
    const conversationComponent = Multiplayer.ConversationSpaceComponent.fromComponentBase(component);

    // Ensure defaults are set
    {
        assert.areEqual(conversationComponent.getIsActive(),true);
        assert.areEqual(conversationComponent.getIsVisible(),true);

        assert.areEqual(conversationComponent.getPosition().x, 0);
        assert.areEqual(conversationComponent.getPosition().y, 0);
        assert.areEqual(conversationComponent.getPosition().z, 0);

        assert.areEqual(conversationComponent.getRotation().w, 1);
        assert.areEqual(conversationComponent.getRotation().x, 0);
        assert.areEqual(conversationComponent.getRotation().y, 0);
        assert.areEqual(conversationComponent.getRotation().z, 0);

        assert.areEqual(conversationComponent.getTitle(), "");
        assert.areEqual(conversationComponent.getDate(), "");
        assert.areEqual(conversationComponent.getNumberOfReplies(), BigInt(0));
    }

    // Set new values
    {
        conversationComponent.setIsActive(false);
        conversationComponent.setIsVisible(false);

        assert.areEqual(conversationComponent.getIsActive(),false);
        assert.areEqual(conversationComponent.getIsVisible(),false);

        var newPosition = Common.Vector3.create_x_y_z(1, 2, 3);
        conversationComponent.setPosition(newPosition);
        assert.areEqual(conversationComponent.getPosition().x, newPosition.x);
        assert.areEqual(conversationComponent.getPosition().y, newPosition.y);
        assert.areEqual(conversationComponent.getPosition().z, newPosition.z);

        var newRotation = Common.Vector4.create_x_y_z_w(4, 5, 6, 7);
        conversationComponent.setRotation(newRotation)
        assert.areEqual(conversationComponent.getRotation().w, newRotation.w);
        assert.areEqual(conversationComponent.getRotation().x, newRotation.x);
        assert.areEqual(conversationComponent.getRotation().y, newRotation.y);
        assert.areEqual(conversationComponent.getRotation().z, newRotation.z);

        conversationComponent.setTitle("TestTitle");
        conversationComponent.setDate("01-02-1993");
        conversationComponent.setNumberOfReplies(BigInt(2));

        assert.areEqual(conversationComponent.getTitle(), "TestTitle");
        assert.areEqual(conversationComponent.getDate(), "01-02-1993");
        assert.areEqual(conversationComponent.getNumberOfReplies(), BigInt(2));

        newPosition.delete();
        newRotation.delete();
    }

    // create conversation
    {
        const result = await conversationComponent.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();
    }

    // Get Conversation Info
    {
        const result = await conversationComponent.getConversationInfo()
        assert.succeeded(result);
        assert.areEqual(result.getConversationInfo().conversationId, conversationId);
        assert.areEqual(result.getConversationInfo().userID, userId);
        assert.areEqual(result.getConversationInfo().userDisplayName, userDisplayName);
        assert.areEqual(result.getConversationInfo().message,"TestMessage");
        assert.isFalse(result.getConversationInfo().edited);
        assert.isFalse(result.getConversationInfo().resolved);
                
        var defaultValue = Multiplayer.SpaceTransform.create();
        assert.areEqual(result.getConversationInfo().cameraPosition.position.x,defaultValue.position.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.y,defaultValue.position.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.z,defaultValue.position.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.w,defaultValue.rotation.w);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.x,defaultValue.rotation.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.y,defaultValue.rotation.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.z,defaultValue.rotation.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.scale.x,defaultValue.scale.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.y,defaultValue.scale.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.z,defaultValue.scale.z);
        
        defaultValue.delete();
        result.delete();
    }

    // Get Conversation Info
    {
        const result = await conversationComponent.getConversationInfo()
        assert.succeeded(result);
        assert.areEqual(result.getConversationInfo().conversationId, conversationId);
        assert.areEqual(result.getConversationInfo().userID, userId);
        assert.areEqual(result.getConversationInfo().userDisplayName, userDisplayName);
        assert.areEqual(result.getConversationInfo().message,"TestMessage");
        assert.isFalse(result.getConversationInfo().edited);
        assert.isFalse(result.getConversationInfo().resolved);
        
        var defaultValue = Multiplayer.SpaceTransform.create();
        assert.areEqual(result.getConversationInfo().cameraPosition.position.x,defaultValue.position.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.y,defaultValue.position.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.z,defaultValue.position.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.w,defaultValue.rotation.w);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.x,defaultValue.rotation.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.y,defaultValue.rotation.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.z,defaultValue.rotation.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.scale.x,defaultValue.scale.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.y,defaultValue.scale.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.z,defaultValue.scale.z);
                
        defaultValue.delete();
        result.delete();
    }

    // Update Conversation Info
    {
        var newData = Multiplayer.ConversationInfo.create();
        var cameraTranfromValue = Multiplayer.SpaceTransform.create_position_rotation_scale(Common.Vector3.one(),Common.Vector4.one(),Common.Vector3.one())
        newData.message = "TestMessage1";
        newData.resolved = true;
        newData.cameraPosition = cameraTranfromValue;
        const result = await conversationComponent.setConversationInfo(newData);
        assert.succeeded(result);
        assert.areEqual(result.getConversationInfo().conversationId, conversationId);
        assert.areEqual(result.getConversationInfo().userID, userId);
        assert.areEqual(result.getConversationInfo().userDisplayName, userDisplayName);
        assert.areEqual(result.getConversationInfo().message,"TestMessage1");
        assert.isTrue(result.getConversationInfo().edited);
        assert.isTrue(result.getConversationInfo().resolved);
        
        assert.areEqual(result.getConversationInfo().cameraPosition.position.x,cameraTranfromValue.position.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.y,cameraTranfromValue.position.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.z,cameraTranfromValue.position.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.w,cameraTranfromValue.rotation.w);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.x,cameraTranfromValue.rotation.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.y,cameraTranfromValue.rotation.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.z,cameraTranfromValue.rotation.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.scale.x,cameraTranfromValue.scale.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.y,cameraTranfromValue.scale.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.z,cameraTranfromValue.scale.z);

        result.delete();
        newData.delete();
        cameraTranfromValue.delete();
    }

    // send message
    {
        const result = await conversationComponent.addMessage("Test");
        messageId = result.getMessageInfo().id;
        assert.areEqual(result.getMessageInfo().edited,false)
        assert.succeeded(result);
        result.delete
    }

    // Get Message Info
    {
        const result = await conversationComponent.getMessageInfo(messageId)
        assert.succeeded(result);
        assert.areEqual(result.getMessageInfo().edited,false)
        result.delete();
    }

    // Update Message Info
    {
        var newMessageData = Multiplayer.MessageInfo.create();
        newMessageData.message = "NewTest";
        const result = await conversationComponent.setMessageInfo(messageId,newMessageData);
        assert.succeeded(result);
        assert.areEqual(result.getMessageInfo().edited,true)
        result.delete();
        newData.delete();
    }

    {
        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        var array = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
        array.set(0,Multiplayer.ReplicatedValue.create_longValue(BigInt(Multiplayer.ConversationMessageType.NewMessage)));
        array.set(1,Multiplayer.ReplicatedValue.create_stringValue(conversationId));
        await connection.sendNetworkEventToClient("ConversationSystem",array,connection.getClientId());
        array.delete();
    }

    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!ConversationNewMessageCallbackCalled && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(ConversationNewMessageCallbackCalled);
    {
        const result = await conversationComponent.deleteConversation();
        assert.succeeded(result);
        result.delete();
    }

    // Cleanup
    transform.delete();
    createdObject.delete();
    component.delete();
    conversationComponent.delete();
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'ConversationComponentMoveTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

     // Log in
     const userId = await logIn(userSystem);
     const userProfile = await getProfileByUserId(userSystem, userId);
     const userDisplayName = userProfile.displayName;
     userProfile.delete();
     
    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

     // Create space
     const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
     const enterResult = await spaceSystem.enterSpace(space.id, true);
     assert.succeeded(enterResult);
     var connection  = enterResult.getConnection();
     var entitySystem = connection.getSpaceEntitySystem();
     enterResult.delete();
     entitySystem.setEntityCreatedCallback((e) => {});
    // Create object to represent the conversation component
    const name1 = "TestObject1";
    const name2 = "TestObject2";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject1 = await entitySystem.createObject(name1, transform);
    const createdObject2 = await entitySystem.createObject(name2, transform);

    // Setup Asset callback
    let ConversationNewMessageCallbackCalled = false;
    let conversationId = "";
    let messageId = "";
    connection.setConversationSystemCallback((p) => {
        if (ConversationNewMessageCallbackCalled)
            return;
        assert.areEqual(Multiplayer.ConversationMessageType.NewMessage, p.messageType);
        assert.areEqual(conversationId, p.messageValue);

        ConversationNewMessageCallbackCalled = true;
    });

    // Create Conversation component
    const component1 = createdObject1.addComponent(Multiplayer.ComponentType.Conversation);
    const conversationComponent1 = Multiplayer.ConversationSpaceComponent.fromComponentBase(component1);

    const component2 = createdObject2.addComponent(Multiplayer.ComponentType.Conversation);
    const conversationComponent2 = Multiplayer.ConversationSpaceComponent.fromComponentBase(component2);

    // create conversation
    {
        const result = await conversationComponent1.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();
    }

    // Get Conversation Info
    {
        const result = await conversationComponent1.getConversationInfo()
        assert.succeeded(result);
        assert.areEqual(result.getConversationInfo().conversationId, conversationId);
        assert.areEqual(result.getConversationInfo().userID, userId);
        assert.areEqual(result.getConversationInfo().userDisplayName, userDisplayName);
        assert.areEqual(result.getConversationInfo().message,"TestMessage");
        assert.isFalse(result.getConversationInfo().edited);
        assert.isFalse(result.getConversationInfo().resolved);
                
        var defaultValue = Multiplayer.SpaceTransform.create();
        assert.areEqual(result.getConversationInfo().cameraPosition.position.x,defaultValue.position.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.y,defaultValue.position.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.z,defaultValue.position.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.w,defaultValue.rotation.w);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.x,defaultValue.rotation.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.y,defaultValue.rotation.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.z,defaultValue.rotation.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.scale.x,defaultValue.scale.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.y,defaultValue.scale.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.z,defaultValue.scale.z);
        
        defaultValue.delete();
        result.delete();
    }

    {
        const result = await conversationComponent2.getConversationInfo()
        assert.failed(result);
        result.delete();
    }

    {
        const result = conversationComponent2.moveConversationFromComponent(conversationComponent1);
        assert.isTrue(result);
    }

    {
        const result = await conversationComponent1.getConversationInfo()
        assert.failed(result);
        result.delete();
    }

    {
        const result = await conversationComponent2.getConversationInfo()
        assert.succeeded(result);
        assert.areEqual(result.getConversationInfo().conversationId, conversationId);
        assert.areEqual(result.getConversationInfo().userID, userId);
        assert.areEqual(result.getConversationInfo().userDisplayName, userDisplayName);
        assert.areEqual(result.getConversationInfo().message,"TestMessage");
        assert.isFalse(result.getConversationInfo().edited);
        assert.isFalse(result.getConversationInfo().resolved);
                
        var defaultValue = Multiplayer.SpaceTransform.create();
        assert.areEqual(result.getConversationInfo().cameraPosition.position.x,defaultValue.position.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.y,defaultValue.position.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.position.z,defaultValue.position.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.w,defaultValue.rotation.w);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.x,defaultValue.rotation.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.y,defaultValue.rotation.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.rotation.z,defaultValue.rotation.z);

        assert.areEqual(result.getConversationInfo().cameraPosition.scale.x,defaultValue.scale.x);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.y,defaultValue.scale.y);
        assert.areEqual(result.getConversationInfo().cameraPosition.scale.z,defaultValue.scale.z);
        
        defaultValue.delete();
        result.delete();
    }
    
    {
        const result = await conversationComponent2.deleteConversation();
        assert.succeeded(result);
        result.delete();
    }

    // Cleanup
    component1.delete();
    component2.delete();
    conversationComponent1.delete();
    conversationComponent2.delete();
    transform.delete();
    createdObject1.delete();
    createdObject2.delete();
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'ConversationScriptInterfaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const connection = Multiplayer.MultiplayerConnection.create_spaceId(space.id);
    const entitySystem = connection.getSpaceEntitySystem();
 
    entitySystem.setEntityCreatedCallback((e) => {});
     
    // Connect
    {
        var ok = await connection.connect();
     
        assert.isTrue(ok);
    }
 
    // Initialise connection
    {
        var ok = await connection.initialiseConnection();
     
        assert.isTrue(ok);
    }

    // Create object to represent the audio
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create conversation component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Conversation);
    const conversationComponent = Multiplayer.ConversationSpaceComponent.fromComponentBase(component);

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Setup script
    let scriptText = `
        var conversation = ThisEntity.getConversationComponents()[0];
        conversation.isVisible = false;
        conversation.isActive = false;
        conversation.position = [1,2,3];
        conversation.rotation = [4,5,6,7];
    `;

    const script = createdObject.getScript();
    script.setScriptSource(scriptText);
    script.invoke();

    entitySystem.processPendingEntityOperations();

    // Ensure values are set correctly
    assert.areEqual(conversationComponent.getIsActive(),false);
    assert.areEqual(conversationComponent.getIsVisible(),false);

    var newPosition = Common.Vector3.create_x_y_z(1, 2, 3);
    conversationComponent.setPosition(newPosition);
    assert.areEqual(conversationComponent.getPosition().x, newPosition.x);
    assert.areEqual(conversationComponent.getPosition().y, newPosition.y);
    assert.areEqual(conversationComponent.getPosition().z, newPosition.z);

    var newRotation = Common.Vector4.create_x_y_z_w(4, 5, 6, 7);
    conversationComponent.setRotation(newRotation)
    assert.areEqual(conversationComponent.getRotation().w, newRotation.w);
    assert.areEqual(conversationComponent.getRotation().x, newRotation.x);
    assert.areEqual(conversationComponent.getRotation().y, newRotation.y);
    assert.areEqual(conversationComponent.getRotation().z, newRotation.z);

    newPosition.delete();
    newRotation.delete();

    await connection.disconnect();
    connection.delete();
});

test('MultiplayerTests', 'CustomComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();
     // Log in
     await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

     // Create space
     const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
     const enterResult = await spaceSystem.enterSpace(space.id, true);
     assert.succeeded(enterResult);
     var connection  = enterResult.getConnection();
     var entitySystem = connection.getSpaceEntitySystem();
     enterResult.delete();
     entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the Custom component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create Custom component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Custom);
    const customComponent = Multiplayer.CustomSpaceComponent.fromComponentBase(component);
    
    // Vector Check
    {
        var vector3 = Common.Vector3.create_x_y_z(1, 1, 1);
        customComponent.setCustomProperty("Vector3",Multiplayer.ReplicatedValue.create_vector3Value(vector3))
        var returnVector3 = customComponent.getCustomProperty("Vector3").getVector3()
        assert.areEqual(returnVector3.x,vector3.x);
        assert.areEqual(returnVector3.y,vector3.y);
        assert.areEqual(returnVector3.z,vector3.z);
        vector3.delete();
        returnVector3.delete();

        var vector4 = Common.Vector4.create_x_y_z_w(1,1,1,1)
        customComponent.setCustomProperty("Vector4",Multiplayer.ReplicatedValue.create_vector4Value(vector4))
        var returnVector4 = customComponent.getCustomProperty("Vector4").getVector4()
        assert.areEqual(returnVector4.w,vector4.w);
        assert.areEqual(returnVector4.x,vector4.x);
        assert.areEqual(returnVector4.y,vector4.y);
        assert.areEqual(returnVector4.z,vector4.z);
        vector4.delete();
        returnVector4.delete();
    }

    // String Check
    {
        customComponent.setCustomProperty("String",Multiplayer.ReplicatedValue.create_stringValue("OKO"));
        assert.areEqual(customComponent.getCustomProperty("String").getString(),"OKO");
    }

    // Boolean Check
    {
        customComponent.setCustomProperty("Boolean",Multiplayer.ReplicatedValue.create_boolValue(true));
        assert.areEqual(customComponent.getCustomProperty("Boolean").getBool(),true);
    }

    // Integer Check
    {
        customComponent.setCustomProperty("Integer",Multiplayer.ReplicatedValue.create_longValue(BigInt(1)));
        assert.areEqual(customComponent.getCustomProperty("Integer").getInt(),BigInt(1));
    }

    // Float Check
    {
        customComponent.setCustomProperty("Float",Multiplayer.ReplicatedValue.create_floatValue(1.0));
        assert.areEqual(customComponent.getCustomProperty("Float").getFloat(),1.0);
    }

    // Has Key Check
    {
        assert.areEqual(customComponent.hasCustomProperty("Boolean"),true);
        assert.areEqual(customComponent.hasCustomProperty("BooleanFalse"),false);
    }

    // Size Check
    {
        assert.areEqual(customComponent.getNumProperties(),6);
    }

    // Remove Key Check
    {
        customComponent.removeCustomProperty("Boolean");
        assert.areEqual(customComponent.getNumProperties(),5);

    }

    // Cleanup
    customComponent.delete();
    component.delete();
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'CustomComponentScriptInterfaceSubscriptionTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the audio
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    const component = createdObject.addComponent(Multiplayer.ComponentType.Custom);
    const customComponent = Multiplayer.CustomSpaceComponent.fromComponentBase(component);

    const numberRepVal = Multiplayer.ReplicatedValue.create_longValue(BigInt(0));
    customComponent.setCustomProperty("Number", numberRepVal);
    numberRepVal.delete();

    const boolRepVal = Multiplayer.ReplicatedValue.create_boolValue(false);
    customComponent.setCustomProperty("NumberChanged", boolRepVal);
    boolRepVal.delete();

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    // Setup script
    let scriptText = `
        var custom = ThisEntity.getCustomComponents()[0];
        custom.setCustomProperty("testFloat", 1.234);
		custom.setCustomProperty("testInt", 1234);
        globalThis.onValueChanged = () => {
        custom.setCustomProperty("NumberChanged", true);
        }  
        // subscribe to entity events 
        ThisEntity.subscribeToPropertyChange(custom.id, custom.getCustomPropertySubscriptionKey("Number"), "valueChanged");
        ThisEntity.subscribeToMessage("valueChanged", "onValueChanged");    
    `;

    const script = createdObject.getScript();
    script.setScriptSource(scriptText);
    script.invoke();

    entitySystem.processPendingEntityOperations();
    assert.areEqual(customComponent.getCustomProperty("testFloat").getFloat(), 1.234);
    assert.areEqual(customComponent.getCustomProperty("testInt").getInt(), 1234);
    assert.areEqual(customComponent.getCustomProperty("Number").getInt(), BigInt(0));
    assert.isFalse(customComponent.getCustomProperty("NumberChanged").getBool());

    const newNumberRepVal = Multiplayer.ReplicatedValue.create_longValue(BigInt(100));
    customComponent.setCustomProperty("Number", newNumberRepVal);
    newNumberRepVal.delete();

    assert.areEqual(customComponent.getCustomProperty("Number").getInt(), BigInt(100));
    assert.isTrue(customComponent.getCustomProperty("NumberChanged").getBool());

    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();
});


test('MultiplayerTests', 'NetworkEventTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

     // Log in
     await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

     // Create space
     const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
     const enterResult = await spaceSystem.enterSpace(space.id, true);
     assert.succeeded(enterResult);
     var connection  = enterResult.getConnection();
     var entitySystem = connection.getSpaceEntitySystem();
     enterResult.delete();

    var eventRecieved = false;

    const arg1 = false;
    const arg2 = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
    connection.listenNetworkEvent("TestEvent", (arg1, arg2) => 
    {
        assert.areEqual(arg2.get(0).getFloat(), 1234);
        assert.areEqual(arg2.get(1).getString(), "TestingString");
        eventRecieved = true;
    });

    {
        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        var array = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
        array.set(0,Multiplayer.ReplicatedValue.create_floatValue(1234));
        array.set(1,Multiplayer.ReplicatedValue.create_stringValue("TestingString"));
        await connection.sendNetworkEventToClient("TestEvent",array,connection.getClientId());
        array.delete();
    }

    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!eventRecieved && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(eventRecieved);
    {
    }

    // Cleanup
var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'RegisterActionHandlerTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

     // Log in
     await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

     // Create space
     const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
     const enterResult = await spaceSystem.enterSpace(space.id, true);
     assert.succeeded(enterResult);
     var connection  = enterResult.getConnection();
     var entitySystem = connection.getSpaceEntitySystem();
     enterResult.delete();
    // Create object to represent the conversation component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    // Create Conversation component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Light);
    const lightComponent = Multiplayer.LightSpaceComponent.fromComponentBase(component);
    var actionCalled = false;
    component.registerActionHandler("TestAction", (arg1, arg2, arg3) => 
    {
        actionCalled = true;
    });

    component.invokeAction("TestAction", "TestParams");

    assert.isTrue(actionCalled);

    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);
    connection.delete();
});

test('MultiplayerTests', 'FogComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the fog component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create fog component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Fog);
    const fogComponent = Multiplayer.FogSpaceComponent.fromComponentBase(component);

    // Ensure defaults are set
    {
        assert.areEqual(fogComponent.getFogMode(), Multiplayer.FogMode.Linear);
        assert.areApproximatelyEqual(fogComponent.getPosition().x, 0);
        assert.areApproximatelyEqual(fogComponent.getPosition().y, 0);
        assert.areApproximatelyEqual(fogComponent.getPosition().z, 0);
        assert.areApproximatelyEqual(fogComponent.getRotation().x, 0);
        assert.areApproximatelyEqual(fogComponent.getRotation().y, 0);
        assert.areApproximatelyEqual(fogComponent.getRotation().z, 0);
        assert.areApproximatelyEqual(fogComponent.getRotation().w, 1);
        assert.areApproximatelyEqual(fogComponent.getScale().x, 1);
        assert.areApproximatelyEqual(fogComponent.getScale().y, 1);
        assert.areApproximatelyEqual(fogComponent.getScale().z, 1);
        assert.areApproximatelyEqual(fogComponent.getStartDistance(), 0.0);
        assert.areApproximatelyEqual(fogComponent.getEndDistance(), 0.0);
        assert.areApproximatelyEqual(fogComponent.getColor().x, 0);
        assert.areApproximatelyEqual(fogComponent.getColor().y, 0);
        assert.areApproximatelyEqual(fogComponent.getColor().z, 0);
        assert.areApproximatelyEqual(fogComponent.getDensity(), 0.0);
        assert.areApproximatelyEqual(fogComponent.getHeightFalloff(), 0.0);
        assert.areApproximatelyEqual(fogComponent.getMaxOpacity(), 0.0);
        assert.isFalse(fogComponent.getIsVolumetric());

    }

    // Set new values
    {
        const newPosition = Common.Vector3.create_x_y_z(1, 1, 1);
        const newRotation = Common.Vector4.create_x_y_z_w(1, 1, 1, 2);
        const newScale = Common.Vector3.create_x_y_z(2, 2, 2);
        fogComponent.setFogMode(Multiplayer.FogMode.Exponential);
        fogComponent.setPosition(newPosition);
        fogComponent.setRotation(newRotation);
        fogComponent.setScale(newScale);
        fogComponent.setStartDistance(1.0);
        fogComponent.setEndDistance(2.0);
        fogComponent.setColor(newPosition);
        fogComponent.setDensity(3.0);
        fogComponent.setHeightFalloff(4.0);
        fogComponent.setMaxOpacity(5.0);
        fogComponent.setIsVolumetric(true);

        assert.areEqual(fogComponent.getFogMode(), Multiplayer.FogMode.Exponential);
        assert.areApproximatelyEqual(fogComponent.getPosition().x, 1);
        assert.areApproximatelyEqual(fogComponent.getPosition().y, 1);
        assert.areApproximatelyEqual(fogComponent.getPosition().z, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().x, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().y, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().z, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().w, 2);
        assert.areApproximatelyEqual(fogComponent.getScale().x, 2);
        assert.areApproximatelyEqual(fogComponent.getScale().y, 2);
        assert.areApproximatelyEqual(fogComponent.getScale().z, 2);
        assert.areApproximatelyEqual(fogComponent.getStartDistance(), 1.0);
        assert.areApproximatelyEqual(fogComponent.getEndDistance(), 2.0);
        assert.areApproximatelyEqual(fogComponent.getColor().x, 1);
        assert.areApproximatelyEqual(fogComponent.getColor().y, 1);
        assert.areApproximatelyEqual(fogComponent.getColor().z, 1);
        assert.areApproximatelyEqual(fogComponent.getDensity(), 3.0);
        assert.areApproximatelyEqual(fogComponent.getHeightFalloff(), 4.0);
        assert.areApproximatelyEqual(fogComponent.getMaxOpacity(), 5.0);
        assert.isTrue(fogComponent.getIsVolumetric());

        newScale.delete();
        newRotation.delete();
        newPosition.delete();
    }

    createdObject.delete();
    component.delete()
    fogComponent.delete();
    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'FogScriptInterfaceTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
 
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the fog component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create fog component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Fog);
    const fogComponent = Multiplayer.FogSpaceComponent.fromComponentBase(component);

    // Set new values
    {
        createdObject.queueUpdate();
        entitySystem.processPendingEntityOperations();
    
        // Setup script
        let scriptText = `    
            var fog = ThisEntity.getFogComponents()[0];
            fog.fogMode = 1;
            fog.position = [1, 1, 1];
            fog.rotation = [1, 1, 1, 2];
            fog.scale = [2, 2, 2];
            fog.startDistance = 1.0;
            fog.endDistance = 2.0;
            fog.color = [1, 1, 1];
            fog.density = 3.0;
            fog.heightFalloff = 4.0;
            fog.maxOpacity = 5.0;
            fog.isVolumetric = true;
        `;
    
        const script = createdObject.getScript();
        script.setScriptSource(scriptText);
        script.invoke();
    
        entitySystem.processPendingEntityOperations();
    
        assert.areEqual(fogComponent.getFogMode(), Multiplayer.FogMode.Exponential);
        assert.areApproximatelyEqual(fogComponent.getPosition().x, 1);
        assert.areApproximatelyEqual(fogComponent.getPosition().y, 1);
        assert.areApproximatelyEqual(fogComponent.getPosition().z, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().x, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().y, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().z, 1);
        assert.areApproximatelyEqual(fogComponent.getRotation().w, 2);
        assert.areApproximatelyEqual(fogComponent.getScale().x, 2);
        assert.areApproximatelyEqual(fogComponent.getScale().y, 2);
        assert.areApproximatelyEqual(fogComponent.getScale().z, 2);
        assert.areApproximatelyEqual(fogComponent.getStartDistance(), 1.0);
        assert.areApproximatelyEqual(fogComponent.getEndDistance(), 2.0);
        assert.areApproximatelyEqual(fogComponent.getColor().x, 1);
        assert.areApproximatelyEqual(fogComponent.getColor().y, 1);
        assert.areApproximatelyEqual(fogComponent.getColor().z, 1);
        assert.areApproximatelyEqual(fogComponent.getDensity(), 3.0);
        assert.areApproximatelyEqual(fogComponent.getHeightFalloff(), 4.0);
        assert.areApproximatelyEqual(fogComponent.getMaxOpacity(), 5.0);
        assert.isTrue(fogComponent.getIsVolumetric());

    }

    createdObject.delete();
    component.delete()
    fogComponent.delete();
    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();
});

test('MultiplayerTests', 'InvalidComponentTest', async function() {
    let systemsManager = Systems.SystemsManager.get();
    let userSystem = systemsManager.getUserSystem();
    let spaceSystem = systemsManager.getSpaceSystem();

    // Log in
    await logIn(userSystem);

    // Create space
    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    var connection  = enterResult.getConnection();
    var entitySystem = connection.getSpaceEntitySystem();
    enterResult.delete();
    entitySystem.setEntityCreatedCallback((e) => {});

    // Create object to represent the invalid component
    const name = "TestObject";
    const transform = Multiplayer.SpaceTransform.create();

    const createdObject = await entitySystem.createObject(name, transform);

    assert.isTrue(createdObject.pointerIsValid());

    transform.delete();

    // Create invalid component
    const component = createdObject.addComponent(Multiplayer.ComponentType.Invalid);

    createdObject.queueUpdate();
    entitySystem.processPendingEntityOperations();

    createdObject.delete();
    component.delete()
    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);connection.delete();
});
