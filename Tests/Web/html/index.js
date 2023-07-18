import { ready, CspOptions, CSPFoundation } from './olympus_foundation.js';

import './tests/usersystem_tests.js';
import './tests/spacesystem_tests.js';
import './tests/miscellaneous_tests.js';
import './tests/assetsystem_tests.js';
import './tests/multiplayer_tests.js';
import './tests/graphqlsystem_tests.js';
import './tests/settingssystem_tests.js';
import './tests/conversationsystem_tests.js';
import './tests/anchorsystem_tests.js';
import './tests/mimetypehelper_tests.js';
import './tests/lod_tests.js';

import { runTests } from './test_framework.js'


async function runAllTests() {
    var options = new CspOptions();
    options.wrapperUrl = "http://localhost:8080/node_modules/@magnopus/com.magnopus.olympus.foundation.web/Debug/OlympusFoundation_WASM.js";
    options.wasmUrl = "http://localhost:8080/node_modules/@magnopus/com.magnopus.olympus.foundation.web/Debug/OlympusFoundation_WASM.wasm";
    options.workerUrl = "http://localhost:8080/node_modules/@magnopus/com.magnopus.olympus.foundation.web/Debug/OlympusFoundation_WASM.worker.js";

    var Module = await ready(options);
    console.log(`Foundation build Id: ${CSPFoundation.getBuildID()}`);

    const params = new URLSearchParams(window.location.search);
    var results = await runTests(params.getAll('test'));
    
    var resultsContainer = document.createElement('div');
    resultsContainer.id = 'test_results';
    resultsContainer.innerText = (new XMLSerializer()).serializeToString(results);
    document.body.appendChild(resultsContainer);
}


runAllTests();