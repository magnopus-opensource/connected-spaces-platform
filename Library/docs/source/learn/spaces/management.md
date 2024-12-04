# Space Management

In CSP, a Space represents a virtual environment where users can interact, collaborate, and engage in multi-user experiences. 

A Space is a container that hosts visual and interactive elements in a virtual environment. It can include various objects, models, avatars, scripts, and other components that create a unique experience for users. Spaces are highly customizable and can be configured to support many different use cases, such as public events, private meetings, or location-based experiences.

## Importance of Spaces in Creating a Multi-User Experience

Spaces are an essential part of multi-user experiences in CSP. 

1. They provide a shared digital environment where multiple users can interact simultaneously. 

2. In a Space, users can communicate, collaborate, and perform activities in real-time, making it ideal for hosting virtual meetings, gaming sessions, social events, and more.

3. The flexibility and dynamic nature of Spaces allow developers to create tailored experiences that meet specific needs, whether it's a small team collaboration or a large-scale public event.

## Creating a New Space

To create a new Space in the Connected Spaces Platform (CSP), use the CreateSpace method or the CreateSpaceWithBuffer method. These methods allow you to define the essential attributes of a Space, such as its name, description, and thumbnail image. Here's how to use each method:

### Steps to Create a New Space Using the CreateSpace Method

1. **Access the Space System**: Retrieve the Space System from the Systems Manager in CSP using:

    ```
    SpaceSystem* spaceSystem = SystemsManager::GetSpaceSystem();
    ```

2. **Define the Space Attributes**: Set the Space's name, description, properties (like discoverability), and invite details:

    ```
    CreateSpace(
        const csp::common::String& Name, 
        const csp::common::String& Description, 
        SpaceAttributes Attributes, 
        const csp::common::Optional<InviteUserRoleInfoCollection>& InviteUsers, 
        const csp::common::Map<csp::common::String, csp::common::String>& Metadata, 
        const csp::common::Optional<csp::systems::FileAssetDataSource>& FileThumbnail, 
        SpaceResultCallback Callback);
    ```

    * **Name**: The Space name.
    
    * **Description**: A short description of the Space.
    
    * **Attributes**: Defines if the Space is public, private, or geolocated.
    
    * **InviteUsers**: (Optional) List of users to invite upon creation.
    
    * **Metadata**: Key-value pairs to store additional information.

3. **Call the `CreateSpace` Method**: Use the method to create the Space, specifying all required attributes.

### Explanation of the `CreateSpaceWithBuffer` Method

The `CreateSpaceWithBuffer` method is similar to `CreateSpace`, but it handles the Space thumbnail differently. A thumbnail is a data buffer that holds the thumbnail image. Instead of accepting a file path for the thumbnail, it accepts a data buffer. 

Use this method when the thumbnail is not stored as a file but needs to be uploaded directly from a memory buffer:

```
CreateSpaceWithBuffer(
    const csp::common::String& Name, 
    const csp::common::String& Description, 
    SpaceAttributes Attributes, 
    const csp::common::Optional<InviteUserRoleInfoCollection>& InviteUsers, 
    const csp::common::Map<csp::common::String, csp::common::String>& Metadata, 
    const csp::systems::BufferAssetDataSource& Thumbnail, 
    SpaceResultCallback Callback);
```

### Differences Between `CreateSpace` and `CreateSpaceWithBuffer`

* **Thumbnail Handling**:  
  * `CreateSpace` accepts a `FileAssetDataSource` for thumbnail images, which uploads a file based on a specified file path.

  * `CreateSpaceWithBuffer` uses a `BufferAssetDataSource` to upload a thumbnail from a data buffer.

* **Use Case**:  
  * Use `CreateSpaceCreateSpace` when you have a file path for the thumbnail.  
  * Use `CreateSpaceWithBuffer` when the thumbnail is stored in memory.

### Handling Space Thumbnails with `FileAssetDataSource` and `BufferAssetDataSource`

When creating a new Space in CSP, you can include a thumbnail image to visually represent the Space. CSP provides two ways to handle thumbnail images: `FileAssetDataSource` and `BufferAssetDataSource`. Both options enable you to upload a thumbnail, but they differ in how the image is provided.

`FileAssetDataSource` is used when the thumbnail image is stored as a file on your system. This method uploads the thumbnail based on a specified file path.

**Steps to Use `FileAssetDataSource`**:

