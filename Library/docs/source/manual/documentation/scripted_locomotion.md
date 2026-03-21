# Scripted Locomotion Design

This document describes the current design for creator-authored player locomotion in the NGX / `CodeSpaceComponent` pipeline.

The goal of scripted locomotion is to let a space opt out of the stock player movement model and replace it with creator-authored logic, while still preserving the existing avatar replication and camera systems as much as possible.

## Summary

The key switch is `LocomotionModel::Scripted`.

When the local player's avatar is configured with `LocomotionModel::Scripted`:

- Stock player translation systems stop owning movement for that player.
- Movement can be driven from script via `ThePlayerController`.
- Existing camera behavior remains native by default.
- Avatar/entity state continues to replicate through the normal multiplayer path.

This is intentionally narrower than "full custom player runtime". In the current slice, only locomotion ownership changes. Camera behavior only changes if a script explicitly requests it.

## Goals

- Allow creators to replace stock locomotion with script-authored behavior.
- Preserve existing camera behavior unless a script explicitly overrides it.
- Reuse existing CSP action-handler infrastructure instead of adding a one-off host bridge.
- Keep replication flowing through the existing player/avatar update path.
- Keep the system compatible with future controller styles such as waypoint movement, vehicles, or rolling-ball movement.

## Non-Goals

The current implementation does not try to solve:

- Generic input forwarding into scripts.
- Server-authoritative player locomotion.
- A full camera scripting API.
- A new native "player controller component" type.

## High-Level Architecture

There are four main pieces:

1. Avatar state chooses locomotion ownership.
2. NGX script exposes a stable player-controller surface.
3. A tagged config entity provides the host action target.
4. The client host executes the movement against the local character controller.

At a high level:

1. The local avatar is created with an `AvatarSpaceComponent`.
2. If its `locomotionModel` becomes `Scripted`, the stock translation systems stop driving that local player.
3. A `CodeSpaceComponent` script calls `ThePlayerController.moveCharacter(...)`.
4. The NGX binding resolves a tagged config entity and invokes a component action on it.
5. The client registers an action handler on that component and forwards the request to the local Jolt character controller.
6. Resulting local transform updates continue through the existing multiplayer replication flow.

## Why Actions Are Used

Script-to-host control uses the existing component action pattern:

- Native/client code registers handlers with `ComponentBase::RegisterActionHandler(...)`.
- Script calls `component.invokeAction(...)`.

This was chosen instead of a raw global host object because it:

- Reuses an established CSP extension point.
- Keeps host behavior attached to entities/components.
- Has a clearer lifecycle than an ad hoc global callback surface.
- Fits the ECS/entity model better.

## Script API Surface

The NGX compatibility layer provides `ThePlayerController` with these functions:

- `moveCharacter(x, y, z, jump = false, isFlying = false)`
- `teleportCharacter(x, y, z)`
- `setFirstPersonEnabled(enabled)`

These functions do not directly manipulate the renderer. Instead, they locate a tagged config entity and forward the request via component actions.

## Config Entity Discovery

The current design uses a space entity tagged:

- `player-controller-config`

At runtime, `ThePlayerController` searches `TheEntitySystem.getEntities()` for the first entity with that tag and then finds a component on that entity capable of `invokeAction(...)`.

In practice, this is expected to be a `CodeSpaceComponent` host entity used as the root for creator-authored controller logic.

This keeps the first slice simple and avoids introducing a dedicated native component before the authoring model settles.

## Client Host Execution

On the client side, Electra registers action handlers on `Code` components attached to entities tagged `player-controller-config`.

Current handled actions are:

- `moveCharacter`
- `teleportCharacter`
- `setFirstPersonEnabled`

`moveCharacter` and `teleportCharacter` forward into the existing local Jolt character controller. This is important because it preserves the renderer's current collision and character-motion behavior instead of bypassing it with raw transform writes.

## Locomotion Ownership

The important design distinction is:

- Scripted locomotion disables stock translation ownership.
- Scripted locomotion does not disable the native camera stack by default.

This means:

- The built-in translation systems should not keep calling their own `moveCharacter(...)` when the player is scripted.
- Camera follow, mouse look, camera mode transitions, and related camera systems are left alone unless the script explicitly requests a camera change.

## How Stock Locomotion Is Disabled

The stock translation systems in the renderer now skip entities marked with:

- `ScriptedLocomotionTag`

This tag is applied to the local player when the avatar's `locomotionModel` is `Scripted`.

This approach was chosen because it is more precise than removing `PlayerTranslationComponent` entirely:

- `PlayerTranslationComponent` is still useful for other parts of the runtime.
- Existing camera logic may still assume that translation state exists.
- Replication/update plumbing is less likely to break if the component remains present.

So the current rule is:

- Keep the translation component.
- Keep the camera systems.
- Skip only the stock translation systems.

## Existing Native Behavior That Still Remains

By design, the following remain native unless a script changes them:

- Existing camera systems.
- Existing camera look behavior.
- Existing camera mode management.
- Existing local avatar replication path.

The first-person camera lock is optional and is only engaged when a script calls `ThePlayerController.setFirstPersonEnabled(true)`.

## Existing Native Behavior That Is Suppressed

For scripted avatars, the following built-in movement paths are suppressed:

- Stock physics translation updates.
- Stock non-physics translation updates.
- Built-in double-click teleport locomotion.

This prevents stock locomotion from competing with creator-authored movement.

## Replication Model

Scripted locomotion is still owner-driven in the current design.

The local client drives the character controller locally, and the resulting movement is propagated through the existing avatar/entity update flow so that other clients observe the movement.

This means scripted locomotion currently behaves like a custom local controller layered on top of the existing multiplayer replication model, not a separate authoritative movement runtime.

## Camera Design

The current camera philosophy is conservative:

- Retain existing camera behavior wherever possible.
- Allow scripts to request targeted camera changes.
- Avoid turning scripted locomotion into a full camera rewrite.

At the moment, the only explicit script-facing camera control in this slice is first-person locking through `setFirstPersonEnabled(...)`.

## Intended Authoring Pattern

The intended first authoring pattern is:

1. Create a root config entity tagged `player-controller-config`.
2. Add a `CodeSpaceComponent` to it.
3. Set the local player's avatar locomotion model to `Scripted`.
4. In the script, decide how and when to call `ThePlayerController.moveCharacter(...)`.

This is enough to build initial creator-authored locomotion prototypes such as:

- Waypoint movement.
- Guided-tour movement.
- Vehicle-like movement.
- Rolling / physics-inspired movement.

## Future Extensions

Likely future work includes:

- Input forwarding into scripts.
- More explicit local-player discovery helpers.
- Dedicated authoring components for player-controller configuration.
- Server-scope locomotion support.
- Richer camera control primitives.

## Design Tradeoffs

The current design deliberately prefers a small integration surface over a large new framework.

Advantages:

- Reuses existing CSP action infrastructure.
- Minimizes disruption to the camera stack.
- Reuses existing character-controller behavior.
- Keeps creator-authored locomotion flexible.

Limitations:

- Config-entity discovery is tag-based rather than strongly typed.
- Movement is still client-owned.
- Scripted locomotion is not yet a full input/controller framework.

## Current Rule Of Thumb

For `LocomotionModel::Scripted`:

- Script owns how the player moves.
- Native systems still own how the player camera behaves, unless the script explicitly asks otherwise.
