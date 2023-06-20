import { ready } from '@magnopus/com.magnopus.olympus.foundation.dummy';

import { runTests } from './test_framework.js';

import './tests.js';


async function runAllTests() {
    await ready(true);
    
    await runTests();
}


runAllTests();
        