1. **Create the `FileAssetDataSource` Object**:  
    Define the file path of the thumbnail image you want to use.

    ```
    csp::systems::FileAssetDataSource fileThumbnail("path/to/image.png");
    ```
    
    * Replace `path/to/image.png` with the actual path to the image file.  
    
2. **Pass the `FileAssetDataSource` to the `CreateSpace` Method**:
    When creating a new Space, use the fileThumbnail object as the thumbnail parameter in the CreateSpace method:

    ```
    spaceSystem->CreateSpace("SpaceName", "SpaceDescription", Attributes, InviteUsers, Metadata, fileThumbnail, Callback);
    ```

### Using `BufferAssetDataSource`

`BufferAssetDataSource` is used when the thumbnail image is stored in memory as a data buffer instead of a file. This method allows you to upload the thumbnail directly from a buffer, which is useful when the image is generated dynamically or received from an external source.

**Steps to Use `BufferAssetDataSource`:**

1. **Create the `BufferAssetDataSource` Object**:    
    Define the buffer containing the image data and the buffer length.

    ```
    csp::systems::BufferAssetDataSource bufferThumbnail(imageBuffer, bufferLength);
    ```
    
    * `imageBuffer` is a pointer to the memory buffer containing the image data.  
    * `bufferLength` is the size of the buffer in bytes.
    
2. **Pass the `BufferAssetDataSource` to the `CreateSpaceWithBuffer` Method**:  
    Use the `bufferThumbnail` object in the `CreateSpaceWithBuffer` method to upload the image directly from the buffer:
    
    ```
    spaceSystem->CreateSpaceWithBuffer("SpaceName", "SpaceDescription", Attributes, InviteUsers, Metadata, bufferThumbnail, Callback);
    ```

### Key Differences Between `FileAssetDataSource` and `BufferAssetDataSource`

* **Source of Image**:  
  * `FileAssetDataSource` uses a file path to locate the image on disk.  
  * `BufferAssetDataSource` uses a memory buffer, which is suitable for dynamic or in-memory images.  
* **Use Cases**:  
  * Use `FileAssetDataSource` when the image is already saved as a file, and you have the file path.  
  * Use `BufferAssetDataSource` when the image is generated or provided in memory without being saved as a file.  
* **Flexibility**:  
  * `FileAssetDataSource` is straightforward to use when you have direct access to the image file.  
  * `BufferAssetDataSource` provides more flexibility for handling images that are not stored as files, such as images generated at runtime or received from other services.

## Setting Space Properties

Configuring Space properties is an essential step when creating and managing Spaces in CSP. These properties define how a Space is discovered, accessed, and visually represented. Understanding these properties helps you create effective, customized Spaces that meet your application's requirements.

CSP uses two key attributes, `IsDiscoverable` and `RequiresInvite`, to define the discoverability and access control of a Space:

* `IsDiscoverable`: Indicates whether a Space is visible to users. If `true`, anyone can search for and find the Space. If `false`, only users who know the Space ID can access it.

* `RequiresInvite`: Specifies whether an invitation is needed to enter the Space. If `true`, only users with an invite can join the Space, regardless of whether it is discoverable.

### Combinations of IsDiscoverable and RequiresInvite Attributes and Their Effects

By combining `IsDiscoverable` and `RequiresInvite`, you can create different types of Spaces with varying levels of visibility and access control:

` IsDiscoverable = true, RequiresInvite = true`

* The Space is visible to everyone, but only invited users can enter.

* Useful for semi-public events where the Space should be found easily, but participation is controlled.

`IsDiscoverable = false, RequiresInvite = true`

* The Space is hidden and can only be found by those who know the Space ID. An invite is required to enter.

* Ideal for confidential or exclusive gatherings where privacy is critical.

`IsDiscoverable = true, RequiresInvite = false`

* The Space is visible to everyone, and anyone can enter without an invite.

* Suitable for open, public events where you want to encourage broad participation.

`IsDiscoverable = false, RequiresInvite = false`

* The Space is hidden and can only be accessed by those who know the Space ID. No invite is needed to enter.

* Best for private Spaces where only a select group of users should know about and access the Space.

## Geolocated Spaces

Geolocated Spaces are Spaces tied to specific geographic locations. You can define a geofence around a Space to limit access to users within a particular area.

### How to Create and Manage Geolocated Spaces

1. **Define the Geolocation Attributes**: Use an array of `GeoLocation` objects to define the boundaries of the geofence.

