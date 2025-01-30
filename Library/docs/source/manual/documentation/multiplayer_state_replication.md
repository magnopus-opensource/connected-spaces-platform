# Multiplayer and State Replication

The following documentation describes the method of replication in the order of the operations that a data change to an entity will undergo to be replicated.

We use an entity-component model to describe state within a space, which maps to the concept of objects and components used by SignalR, the library which is used internally to communicate with cloud-hosted services.

## Setting Data
A client will access the `SpaceEntity`, and the component of the Space Entity they wish to set a value on. This Component will contain setters for the data the client wishes to set.

As an example, we will use `StaticModelSpaceComponent` to demonstrate, but this would be the same process for any component. Let's say we wish to set the ModelAssetId of this component and replicate it to other clients. We would use something like the following...

```c++
auto& MyStaticModelComponent = (StaticModelSpaceComponent&) MyEntity->AddComponent(ComponentType::StaticModel);

const oly_common::String ModelAssetId = "MyAssetId";
MyStaticModelComponent.SetModelAssetId(ModelAssetId);
```

## Invoking a Patch Message

Once the client has made their changes to the component properties they need to (They are free to make as many changes over any period of time, no replication will occur until they specify), they will queue up a patch message to be sent to cloud hosted services by invoking `QueueUpdate` on the space entity.

```c++
void SpaceEntity::SendUpdate(CallbackHandler Callback);
```

This calls into the `SpaceEntitySystem` (which manages all entities and is responsible for multiplayer service communication) and the entity will be enqueued for patch message transmission.

## Patch Message Serialization

When the entity update is dequeued by the `SpaceEntitySystem` and prepared for transmission, it is first serialised into the expected data format for SignalR and MsgPack, as the payload of `SendObjectPatch`.

```c++
SignalRMsgPackEntitySerialiser Serialiser;

UpdatedEntity->SerialisePatch(Serialiser);
auto SerialisedEntity = Serialiser.Finalise();

std::vector<signalr::value> InvokeArguments = {SerialisedEntity};
Connection->Invoke("SendObjectPatch", InvokeArguments, LocalCallback);
```

The serializer is set to parse over the `SpaceEntity` data in a specific way in order to format the message correctly. it starts with the Dto metadata of `Id`, `OwnerId`, `Destroy`, `ParentId`.

```c++
Serialiser.WriteUInt64(Id);
Serialiser.WriteUInt64(OwnerId);
Serialiser.WriteBool(false); // Destroy
Serialiser.BeginArray();	 // ParentId
{
	Serialiser.WriteBool(false);
	Serialiser.WriteNull();
}
Serialiser.EndArray();
```

Then we start the SignalR component processing, of which there are two types; View Components and CSP Components.

### View Components

A View Component is essentially a core property, present for any every Space Entity. These are:

* Entity Name
* Position
* Rotation
* Scale

 View components are represented by _specific_ keys in the Component map, which are 1024 reserved keys at the end of the component map. These keys can be seen in [SpaceEntityKeys.h](https://github.com/magnopus-opensource/connected-spaces-platform/blob/main/Library/src/Multiplayer/SpaceEntityKeys.h).

### CSP Components

```eval_rst
CSP components are the components that inherit from :class:`csp::multiplayer::ComponentBase` (such as `StaticModelSpaceComponent`), and they utilise the component keys starting from zero and incrementing as new components are added to an entity.
```

In order to serialise these components, we iterate over the `DirtyComponents` map, which contains pointers to all the components we've marked as dirty.

```c++
const oly_common::Array<uint16_t>* DirtyComponentKeys = DirtyComponents.Keys();

for (int i = 0; i < DirtyComponentKeys->Size(); ++i)
{
    auto* Component = DirtyComponents[DirtyComponentKeys->operator[](i)].Component;

    SerialiseComponent(Serialiser, Component);
}
```

`SerialiseComponent` then iterates over all properties and writes them using MsgPack's property packing to convert the `ReplicatedValue`s.

```c++
PropertyPacker.pack_uint64(Id);
PropertyPacker.pack_uint64((uint64_t) Value.GetReplicatedValueType());

switch (Value.GetReplicatedValueType())
{
	case ReplicatedValueType::Boolean:
		Value.GetBool() ? PropertyPacker.pack_true() : PropertyPacker.pack_false();
		Break;
	...
```

As you can see above, we first pack the `Id` for the property, so that we can apply to the correct property in the map on the receiving end. We then pack the type of the replicated value, so that we know what data we should expect when unpacking (This is important as we need to know how many bytes to read, especially for data like strings).

Then, depending on the type of data, we use the appropriate packing function from MsgPack. The code above is clipped to save space, but it currently packs; Bools, Integers, Floats, Strings, Vector3's and Vector4's. More can be added if they are needed, but these have suited our use-cases so far.




## Sending a Patch Message
We touched on the code for sending a patch message previously, where clients call `SpaceEntity::Queue` and we internally queue the entity for serialization and transmission (using the `SendObjectPatch` multiplayer service call).

When the patch is transmitted, we invoke a `ApplyLocalPatch` on the entity whose data is being transmitted. 

This function emulates certain parts of the application of the patch message on the local sending client, so that it can ensure the state of its objects are the same as they would be on receiving clients. For this reason, it also invokes the `EntityUpdateCallback` bound to the entity, so that a client can respond to the update of an entity.

Two of the essential parts of this callback are `SpaceEntityUpdateFlags`, which is a set of enum bit flags that tell the client if a property has been updated, and `ComponentUpdateInfo`, which is a set of enum bit flags that tell the client if a component has been added, deleted, or updated.

Using these callbacks, a client application can update their client-level representation of the entity and the components the entity owns.

> ℹ️ `EntityUpdateCallback` is called from both `ApplyLocalPatch` (local client) and `DeserialiseFromPatch` (remote client), which ensures the client implementations receive equivalent notifications and can apply this information whether it's a sending or receiving client.

## Receiving a Patch Message

When the `SpaceEntitySystem` sets the SignalR connection (`SpaceEntitySystem::SetConnection`), we set the callback for the `OnObjectPatch` multiplayer service invocation.

This callback then fires when we receive a patch message from another client.

When this happens, the callback uses the deserializer to iterate through the data received and unpack it to the relevant Components and Properties. This then fires the same callback as discussed previously, EntityUpdateCallback.

One important thing we do here is handle the Destroy flag in the Dto metadata. If the flag is set, we find the entity and locally destroy it, removing it from CSP and also firing a callback to the client to deal with deletion on their side.

## Deserialising Data & Firing Client Callbacks

This part is very similar to the serialisation, for obvious reasons, and also quite similar to the code used in `ApplyLocalPatch`, especially in the case of the building of `UpdateFlags` and `ComponentUpdateInfo`.

The basic idea here is to apply the received patch message data on the receiving clients' entities and components, while also firing a callback with information on what has changed.

Data is updated as it gets deserialized, and the `UpdateFlags` and `ComponentUpdateInfo` structures are populated at the same time.

Finally, at the end of the deserialization, the callback is fired to the client to let them know things have changed.
