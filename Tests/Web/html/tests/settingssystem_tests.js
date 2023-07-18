import { test, assert } from '../test_framework.js';
import { generateUniqueString } from '../test_helpers.js';
import { logIn } from './usersystem_tests_helpers.js';
import { createSpace } from './spacesystem_tests_helpers.js';

import { freeBuffer, Systems } from '../connected_spaces_platform.js';
import { createBufferAssetDataSource} from './assetsystem_tests_helpers.js'


test('SettingsSystemTests', 'BlockSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const settingsSystem = systemsManager.getSettingsSystem();

    const spaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';

    // Log in
    const userId = await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Clear blocked spaces
    {
        const result = await settingsSystem.clearBlockedSpaces(userId);

        assert.succeeded(result);

        result.delete();
    }

    // Block space
    {
        const result = await settingsSystem.addBlockedSpace(userId, space.id);

        assert.succeeded(result);

        result.delete();
    }

    // Get blocked spaces
    {
        const result = await settingsSystem.getBlockedSpaces(userId);

        assert.succeeded(result);

        const spaceIds = result.getValue();
        result.delete();

        assert.areEqual(spaceIds.size(), 1);

        const spaceId = spaceIds.get(0);
        spaceIds.delete();

        assert.areEqual(spaceId, space.id);
    }
});


test('SettingsSystemTests', 'UnBlockSpaceTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const settingsSystem = systemsManager.getSettingsSystem();

    const spaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';

    // Log in
    const userId = await logIn(userSystem);

    // Create space
    const space = await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);

    // Clear blocked spaces
    {
        const result = await settingsSystem.clearBlockedSpaces(userId);

        assert.succeeded(result);

        result.delete();
    }

    // Block space
    {
        const result = await settingsSystem.addBlockedSpace(userId, space.id);

        assert.succeeded(result);

        result.delete();
    }

    // Get blocked spaces
    {
        const result = await settingsSystem.getBlockedSpaces(userId);

        assert.succeeded(result);

        const spaceIds = result.getValue();
        result.delete();

        assert.areEqual(spaceIds.size(), 1);

        const spaceId = spaceIds.get(0);
        spaceIds.delete();

        assert.areEqual(spaceId, space.id);
    }

    // Unblock space
    {
        const result = await settingsSystem.removeBlockedSpace(userId, space.id);

        assert.succeeded(result);

        result.delete();
    }

    // Get blocked spaces
    {
        const result = await settingsSystem.getBlockedSpaces(userId);

        assert.succeeded(result);

        const spaceIds = result.getValue();
        result.delete();

        assert.areEqual(spaceIds.size(), 0);

        spaceIds.delete();
    }
});

test('SettingsSystemTests', 'UpdateAvatarPortraitWithBuffer', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const assetSystem = systemsManager.getAssetSystem();
    const settingsSystem = systemsManager.getSettingsSystem();


    // Log in
    var userID = await logIn(userSystem);

    {
        // Create Avatar Portrait
        var avatarPortrait = await createBufferAssetDataSource("/assets/OKO.png","image/png");

        const result = await settingsSystem.updateAvatarPortraitWithBuffer(userID, avatarPortrait);
        freeBuffer(avatarPortrait.buffer);

        assert.succeeded(result);

        let avatarPortraitUri = "";

        // Get Avatar Portrait
        {
            const result = await settingsSystem.getAvatarPortrait(userID);

            assert.succeeded(result);

            avatarPortraitUri = result.getUri();
            result.delete();
        }
        // Create asset
        let testAsset = Systems.Asset.create();
        testAsset.uri = avatarPortraitUri;

        // Download current Avatar Portrait data
        {
            const result = await assetSystem.downloadAssetData(testAsset);

            assert.succeeded(result);

            // Check data lengths
            const dataLength =  result.getDataLength();
            result.delete();
            console.log("New Thumbnail From Space Data Length: " + String(dataLength) + " bytes");

            assert.areEqual(dataLength,avatarPortrait.bufferLength);
        }
        avatarPortrait.delete();
        testAsset.delete();
    }
});