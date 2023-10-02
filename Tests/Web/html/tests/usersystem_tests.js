import { test, assert } from '../test_framework.js';
import { sleep, generateUniqueString, CHS_ENDPOINT_BASE_URI } from '../test_helpers.js';
import { jsArrayToCommonArray } from '../conversion_helpers.js';
import { DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD, getProfileByUserId, logIn, logOut } from './usersystem_tests_helpers.js';
import { createSpace } from './spacesystem_tests_helpers.js';

import { Services, Systems } from '../connected_spaces_platform.js';


const ThirdPartyAuthenticationDefines = 
{
    STATE_ID_URL_PARAM : "state=",
    CLIENTID_URL_PARAM : "client_id=",
    SCOPE_URL_PARAM : "scope=",
    REDIRECT_URL_PARAM : "redirect_uri=",
    INVALID_URL_PARAM_VALUE : "N/A"
}


function validateThirdPartyAuthoriseURL(authoriseURL, redirectURL)
{
    assert.areNotEqual(authoriseURL, "");
    assert.areNotEqual(authoriseURL, "error");

    const tokens = authoriseURL.split(/[&?]+/); // Regex looking for & and ? characters, multiple times 

    let stateId = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;
    let clientId = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;
    let scope = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;
    let retrievedRedirectURL = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;

    for (let i = 0; i < tokens.length; i++)
    {
        let URLElement = tokens[i];

        if(URLElement.startsWith(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM))
            stateId = URLElement.substring(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM.length);
        else if (URLElement.startsWith(ThirdPartyAuthenticationDefines.CLIENTID_URL_PARAM))
            clientId = URLElement.substring(ThirdPartyAuthenticationDefines.CLIENTID_URL_PARAM.length);
        else if(URLElement.startsWith(ThirdPartyAuthenticationDefines.SCOPE_URL_PARAM))
            scope = URLElement.substring(ThirdPartyAuthenticationDefines.SCOPE_URL_PARAM.length);
        else if(URLElement.startsWith(ThirdPartyAuthenticationDefines.REDIRECT_URL_PARAM))
            retrievedRedirectURL = URLElement.substring(ThirdPartyAuthenticationDefines.REDIRECT_URL_PARAM.length);
    }

    // Validate that the following contain something that potentially makes sense
    assert.areNotEqual(stateId, ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE);
    assert.areNotEqual(clientId, ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE);
    assert.areNotEqual(scope, ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE);
    assert.areNotEqual(retrievedRedirectURL, ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE);

    assert.isGreaterThan(stateId.length, 10);
    assert.isGreaterThan(clientId.length, 10);
    assert.isGreaterOrEqualThan(scope.length, 5);
    assert.areEqual(retrievedRedirectURL, redirectURL);
}


async function getFullProfileByUserId(userSystem, userId)
{
    const result = await userSystem.getProfileByUserId(userId);
    
    assert.succeeded(result);

    const profile = result.getProfile();
    result.delete();

    return profile;
}


test('UserSystemTests', 'LoginTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    await logIn(userSystem);
});

test('UserSystemTests', 'LoginTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    await logIn(userSystem);
});

test('UserSystemTests', 'FalseAgeVerificationLoginTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in with false age verification
    {
        const result = await userSystem.login('', DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD,false);
    
        assert.failed(result);

        result.delete();
    }

    // Log in with true age verification
    {
        const result = await userSystem.login('', DEFAULT_LOGIN_EMAIL, DEFAULT_LOGIN_PASSWORD,true);
    
        assert.succeeded(result);

        result.delete();
    }

});


