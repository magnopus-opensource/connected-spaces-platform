# User Management

Managing users within the Connected Spaces Platform (CSP) is vital for effective access control. The system allows administrators to create new users, reset passwords, and update display names. These actions ensure that only authorized users can access the platform while maintaining a smooth user experience.

## Overview of User Management in CSP

In CSP, user management focuses on creating and maintaining user profiles. Each profile contains personal information like usernames, email addresses, and display names. CSP supports several features to manage these profiles efficiently, such as creating users, resetting passwords, and updating profile information. These features ensure a secure and flexible environment for handling user accounts. The login process is at the core of this, ensuring users authenticate themselves before accessing any services or resources.

## Importance of User Creation, Password Management, and Updating Profile Information

Proper user creation is crucial for CSP. Each user has a unique profile, which controls access to various parts of the platform. Password management, including resetting forgotten or compromised passwords, ensures security. Users can easily recover access without involving technical support. Additionally, updating profile information, such as the display name, allows for a personalized and organized user experience. Together, these aspects make it easier for administrators to manage users and for users to maintain control over their accounts.

## Creating a New User

Creating users in CSP is essential for controlling access and managing user accounts. Each user must have a unique profile to access various features of the platform securely. User creation enables administrators to define who can log in, what resources they can access, and what actions they are authorized to perform.

### Why Creating Users is Critical for Access Control in CSP

In CSP, user creation is at the heart of managing access control. Every user, whether administrators, developers, or guests, needs an account to interact with the platform. Creating users allows administrators to assign permissions, roles, and settings specific to each individual or group. This structure ensures that sensitive data is protected, and users can only access the areas of the platform they are authorized to use.

By defining user roles early, CSP prevents unauthorized access and ensures the system operates securely. Managing user rights would be impossible without proper user creation, leading to security risks and potential system misuse.

### User Creation Methods

CSP offers several methods for creating users, with the primary one being the `CreateUser` method. This method allows administrators to set up new users by providing basic details such as email, display name, and password or by utilizing optional settings for more advanced configurations.

The `CreateUser` method is a key part of the CSP API. It automates the process of generating new user profiles and makes it easy to integrate the system with external services. By calling this method, CSP creates a new profile and securely stores the user’s information.

### Key Arguments Required for User Creation

When creating a new user in CSP, several arguments are needed to define the profile:

* `UserName`: The user's unique identifier within the platform.

* `DisplayName`: A name that will be visible to other users within the platform.

* `Email`: The user's email address, which is used for authentication and communication.

* `Password`: A secure password the user will use to log in.

Optional arguments include:

* `ReceiveNewsletter`: A setting to determine if the user opts in to receive newsletters.

* `HasVerifiedAge`: Confirms if the user has met any age-related requirements.

* `RedirectUrl`: A URL to redirect the user after creation.

* `InviteToken`: An optional token if the user has been invited via an external URL that contains a token.

### Step-by-Step Guide to Creating a User

The `CreateUser` method is used to generate a user profile. Here’s a step-by-step guide to creating a new user:

1\. **Construct the CreateUserRequest Object**  
   * Begin by constructing the `CreateUserRequest` object. This object will hold all the necessary information for the new user.

```
auto Request = std::make_shared<CreateUserRequest>();
```

2\. **Set Up User Details (UserName, DisplayName, Email, Password)**

* Populate the request with essential user details:  
  * If a `UserName` is provided, add it to the request.

  * Add the `DisplayName` to specify the name others will see.

  * Include the `Email` and `Password` to set up login credentials.

```
if (UserName.HasValue()) {
	Request->SetUserName(*UserName);
}

Request->SetDisplayName(*DisplayName);
Request->SetEmail(Email);
Request->SetPassword(Password);
```

3\. **Add Optional Settings (Newsletter Subscription, Age Verification, Redirect URL, Invite Token)**

* Additional optional settings can be added, such as whether the user will receive newsletters, if their age has been verified, or if they should be redirected to a specific URL after account creation.

