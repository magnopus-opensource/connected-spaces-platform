# Anchoring

Anchoring in the Connected Spaces Platform (CSP) links a real-world location to a digital coordinate system. This connection allows augmented reality (AR) digital objects to align accurately with physical spaces, ensuring a consistent and precise user experience. Anchors provide a "point of truth" where physical and digital worlds converge as stable reference points in AR applications.

### Purpose of Anchoring in Achieving Spatial Positioning and Spatiotemporal Coherence

Anchoring supports spatial positioning by accurately mapping physical locations to digital coordinates. When multiple users interact with the same AR scene, they experience the content as if they see it from the same physical point, regardless of location. This alignment ensures spatiotemporal coherence, where digital content's position appears the same to all users, creating a cohesive experience between digital and physical realities.

In CSP, anchoring bridges the physical-digital divide, making digital elements appear fixed in real-world locations. This accuracy and stability are critical in collaborative AR applications where multiple users need a shared, unchanging view of digital content.

### Types of Anchors in CSP

CSP supports two main types of anchors: **global anchors** and **space-local anchors**.

1. **Global Anchors**  
   Global anchors are not associated with any specific CSP space. They are linked to absolute physical locations, such as GPS coordinates, allowing digital content to remain stable and accurately positioned regardless of user location. Global anchors are ideal for AR applications that require content to stay fixed in general outdoor or large-scale environments.

2. **Space-Local Anchors**  
   Space-local anchors are tied to specific CSP spaces, such as rooms or other confined areas within a building. These anchors support indoor positioning, allowing AR content to align accurately within that space. Space-local anchors are highly effective for applications where precise positioning is necessary within smaller, defined spaces.

## CRUD Operations for Anchors

Anchors in CSP provide crucial points of alignment between physical locations and digital space. Managing anchors involves creating, reading, updating, and deleting them (CRUD operations). Each operation serves a specific purpose and enables developers to achieve accurate, high-precision positioning of AR content in a connected space.

### Creating Anchors

CSP allows for the creation of two types of anchors: global anchors and space-local anchors. Each type has a distinct set of parameters and use cases.

1. **Global Anchor Creation**  
   Global anchors are used when the anchor is not associated with a particular CSP space. These anchors are bound to absolute geographic locations and provide stable, large-scale positioning for AR content.

   **Parameters Required:**

   * **ThirdPartyAnchorProvider**: Specifies the third-party cloud anchor provider, such as Google Cloud Anchors (GCA).

   * ThirdPartyAnchorId: A unique ID from the cloud provider that links CSP's anchor to the provider's anchor.

   * **Digital Transform**: Defines the position and orientation of the anchor in digital space.

   * **Optional Tags and AssetCollectionId**: Tags help categorize the anchor, and the AssetCollectionId links the anchor to a specific asset.

   **Code Example for Global Anchor Creation**

```
csp::systems::AnchorSystem* AnchorSystem = csp::systems::SystemsManager::Get().GetAnchorSystem();
AnchorSystem->CreateAnchor(
    csp::systems::AnchorProvider::GoogleCloudAnchors,
    "thirdPartyAnchorId123",
    "assetCollectionIdABC",
    csp::systems::GeoLocation(34.0549, -118.2426),
    csp::systems::OlyAnchorPosition(1.0, 2.0, 3.0),
    csp::systems::OlyRotation(0.0, 0.0, 0.0, 1.0),
    nullptr,
    nullptr,
    [](const AnchorResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Handle successful anchor creation
        }
    }
);
```

This example creates a global anchor using Google Cloud Anchors, associating it with a specific geographic location and digital transform. The callback handles the success or failure of the creation.

**2. Space-Local Anchor Creation**  
    Space-local anchors are associated with a specific CSP space and entity, allowing for precise positioning within confined areas like rooms. This type is useful for AR applications requiring indoor positioning accuracy.

    **Additional Parameters**