test('UserSystemTests', 'LoginWithTokenTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();

    let loginToken = "";
    let loginTokenAvailable = false;

    userSystem.setNewLoginTokenReceivedCallback((result) => {
        const resCode = result.getResultCode();

        assert.succeeded(result);

        if (resCode === Services.EResultCode.Success) {
            const tokenInfo = result.getLoginTokenInfo();
            const tokenExpiry = tokenInfo.refreshExpiryTime;
            loginToken = tokenInfo.refreshToken;
            tokenInfo.delete();

            loginTokenAvailable = true;

            console.debug(`New login token ${loginToken} expires at ${tokenExpiry}`);
        }

        result.delete();
    });

    // Log in
    const userId = await logIn(userSystem);

    let waitForTestTimeoutCountMs = 0;
    const WAIT_FOR_TEST_TIMEOUT_LIMIT_MS = 1000;

    while (!loginTokenAvailable && (waitForTestTimeoutCountMs < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS)) {
        await sleep(50);
        waitForTestTimeoutCountMs += 50;
    }

    loginTokenAvailable = false;
    waitForTestTimeoutCountMs = 0;

    // Log in with refresh token
    {
        const result = await userSystem.loginWithToken(userId, loginToken);
        
        assert.succeeded(result);

        result.delete();
    }

    while (!loginTokenAvailable && waitForTestTimeoutCountMs < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS) {
        await sleep(50);
        waitForTestTimeoutCountMs += 50;
    }
    
    const currentLoginState = userSystem.getLoginState();
    
    assert.areEqual(currentLoginState.state, Systems.ELoginState.LoggedIn);
    
    currentLoginState.delete();

    // Create space to verify that we successfully logged in
    const spaceName = generateUniqueString('CSP-TESTS-WASM-SPACE');
    const spaceDescription = 'CSP-TESTS-WASM-SPACEDESC';

    await createSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private);
});


