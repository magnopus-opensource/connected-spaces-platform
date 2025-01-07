# Assets

In CSP, an **asset** is any resource used to create and enhance virtual spaces. Assets include files such as 3D models, textures, images, sounds, and scripts. These assets are essential for building immersive, interactive environments.

An asset is a digital resource that adds content or functionality to a space. In CSP, assets help define the visual and interactive elements, enabling users to populate spaces with engaging components. Assets are managed by **asset collections**, making it easier to organize and access them.

## The Role of Assets in Building Rich, Interactive Spaces

Assets form the foundation of any Space in CSP. They bring visual elements to life and add interactivity. For instance, 3D models represent objects within a Space, video assets can be applied to surfaces to create dynamic visuals, and scripts control how objects behave when users interact with them. By combining various assets, you can create detailed environments where users can explore, interact, and engage with each other in real-time.

## Types of Assets and Their Use Cases

There are several types of assets in CSP, each with specific uses:

* **3D Models:** These are used to create objects and environments in a space. They can be static or animated, allowing for dynamic interactions.

* **Images:** These assets are used for displaying visuals like posters or other images on surfaces, such as quad meshes, within a Space.

* **Sounds:** Audio files, such as music or sound effects, enhance the immersive experience by adding an auditory layer to the environment.

* **Scripts:** These assets define how objects behave or interact with users. Scripts control everything from simple button actions to complex animations.

Each asset type plays a unique role in crafting an interactive, engaging space that responds to user input and enhances the overall experience.

## Overview of Asset Management and Its Importance in a Multi-user Environment

Asset management in CSP involves organizing, creating, and updating assets to ensure they are readily available for use in spaces. In multi-users environments, managing assets efficiently is critical for smooth, synchronized gameplay. When assets are properly managed:

* **Performance Improves:** Optimized assets reduce load times and prevent lag, making the space run smoothly across multiple clients.

* **Consistency is Ensured:** Proper asset management ensures that all users experience the same space with identical assets, preventing discrepancies in what users see and interact with.

* **Scalability Increases:** Managing assets well allows for easier updates and expansions, enabling the space to grow without performance issues.

## Asset CRUD Operations

In CSP, managing assets involves four key actions: **Create, Read, Update, and Delete** (CRUD). These operations are essential for organizing, maintaining, and modifying assets within a connected space.

### Creating an Asset Collection

An asset collection is a group of related assets that can be managed together. The process begins by creating a collection where assets will be stored.

**Step-by-Step Guide:**

* **Step 1:** Prompt the user to enter a unique name for the asset collection.  
* **Step 2:** Use the CreateAssetCollection() function to generate the collection.  
* **Step 3:** Confirm the collection was successfully created.

```
void CreateAssetCollection(const std::string& AssetCollectionName) {
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->CreateAssetCollection(CurrentSpaceId, nullptr, AssetCollectionName.c_str(),
    nullptr, csp::systems::EAssetCollectionType::DEFAULT, nullptr,
    [&](const csp::systems::AssetCollectionResult Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            auto AssetCollection = Result.GetAssetCollection();
            cout << "Created a new Asset Collection called " << AssetCollection.Name
            << ". ID: " << AssetCollection.Id << endl;
        }
        else
        {
            cout << "Error: Could not create a new Asset Collection. " << Result.GetResponseBody() << endl;
        }
    });
}
```

**Explanation:**  
This function prompts the user to provide a unique name for the asset collection. After receiving the input, the system attempts to create a new asset collection. The success or failure of the operation is then displayed.

### Creating an Asset

Assets, such as images or models, must be created within an asset collection. The following steps outline the asset creation process.

**Step-by-Step Guide:**

* **Step 1:** Request a unique name for the asset.  
* **Step 2:** Call the CreateAsset() function to add the asset to the collection.  
* **Step 3:** Confirm the asset creation.