```
* **SpaceId**: Identifies the CSP space where the anchor will be positioned.  
* SpaceEntityId: Links the anchor to a particular entity within that space.  
  **Code Example for Space-Local Anchor Creation**

AnchorSystem->CreateAnchorInSpace(
    csp::systems::AnchorProvider::GoogleCloudAnchors,
    "thirdPartyAnchorId123",
    "spaceIdXYZ",
    456789, // SpaceEntityId
    "assetCollectionIdABC",
    csp::systems::GeoLocation(34.0549, -118.2426),
    csp::systems::OlyAnchorPosition(1.0, 2.0, 3.0),
    csp::systems::OlyRotation(0.0, 0.0, 0.0, 1.0),
    nullptr,
    nullptr,
    [](const AnchorResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Handle successful anchor creation in space
        }
    }
);
```

  This code shows the creation of a space-local anchor linked to both a specific space and an entity within that space.

### Reading and Retrieving Anchors

CSP provides multiple methods to retrieve anchors based on location, space, or asset collection.

* `GetAnchorsInArea`: Retrieves anchors within a specified radius of a geographic location.

```
AnchorSystem->GetAnchorsInArea(
    csp::systems::GeoLocation(34.0549, -118.2426),
    100.0, // Area radius in meters
    nullptr, nullptr, nullptr, nullptr, nullptr, 0, 10,
    [](const AnchorCollectionResult& Result) {
        // Process retrieved anchors within the area
    }
);
```

  This query allows applications to find anchors near a specific geographic point, which is useful for location-based AR applications.

* `GetAnchorsInSpace`: Retrieves all anchors associated with a specific CSP space.

```
AnchorSystem->GetAnchorsInSpace("spaceIdXYZ", 0, 10, [](const AnchorCollectionResult& Result) {
    // Handle retrieved anchors in space
});
```

  This method is helpful when accessing all anchors tied to a defined space, such as a building or room.

* `GetAnchorsByAssetCollectionId`: Retrieves anchors linked to a particular asset collection.

```
AnchorSystem->GetAnchorsByAssetCollectionId("assetCollectionIdABC", 0, 10, [](const AnchorCollectionResult& Result) {
    // Process retrieved anchors by asset collection
});
```

This query is useful when an anchor is associated with specific digital content stored in an asset collection.

### Updating Anchors

In some scenarios, updating an anchor's attributes may be necessary, such as modifying its digital transform or reassigning it to a different space. Although CSP does not currently support direct updates to anchor attributes, developers can delete an existing anchor and create a new one with the updated parameters to achieve the same result.

### Deleting Anchors

CSP allows the deletion of anchors using the DeleteAnchors method. This operation enables developers to manage obsolete or irrelevant anchors effectively.

**Code Example for Deleting Anchors**

```
csp::common::Array<csp::common::String> AnchorIDsToDelete = {"anchorId123"};

AnchorSystem->DeleteAnchors(AnchorIDsToDelete, [](const NullResult& Result) {
    if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
        // Handle successful anchor deletion
    }
});
```

This code deletes a specified anchor and provides a callback to handle the result. If successful, the anchor will no longer be accessible or queryable in CSP.

## Querying for Anchors

Querying anchors in CSP allows developers to locate and manage anchors based on specific criteria. CSP provides methods to query anchors by location, space, or asset collection, enabling flexible and precise retrieval of anchor data.

Anchors can be retrieved via CSP using three different methods in the Anchor System: `GetAnchorsInArea`, `GetAnchorsInSpace`, and `GetAnchorsByAssetCollectionId`.

All of the anchor querying methods support paging. Each parameter allows the application to specify how many anchors to **Skip are** returning and a **Limit** to how many per invocation should be returned.

### GetAnchorsInArea

You can use the `GetAnchorsInArea` method to find anchors within a specified radius of a geographic location. This query is most useful for outdoor or large-area AR experiences where geographic proximity matters.

**Parameters for GetAnchorsInArea**

* **OriginLocation**: The latitude and longitude of the center point for the search.

* AreaRadius: The radius (in meters) around the origin within which to search for anchors.

* **Optional Filters**: Tags, spatial keys, or space IDs to refine the search.

