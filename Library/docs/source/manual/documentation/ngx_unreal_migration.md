# NGX Scripting Migration For Unreal

This note describes the minimum Unreal integration needed to run the new NGX `CodeSpaceComponent` scripting path.

It is aimed at migration and runtime support, not authoring tools.

## Scope

This guide assumes:

- Unreal already uses the CSP shared library
- Unreal already enters spaces through `SpaceSystem`
- Unreal already uses a CSP realtime engine
- Unreal does not need code-component authoring UI

The goal is:

- run already-installed scripts
- support correct Edit/Play behavior inside Unreal Editor
- add extra host features only when scripts actually need them

## Required Core Integration

This guide assumes the existing Unreal integration already handles:

- NGX system creation
- space enter and exit wiring
- CSP foundation ticking

The remaining required migration work is below.

### 1. Drive animation-frame ticking every rendered frame

Call:

- `NgxScriptSystem::TickAnimationFrame(TimestampMs)`

once per rendered frame from Unreal.

This is required for any script using `requestAnimationFrame(...)`.

Recommendation:

- use Unreal's frame time in milliseconds
- call this even if you do not know whether scripts currently use RAF

### 2. Implement Edit/Play runtime mode switching

Unreal runs in the editor, so runtime mode switching is required.

Call:

- `SpaceSystem::SetRuntimeMode(ESpaceRuntimeMode::Edit)`
- `SpaceSystem::SetRuntimeMode(ESpaceRuntimeMode::Play)`

when Unreal changes between editor-editing and play/simulate states.

Important:

- new spaces begin in `Unset`
- code components remain dormant until a mode is chosen

### 3. Sync editor selection into CSP entity selection

This is required if you want `Local` code components to behave correctly in `Edit`.

Current runtime rules are:

- `Editor` components run only in `Edit`
- `Local` components run in `Play`
- `Local` components also run in `Edit` when the entity, or one of its ancestors, is selected by the local client

So Unreal should:

- mark the corresponding `SpaceEntity` selected when the Unreal object is selected
- clear that state when deselected
- use the local Unreal client/editor user as the selecting client

Without this:

- `Editor` components will work in `Edit`
- `Local` components will work in `Play`
- `Local` components will not activate in `Edit`

## Migration Checklist

The shortest practical migration path is:

1. Add per-frame `TickAnimationFrame(...)`.
2. Add Edit/Play runtime-mode switching.
3. Add editor-selection sync to CSP entities.
4. Test one `Editor` code component in `Edit`.
5. Test one `Local` code component in `Play`.
6. Test one `Local` code component in `Edit` while selected.

## Optional Features

The sections below are ordered by priority.

Implement them only if installed scripts depend on them.

### Priority 1: Entity Event Forwarding

Add this first if existing scripts use:

- `entity.on("click", ...)`
- `entity.on("trigger-enter", ...)`
- `entity.on("trigger-exit", ...)`
- collision-style entity events

Required Unreal work:

- detect the relevant engine/gameplay event
- map it to the CSP entity id
- call `NgxScriptSystem::FireEntityEvent(EntityId, EventName, PayloadJson)`

If you skip this:

- scripts still run
- entity event listeners never fire

### Priority 2: Keyboard And Mouse Input

Add this if scripts use `@csp/code` input APIs:

- `keyboard.on(...)`
- `mouse.on(...)`
- `keyboard.isPressed(...)`
- `mouse.isPressed(...)`

Required Unreal work:

- translate Unreal input events into the normalized CSP event shape
- call:
  - `NgxScriptSystem::FireKeyboardEvent(...)`
  - `NgxScriptSystem::FireMouseEvent(...)`

If you skip this:

- scripts still run
- input callbacks and pressed-state queries will not work

### Priority 3: Camera State Updates

Add this if scripts use `ThePlayerController` camera helpers, such as:

- `getCameraPosition()`
- `getCameraForward()`
- `getCameraRight()`
- `getCameraUp()`

Required Unreal work:

- gather the active local camera transform and basis vectors
- call `NgxScriptSystem::SetLocalPlayerCameraState(...)`

If you skip this:

- scripts still run
- camera-query helpers return fallback/default values

### Priority 4: Scripted UI Rendering And Clicks

Add this if installed scripts export `ui(...)`.

Required Unreal work:

- call `NgxScriptSystem::SetUIViewportSize(...)` when viewport size changes
- poll `NgxScriptSystem::DrainPendingUIUpdates()`
- create/update/remove local UI render objects from the returned diff stream
- hit test clickable drawables
- call `NgxScriptSystem::DispatchUIAction(EntityId, HandlerId)` on click

If you skip this:

- script logic still runs
- scripted UI will not be visible or interactive

### Priority 5: World-Space UI

Add this only if scripts use `world(...)` UI surfaces.

Required Unreal work:

- group drawables by script entity
- resolve `targetEntityId` to a live transform
- place the world UI panel using `worldOffset`
- apply billboard behavior if requested

If you skip this:

- screen-space UI may still work
- world-space scripted UI will not render correctly

## What Is Not Required For This Migration

You do not need to build any of the following just to run existing scripts:

- code-component authoring tools
- schema editing UI
- editor inspectors for script attributes
- script upload tooling
- custom JS module authoring workflow in Unreal

## Recommended First Test Pass

Use three simple validation scripts:

1. An `Editor` component that logs once when active in `Edit`
2. A `Local` component that logs once when active in `Play`
3. A `Local` component that logs once when selected in `Edit`

After that, add one test for each optional feature you choose to support:

- entity click event
- keyboard input
- camera query
- button UI click

## Suggested Delivery Order

For the safest Unreal rollout:

1. Animation-frame ticking
2. Edit/Play mode switching
3. Selection sync
4. Entity event forwarding
5. Keyboard and mouse input
6. Camera state updates
7. Screen-space scripted UI
8. World-space scripted UI

This gives you the best compatibility with the smallest amount of host-side work.
