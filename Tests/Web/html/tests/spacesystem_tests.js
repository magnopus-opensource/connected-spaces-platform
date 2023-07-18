import { test, assert } from '../test_framework.js';
import { generateUniqueString } from '../test_helpers.js';
import { logIn, logInAsGuest, logOut, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, ALT_LOGIN_EMAIL, } from './usersystem_tests_helpers.js';
import { createSpace, getSpace, getSpacesByIds, updateSpace, createInviteUsers, deleteSpace } from './spacesystem_tests_helpers.js';

import { freeBuffer, uint8ArrayToBuffer, Systems, Common, Services, Web } from '../connected_spaces_platform.js';


test('SpaceSystemTests', 'CreateSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    assert.areEqual(space.name, spaceName);
});

test('SpaceSystemTests', 'CreateSpaceWithBulkInviteTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    const inviteUsers = createInviteUsers();

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, inviteUsers);

    assert.areEqual(space.name, spaceName);

    const pendingUserInvitesResult = await spaceSystem.getPendingUserInvites(space.id);
    assert.succeeded(pendingUserInvitesResult);

    const pendingUserInviteEmails = pendingUserInvitesResult.getPendingInvitesEmails();
    assert.areEqual(pendingUserInviteEmails.size(), inviteUsers.size());

    pendingUserInvitesResult.delete();
    inviteUsers.delete();

    for (let i = 0; i < pendingUserInviteEmails.size(); i++) {
        console.log('Pending space invite for email: ' + pendingUserInviteEmails.get(i));
    }
     
    pendingUserInviteEmails.delete();
});


test('SpaceSystemTests', 'UpdateSpaceDescriptionTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Update space description
    const updatedDescription = space.description + '-Updated';
    const updatedBasicSpace = await updateSpace(spaceSystem, space, null, updatedDescription);

    assert.areEqual(updatedBasicSpace.description, updatedDescription);

    updatedBasicSpace.delete();

    const updatedSpace = await getSpace(spaceSystem, space.id);

    assert.areEqual(updatedSpace.name, space.name);
    assert.areEqual(updatedSpace.description, updatedDescription);
    assert.areEqual(updatedSpace.attributes, space.attributes);

    updatedSpace.delete();
});


test('SpaceSystemTests', 'UpdateSpaceTypeTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Update space description
    const updatedType = Systems.SpaceAttributes.Public;
    const updatedBasicSpace = await updateSpace(spaceSystem, space, null, null, updatedType);

    assert.areEqual(updatedBasicSpace.attributes, updatedType);

    updatedBasicSpace.delete();

    const updatedSpace = await getSpace(spaceSystem, space.id);

    assert.areEqual(updatedSpace.name, space.name);
    assert.areEqual(updatedSpace.description, space.description);
    assert.areEqual(updatedSpace.attributes, updatedType);

    updatedSpace.delete();
});


test('SpaceSystemTests', 'GetSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Get space
    const retrievedSpace = await getSpace(spaceSystem, space.id);

    assert.areEqual(retrievedSpace.name, space.name);

    retrievedSpace.delete();
});


test('SpaceSystemTests', 'GetSpacesTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Get spaces
    {
        const result = await spaceSystem.getSpaces();

        assert.succeeded(result);

        const spaces = result.getSpaces();
        result.delete();

        assert.isGreaterThan(spaces.size(), 0);

        let found = false;

        for (let i = 0; i < spaces.size(); i++) {
            const space = spaces.get(i);
            const name = space.name;
            space.delete();

            if (name === spaceName) {
                found = true;

                break;
            }
        }

        assert.isTrue(found);
        
        spaces.delete();
    }
});


test('SpaceSystemTests', 'GetSpacesByIdsTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const publicSpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const privateSpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create spaces
    const publicSpace = await createSpace(spaceSystem, publicSpaceName, spaceDescription, Systems.SpaceAttributes.Public);
    const privateSpace = await createSpace(spaceSystem, privateSpaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const spaceIds = [publicSpace.id, privateSpace.id];

    const spaces = await getSpacesByIds(spaceSystem, spaceIds);

    assert.areEqual(spaces.length, 2);

    let privateSpaceFound = false;
    let publicSpaceFound = false;

    for (let space of spaces) {
        if (space.name === publicSpaceName)
            publicSpaceFound = true;
        else if (space.name === privateSpaceName)
            privateSpaceFound = true;
    }

    assert.isTrue(privateSpaceFound && publicSpaceFound);
});


