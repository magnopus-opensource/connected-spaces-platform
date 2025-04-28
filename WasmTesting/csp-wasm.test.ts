import './pretend-to-be-a-browser'

import { test } from 'uvu';
import * as assert from 'uvu/assert';
import { CSPFoundation, ready, Systems } from 'connected-spaces-platform.web';

let CHS_ENDPOINT_ROOT = "https://ogs-internal.magnopus-dev.cloud";
let TENANT = "OKO_TESTS";

//Initialize CSPFoundation before the tests run
//True if USE_RELEASE_CSP is not set, false otherwise. Idea here is we want debug to be the default mode.
const USE_DEBUG_CSP: boolean = process.env.USE_RELEASE_CSP === undefined;

test.before(async () => {
  console.log(
      USE_DEBUG_CSP
        ? "Running with Debug CSP"
        : "Running with Release CSP"
    );
  await ready(USE_DEBUG_CSP);
});

test('Initialize CSP', () => {
    CSPFoundation.initialise(CHS_ENDPOINT_ROOT, TENANT);
    assert.equal(CSPFoundation.getIsInitialised(), true);
    assert.equal(CSPFoundation.getTenant(), TENANT);
});

test.run();