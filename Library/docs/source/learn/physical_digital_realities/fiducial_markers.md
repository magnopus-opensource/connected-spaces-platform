# Fiducial Markers

Fiducial markers in CSP serve as a method for anchoring digital content in physical spaces. They provide a visual reference that applications can detect and use to align digital entities with the real world. Unlike cloud-hosted anchors, which are virtual points in space, fiducial markers are physical objects that applications can interact with directly.

### Comparison: Fiducial Markers vs. Cloud-Hosted Anchors

Both fiducial markers and cloud-hosted anchors offer distinct advantages. Choosing the right one depends on the use case.

1. **Precision vs. Flexibility**  
   * Cloud-hosted anchors provide high-precision localization, often with sub-centimeter accuracy. However, they rely on third-party services like Google Cloud Anchors and require an internet connection to function.

   * Fiducial markers are less precise, but they offer greater flexibility. Since they are physical objects, they can be moved or repositioned as needed without relying on external services.

2. **Markerless Anchoring vs. Physical Mobility**  
   * Anchors are invisible and work without physical markers, offering a cleaner user experience in augmented reality (AR) applications.

   * Fiducial markers, being tangible, are easier to detect and can act as a practical tool in environments where physical interaction or visibility is necessary.

### Use Cases and Benefits

Fiducial markers are a practical choice in several scenarios:

* **Limited Connectivity:** They work entirely on-device and do not depend on third-party services, making them ideal for offline or remote environments.

* **Real-World Movement:** Their physical nature allows them to be relocated, which is useful in dynamic spaces where objects or setups frequently change.

* **Cross-Platform Compatibility:** Fiducial markers are widely supported across AR ecosystems, enabling consistent functionality in diverse platforms.

* **Quick Deployment:** Their setup is straightforward, and they do not require extensive configuration or cloud integration.

## Core Concepts of Fiducials

Fiducial markers are key in spatial positioning within the Connected Spaces Platform (CSP). They help bridge the gap between physical spaces and digital coordinate systems by providing tangible reference points for aligning virtual entities with real-world environments.

### Fiducial Markers and Digital Coordinate Spaces

Fiducial markers are physical objects that exist in the real world and can be detected by CSP-enabled applications. They establish a relationship between physical spaces and digital coordinate systems.

In digital spaces, every entity has a position, rotation, and scale that defines its location and orientation. Fiducial markers create a link between these digital transforms and the physical world. This ensures that digital entities align correctly with real-world objects.

#### Importance of Dimensions and Scale

The size and scale of fiducial markers are critical for maintaining accurate spatial relationships. Since fiducial markers have real-world dimensions, their size must be defined explicitly in CSP. This ensures that:

* The digital representation of the marker matches its physical counterpart.

* AR content, such as virtual objects or avatars, appears at the correct distance and size relative to the marker.

* Spatial coherence is preserved, enabling a seamless user experience.

CSP uses the metric system to measure dimensions, ensuring consistency in scale across different devices and platforms.

### Marker Resolution and Transform

Resolving a fiducial marker involves detecting its physical presence and calculating its transform within the digital space. The process ensures the marker is properly aligned with CSP's coordinate system.

#### Steps for Resolving Fiducial Markers

1. **Detect the Marker:** The application identifies the fiducial marker using its image recognition system.

2. **Compute the Marker's Transform:** CSP calculates the marker's position, rotation, and scale relative to the digital space's origin.

3. **Inverse the Transform:** The inverse of the marker's transform is computed to adjust all other entities relative to the marker.

4. **Rebase the Origin:** The digital space's origin is shifted to align with the marker, ensuring all spatial content maintains the correct relationships.

#### Role of Marker Component Transforms

Each fiducial marker has three key transform attributes:

* **Position:** Specifies the marker's location in the digital space. This is defined relative to the space's origin or a parent entity.

* **Rotation:** Determines the marker's orientation. Correct rotation ensures that digital content appears aligned with real-world objects.

* **Scale:** Represents the real-world dimensions of the marker. Proper scaling is essential for accurate placement and interaction of digital entities.

## Creating a Fiducial Marker Component

A fiducial marker component is a key element for anchoring digital content to a physical marker in the Connected Spaces Platform (CSP). This section covers the prerequisites, step-by-step creation process, and implementation details to help you configure a fiducial marker for your project.

