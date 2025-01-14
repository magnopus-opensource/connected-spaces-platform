# Assets

Assets form the foundation of any space in CSP. They bring visual elements to life and add interactivity. By combining various assets, you can create detailed environments where users can explore, interact, and engage with each other in real-time.

In CSP, an **asset** is any resource used to create and enhance virtual spaces. Assets include files such as 3D models, textures, images, sounds, and gaussian splats. These assets are essential for building immersive, interactive environments, enabling users to populate spaces with engaging components. Assets are managed by **asset collections**, making it easier to organize and access them.

Asset management in CSP involves organizing, creating, and updating assets to ensure they are readily available for use in spaces. In multi-users environments, managing assets efficiently is critical for smooth, synchronized interaction. When assets are properly managed:

* **Performance Improves:** Optimized assets reduce load times and prevent lag, making the space run smoothly across multiple clients.

* **Consistency is Ensured:** Proper asset management ensures that all users experience the same space with identical assets, preventing discrepancies in what users see and interact with.

* **Scalability Increases:** Managing assets well allows for easier updates and expansions, enabling the space to grow without performance issues.

There are many types of assets in CSP, each with specific uses. Some of the more common asset types are:

* **3D Models:** These are used to create objects and environments in a space. They can be static or animated, allowing for dynamic interactions.

* **Images:** These assets are used for displaying visuals like posters or other images on surfaces, such as quad meshes, within a space.

* **Sounds:** Audio files, such as music or sound effects, enhance the immersive experience by adding an auditory layer to the environment.

Each asset type plays a unique role in crafting an interactive, engaging space that responds to user input and enhances the overall experience.

## Understanding Asset Collections

Asset Collections allow applications to group related assets together for easier management. For example, in a multi-user space, all images and models for an object that exists in the space, such as a table, might be stored in a single asset collection. This approach allows developers to ensure assets remain logically grouped.

While the name reflects the primary purpose (to represent a collection of assets) its metadata-centric design also supports the storage of **generic data** in services.

As such, asset collections support a broader set of use cases. You should consider both its **asset grouping** and **generic data storage** functionalities when integrating Asset Collections into your projects.

### Purpose and Flexibility

1. **Asset Management**  
   Asset Collections group and manage multiple assets such as images, models, and textures required for virtual environments. This many-to-one relationship is defined at the asset level, where a single Asset Collection can hold multiple associated assets, but an asset is tied to only one Asset Collection.

2. **Data Storage**  
   Asset Collections also store non-asset data through their metadata. This enables applications to store key-value pairs in a flexible manner, making Asset Collections suitable for systems that do not rely on physical assets.


## Asset CRUD Operations

In CSP, managing assets involves four key actions: **Create, Read, Update, and Delete** (CRUD). These operations are essential for organizing, maintaining, and modifying assets within a connected space.

### Creating an Asset Collection

An asset collection is a group of related assets that can be managed together. The process begins by creating a collection where assets will be stored.

**Step-by-Step Guide:**

```eval_rst
* **Step 1:** Prompt the user to enter a unique name for the asset collection.
* **Step 2:** Use the :func:`csp::systems::AssetSystem::CreateAssetCollection` function to generate the collection.
* **Step 3:** Confirm the collection was successfully created.
```

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
                cout << "Created a new Asset Collection named " << AssetCollection.Name << ". Id: " << AssetCollection.Id << endl;
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

```eval_rst
* **Step 1:** Request a unique name for the asset.  
* **Step 2:** Call the :func:`csp::systems::AssetSystem::CreateAsset` function to add the asset to the collection.  
* **Step 3:** Confirm the asset creation.
```

```
void CreateImageAsset(const std::string& AssetName) {
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->CreateAsset(AssetCollection, AssetName.c_str(), nullptr, nullptr,
    csp::systems::EAssetType::IMAGE, [](const csp::systems::AssetResult Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            auto Asset = Result.GetAsset();
            cout << "Created a new Asset called " << Asset.Name
            << ". Id: " << Asset.Id << endl;
        }
        else
        {
            cout << "Error: Could not create a new Asset. " << Result.GetResponseBody() << endl;
        }
    });
}
```

**Explanation:**  
This code creates an asset within the existing asset collection. It prompts the user for a name and defines the asset type as `IMAGE`. After creation, the asset's Id and name are displayed to the user.

### Uploading an Asset

Uploading allows users to add data (such as images or 3D models) to the newly created asset. 

In CSP, you can upload assets either via a `FileAssetDataSource` or a `BufferAssetDataSource`. These methods allow you to provide the asset data either from disk or from memory.

1. **BufferAssetDataSource**  
    This method reads the asset data from a provided buffer. It is useful when the data is already in memory or if you are processing asset data on the fly.

    ```
    csp::systems::BufferAssetDataSource MyAssetBuffer;
    MyAssetBuffer.Buffer = imageData;
    MyAssetBuffer.BufferLength = sizeof(imageData);
    MyAssetBuffer.SetMimeType("image/png");
    ```

