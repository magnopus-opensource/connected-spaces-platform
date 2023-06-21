import {
  ClientUserAgent,
  Multiplayer,
  OlympusFoundation,
  Systems,
  ready,
  freeBuffer,
  Common,
  Services,
} from "./node_modules/@magnopus/com.magnopus.olympus.foundation.web/olympus.foundation.js";

// Magnopus Services Endpoint to connect to.
const ENDPOINT = "https://ogs-odev.magnoboard.com";

// Tenant defines the application scope.
const TENANT = "FOUNDATION_HELLO_WORLD";

const runInitialisingFoundationExample = () => {

  // We call ready as promise, as to know Foundation is ready.
  ready(debug).then(async () => {

    // Initialise Foundation against a given endpoint and tenant
    if(await OlympusFoundation.initialise(ENDPOINT, TENANT))
    {
      console.log("Foundation initalised")
      
    }
    else
    {
      console.log("Foundation failed to initalise")
    }

    if(OlympusFoundation.getIsInitialised()){
      
      if(await OlympusFoundation.shutdown())
      {
        console.log("Foundation shutdown")
      }
      else
      {
        console.log("Foundation failed to shutdown")
      }
    }
    }, 3000);
};

runInitialisingFoundationExample();