2. **Update or Delete Geolocation Information**: Invoke the `UpdateSpaceGeoLocation` and `DeleteSpaceGeoLocation` methods to modify or remove the geolocation settings for a Space.

Geolocated Spaces are useful for location-based events or activities, such as local meetups, site-specific collaborations, or augmented reality experiences tied to a real-world location.

## Managing Spaces

Managing Spaces in CSP involves creating, editing, and deleting Spaces to meet specific needs. This section covers how to delete and edit a Space, manage its members, and update its properties to ensure it aligns with your application's requirements.

### Deleting a Space

Only the owner of a Space can delete it; you cannot delete Spaces created by others. To permanently remove a Space from CSP, use the `DeleteSpace` method. This method permanently deletes the Space, including all its data and settings.

1. **Access the Space System**:  
    Retrieve the Space System from the Systems Manager in CSP:
    
    ```
    SpaceSystem* spaceSystem = SystemsManager::GetSpaceSystem();
    ```

2. **Call the `DeleteSpace` Method**:  
Use the `DeleteSpace` method, providing the required attributes:

    ```
    spaceSystem->DeleteSpace("SpaceId", Callback);
    ```
    
    Replace `"SpaceId"` with the unique identifier of the Space you want to delete.
    
**Required Attributes and Handling Callbacks for Deletion:**
    
* **`SpaceId`:** The unique ID of the Space to delete.

* **`Callback`**: A `NullResultCallback` to handle the result of the delete operation. This callback provides a `NullResult` object that indicates the status of the request (e.g., success, in progress, or failed).

Example of handling the callback:

```
const NullResultCallback MyCallback = [](const NullResult& Result) {
    if (Result.GetResultCode() == EResultCode::Success) {
        // Deletion was successful
    } else if (Result.GetResultCode() == EResultCode::Failed) {
        // Handle failure
    }
};
```

### Editing a Space

You can edit various properties of a Space in CSP to update its name, description, members, and other attributes.

**Overview of Editable Space Properties:**

1. **Name and Description**:  
    The Space's name and description provide essential information and context to users.

2. **Metadata**:  
    Custom key-value pairs that store additional information about the Space.

3. **Geolocation**:  
    The geographic boundaries or location-based settings of a Space.

4. **Member Roles**:  
    User roles within the Space, such as Owner, Moderator, or User.

5. **Ban List**:  
    A list of users banned from accessing the Space.

### Methods to Update Space Attributes

1. **Update the Name, Description, and Metadata**:  
    Use the `UpdateSpace` method to modify basic Space information:
    
    ```
    spaceSystem->UpdateSpace("SpaceId", "NewName", "NewDescription", Metadata, Callback);
    ```
    
    * `SpaceId`: The ID of the Space to update.
    * `NewName`: The new name for the Space.
    * `NewDescription`: The new description for the Space.
    * `Metadata`: Key-value pairs for custom data.

2. **Update Space Members and Their Roles**:  
    Use specific methods to manage Space members:
    * ``InviteToSpace``: Add new members by sending invitations.    
    * `AddUserToSpace`: Directly add a user to the Space.    
    * `RemoveUserFromSpace`: Remove a user from the Space
    * `UpdateUserRole`: Change the role of an existing member (e.g., Owner, Moderator, User).

3. **Update the Space's Geolocation Information**:  
    Manage the geolocation of a Space using `UpdateSpaceGeoLocation` or `DeleteSpaceGeoLocation` methods:
    * `UpdateSpaceGeoLocation`: Define or modify the geolocation attributes of a Space using an array of `GeoLocation` objects.
    * `DeleteSpaceGeoLocation`: Remove the geolocation settings for a Space if it no longer requires location-based access.

4. **Work with the Space's Ban List**:  
    Manage the list of banned users to control access to the Space:
    * `AddUserToSpaceBanList`: Add a user to the ban list, preventing them from entering the Space.
    * `DeleteUserFromSpaceBanList`: Remove a user from the ban list, allowing them to access the Space again.

## Querying for Spaces

Querying for Spaces in CSP allows you to retrieve information about existing Spaces based on various criteria. The SpaceSystem provides several methods to search and filter Spaces by user ID, Space ID, and specific attributes. This section explains how to use these methods and handle the results effectively.

To query Spaces, first access the `SpaceSystem` singleton from the Systems Manager in CSP. This allows you to use different methods to search for Spaces.

**Accessing the SpaceSystem:**

```
csp::systems::SystemsManager& systemManager = csp::systems::SystemsManager::Get();
csp::systems::SpaceSystem* spaceSystem = systemManager.GetSpaceSystem();
```

