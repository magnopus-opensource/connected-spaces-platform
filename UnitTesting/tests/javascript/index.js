import { ready } from '@magnopus-opensource/connected-spaces-platform.dummy';

import { runTests } from './test_framework.js';

import './tests.js';

async function runAllTests() {
    await ready(true);

    await runTests();
}

runAllTests();