test('UserSystemTests', 'ExchangeKeyTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let response = await fetch(CHS_ENDPOINT_BASE_URI + '/mag-user/api/v1/users/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            email: DEFAULT_LOGIN_EMAIL,
            password: DEFAULT_LOGIN_PASSWORD,
            deviceId: 'GoodbyeAndThanksForAllTheFish'
        })
    });

    let json = await response.json();
    const userId = json.userId;
    const accessToken = json.accessToken;

    console.debug(`Logged in via fetch (UserId: ${userId})`);

    response = await fetch(CHS_ENDPOINT_BASE_URI + '/mag-user/api/v1/users/onetimekey', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${accessToken}`
        },
        body: JSON.stringify({
            userId: userId,
            deviceId: 'GoodbyeAndThanksForAllTheFish'
        })
    });

    json = await response.json();
    const oneTimeKey = json.oneTimeKey;

    console.debug(`Got OTK via fetch (${oneTimeKey})`);

    // Log in with OTP
    {
        const result = await userSystem.exchangeKey(userId, oneTimeKey);
        
        assert.succeeded(result);

        const loginState = result.getLoginState();
        result.delete();

        const receivedUserId = loginState.userId;
        loginState.delete();

        assert.areEqual(receivedUserId, userId);

        console.debug(`Exchanged key (UserId: ${receivedUserId})`);
    }

    // Log out
    await logOut(userSystem);
});


test('UserSystemTests', 'LoginErrorTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in with invalid credentials
    await logIn(userSystem, 'invalidlogin@magnopus.com', 'notarealpassword', Services.EResultCode.Failed);

    // Log in
    await logIn(userSystem);
});


test('UserSystemTests', 'UpdateDisplayNameTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    const userId = await logIn(userSystem);
    const newDisplayName = generateUniqueString('CSP-TESTS-WASM-NAME').substring(0, 24);

    // Update display name
    {
        const result = await userSystem.updateUserDisplayName(userId, newDisplayName);
        
        assert.succeeded(result);

        result.delete();
    }

    // Retrieve user profile and verify new display name
    const profile = await getProfileByUserId(userSystem, userId);

    assert.areEqual(profile.userId, userId);
    assert.areEqual(profile.displayName, newDisplayName);

    profile.delete();
});


test('UserSystemTests', 'UpdateDisplayNameTestWithSpaces', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    const userId = await logIn(userSystem);
    const newDisplayName = generateUniqueString('OLY TESTS WASM NAME WITH SPACES').substring(0, 24);

    // Update display name
    {
        const result = await userSystem.updateUserDisplayName(userId, newDisplayName);
        
        assert.succeeded(result);

        result.delete();
    }

    // Retrieve user profile and verify new display name
    const profile = await getProfileByUserId(userSystem, userId);

    assert.areEqual(profile.userId, userId);
    assert.areEqual(profile.displayName, newDisplayName);

    profile.delete();
});


test('UserSystemTests', 'UpdateDisplayNameTestWithSymbols', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    const userId = await logIn(userSystem);
    const newDisplayName = generateUniqueString('NAME SYMBOLS *&^%%$@~#!;?.,{}[]()=').substring(0, 24);

    // Update display name
    {
        const result = await userSystem.updateUserDisplayName(userId, newDisplayName);
        
        assert.succeeded(result);

        result.delete();
    }

    // Retrieve user profile and verify new display name
    const profile = await getProfileByUserId(userSystem, userId);

    assert.areEqual(profile.userId, userId);
    assert.areEqual(profile.displayName, newDisplayName);

    profile.delete();
});

test('UserSystemTests', 'UpdateDisplayNameTestWithTooLongName', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    const userId = await logIn(userSystem);

    const newDisplayName = generateUniqueString('OLY WASM{}[()= LONG NAME MORE THAN 24 CHARACTERS');

    // Update display name
    {
        const result = await userSystem.updateUserDisplayName(userId, newDisplayName);
        
        assert.failed(result);

        result.delete();
    }

    // Retrieve user profile and verify new display name
    const profile = await getProfileByUserId(userSystem, userId);

    profile.delete();
});

test('UserSystemTests', 'PingTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Ping UserSystem CHS services
    const result = await userSystem.ping();   
    
    assert.succeeded(result);

    result.delete();
});


test('UserSystemTests', 'CreateUserTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const settingsSystem = systemsManager.getSettingsSystem();

    const newUsername = generateUniqueString("OLY-TEST-NAME");
    const newDisplayName = "OLY-TEST-DISPLAY";
    const newEmail = generateUniqueString('testnopus.') + "@magnopus.com";
    const newPassword = generateUniqueString('Password');

    let createdUserId;
    
    // Test user creation
    {
        const result = await userSystem.createUser(newUsername, newDisplayName, newEmail, newPassword, true, null, null);
        
        assert.succeeded(result);

        const profile = result.getProfile();
        result.delete();
        
        createdUserId = profile.userId;

        console.debug(`Created user (UserId: ${profile.userId})`);

        assert.areEqual(profile.userName, newUsername);
        assert.areEqual(profile.displayName, newDisplayName);
        assert.areEqual(profile.email, newEmail);

        profile.delete();
    }

    // Log in
    await logIn(userSystem);

    // Verify that newsletter preference was set
    {
        const result = await settingsSystem.getNewsletterStatus(createdUserId);

        assert.succeeded(result);
        assert.isTrue(result.getValue());
    }

    // Test lite profile
    {
        let userIds = [ createdUserId ];
        let _userIds = jsArrayToCommonArray(userIds, String);
        const result = await userSystem.getProfilesByUserId(_userIds);

        _userIds.delete();

        assert.succeeded(result);

        const profiles = result.getProfiles();
        result.delete();

        assert.areEqual(profiles.size(), 1);

        const profile = profiles.get(0);
        profiles.delete();

        assert.areEqual(profile.userId, createdUserId);
        assert.areEqual(profile.displayName, newDisplayName);

        profile.delete();
    }

    // Test full profile
    {
        const result = await userSystem.getProfileByUserId(createdUserId);

        assert.succeeded(result);

        const profile = result.getProfile();
        result.delete();

        assert.areEqual(profile.userId, createdUserId);
        assert.areEqual(profile.userName, newUsername);
        assert.areEqual(profile.displayName, newDisplayName);
        assert.areEqual(profile.email, newEmail);

        profile.delete();
    }

    // Cleanup
    await userSystem.deleteUser(createdUserId);

    console.debug(`Deleted user (UserId: ${createdUserId})`);
});


test('UserSystemTests', 'GetAuthoriseURLForGoogleTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let redirectURL = "https://odev.magnoverse.space/oauth";
    
    // Retrieve Authorise URL for Google
    const result = await userSystem.getThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Google, redirectURL);

    assert.succeeded(result);

    let authoriseURL = result.getValue();
    result.delete();

    validateThirdPartyAuthoriseURL(authoriseURL, redirectURL);
});


test('UserSystemTests', 'GetAuthoriseURLForDiscordTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let redirectURL = "https://odev.magnoverse.space/oauth";
    
    // Retrieve Authorise URL for Discord
    const result = await userSystem.getThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Discord, redirectURL);

    assert.succeeded(result);

    let authoriseURL = result.getValue();
    result.delete();

    validateThirdPartyAuthoriseURL(authoriseURL, redirectURL);
});

test('UserSystemTests', 'GetAuthoriseURLForAppleTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let redirectURL = "https://odev.magnoverse.space/oauth";
    
    // Retrieve Authorise URL for Discord
    const result = await userSystem.getThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Apple, redirectURL);

    assert.succeeded(result);

    let authoriseURL = result.getValue();
    result.delete();

    validateThirdPartyAuthoriseURL(authoriseURL, redirectURL);
});
// As the following two tests require manual actions explained inside, they are currently disabled
// only these WASM tests would be able to have a end-to-end testing flow using Selenium for the URL redirects. 
// The Selenium work involved is part of a separate ticket
/*

test('UserSystemTests', 'GoogleLogInTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let redirectURL = "https://odev.magnoverse.space/oauth";
    
    // Retrieve Authorise URL for Google
    {
        const result = await userSystem.getThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Google, redirectURL);
        
        assert.succeeded(result);

        let authoriseURL = result.getValue();
        result.delete();

        let tokens = authoriseURL.split("&");
        let stateId = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;

        for (let i = 0; i < tokens.length; i++)
        {
            let URLElement = tokens[i];

            if(URLElement.startsWith(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM))
            {
                stateId = URLElement.substring(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM.length);
                break;
            }
        }

        console.log("AuthoriseURL: ", authoriseURL);
    }

    // 1. Set a breakpoint on the next line before reading from the file
    // 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
    // 3. Get the "code" param value from the response URL 
    // 4. Drop the token it in the file below
    // 5. Due to the fact that the browser caches the file content you will have to "Empty cache and and hard refresh" in Chrome now by right click-ing on the Refresh button and select the option.
    // 6. Rerun the same test and when it gets again here to read the file content your new token will be ready for use!

    const LocalFilePath = "/assets/third_party_auth_token.txt";
    const FilePath = window.location.host + LocalFilePath;
    let response = await fetch("http://" + FilePath,{
        method: 'get',
    });

    let googleToken = await response.text();

    {
        const result = await userSystem.loginToThirdPartyAuthenticationProvider(googleToken, stateId);

        assert.succeeded(result);

        const loginState = result.getLoginState();
        result.delete();

        const userId = loginState.userId;
        loginState.delete();

        const fullProfile = await getFullProfileByUserId(userSystem, userId);
        console.log(fullProfile);

        fullProfile.delete();
    }
});


test('UserSystemTests', 'DiscordLogInTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let redirectURL = "https://odev.magnoverse.space/oauth";

    // Retrieve Authorise URL for Discord
    {
        const result = await userSystem.getThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Discord, redirectURL);

        assert.succeeded(result);

        let authoriseURL = result.getValue();
        result.delete();

        let tokens = authoriseURL.split("&");
        let stateId = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;

        for (let i = 0; i < tokens.length; i++)
        {
            let URLElement = tokens[i];

            if(URLElement.startsWith(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM))
            {
                stateId = URLElement.substring(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM.length);
                break;
            }
        }

        console.log("AuthoriseURL: ", authoriseURL);
    }

    // 1. Set a breakpoint on the next line before reading from the file
    // 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
    // 3. Get the "code" param value from the response URL 
    // 4. Drop the token it in the file below
    // 5. Due to the fact that the browser caches the file content you will have to "Empty cache and and hard refresh" in Chrome now by right click-ing on the Refresh button and select the option.
    // 6. Rerun the same test and when it gets again here to read the file content your new token will be ready for use!
    
    const LocalFilePath = "/assets/third_party_auth_token.txt";
    const FilePath = window.location.host + LocalFilePath;
    let response = await fetch("http://" + FilePath,{
        method: 'get',
    });

    let discordToken = await response.text();

    {
        const result = await userSystem.loginToThirdPartyAuthenticationProvider(discordToken, stateId);
        assert.succeeded(result);

        const loginState = result.getLoginState();
        result.delete();

        const userId = loginState.userId;
        loginState.delete();

        const fullProfile = await getFullProfileByUserId(userSystem, userId);
        console.log(fullProfile);

        fullProfile.delete();
    }
});

test('UserSystemTests', 'AppleLogInTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    let redirectURL = "https://example-app.com/redirect";
    let stateId = ThirdPartyAuthenticationDefines.INVALID_URL_PARAM_VALUE;

    // Retrieve Authorise URL for Discord
    {
        const result = await userSystem.getThirdPartyProviderAuthoriseURL(Systems.EThirdPartyAuthenticationProviders.Apple, redirectURL);
        console.log(result.getResponseBody());
        assert.succeeded(result);

        let authoriseURL = result.getValue();
        result.delete();

        let tokens = authoriseURL.split("&");

        for (let i = 0; i < tokens.length; i++)
        {
            let URLElement = tokens[i];

            if(URLElement.startsWith(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM))
            {
                stateId = URLElement.substring(ThirdPartyAuthenticationDefines.STATE_ID_URL_PARAM.length);
                break;
            }
        }

        console.log("AuthoriseURL: ", authoriseURL);
    }

    // 1. Set a breakpoint on the next line before reading from the file
    // 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
    // 3. Get the "code" param value from the response URL 
    // 4. Drop the token it in the file below
    // 5. Due to the fact that the browser caches the file content you will have to "Empty cache and and hard refresh" in Chrome now by right click-ing on the Refresh button and select the option.
    // 6. Rerun the same test and when it gets again here to read the file content your new token will be ready for use!
    
    const LocalFilePath = "/assets/third_party_auth_token.txt";
    const FilePath = window.location.host + LocalFilePath;
    let response = await fetch("http://" + FilePath,{
        method: 'get',
    });

    let AppleToken = await response.text();

    {
        const result = await userSystem.loginToThirdPartyAuthenticationProvider(AppleToken, stateId);
        assert.succeeded(result);

        const loginState = result.getLoginState();
        result.delete();

        const userId = loginState.userId;
        loginState.delete();

        const fullProfile = await getFullProfileByUserId(userSystem, userId);
        console.log(fullProfile);

        fullProfile.delete();
    }
});

// Disabled by default as it requires superuser access to delete the account

test('UserSystemTests', 'CreateUserEmptyUsernameDisplaynameTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    const newEmail = generateUniqueString('testnopus.') + "@magnopus.com";
    const newPassword = generateUniqueString('Password');

    let userId = "";
    
    // Test user creation
    {
        const result = await userSystem.createUser(null, null, newEmail, newPassword, false, null);
        
        assert.succeeded(result);

        const profile = result.getProfile();
        result.delete();

        userId = profile.userId;

        assert.isTrue(profile.userName.length === 0);
        assert.isFalse(profile.displayName.length === 0);
        assert.areEqual(profile.email, newEmail);

        profile.delete();
    }

    // Log in
    await logIn(userSystem);

    // Test lite profile
    {
        const userIds = [ userId ];
        const _userIds = jsArrayToCommonArray(userIds, String);

        const result = await userSystem.getProfilesByUserId(_userIds);
        _userIds.delete();

        assert.succeeded(result);

        const profiles = result.getProfiles();
        result.delete();

        const profile = profiles.get(0);

        assert.areEqual(profile.userId, userId);
        assert.isFalse(profile.displayName.length === 0);

        profile.delete();
    }

    // Test full profile
    {
        const result = await userSystem.getProfileByUserId(userId);

        assert.succeeded(result);

        const profile = result.getProfile();

        assert.areEqual(profile.userId, userId);

        assert.isTrue(profile.userName.length === 0)
        assert.isFalse(profile.displayName.length === 0);
        assert.areEqual(profile.email, newEmail);

        profile.delete();
    }

    // Cleanup
    await userSystem.deleteUser(userId);
});
*/

test('UserSystemTests', 'GetAgoraUserTokenTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    // Log in
    await logIn(userSystem);

    /*
	  Setup token params 
	  For this test, it's exeptable to use 0 for the UserId and ChannelId
	  This is because the endpoint is just using an algorithm to generate the token
	  So no valid Ids are needed for verification
	*/
    const params = Systems.AgoraUserTokenParams.create();
    params.agoraUserId = "0";
    params.channelName = "0";
    params.lifespan = 10000;
    params.shareAudio = true;
    params.shareScreen = false;
    params.shareVideo = false;
    params.readOnly = false;

    // Get token
    const result = await userSystem.getAgoraUserToken(params);

    assert.succeeded(result);
    assert.areNotEqual(result.getUserToken(), "");
});

test('UserSystemTests', 'GetGuestProfileTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();

    const id = await logIn(userSystem);

    // Uncomment below to run this as a test of the access token refresh
    // await new Promise(r => setTimeout(r, 40 * 60 * 1000));

    getFullProfileByUserId(userSystem, id);
});