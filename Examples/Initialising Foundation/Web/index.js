import {
  CSPFoundation,
  ready,
} from "./node_modules/@magnopus-opensource/connected-spaces-platform.web/connectedspacesplatform.js";

// Magnopus Services Endpoint to connect to.
const ENDPOINT = "https://ogs-ostage.magnoboard.com";

// Tenant defines the application scope.
const TENANT = "FOUNDATION_HELLO_WORLD";

const runInitialisingCSPExample = () => {
  // If we should use debug version of Connected Spaces Platform (CSP).
  const debug = false;

  // We call ready as promise, as to know CSP is ready.
  ready(debug).then(async () => {
    // Initialise CSP against a given endpoint and tenant
    if (await CSPFoundation.initialise(ENDPOINT, TENANT)) {
      console.log("CSP initalised");
    } else {
      console.log("CSP failed to initalise");
    }

    if (CSPFoundation.getIsInitialised()) {
      if (await CSPFoundation.shutdown()) {
        console.log("CSP shutdown");
      } else {
        console.log("CSP failed to shutdown");
      }
    }
  }, 3000);
};

runInitialisingCSPExample();