2. **FileAssetDataSource**  
    This method uses a file path to upload an asset directly from a local file. It is suitable when the asset exists as a separate file and can be easily referenced.  

    ```
    csp::systems::FileAssetDataSource MyAssetFile;
    MyAssetFile.FilePath = "path/to/asset.png";
    MyAssetFile.SetMimeType("image/png");
    ```

#### Step-by-Step Guide
The following example demonstrates an entire flow for uploading an asset from a file.

```eval_rst
* **Step 1:** Define the file path for the asset.  
* **Step 2:** Create a file asset data source.
* **Step 3:** Upload the asset data using the :func:`csp::systems::AssetSystem::UploadAssetData` function.
```

```
void UploadAsset()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    //Create Asset Data Source from file
    csp::systems::FileAssetDataSource AssetDataSource;
    AssetDataSource.FilePath = "TestAsset/TestImage.png";
    AssetDataSource.SetMimeType("image/png");

    //Upload Asset
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->UploadAssetData(AssetCollection, Asset, AssetDataSource, [&CallbackPromise](const csp::systems::UriResult& Result)
    {
        if(Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            cout << "Uploaded Test Asset from path: " << AssetDataSource.FilePath << endl;
            CallbackPromise.set_value();
        }
        else
        {
            cout << "Error: Could not upload Test Asset. " << Result.GetResponseBody() << endl;
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

```eval_rst
* **Step 1:** Call the :func:`csp::systems::AssetSystem::DeleteAsset` function.  
* **Step 2:** Confirm that the asset has been successfully deleted.
```

```
void DeleteAsset() {
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->DeleteAsset(AssetCollection, Asset, [&CallbackPromise](const csp::systems::NullResult Result)
    {
        if(Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            cout << "Deleted Asset called " << Asset.Name << ". Id: " << Asset.Id << endl;
            CallbackPromise.set_value();
        }
        else
        {
            cout << "Error: Could not delete Asset. " << Result.GetResponseBody() << endl;
            CallbackPromise.set_value();
        }
    });
    
    CallbackFuture.wait();
}
```

**Explanation:**  
The `DeleteAsset` function removes an asset from the collection. After the function is executed, the user is informed whether the deletion was successful or if there were any issues.

## Relationship with Entity Components

In CSP, Asset Collections are associated with various components within a space to bring visual and interactive elements to life. Components are the building blocks of a space, and assets such as 3D models, textures, and sounds are attached to these components to create rich, immersive environments.

Asset Collections in CSP do not inherently reference the assets associated with them. Instead, an Asset explicitly stores the Id of the Asset Collection it belongs to. This design ensures a unidirectional relationship where assets know their respective Asset Collection, but Asset Collections do not maintain references to their associated assets.

When an Asset is linked to a component, the Asset Id and Asset Collection Id are associated with the component. These identifiers are then used to retrieve the asset data. The asset itself does not store the actual data but provides a **URL** pointing to the asset's storage. The data (such as a 3D model mesh or sound file) is then downloaded directly from the storage.

**Examples of Component Types That Can Use Assets**

Various components within a CSP space rely on assets to function. Here are a few examples:

* **Static Models:** These are 3D objects that remain stationary in a space. Assets such as 3D geometry and textures are linked to static model components to define their appearance.

* **Skeletal Models:** CSP supports skeletal meshes, which include a rigged skeleton structure for animation.

* **Sound Components:** Sound components enable audio interactions within a space.

* **Script Components:** Interactive elements, such as buttons, often use scripts that interact with assets to define behavior.

## Querying for Assets

Querying for assets in CSP allows users to search, retrieve, and manage assets within a space. CSP provides flexible methods for querying assets based on various criteria, ensuring that users can efficiently locate the assets that they need.

### Methods for Querying Assets Within a Space

To manage assets effectively, you can query them based on different attributes like asset type, collection, or space. CSP offers several built-in methods to handle these queries.

1. **Querying All Assets in a Collection**  
    This method retrieves all assets within a specific Asset Collection. It is useful when you need to list or manage all assets grouped together in the same collection.

    ```
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    
    AssetSystem->GetAssetsInCollection(AssetCollection, [](const csp::systems::AssetsResult& Result)
    {
        if(Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            for (size_t i = 0; i < Result.GetAssets().Size(); i++)
            {
                cout << "Asset found: " << Result.GetAssets()[i].Name << " (Id: " << Result.GetAssets()[i].Id << ")" << endl;
            }
        }
        else
        {
            cout << "Error retrieving assets: " << Result.GetResponseBody() << endl;
        }
    });
    ```
    
    **Explanation:**  
    This code queries all assets in a specified asset collection and prints their names and Ids. It provides a straightforward way to retrieve and manage assets associated with a particular collection.

2. **Querying a Single Asset by Id**  
    This method retrieves a specific asset using its unique Id. It is ideal when you need to access a known asset quickly.

    ```
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    
    AssetSystem->GetAssetById(AssetCollectionId, AssetId, [](const csp::systems::AssetResult& Result)
    {
        if(Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            cout << "Asset found: " << Result.GetAsset().Name << " (Id: " << Result.GetAsset().Id << ")" << endl; }
        else
        {
            cout << "Error retrieving asset: " << Result.GetResponseBody() << endl;
        }
    });
    ```
    
    **Explanation:**  
    This function takes an asset Id and retrieves the corresponding asset. It is helpful when you already know the asset Id and need to fetch its details.

3. **Querying Assets by Criteria**  
    To search for assets based on multiple attributes (such as type, tag, or collection), use `GetAssetsByCriteria`. This function provides flexibility by allowing additional arguments for filtering.  

There are other methods for querying Assets and Asset Collections, which are:

```eval_rst
* :func:`csp::systems::AssetSystem::GetAssetCollectionById` Retrieves an Asset Collection by its unique Id. It is useful when you know the specific Id of the Asset Collection you want to access.

