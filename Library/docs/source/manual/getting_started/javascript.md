# JavaScript

1. Grab and install a version of the NPM package from [npmjs](https://www.npmjs.com/package/connected-spaces-platform.web).
2. Import the Connected Spaces Platform (CSP) for Web module:
    - `import * as Olympus from './node_modules/@magnopus-opensource/connected-spaces-platform-web/connectedspacesplatform.js';`
3. Await `Olympus.ready()`, passing `true` to load the Debug build of CSP, and `false` or no value to load the Release build:
    - `await Olympus.ready(true);`
4. If needed, custom paths for the WASM files are supported. e.g.
    - `var options = new FoundationOptions();`
    - `options.wrapperUrl = "./Debug/ConnectedSpacesPlatform_WASM.js";`
    - `options.wasmUrl = "./ConnectedSpacesPlatform_WASM.wasm";`
    - `options.workerUrl = "./ConnectedSpacesPlatform_WASM.worker.js";`
    - `await Olympus.ready(options);`
5. That's all! You can now start calling CSP functions:
    - `console.log(Olympus.OlympusFoundation.getVersion());`

NOTE: You can also store the return value from the `ready()` promise and use it to access Emscripten functions directly:
  * `var Module = await Olympus.ready(); var buf = Module._malloc(32);`