**Code Example**  
The following code retrieves anchors near a location with a 100-meter radius:

```
csp::systems::AnchorSystem* AnchorSystem = csp::systems::SystemsManager::Get().GetAnchorSystem();

AnchorSystem->GetAnchorsInArea(
    csp::systems::GeoLocation(34.0549, -118.2426), // Los Angeles location
    100.0, // Radius in meters
    nullptr, // Optional spatial keys
    nullptr, // Optional spatial values
    nullptr, // Optional tags
    nullptr, // Optional AllTags flag
    nullptr, // Optional space IDs
    0,       // Skip count
    10,      // Limit on the number of anchors to return
    [](const AnchorCollectionResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Process retrieved anchors
        }
    }
);
```

This example searches for anchors around Los Angeles within a 100-meter radius. The callback handles the retrieved anchors, allowing developers to use them in their applications.

### GetAnchorsInSpace
Use the `GetAnchorsInSpace` method to find all anchors within a defined CSP space. This method is ideal for indoor AR experiences with anchors localized to specific rooms or areas.

**Code Example**

```
AnchorSystem->GetAnchorsInSpace(
    "spaceIdXYZ", // Space ID to query
    0,            // Skip count
    10,           // Limit on the number of anchors to return
    [](const AnchorCollectionResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Handle anchors retrieved from the space
        }
    }
);
```

This query is useful for managing anchors in specific spaces, such as retrieving all anchors in a conference room or exhibition hall.

### GetAnchorsByAssetCollectionId

Use the `GetAnchorsByAssetCollectionId` method to retrieve anchors associated with a specific asset collection. This query is helpful when anchors are linked to specific digital content.

**Code Example**

```
AnchorSystem->GetAnchorsByAssetCollectionId(
    "assetCollectionIdABC", // Asset collection ID to query
    0,                      // Skip count
    10,                     // Limit on the number of anchors to return
    [](const AnchorCollectionResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Process retrieved anchors tied to the asset collection
        }
    }
);
```

This query is commonly used in applications where anchors are linked to specific assets, such as AR markers for virtual exhibits or linked product data.

## Associating a Google Cloud Anchor (GCA) with a CSP Anchor

Google Cloud Anchors (GCA) provide high-precision anchoring for augmented reality (AR) experiences. Developers can associate GCAs with CSP anchors to integrate real-world locations and digital content in CSP. This association ensures precise spatial positioning across applications.

![image info](../../_static/anchoring/csp_gca_flow.png)

### Using Third-Party Anchors

To integrate third-party anchors like GCAs, CSP provides the parameters `ThirdPartyAnchorProvider` and `ThirdPartyAnchorId`.

* `ThirdPartyAnchorProvider`
  This parameter specifies the external cloud anchoring provider. For GCAs, this value is set to GoogleCloudAnchors.

* `ThirdPartyAnchorId`
  This parameter is a unique identifier the third-party provider assigns to the anchor. It links the CSP anchor to the corresponding GCA, enabling the system to retrieve and resolve the anchor as needed.

## Resolving a Google Cloud Anchor (GCA) at Runtime

Resolving a Google Cloud Anchor (GCA) at runtime ensures that CSP applications can align digital content with precise physical locations. This process retrieves anchor data from the third-party provider, integrates it into the CSP system, and applies transformations to achieve accurate spatial alignment.

### Anchor Resolution Process

The resolution of a GCA within CSP involves these key steps:

1. **Retrieve Third-Party Anchor Data**  
   Request data from the third-party anchor provider, such as Google Cloud Anchors. This step retrieves essential information like the anchor's physical location, position, and rotation.

2. **Pass Data to the CSP Resolution System**  
   Send the retrieved anchor data to CSP's resolution system. This action initializes the resolution process within the CSP application.

3. **Compute the Inverse Transform**  
   Use the anchor's digital transform (position and rotation) to compute its inverse. The inverse transform repositions the origin of the digital space to align with the resolved anchor.

4. **Rebase the Digital Space**  
   Apply the inverse transform to adjust the entire digital space relative to the resolved anchor. This ensures that all digital content is aligned with the physical space.

