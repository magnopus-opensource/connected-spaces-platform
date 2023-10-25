import { assert, pushCleanupFunction } from '../test_framework.js';

import { Services, Systems } from '../connected_spaces_platform.js';

export var DEFAULT_LOGIN_EMAIL;
export var ALT_LOGIN_EMAIL;
export var DEFAULT_LOGIN_PASSWORD;
export var ALT_LOGIN_PASSWORD;

fetch("/assets/test_account_creds.txt")
  .then((res) => res.text())
  .then((text) => {
    const Accounts = text.split(/\s+/)
    if(Accounts.length == 4){
        DEFAULT_LOGIN_EMAIL = Accounts[0];
        DEFAULT_LOGIN_PASSWORD = Accounts[1];
        ALT_LOGIN_EMAIL = Accounts[2];
        ALT_LOGIN_PASSWORD = Accounts[3];
    }else{
        throw "test_account_creds.txt formatted incorrectly. This file must contain the following information:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>";
    }

   })
  .catch((e) => console.error("test_account_creds.txt not found! This file must exist and must contain the following information:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>"));

/**
 * 
 * @param {!Systems.UserSystem} userSystem 
 * @returns {Promise<void>}
 */
export async function logOut(userSystem) {
    const result = await userSystem.logout();
    
    assert.succeeded(result);

    result.delete();

    console.debug('Logged out');
}


/**
 * 
 * @param {!Systems.UserSystem} userSystem 
 * @param {!string} [email] 
 * @param {!string} [password] 
 * @param {!boolean} [ageVerified] 
 * @param {!Services.EResultCode} [expectedResult]
 * @param {!Systems.ELoginStateResultFailureReason} [expectedFailureResultCode]
 * @param {!boolean} [pushCleanup]
 * @returns {Promise<?string>} the userId of the logged in account
 */
export async function logIn(userSystem, email = DEFAULT_LOGIN_EMAIL, password = DEFAULT_LOGIN_PASSWORD, ageVerified = true, expectedResult = Services.EResultCode.Success, expectedFailureResultCode = Systems.ELoginStateResultFailureReason.None, pushCleanup = true) {

    const result = await userSystem.login('', email, password, ageVerified);
    const resCode = result.getResultCode();

    assert.succeeded(result, expectedResult);

    assert.areEqual(result.getFailureReason(), expectedFailureResultCode);

    const loginState = result.getLoginState();
    result.delete();

    const userId = loginState.userId;
    loginState.delete();

    if (resCode === Services.EResultCode.Success) {
        if (pushCleanup) {
            pushCleanupFunction(async () => {
                await logOut(userSystem);
            });
        }

        console.debug(`Logged in (UserId: ${userId})`);

        return userId;
    }

    return null;
}


/**
 * 
 * @param {!Systems.UserSystem} userSystem
 * @param {!Services.EResultCode} [expectedResult] 
 * @param {!boolean} [pushCleanup] 
 * @returns {Promise<?string>} the guest userId of the logged in account
 */
 export async function logInAsGuest(userSystem, expectedResult = Services.EResultCode.Success, pushCleanup = true) {
    const result = await userSystem.loginAsGuest(true);
    const resCode = result.getResultCode();

    assert.succeeded(result, expectedResult);

    const loginState = result.getLoginState();
    result.delete();

    const userId = loginState.userId;
    loginState.delete();

    if (resCode === Services.EResultCode.Success) {
        if (pushCleanup) {
            pushCleanupFunction(async () => {
                await logOut(userSystem);
            });
        }

        console.debug(`Logged in as guest (UserId: ${userId})`);

        return userId;
    }

    return null;
}


/**
 * 
 * @param {!Systems.UserSystem} userSystem 
 * @param {!string} userId 
 * @returns {Promise<Systems.Profile>}
 */
export async function getProfileByUserId(userSystem, userId) {
    const result = await userSystem.getProfileByUserId(userId);
    
    assert.succeeded(result);

    const profile = result.getProfile();
    result.delete();

    return profile;
}