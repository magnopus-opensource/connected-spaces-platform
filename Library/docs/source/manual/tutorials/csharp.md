# C#

## Initializing

Connected Spaces Platform needs to be initialized before making any calls to other parts of its API. For this, you will need a valid CSP tenant. If you don't already have one, you'll need to [apply for one](https://www.magnopus.com/csp/for-developers#tenant-id).

```c#
using Csp;

/* 
 * The first parameter passed to Initialise is the CHS services root URL.
 * "ogs-ostage" is the environment, and is currently the only environment publicly available for clients.
 * The last parameter is your tenant ID.
 */
CSPFoundation.Initialise("https://ogs-ostage.magnoboard.com", "<your tenant>");
```

After initialising, you should then set the user agent information that will be used for all requests made by CSP.

```c#
using var userAgent = new ClientUserAgent()
{
  CSPVersion        = CSPFoundation.GetVersion(),
  ClientOS          = "<your current OS identifier>",
  ClientSKU         = "<an identifier for your project>",
  ClientVersion     = "<your project version identifier>",
  ClientEnvironment = "<your project environment>"
};

CSPFoundation.SetClientUserAgentInfo(userAgent);
```

## Authenticating

Authentication is done using the `UserSystem` singleton. System singleton instances are managed by the static `SystemsManager` class. The code snippet below shows how to retrieve the singleton instance for `UserSystem`.

```c#
using Csp;
using Csp.Services;
using Csp.Systems;

SystemsManager systemManager = SystemsManager.Get();
UserSystem userSystem = systemManager.GetUserSystem();
```

You can then proceed to log in using an account or as a guest user. Most endpoints in CSP are asynchronous and will need to be awaited.

```c#
void OnLogin(LoginStateResult result) {
  EResultCode resCode = result.GetResultCode();

  // If the request fails, ResultCode will be set to EResultCode.Failed
  if (resCode == EResultCode.Failed) {
    // ResultBase.FailureReason will be set if the request failed and should
    // be used to determine the cause of the failure.
    var reason = (ELoginStateFailureReason)result.GetFailureReason();

    switch (reason) {
      case ELoginStateFailureReason.EmailNotConfirmed:
        Console.WriteLine("User email not verified! Please follow the link in the verification email sent to you after registering an account.");
        break;
      case ELoginStateFailureReason.AgeNotVerified:
        Console.WriteLine("User has not confirmed they are over 18! Either you are too young or should re-enter your DoB on the sign in page.");
        break;
      case ELoginStateFailureReason.Unknown:
        // If FailureReason is unknown, check the HTTP response code.
        // If it is 403, the provided account credentials were incorrect.
        Console.WriteLine($"Request failed with HTTP response code {result.GetHttpResultCode()}");
        break;
    }
  }
}

// Log in using a registered account
// The 4th parameter is a boolean indicating whether the user has verified their age or not,
// and can be null to use the value supplied during a previous call to Login() or CreateUser()
using LoginStateResult result = await userSystem.Login("<username - leave empty>", "<account email>", "<account password>", true);
OnLogin(result);

// Log in using a guest account
// The 1st parameter is the same boolean as above
using LoginStateResult result = await userSystem.LoginAsGuest(true);
OnLogin(result);
```

## Querying Spaces

Space Querying is done using the `SpaceSystem` singleton.

The code snippet below, shows how to retrieve the singleton instance for `SpaceSystem`.
```
using Csp;
using Csp.Services;
using Csp.Systems;

var spaceSystem = Csp.Systems.SystemsManager.Get().GetSpaceSystem();
```

You can then proceed to log in using an account or as a guest user, as described in the previous tutorial, entitled "Authenticating”.

### Retrieving all spaces associated with the currently logged in user
```
using CspSystems.SpacesResult spaceResult = await spaceSystem.GetSpaces();

if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
{
    using var spaces = spaceResult.GetSpaces();
    await EnterSpace(spaces[0]);
}
else if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.InProgress)
{
    Console.WriteLine("Progress: ” + result.GetRequestProgress());
}
else
{
    // Example failure logging
    Debug.LogError($"Failed to get space". Unable to enter.");
    Debug.LogError($"ToString(result.GetFailureReason())");
}
```

### Retrieving a single space, associated with a particular space id
```
using Csp.Systems.SpaceResult spaceResult = await spaceSystem.GetSpace("ah34hgt25prv”);


if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
{
    using var space = spaceResult.GetSpace();
    await EnterSpaceAsync(space);
}
else if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.InProgress)
{
    Console.WriteLine("Progress: ” + result.GetRequestProgress());
}
else
{
    // Example failure logging
    Debug.LogError($"Failed to get space". Unable to enter.");
    Debug.LogError($"ToString(result.GetFailureReason())");
}
```
Where the id is an alphanumeric string.


### Retrieving spaces associated with a particular user id
```
using CspSystems.SpaceResult spaceResult = await spaceSystem.GetSpacesForUserId("bc649hnt21h”);

if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
{
    using var spaces = spaceResult.GetSpaces();
    await EnterSpace(spaces[0]);
}
else if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.InProgress)
{
    Console.WriteLine("Progress: ” + result.GetRequestProgress());
}
else
{
    // Example failure logging
    Debug.LogError($"Failed to get space". Unable to enter.");
    Debug.LogError($"ToString(result.GetFailureReason())");
}
```
Where the user id is an alphanumeric string. 
It should be noted that this can only be performed by users with elevated privileges. Calling this function without elevated privileges will fail.

### Retrieving spaces by specifying a number of space identifiers associated with the currently logged in user
```
using idArray = new Csp.Common.Array<string>(3);
idArray[0] = "ah34hgt25prv”;
idArray[1] = "ev936bym27c”; 
idArray[2] = "jt94nrp74k”;

using CspSystems.SpacesResult spaceResult = await spaceSystem.GetSpacesByIds(idArray);

if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
{
    using var spaces = spaceResult.GetSpaces();
    await EnterSpace(spaces[0]);
}
else if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.InProgress)
{
    Console.WriteLine("Progress: ” + result.GetRequestProgress());
}
else
{
    // Example failure logging
    Debug.LogError($"Failed to get space". Unable to enter.");
    Debug.LogError($"ToString(result.GetFailureReason())");
}
```
Where the space ids are alphanumeric strings.

### Retrieving spaces with the given set of attributes available to the logged in user
```
using CspSystems.SpacesResult spaceResult = await spaceSystem.GetSpacesByAttributes(
    true, 		// isDiscoverable
    false,    	        // isArchivable
    true, 		// requiresInvite
    0, 		        // results skip (in this case start the first result)
    4 		        // results max ( in this case stop at 4)
);
```
Each of the attribute arguments are optional. 
* `isDiscoverable` is set to true if we are searching for spaces which are publicly listed.
* `isArchived` is set to true if we are searching for archived spaces.
* `requiresInvite` is set to true if we are searching for spaces which require an invite to enter.
* The next two arguments are to enable paginating results.
* results skip sets which result to start at (in the above example, the first result)
* results max sets which result to stop the current search (in the above example, result 4)

In the case of the C# wrapper, you can disable pagination, by changing the last two arguments to null values:
```
using CspSystems.SpacesResult spaceResult = await spaceSystem.GetSpacesByAttributes(
    true, 		// isDiscoverable
    false, 	        // isArchivable
    true, 		// requiresInvite
    null,
    null 		// Two null arguments mean all spaces will be retrieved
);

if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
{
    using var spaces = spaceResult.GetSpaces();
    await EnterSpace(spaces[0]);
}
else if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.InProgress)
{
    Console.WriteLine("Progress: ” + result.GetRequestProgress());
}
else
{
    // Example failure logging
    Debug.LogError($"Failed to get space". Unable to enter.");
    Debug.LogError($"ToString(result.GetFailureReason())");
}
```