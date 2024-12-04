# Space Entities

In CSP, **Spaces** are environments where multi-user experiences take place. These virtual spaces are populated with objects and interactive elements to create rich, immersive worlds. At the core of these spaces are space entities, which act as the building blocks for everything you see and interact with.

 A `SpaceEntity` is a conceptual object that acts as a parent transform within a Space. It does not have any inherent visual representation but serves as a foundation to which child components can be added. These components define the visual and interactive behavior of the `SpaceEntity`, allowing for rich and dynamic interactions within the Space.

Unlike a "container" (which implies merely holding properties), a `SpaceEntity` manages and integrates these properties, such as position, rotation, and scale, as part of its role in facilitating transformations and interactions in the 3D environment. Furthermore, space entities support CRUD operations for components, handle their multiplayer replication, and coordinate updates.

## Types of Space Entities in CSP

In CSP, the `SpaceEntity` is the core object that populates the virtual environment. 

There are two types of space entities: **Avatar Entities** and **Object Entities**. Each type serves a different purpose, whether representing users or inanimate objects within the Space. These entities allow for interaction and visual representation, creating immersive experiences.

* **Avatars** are the avatar representations of users in spaces. At a minimum they comprise a `SpaceEntity`, with a `SpaceEntityType` of Avatar, and a single AvatarSpaceComponent.  
* **Objects** represent everything else in your Space. They have no components added by default (unlike an Avatar which has the AvatarSpaceComponent automatically attached), but can have components added to define the visual elements and behavior of your experience.

### Avatar Entities

In CSP, **Avatar Entities** represent users within a multi-user space. These entities serve as the visual and interactive representations of users, allowing them to participate in the virtual environment. However, **Avatar Entities** are not fundamentally different from other entities in their structure; they are essentially **space entities** with specific components attached.

**AvatarSpaceComponent**
The `AvatarSpaceComponent` is a special component that is automatically attached when creating an avatar entity. This component gives the avatar its identity and role within the space. It handles everything related to the user's presence, including the display of the user's visual model and replicated motion.

### The Difference Between Avatar Entities and Object Entities

**Avatar Entities** differ from **Object Entities** in one key way. Avatar entities are automatically assigned the AvatarSpaceComponent by default. This gives them the built-in capability to represent a user, whereas object entities do not come with default components and must be customized based on their intended use.

## Transient and Persistent Entities

In CSP, space entities may be either **transient** or **persistent (non-transient)** based on whether they should persist in a space.

The distinction between transient and persistent entities plays a significant role in creating a consistent multi-user experience. Setting persistence incorrectly can lead to issues in the user experience.

### Transient Entities

**Transient Entities** exist only while the user who owns them is present in the space. They are designed to be temporary, meaning that they are automatically removed when that user leaves.

An example of a transient entity is the **Avatar Entity**. Since an avatar represents the user, it should only exist during that user's session. Once the user logs out or exits the space, their avatar is no longer needed and is removed from the space. This ensures the space does not become cluttered with inactive avatars.

Transient entities are particularly useful in scenarios where real-time interaction is essential, but the presence of certain entities is tied directly to specific users. By making these entities transient, you prevent unused or irrelevant objects from lingering in the space, which could otherwise lead to visual clutter or even performance issues.

### Persistent Entities

**Persistent (non-transient) entities** are objects that remain in the space, regardless of whether users are present. These entities are intended to persist across user sessions and to remain in the space to maintain continuity.

An example of a persistent entity is a piece of furniture or other decorative items. These objects are part of the space's design and are not tied to the presence of any specific user. As such, they must remain in place even when no users are actively in the environment. Persistent entities create a stable environment that doesn't change unexpectedly when users leave, ensuring that the space looks the same for each user every time they log in.

Persistent entities are ideal for objects that define the space's structure, behavior or aesthetic and must be available at all times. These can range from simple visual elements, like a painting on the wall, to more functional objects, like furniture or interactive devices that users can engage with whenever they are present in the space.

## The Role of Components

CSP uses an entity-component-based architecture to give space entities their functionality and interactivity. Components are child objects attached to space entities, defining how they behave, interact, and appear in the virtual environment. Each component serves a specific purpose, ranging from visual representation to handling complex interactions.

Components are consequently integral to building meaningful interactions and visuals within a Space. They are not modifications to the characteristics of a `SpaceEntity` but are instead child objects attached to it.

Each component introduces specific functionality to the overall composition, enabling rich and interactive experiences. Here are some examples.

* A `StaticModelSpaceComponent` adds a 3D model to the entity, giving it a visible appearance.  
* A `ButtonSpaceComponent` adds a button as a child, introducing a clickable element.  
* An `AudioSpaceComponent` plays sound, enhancing the immersive experience.

These components allow the `SpaceEntity` to act as a parent for various functionalities, combining them to create a cohesive and dynamic entity in the virtual environment.

The relationship between entity and component in CSP is hierarchical. The `SpaceEntity` is at the top of this hierarchy and serves as the parent.  The attached components act as children, each responsible for a specific aspect of the entity's behavior or appearance. 

This relationship allows for flexible positioning and synchronization of components within the Space, as each component's transform can be adjusted independently, while still maintaining its overall alignment with the parent entity.

### Role of ComponentBase

