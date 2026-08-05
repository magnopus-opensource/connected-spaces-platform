import { ready, CSPFoundation, ClientUserAgent, Common, Systems} from '../node_modules/connected-spaces-platform.web/connectedspacesplatform.js';

let CHS_ENDPOINT_ROOT = "https://ogs.magnopus-dev.cloud";
let TENANT = "OKO_TESTS";
let TESTS_CLIENT_SKU = "WASMTest";
let TESTS_CLIENT_OS = "WASMTestsOS";

export async function initializeCSP(useDebugCSP, wafBypass) {
  try {
    console.log(
      useDebugCSP
        ? "Initializing with Debug CSP"
        : "Initializing with Release CSP"
    );

    await ready(useDebugCSP);

    const userAgent = ClientUserAgent.create();
    userAgent.cSPVersion = CSPFoundation.getBuildID();
    userAgent.clientOS = TESTS_CLIENT_OS;
    userAgent.clientSKU = TESTS_CLIENT_SKU;
    userAgent.clientVersion = CSPFoundation.getBuildID();
    userAgent.clientEnvironment = "ODev";
    userAgent.cHSEnvironment = "oDev";

    CSPFoundation.initialise(CHS_ENDPOINT_ROOT, TENANT, userAgent, {});
    if (!CSPFoundation.getIsInitialised()) {
      throw new Error("CSPFoundation failed to initialize");
    }

    console.log("CSP Initialization complete.");
  } catch (error) {
      console.error("Failed to initialize CSP:", error.message);
      console.error(error.stack);
  }

  if (wafBypass === undefined) {
    console.log("WAF bypass not set.");
    return;
  }

  Systems.SystemsManager.get().__SetWAFBypass(wafBypass);
  console.log("WAF bypass set.");
}