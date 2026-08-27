# Data-Driven Components

In contrast to adding a new component by writing code [in C++](adding_components), which necessitates modifying the CSP library itself, building and distributing a new release etc, a "data-driven" component is one described by a schema, specifically a JSON document, that can be defined externally from the library and injected at runtime, allowing clients to author their own components for use in their connected applications.

The schema describes the component, its properties, their types, any constraints on values, names for exposing to the script system etc, and then CSP takes care of replicating the component state as it always has, and provides a dynamic runtime API for reading and writing properties, ensuring that values conform to those described in the schema when set programmatically. 

Below is an example of such a component which we'll reference throughout:

```json
{
  "typeId": 808,
  "name": "DistortionAudioEffect",
  "properties": [
    {
      "key": 0,
      "name": "gain",
      "type": "float",
      "defaultValue": 0.25,
      "range": {
        "min": 0.0,
        "max": 1.0
      }
    },
    {
      "key": 1,
      "name": "level",
      "type": "float",
      "defaultValue": 0.5,
      "range": {
        "min": 0.0,
        "max": 1.0
      }
    },
    {
      "key": 2,
      "name": "mode",
      "type": "int",
      "defaultValue": 0,
      "options": [
        {
          "name": "Vintage",
          "value": 0
        },
        {
          "name": "Modern",
          "value": 1
        }
      ]
    }
  ]
}
```

## Schema Reference

A data-driven component conforms to the JSON structure detailed below
### Component Schema object

| Field | | |
|---|---|---|
| `typeId` | required | Integer (`uint64_t` on the wire, but as this is JSON we're limited to 53 bits). Must be unique across everything registered with an engine. |
| `name` | required | String, `PascalCase`. Must be unique across registered components. Names the script binding: `Example` gives `ThisEntity.getExampleComponents()`. |
| `scriptable` | optional | Bool, default `true`. Whether to register the component with the embedded script system. |
| `properties` | required | Array of property objects. |

### Property object

| Field | | |
|---|---|---|
| `key` | required | Integer (`uint16_t` on the wire), `0` to `64510`. The library reserves the keys above that. Identifies the property within its component on the wire. Unique within the component, though not globally. |
| `name` | required | String, `camelCase`, unique within the component. Names the property in scripts: `example.propertyName`. |
| `type` | required | One of `bool`, `int`, `float`, `string`, `vec2`, `vec3`, `vec4`, `stringToStringMap`. |
| `defaultValue` | required | A value matching `type`. New instances are initialised from it. |
| `range` | optional | `{ "min": n, "max": n }`, inclusive. `int` and `float` only. |
| `options` | optional | Array of `{ "name": string, "value": v }`. `int`, `float` and `string` only. Mainly for enums, whose cases are usually ints. |
| `scriptable` | optional | Bool, default `true`. Whether to register the property with the embedded script system. If `scriptable` is false at the component level, that takes precedence. |

`range` and `options` are currently mutually exclusive. A value is unconstrained, bounded (has a `range`) or enumerated (has `options`), and declaring both is a parse error. The `value` of an option is what replicates, and its `name` is a label for a client presenting a choice (i.e. in a UI).

How `defaultValue` is written follows from `type`:

| `type` | JSON | Example |
|---|---|---|
| `bool` | a bool | `true` |
| `int` | an integer | `42` |
| `float` | a number | `0.25` |
| `string` | a string | `"soft"` |
| `vec2` | an array of exactly 2 numbers | `[1.0, 2.0]` |
| `vec3` | an array of exactly 3 numbers | `[1.0, 2.0, 3.0]` |
| `vec4` | an array of exactly 4 numbers | `[1.0, 2.0, 3.0, 4.0]` |
| `stringToStringMap` | an object whose values are all strings | `{ "modelA": "materialA" }` |

## Usage

The following code snippets are in C++, but are illustrative of the general shape and so should hopefully translate easily to any of the other languages CSP provides bindings for.

### Registering Component Schemas

The library expects a serialized string of JSON, specifically a JSON array of Schema objects as described above, to be passed on construction of the engine. It is up to the client developer to decide how to distribute / bundle the component definitions within their applications.

In the snippet below, we'll refer to an existing variable `JsonSchemas` that is known to contain at least the example JSON schema from earlier. The variable could have been initialised with a string literal, some otherwise embedded resource, or sourced from somewhere at runtime (it is worth noting that large string literals may be an issue for some C++ compilers, so it is advised to use whatever asset bundling mechanism is typical for the application's underlying engine or target platform).