```
void CreateAsset(const std::string& AssetName) {
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->CreateAsset(AssetCollection, AssetName.c_str(), nullptr, nullptr,
    csp::systems::EAssetType::IMAGE, [&](const csp::systems::AssetResult Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            auto Asset = Result.GetAsset();
            cout << "Created a new Asset called " << Asset.Name
            << ". ID: " << Asset.Id << endl;
        }
        else
        {
            cout << "Error: Could not create a new Asset. " << Result.GetResponseBody() << endl;
        }
    });
}
```

**Explanation:**  
This code creates an asset within the existing asset collection. It prompts the user for a name and defines the asset type. After creation, the asset's ID and name are displayed to the user.

### Uploading an Asset

Uploading allows users to add data (e.g., images or 3D models) to the newly created asset. The following example demonstrates how to upload an asset from a file.

**Step-by-Step Guide:**

* **Step 1:** Define the file path for the asset.  
* **Step 2:** Create an asset data source from the file.  
* **Step 3:** Upload the asset data using the UploadAsset() function.

```
void UploadAsset()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();
    //Create Asset Data Source from file
    string SolutionPath = _SOLUTION_DIR;
    filesystem::path FilePath = std::filesystem::absolute(SolutionPath + "TestAsset/TestImage.png");
    
    csp::systems::FileAssetDataSource AssetDataSource;
    AssetDataSource.FilePath = FilePath.u8string().c_str();
    AssetDataSource.SetMimeType("image/png");
    //Upload Asset
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->UploadAssetData(AssetCollection, Asset, AssetDataSource, [&](const csp::systems::UriResult& Result)
    {
        if(Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            cout << "nUploaded Test Asset from path: " + AssetDataSource.FilePath << endl;
            CallbackPromise.set_value();
        }
        else
        {
            cout << "nError: Could not upload Test Asset. " + Result.GetResponseBody() << endl;
            CallbackPromise.set_value();
        }
    });
    CallbackFuture.wait();
}
```

**Explanation:**  
This function handles the file upload process. It first defines the file path, assigns the correct MIME type, and uploads the asset data to the server. Once completed, it confirms the upload or shows an error message.

### Deleting an Asset

Assets can be deleted from a collection if they are no longer needed. The following example demonstrates the deletion process.

**Step-by-Step Guide:**

* **Step 1:** Call the DeleteAsset() function.  
* **Step 2:** Confirm that the asset has been successfully deleted.

```
void DeleteAsset() {
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->DeleteAsset(AssetCollection, Asset, [&](const csp::systems::NullResult Result)
    {
        if(Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            cout << "nDeleted Asset called " + Asset.Name + ". ID: " + Asset.Id << endl;
            CallbackPromise.set_value();
        }
        else
        {
            cout << "nError: Could not delete Asset. " + Result.GetResponseBody() << endl;
            CallbackPromise.set_value();
        }
    });
    
    CallbackFuture.wait();
}
```

**Explanation:**

The DeleteAsset() function removes an asset from the collection. After the function is executed, the user is informed whether the deletion was successful or if there were any issues.

## Understanding AssetCollections

An AssetCollection in CSP serves a dual purpose. While its name suggests it might exclusively represent a collection of assets, it is also designed to store **generic data** in the MCS.

Although the name "AssetCollection" emphasizes its asset-related role, its metadata-centric design supports a broader application. You should consider both its **asset grouping** and **generic data storage** functionalities when integrating AssetCollections into your projects.

### Purpose and Flexibility

1. **Asset Management**:  
   AssetCollections group and manage multiple assets such as images, models, and textures required for virtual environments. This many-to-one relationship is defined at the asset level, where a single AssetCollection can hold multiple associated assets, but an asset is tied to only one AssetCollection.

2. **Data Storage**:  
   AssetCollections also store non-asset data through their metadata. This enables applications to store key-value pairs in a flexible manner, making AssetCollections suitable for systems that do not rely on physical assets.

### Key Features

