# Anchoring

Anchoring in the Connected Spaces Platform (CSP) links a real-world location to a digital coordinate system. This connection allows augmented reality (AR) digital objects to align accurately with physical spaces, ensuring a consistent and precise user experience. Anchors provide a "point of truth" where physical and digital worlds converge via stable reference points in AR applications.

### Purpose of Anchoring in Achieving Spatial Positioning and Spatiotemporal Coherence

Anchoring supports spatial positioning by accurately mapping physical locations to digital coordinates. When multiple users interact with the same anchored space, they experience the content as if they were copresent, regardless of whether they are there physically or not.

This alignment ensures _spatiotemporal coherence_; where all users see the same thing in the same place at the same time, creating a cohesive experience between digital and physical realities.

In CSP, anchoring is used to bridge the physical-digital divide, making digital elements appear fixed in real-world locations. The precision, accuracy and stability of anchors are critical in enabling collaborative AR applications where multiple users need a shared, unchanging view of digital content.

### Understanding Digital Coordinates and Transforms

#### Digital and Physical Coordinate Systems

In CSP, the digital coordinate system defines the spatial relationships between virtual entities. It has a defined origin, set to ``(0, 0, 0)`` in a three-dimensional space. All digital entities, including anchors, have positions and orientations relative to this origin.

On the other hand, the physical coordinate system represents real-world locations using geographic coordinates, such as latitude, longitude, and elevation. Aligning these two systems is critical for creating cohesive AR experiences.

#### Significance of Aligning Coordinates

Anchors serve as the bridge between digital and physical coordinates. A GCA anchor defines a real-world location and links it to a corresponding point in the digital coordinate system. This alignment ensures that AR content appears fixed in the intended physical location, even as users move around.

### Types of Anchors in CSP

CSP supports two main types of anchors: **global anchors** and **space-local anchors**.

1. **Global Anchors**  
   Global anchors are not associated with any specific CSP space. They are linked to absolute physical locations, such as GPS coordinates, and can be used regardless of what space a user is in. Global anchors are ideal for use cases which require many variants of the digital representation of a real world space, as the same anchor can be used for all of them.

2. **Space-Local Anchors**  
   Space-local anchors are tied to specific CSP spaces, and optionally even individual entities within that space, such as rooms or other confined areas within a building. Since space-local anchors support entity association, they are highly effective when creating digital recreations of a particular location, as all digital content can be authored relative to that entity.

## CRUD Operations for Anchors

Anchors in CSP provide crucial points of alignment between physical locations and digital space. Managing anchors involves creating, reading, updating, and deleting them (CRUD operations). Each operation serves a specific purpose and enables developers to achieve accurate, high-precision positioning of AR content in a connected space.

### Creating Anchors

CSP allows for the creation of two types of anchors: global anchors and space-local anchors. Each type has a distinct set of parameters and use cases.

1. **Global Anchor Creation**  
   Global anchors are used when the anchor is not associated with a particular CSP space. These anchors are bound to absolute geographic locations and can be used regardless of what digital space a user is in.

   **Parameters Required:**

   * **ThirdPartyAnchorProvider**: Specifies the third-party cloud anchor provider, such as Google Cloud Anchors (GCA).

   * ThirdPartyAnchorId: A unique ID from the cloud provider that links CSP's anchor to the provider's anchor.

   * **Digital Transform**: Defines the position and orientation of the anchor in digital space.

   * **Optional Tags and AssetCollectionId**: Tags help categorize the anchor, and the AssetCollectionId links the anchor to a specific asset.

   **Code Example**

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
   Space-local anchors are associated with a specific CSP space and entity, allowing for precise positioning of digital content relative to a known representation of a real-world object.

   **Code Example**

   ```
// The identifier of the anchor provided by the third-party cloud-hosted anchor platform.
csp::common::String ThirdPartyAnchorID = "d8n6djkkt63lgh3";

// The identifier of the space to associate this anchor with.
csp::common::String SpaceId = "668d71217905471f0b134521";
// The identifier of the entity within the space to assocaite ths anchor with.
csp::common::String SpaceEntityId = "665f8cd01313833472d9d39e";

// The latitude and longitude of the anchor.
auto Geolocation = csp::systems::GeoLocation(34.0549, 118.2426);

// The digital transform of the anchor.
auto DigitalTransformPos = csp::systems::OlyAnchorPosition(1.0, 2.0, 3.0);
auto DigitalTransformRot = csp::systems::OlyRotation(0.0, 0.0, 0.0, 1.0);

// The application can optionally relate an asset collection to the anchor.
csp::common::String AssetCollectionID = "D8N6DJKKT63LGH3";

// The callback to respond to the completion of the anchor creation operation.
csp::systems::AnchorResultCallback Callback = [](const AnchorResult& Result)
{
    if(Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        // Application logic goes here.
    };
};

csp::systems::AnchorSystem* AnchorSystem = csp::systems::SystemsManager::Get().GetAnchorSystem();
AnchorSystem->CreateAnchorInSpace(
    csp::systems::AnchorProvider::GoogleCloudAnchors,
    ThirdPartyAnchorID,
    SpaceId,
    SpaceEntityId,
    AssetCollectionID,
    Geolocation,
    DigitalTransformPos,
    DigitalTransformRot,
    nullptr,
    nullptr,
    Callback);

   ```

   This code shows the creation of a space-local anchor linked to both a specific space and an entity within that space.