Once you have the SpaceSystem, you can use various methods to query for Spaces based on different criteria.

### Methods to Retrieve Spaces Based on Different Criteria

1. **Retrieve All Spaces for the Current User:**  
    To get all Spaces associated with the currently logged-in user, use the `GetSpaces` method. Note that the `resultHandler` is a callback function that processes the query results.  
    
    ```
    spaceSystem->GetSpaces(resultHandler);
    ```

2. **Retrieve a Single Space by Space ID**:  
    To find a specific Space using its unique identifier, use the `GetSpace` method:
    
    ```
    spaceSystem->GetSpace("SpaceId", resultHandler);
    ```

3. **Retrieve All Spaces Associated with a Specific User ID**:  
    To get all Spaces linked to a particular user, use the `GetSpacesForUserId` method. This operation requires elevated privileges:
    
    ```
    spaceSystem->GetSpacesForUserId("UserId", resultHandler);
    ```

  Replace `"UserId"` with the user's alphanumeric ID.

4. **Retrieve Spaces Based on Specific Attributes**:  
    To filter Spaces by attributes such as discoverability or invitation requirements, use the `GetSpacesByAttributes` method:
    
    ```
    spaceSystem->GetSpacesByAttributes(
        true,  // isDiscoverable
        false, // isArchived
        true,  // requiresInvite
        0,     // resultsSkip
        10,    // resultsMax
        resultHandler
    );
    ```
    
    * `isDiscoverable`: Set to `true` to find publicly listed Spaces.
    
    * `isArchived`: Set to `true` to search for archived Spaces.
    
    * `requiresInvite`: Set to `true` to find Spaces that require an invitation to enter.

### Pagination in Querying Results

Pagination helps manage large sets of query results by breaking them into smaller, manageable batches. This is particularly useful when dealing with many Spaces.

* `resultsSkip`: Specifies the number of results to skip. For example, if set to 5, the query will skip the first 5 results.

* `resultsMax`: Defines the maximum number of results to return. For instance, if set to 10, the query returns a maximum of 10 results.

To retrieve additional results, adjust the resultsSkip and resultsMax values in subsequent queries. 

### Handling Results from Queries (Success, In Progress, or Failure Scenarios)

When querying for Spaces, the SpaceSystem uses a callback function to handle the results. The callback function will process the query status, which can be **success**, **in progress**, or **failure**.

**Example of Handling Results:**

1. **Success**:  
    If the query succeeds, process the retrieved Spaces. For example:
    
    ```
    auto resultHandler = [](const SpacesResult& result) {
        if (result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Handle successful query
            for (int i = 0; i < result.GetSpaces().Size(); i++) {
                CSP_LOG_MSG << "Space found: " << result.GetSpaces()[i].Id << std::endl;
            }
        }
    };
    ```   

2. **In Progress**:  
    If the query is still processing, you can provide feedback or track progress:
    
    ```
    else if (result.GetResultCode() == csp::systems::EResultCode::InProgress) {
        CSP_LOG_MSG << "Progress: " << result.GetRequestProgress() << std::endl;
    }
    ```

3. **Failure**:  
    If the query fails, handle the error and provide appropriate feedback:
    
    ```
    else {
        CSP_LOG_MSG << "Error: " << ToString(result.GetFailureReason()) << std::endl;
    }
    ```

## Summary

In this topic, you have learned the core concepts and techniques for managing spaces in CSP. You now understand how to create, configure, and maintain Spaces to meet different requirements, making them a vital part of any multi-user or collaborative experience.

1. **Creating Spaces**: You can create new Spaces using methods like `CreateSpace` and `CreateSpaceWithBuffer`. These methods allow you to define essential properties, including name, description, access control, and visual thumbnails. You have also learned how to choose the appropriate method for handling Space thumbnails using `FileAssetDataSource` or `BufferAssetDataSource`.

2. **Managing Spaces**: You know how to manage Spaces effectively by editing key attributes such as name, description, and metadata. You have seen how to update geolocation information, manage members, assign roles, and control access by using ban lists. These techniques enable you to maintain the integrity and relevance of your Spaces.

3. **Querying for Spaces**: You have learned how to query Spaces using the SpaceSystem. You can retrieve Spaces based on different criteria, such as user ID, Space ID, or specific attributes. You also understand the importance of pagination for handling large sets of results and how to manage query outcomes, whether they are successful, in progress, or have failed.
