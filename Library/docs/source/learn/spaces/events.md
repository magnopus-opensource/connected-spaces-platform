# Events

Event and messaging systems drive multiplayer communication and entity synchronization in CSP, making it central to creating dynamic, responsive multi-user environments. CSP also provides an application-facing API to allow applications to emit and respond to bespoke events and interact with one another in real time. 

```eval_rst
CSP supports this through the :class:`csp::multiplayer::EventBus`, which manages **SignalR Multiplayer Events**. Together, these enable real-time communication across various components and clients.
```

* The EventBus manages internal and remote events within the CSP infrastructure, allowing various subsystems to share updates, notify client applications, and maintain a coordinated state.  
* SignalR Multiplayer Events are the medium through which CSP notifies other remote client applications, facilitating the transfer of messages and data across the network in a shared multi-user environment.

## Custom Events

```eval_rst
CSP offers flexibility in event handling by allowing clients to register or stop listening for specific events. For example, the :func:`csp::multiplayer::EventBus::ListenNetworkEvent` method registers a callback for a bespoke event, allowing clients to receive custom events by name. 

Clients can specify any event name to listen for, and CSP will trigger the registered callback when that event occurs. Conversely, if a client no longer needs to listen for an event, it can use :func:`csp::multiplayer::EventBus::StopListenNetworkEvent` to stop receiving updates for that event. This method efficiently reduces network traffic and ensures clients only process necessary data.
```

### Receiving Custom Events

```eval_rst
In a multiplayer environment, clients often need to respond to a range of custom events. CSP's :func:`csp::multiplayer::EventBus::ListenNetworkEvent` method enables applications to listen for specific events across all connected clients. By registering an event listener with a unique event name, clients can trigger a callback when that event occurs, ensuring they're ready to act on updates from other users.
```

Example:

```
EventBus->ListenNetworkEvent("CustomEventName", [](const Parameters& params) {
    // Handle the received event
});
```

This setup allows developers to tailor client responses to unique scenarios, from contextual triggers to user actions, ensuring a highly interactive space. 

### Sending Custom Events

```eval_rst
CSP also supports broadcasting events to all users within a space, allowing a single client to communicate with multiple users simultaneously. The :func:`csp::multiplayer::EventBus::SendNetworkEvent` method enables clients to send custom events with a unique event name and a payload containing the event details. This approach is useful for scenarios where all clients need to be informed of an event, such as an important space update.
```

Example:

```
EventBus->SendNetworkEvent("CustomEventName", eventPayload, [](ErrorCode error) {
    // Confirm event sent successfully or handle errors
});
```

By using `SendNetworkEvent`, developers can ensure that every user in the space receives important updates, enhancing consistency across the platform.

### Targeted Custom Events

```eval_rst
Sometimes, an event should only reach a specific client rather than all users. CSP allows targeted event delivery using the :func:`csp::multiplayer::EventBus::SendNetworkEventToClient` method. Developers can send custom events to individual clients by specifying a client Id. This feature is helpful for private messages, individual user updates, or tailored instructions.
```

Example:

```
EventBus->SendNetworkEventToClient("TargetedEventName", eventPayload, targetClientId, [](ErrorCode error) {
    // Confirm event sent to specific client or handle errors
});
```

This function gives developers control over which users receive specific updates, making it easier to personalize interactions and deliver user-specific content.

## Predefined Events

In CSP, there are certain events, or callbacks, which are automatically transmitted and received on behalf of the client application, as they either require unique handling due to their impact on system functions, or they require applications to be able to consistently reason about them regardless of the client application tech stack.

```eval_rst
These include critical events such as when assets are updated (via :func:`csp::systems::AssetSystem::SetAssetDetailBlobChangedCallback`) or when user permissions have changed (via :func:`csp::systems::UserSystem::SetUserPermissionsChangedCallback`). Each of these events carries specific information that affects the client experience, often requiring dedicated processing within CSP before they're passed to client applications.
```