### Reading and Retrieving Anchors

CSP provides multiple methods to retrieve anchors based on location, space, or asset collection.

* `GetAnchorsInArea`: Retrieves anchors within a specified radius of a geographic location.
   This query allows applications to find anchors near a specific geographic point, which is useful for location-based AR applications.

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

* `GetAnchorsInSpace`: Retrieves all anchors associated with a specific CSP space.
   This method is helpful when accessing all anchors tied to a defined space, such as a building or room. 

   ```
   AnchorSystem->GetAnchorsInSpace("spaceIdXYZ", 0, 10, [](const AnchorCollectionResult& Result) {
       // Handle retrieved anchors in space
   });
   ```

* `GetAnchorsByAssetCollectionId`: Retrieves anchors linked to a particular asset collection.
   This query is useful when an anchor is associated with specific digital content stored in an asset collection.

   ```
   AnchorSystem->GetAnchorsByAssetCollectionId("assetCollectionIdABC", 0, 10, [](const AnchorCollectionResult& Result) {
       // Process retrieved anchors by asset collection
   });
   ```

### Updating Anchors

In some scenarios, updating an anchor's attributes may be necessary, such as modifying its digital transform or reassigning it to a different space. Although CSP does not currently support direct updates to anchor attributes, developers can delete an existing anchor and create a new one with the updated parameters to achieve the same result.

### Deleting Anchors

CSP allows the deletion of anchors using the `DeleteAnchors` method. This operation enables developers to manage obsolete or irrelevant anchors effectively.

**Code Example**

```
// The set of anchors that the client application wishes to delete.
csp::common::Array<csp::common::String> AnchorIDsToDelete(1) = {"66338b7fcd13d3f163e38c25"};

// The callback to respond to the completion of the anchor delete operation.                        
csp::systems::NullResultCallback Callback = [](const NullResult& Result)
{
    if(Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        // Application logic goes here.
    };
};
                          
csp::systems::AnchorSystem* AnchorSystem = csp::systems::SystemsManager::Get().GetAnchorSystem();
AnchorSystem->DeleteAnchors(AnchorIDsToDelete, Callback);
```

This code deletes a specified anchor and provides a callback to handle the result. If successful, the anchor will no longer be accessible or queryable in CSP.

## Querying for Anchors

Querying anchors in CSP allows developers to locate and manage anchors based on specific criteria. CSP provides methods to query anchors by location, space, or asset collection, enabling flexible and precise retrieval of anchor data.

Anchors can be retrieved via CSP using three different methods in the Anchor System: `GetAnchorsInArea`, `GetAnchorsInSpace`, and `GetAnchorsByAssetCollectionId`.

All of the anchor querying methods support paging. The application can specify how many anchors to `Skip` returning (starting from the first anchor that would be returned by the query), and a `Limit` controlling the maximum number of anchors that the query can return.

### GetAnchorsInArea

You can use the `GetAnchorsInArea` method to find anchors within a specified radius of a geographic location.

