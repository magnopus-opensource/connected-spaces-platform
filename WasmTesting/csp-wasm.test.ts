import './pretend-to-be-a-browser'
import { CreatePublicTestSpace, CreateTestUser, LoginAsUser, LaunchTestPage, DeleteSpace, LogoutUser, TEST_ACCOUNT_PASSWORD } from './testhelpers'

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

test('Login', async () => {
  const user = await CreateTestUser();
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/Login.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully logged in')));
  assert.ok(errors.length == 0); //Should be no errors
})

test('EnterSpace', async () => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/EnterSpace.html', USE_DEBUG_CSP, null, spaceId)

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
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/CrossThreadLogCallbackLogin.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, null)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(
    !errors.some(e => e.message.includes('table index is out of bounds')));

});

test.only('Cross Thread Callbacks From NetworkInterrupted Callback, OB-1524', async () => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/CrossThreadConnectionInterrupted.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(
    !errors.some(e => e.message.includes('table index is out of bounds')));

  assert.ok(consoleMessages.some(e => e.includes('Connection inturrupted: true')));

});


test('SendReceiveNetworkEvent', async () => {
  // This test was added as a regression test against `RuntimeError: null function or function signature mismatch`
  // Caused by a wrapper gen bug when you make a return type of an enclosing function different for the return type of the callback
  // We didn't actually fix it at time of writing, change `ListenNetworkEvent` to return a bool and you'll see what I mean.
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();

  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/SendReceiveNetworkEvent.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD }, spaceId);

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Received event: EventName')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
})

test('CreateAvatar', async () => {
  const user = await CreateTestUser();
  await LoginAsUser(user);
  const spaceId = await CreatePublicTestSpace();
  const { errors, consoleMessages } = await LaunchTestPage('http://127.0.0.1:8888/CreateAvatar.html', USE_DEBUG_CSP, null, spaceId)

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully created avatar')));
  assert.ok(errors.length == 0); //Should be no errors

  //Cleanup
  await DeleteSpace(spaceId);
  await LogoutUser(user);
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