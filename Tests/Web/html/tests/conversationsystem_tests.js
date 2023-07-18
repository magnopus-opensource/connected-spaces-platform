import { test, assert } from '../test_framework.js';
import { sleep, generateUniqueString, CHS_ENDPOINT_BASE_URI, CONSOLE_RED } from '../test_helpers.js';
import { DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, getProfileByUserId, logIn, logInAsGuest, logOut } from './usersystem_tests_helpers.js'
import { createSpace, deleteSpace, getSpace, getSpacesByIds, updateSpace } from './spacesystem_tests_helpers.js'
import { createAsset, createAssetCollection, createBufferAssetDataSource, uploadAssetData } from './assetsystem_tests_helpers.js';

import { freeBuffer, CSPFoundation, Multiplayer, Services, Systems, Common } from '../connected_spaces_platform.js';
import { commonArrayToJSArray } from '../conversion_helpers.js';


test('ConversationSystemTests', 'ConversationNewMessageCallbackTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';
    const testAssetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    // Log in
    const userId = await logIn(userSystem);
    const userProfile = await getProfileByUserId(userSystem, userId);
    const userDisplayName = userProfile.displayName;
    userProfile.delete();

    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    const connection  = enterResult.getConnection();
    const entitySystem = connection.getSpaceEntitySystem();

    // Setup Asset callback
    let ConversationNewMessageCallbackCalled = false;
    let conversationId = "";

    connection.setConversationSystemCallback((p) => {
        if (ConversationNewMessageCallbackCalled)
            return;
        assert.areEqual(Multiplayer.ConversationMessageType.NewMessage, p.messageType);
        assert.areEqual(conversationId, p.messageValue);

        ConversationNewMessageCallbackCalled = true;
    });

    var conversationSystem = connection.getConversationSystem();
    {
        const result = await conversationSystem.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();
    }

    // Get Conversation Info
    {
        const result = await conversationSystem.getConversationInformation(conversationId)
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
        const result = await conversationSystem.addMessageToConversation(conversationId,"Test","");
        assert.succeeded(result);
        result.delete();
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
        const result = await conversationSystem.deleteConversation(conversationId);
        assert.succeeded(result);
        result.delete();
    }
    
    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);
    
    enterResult.delete();
});

test('ConversationSystemTests', 'ConversationDeleteMessageCallbackTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';
    const testAssetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    // Log in
    const userId = await logIn(userSystem);
    const userProfile = await getProfileByUserId(userSystem, userId);
    const userDisplayName = userProfile.displayName;
    userProfile.delete();
    
    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    const connection  = enterResult.getConnection();
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup Asset callback
    let ConversationDeleteMessageCallbackCalled = false;
    let conversationId = "";
    let messageId = "";

    var conversationSystem = connection.getConversationSystem();
    {
        const result = await conversationSystem.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete()
    }
    // Get Conversation Info
    {
        const result = await conversationSystem.getConversationInformation(conversationId)
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
        const result = await conversationSystem.addMessageToConversation(conversationId,"Test","");
        messageId = result.getMessageInfo().id;
        assert.succeeded(result);
        result.delete();
    }

    {
        const result = await conversationSystem.deleteMessage(messageId);
        
        assert.succeeded(result);
        result.delete();
    }

    connection.setConversationSystemCallback((p) => {
        if (ConversationDeleteMessageCallbackCalled)
            return;
        assert.areEqual(Multiplayer.ConversationMessageType.DeleteMessage, p.messageType);
        assert.areEqual(messageId, p.messageValue);

        ConversationDeleteMessageCallbackCalled = true;
    });

    {
        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        var array = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
        array.set(0,Multiplayer.ReplicatedValue.create_longValue(BigInt(Multiplayer.ConversationMessageType.DeleteMessage)));
        array.set(1,Multiplayer.ReplicatedValue.create_stringValue(messageId));
        await connection.sendNetworkEventToClient("ConversationSystem",array,connection.getClientId());
        array.delete()
    }

    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!ConversationDeleteMessageCallbackCalled && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(ConversationDeleteMessageCallbackCalled);
    {
        const result = await conversationSystem.deleteConversation(conversationId);
        assert.succeeded(result);
        result.delete();
    }
    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
   assert.isTrue(ok);
    
    enterResult.delete();
});

test('ConversationSystemTests', 'ConversationDeleteConversationCallbackTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';
    const testAssetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    // Log in
    const userId = await logIn(userSystem);
    const userProfile = await getProfileByUserId(userSystem, userId);
    const userDisplayName = userProfile.displayName;
    userProfile.delete();
        
    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);
    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    const connection  = enterResult.getConnection();
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup Asset callback
    let ConversationDeleteConversationeCallbackCalled = false;
    let conversationId = "";

    var conversationSystem = connection.getConversationSystem();
    {
        const result = await conversationSystem.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();
    }

    // Get Conversation Info
    {
        const result = await conversationSystem.getConversationInformation(conversationId)
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
        const result = await conversationSystem.addMessageToConversation(conversationId,"Test","");
        assert.succeeded(result);
        result.delete();
    }

    connection.setConversationSystemCallback((p) => {
        if (ConversationDeleteConversationeCallbackCalled)
            return;
        assert.areEqual(p.messageType, Multiplayer.ConversationMessageType.DeleteConversation);
        assert.areEqual(conversationId, p.messageValue);

        ConversationDeleteConversationeCallbackCalled = true;
    });

    {
        const result = await conversationSystem.deleteConversation(conversationId);
        assert.succeeded(result);
        result.delete();
    }

    {
        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        var array = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
        array.set(0,Multiplayer.ReplicatedValue.create_longValue(BigInt(Multiplayer.ConversationMessageType.DeleteConversation)));
        array.set(1,Multiplayer.ReplicatedValue.create_stringValue(conversationId));
        await connection.sendNetworkEventToClient("ConversationSystem",array,connection.getClientId());
        array.delete();
    }

    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!ConversationDeleteConversationeCallbackCalled && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(ConversationDeleteConversationeCallbackCalled);

    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
   assert.isTrue(ok);
    
    enterResult.delete();
});

