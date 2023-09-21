import { assert, pushCleanupFunction } from '../test_framework.js';
import { commonArrayToJSArray, jsArrayToCommonArray } from '../conversion_helpers.js';

import { Common, Systems } from '../connected_spaces_platform.js';


/**
 * 
 * @param {!Systems.SpaceSystem} spaceSystem 
 * @param {!Systems.Space} space 
 * @param {!boolean} [deleteFoundationResources] 
 * @returns {Promise<void>}
 */
export async function deleteSpace(spaceSystem, space, deleteFoundationResources = true) {
    const spaceId = space.id;
    const result = await spaceSystem.deleteSpace(space.id);
    
    assert.succeeded(result);

    result.delete();

    console.debug(`Space deleted (Id: ${spaceId})`);

    if (deleteFoundationResources)
        space.delete();
}


/**
 * 
 * @param {!Systems.SpaceSystem} spaceSystem 
 * @param {!string} name 
 * @param {!string} description 
 * @param {!Systems.SpaceAttributes} type 
 * @param {?Common.Map<string, string>} [metadata] 
 * @param {?Systems.InviteUserRoleInfoCollection} [inviteUsers]
 * @param {?Systems.FileAssetDataSource} [thumbnail] 
 * @param {!boolean} [pushCleanup] 
 * @param {!boolean} [deleteFoundationResources] 
 * @returns {Promise<!Systems.Space>} the created space
 */
export async function createSpace(spaceSystem, name, description, type, metadata = null, inviteUsers = null, thumbnail = null, pushCleanup = true, deleteFoundationResources = true) {
    let _metadata = metadata;

    if (_metadata === null) {
        _metadata = Common.Map.ofStringAndString();
        _metadata.set('site', 'Void');
    }

    const result = await spaceSystem.createSpace(name, description, type, inviteUsers, _metadata, thumbnail);
    
    assert.succeeded(result);

    const space = result.getSpace();
    result.delete();
    
    console.debug(`Space created (Id: ${space.id}, Name: ${space.name})`);

    if (pushCleanup) {
        pushCleanupFunction(async () => {
            await deleteSpace(spaceSystem, space, deleteFoundationResources);
        });
    }

    return space
}

/**
 * 
 * @param {!Systems.SpaceSystem} spaceSystem 
 * @param {!string[]} spaceIds
 * @returns {Promise<Systems.Space[]>} 
 */
export async function getSpacesByIds(spaceSystem, spaceIds) {
    let _spaceIds = jsArrayToCommonArray(spaceIds, String);

    const result = await spaceSystem.getSpacesByIds(_spaceIds);

    assert.succeeded(result);

    const spaces = result.getSpaces();
    result.delete();

    const _spaces = commonArrayToJSArray(spaces);
    spaces.delete();

    return _spaces;
}


/**
 * 
 * @param {!Systems.SpaceSystem} spaceSystem 
 * @param {!string} spaceId 
 * @returns {Promise<Systems.Space>}
 */
export async function getSpace(spaceSystem, spaceId) {
    const result = await spaceSystem.getSpace(spaceId);

    assert.succeeded(result);

    const space = result.getSpace();
    result.delete();

    return space;
}


/**
 * 
 * @param {!Systems.SpaceSystem} spaceSystem 
 * @param {!Systems.Space} space 
 * @param {?string} [newName] 
 * @param {?string} [newDescription] 
 * @param {?Systems.SpaceAttributes} [newType] 
 * @returns {Promise<Systems.BasicSpace>}
 */
export async function updateSpace(spaceSystem, space, newName = null, newDescription = null, newType = null) {
    const result = await spaceSystem.updateSpace(space.id, newName, newDescription, newType);
    
    assert.succeeded(result);

    const updatedSpace = result.getSpace();
    result.delete();

    return updatedSpace;
}

export function createInviteUsers() {
    const inviteUsers = Systems.InviteUserRoleInfoCollection.create();
    inviteUsers.emailLinkUrl = "https://dev.magnoverse.space";
    inviteUsers.inviteUserRoleInfos = Common.Array.ofcsp_systems_InviteUserRoleInfo_number(4);

    const inviteUser1 = Systems.InviteUserRoleInfo.create();
    inviteUser1.userEmail = "testnopus.pokemon+1@magnopus.com";
    inviteUser1.userRole = Systems.SpaceUserRole.User;
    inviteUsers.inviteUserRoleInfos.set(0, inviteUser1);
    inviteUser1.delete();

    const inviteUser2 = Systems.InviteUserRoleInfo.create();
    inviteUser2.userEmail = "testnopus.pokemon+2@magnopus.com";
    inviteUser2.userRole = Systems.SpaceUserRole.User;
    inviteUsers.inviteUserRoleInfos.set(1, inviteUser2);
    inviteUser2.delete();

    const modUser1 = Systems.InviteUserRoleInfo.create();
    modUser1.userEmail = "testnopus.pokemon+mod1@magnopus.com";
    modUser1.userRole = Systems.SpaceUserRole.Moderator;
    inviteUsers.inviteUserRoleInfos.set(2, modUser1);
    modUser1.delete();

    const modUser2 = Systems.InviteUserRoleInfo.create();
    modUser2.userEmail = "testnopus.pokemon+mod2@magnopus.com";
    modUser2.userRole = Systems.SpaceUserRole.Moderator;
    inviteUsers.inviteUserRoleInfos.set(3, modUser2);
    modUser2.delete();

    return inviteUsers;
}