test('SpaceSystemTests', 'UpdateSpaceThumbnailWithBuffer', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();
    
    const publicSpaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create spaces
    const space = await createSpace(spaceSystem, publicSpaceName, spaceDescription, Systems.SpaceAttributes.Public);

    {
        // Create thumbnail
        var thumbnail = Systems.BufferAssetDataSource.create();
        const LocalFilePath = "/assets/duck.gif";
        const FilePath = window.location.host + LocalFilePath;
        
        let response = await fetch("http://" + FilePath,{
            method: 'get',
        });

        // Receive blob
        let json = await response.blob();
        const arrayBuffer = await json.arrayBuffer();
        const rawData = new Uint8Array(arrayBuffer, 0, json.size);
        thumbnail.bufferLength = rawData.buffer.byteLength;
        console.log("New Thumbnail From File Data Length: " + String(thumbnail.bufferLength) + " bytes");

        // Convert uint8 to buffer
        thumbnail.buffer = uint8ArrayToBuffer(rawData);
        thumbnail.setMimeType("image/gif");

        // Update thumbnail
        const result = await spaceSystem.updateSpaceThumbnailWithBuffer(space.id, thumbnail);
        
        freeBuffer(thumbnail.buffer);
        thumbnail.delete();

        assert.succeeded(result);

        let spaceThumbnailUri = "";

        // Get space thumbnail
        {
            const result = await spaceSystem.getSpaceThumbnail(space.id);

            assert.succeeded(result);

            spaceThumbnailUri = result.getUri();
            result.delete();
        }
        
        // Create asset
        let testAsset = Systems.Asset.create();
        testAsset.uri = spaceThumbnailUri;

        // Download current space data
        {
            const result = await assetSystem.downloadAssetData(testAsset);

            assert.succeeded(result);

            // Check data lengths
            const dataLength =  result.getDataLength();
            result.delete();
            console.log("New Thumbnail From Space Data Length: " + String(dataLength) + " bytes");

            assert.areEqual(dataLength, rawData.buffer.byteLength);
        }

        testAsset.delete();
    }

    {
        // Upload second updated thumbnail
        var thumbnail = Systems.BufferAssetDataSource.create();
        const LocalFilePath = "/assets/OKO.png";
        const FilePath = window.location.host + LocalFilePath;

        let response = await fetch("http://" + FilePath,{
            method: 'get',
        });

        // Receive blob
        let json = await response.blob();
        const arrayBuffer = await json.arrayBuffer();
        const rawData = new Uint8Array(arrayBuffer, 0, json.size);
        thumbnail.bufferLength = rawData.buffer.byteLength;
        console.log("Second New Thumbnail From File Data Length: " + String(thumbnail.bufferLength) + " bytes");
        
        // Convert uint8 to buffer
        thumbnail.buffer = uint8ArrayToBuffer(rawData);
        thumbnail.setMimeType("image/png");

        // Update thumbnail
        {
            const result = await spaceSystem.updateSpaceThumbnailWithBuffer(space.id, thumbnail);

            assert.succeeded(result);

            freeBuffer(thumbnail.buffer);
            thumbnail.delete();
        }

        let newThumbnailUri = "";

        // Get space thumbnail
        {
            const result = await spaceSystem.getSpaceThumbnail(space.id);

            assert.succeeded(result);

            newThumbnailUri = result.getUri();
            result.delete();
        }

        // Create asset
        let testAsset = Systems.Asset.create();
        testAsset.uri = newThumbnailUri;

        // Download current space data
        {
            const result = await assetSystem.downloadAssetData(testAsset);

            assert.succeeded(result);

            // Check data lengths
            const dataLength =  result.getDataLength();
            result.delete();
            console.log("Second New Thumbnail From Space Data Length: " + String(dataLength) + " bytes");

            assert.areEqual(dataLength, rawData.buffer.byteLength);
        }

        testAsset.delete();
    }
});

