# C++

## Initializing
Connected Spaces Platform needs to be initiali\ed before making any calls to other parts of its API. For this, you will need a valid CSP tenant. If you don't already have one, you'll need to [apply for one](https://www.magnopus.com/csp/for-developers#tenant-id).

```c++
#include "CSP/CSPFoundation.h"

/* 
 * The first parameter passed to Initialise is the CHS services root URL.
 * "ogs-ostage" is the environment, and is currently the only environment publicly available for clients.
 * The last parameter is your tenant ID.
 */
csp::CSPFoundation::Initialise("https://ogs-ostage.magnoboard.com", "<your tenant>");
```

After initialising, you should then set the user agent information that will be used for all requests made by CSP.

```c++
csp::ClientUserAgent userAgent;
userAgent.CSPVersion        = csp::CSPFoundation::GetVersion();
userAgent.ClientOS          = "<your current OS identifier>";
userAgent.ClientSKU         = "<an identifier for your project>";
userAgent.ClientVersion     = "<your project version identifier>";
userAgent.ClientEnvironment = "<your project environment>";

csp::CSPFoundation::SetClientUserAgentInfo(userAgent);
```

## Authenticating

Authentication is done using the `UserSystem` singleton. System singleton instances are managed by the static `SystemsManager` class. The code snippet below shows how to retrieve the singleton instance for `UserSystem`.

```c++
#include "CSP/Systems/Users/UserSystem.h"
#include "CSP/Systems/SystemsManager.h"

csp::systems::SystemsManager& systemManager = csp::systems::SystemsManager::Get();
csp::systems::UserSystem* userSystem = systemManager.GetUserSystem();
```

You can then proceed to log in using an account or as a guest user. Most endpoints in CSP are asynchronous and you will need to pass a callback to them. This callback will be called once for each progress update, and once more when the request has completed.

```c++
void OnLogin(const csp::systems::LoginStateResult& result) {
  csp::services::EResultCode resCode = result.GetResultCode();

  // Some endpoints (like AssetSystem::UploadAssetData()) report progress status.
  // This endpoint does not, so it can be ignored.
  if (resCode == csp::services::EResultCode::InProgress)
    return;

  // If the request fails, ResultCode will be set to EResultCode::Failed
  if (resCode == csp::services::EResultCode::Failed) {
    // ResultBase::FailureReason will be set if the request failed and should
    // be used to determine the cause of the failure.
    auto reason = static_cast<csp::systems::ELoginStateFailureReason>(result.GetFailureReason());

    switch (reason) {
      case csp::systems::ELoginStateFailureReason::EmailNotConfirmed:
        std::cout << "User email not verified! Please follow the link in the verification email sent to you after registering an account." << std::endl;
        break;
      case csp::systems::ELoginStateFailureReason::AgeNotVerified:
        std::cout << "User has not confirmed they are over 18! Either you are too young or should re-enter your DoB on the sign in page." << std::endl;
        break;
      case csp::systems::ELoginStateFailureReason::Unknown:
        // If FailureReason is unknown, check the HTTP response code.
        // If it is 403, the provided account credentials were incorrect.
        std::cout << "Request failed with HTTP response code " << result.GetHttpResultCode() << std::endl;
        break;
    }
  }
}

// Log in using a registered account
// The 4th parameter is a boolean indicating whether the user has verified their age or not,
// and can be null to use the value supplied during a previous call to Login() or CreateUser()
userSystem->Login("<username - leave empty>", "<account email>", "<account password>", true, OnLogin);

// Log in as a guest user
// The 1st parameter is the same boolean as above
userSystem->LoginAsGuest(true, OnLogin);
```

## Querying Spaces

Space Querying is done using the SpaceSystem singleton.

The code snippet below, shows how to retrieve the singleton instance for SpaceSystem.
```
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Spaces/SpaceSystem.h


csp::systems::SystemsManager& systemManager = csp::systems::SystemsManager::Get();
csp::systems::SpaceSystem* spaceSystem = SystemsManager.GetSpaceSystem();
```

You can then proceed to log in using an account or as a guest user, as described in the previous tutorial, entitled "Authenticating".

### Retrieving all spaces associated with the currently logged in user

First, a callback is defined to handle the result of the query call.

```
auto resultHandler = [](const SpacesResult& result)
{
    // Example code responding to a successful query
    if(result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cout << "SUCCESS: " << std::endl;

        for (int i = 0; i < result.GetSpaces().Size(); i++)
        {
            std::cout << "Space found: " << result.GetSpaces()[i].Id << std::endl;
        } 
        return true;
    }
    else if(result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        // Example progress reporting code
        std::cout << "Progress: " << result.GetRequestProgress() << std::endl;
        return true;
    }
    else
    {
        // Example failure logging
        std::cout << "Error: " << ToString(result.GetFailureReason()) << std::endl;
        return false;
    }
};
spaceSystem->GetSpaces(resultHandler);
```

Where the space ids are alphanumeric strings.

### Retrieving a single space, associated with a particular space id

```
auto resultHandler = [](const SpaceResult& result)
{
    if(result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cout << "SUCCESS: " << std::endl;

        // Example code responding to a successful query
        std::cout << "Space found: " << result.GetSpace().Id << std::endl;

        return true;
    }
    else if(result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        // Example progress reporting code
        std::cout << "Progress: " << result.GetRequestProgress() << std::endl;
        return true;
    }
    else
    {
        // Example failure logging
        std::cout << "Error: " << ToString(result.GetFailureReason()) << std::endl;
        return false;
    }
};

SpaceSystem->GetSpace("ah34hgt25prv", resultHandler);
```

Where the space id is an alphanumeric string.

### Retrieving all spaces associated with a particular user id
```
auto resultHandler = [](const SpacesResult& result)
{
    // Example code responding to a successful query
    if(result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cout << "SUCCESS: " << std::endl;

        for (int i = 0; i < result.GetSpaces().Size(); i++)
        {
	    std::cout << "Space found: " << result.GetSpaces()[i].Id << std::endl;
        } 

        return true;
    }
    else if(result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        // Example progress reporting code
	std::cout << "Progress: " << result.GetRequestProgress() << std::endl;
	return true;
    }
    else
    {
	// Example failure logging
        std::cout << "Error: " << ToString(result.GetFailureReason()) << std::endl;
        return false;
    }
};

SpaceSystem->GetSpacesForUserId(
    "bc649hnt21h", 
    resultHandler
);
```
Where the user id is an alphanumeric string. 
It should be noted that this can only be performed by users with elevated privileges. Calling this function without elevated privileges will fail.

### Retrieving spaces by specifying a number of space identifiers associated with the currently logged in user

```
auto resultHandler = [](const SpacesResult& result)
{
    // Example code responding to a successful query
    if(result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        std::cout << "SUCCESS: " << std::endl;

        for (int i = 0; i < result.GetSpaces().Size(); i++)
        {
            std::cout << "Space found: " << result.GetSpaces()[i].Id << std::endl;
        } 
        return true;
    }
    else if(result.GetResultCode() == csp::systems::EResultCode::InProgress)
    {
        // Example progress reporting code
        std::cout << "Progress: " << result.GetRequestProgress() << std::endl;
        return true;
    }
    else
    {
        // Example failure logging
        std::cout << "Error: " << ToString(result.GetFailureReason()) << std::endl;
        return false;
    }
};

spaceSystem->GetSpacesByAttributes(
    true, 	// isDiscoverable
    false, 	// isArchived
    true, 	// requiresInvite
    0, 		// results skip (in this case start the first result)
    4, 		// results max ( in this case stop at 4)
    resultHandler	
);


```

Each of the attribute arguments are optional.
* `isDiscoverable` is set to true if we are searching for spaces which are publicly listed.
* `isArchived` is set to true if we are searching for archived spaces.
* `requiresInvite` is set to true if we are searching for spaces which require an invite to enter.
* The next two arguments are to enable paginating results.
* `resultsSkip` sets which result to start at (in the above case, the first result)
* `resultsMax` sets which result to stop the current search (in the above example, result 4)

You could then fetch the next 5 spaces in the following way:
```
spaceSystem->GetSpacesByAttributes(
    true,    // isDiscoverable
    false,   // isArchivable
    true,    // requiresInvite
    5, 	     // results skip (in this case start at space result 5)
    9, 	     // results max ( in this case stop at 9)
    resultHandler
);
```

You can disable pagination, by changing the last two arguments to zero values:

```
spaceSystem->GetSpacesByAttributes(
    true,    // isDiscoverable
    false,   // isArchivable
    true,    // requiresInvite
    0, 	     // resultsSkip
    0, 	     // resultsMax
    resultHandler
);
```