### Runtime Resolution Workflow

CSP applications follow a structured workflow to resolve GCAs at runtime:

1. **Retrieve Anchor Data**  
   The application communicates with the third-party anchor provider, requesting the specific anchor data identified by ThirdPartyAnchorId. The provider returns the anchor's location and orientation in physical space.

2. **Initialize Anchor Resolution**  
   The retrieved data, including the anchor's position and rotation, is passed to CSP's anchor resolution system. This step synchronizes the anchor's physical attributes with its digital representation.

3. **Wait for Resolution**  
   The CSP system attempts to resolve the anchor while the user interacts with the AR environment. This process ensures the anchor's physical position is accurately reflected in digital space.

4. **Compute and Apply the Inverse Transform**  
   The anchor's digital-space transform (position and rotation) defines its location relative to the origin. The application calculates the inverse of this transform to shift the origin of the digital space to align with the anchor.

5. **Rebase the Origin**  
   The computed inverse transform is applied to the digital space. This operation adjusts all other entities relative to the anchor, ensuring digital content appears correctly positioned in AR.

![image info](../../_static/anchoring/anchor_resolution.png)

**Code Example for Anchor Resolution**

The following code demonstrates the high-level logic for resolving a GCA:

```
// Define the anchor details
csp::common::String ThirdPartyAnchorId = "gca123456789"; // GCA ID
csp::systems::GeoLocation AnchorLocation(34.0549, -118.2426); // Physical location
csp::systems::OlyAnchorPosition AnchorPosition(1.0, 2.0, 3.0); // Digital position
csp::systems::OlyRotation AnchorRotation(0.0, 0.0, 0.0, 1.0); // Digital rotation

// Retrieve the third-party anchor data
ThirdPartyAnchorProvider->GetAnchorData(
    ThirdPartyAnchorId,
    [](const ThirdPartyAnchorResult& Result) {
    if (Result.IsSuccess()) {
        // Pass the retrieved data to the CSP resolution system
        csp::systems::AnchorSystem* AnchorSystem = csp::systems::SystemsManager::Get().GetAnchorSystem();
        AnchorSystem->ResolveAnchor(
            Result.AnchorLocation,
            Result.AnchorPosition,
            Result.AnchorRotation,
            [](const ResolutionResult& ResolutionStatus) {
                if (ResolutionStatus.IsSuccess()) {
                    // Compute inverse transform and rebase the digital space
                    // Anchor resolution successful
                } else {
                    // Handle resolution failure
                }
            }
        );
    } else {
        // Handle retrieval failure
    }
    }
);
```

**Key Concepts in the Code**

1. **Third-Party Anchor Data Retrieval**  
   The GetAnchorData function fetches anchor details from Google Cloud Anchors, using the ThirdPartyAnchorId to identify the specific anchor.

2. **CSP Resolution System Integration**  
   The ResolveAnchor method in CSP integrates the retrieved anchor data, synchronizing it with the application's digital space.

3. **Transform Calculations**  
   The resolution process computes the inverse transform of the anchor's position and rotation. This calculation adjusts the entire digital space relative to the anchor.

4. **Error Handling**  
   Both data retrieval and resolution operations include callbacks to manage success or failure scenarios.

**Applying the Inverse Transform**

Anchors define their position and orientation relative to the origin of the digital space. To align the digital space with the resolved anchor:

1. Compute the inverse of the anchor's digital transform.

2. Adjust the digital space by applying this inverse transform.

3. Ensure all digital entities remain spatially consistent after the transformation.

## Mapping GCA Anchors to Digital Coordinates for AR Alignment

Mapping Google Cloud Anchors (GCA) to digital coordinates ensures accurate alignment between the physical world and augmented reality (AR) content. This process allows AR applications to place digital elements precisely where users expect them, enabling seamless integration between digital and physical spaces.

### Understanding Digital Coordinates and Transforms

