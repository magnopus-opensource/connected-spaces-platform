# Settings

The `SettingsSystem` in the Connected Space Platform (CSP) allows users to manage their preferences and settings across all CSP-enabled applications. It provides a unified way for users to control their experience, ensuring consistency no matter which application they use.

The system stores user settings in the cloud hosted services CSP has been configured to use. These settings are linked to the user's profile and are automatically synchronized across all CSP-enabled applications. 

For example, if a user changes their avatar in one application, the same avatar will appear in all other applications that support CSP. This synchronization guarantees a seamless experience for users, allowing them to move effortlessly between different applications without losing their personalized settings.

The `SettingsSystem` is designed with privacy in mind. CSP focuses on using non-personally identifiable information wherever possible. This approach minimizes the risk of exposing sensitive data and ensures compliance with data protection standards. Settings related to personal information, such as user choices or preferences, are handled securely and stored only when necessary.

## Understanding the Asynchronous Nature of Setting Changes

When a setting is changed in CSP, the process of transmitting and applying the update at the services-level is handled asynchronously. Asynchronous operations allow tasks to run in the background without interrupting the application's main flow.

For example, when a user updates a setting, such as opting into a newsletter, the application sends a request to the server. This request does not block or delay the user's interaction with the application. Instead, it allows the user to continue using the app while the server processes the update.

The asynchronous nature of setting changes is crucial for maintaining application responsiveness and ensuring a smooth user experience. Once the setting change is processed, a callback function confirms the result.

## Step-by-Step Guide to Changing a Setting

Here is a detailed guide on how to change a setting for the current user:

1. **Select the Desired Setting:**   
   Identify the specific setting you want to change. CSP provides several settings that users can modify, such as opting in or out of newsletters, managing recently visited spaces, or updating an avatar.

   Each setting has a dedicated setter function provided by the CSP API, designed to handle the specific type of data or preference being updated.  

2. **Invoke the Corresponding Setter Function:**   
   You need to call the appropriate setter function to change the selected setting. Each setting in CSP has a unique function for setting its value.

   For instance, to change the user's acceptance of a Non-Disclosure Agreement (NDA), use the `SetNDAStatus` function. When calling this function, you must provide the new value (e.g., true or false for a boolean setting) and a callback function to handle the response from the server.  
   
   Example client application code for changing the NDA status:

```
// Define the desired NDA status
bool NDAStatus = true; // The user accepts the NDA

// Access the Settings System
csp::systems::SettingsSystem* SettingsSystem = csp::systems::SystemsManager::Get().GetSettingsSystem();

// Invoke the setter function with a callback
SettingsSystem->SetNDAStatus(NDAStatus, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
		// Handle successful update
		// Notify user or update the interface accordingly
    } else {
		// Handle error
		// Provide feedback or retry mechanism
    }
});
```

In this example, the application invokes the `SetNDAStatus` function to set the new value and uses the provided callback to handle the response from the server once the setting update is processed.

3. **Handle Callbacks to Confirm Changes:**   
   The callback function is crucial in confirming whether the setting change was successful. When the server processes the request, it returns a result indicating success or failure. The callback function can evaluate this result to determine the next steps.

   If the result indicates success (`EResultCode::Success`), the application can proceed with actions such as notifying the user or refreshing the settings interface. If there is an error, the callback function should handle it appropriately, including displaying an error message, providing user guidance, or attempting the change again.  

   Example of handling the callback for a setting change:

    ```
    SettingsSystem->SetNewsletterStatus(true, [](const csp::systems::NullResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
		    // Successful update
		    CSP_LOG_MSG(csp::systems::LogLevel::Log, "Newsletter subscription updated successfully.");
        } else {
		    // Error handling
		    CSP_LOG_ERROR_MSG("Failed to update subscription. Please try again.");
        }
    });
    ```

In this snippet, the callback function checks the result code and provides appropriate feedback. This approach keeps the user informed about the status of their request and enhances the overall user experience.

## Types of Settings Available for Change

CSP offers various settings that users can modify to personalize their experience across all CSP-enabled applications.

These settings help users manage preferences, control privacy, and maintain a consistent experience. Here is a detailed explanation of the available settings, their importance, and the steps to change them.

### 1\. Acceptance of Non-Disclosure Agreement (NDA)

The acceptance of a Non-Disclosure Agreement (NDA) may be required for users to access sensitive information within CSP-enabled applications. Accepting the NDA is critical for protecting confidential data and ensuring compliance with legal requirements.

* **Description and Importance**:  
  The NDA status determines whether a user has agreed to the terms governing sensitive information use and confidentiality. This setting must reflect the user's agreement before accessing restricted content.

* **How to Set NDA Status**:  
  To update the NDA status, use the `SetNDAStatus` function. This function accepts a boolean value (`true` for acceptance, `false` for non-acceptance) and a callback function to handle the response.

  **Example Code to Set NDA Status:**