* **Metadata Usage**:  
  The Metadata property of an AssetCollection allows developers to store structured data as key-value pairs. This is particularly useful for features that require hierarchical or relational data structures.

* **Example Use Case**:  
  The **Comments System** illustrates the flexibility of AssetCollections for data storage.  
  * **Original Comments**: Represented by an AssetCollection with the type COMMENT_CONTAINER. The comment's message is stored within the metadata.

  * **Replies**: Stored as child AssetCollections with the type COMMENT, linked to the parent comment via the ParentId field. This establishes a hierarchy for threaded discussions.

### How Asset Collections are Used to Group and Manage Multiple Assets

Asset collections allow users to group related assets for easier management. For example, in a multi-users space, all textures and models for a particular scene might be stored in a single asset collection. This approach allows developers to:

* **Create New Assets:** Assets can be created within a specific collection, ensuring they are logically grouped.

* **Upload and Update Assets:** When updates are needed, asset collections simplify the process by allowing bulk operations. For instance, if several assets need updates, developers can easily identify and modify them within their collection.

* **Query Assets:** When querying for assets, users can search within specific collections rather than across all assets, improving performance and reducing complexity.

## Thumbnail Generation

**Thumbnail generation** in CSP creates small, visual previews of assets. These thumbnails provide a quick and easy way to identify assets without loading the full version, making asset management more efficient.

### Overview of Thumbnail Generation for Assets

A thumbnail is a reduced-size image that represents an asset, such as a 3D model or texture. Thumbnails are automatically generated or manually uploaded to give users a visual reference for assets within an asset collection. This visual representation helps browse, search, and organize assets, especially in large projects with numerous files.

Thumbnails in CSP are typically used for images, models, and other visual resources, allowing users to recognize their content at a glance.

### Use Cases for Thumbnail Images in Asset Management

Thumbnails are useful in various asset management scenarios:

* **Quick Identification:** Thumbnails provide a fast way to recognize assets without loading the entire file. This is particularly helpful when dealing with a large number of assets.

* **Efficient Browsing:** In an asset collection, having thumbnails for each item allows users to visually browse through assets and save time.

* **Improved Search Results:** When querying for assets, thumbnails give context to search results, making it easier to find the correct asset.

* **User-Friendly Interfaces:** Thumbnails improves the user interface by offering a more intuitive experience when interacting with assets. Instead of reading file names, users can identify assets through visual previews.

### How to Generate Thumbnails Using FileAssetDataSource or BufferAssetDataSource

In CSP, you can generate thumbnails using either FileAssetDataSource or BufferAssetDataSource. These methods allow you to specify the thumbnail image in different formats.

1. FileAssetDataSource: This method uses a file path to upload a thumbnail image directly from a local or remote file. It is suitable when the thumbnail image exists as a separate file and can be easily referenced.  
   **Example:**

```
csp::systems::FileAssetDataSource Thumbnail;
Thumbnail.FilePath = "path/to/thumbnail.png";
Thumbnail.SetMimeType("image/png");
```

In this example, the thumbnail is loaded from the specified file path and assigned the correct MIME type. The system will use this image to represent the asset.

2. BufferAssetDataSource: This method generates a thumbnail from a buffer of image data rather than a file. It is useful when the image data is already in memory or if you are processing image data on the fly.

**Example:**

```
csp::systems::BufferAssetDataSource ThumbnailBuffer;
ThumbnailBuffer.Buffer = imageData;
ThumbnailBuffer.BufferLength = sizeof(imageData);
ThumbnailBuffer.SetMimeType("image/png");
```

Here, the image data is provided directly from a buffer. The system processes the buffer and generates the thumbnail accordingly.

Both methods allow for efficient thumbnail generation, ensuring that assets in the system are easily identifiable. Using either FileAssetDataSource or BufferAssetDataSource, you can control how thumbnails are created and displayed, streamlining asset management within CSP.