```cpp
auto& SystemsManager = csp::systems::SystemsManager::Get();

auto& LogSystem = *SystemsManager.GetLogSystem();

// It is important that the LogSystem has a callback set before instantiating the engine, as any errors with parsing
// schema will be reported via the LogSystem
LogSystem.SetLogCallback([&](auto LogLevel, const auto& Message) {
    // ...details
});

// Note that despite the fact `JsonSchemas` is itself a string containing a JSON array, the API takes a List of these.
// This is primarily a workaround for an issue in our legacy wrapper generator.
const auto Schemas = csp::common::List<csp::common::String> { JsonSchemas };

// Instantiate an engine with the schemas. An engine instance has a fixed set of known components for its lifetime.
auto Engine = csp::multiplayer::OnlineRealtimeEngine {
    *SystemsManager.GetMultiplayerConnection(),
    LogSystem,
    *SystemsManager.GetEventBus(),
    *SystemsManager.GetScriptSystem(),
    Schemas,
};
```

### Adding a Component to an Entity

With an entity obtained from the `Engine` above, adding a component involves knowing its `typeId`. These will need to be sourced from the schema, and likely defined within the client application with some named constants. This could be done by hand, and likely would be in the first instance. Once a pattern is established, these could be generated automatically for any given schema definition via a layer of custom codegen (and the same will be true for the other code snippets that follow).

```cpp
const auto DistortionAudioEffectTypeId = uint64_t { 808 };

auto* Component = entity->AddComponentByTypeId(DistortionAudioEffectTypeId);
```

### Reading Component Properties

Traditionally, there has been a concrete class for each component type that CSP provides, and the caller was expected to cast to that (knowing the mapping from `ComponentType` to class), and could then call distinct methods for reading and writing properties, in a type-safe way.

Defining components at runtime necessitates a more dynamic API, looking up properties by key, and using `ReplicatedValue`. As above, the client code needs to know information from the schema to interact with the library, specifically the property keys and what type each property is, in order to obtain a value and interact with it, or to set a new value (note, that when programmatically setting a value the library will validate the given value against the schema, and reject the edit on mismatch, logging accordingly).

Again, the property keys will need to be manually defined or generated from the schema, and will likely either be more named constants or defined using an enum (similarly `int` properties with distinct `options` may reasonably be represented as enums).

```cpp
// named constants for keys
struct DistortionAudioEffectPropertyKeys
{
    static constexpr auto Gain = uint16_t { 0 };
    static constexpr auto Level = uint16_t { 1 };
    static constexpr auto Mode = uint16_t { 2 };
};
```

```cpp
const auto* CurrentGain = Component->GetProperty(DistortionAudioEffectPropertyKeys::Gain); // non-owning, nullptr if key not found

const auto Updated = csp::common::ReplicatedValue { CurrentGain->GetFloat() + 0.1f };

Component->SetProperty(DistortionAudioEffectPropertyKeys::Gain, Updated);
```

### Responding to Component Changes

When an entity is notified of a remote update, for example when a component is added, a typical pattern in client code is to obtain a `ComponentBase` instance from the entity via `GetComponent` using `ComponentUpdateInfo::ComponentId`, and switch on the result of calling `GetComponentType()` on the instance, in order to decide what app-specific representation of the component to instantiate and associate the CSP instance with. `GetComponentType()` returns a `ComponentType`, and an injected schema component isn't typically represented there, so `ComponentType::Invalid` is returned. `GetTypeId()` returns an integer instead, and works for both the built-in components and dynamically injected schema components (for a built-in component that integer is exactly its `ComponentType` value cast to a `uint64_t`).

```cpp
auto* Component = Entity->GetComponent(ComponentId);

switch (Component->GetTypeId())
{
case DistortionAudioEffectTypeId:
    // ... instantiate a wrapper class or whatever is appropriate
    break;
default:
    break;
}
```

As `GetTypeId()` covers both kinds of component, it is worth moving to across the board. An alternative would be to fallback to `GetTypeId()` after additionally checking if `ComponentType::Invalid` is received, or if the `default` case of an existing switch is hit.

### Scripting