```
auto InitialSettings = std::make_shared<InitialSettingsDto>();
InitialSettings->SetContext("UserSettings");
InitialSettings->SetSettings({{"Newsletter", ReceiveNewsletter ? "true" : "false"}});
Request->SetInitialSettings({InitialSettings});
```

4\. **Invoke the API to Create the User Profile**

* Finally, call the CSP API to create the user profile, pass the `CreateUserRequest` object, and handle the response.

```
static_cast<ProfileApi*>(ProfileAPI)->apiV1UsersPost(Request, ResponseHandler);
```

Here is a simplified example of the CreateUser method in action:

```
void UserSystem::CreateUser(const Optional<String>& UserName,
                            const Optional<String>& DisplayName,
                            const String& Email,
                            const String& Password,
                            bool ReceiveNewsletter,
                            bool HasVerifiedAge,
                            const Optional<String>& RedirectUrl,
                            const Optional<String>& InviteToken,
                            ProfileResultCallback Callback)
{
	auto Request = std::make_shared<CreateUserRequest>();
	
	if (UserName.HasValue()) {
		Request->SetUserName(*UserName);
	}
	
	Request->SetDisplayName(*DisplayName);
	Request->SetEmail(Email);
	Request->SetPassword(Password);

	auto InitialSettings = std::make_shared<InitialSettingsDto>();
	InitialSettings->SetContext("UserSettings");
	InitialSettings->SetSettings({{"Newsletter", ReceiveNewsletter ? "true" : "false"}});
	Request->SetInitialSettings({InitialSettings});

	static_cast<ProfileApi*>(ProfileAPI)->apiV1UsersPost(Request, ResponseHandler);
}
```

## Resetting Password

### Importance of Password Management

Password management is a critical part of securing user access in CSP. Strong password policies help protect user accounts from unauthorized access. However, users may forget their passwords or need to reset them if they suspect a breach. In these situations, a secure password reset process is essential. It allows users to regain access to their accounts without compromising security, ensuring that passwords remain private and protected from unauthorized parties.

### Password Reset Methods

CSP provides several methods for password management. The two main methods for resetting a password are:

* `ResetUserPassword`: This method is used when users need to change their password after verification (such as receiving a reset link via email).

* `ForgetPassword`: This method allows users to initiate a password reset by requesting a reset link. The system sends an email with instructions on how to reset the password.

### Step-by-Step Guide to Resetting a Password

**Step 1: Confirming the User’s Email**

Before resetting the password, confirm the user’s email address. This step ensures that the reset request comes from the legitimate user. CSP often sends a verification email to the registered email address.

**Step 2: Sending the Password Reset Request**

Use the ResetUserPassword method to initiate the password reset process. The system will trigger a reset email to the user containing a link that allows them to change their password.

```
void UserSystem::ResetUserPassword(const String& Email,
                                   ProfileResultCallback Callback)
{
	auto Request = std::make_shared<ResetPasswordRequest>();
	Request->SetEmail(Email);

	const ResponseHandlerPtr ResponseHandler =
		ProfileAPI->CreateHandler<ProfileResultCallback, ProfileResult, 
								void, chs_user::ProfileDto>(Callback, nullptr, 
								csp::web::EResponseCodes::ResponseOk);

	static_cast<ProfileApi*>(ProfileAPI)->apiV1UsersPasswordResetPost(Request, ResponseHandler);
}
```

**Step 3: Implementing a Password Reset Form or Interface**

Once the user receives the reset email, they click the link to a password reset form. The form typically prompts the user to enter and confirm a new password. Implement a simple and user-friendly interface to ensure users can easily update their passwords without confusion.

**Step 4: Processing the Password Reset**

After the user submits the form with a new password, the system updates the user’s profile with the new credentials. This step ensures the user can log in with the updated password immediately.

## Updating Display Name

Keeping user information accurate and updated is important for a good user experience within CSP. Allowing users to update their display name provides flexibility and personalization. This feature helps users maintain current and relevant profiles, reflecting changes in their roles, preferences, or identities. A personalized display name ensures that users are easily recognizable within the platform, enhancing communication and collaboration.

### Updating Display Name Method