In summary, thumbnail generation enhances the user experience by making assets visually accessible and easier to manage in large-scale projects.

## Automated Model Decimation Service

### Introduction to the Automated Model Decimation Service

The **automated model decimation service** in MCS reduces the complexity of 3D models by lowering their polygon count. This process is known as **model decimation** and helps optimize asset performance without sacrificing visual quality.

Model decimation is crucial in projects that use complex 3D models in a real-time application. The automated decimation service in CSP automatically reduces the size of 3D models, making them more manageable for real-time rendering. This service ensures that the models maintain an acceptable level of detail while reducing the data size, leading to better performance in the virtual space.

### Benefits of Model Decimation for Optimizing Asset Performance

Optimizing 3D models through decimation brings several key benefits:

* **Improved Performance:** Decimated models use fewer resources, leading to faster rendering and smoother interactions, especially in environments with many assets.

* **Reduced Load Times:** Smaller models load quicker, reducing wait times for users and improving the overall experience.

* **Optimized for Multi-users:** In multi-users environments, decimation reduces the bandwidth needed to send models to multiple clients, ensuring a seamless experience for all users.

* **Memory Efficiency:** Decimated models take up less memory, making it possible to use more assets without overwhelming the system.

## Relating Asset Collections to CSP Components

In CSP, assets and AssetCollection are associated with various components within a space to bring visual and interactive elements to life. Components are the building blocks of a space, and assets such as 3D models, textures, and sounds are attached to these components to create rich, immersive environments.

### How AssetCollections Are Linked to Specific Components Within a Space

AssetCollection in CSP do not inherently reference the assets associated with them. Instead, an Asset explicitly stores the Id of the AssetCollection it belongs to. This design ensures a unidirectional relationship where assets know their respective AssetCollection, but AssetCollections do not maintain references to their associated assets.

When assets are linked to components, the Asset Id and AssetCollection Id are passed to the component. These identifiers are then used to retrieve the asset data. The asset itself does not store the actual data but provides a **URL** pointing to the asset's storage. The data (such as a 3D model mesh or sound file) is then downloaded directly from the storage.

**Examples of Component Types That Can Use Assets**

Various components within a CSP space rely on assets to function. Here are a few examples:

* **Static Models:** These are 3D objects that remain stationary in a space. Assets such as 3D geometry and textures are linked to static model components to define their appearance.

* **Skeletal Models:** CSP supports skeletal meshes, which include a rigged skeleton structure for animation. The Skeletal Animation data is stored within the same .glb file as the skeletal mesh. Unlike traditional animation systems, CSP does not support keyframe animation on transforms. Instead, skeletal animation relies entirely on the rig structure within the mesh file.

* **Sound Components:** Sound components enable audio interactions within a space. These components use Asset Ids to reference sound files stored in blob storage.

* **Script Components:** Interactive elements, such as buttons, often use scripts that are associated with assets to define their behavior and interaction logic.

## Querying for Assets

Querying for assets in CSP allows users to search, retrieve, and manage assets within a space. CSP provides flexible methods for querying assets based on various criteria, ensuring that users can efficiently locate the assets they need.

### Methods for Querying Assets Within a Space

To manage assets effectively, you can query them based on different attributes like asset type, collection, or space. CSP offers several built-in methods to handle these queries.

1. **Querying All Assets in a Collection:** This method retrieves all assets within a specific AssetCollection. It is useful when you need to list or manage all assets grouped together in the same collection.

   **Example Code:**

```
csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

AssetSystem->GetAssetsInCollection(AssetCollection, [&](const csp::systems::AssetsResult& Result) {
    if(Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        for (size_t i = 0; i < Result.GetAssets().Size(); i++)
        {
            cout << "Asset found: " << Result.GetAssets()[i].Name << " (ID: " << Result.GetAssets()[i].Id << ")" << endl;
        }
    }
    else
    {
        cout << "Error retrieving assets: " << Result.GetResponseBody() << endl;
    }
    });
```