### Prerequisites

Before creating a fiducial marker component, ensure the following are ready:

1. **Entities and Asset Collections**  
   * An entity in CSP serves as the parent for the fiducial marker component.  
   * Ensure you have created or identified an entity to associate with the marker.  
   * Use CSP's asset system to manage assets and collections needed for the marker.

2. **Fiducial Marker Images**  
   * Prepare the marker image you will use. This image must be uploaded to CSP's asset system and associated with a collection.  
   * Confirm that the image's dimensions reflect its real-world size, ensuring accurate spatial alignment.

### Step-by-Step Process

**Step 1: Create a New Entity**

An entity serves as a container for the fiducial marker component. To create one, use the CreateObject method in CSP's entity system.

```
SpaceTransform MarkerEntityTransform =
{
    csp::common::Vector3::Zero(),
    csp::common::Vector4::Zero(),
    csp::common::Vector3::One()
};

// Callback to confirm entity creation.
csp::multiplayer::SpaceEntitySystem::EntityCreatedCallback Callback = [](csp::multiplayer::SpaceEntity* CreatedSpaceEntity) {
    if (CreatedSpaceEntity)
    {
        // Logic for handling the new entity.
    }
};

// Create a new entity.
SpaceEntitySystem* EntitySystem = SystemsManager.GetSpaceEntitySystem();
EntitySystem->CreateObject("MyMarkerEntity", MarkerEntityTransform, Callback);
```

**Step 2: Add a Fiducial Marker Component**

Attach a fiducial marker component to the created entity using the AddComponent method.

```
FiducialMarkerSpaceComponent* FiducialMarkerComponent = static_cast<FiducialMarkerSpaceComponent*>(Object->AddComponent(ComponentType::FiducialMarker));
```

**Step 3: Associate the Marker with an Asset Collection**

Specify the marker's image by linking the component to an asset collection and its corresponding asset ID. These details help the application recognize and resolve the marker.

```
// Assuming an asset collection and asset identifiers...
csp::systems::AssetCollection AssetCollection = ...;
csp::systems::Asset MarkerAsset = ...;

// Associate the marker component with the asset.
FiducialMarkerComponent->SetAssetCollectionId(MarkerAsset.AssetCollectionId);
FiducialMarkerComponent->SetMarkerAssetId(MarkerAsset.Id);
```

**Step 4: Set Transform Properties**

The transform defines the fiducial marker's position, orientation, and scale. Ensure the scale reflects the marker's real-world dimensions.

```
// Set the transform to define position, rotation, and scale.
SpaceTransform MarkerComponentTransform =
{
    csp::common::Vector3(1.0, 2.0, 3.0),
    csp::common::Vector4(0.0, 0.0, 0.0, 1.0),
    csp::common::Vector3(0.5, 0.5, 0.01)
};

FiducialMarkerComponent->SetTransform(MarkerComponentTransform);
```

**Step 5: Update the Entity**

Queue the entity for a network update to replicate the marker's configuration across connected devices.

```
Entity->QueueUpdate();
```

Here is a complete example of creating and configuring a fiducial marker component:

```
// Create an entity to hold the fiducial marker.
SpaceTransform MarkerEntityTransform =
{
    csp::common::Vector3::Zero(),
    csp::common::Vector4::Zero(),
    csp::common::Vector3::One()
};

csp::multiplayer::SpaceEntitySystem::EntityCreatedCallback Callback = [](csp::multiplayer::SpaceEntity* CreatedSpaceEntity)
{
    if (CreatedSpaceEntity)
    {
        // Add fiducial marker component.
        FiducialMarkerSpaceComponent* MarkerComponent =
            static_cast<FiducialMarkerSpaceComponent*>(CreatedSpaceEntity->AddComponent(ComponentType::FiducialMarker));
        
        // Set marker properties.
        MarkerComponent->SetAssetCollectionId("AssetCollection123");
        MarkerComponent->SetMarkerAssetId("MarkerAsset456");

        SpaceTransform Transform = 
        {
            csp::common::Vector3(1.0, 2.0, 3.0),
            csp::common::Vector4(0.0, 0.0, 0.0, 1.0),
            csp::common::Vector3(0.5, 0.5, 0.01)
        };
        MarkerComponent->SetTransform(Transform);
        
        // Queue the entity for network update.
        CreatedSpaceEntity->QueueUpdate();
    }
};

EntitySystem->CreateObject("MyFiducialMarker", MarkerEntityTransform, Callback);
```