test('SpaceSystemTests', 'GetSpacesMetadataTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);
    
    // Create space
    let metadata = Common.Map.ofStringAndString();
    metadata.set('site', 'MagOffice');

    const space1 = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, metadata);
    const space2 = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, metadata);

    var spaces = Common.Array.ofString_number(2);
    spaces.set(0, space1.id);
    spaces.set(1, space2.id);

    // Get space metadata
    {
        const result = await spaceSystem.getSpacesMetadata(spaces);

        assert.succeeded(result);

        const spacesMetadata = result.getMetadata();
        result.delete();

        assert.areEqual(spacesMetadata.size(), 2);

        const spaceMetadata1 = spacesMetadata.get(space1.id);
        const spaceMetadata2 = spacesMetadata.get(space2.id);
        spacesMetadata.delete();

        assert.areEqual(spaceMetadata1.size(), 1);
        assert.areEqual(spaceMetadata1.get('site'), 'MagOffice');
        assert.areEqual(spaceMetadata2.size(), 1);
        assert.areEqual(spaceMetadata2.get('site'), 'MagOffice');

        spaceMetadata1.delete();
        spaceMetadata2.delete();
    }
});

test('SpaceSystemTests', 'UpdateSpaceMetadataTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);
    
    // Create space
    let metadata = Common.Map.ofStringAndString();
    metadata.set('site', 'MagOffice');

    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, metadata);

     // update space metadata
     {
        const result = await spaceSystem.getSpaceMetadata(space.id);

        assert.areEqual(result.getMetadata().size(), 1);
        assert.areEqual(result.getMetadata().get('site'), 'MagOffice');
        
        metadata.set('tenant', 'OKO');

        await spaceSystem.updateSpaceMetadata(space.id,metadata)

        const resultUpdated = await spaceSystem.getSpaceMetadata(space.id);

        assert.areEqual(resultUpdated.getMetadata().size(), 2);
        assert.areEqual(resultUpdated.getMetadata().get('site'), 'MagOffice');
        assert.areEqual(resultUpdated.getMetadata().get('tenant'), 'OKO');
    }
});

test('SpaceSystemTests', 'EnterSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem,DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    {
        const result = await spaceSystem.enterSpace(space.id, true);
        assert.succeeded(result);
        const ok = await spaceSystem.exitSpaceAndDisconnect(result.getConnection());
        assert.isTrue(ok);
        result.delete()
    }

    await logOut(userSystem)

    // log in as guest to check we cannot enter
    await logIn(userSystem,ALT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    {
        const result = await spaceSystem.enterSpace(space.id, true);
        assert.areEqual(result.getResultCode(),Services.EResultCode.Failed)
        result.delete();
    }

    await logOut(userSystem)

    // Log in
    await logIn(userSystem);
    await spaceSystem.deleteSpace(space.id);
    space.delete();
});

test('SpaceSystemTests', 'BulkInviteToSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
    assert.areEqual(space.name, spaceName);

    const inviteUsers = createInviteUsers();

    const result = await spaceSystem.bulkInviteToSpace(space.id, inviteUsers);
    assert.succeeded(result);
    result.delete();

    const pendingUserInvitesResult = await spaceSystem.getPendingUserInvites(space.id);
    assert.succeeded(pendingUserInvitesResult);

    const pendingUserInviteEmails = pendingUserInvitesResult.getPendingInvitesEmails();
    assert.areEqual(pendingUserInviteEmails.size(), inviteUsers.size());

    pendingUserInvitesResult.delete();
    inviteUsers.delete();

    for (let i = 0; i < pendingUserInviteEmails.size(); i++) {
        console.log('Pending space invite for email: ' + pendingUserInviteEmails.get(i));
    }

    pendingUserInviteEmails.delete();
});

