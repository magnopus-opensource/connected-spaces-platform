# Authentication

Authentication is the process of verifying a user's identity before granting access to the cloud-hosted services. It ensures that only authorized individuals can interact with the platform.

It is often important for several reasons:

* **Security**: It protects sensitive data by ensuring that only authenticated users can access the services.

* **Personalized Experience**: Users have tailored access based on their account, including saved settings and preferences.

* **GDPR Compliance**: Proper authentication ensures that services meets regulatory data protection standards.

## Overview of CSP Authentication Methods

1. **Traditional Login**  
   Users authenticate by entering their username or email and password. This is the standard way to gain access to CSP.

2. **Single Sign-On (SSO)**  
   With SSO, users can log in using third-party identity providers like Google, Discord, or Apple, making the login process more seamless.

3. **Guest Login**  
   Guest users can temporarily access CSP without creating an account. This is ideal for users who want to explore the platform with limited permissions.

4. **Refresh Token Authentication**  
   Users who select the "Remember Me" option can use refresh tokens to maintain their session without needing to log in again, providing a smooth experience.

5. **Logging Out**  
   Logging out terminates the session, ensuring no unauthorized access continues after the user is finished.

## Log in
Logging into services involves using a username or email along with a password. This allows users to access the cloud-hosted services and interact with their environment. The login process is handled by CSP's `UserSystem::Login` method, which ensures secure authentication. 

### Steps for Logging In

1. **Input: Username/Email, Password**  
   Users can log in by providing their username or email and a password. The email parameter is left blank if the username is provided, and vice versa.

2. **Age Verification Requirement**  
   Services typically require users to confirm they are over 18 years old to use the platform. If age verification is not completed, the login attempt will be rejected.

3. **Login Request Construction**  
   When an application invokes `Login`, CSP internally constructs a `LoginRequest` object that is passed to the services. In addition to user credentials, this object includes:

   * **Device ID**: A unique identifier for the device being used.
   * **Tenant Info**: The specific tenant the user is logging into.
   * **Age Verification Flag**: Indicates whether the user has verified their age.

The following simplified example shows how CSP handles the login request:

```
void UserSystem::Login(const String& UserName, const csp::common::String& Email, const String& Password,Optional<bool> UserHasVerifiedAge, LoginStateResultCallback Callback) {
	// Check if user is logged out or has an error state
	if (CurrentLoginState.State == ELoginState::LoggedOut ||
	CurrentLoginState.State == ELoginState::Error) {
	
		// Construct LoginRequest object
		auto Request = std::make_shared<LoginRequest>();
		Request->SetDeviceId(CSPFoundation::GetDeviceId());
		Request->SetUserName(UserName);
		Request->SetEmail(Email);
		Request->SetPassword(Password);
		Request->SetTenant(CSPFoundation::GetTenant());
		
		// Add age verification if provided
		if (UserHasVerifiedAge.HasValue()) {
			Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
		}
		
		// Make the login call and handle response
		static_cast<AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLoginPost(Request, ResponseHandler);
	}
}
```

### LoginStateResultCallback and Response Handling
Once the login request is made, CSP processes the response using the `LoginStateResultCallback`. If the login is successful, the callback triggers a connection to the Multiplayer Service. CSP returns an error code if unsuccessful, notifying the user of the failed attempt.

### Notes

* **Verifying Current Login State**: Before proceeding with a login, CSP checks the user's current login state. CSP resets the login state if the user is already logged in or in an error state.
* **Handling Unsuccessful Logins**: CSP provides clear feedback to the user if login fails. The system ensures that proper error codes are returned, enabling users to take corrective action.

## Login with Single Sign-On (SSO)
Single Sign-On (SSO) is a method that allows CSP users to authenticate using an external identity provider, reducing the need to remember multiple credentials. With SSO, users log in once with a third-party provider like Google, Discord, or Apple and gain access to the services. SSO simplifies login, enhances security, and improves user experience by providing single identity management.

### SSO Process Overview

