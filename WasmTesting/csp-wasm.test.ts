import './pretend-to-be-a-browser'
import  {CreatePublicTestSpace, CreateTestUser, LoginAsUser, LaunchTestPage, DeleteSpace, LogoutUser, TEST_ACCOUNT_PASSWORD} from './testhelpers'

import { test } from 'uvu';
import * as assert from 'uvu/assert';
import { CSPFoundation, ready, Systems } from 'connected-spaces-platform.web';
import { initializeCSP } from './shared/csp-initializer.js';

//Initialize CSPFoundation before the tests run
//True if USE_RELEASE_CSP is not set, false otherwise. Idea here is we want debug to be the default mode.
const USE_DEBUG_CSP: boolean = process.env.USE_RELEASE_CSP === undefined;

test.before(async () => {
  return initializeCSP(USE_DEBUG_CSP); //gotta return the promise or tests wont automatically await
});

test('Login', async() => {
  const user = await CreateTestUser();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/Login.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully logged in')));
  assert.ok(errors.length == 0); //Should be no errors
})

test('EnterSpace', async() => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/EnterSpace.html', USE_DEBUG_CSP, null, spaceId)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully entered space')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
})

test('Cross Thread Callbacks From Log Callback, OB-3782', async () => {
  const user = await CreateTestUser();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/CrossThreadLogCallbackLogin.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(
    errors.some(e => e.message.includes('table index is out of bounds')),
    'Expected cross-thread `table index is out of bounds` error. Message not found, did you fix the bug (OB-3782)? Nice job! ',
  );
  
});

test('SendReceiveNetworkEvent', async() => {
  // This test was added as a regression test against `RuntimeError: null function or function signature mismatch`
  // Caused by a wrapper gen bug when you make a return type of an enclosing function different for the return type of the callback
  // We didn't actually fix it at time of writing, change `ListenNetworkEvent` to return a bool and you'll see what I mean.
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();

  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/SendReceiveNetworkEvent.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId);

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Received event: EventName')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
})

test('CreateAvatar', async() => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/CreateAvatar.html', USE_DEBUG_CSP, null, spaceId)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully created avatar')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
})

test('Offline', async() => {
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/Offline.html', USE_DEBUG_CSP, null, null)

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

test('EnterSpaceFromCheckpoint', async() => {
  // This test specifically also checks a stack overflow that occurred passing large data (the checkpoint json) over the ABI boundary.
  // The JSON in question is an anonymized, in-progress test one, so all the asset paths are nonsense. What matters here is that it's largish.
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/EnterSpaceFromCheckpoint.html', USE_DEBUG_CSP, null, null)

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

test('DisconnectWhenNewMultiplayerSessionInitiatedInSpace', async() => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();

  function delay(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  // Start 2 new signalR connections with the same user without either entering a space.
  // Delay the second one so we know the first one will be disconnected
  const [{errors, consoleMessages}] = await Promise.all([
    LaunchTestPage('http://127.0.0.1:8888/DisconnectWhenNewMultiplayerSessionInitiatedInSpace.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId),
    (async () => {
      await delay(3000); // 3 second delay
      return LaunchTestPage('http://127.0.0.1:8888/DisconnectWhenNewMultiplayerSessionInitiatedInSpace.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId);
    })()
  ])
  .then(values => {
    // Ensure that all information for each promise is logged to the console.
    for (const {errors, consoleMessages} of values) {
      console.log(consoleMessages);
      console.log(errors);
    }

    return values;
  });

  assert.ok(consoleMessages.some(e => e.includes('Successfully entered space')));
  assert.ok(consoleMessages.some(e => e.includes('Disconnected from server! Reason: New Multiplayer Session Initiated')));

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
})

test('DisconnectWhenNewMultiplayerSessionInitiatedOutOfSpace', async() => {
  const user = await CreateTestUser();
  await LoginAsUser(user);

  function delay(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  // Start 2 new signalR connections with the same user without either entering a space.
  // Delay the second one so we know the first one will be disconnected
  const [{errors, consoleMessages}] = await Promise.all([
    LaunchTestPage('http://127.0.0.1:8888/DisconnectWhenNewMultiplayerSessionInitiatedOutOfSpace.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null),
    (async () => {
      await delay(2000); // 2 second delay
      return LaunchTestPage('http://127.0.0.1:8888/DisconnectWhenNewMultiplayerSessionInitiatedOutOfSpace.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null);
    })()
  ])
  .then(values => {
    // Ensure that all information for each promise is logged to the console.
    for (const {errors, consoleMessages} of values) {
      console.log(consoleMessages);
      console.log(errors);
    }

    return values;
  });

  assert.ok(consoleMessages.some(e => e.includes('Disconnected from server! Reason: New Multiplayer Session Initiated')));

  //Cleanup
  await LogoutUser(user);
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