**Digital and Physical Coordinate Systems**  
In CSP, the digital coordinate system defines the spatial relationships between virtual entities. It has a defined origin, often set to (0, 0, 0) in a three-dimensional space. All digital entities, including anchors, have positions and orientations relative to this origin.

On the other hand, the physical coordinate system represents real-world locations using geographic coordinates, such as latitude, longitude, and elevation. Aligning these two systems is critical for creating cohesive AR experiences.

**Significance of Aligning Coordinates**  
Anchors serve as the bridge between digital and physical coordinates. A GCA anchor defines a real-world location and links it to a corresponding point in the digital coordinate system. This alignment ensures that AR content appears fixed in the intended physical location, even as users move around.

### Transforming Coordinates

Mapping a GCA anchor to digital coordinates involves the following steps:

1. **Retrieve Anchor Data**  
   Obtain the anchor's physical location (latitude and longitude) and digital transform (position and rotation) from the GCA service.

2. **Define the Digital Transform**  
   Map the anchor's real-world location to a position in the digital coordinate system. The digital position is a three-dimensional point (x, y, z). The digital rotation is a quaternion (x, y, z, w).

3. **Apply the Transform**  
   Use the retrieved data to place the anchor in the digital coordinate system. CSP automatically aligns the digital transform with the physical location.

### Achieving Precise AR Alignment

Precise alignment between digital content and physical locations depends on the following techniques:

1. **High-Precision Coordinates**  
   Use double-precision floating-point values for anchor positions and rotations. This ensures minimal error when mapping physical locations to digital coordinates.

2. **Inverse Transform for Digital Alignment**  
   Compute the inverse transform of the anchor's digital position and rotation. This operation shifts the digital coordinate system to align perfectly with the anchor, ensuring that all other digital entities adjust relative to the anchor's location.

3. **Account for Environmental Factors**  
   Incorporate real-world constraints like GPS accuracy, lighting conditions, and device capabilities. 

4. **Test and Calibrate**  
   Continuously test the AR alignment under different conditions. Calibrate the digital transform based on user feedback and environmental observations to ensure optimal performance.

## Summary

Managing anchors in CSP involves four key operations: creating, querying, updating, and deleting anchors.

* **Creating Anchors**  
  You can create global anchors for outdoor environments or space-local anchors for confined spaces. Global anchors associate with geographic locations, while space-local anchors link to specific spaces or entities. Use the CreateAnchor or CreateAnchorInSpace methods to define the anchor's digital transform, physical location, and optional metadata, such as asset collections or tags.

* **Querying Anchors**  
  CSP provides flexible methods to retrieve anchors based on proximity, space, or asset collection. Use GetAnchorsInArea to find anchors near a geographic location. For anchors within a specific space, use GetAnchorsInSpace. If anchors are tied to a digital asset collection, retrieve them using GetAnchorsByAssetCollectionId.

* **Updating Anchors**  
  While CSP does not currently support direct updates to anchor attributes, you can delete an existing anchor and create a new one with updated parameters to achieve the same result.

* **Deleting Anchors**  
  The DeleteAnchors method allows you to remove anchors that are no longer needed. You can ensure anchors are deleted and handle the result programmatically by providing anchor IDs and a callback.

**Integration and Resolution Workflow**

CSP enables integration of Google Cloud Anchors (GCA) through well-defined workflows.

* **Associating a GCA with a CSP Anchor**  
  To link a GCA with a CSP anchor, specify GoogleCloudAnchors as the third-party provider and include the unique ThirdPartyAnchorId. Use methods like CreateAnchor or CreateAnchorInSpace to define the anchor's digital transform and its relationship to the GCA. This integration ensures CSP anchors benefit from GCA's high precision and stability.

* **Resolving a GCA Anchor at Runtime**  
  Runtime resolution retrieves the anchor's physical location and digital transform from GCA and synchronizes it with the CSP application. This process involves:  
  1. Retrieving the anchor's data from the GCA service.

  2. Passing the data to CSP's resolution system.

  3. Calculating the inverse transform to align the digital coordinate system with the resolved anchor.

  4. Rebasing the digital space to ensure precise alignment between digital and physical environments.