## Relating Fiducial Marker Transforms to AR Spaces

Fiducial markers are critical in aligning digital content with physical spaces in augmented reality (AR). Their transforms-defined by position, rotation, and scale-determine the placement and orientation of AR content. Understanding how to manage fiducial marker transforms ensures precise spatial alignment in CSP applications.

![image info](../../_static/physical_digital/fiducial_flow.png)

### Transform Relationships

A fiducial marker's transform defines its spatial relationship to its associated entity. This includes the marker's:

* **Position:** The location of the marker relative to the entity or space.  
* **Rotation:** The orientation of the marker, which aligns AR content accurately.  
* **Scale:** The marker's dimensions represent its real-world size in meters.

The fiducial marker acts as an anchor for digital content. When the marker's transform is updated (e.g., if it is moved or rotated), the AR content adjusts automatically to maintain its relative position and orientation. For example:

* A marker placed on a table ensures digital objects remain fixed on the table's surface, even if the marker is shifted slightly.

* Adjusting the marker's scale reflects changes in the perceived size of the marker, which in turn modifies the size and placement of AR objects.

**Adjusting Digital Content Placement**

Fiducial markers require their scale to match real-world dimensions. If a marker represents a 0.5-meter square, its scale property must reflect this size. This alignment ensures:

* AR content appears proportional to the marker.  
* The distance and positioning of other digital entities are consistent with real-world expectations.

By managing these properties, you ensure that AR applications maintain spatial coherence across different devices and environments.

### Inverse Transform and Space Rebasement

The **inverse transform** of a fiducial marker is a mathematical operation used to adjust the digital space origin relative to the marker. This process ensures that the marker is the reference point for all other digital entities.

**Using Inverse Transforms**

1. Calculate the inverse of the marker's transform, including position, rotation, and scale.  
2. Apply this inverse transform to the digital space origin.  
3. Shift all other digital entities accordingly to maintain relative alignment with the marker.

For example, suppose a marker's transform indicates it is 2 meters forward and 1 meter to the right of the origin. In that case, the inverse transform repositions the origin to align with the marker's location.

**Rebasing the Digital Space**

Rebasing adjusts the digital space's origin to the fiducial marker's position. This allows other entities to inherit their relative placement based on the marker's new location. The result is a coordinated AR environment where:

* Digital objects stay in the correct positions relative to the marker.  
* Movement or rotation of the marker dynamically adjusts all related digital content.

**Aligning Other Entities and Content**

When rebasing the digital space, the marker's transform affects all associated entities. For example:

* A fiducial marker scaled to 0.5 meters ensures digital objects placed at a distance of 1 meter appear twice as far from the marker as its width.  
* A rotated marker realigns AR content, maintaining the correct orientation relative to the user's perspective.

## Summary

Fiducial markers are essential tools in CSP for aligning digital content with physical spaces. They provide a tangible and flexible anchoring solution, enabling accurate spatial positioning in augmented reality (AR) applications. By linking the physical and digital worlds, fiducial markers ensure AR content is displayed correctly, regardless of user perspective or device.

1. **Fiducial Markers and AR Spatial Coherence**  
   Fiducial markers create a consistent relationship between the physical environment and digital coordinate systems. Their transforms-comprising position, rotation, and scale-define how digital content aligns with the real world. Proper configuration ensures spatial coherence, making AR experiences intuitive and immersive.

2. **Practical Applications in Real-World AR Projects**  
   * **Offline Scenarios:** Fiducial markers work entirely on-device, making them ideal for environments without internet access.

   * **Dynamic Spaces:** Their physical mobility allows seamless adaptation in setups like exhibitions or training environments where markers may be relocated.

   * **Collaborative Experiences:** Fiducial markers enable shared AR experiences by providing a common reference point for multiple users.

   * **Cost-Effective Solutions:** Without reliance on third-party services, fiducial markers offer a budget-friendly alternative for spatial anchoring.
