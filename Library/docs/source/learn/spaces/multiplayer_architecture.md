# Multiplayer Architecture

The multiplayer architecture in the Connected Spaces Platform (CSP) is designed to enable real-time, interactive, and scalable experiences in any given space. It relies on a robust tech stack to ensure seamless communication between clients and servers, allowing many users to engage within the same space.

In service of this, various systems within CSP manage key components, including **spaces, entities, scripted behavior, assets and space anchoring**. These systems work together to handle the state and behavior of objects in the virtual space, synchronizing them across all connected clients.

One critical aspect of CSP's multiplayer design is its ability to handle **real-time communication** efficiently. It leverages the **SignalR protocol**, which enables continuous two-way communication between servers and clients, unlike traditional **RESTful APIs** that rely on request-response patterns.

SignalR ensures that updates to space entities, user actions, and events are transmitted immediately, creating a responsive multi-user environment.

When used in conjunction with MCS, the multiplayer system within CSP's architecture supports service-level features like **persistence**, **global scalability**, **load balancing**, and **regional deployment**. These ensure that multiplayer interactions are not only real time but also reliable and scalable, accommodating many users across different regions.

Understanding this architectural design is crucial for anyone working on multi-user projects with CSP, as it underpins how spaces are created, managed, and interacted with in real-time. This section will explore the key components of the multiplayer stack, how SignalR facilitates communication, and why multiplayer communication is distinct from RESTful API patterns elsewhere in CSP.

## Event-Driven Design

In real time multi-user environments, an  **event-driven design** is necessary. Events such as user movements, object interactions, or system updates must instantly propagate to all users, and they happen all the time. An event-driven design ensures that each user receives updates as soon as they occur.

**State Synchronization** is another critical feature in multi-user real time environments. The system must coordinate the coherence of all connected clients and the shared space itself. State synchronization ensures that all clients are in sync, meaning they all see the same state at the same time.

**Efficiency** is required in order to support many users being copresent within the same space. By maintaining a continuous connection which transmits *only those states which have changed* for a given entity, CSP minimises overhead and latency. This makes it much more efficient for handling the rapid, continuous updates essential in real time applications.

## SignalR

Within the realm of connected spaces, spatiotemporal coherence amongst users is key to facilitate copresence. All users should expect to see the same thing, at the same place, at the same time.

SignalR is a powerful real-time communication protocol that is used by CSP to facilitate these multi-user interactions. Unlike traditional communication methods, SignalR maintains an active connection, enabling real-time updates with low latency between server and client.

While WebSockets enables bidirectional, long-lived communication channels, SignalR builds on this with added features and fallbacks. SignalR can automatically switch to other communication techniques, such as Server-Sent Events (SSE) or Long Polling when WebSockets are unavailable. This ensures that SignalR can function in environments with restricted WebSocket support, providing greater flexibility and compatibility.

### How SignalR is Used in CSP

In CSP's multiplayer architecture, SignalR handles real-time data transmission for users' actions, events, and state synchronization (both inside and outside of spaces).

For instance, when a user moves or interacts within a space, the information is transferred via SignalR to ensure that other users within the space immediately receive the updates. This process happens in real time, so all users see a synchronized and consistent state with minimal lag or data loss.

Instead of traditional polling, where clients must constantly request updates from the server, SignalR pushes updates as soon as changes occur.

### Advantages of Using SignalR

SignalR offers several advantages over traditional polling or long-polling methods:

* **Low Latency**: SignalR minimizes delays, providing real-time interaction with near-instantaneous data transmission.

* **Persistent Connections**: It maintains a persistent connection, reducing the overhead of constantly re-establishing communication.

* **Scalability**: SignalR's architecture allows for better scalability, ensuring that large numbers of concurrent users can interact smoothly without overwhelming the server.

## SignalR vs RESTful API

In CSP, there are two primary communication methods that serve distinct purposes: **RESTful APIs** and **SignalR**. Each is designed to handle different types of interactions and offer specific advantages depending on the nature of the task.

**RESTful APIs** in CSP follow a request-response model. This pattern is typically used for non-real-time transactional interactions such as login, retrieving user data, or submitting changes to the system. RESTful APIs are stateless, meaning each request from a client to the server is independent of previous interactions. The server processes the request, performs the necessary action, and returns the result. This makes it ideal for tasks that do not require continuous communication between the server and the client.

**SignalR**, on the other hand, is designed specifically for real time, continuous, event-driven updates. It operates in a persistent connection model, which means that once a connection is established, the server and client can exchange data continuously without the need to open and close connections for each interaction. This type of communication keeps all connected clients in sync by pushing updates as soon as they happen, ensuring that every user sees the same version of the shared environment at all times.

###  RESTful API Pattern in CSP