CSP provides a straightforward method for updating the display name: UpdateUserDisplayName. This method is part of the user profile management system and allows users to modify their display name while keeping other profile details intact.

### Step-by-Step Guide to Updating Display Name

Here’s how to update a user’s display name using the `UpdateUserDisplayName` method:

**Step 1: Retrieving User Profile Information**

Before making changes, you must retrieve the user’s current profile information. This can be done using methods like `GetProfileByUserId` to ensure you have the correct user data.

**Step 2: Modifying the Display Name**

Once you have the user’s profile, modify the display name to the desired value. The system allows users to enter a new name that will be shown across the platform.

**Step 3: Sending an Update Request**

After modifying the display name, send the update request using the `UpdateUserDisplayName` method. The system will process the request and apply the changes.

```
void UserSystem::UpdateUserDisplayName(const String& UserId,
                                       const String& NewDisplayName,
                                       ProfileResultCallback Callback)
{
   auto Request = std::make_shared<UpdateDisplayNameRequest>();
   Request->SetUserId(UserId);
   Request->SetNewDisplayName(NewDisplayName);

   const ResponseHandlerPtr ResponseHandler =
      ProfileAPI->CreateHandler<ProfileResultCallback, ProfileResult,
                                void, chs_user::ProfileDto>(Callback, nullptr,
                                csp::web::EResponseCodes::ResponseOk);

   static_cast<ProfileApi*>(ProfileAPI)->apiV1UsersDisplayNamePatch(Request, ResponseHandler);
}
```

## User Profile Management Best Practices

### Security Considerations

To ensure user profile security in CSP, follow these practices:

* **Use Strong Passwords**:  
  * Encourage users to create strong passwords with a combination of uppercase and lowercase letters, numbers, and special characters.
  * Passwords should be unique and changed regularly.

* **Confirm Emails**:  
  * Require users to confirm their email addresses during registration.
  * Verified emails ensure accounts are legitimate and aid in password recovery.

* **Avoid Plain Text Passwords**:  
  * Always store passwords securely using encryption.  
  * Never store passwords in plain text or send them via email.

### User Experience Enhancements

To enhance user experience in managing profiles, consider these strategies:

* **Simplify Password Resets**:  
  * Provide clear, user-friendly interfaces for resetting passwords.
  * Include instructions and validation steps to avoid errors during the process.
  * Notify users via email when their password has been successfully reset.

* **Enable Easy Display Name Updates**:  
  * Make the option to update display names easily accessible.
  * Allow users to personalize their profiles with minimal steps.

* **Provide Immediate Feedback**:  
  * To improve the process flow, offer real-time validation (e.g., strength meters for passwords).
  * Highlight errors when users enter invalid information.

### Compliance and Data Privacy

Ensuring user data protection and compliance with privacy standards is crucial:

* **Follow Global Privacy Regulations**:  
  * Comply with regulations like GDPR to protect personal information.  
  * Allow users to access, update, and delete their data per legal requirements.

* **Encrypt User Data**:  
  * Use encryption to protect sensitive information like emails and passwords.  
  * Ensure data is secure both in transit and at rest.

* **Limit Access**:  
  * Restrict access to user data to only authorized personnel.  
  * Implement role-based access control (RBAC) to protect user information.

## Summary

This topic covered the key aspects of managing user authentication within CSP. It focused on three essential components: creating new users, resetting passwords, and updating display names. Each task is critical for maintaining secure and organized access control in CSP.

User creation is the foundation of access management. By creating unique user profiles, administrators ensure that each user has the proper permissions and access to the necessary resources. It also enables the platform to control who can log in and what actions they are authorized to perform.

Password resets are vital for securing user accounts. They provide a way for users to regain access if they forget their password or their account is compromised. The process ensures that user credentials remain secure while allowing users to restore access quickly.

Updating display names offers flexibility and personalization for users. It allows them to keep their profiles current and maintain a consistent identity across the platform. This enhances communication and collaboration between users.

By managing user creation, password resets, and profile updates efficiently, CSP ensures secure access control, improved user experience, and compliance with privacy standards. These elements form the backbone of CSP’s robust authentication system, ensuring the platform remains secure and user-friendly.