test('SpaceSystemTests', 'GeoLocationTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const initialLocation = Systems.GeoLocation.create();
    initialLocation.latitude = 1.1;
    initialLocation.longitude = 2.2;

    const initialOrientation = 90.0;

    const initialGeoFence = Common.Array.ofoly_systems_GeoLocation_number(4);
    const geoFence0 = Systems.GeoLocation.create();
    geoFence0.latitude = 5.5;
    geoFence0.longitude = 6.6;
    initialGeoFence.set(0, geoFence0);
    initialGeoFence.set(3, geoFence0);
    const geoFence1 = Systems.GeoLocation.create();
    geoFence1.latitude = 7.7;
    geoFence1.longitude = 8.8;
    initialGeoFence.set(1, geoFence1);
    const geoFence2 = Systems.GeoLocation.create();
    geoFence2.latitude = 9.9;
    geoFence2.longitude = 10.0;
    initialGeoFence.set(2, geoFence2);
    
    const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, initialLocation, initialOrientation, initialGeoFence);
    assert.succeeded(addGeoResult);
    assert.isTrue(addGeoResult.hasSpaceGeoLocation());
    assert.areApproximatelyEqual(addGeoResult.getSpaceGeoLocation().location.latitude, initialLocation.latitude);
    assert.areApproximatelyEqual(addGeoResult.getSpaceGeoLocation().location.longitude, initialLocation.longitude);
    assert.areApproximatelyEqual(addGeoResult.getSpaceGeoLocation().orientation, initialOrientation);
    assert.areEqual(addGeoResult.getSpaceGeoLocation().geoFence.size(), initialGeoFence.size());
    for (let i = 0; i < addGeoResult.getSpaceGeoLocation().geoFence.size(); i++)
    {
        assert.areApproximatelyEqual(addGeoResult.getSpaceGeoLocation().geoFence.get(i).latitude, initialGeoFence.get(i).latitude);
        assert.areApproximatelyEqual(addGeoResult.getSpaceGeoLocation().geoFence.get(i).longitude, initialGeoFence.get(i).longitude);
    }
    addGeoResult.delete();

    const getGeoResult = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.succeeded(getGeoResult);
    assert.isTrue(getGeoResult.hasSpaceGeoLocation());
    assert.areApproximatelyEqual(getGeoResult.getSpaceGeoLocation().location.latitude, initialLocation.latitude);
    assert.areApproximatelyEqual(getGeoResult.getSpaceGeoLocation().location.longitude, initialLocation.longitude);
    assert.areApproximatelyEqual(getGeoResult.getSpaceGeoLocation().orientation, initialOrientation);
    assert.areEqual(getGeoResult.getSpaceGeoLocation().geoFence.size(), initialGeoFence.size());
    for (let i = 0; i < getGeoResult.getSpaceGeoLocation().geoFence.size(); i++)
    {
        assert.areApproximatelyEqual(getGeoResult.getSpaceGeoLocation().geoFence.get(i).latitude, initialGeoFence.get(i).latitude);
        assert.areApproximatelyEqual(getGeoResult.getSpaceGeoLocation().geoFence.get(i).longitude, initialGeoFence.get(i).longitude);
    }
    getGeoResult.delete();

    const secondLocation = Systems.GeoLocation.create();
    secondLocation.latitude = 3.3;
    secondLocation.longitude = 4.4;

    const secondOrientation = 270.0;

    const secondGeoFence = Common.Array.ofoly_systems_GeoLocation_number(4);
    geoFence0.latitude = 11.1;
    geoFence0.longitude = 12.2;
    secondGeoFence.set(0, geoFence0);
    secondGeoFence.set(3, geoFence0);
    geoFence1.latitude = 13.3;
    geoFence1.longitude = 14.4;
    secondGeoFence.set(1, geoFence1);
    geoFence2.latitude = 15.5;
    geoFence2.longitude = 16.6;
    secondGeoFence.set(2, geoFence2);

    const updateGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, secondLocation, secondOrientation, secondGeoFence);
    assert.succeeded(updateGeoResult);
    assert.isTrue(updateGeoResult.hasSpaceGeoLocation());
    assert.areApproximatelyEqual(updateGeoResult.getSpaceGeoLocation().location.latitude, secondLocation.latitude);
    assert.areApproximatelyEqual(updateGeoResult.getSpaceGeoLocation().location.longitude, secondLocation.longitude);
    assert.areApproximatelyEqual(updateGeoResult.getSpaceGeoLocation().orientation, secondOrientation);
    assert.areEqual(updateGeoResult.getSpaceGeoLocation().geoFence.size(), secondGeoFence.size());
    for (let i = 0; i < updateGeoResult.getSpaceGeoLocation().geoFence.size(); i++)
    {
        assert.areApproximatelyEqual(updateGeoResult.getSpaceGeoLocation().geoFence.get(i).latitude, secondGeoFence.get(i).latitude);
        assert.areApproximatelyEqual(updateGeoResult.getSpaceGeoLocation().geoFence.get(i).longitude, secondGeoFence.get(i).longitude);
    }
    updateGeoResult.delete();

    const getUpdatedGeoResult = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.succeeded(getUpdatedGeoResult);
    assert.isTrue(getUpdatedGeoResult.hasSpaceGeoLocation());
    assert.areApproximatelyEqual(getUpdatedGeoResult.getSpaceGeoLocation().location.latitude, secondLocation.latitude);
    assert.areApproximatelyEqual(getUpdatedGeoResult.getSpaceGeoLocation().location.longitude, secondLocation.longitude);
    assert.areApproximatelyEqual(getUpdatedGeoResult.getSpaceGeoLocation().orientation, secondOrientation);
    assert.areEqual(getUpdatedGeoResult.getSpaceGeoLocation().geoFence.size(), secondGeoFence.size());
    for (let i = 0; i < getUpdatedGeoResult.getSpaceGeoLocation().geoFence.size(); i++)
    {
        assert.areApproximatelyEqual(getUpdatedGeoResult.getSpaceGeoLocation().geoFence.get(i).latitude, secondGeoFence.get(i).latitude);
        assert.areApproximatelyEqual(getUpdatedGeoResult.getSpaceGeoLocation().geoFence.get(i).longitude, secondGeoFence.get(i).longitude);
    }
    getUpdatedGeoResult.delete();

    const deleteGeoResult = await spaceSystem.deleteSpaceGeoLocation(space.id);
    assert.succeeded(deleteGeoResult);
    deleteGeoResult.delete();    

    const getDeletedGeoResult = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.succeeded(getDeletedGeoResult);
    assert.isFalse(getDeletedGeoResult.hasSpaceGeoLocation());
    getDeletedGeoResult.delete();

    initialLocation.delete();
    secondLocation.delete();
    initialGeoFence.delete();
    secondGeoFence.delete();
    geoFence0.delete();
    geoFence1.delete();
    geoFence2.delete();
});