1. **Retrieve Supported Third-Party Authentication Providers**  
   CSP first identifies which third-party providers are supported. The `GetSupportedThirdPartyAuthenticationProviders` method returns a list of providers such as Google, Discord, and Apple.

2. **Get the Full Authorization URL for the Provider**  
   After selecting a provider, CSP constructs the full authorization URL using the `GetThirdPartyProviderAuthoriseURL` method. This URL is built using:

   * The base authorization URL of the provider.

   * The provider's client ID.

   * The scopes required for the authentication request.

   * A unique state ID generated by CSP.

   * A redirect URL where the user will be sent after logging in.

3. **Log in Using the Third-Party Provider with a Token**  
   CSP receives an authentication token once the user authorizes via the third-party authorization URL. The `LoginToThirdPartyAuthenticationProvider` method processes this token with the state ID and completes the login.

Below is a simplified example showing how to retrieve supported providers and initiate an SSO login:

```
// Application retrieves supported third-party providers from CSP
csp::common::Array<EThirdPartyAuthenticationProviders> providers =
	UserSystem->GetSupportedThirdPartyAuthenticationProviders();
	
// Application gets authorization URL for the selected provider
UserSystem->GetThirdPartyProviderAuthoriseURL(providers[0], RedirectURL, [](StringResultCallback callback) {
	// Authorisation URL returned
});

// Application redirects user to the third-party authorization URL and retrieves the token once the user has logged in

// Application logs in via CSP in with third-party token
UserSystem->LoginToThirdPartyAuthenticationProvider(ThirdPartyToken, ThirdPartyStateId, Optional<bool>(), LoginStateResultCallback);
```

### Benefits of SSO

* **Single Identity Management**: Users can manage one set of credentials across multiple platforms, simplifying login processes.

* **Reduced Friction**: Users avoid repeatedly entering credentials for different services, improving convenience.

* **Enhanced Security**: Using trusted identity providers ensures secure handling of user credentials.

## Log in as a Guest

Guest Login provides temporary access to services for users who do not have an account. It allows these users to explore the platform without creating login credentials, offering a simple way to experience the platform with limited functionality.

There are some aspects of guest login to be aware of when considering supporting it in your application.

1. **No Need for Username or Email**  
   Unlike standard login, Guest Login does not require a username or email. Instead, the system uses a **Device ID** and **Tenant Information** to identify the user.

2. **Age Verification Requirement**  
   Like other login methods, Guest Login requires users to verify that they are over 18. Users who fail to verify their age will be unable to log in.

3. **Similarities with Standard Login**  
   The Guest Login process follows a structure similar to the standard login flow. However, it simplifies the login request by omitting user-specific credentials and relying only on the user's device and tenant information.

Here's a simplified example of how CSP handles Guest Login:

```
// Construct Guest Login request
auto Request = std::make_shared<chs_user::LoginRequest>();
Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
Request->SetTenant(csp::CSPFoundation::GetTenant());

// Optional: Set age verification if available
if (UserHasVerifiedAge.HasValue()) {
    Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
}
	
// Make the call to log in as guest
static_cast<AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLoginAsGuestPost(Request, ResponseHandler);
```

### Key Points

* **Guest User Management**  
  Guest users are treated as temporary users in CSP. They are not given full account privileges, and their session is often tied to the current device rather than a permanent user profile.

* **Permissions for Guest Users**  
  Guest users have restricted access compared to regular users. They are restricted from creating spaces, they can only enter spaces as a viewer, which means that they cannot edit a space.

## Login with a Refresh Token

Login with a refresh token allows users to maintain their session without repeatedly entering login credentials.

Once authenticated, CSP issues a refresh token that the client can retrieve and store to extend the session or login again. This method improves user convenience and security by managing sessions in the background.

### How It Works

Client applications can register a callback to receive new refresh tokens when a session is created or updated. This is done using the `UserSystem::SetNewLoginTokenReceivedCallback` method. Using the callback, the application can receive and store the token securely, often in persistent storage like the browser's local storage.

