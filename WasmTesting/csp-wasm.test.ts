import './pretend-to-be-a-browser'
import  {CreateTestUser, LaunchTestPage, TEST_ACCOUNT_PASSWORD} from './testhelpers'

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

test ('Login', async() => {
  const user = await CreateTestUser();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/Login.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD })

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(consoleMessages.some(e => e.includes('Successfully logged in')));
  assert.ok(errors.length == 0); //Should be no errors
})

test('Cross Thread Callbacks From Log Callback, OB-3782', async () => {
  const user = await CreateTestUser();
  const {errors, consoleMessages} = await LaunchTestPage('http://127.0.0.1:8888/CrossThreadLogCallbackLogin.html', USE_DEBUG_CSP, { email: user.getProfile().email, password: TEST_ACCOUNT_PASSWORD })

  console.log(consoleMessages);
  console.log(errors);

  assert.ok(
    errors.some(e => e.message.includes('table index is out of bounds')),
    'Expected cross-thread `table index is out of bounds` error. Message not found, did you fix the bug (OB-3782)? Nice job! ',
  );
  
});

test.run();