The RESTful API pattern in CSP is used for operations that don't require immediate feedback or live updates. For instance, user authentication typically uses RESTful APIs. When users log into the system, they send a login request that includes their credentials. The server processes the request, validates the credentials, and responds with the necessary authentication token or error message. The client then uses this token to access the system's resources.

Another common use case for RESTful APIs in CSP is data retrieval. Users can send a GET request to the server if they want to fetch information about a space, like metadata or the assets contained within. The server processes the request and returns the requested data. Because this operation doesn't need to occur in real time, the slight delay that comes with the request-response cycle is acceptable. RESTful APIs are great because they're efficient, stateless, and scalable. Each interaction stands alone, and the server doesn't need to maintain an ongoing connection to the client once the request is fulfilled.

### SignalR

SignalR, by contrast, is designed for scenarios where real-time, continuous updates are necessary. This is particularly important in environments where many users are copresent and interacting with one another in real-time. For example, in a virtual world where multiple users interact simultaneously, the system must constantly broadcast the changes to positions and actions of each user to all other participants. This ensures that all users see the same environment and the same events as they unfold.

Unlike RESTful APIs, which rely on separate requests and responses for each interaction, SignalR maintains an ongoing connection via a WebSocket. This allows updates to be sent immediately, without the delay associated with repeatedly opening and closing connections. This continuous connection is vital in fast-paced, real-time environments where delays could result in a poor user experience. Delays of even a few seconds can disrupt the experience and make the application feel unresponsive.

### Examples of Communication Flows

* **RESTful API Flow**: In a RESTful API flow, the client sends a request to the server, such as logging in or retrieving data. The server processes the request and sends a response. This interaction is transactional, meaning it's complete once the response is received. There is no continuous communication, and each request is separate from the others.

* **Multiplayer Service Flow**: In Multiplayer Service Communication, the server maintains a continuous connection with all connected clients. When an event occurs, such as avatar movement or object interaction, the server updates all clients. This ensures that all users see the same version of the environment in real time. Unlike RESTful APIs, Multiplayer Service Communication supports ongoing interaction and updates, ensuring real-time synchronization across all clients.


## High-Level Abstractions

The multiplayer system in CSP is built to support real-time, scalable, and interactive virtual spaces. This section introduces the key high-level abstractions of the multiplayer architecture that client application developers can expect to interact with.

```eval_rst
1. :class:`csp::systems::SpaceEntitySystem` handles all multiplayer network traffic related to updates for Space Entities and their components. It ensures that changes to entities, avatars, and interactive objects are synchronized across all connected clients in real time. This system is essential for maintaining the consistency of shared spaces and ensuring that all users experience the same state of the environment.

2. :class:`csp::multiplayer::MultiplayerConnection` manages the actual connection between the server and clients. It is responsible for establishing and maintaining the network link that supports real-time communication. This system also handles the communication of transient events that can be sent and received via the CSP Event Bus.

3. :class:`csp::multiplayer::SpaceEntity` and the components it may own are foundational to spaces in CSP. An entity could represent a user, a 3D object, or any other element within the space. All entities have movement capabilities, and an avatar is represented by an entity with an Avatar Component by default.
```

The interaction between these systems ensures:

* **Low-latency updates for clients**: Changes within the space are processed and distributed efficiently, ensuring updates reach connected clients without delay.  
* **Consistency across clients**: The CSP multiplayer API abstracts over the protocols it uses, enforces a pub/sub transaction model, and uses terms that are applicable across engines. 
* **Optimized data transmission**: When communicating updates between Magnopus Connected Services and clients, only the deltas (changes) are sent, minimizing network usage and improving performance.

## Summary

In this section, we covered essential concepts surrounding the multiplayer architecture in CSP. You now have a high-level understanding of how the architecture supports real-time, interactive experiences by efficiently managing multiple users in shared spaces. We explored the use of the SignalR protocol, which is crucial for enabling low-latency, real-time communication between the server and clients.

We also distinguished between the roles of multiplayer service communication and RESTful APIs. While RESTful APIs are ideal for transactional operations like logging in or retrieving data, multiplayer service communication is optimized for real time, event-driven updates that require continuous synchronization.

By grasping these core principles, you can better appreciate how CSP creates scalable, dynamic, and interactive multiplayer experiences.

* **Real-time, low-latency interaction**: Enables seamless multiplayer experiences with minimal delays in user actions, movements, and updates.

* **Scalable design**: Supports multiple users across different spaces while maintaining stable performance.

* **Efficient entity and component synchronization**: Ensures all users see the same state of the environment with real-time updates.

* **Event-driven model**: Reduces the need for constant server polling by transmitting only relevant state changes, optimizing bandwidth usage.