test('SpaceSystemTests', 'GeoLocationValidationTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    const validLocation = Systems.GeoLocation.create();
    validLocation.latitude = 1.1;
    validLocation.longitude = 2.2;

    const invalidLocation = Systems.GeoLocation.create();
    invalidLocation.latitude = -500.0;
    invalidLocation.longitude = 2.2;

    const validOrientation = 90.0;
    const invalidOrientation = 400.0;

    const validGeoFence = Common.Array.ofoly_systems_GeoLocation_number(4);
    const shortGeoFence = Common.Array.ofoly_systems_GeoLocation_number(2);
    const invalidGeoFence = Common.Array.ofoly_systems_GeoLocation_number(4);
    const invalidGeoLocationGeoFence = Common.Array.ofoly_systems_GeoLocation_number(4);
    
    const geoFence0 = Systems.GeoLocation.create();
    geoFence0.latitude = 5.5;
    geoFence0.longitude = 6.6;
    const geoFence1 = Systems.GeoLocation.create();
    geoFence1.latitude = 7.7;
    geoFence1.longitude = 8.8;
    const geoFence2 = Systems.GeoLocation.create();
    geoFence2.latitude = 9.9;
    geoFence2.longitude = 10.0;
    
    validGeoFence.set(0, geoFence0);
    validGeoFence.set(1, geoFence1);
    validGeoFence.set(2, geoFence2);
    validGeoFence.set(3, geoFence0);

    shortGeoFence.set(0, geoFence0);
    shortGeoFence.set(1, geoFence1);

    invalidGeoFence.set(0, geoFence0);
    invalidGeoFence.set(1, geoFence1);
    invalidGeoFence.set(2, geoFence2);
    invalidGeoFence.set(3, geoFence2);

    invalidGeoLocationGeoFence.set(0, geoFence0);
    invalidGeoLocationGeoFence.set(1, geoFence1);
    invalidGeoLocationGeoFence.set(2, invalidLocation);
    invalidGeoLocationGeoFence.set(3, geoFence0);

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, invalidLocation, validOrientation, validGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, invalidOrientation, validGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, shortGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, invalidGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, invalidGeoLocationGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    // Actually add a geo location and test again since a different code path is followed when one exists
    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, validGeoFence);
        assert.succeeded(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, invalidLocation, validOrientation, validGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, invalidOrientation, validGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, shortGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, invalidGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const addGeoResult = await spaceSystem.updateSpaceGeoLocation(space.id, validLocation, validOrientation, invalidGeoLocationGeoFence);
        assert.failed(addGeoResult);
        addGeoResult.delete();
    }

    {
        const deleteGeoResult = await spaceSystem.deleteSpaceGeoLocation(space.id);
        assert.succeeded(deleteGeoResult);
        deleteGeoResult.delete();
    }


    validLocation.delete();
    invalidLocation.delete();
    validGeoFence.delete();
    shortGeoFence.delete();
    invalidGeoFence.delete();
    invalidGeoLocationGeoFence.delete();
    geoFence0.delete();
    geoFence1.delete();
    geoFence2.delete();
});

