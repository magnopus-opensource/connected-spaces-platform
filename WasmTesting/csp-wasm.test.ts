import './pretend-to-be-a-browser'

import { test } from 'uvu';
import * as assert from 'uvu/assert';
import { CSPFoundation, ready, Systems } from 'connected-spaces-platform.web';

let CHS_ENDPOINT_ROOT = "https://ogs-internal.magnopus-dev.cloud";
let TENANT = "OKO_TESTS";

//Initialize CSPFoundation before the tests run
let USE_DEBUG_CSP = true

test.before(async () => {
  await ready(USE_DEBUG_CSP);
});

test('Initialize CSP', () => {
    CSPFoundation.initialise(CHS_ENDPOINT_ROOT, TENANT);
    assert.equal(CSPFoundation.getIsInitialised(), true);
    assert.equal(CSPFoundation.getTenant(), TENANT);
});

test.run();