```
// Define the NDA status to be set
bool NDAStatus = true; // true if the user accepts the NDA

// Access the Settings System
csp::systems::SettingsSystem* SettingsSystem = csp::systems::SystemsManager::Get().GetSettingsSystem();

// Set the NDA status
SettingsSystem->SetNDAStatus(NDAStatus, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
	// Handle successful update
	CSP_LOG_MSG << "NDA status updated successfully." << std::endl;
    } else {
	// Handle error
	CSP_LOG_MSG << "Failed to update NDA status." << std::endl;
    }
	});
```

* **How to Retrieve NDA Status:**  
  To check the NDA status, use the `GetNDAStatus` function. This function triggers a callback that returns the user's current NDA status.

**Example Code to Retrieve NDA Status:**

```
// Get the NDA status
SettingsSystem->GetNDAStatus([](const csp::systems::BooleanResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
		bool NDAStatus = Result.GetValue();
		CSP_LOG_MSG << "Current NDA status: " << (NDAStatus ? "Accepted" : "Not Accepted") << std::endl;
    } else {
		// Handle error
		CSP_LOG_MSG << "Failed to retrieve NDA status." << std::endl;
    }
});
```

### 2\. Newsletter Subscription Status

If your use case involves newsletters, users can subscribe to or unsubscribe from newsletters and updates sent by CSP-enabled applications. This setting helps users control the type and frequency of communications they receive.

* **Details on Opting In or Out**:  
  Opting in means the user will receive regular updates or newsletters, while opting out means they will not receive such communications.

* **Steps to Change the Subscription Status**:  
  Use the `SetNewsletterStatus` function to change the subscription status. Provide a boolean value (`true` to subscribe, `false` to unsubscribe) and a callback to handle the response.

**Example Code to Change Subscription Status:**

```
// Define the subscription status
bool Subscribe = true; // true to subscribe, false to unsubscribe

// Set the newsletter subscription status
SettingsSystem->SetNewsletterStatus(Subscribe, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
		CSP_LOG_MSG << "Subscription status updated successfully." << std::endl;
		CSP_LOG_MSG << "Subscription status updated successfully." << std::endl;
    } else {
		CSP_LOG_MSG << "Failed to update subscription status." << std::endl;
    }
});
```

* **Retrieve Subscription Status:**  
  To check if the user is currently subscribed, use the `GetNewsletterStatus` function. This function calls a callback to return the current status.

**Example Code to Retrieve Subscription Status:**

```
// Retrieve the newsletter subscription status
SettingsSystem->GetNewsletterStatus([](const csp::systems::BooleanResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
		bool Subscribed = Result.GetValue();
		CSP_LOG_MSG << "Newsletter subscription status: " << (Subscribed ? "Subscribed" : "Not Subscribed") << std::endl;
    } else {
		CSP_LOG_MSG << "Failed to retrieve subscription status." << std::endl;
    }
});
```

### 3\. Recently Visited Spaces

CSP tracks the spaces a user has recently visited to provide quick access to these locations. This feature enhances user experience by simplifying navigation.

* **Purpose of Tracking Recently Visited Spaces**:  
  Tracking recently visited spaces allows users to easily return to spaces they frequently use or have recently interacted with. This helps streamline workflows and improve efficiency.

* **Methods to Add, Retrieve, and Clear Recently Visited Spaces**:  
  The following functions can be used to manage recently visited spaces.

  * `AddRecentlyVisitedSpace`: Adds a new space to the top of the recently visited list.

  * `GetRecentlyVisitedSpaces`: Retrieves the list of recently visited spaces, sorted from most to least recent.

  * `ClearRecentlyVisitedSpaces`: Clears the list of all recently visited spaces.

**Example Code to Add a Recently Visited Space:**

```
// Define the ID of the recently visited space
csp::common::String SpaceID = "example_space_id";

// Add the space to the recently visited list
SettingsSystem->AddRecentlyVisitedSpace(SpaceID, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
		CSP_LOG_MSG << "Space added to recently visited list." << std::endl;
    } else {
		CSP_LOG_MSG << "Failed to add space to recently visited list." << std::endl;
    }
});
```

**Example Code to Retrieve Recently Visited Spaces:**

```
| // Retrieve the list of recently visited spaces
SettingsSystem->GetRecentlyVisitedSpaces([](const csp::systems::StringArrayResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        auto RecentlyVisitedSpaces = Result.GetValue();
        CSP_LOG_MSG << "Recently visited spaces:" << std::endl;
        for (const auto& SpaceId : RecentlyVisitedSpaces) {
            CSP_LOG_MSG << SpaceId << std::endl;
        }
    } else {
        CSP_LOG_MSG << "Failed to retrieve recently visited spaces." << std::endl;
    }
}); |
```

**Example Code to Clear Recently Visited Spaces:**

