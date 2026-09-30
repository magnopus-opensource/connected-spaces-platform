import './pretend-to-be-a-browser'
import { CreatePublicTestSpace, CreateTestUser, LoginAsUser, LaunchTestPage, DeleteSpace, LogoutUser, TEST_ACCOUNT_PASSWORD } from './testhelpers'

import { suite } from 'uvu';
import * as assert from 'uvu/assert';
import { Common, CSPFoundation, ready, Systems } from 'connected-spaces-platform.web';
import { initializeCSP } from './shared/csp-initializer.js';

//Initialize CSPFoundation before the tests run
//True if USE_RELEASE_CSP is not set, false otherwise. Idea here is we want debug to be the default mode.
const USE_DEBUG_CSP: boolean = process.env.USE_RELEASE_CSP === undefined;

interface UserContext {
  user: Systems.ProfileResult;
}

const test = suite<UserContext>();

test.before.each(async (context) => {
  const user = await CreateTestUser();
  await LoginAsUser(user);

  context.user = user;
});

test.after.each(async (context) => {
  await LogoutUser(context.user)
});

test.before(async () => {
  const wafBypassEnv = process.env.MCS_X_WAF_BYPASS;
  const wafBypass = wafBypassEnv && wafBypassEnv.length > 0
  ? wafBypassEnv
  : undefined;
  
  return initializeCSP(USE_DEBUG_CSP, wafBypass); //gotta return the promise or tests wont automatically await
});

test('Login', async ({ user }) => {
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/Login.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully logged in')));
  assert.ok(errors.length == 0); //Should be no errors
})

test('EnterSpace', async () => {
  const spaceId = await CreatePublicTestSpace();
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/EnterSpace.html', USE_DEBUG_CSP, null, spaceId)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully entered space')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
})

test('Cross Thread Callbacks From Log Callback, OB-3782', async ({ user }) => {
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/CrossThreadLogCallbackLogin.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(
    !errors.some(e => e.message.includes('table index is out of bounds')));

});

test('Cross Thread Callbacks From NetworkInterrupted Callback, OB-1524', async ({ user }) => {
  const spaceId = await CreatePublicTestSpace();
  // This test relies on a 10s timeout for a C++ thread. Add that time to the default timeout.
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/CrossThreadConnectionInterrupted.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId, undefined, undefined, 15000)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(
    !errors.some(e => e.message.includes('table index is out of bounds')));

  assert.ok(consoleMessages.some(e => e.includes('Connection interrupted: true')));
});

test('SendReceiveNetworkEvent', async ({ user }) => {
  // This test was added as a regression test against `RuntimeError: null function or function signature mismatch`
  // Caused by a wrapper gen bug when you make a return type of an enclosing function different for the return type of the callback
  // We didn't actually fix it at time of writing, change `ListenNetworkEvent` to return a bool and you'll see what I mean.
  const spaceId = await CreatePublicTestSpace();

  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/SendReceiveNetworkEvent.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId);

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Received event: EventName')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
})

test('CreateAvatar', async ({ user }) => {
  const spaceId = await CreatePublicTestSpace();
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/CreateAvatar.html', USE_DEBUG_CSP, null, spaceId)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully created avatar')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
})

test('Offline', async () => {
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/Offline.html', USE_DEBUG_CSP, null, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Not starting a Multiplayer Connection')));
  assert.ok(consoleMessages.some(e => e.includes('Entering Offline Space')));
  assert.ok(consoleMessages.some(e => e.includes('Successfully entered space.')));
  assert.ok(consoleMessages.some(e => e.includes('Successfully created avatar')));
  assert.ok(consoleMessages.some(e => e.includes('Exiting Space Offline Space')));
  assert.ok(consoleMessages.some(e => e.includes('Multiplayer connection not connected when exiting space, skipping disconnect.')));
  assert.ok(errors.length == 0); //Should be no errors
})

