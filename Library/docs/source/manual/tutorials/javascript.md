# JavaScript

## Initializing

Connected Spaces Platform needs to be initialized before making any calls to other parts of its API. For this, you will need a valid CSP tenant. If you don't already have one, you'll need to [apply for one](https://www.magnopus.com/csp/for-developers#tenant-id).

```javascript
import { ready, CSPFoundation } from 'connected-spaces-platform.web';

// We first need to load the CSP WASM module by calling `ready`
await ready();

/* 
 * The first parameter passed to Initialise is the CHS services root URL.
 * "ogs-ostage" is the environment, and is currently the only environment publicly available for clients.
 * The last parameter is your tenant ID.
 */
CSPFoundation.initialise('https://ogs-ostage.magnoboard.com', '<your tenant>');
```

After initialising, you should then set the user agent information that will be used for all requests made by CSP.

```javascript
import { ClientUserAgent } from 'connected-spaces-platform.web';

const userAgent = ClientUserAgent.create();
userAgent.cSPVersion        = CSPFoundation.getVersion();
userAgent.clientOS          = '<your current OS identifier>';
userAgent.clientSKU         = '<an identifier for your project>';
userAgent.clientVersion     = '<your project version identifier>';
userAgent.clientEnvironment = '<your project environment>';

CSPFoundation.setClientUserAgentInfo(userAgent);
userAgent.delete();
```

## Authenticating

Authentication is done using the `UserSystem` singleton. System singleton instances are managed by the static `SystemsManager` class. The code snippet below shows how to retrieve the singleton instance for `UserSystem`.

```javascript
import { Services, Systems } from 'connected-spaces-platform.web';

const systemManager = Systems.SystemsManager.get();
const userSystem = systemManager.getUserSystem();
```

You can then proceed to log in using an account or as a guest user. Most endpoints in CSP are asynchronous and should be awaited.

```javascript
function onLogin(result) {
  const resCode = result.getResultCode();

  // If the request fails, ResultCode will be set to EResultCode.Failed
  if (resCode == Services.EResultCode.Failed) {
    // ResultBase.FailureReason will be set if the request failed and should
    // be used to determine the cause of the failure.
    const reason = result.getFailureReason();

    switch (reason) {
      case Systems.ELoginStateFailureReason.EmailNotConfirmed:
        console.log('User email not verified! Please follow the link in the verification email sent to you after registering an account.');
        break;
      case Systems.ELoginStateFailureReason.AgeNotVerified:
        console.log('User has not confirmed they are over 18! Either you are too young or should re-enter your DoB on the sign in page.');
        break;
      case Systems.ELoginStateFailureReason.Unknown:
        // If FailureReason is unknown, check the HTTP response code.
        // If it is 403, the provided account credentials were incorrect.
        console.log('Request failed with HTTP response code ', result.GetHttpResultCode());
        break;
    }
  }
}

// Log in using a registered account
// The 4th parameter is a boolean indicating whether the user has verified their age or not,
// and can be null to use the value supplied during a previous call to login() or createUser()
const result = await userSystem.login('<username - leave empty>', '<account email>', '<account password>', true);
onLogin(result);
result.delete();

// Log in using a guest account
// The 1st parameter is the same boolean as above
const result = await userSystem.loginAsGuest(true);
onLogin(result);
result.delete();
```

## Querying Spaces

Space Querying is done using the `SpaceSystem `singleton.

The code snippet below, shows how to retrieve the singleton instance for `SpaceSystem`.

`const spaceSystem = systemsManager.getSpaceSystem();`

You can then proceed to log in using an account or as a guest user, as described in the previous tutorial, entitled "Authenticating”.


### Retrieving all spaces associated with the currently logged in user
```
const spacesResult = await spaceSystem.getSpaces();

if (spaceResult.getResultCode() === Systems.EResultCode.Success)
{
    let spaces = spacesResult.GetSpaces();
    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(spaces[0].id, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();
    spaces.delete();
}
else if (spaceResult.getResultCode() === Systems.EResultCode.InProgress)
{
    console.log("In progress: ", result.GetRequestProgress());
    enterSpaceResult.delete();
}
else
{
    console.log("Failed to get spaces", resultStatus(spacesResult));
    enterSpaceResult.delete();
}
```


### Retrieving a single space, associated with a particular id
```
let spaceId = "ah34hgt25prv”;

const spaceResult = await spaceSystem.getSpace(spaceId);

if (spaceResult.getResultCode() === Systems.EResultCode.Success)
{
    let space = spacesResult.GetSpace();
    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(space.id, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();
    spaces.delete();
}
else if (spaceResult.getResultCode() === Systems.EResultCode.InProgress)
{
    console.log("In progress: ", result.GetRequestProgress());
    enterSpaceResult.delete();
}
else
{
    console.log("Failed to get spaces", resultStatus(spacesResult));
    enterSpaceResult.delete();
}
```


### Retrieving spaces associated with a particular user id
```
let user_id = "bc649hnt21h”;

const spacesResult = await spaceSystem.getSpacesForUserId(user_id);

if (spaceResult.getResultCode() === Systems.EResultCode.Success)
{
    let spaces = spacesResult.GetSpaces();
    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(spaces[0].id, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();
    spaces.delete();
}
else if (spaceResult.getResultCode() === Systems.EResultCode.InProgress)
{
    console.log("In progress: ", result.GetRequestProgress());
    enterSpaceResult.delete();
}
else
{
    console.log("Failed to get spaces", resultStatus(spacesResult));
    enterSpaceResult.delete();
}
```
It should be noted that this can only be performed by users with elevated privileges. Calling this function without elevated privileges will fail.


### Retrieving spaces, by specifying a number of space identifiers associated with the currently logged in user
```
let space_ids = [ "ah34hgt25prv”, 
"ev936bym27c”, 
"jt94nrp74k” ];

const spacesResult = await spaceSystem.getSpacesByIds(space_ids);

if (spaceResult.getResultCode() === Systems.EResultCode.Success)
{
    let spaces = spacesResult.GetSpaces();
    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(spaces[0].id, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();
    spaces.delete();
}
else if (spaceResult.getResultCode() === Systems.EResultCode.InProgress)
{
    console.log("In progress: ", result.GetRequestProgress());
    enterSpaceResult.delete();
}
else
{
    console.log("Failed to get spaces", resultStatus(spacesResult));
    enterSpaceResult.delete();
}
```

### Retrieving spaces with the given set of attributes available to the logged in user
```
const spacesResult = await spaceSystem.getSpacesByAttributes(
    true, 		// isDiscoverable
    false, 	        // isArchivable
    true, 		// requiresInvite
    0, 		        // results skip (in this case start the first result)
    4 		        // results max ( in this case stop at 4)
);

if (spaceResult.getResultCode() === Systems.EResultCode.Success)
{
    let spaces = spacesResult.GetSpaces();
    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(spaces[0].id, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();
    spaces.delete();
}
else if (spaceResult.getResultCode() === Systems.EResultCode.InProgress)
{
    console.log("In progress: ", result.GetRequestProgress());
    enterSpaceResult.delete();
}
else
{
    console.log("Failed to get spaces", resultStatus(spacesResult));
    enterSpaceResult.delete();
}
```

Each of the attribute arguments are optional. 
* `isDiscoverable `is set to true if we are searching for spaces which are publicly listed.
* `isArchived `is set to true if we are searching for archived spaces.
* `requiresInvite `is set to true if we are searching for spaces which require an invite to enter.
* The next two arguments are to enable paginating results.
* `resultsSkip `sets which result to start at (in the above case, the first result)
* `resultsMax `sets which result to stop the current search (in the above example, result 4)

You can disable pagination, by changing the last two arguments to null values:
```
const spacesResult = await spaceSystem.getSpacesByAttributes(
    true, 		// isDiscoverable
    false, 	        // isArchivable
    true, 		// requiresInvite
    null, 		
    null 		// Two null arguments mean all spaces will be retrieved
);

if (spaceResult.getResultCode() === Systems.EResultCode.Success)
{
    let spaces = spacesResult.GetSpaces();
    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(spaces[0].id, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();
    spaces.delete();
}
else if (spaceResult.getResultCode() === Systems.EResultCode.InProgress)
{
    console.log("In progress: ", result.GetRequestProgress());
    enterSpaceResult.delete();
}
else
{
    console.log("Failed to get spaces", resultStatus(spacesResult));
    enterSpaceResult.delete();
}
```