The application may then use the `UserSystem::LoginWithRefreshToken` method to authenticate the user using the token, eliminating the need for the user to log in again manually.

Here's a simplified example of how CSP handles Login with a Refresh Token:

```
// Register a callback to receive the refresh token
UserSystem->SetNewLoginTokenReceivedCallback([](const LoginTokenInfo& TokenInfo) {
    // Store the refresh token securely (e.g., in local storage)
    SaveTokenToStorage(TokenInfo.RefreshToken);
});

// Use the refresh token to log in
UserSystem->LoginWithRefreshToken(UserId, StoredRefreshToken, [](LoginStateResultCallback Result) {
    if (Result.Success()) {
		// Successfully logged in with the refresh token
	} else {
        // Handle login failure
	}
});
```

### Key Points

* **Session Continuity**  
  Refresh tokens enable CSP to maintain user sessions without requiring them to log in manually each time, ensuring a seamless experience.
* **Security Considerations**  
  Refresh tokens should be securely stored to prevent unauthorized access. Only trusted applications should handle the storage and retrieval of tokens.

## Log out

Logging out in CSP ends the user's session, disconnecting them from all services, including multi-user features. It ensures the user's credentials are no longer active, protecting their account and preventing unauthorized access.

### How it works

1. **Constructing a Logout Request**  
   Upon log out, CSP first constructs a `LogoutRequest`. This request includes the user's **User ID** and **Device ID** to ensure the correct session is terminated.

2. **Handling responses**  
   The logout request triggers a response handler confirming whether the logout was successful or an error occurred. The response includes feedback on whether the session ended correctly.

3. **Disconnection from multi-user services**  
   CSP also disconnects the user from any active multi-user connections. This step ensures the user is no longer connected to real-time services that require ongoing authentication.

Below is an example of how CSP handles the logout process:

```
void UserSystem::Logout(NullResultCallback Callback) {
    // Check if user is logged in before logging out
    if (CurrentLoginState.State == ELoginState::LoggedIn) {
        // Construct a logout request
        auto Request = std::make_shared<LogoutRequest>();
        Request->SetUserId(CurrentLoginState.UserId);
        Request->SetDeviceId(CurrentLoginState.DeviceId);
        
        // Disconnect from multiplayer services
        MultiplayerConnection->Disconnect(ErrorCallback);
        
        // Handle logout response
        static_cast<AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLogoutPost(Request, ResponseHandler);
	}
}
```

### Key Points

* **Verify Login State**  
  Before attempting to log out, CSP verifies if the user is logged in. If the user is logged out or in an invalid state, the system avoids making unnecessary logout requests.

* **Handling Errors**  
  CSP manages potential errors during logout, providing appropriate feedback to the user if the logout attempt fails. This ensures the process is handled smoothly and securely.

## Summary

In this module, we covered several methods for logging into CSP:

* **Standard Login**: Users login using their username or email and password.

* **Single Sign-On (SSO)**: A convenient method where users can authenticate through third-party providers like Google, Discord, or Apple.

* **Guest Login**: Allows users to access CSP without an account, offering limited functionality.

* **Refresh Token Login**: This keeps users logged in by using stored tokens to re-authenticate without needing to re-enter credentials.

* **Logging Out**: It properly ends a user session by disconnecting them from services, including multiplayer.

### Best Practices

1. **Security First**  
   Always ensure that user credentials and tokens are securely handled. Store refresh tokens securely and avoid exposing them in unsafe environments. For SSO, use trusted identity providers to protect user data.

2. **Session Management**  
   Implement proper session management by regularly refreshing tokens and securely logging out users when needed. Avoid leaving sessions active unnecessarily, especially for shared or public devices.

3. **Maintain User Trust**  
   User trust depends on security. Ensure that age verification, secure logouts, and trusted authentication processes are in place. Always comply with your target regions' relevant data protection laws.

By following these best practices, CSP ensures a secure and efficient login process for all users while maintaining their trust in the platform's integrity.