test('EnterSpaceFromCheckpoint', async () => {
  // This test specifically also checks a stack overflow that occurred passing large data (the checkpoint json) over the ABI boundary.
  // The JSON in question is an anonymized, in-progress test one, so all the asset paths are nonsense. What matters here is that it's largish.
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/EnterSpaceFromCheckpoint.html', USE_DEBUG_CSP, null, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Not starting a Multiplayer Connection')));
  assert.ok(consoleMessages.some(e => e.includes('Entering Offline Space')));
  assert.ok(consoleMessages.some(e => e.includes('Successfully entered space.')));
  assert.ok(consoleMessages.some(e => e.includes('Successfully created avatar')));
  assert.ok(consoleMessages.some(e => e.includes('Exiting Space Offline Space')));
  assert.ok(consoleMessages.some(e => e.includes('Multiplayer connection not connected when exiting space, skipping disconnect.')));
  assert.ok(errors.length == 0); //Should be no errors
})

test('ReplicatedValueDoubleRetainsPrecision', () => {
  const value = 1 + Number.EPSILON;

  const doubleValue = Common.ReplicatedValue.create_doubleValue(value);
  const floatValue = Common.ReplicatedValue.create_floatValue(value);

  try {
    assert.is(doubleValue.getReplicatedValueType(), Common.ReplicatedValueType.Double);
    assert.is(doubleValue.getDouble(), value);

    assert.is(floatValue.getReplicatedValueType(), Common.ReplicatedValueType.Float);
    assert.is(floatValue.getFloat(), 1); // `value` is too precise for a float, so it rounds back to 1.
  } finally {
    doubleValue.delete();
    floatValue.delete();
  }
})

test.after(async () => {
  CSPFoundation.shutdown();
});


test('Failing status codes are retried', async ({ user }) => {

  const DEFAULT_NUM_REQUEST_RETRIES = 4; // This is just the number of retries CSP is supposed to do.
  let loginAttempts = 0;

  let loginAttemptInFlight = false;
  let overlappingLoginAttempts = 0;

  // See pretend-to-be-a-browser.ts, we use xhr2 to enable web requests in node, and can intercept requests by substituting our own implementation.
  const RealXMLHttpRequest = (globalThis as any).XMLHttpRequest;

  // Request that checks if it is a login request via URL inspection
  class LoginUnavailableXMLHttpRequest extends RealXMLHttpRequest {
    isLoginRequest = false;

    open(method: string, url: string, ...rest: any[]) {
      this.isLoginRequest = new URL(url).pathname.endsWith('/users/login');
      super.open(method, url, ...rest);
    }

    send(body?: any) {
      // If we're not a login request, just passthrough
      if (!this.isLoginRequest) {
        super.send(body);
        return;
      }

      // Otherwise, pretend that the service is unavailable to trigger retries
      this.readyState = RealXMLHttpRequest.DONE;
      this.status = 503;
      this.statusText = 'Service Unavailable';
      this.response = new ArrayBuffer(0);
      ++loginAttempts;

      // This is paranoid. I just want to assert that we don't send 4 retries all at once, but rather wait for the 
      // response before sending another. The web retry mechanism does not do delays currently.
      if (loginAttemptInFlight) {
        ++overlappingLoginAttempts;
      }
      loginAttemptInFlight = true;
      
      // This is effectively "resolving the promise", but we don't do it
      // here, because we'd deadlock if we did it "on-thread", remember we're
      // in an event loop. setTimeout isn't delaying anything, it's just moving something
      // into the queue so it's not on-thread.
      // This is what a real XHR does, apparently, can't say I fully understand. 
      setTimeout(() => {
        loginAttemptInFlight = false;
        this.onload?.({});
      });
    }
  }

  // The test framework logs in for each test, and we want to re-login.
  await LogoutUser(user);

  // Set the intercepting request to be the global request so it's used on the next login.
  (globalThis as any).XMLHttpRequest = LoginUnavailableXMLHttpRequest;

  // Perform the intercepted login
  const userSystem = Systems.SystemsManager.get().getUserSystem();
  let loginResult: Systems.LoginStateResult;
  try {
    loginResult = await userSystem.login(user.getProfile().email, TEST_ACCOUNT_PASSWORD, false, true, null);
  } finally {
    // Unset the intercepting request for any subsequent tests
    (globalThis as any).XMLHttpRequest = RealXMLHttpRequest;
  }

  try {
    assert.ok(loginAttempts > 0, 'Login request was never intercepted');
    assert.is(loginAttempts, 1 + DEFAULT_NUM_REQUEST_RETRIES, 'Expected the initial request plus additional retries');
    assert.is(loginResult.getResultCode(), Systems.EResultCode.Failed);
    assert.is(loginResult.getHttpResultCode(), 503);
    assert.is(overlappingLoginAttempts, 0, 'Expected each retry to be sent only after the previous attempt received its response');
  } finally {
    loginResult.delete();
  }
})