test('ConversationSystemTests', 'ConversationUpdateInfoTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';
    const testAssetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    // Log in
    const userId = await logIn(userSystem);
    const userProfile = await getProfileByUserId(userSystem, userId);
    const userDisplayName = userProfile.displayName;
    userProfile.delete();
    
    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    const connection  = enterResult.getConnection();
    const entitySystem = connection.getSpaceEntitySystem();

    // Setup Asset callback
    let ConversationInformationCallbackCalled = false;
    let conversationId = "";

    var conversationSystem = connection.getConversationSystem();
    {
        const result = await conversationSystem.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();
    }

    connection.setConversationSystemCallback((p) => {
        if (ConversationInformationCallbackCalled)
            return;
        assert.areEqual(p.messageType, Multiplayer.ConversationMessageType.ConversationInformation);
        assert.areEqual(conversationId, p.messageValue);

        ConversationInformationCallbackCalled = true;
    });

    // Get Conversation Info
    {
        const result = await conversationSystem.getConversationInformation(conversationId)
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
        newData.resolved = true;
        newData.cameraPosition = cameraTranfromValue;
        newData.message = "TestMessage1";
        const result = await conversationSystem.setConversationInformation(conversationId,newData);
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

    {
        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        var array = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
        array.set(0,Multiplayer.ReplicatedValue.create_longValue(BigInt(Multiplayer.ConversationMessageType.ConversationInformation)));
        array.set(1,Multiplayer.ReplicatedValue.create_stringValue(conversationId));
        await connection.sendNetworkEventToClient("ConversationSystem",array,connection.getClientId());
        array.delete();
    }

    {
        const result = await conversationSystem.deleteConversation(conversationId);
        assert.succeeded(result);
        result.delete();
    }

    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!ConversationInformationCallbackCalled && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(ConversationInformationCallbackCalled);

    // Cleanup
   var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
   assert.isTrue(ok);
    
    enterResult.delete();
});

test('ConversationSystemTests', 'MessageUpdateInfoTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();

    const SpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';
    const testAssetCollectionName = generateUniqueString('OLY-TESTS-WASM-ASSETCOLLECTION');
    const testAssetName = generateUniqueString('OLY-TESTS-WASM-ASSET');

    // Log in
    const userId = await logIn(userSystem);
    const userProfile = await getProfileByUserId(userSystem, userId);
    const userDisplayName = userProfile.displayName;
    userProfile.delete();
    
    // Create space
    const space = await createSpace(spaceSystem, SpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const enterResult = await spaceSystem.enterSpace(space.id, true);
    assert.succeeded(enterResult);
    const connection  = enterResult.getConnection();
    const entitySystem = connection.getSpaceEntitySystem();

    entitySystem.setEntityCreatedCallback((e) => {});

    // Setup Asset callback
    let messageInformationCallbackCalled = false;
    let conversationId = "";
    let messageId = "";

    var conversationSystem = connection.getConversationSystem();
    {
        const result = await conversationSystem.createConversation("TestMessage")
        assert.succeeded(result);
        conversationId = result.getValue();
        result.delete();
    }

    // Get Conversation Info
    {
        const result = await conversationSystem.getConversationInformation(conversationId)
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
        const result = await conversationSystem.addMessageToConversation(conversationId,"test","test")
        assert.succeeded(result);
        messageId = result.getMessageInfo().id;
        result.delete();
    }

    connection.setConversationSystemCallback((p) => {
        if (messageInformationCallbackCalled)
            return;
        assert.areEqual(p.messageType, Multiplayer.ConversationMessageType.MessageInformation);
        assert.areEqual(messageId, p.messageValue);

        messageInformationCallbackCalled = true;
    });

    // Get Message Info
    {
        const result = await conversationSystem.getMessageInformation(messageId)
        assert.succeeded(result);
        assert.areEqual(result.getMessageInfo().edited,false)
        result.delete();
    }

    // Update Message Info
    {
        var newData = Multiplayer.MessageInfo.create();
        newData.message = "newTest";
        const result = await conversationSystem.setMessageInformation(messageId,newData);
        assert.succeeded(result);
        assert.areEqual(result.getMessageInfo().edited,true)
        assert.areEqual(result.getMessageInfo().message,newData.message)
        result.delete();
        newData.delete();
    }

    {
        // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
        var array = Common.Array.ofoly_multiplayer_ReplicatedValue_number(2);
        array.set(0,Multiplayer.ReplicatedValue.create_longValue(BigInt(Multiplayer.ConversationMessageType.MessageInformation)));
        array.set(1,Multiplayer.ReplicatedValue.create_stringValue(messageId));
        await connection.sendNetworkEventToClient("ConversationSystem",array,connection.getClientId());
        array.delete();
    }

    {
        const result = await conversationSystem.deleteConversation(conversationId);
        assert.succeeded(result);
        result.delete();
    }

    // Wait for message
    const start = Date.now();
    let current = Date.now();
    let testTime = 0;

    while(!messageInformationCallbackCalled && testTime < 20){
        await sleep(50);

        current = Date.now();
        testTime = (current - start) / 1000;
    }

    assert.isTrue(messageInformationCallbackCalled);

    // Cleanup
    var ok = await spaceSystem.exitSpaceAndDisconnect(connection);
    assert.isTrue(ok);
    enterResult.delete();
});