A component is automatically bound into the script system if it is `scriptable` (which is the default), has a non-empty `name`, and has at least one scriptable property (again, if it is `scriptable`, which is the default, and it has a non-empty `name`):

```js
const effect = ThisEntity.getDistortionAudioEffectComponents()[0];

effect.level = effect.level + 0.1;
```

Assignment goes through the same validation as `SetProperty`, and a value the schema rejects will throw. `vec2`, `vec3` and `vec4` appear as JS arrays, and `stringToStringMap` as a plain object.

## Component TypeId allocation

The `typeId` for each component must be globally unique within the context of any connected application built with CSP. It is recommended that clients maintain their schema in a central place, especially if consumed by multiple applications within the same connected experience, so that they can coordinate changes. One of the main reasons for providing a JSON array of Schemas on construction, is so that any given client is encouraged to declare all their schemas centrally so that their allocated `typeId`s are visible in context with each other.

Component state is persisted on the server, and the `typeId` is what distinguishes a component when it arrives on the wire / over the multiplayer connection. Data-driven components currently abstract over the same existing wire protocol as the traditional hand-written C++, where historically the `ComponentType` enum was the source-of-truth for these and the integer representation of that is sent over the wire as an unsigned 64-bit integer. Currently, how any given application decides to partition that 64-bit address space is up to them; any IDs used for externally defined components that clash with CSP's built-in components will be rejected by the engine.

This is obviously not ideal, and something we may consider changing. String identifiers, perhaps in a reverse domain format (e.g. `net.connected-spaces-platform.components.example`) would be one such alternative way to ID a component (which would also make them more easily shared across different apps, as currently the `typeId` is a bit of a portability issue, it could be considered a mapping at the app level of a type identifier to component vs an intrinsic ID of the component). This would impact the library API surface, and potentially cause data-driven components to be different to traditional components on the wire (depending on implementation).

## Schema Compatibility / Changing schemas over time

Component types used to be fixed in a CSP release, and clients moved to a new release together in order to pick them up. A shared schema document works the same way in practice, and is best consumed by every client in one step. What is different is that an experience can define components of its own, without being limited to the set CSP ships or having to contribute to it, but changes do still need to be coordinated during development of those connected applications.

Once a schema for a component has been published, ideally it wouldn't change. There isn't currently an explicit versioning mechanism for schemas, but changes to them are possible as long as some core rules are followed. Traditional hardcoded components have been evolved over time and follow some implicit rules, which we'll outline here, to avoid breaking existing clients when schemas change.

- A `typeId` or a `key` must never change meaning. Never point one at something else, or old clients will read unexpected data
- A property's `type` must never change, similar to the above
- Adding a property under a new key is safe. Clients that don't know about it carry on without it (this is safe from a data perspective, but may not make sense within the domain of the app itself if something is missing from the space etc)
- Removing a property is possible, as long as the key isn't reused. Old clients will continue to initialise missing properties with the defaults defined in the older version of the schema they were shipped with, and newer clients will simply ignore those fields.
  - It is advisable to preserve all properties in the schema still, even if you want to formally remove them. There isn't a specific mechanism for this at the moment, but whilst not documented above (as CSP doesn't use the information), a reasonable thing to do would be something like `{"typeId": 42, reserved: true }`, or `{"key": 0, reserved: true }`.

Other changes are possible but generally inadvisable for similar compatibility reasons:

- Renaming a component or a property, or setting `scriptable` to `false`, breaks scripts written against the old names.
- Tightening a `range` or removing an `option` leaves values already stored untouched, because validation happens when a value is written. Technically workable, but may complicate app logic.

This currently needs to be enforced by convention, and is currently the responsibility or the authors of the set of schemas used within any given connected application.

## Built-in components

As mentioned above, CSP still has its own components that occupy the same ID space. Internally, CSP is using schema definitions to drive automatic script bindings, and the `GetProperty`/`SetProperty` API also work, so client code could move over to those wholesale (for the most part, there are some components that have behaviour which can't reasonably be moved to this model fully yet).

You can obtain JSON representations of the built-in components via `csp::GetComponentSchemasJson()`. It is technically possible to inject those schemas too on construction, and even evolve them ahead of the library if that makes sense for your purposes (if a compatible change is made, which basically boils down to adding new properties and preserving all the existing ones).