### General Callback Pattern for Predefined Events

CSP allows developers to register specific callbacks for each event, ensuring they can handle event data as it arrives. To set up a callback, developers define a function that will execute when the event triggers and pass that to the corresponding CSP API endpoint.

Here's an example of how to set up a callback for the `AssetDetailBlobChanged` event:

```
// Register callback for AssetDetailBlobChanged
AssetSystem->SetAssetDetailBlobChangedCallback([](const AssetDetailBlobParams& params) {
    // Process asset details and update the client
});
```

By registering to these callbacks, developers can ensure that client applications handle special events appropriately, whether by updating local assets, reflecting user permissions, or displaying new messages.

### Execution Flow for Predefined Events

When CSP receives a predefined event, it processes it through the following sequence before passing it on to clients:

1. **Event Reception**: CSP receives the event from the server and identifies it as a special case in the EventBus.

2. **Local Processing**: CSP processes the event data internally, handling any necessary deserialization or data validation.

3. **Routing to Callback**: Once processed, CSP executes the relevant registered callback, forwarding the event to the client application.

4. **Client Handling**: The client receives the event data and applies necessary application-level logic, such as adjusting permissions, displaying messages, or updating assets.

## Responding to Network Interruptions

Managing responses to general events in a multi-user environment is essential to providing a seamless experience, and in these scenarios, network interruptions are inevitable.

```eval_rst
With CSP, you can set up a specific response for this predefined event using :func:`csp::multiplayer::MultiplayerConnection::SetNetworkInterruptionCallback`. This callback activates when an interruption in the connection occurs, and it provides the reason for the failure to the client. Since connections cannot be automatically recovered, the general best practice is to disconnect the client to prevent errors.
```

Handling interruptions this way maintains system stability and keeps users informed.

## Best Practices and Considerations

```eval_rst
1. To optimize event handling in CSP, keep event listeners focused and lightweight. 

2. Register only the necessary listeners and avoid redundant callbacks. 

3. Use the :func:`csp::multiplayer::EventBus::StopListenNetworkEvent()` method to remove listeners for events no longer needed, helping to conserve resources and streamline data flow.
```

Example:

```
// Unregister unnecessary listeners
EventBus->StopListenNetworkEvent("MyEvent");
```

### Error Handling and Debugging

Effective error handling in event callbacks is critical for maintaining a stable multiplayer experience. 

1. Use error codes provided by CSP's methods to diagnose issues quickly and/or procedurally.

2. Set up conditional checks within each callback to capture and log errors, which helps in identifying the source of problems during runtime.

Example:

```
EventBus->SendNetworkEvent("UpdatePosition", eventPayload, [](ErrorCode error) {
    if (error != ErrorCode::Success) {
        // Log error for debugging
        DebugLog("Event send failure: ", error);
    }
});
```

### Security Considerations

Security is vital in multi-user environments, where data frequently passes between clients. The following practices can be helpful to preserve levels of user security in your connected spaces ecosystem.

1. Validate all incoming events. 

2. Avoid sharing sensitive data through public events.

3. Implementing a whitelist for acceptable event types is also recommended, as it limits interactions to predefined events and minimizes the risk of unauthorized event handling.

Example:

```
// Verify event type before processing
if (IsAllowedEvent(eventType, eventData)) {
    ProcessEvent(eventData);
} else {
    DebugLog("Unauthorized event attempt blocked.");
}
```

## Summary

In CSP, events are a powerful mechanism that helps enable cohesive multiplayer environments. This topic covered events in CSP (both internal and custom) and  network-wide interactions. It also discussed setting up callbacks for general events like network interruptions, ensuring that clients respond seamlessly to changes in the environment. 

Handling special events like `AssetDetailBlobChanged` and `UserPermissionsChanged` ensures clients remain updated on critical changes. Finally, following best practices for efficient event handling, error management, and security helps maintain a stable, responsive multiplayer experience.