* :func:`csp::systems::AssetSystem::GetAssetCollectionByName` Retrieves an Asset Collection by its name. It is ideal for situations where collections are named logically for easier identification.

* :func:`csp::systems::AssetSystem::FindAssetCollections` Facilitates searching AssetsCollections by various criteria. It supports more flexible queries compared to direct Id or name lookups.

* :func:`csp::systems::AssetSystem::GetAssetsByCollectionIds` Retrieves all Assets associated with an array of Asset Collection Ids. It facilitates batch queries for multiple collections at once.
```

## Automated Model Decimation

If your use case includes the adoption of Magnopus Cloud Services (MCS), the **automated model decimation service** in MCS reduces the complexity of 3D models by lowering their polygon count and compressing their textures. This process is known as **model decimation** and helps optimize asset performance without sacrificing visual quality.

Model decimation is crucial in projects that use complex 3D models in a real-time application. The decimation service automatically reduces the size of 3D models, making them more manageable for delivery over the network and enables performant real-time rendering. This service ensures that the models maintain an acceptable level of detail whilst reducing the data size, leading to better performance in the virtual space.

### Benefits of Model Decimation for Optimizing Asset Performance

Optimizing 3D models through decimation brings several key benefits:

* **Improved Performance:** Decimated models use fewer resources, leading to faster rendering and smoother interactions, especially in environments with many assets.

* **Reduced Load Times:** Smaller models load quicker, reducing wait times for users and improving the overall experience.

* **Optimized for Many Users:** In multi-user environments, decimation reduces the bandwidth needed to send models to multiple clients, ensuring a seamless experience for all users.

* **Memory Efficiency:** Decimated models take up less memory, making it possible to use more assets without overwhelming the system.

## Summary
In this topic, you learned how to manage assets in the Connected Spaces Platform (CSP). We covered key topics, including asset CRUD operations, asset collections, the automated model decimation service, and how asset collections relate to CSP components. We also explored methods for querying assets.

* **Asset CRUD Operations:** Create, Read, Update, and Delete (CRUD) operations allow you to manage assets in a collection efficiently.

* **Asset Collections:** Asset Collections are versatile tools for grouping assets or storing generic data. They do not inherently reference the assets associated with them and can also be used to organize related data through metadata.

* **Asset-Component Relationships:** Assets are linked to components like static model components, animated model components, and script components.

* **Automated Model Decimation**: This MCS service optimizes 3D models to improve performance without manual intervention.

### Querying for Assets

CSP provides multiple methods to search for and retrieve assets:

```eval_rst
* :func:`csp::systems::AssetSystem::GetAssetCollectionById` Retrieve an Asset Collection using its unique Id.

* :func:`csp::systems::AssetSystem::GetAssetCollectionByName` Retrieve an Asset Collection by its name.

* :func:`csp::systems::AssetSystem::FindAssetCollections` Search for Asset Collections using various criteria.

* :func:`csp::systems::AssetSystem::GetAssetsInCollection` Retrieve all assets in a specific Asset Collection.

* :func:`csp::systems::AssetSystem::GetAssetById` Retrieve a specific asset by its Id and associated Asset Collection Id.

* :func:`csp::systems::AssetSystem::GetAssetsByCollectionIds` Retrieve assets from multiple Asset Collections.

* :func:`csp::systems::AssetSystem::GetAssetsByCriteria` Query assets based on attributes such as type, tags, or collection.
```

### Best Practices for Managing Assets Efficiently

* **Organize Assets in Collections:** To simplify management and updates, keep related assets in the same collection.

* **Optimize Assets:** Use decimated models and compressed textures to reduce file sizes and improve performance.

* **Remove Unused Assets**: Regularly review and delete outdated or unused assets in the space to keep the asset library lean and manageable.
