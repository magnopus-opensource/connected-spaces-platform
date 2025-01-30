# Script System

The entity-component-based nature of the CSP multiplayer architecture comprises individual components, such as Images, Videos and Buttons, which can be combined together to create unique pieces of content.

```eval_rst
The Connected Spaces Platform's :class:`csp::multiplayer::ScriptSpaceComponent` and :class:`csp::systems::ScriptSystem` are what enables content creators to author complex and engaging interactions with this content via CSP client applications.
```

The Script Component is the glue that holds these components together. Using the Script Component, creators can define the causal relationships between different components and triggering events.

## EntityScript
The Script Component can be added to all Entities, including Avatars. Like the other components, it has replicated properties accessible from CSP clients via getters and setters. The Script Component's associated JavaScript code is stored as a replicated string property.

The JavaScript code is compiled to bytecode and interpreted by [QuickJS Javascript Engine](https://bellard.org/quickjs/), which supports the ES2020 standard and features.

To access the script on a CSP Entity we use the following:

`EntityScript* Script = Entity->GetScript();`

There are several useful things you can do with `EntityScript`, including updating the script, invoking it directly, checking for errors, and getting error messages back. For more information on the API, head over [here](https://builds.magnoboard.com/connected-spaces-platform/api/classcsp_1_1multiplayer_1_1_entity_script.html#class-documentation).

## Actions
```eval_rst
Scripts can invoke events, known as actions, to which client applications can respond directly to. At the client level, each component is responsible for defining and managing its own component actions using the `RegisterActionHandler` and `UnregisterActionHandler` functions on :class:`csp::multiplayer::ComponentBase`.
```

So if a script invoked an action like this...
```js
processButtonClick()
{
    invokeAction('PlayVideo')
}
```
Then an Unreal client could respond to it at the client level like this...

```c++
void MyUnrealCSPComponent::RegisterActionHandler(...)
{
    ...
    // Initialise the action handler for a 'play video' action
    const csp::multiplayer::ComponentBase::EntityActionHandler PlayVideoHandler =

        [](csp::multiplayer::ComponentBase& Component,         
        csp::common::String ActionId, csp::common::String ActionParams)
        {
            // my response to the action being fired from script
        };

    // Register the action handler
    BoundCSPSpaceComponent->RegisterActionHandler("PlayVideo", PlayVideoHandler);
}

void MyUnrealCSPComponent::UnregisterActionHandler()
{
    ...
    // Unregister the action handler for the play video action
    BoundCSPSpaceComponent->UnregisterActionHandler("PlayVideo");
}
```

## Messages
Script messages are synonymous with events within the CSP scripting runtime environment. Any object with access to the Script Component of a specific SpaceEntity can post messages to it. The Script Component can then define how it responds to that message.

### Sending Messages between Entity Scripts
Using the script system, entities can send messages to other entities running scripts. Scripts can post messages to one another using the 'PostMessage' function.

So when one entity posts a `HeyThere` message...

```js
PostAMessage()
{
    var spaceEntity = TheEntitySystem.getEntityByName("OLY-SPACEENTITY-2115");
    spaceEntity.postMessage("HeyThere", "MyParameters");
}

```

The other entity can respond to it.

```js
SomebodySaidHey(Params)
{
    // respond to the message
}

ThisEntity.subscribeToMessage("HeyThere", "SomebodySaidHey");
```

## Referencing other scripts as Modules
Scripts can import and reference other scripts as Modules using the import keyword with the name of the entity to import the script from.

`import {MyTriggers} from "CSP-SCRIPTENTITY-TRIGGERS-MODULE"`

## Leader Election
For coherence purposes, it is important that scripts are ticked by one and only one client application at any given time. The leader election system provides a method for CSP-based clients to agree on a single client to handle the execution of scripts.

The current system is based on the [Bully Algorithm](https://en.wikipedia.org/wiki/Bully_algorithm#Algotithm), using the `ClientId` to determine the Leader.

### Process Summary
The algorithm uses the following message types:

* Election Message: Sent to announce election.
* Answer (Alive) Message: Responds to the Election message.
* Coordinator (Victory) Message: Sent by winner of the election to announce victory.

When a process P recovers from failure, or the failure detector indicates that the current coordinator has failed, P performs the following actions:

> 1. If P has the highest process ID, it sends a Victory message to all other processes and becomes the new Coordinator. Otherwise, P broadcasts an Election message to all other processes with higher process IDs than itself.
> 1. If P receives no Answer after sending an Election message, then it broadcasts a Victory message to all other processes and becomes the Coordinator.
> 1. If P receives an Answer from a process with a higher ID, it sends no further messages for this election and waits for a Victory message. (If there is no Victory message after a period of time, it restarts the process at the beginning.)
> 1. If P receives an Election message from another process with a lower ID it sends an Answer message back and if it has not already started an election, it starts the election process at the beginning, by sending an Election message to higher-numbered processes.
> 1. If P receives a Coordinator message, it treats the sender as the coordinator.

### Leader Heartbeat
The purpose of the leader heartbeat is to allow the leader election system to respond to a connection error with the current leader. Without the heartbeat, if the leader's connection drops, then all leader functionality (e.g. running scripts) will stop until the server notices that the leader is no longer there, which could currently take up to several minutes.

When a client becomes a leader it will start sending periodic heartbeat messages to all other clients. The heartbeat period is configurable, but defaults to every 5 seconds.

Clients expect to receive this heartbeat, and if they don't receive it after some period of time (again configurable, but currently 3 heartbeat periods) then they will broadcast a 'Leader Lost' message to other clients.

If enough 'Leader Lost' messages are received by any client then they will initiate a new leader election.

The purpose of this approach is to try and avoid a re-election being caused by just one or two clients temporarily losing contact with the leader. Instead a majority must agree before the election is triggered.

When some threshold (currently more than half) of the clients have lost contact with the leader then they will re-elect.

Note it is possible that this re-election could re-elect the same leader again if their connection has returned.