```
// Clear the list of recently visited spaces
SettingsSystem->ClearRecentlyVisitedSpaces([](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        CSP_LOG_MSG << "Recently visited spaces cleared." << std::endl;
    } else {
        CSP_LOG_MSG << "Failed to clear recently visited spaces." << std::endl;
    }
});
```

### 4\. Blocked Spaces

Blocking spaces allows users to avoid unwanted content or interactions, enhancing their experience by filtering out specific spaces.

* **Functionality for Blocking and Unblocking Spaces:**  
  Users can block or unblock spaces as needed.
  * `AddBlockedSpace`: Adds a space to the user's blocked list.

  * `RemoveBlockedSpace`: Removes a space from the blocked list.

  * `GetBlockedSpaces`: Retrieves the list of currently blocked spaces.

  * `ClearBlockedSpaces`: Clears all blocked spaces from the user's list.

**Example Code to Block a Space:**

```
// Define the ID of the space to block
csp::common::String BlockedSpaceID = "blocked_space_id";

// Block the space
SettingsSystem->AddBlockedSpace(BlockedSpaceID, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        CSP_LOG_MSG << "Space blocked successfully." << std::endl;
    } else {
        CSP_LOG_MSG << "Failed to block space." << std::endl;
    }
});
```

**Example Code to Unblock a Space:**

```
// Unblock the space
SettingsSystem->RemoveBlockedSpace(BlockedSpaceID, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        CSP_LOG_MSG << "Space unblocked successfully." << std::endl;
    } else {
        CSP_LOG_MSG << "Failed to unblock space." << std::endl;
    }
});
```


### 5\. User Avatar Configuration

Users can personalize their appearance across CSP-enabled applications by setting their avatars. CSP supports three types of avatars: **Predefined avatars**, **Ready Player Me avatars**, and **Custom avatars**. This flexibility allows users to select an avatar that best represents them digitally.

**Types of Avatar Settings:**

* **Predefined**: A set of avatars provided by the application. Users can choose from these built-in options to represent themselves.

* **Ready Player Me**: Avatars created through the third-party platform [Ready Player Me](https://readyplayer.me/). Users can generate a custom 3D avatar and link it to their CSP profile.

* **Custom**: Any cloud-hosted 3D model that the user chooses. This option allows for maximum personalization by enabling the use of unique, user-generated models.

**Steps to Set Avatar Information:**

To set or update a user's avatar, use the `SetAvatarInfo` function. Provide the type of avatar (`Predefined`, `ReadyPlayerMe`, or `Custom`) and the corresponding identifier (such as a URL or an ID) to specify the avatar.

**Example Code to Set Avatar Information:**

```
// Define the avatar type and identifier
csp::systems::AvatarType AvatarType = csp::systems::AvatarType::ReadyPlayerMe;
csp::common::String AvatarIdentifier = "https://models.readyplayer.me/example_avatar.glb";

// Set the avatar information
SettingsSystem->SetAvatarInfo(AvatarType, AvatarIdentifier, [](const csp::systems::NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        CSP_LOG_MSG << "Avatar information updated successfully." << std::endl;
    } else {
        CSP_LOG_MSG << "Failed to update avatar information." << std::endl;
    }
});
```

In this example, the avatar is set to a model from `ReadyPlayerMe`, identified by a URL. The callback confirms whether the operation was successful.

**Steps to Retrieve Avatar Information:**

To retrieve a user's current avatar information, use the `GetAvatarInfo` function. This function returns the avatar type and identifier through a callback.

**Example Code to Retrieve Avatar Information:**

```
// Retrieve the user's avatar information
SettingsSystem->GetAvatarInfo([](const csp::systems::AvatarInfoResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        auto AvatarInfo = Result.GetValue();
        CSP_LOG_MSG << "Current avatar type: " << AvatarInfo.Type << std::endl;
        CSP_LOG_MSG << "Avatar identifier: " << AvatarInfo.Identifier << std::endl;
    } else {
        CSP_LOG_MSG << "Failed to retrieve avatar information." << std::endl;
    }
});
```

This code snippet retrieves the avatar type (e.g., `Predefined`, `ReadyPlayerMe`, or `Custom`) and its identifier (such as an ID or URL). It provides this information to the user for review or further action.

## Summary

The Settings System in the Connected Space Platform (CSP) enables users to personalize their experience across all CSP-enabled applications. Users can change key settings, such as NDA acceptance, newsletter subscriptions, recently visited spaces, blocked spaces, and avatar information. 

Each setting is stored securely in the cloud and synchronized across applications, ensuring changes are applied consistently. CSP itself stores no personal data.

Using the Settings System is essential for creating a good user experience. By correctly managing these settings, users can maintain their preferences, control their interactions, and ensure a cohesive experience, no matter which CSP-enabled application they use. 

This focus on personalization and consistency enhances user satisfaction and engagement across the platform.
