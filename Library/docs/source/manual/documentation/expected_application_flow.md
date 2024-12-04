# Expected Application Flow

This document explains the CSP expectations of client implementations of multiplayer functionality, primarily focussed on the order of methods called and any relevant details. It should not be considered a comprehensive implementation guide, and as such will not go into details on parameter inputs for the API functions listed.

The list reads numerically in the suggested order of execution. While some operations _may_ work or _appear_ to work when executed out of this order, it is not recommended and we cannot reasonably provide support in situations where this is the case.
Language details and CSP API syntax may differ depending on the client.

For details about the sending of data using the Multiplayer System, please see the following page:
[Multiplayer and State Replication](multiplayer_state_replication)

## 1. Initialise CSP
```c++
bool CSPFoundation::Initialise(const csp::common::String& EndpointRootURI, const csp::common::String& InTenant)
```

This starts up CSP and its various systems, including the MultiplayerConnection.

### a. Set UserAgent information
```c++
void CSPFoundation::SetClientUserAgentInfo(const csp::ClientUserAgent& ClientUserAgentHeader)
```

While not strictly necessary for multiplayer functionality, this is a recommended step which should be done following the initialisation of CSP.

## 2. Log In
```c++
void UserSystem::Login(const csp::common::String& UserName,
   			   const csp::common::String& Email,
   			   const csp::common::String& Password,
   			   const csp::common::Optional<bool>& UserHasVerifiedAge,
   			   LoginStateResultCallback Callback)
```

This is the main point of authentication, and can be done at any stage after initialisation has completed, but is required to call the majority of the CSP endpoints.
Note that there are several different methods of logging in, such as:
* UserSystem::Login
* UserSystem::LoginWithRefreshToken
* UserSystem::LoginAsGuest
* UserSystem::LoginToThirdPartyAuthenticationProvider

This call also initiates the MultiplayerConnection, which will provide real time data replication to entities, and network events, later in the lifecycle.

This call triggers a callback (provided in any above methods parameter list), which informs the client of a successful login attempt. Until this callback returns, the client cannot assume the user has logged in.

### a. Tick
Once Login is complete, the client application should consider starting to call Tick at a regular rate. This will be required to receive any event messages, including messages relating to forced disconnects, or messages relating to Organizations, for example.

## 3. Set the EntityCreated Callback
```c++
void SpaceEntitySystem::SetEntityCreatedCallback(EntityCreatedCallback Callback)
```

This callback will be triggered any time the client receives a message from a space the user is subscribed to, stating that an Entity has been created. It is advised to call this just prior to entering a space, as once subscribed to a space, this callback will be triggered for each Entity currently in the space. The Client should use the data passed here to initialize any local representation of the entities in their application.

This call does not need to be awaited, the callback passed in is immediately registered by CSP and is considered active as soon as this method is invoked. If this method is called multiple times, only the latest callback will be active, as the old ones will be discarded.

## 4. Enter Space
```c++
void SpaceSystem::EnterSpace(const String& SpaceId, NullResultCallback Callback)
```

This call does several things:
* First, it validates the user calling it has the relevant permissions to be in the space given. If not it will reject the call. 
* Secondly, it sets 'scopes', which are a subscription to a set of Entity replication messages. At the time of writing there is a single scope for a space, so any Entity in that space is replicated to any other user in the space. This is due to change in future, with more complex scopes representing 'layers' that can be used to alter what data is replicated to which user.
* Thirdly, it retrieves all Entities currently in the space. When received these will trigger the callback in the previous step, allowing the Client to create local representations based on their data.

The Client must wait for the callback to be triggered before assuming the Space has been entered and performing any Entity operations.

## 5. Exit Space
```c++
void SpaceSystem::ExitSpace(NullResultCallback Callback)
```

This call unsubscribes from the scopes that have been set so far. It also clears all local Entity objects within CSP, along with any pending updates.
The client must wait for the callback to be triggered before assuming the Space has been left successfully.

## 6. Log Out
```c++
void UserSystem::Logout(NullResultCallback Callback)
```

This call does 2 things, it revokes the users authentication (and associated session refresh tokens), and it also disconnects the MultiplayerConnection. Again, the callback must be triggered before the Client can assume that any of this process has occurred.

## 7. Shutdown
```c++
bool CSPFoundation::Shutdown()
```

This call closes out CSP, destroying all Systems and unregistering listeners, tidying memory. It's not a latent call, so once called, it should be assumed that CSP is now inactive.
