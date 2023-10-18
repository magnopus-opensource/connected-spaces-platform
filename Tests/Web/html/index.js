import { ready, CspOptions, CSPFoundation } from './connected_spaces_platform.js';

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
import './tests/logsystem_tests.js';

import { runTests } from './test_framework.js'


async function runAllTests() {
    var options = new CspOptions();
    options.wrapperUrl = "http://localhost:8080/node_modules/connected-spaces-platform.web/Debug/ConnectedSpacesPlatform_WASM.js";
    options.wasmUrl = "http://localhost:8080/node_modules/connected-spaces-platform.web/Debug/ConnectedSpacesPlatform_WASM.wasm";
    options.workerUrl = "http://localhost:8080/node_modules/connected-spaces-platform.web/Debug/ConnectedSpacesPlatform_WASM.worker.js";

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