test('SpaceSystemTests', 'GeoLocationWithoutPermissionTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space as the primary user
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null, false);

    // Switch to the alt user to try and create the geo location
    await logOut(userSystem);
    await logIn(userSystem, ALT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    const initialLocation = Systems.GeoLocation.create();
    initialLocation.latitude = 1.1;
    initialLocation.longitude = 2.2;

    const initialOrientation = 90.0;
    
    const addGeoResultAsAlt = await spaceSystem.updateSpaceGeoLocation(space.id, initialLocation, initialOrientation, null);
    assert.areEqual(addGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(addGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    addGeoResultAsAlt.delete();

    // Switch back to the primary user to actually create the geo location
    await logOut(userSystem);
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    const addGeoResultAsPrimary = await spaceSystem.updateSpaceGeoLocation(space.id, initialLocation, initialOrientation, null);
    assert.succeeded(addGeoResultAsPrimary);
    addGeoResultAsPrimary.delete();

    // Switch to the alt user again
    await logOut(userSystem);
    await logIn(userSystem, ALT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Test they cannot access the geo location
    const getGeoResultAsAlt = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.areEqual(getGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(getGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    getGeoResultAsAlt.delete();

    // Test they cannot update the geo location
    const secondLocation = Systems.GeoLocation.create();
    secondLocation.latitude = 3.3;
    secondLocation.longitude = 4.4;

    const secondOrientation = 270.0;

    const updateGeoResultAsAlt = await spaceSystem.updateSpaceGeoLocation(space.id, secondLocation, secondOrientation, null);
    assert.areEqual(updateGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(updateGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    updateGeoResultAsAlt.delete();

    const deleteGeoResultAsAlt = await spaceSystem.deleteSpaceGeoLocation(space.id);
    assert.areEqual(deleteGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(deleteGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    deleteGeoResultAsAlt.delete();    

    // Switch back to the primary user to clean up
    await logOut(userSystem);
    await logIn(userSystem);

    const deleteGeoResultAsPrimary = await spaceSystem.deleteSpaceGeoLocation(space.id);
    assert.succeeded(deleteGeoResultAsPrimary);
    deleteGeoResultAsPrimary.delete();    

    const getDeletedGeoResultAsPrimary = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.succeeded(getDeletedGeoResultAsPrimary);
    assert.isFalse(getDeletedGeoResultAsPrimary.hasSpaceGeoLocation());
    getDeletedGeoResultAsPrimary.delete();

    initialLocation.delete();
    secondLocation.delete();

    await spaceSystem.deleteSpace(space.id);
    space.delete();
});

test('SpaceSystemTests', 'GeoLocationPublicSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    const spaceName = generateUniqueString('OLY-TESTS-WASM-SPACE');
    const spaceDescription = 'OLY-TESTS-WASM-SPACEDESC';

    // Log in as primary user
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Create space as the primary user
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Public, null, null, null, false);

    // Switch to the alt user to try and create the geo location
    await logOut(userSystem);
    await logIn(userSystem, ALT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    const initialLocation = Systems.GeoLocation.create();
    initialLocation.latitude = 1.1;
    initialLocation.longitude = 2.2;

    const initialOrientation = 90.0;
    
    const addGeoResultAsAlt = await spaceSystem.updateSpaceGeoLocation(space.id, initialLocation, initialOrientation, null);
    assert.areEqual(addGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(addGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    addGeoResultAsAlt.delete();

    // Switch back to the primary user to actually create the geo location
    await logOut(userSystem);
    await logIn(userSystem, DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    const addGeoResultAsPrimary = await spaceSystem.updateSpaceGeoLocation(space.id, initialLocation, initialOrientation, null);
    assert.succeeded(addGeoResultAsPrimary);
    addGeoResultAsPrimary.delete();

    // Switch to the alt user again
    await logOut(userSystem);
    await logIn(userSystem, ALT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, Services.EResultCode.Success, false);

    // Test they can access the geo location
    const getGeoResultAsAlt = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.succeeded(getGeoResultAsAlt);
    assert.isTrue(getGeoResultAsAlt.hasSpaceGeoLocation());
    assert.areApproximatelyEqual(getGeoResultAsAlt.getSpaceGeoLocation().location.latitude, initialLocation.latitude);
    assert.areApproximatelyEqual(getGeoResultAsAlt.getSpaceGeoLocation().location.longitude, initialLocation.longitude);
    assert.areApproximatelyEqual(getGeoResultAsAlt.getSpaceGeoLocation().orientation, initialOrientation);
    getGeoResultAsAlt.delete();

    // Test they cannot update the geo location
    const secondLocation = Systems.GeoLocation.create();
    secondLocation.latitude = 3.3;
    secondLocation.longitude = 4.4;

    const secondOrientation = 270.0;

    const updateGeoResultAsAlt = await spaceSystem.updateSpaceGeoLocation(space.id, secondLocation, secondOrientation, null);
    assert.areEqual(updateGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(updateGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    updateGeoResultAsAlt.delete();

    const deleteGeoResultAsAlt = await spaceSystem.deleteSpaceGeoLocation(space.id);
    assert.areEqual(deleteGeoResultAsAlt.getResultCode(), Services.EResultCode.Failed);
    assert.areEqual(deleteGeoResultAsAlt.getHttpResultCode(), Web.EResponseCodes.ResponseForbidden);
    deleteGeoResultAsAlt.delete();    

    // Switch back to the primary user to clean up
    await logOut(userSystem);
    await logIn(userSystem);

    const deleteGeoResultAsPrimary = await spaceSystem.deleteSpaceGeoLocation(space.id);
    assert.succeeded(deleteGeoResultAsPrimary);
    deleteGeoResultAsPrimary.delete();    

    const getDeletedGeoResultAsPrimary = await spaceSystem.getSpaceGeoLocation(space.id);
    assert.succeeded(getDeletedGeoResultAsPrimary);
    assert.isFalse(getDeletedGeoResultAsPrimary.hasSpaceGeoLocation());
    getDeletedGeoResultAsPrimary.delete();

    initialLocation.delete();
    secondLocation.delete();

    await spaceSystem.deleteSpace(space.id);
    space.delete();
});