test('Network failures are retried', async ({ user }) => {

  const DEFAULT_NUM_REQUEST_RETRIES = 4; // This is just the number of retries CSP is supposed to do.
  let loginAttempts = 0;

  let loginAttemptInFlight = false;
  let overlappingLoginAttempts = 0;

  // See pretend-to-be-a-browser.ts, we use xhr2 to enable web requests in node, and can intercept requests by substituting our own implementation.
  const RealXMLHttpRequest = (globalThis as any).XMLHttpRequest;

  // Request that checks if it is a login request via URL inspection
  class LoginUnavailableXMLHttpRequest extends RealXMLHttpRequest {
    isLoginRequest = false;

    open(method: string, url: string, ...rest: any[]) {
      this.isLoginRequest = new URL(url).pathname.endsWith('/users/login');
      super.open(method, url, ...rest);
    }

    send(body?: any) {
      // If we're not a login request, just passthrough
      if (!this.isLoginRequest) {
        super.send(body);
        return;
      }

      // Pretend the request never got a response, which is how browsers report failures
      // no status and no body.
      this.readyState = RealXMLHttpRequest.DONE;
      this.status = 0;
      this.statusText = '';
      this.response = null;
      ++loginAttempts;

      // This is paranoid. I just want to assert that we don't send 4 retries all at once, but rather wait for the 
      // response before sending another. The web retry mechanism does not do delays currently.
      if (loginAttemptInFlight) {
        ++overlappingLoginAttempts;
      }
      loginAttemptInFlight = true;
      
      // This is effectively "resolving the promise", but we don't do it
      // here, because we'd deadlock if we did it "on-thread", remember we're
      // in an event loop. setTimeout isn't delaying anything, it's just moving something
      // into the queue so it's not on-thread.
      // This is what a real XHR does, apparently, can't say I fully understand. 
      setTimeout(() => {
        loginAttemptInFlight = false;
        this.onerror?.({});
      });
    }
  }

  // The test framework logs in for each test, and we want to re-login.
  await LogoutUser(user);

  // Set the intercepting request to be the global request so it's used on the next login.
  (globalThis as any).XMLHttpRequest = LoginUnavailableXMLHttpRequest;

  // Perform the intercepted login
  const userSystem = Systems.SystemsManager.get().getUserSystem();
  let loginResult: Systems.LoginStateResult;
  try {
    loginResult = await userSystem.login(user.getProfile().email, TEST_ACCOUNT_PASSWORD, false, true, null);
  } finally {
    // Unset the intercepting request for any subsequent tests
    (globalThis as any).XMLHttpRequest = RealXMLHttpRequest;
  }

  try {
    assert.ok(loginAttempts > 0, 'Login request was never intercepted');
    assert.is(loginAttempts, 1 + DEFAULT_NUM_REQUEST_RETRIES, 'Expected the initial request plus additional retries');
    assert.is(loginResult.getResultCode(), Systems.EResultCode.Failed);
    assert.is(loginResult.getHttpResultCode(), 0);
    assert.is(overlappingLoginAttempts, 0, 'Expected each retry to be sent only after the previous attempt received its response');
  } finally {
    loginResult.delete();
  }
})


test.run();

/*
 * Some ideas for further tests that would be good here
 * 
 * Test avatar creation
 * Test space creation and deletion
 * Create a SpaceEntity in a space
 * Add/Remove a component to a space entity
 * Execute a script
 */