This code queries all assets in a specified asset collection and prints their names and IDs. It provides a straightforward way to retrieve and manage assets associated with a particular collection.

2. **Querying a Single Asset by ID:** This method retrieves a specific asset using its unique ID. It is ideal when you need to access a known asset quickly.

   **Example:** 

```
csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

AssetSystem->GetAssetById(AssetCollectionId, AssetId, [&](const csp::systems::AssetResult& Result) {
    if(Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        cout << "Asset found: " << Result.GetAsset().Name << " (ID: " << Result.GetAsset().Id << ")" << endl; }
    else
    {
        cout << "Error retrieving asset: " << Result.GetResponseBody() << endl;
    }
    });
```

   This function takes an asset ID and retrieves the corresponding asset. It is helpful when you already know the asset ID and need to fetch its details.

3. **Querying Assets by Criteria:** To search for assets based on multiple attributes (e.g., type, tag, or collection), use GetAssetsByCriteria. This function provides flexibility by allowing additional arguments for filtering.  

There are other methods for querying Assets, which are 

* GetAssetsCollectionById**:** Retrieves an AssetCollection by its unique ID. It is useful when you know the specific ID of the AssetCollection you want to access.

* GetAssetsCollectionByName: It retrieves an AssetCollection by its name. It is ideal for situations where collections are named logically for easier identification.

* FindAssetsCollections: It allows for searching AssetsCollections by various criteria. It supports more flexible queries compared to direct ID or name lookups.

* GetAssetsByCollectionIds: It retrieves all assets associated with an array of AssetCollection IDs. It facilitates batch queries for multiple collections at once.

# Summary and Best Practices

In this topic, you learned how to manage assets in the Connected Spaces Platform (CSP). We covered key topics, including asset CRUD operations, asset collections, thumbnail generation, the automated model decimation service, and how asset collections relate to CSP components. We also explored methods for querying assets and handling multi-users events when assets are updated.

## Recap of Key Concepts

* **Asset CRUD Operations:** Create, read, update, and delete (CRUD) operations allow you to manage assets in a collection efficiently.

* **Asset Collections:** AssetCollections are versatile tools for grouping assets or storing generic data. They do not inherently reference the assets associated with them but can be used to organize related data through metadata.

* **Thumbnail Generation:** Thumbnails provide visual references for assets, enhancing search and browsing.

* **Automated Model Decimation**: This service optimizes large 3D models to improve performance without manual intervention.

* **Asset-Component Relationships:** Assets are linked to components like static models, animated models, and scripts, enabling interactive behavior in spaces.

### Querying for Assets

CSP provides multiple methods to search for and retrieve assets:

* `GetAssetCollectionById` Retrieve an AssetCollection using its unique ID.

* `GetAssetCollectionByName` Retrieve an AssetCollection by its name.

* `FindAssetCollections` Search for AssetCollections using various criteria.

* `GetAssetsInCollection` Retrieve all assets in a specific AssetCollection.

* `GetAssetById` Retrieve a specific asset by its ID and associated AssetCollection ID.

* `GetAssetsByCollectionIds` Retrieve assets from multiple AssetCollections.

* `GetAssetsByCriteria` Query assets based on attributes such as type, tags, or collection.

## Best Practices for Managing Assets Efficiently in CSP

* **Organize Assets in Collections:** To simplify management and updates, keep related assets in the same collection.

* **Use Precise Queries:** When querying for assets, provide both the AssetCollection Id and Asset Id. This ensures accurate and efficient retrieval.

* **Optimize Assets:** Use decimated models and compressed textures to reduce file sizes and improve performance.

* **Remove Unused Assets**: Regularly review and delete outdated or unused assets in the space to keep the asset library lean and manageable.

* **Test Changes Before Deployment:** Always test asset updates in a controlled environment to ensure they function correctly in the live space.