**Code Example**  
This example searches for anchors around Los Angeles within a 100-meter radius. The callback handles the retrieved anchors, allowing developers to use them in their applications.

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
    0,       // How many anchors to skip returning (starting from the zeroth anchor)
    10,      // Limit how many anchors are returned by this query
    [](const AnchorCollectionResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success) {
            // Process retrieved anchors
        }
    }
);
```

### GetAnchorsInSpace
Use the `GetAnchorsInSpace` method to find all anchors within a defined CSP space. This method is ideal for indoor AR experiences with anchors localized to specific rooms or areas, particularly if it is expected that GPS signal will be poor.

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

## Associating a Google Cloud Anchor with a CSP Anchor

Google Cloud Anchors (GCA) provide high-precision anchoring for augmented reality (AR) experiences. They do not describe an associated digital transform themselves, nor a geolocation, but when used in conjunction with CSP, developers can create inversible coordinate transformations between the digital and physical world. This association ensures precise spatial positioning across applications.

![image info](../../_static/physical_digital/gca_flow.png)

### Using Third-Party Anchors

To integrate third-party anchors like GCA, CSP provides the parameters `ThirdPartyAnchorProvider` and `ThirdPartyAnchorId`.

* `ThirdPartyAnchorProvider`
  This parameter specifies the external cloud anchoring provider. For GCA, this value is set to `GoogleCloudAnchors`.

* `ThirdPartyAnchorId`
  This parameter is a unique identifier defined by the third-party anchor provider. It acts as a link between the CSP anchor and the corresponding anchor on the third-party platform, enabling applications to retrieve and resolve the anchor as needed.

## Resolving a Google Cloud Anchor at Runtime

Resolving a Google Cloud Anchor (GCA) at runtime means that CSP applications can align digital content with precise physical locations. This process involves retrieving anchor data from CSP, using the third-party anchoring attributes returned, and using those to identify where in reality a user is.

### Anchor Resolution Process

The resolution of a GCA within a CSP-enabled AR application involves these key steps:

1. **Retrieve Third-Party Anchor Data**  
   Request anchoring data from CSP based on the user's context, such as the user's geolocation or the space they are within. The returned results inform the application of which third-party anchors are relevant.

2. **Resolves the third-party anchor**  
   The AR application then uses the anchor data returned by CSP to attempt to resolve nearby anchors.

3. **Compute the Inverse Transform**  
   When an anchor is detected by the application, it uses the digital transform associated with the anchor and computes the inverse. 
   This inverse transform is then used to _rebase the origin_ of the digital space to align all digital content with the resolved anchor.

4. **Rebase the Digital Space**  
   Apply the inverse transform to adjust the entire digital space relative to the resolved anchor. This ensures that all digital content is aligned with the physical space.

![image info](../../_static/physical_digital/anchor_resolution.png)

## Summary

Managing anchors in CSP involves four key operations: creating, querying, updating, and deleting anchors.

* **Creating Anchors**  
  You can create global anchors for any space or space-local anchors. Global anchors often associate with geographic locations, whereas space-local anchors link to specific spaces and entities. Use the CreateAnchor or CreateAnchorInSpace methods to define the anchor's digital transform, physical location, and optional metadata, such as asset collections or tags.

* **Querying Anchors**  
  CSP provides flexible methods to retrieve anchors based on proximity, space, or asset collection. Use GetAnchorsInArea to find anchors near a geographic location. For anchors within a specific space, use GetAnchorsInSpace. If anchors are tied to a digital asset collection, retrieve them using GetAnchorsByAssetCollectionId.

* **Updating Anchors**  
  While CSP does not currently support direct updates to anchor attributes, you can delete an existing anchor and create a new one with updated parameters to achieve the same result.

* **Deleting Anchors**  
  The DeleteAnchors method allows you to remove anchors that are no longer needed. You can ensure anchors are deleted and handle the result programmatically by providing anchor IDs and a callback.

### Achieving Precise AR Alignment

Precise alignment between digital content and physical locations depends on the following techniques.

1. **High-Precision Coordinates**  
   Use double-precision floating-point values for anchor positions and rotations. This ensures minimal error when mapping physical locations to digital coordinates.

2. **Inverse Transform for Digital Alignment**  
   Compute and apply the inverse transform of the anchor to the origin of the space. This operation shifts the digital coordinate system to align perfectly with the space defined by the anchor, ensuring that all other digital entities adjust relative to the anchor's location.

3. **Account for Environmental Factors**  
   Incorporate real-world constraints like GPS accuracy, lighting conditions, and device capabilities. 

4. **Test and Calibrate**  
   Continuously test the AR alignment under different conditions to ensure optimal performance.

### Integration and Resolution Workflow

CSP enables integration of Google Cloud Anchors (GCA) through well-defined workflows.

* **Associating a GCA with a CSP Anchor**  
  To link a GCA with a CSP anchor, specify GoogleCloudAnchors as the third-party provider and include the unique ThirdPartyAnchorId. Use methods like CreateAnchor or CreateAnchorInSpace to define the anchor's digital transform and its relationship to the GCA. This integration ensures CSP anchors benefit from GCA's high precision and stability.

* **Resolving a GCA Anchor at Runtime**  
  Runtime resolution retrieves the anchor's physical location and digital transform from GCA and synchronizes it with the CSP application. This process involves:  
  1. Retrieving the anchor's data from the GCA service.

  2. Passing the data to CSP's resolution system.

  3. Calculating the inverse transform to align the digital coordinate system with the resolved anchor.

  4. Rebasing the digital space to ensure precise alignment between digital and physical environments.