All components in CSP inherit from the ComponentBase class. This class provides essential functionality for managing the properties of each component and facilitating integration with the Script System.

The ComponentBase class is responsible for handling the replicated component properties. These properties are stored in a map, and the class provides getter and setter methods to access and modify them. 

There are some important aspects to the ComponentBase class that it is worth reviewing in more detail.

* **SubscribeToPropertyChange**  
  This method provides a mechanism by which users can be notified when the value of a specific property changes in their scripts. 

* **Actions**  
  Each component is responsible for managing its own component actions. The ActionHandlers (what to do when an action is invoked) are defined at the client application level as functions to be executed. These actions can then be invoked via authored scripts defined in a Script Component. For example, an application may define ActionHandlers to be invoked for play, pause and reset actions on an AnimatedModelSpaceComponent. These actions can then be invoked via script as shown below:

  ```
  animatedModels[0].invokeAction("PauseAnimation", JSON.stringify(animatedModels[0]));
  ```

* **Properties and the ReplicatedValue type**  
  All component properties are backed by the ReplicatedValue type. ReplicatedValue is an intermediate class that enable clients to pack data into types supported by CSP. Under-the-hood a ReplicatedValue uses a Union to handle the internal value.
  
  ```
  union InternalValue
  {
  	InternalValue();
  	~InternalValue();
  
  	bool Bool;
  	float Float;
  	int64_t Int;
  	csp::common::String String;
  	csp::common::Vector3 Vector3;
  	csp::common::Vector4 Vector4;
  };
  
  InternalValue Value;
  ```
  
  The `ReplicatedValue` provides type-safe CRUD operations on the underlying type.

## Entity Lifetime Management

In CSP, the `SpaceEntitySystem` governs how objects (space entities) are created, modified, synchronized, and deleted within a virtual space. Understanding this flow is crucial for building responsive, real-time applications in multi-user environments. Below is an overview of the expected flow when using space entities.

1. **Entity Creation** 

    The first step in the entity lifecycle is **creating** a space entity. A `SpaceEntity` acts as the base object, which can represent anything from a user's avatar to an inanimate object like furniture. Each `SpaceEntity` is instantiated with basic properties like position and can be customized by attaching various components.
	
	```
    EntityCreatedCallback callback = [](SpaceEntity* entity) {
        // Respond to entity creation here...
    }
    SpaceTransform transform = SpaceTransform(); // Create an identity transform. 
	SpaceEntity* entity = spaceEntitySystem->CreateEntity("myEntityName", transform, callback);
	```

    This code creates a new `SpaceEntity` within the space. Once created, you can modify this entity by adding components that define its behavior and appearance.

2. **Component Addition/Modification**

	After creating a `SpaceEntity`, you can add or modify components to control its interaction, visual appearance, or other behavior. Components are child objects that give the entity functionality, such as movement, rendering, or interaction.
	
	```
	StaticModelSpaceComponent* model = entity->AddComponent(ComponentType::StaticModel);
	model->SetExternalResourceAssetId("table_model");
	```

    In this example, a static model component is added to the `SpaceEntity`, representing a table in the virtual space. You can also modify components to refine their behavior or appearance as needed.

3. **Multi-User Replication**

    In a multi-user setting, it is essential to synchronize changes across all connected clients. When an entity or component is created or modified, CSP will automatically handle any queued updates to ensure all clients have an updated view of the space.
	
	```
	entity->QueueUpdate();
	```

    This method ensures that any changes to the entity, such as its position or appearance, are sent to all connected clients. Synchronization happens in real-time, allowing all users to see the updated state of the entity.

4. **Entity Deletion**

	**Space entities** can be deleted when they are no longer needed. 

	```
	space->DestroyEntity(entity);
	```

    This command deletes a `SpaceEntity`, removing it from the space.

### Importance of SpaceEntityUpdates

Responding to entity updates from remote users is vital for maintaining real-time synchronization across all clients. Any queued change to a `SpaceEntity` or its components triggers an update, which must be efficiently managed to ensure that all users see the same state of the space simultaneously.

```
entity->SetUpdateCallback(updateHandler);
```

By setting an update callback, you can ensure that any changes made to an entity are correctly processed and replicated across all clients. This helps maintain consistency in multi-user environment, avoiding discrepancies between users' views of the space.

## Summary

In this topic, you learned about managing space entities in CSP. We covered key areas, including the entity-component hierarchy, the difference between transient and persistent entities, the role of components in CSP, the unique function of avatar entities in multi-user spaces, and the application flow for handling space entities.

* **Entity-Component Hierarchy**: Space entities act as containers, with components adding behavior and visual elements. This structure enables interactive and dynamic environments in CSP.

* **Transient vs. Persistent Entities**: Transient entities, like avatars, exist only while users are present. Persistent entities, such as furniture, remain in the space even when users exit.

* **CSP Components**: Components define how space entities behave and appear. These include static models, animations, audio, video, and interactive scripts.

* **Avatar Entities**: Avatars represent a user in multi-user spaces and automatically synchronize across clients, ensuring real-time updates and consistent representation for all users.

* **Application Flow**: Managing space entities involves creating entities, adding or modifying components, synchronizing changes across clients, and deleting transient entities when users leave. Proper handling ensures smooth, real-time multi-user experiences.
