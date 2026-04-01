# NGX UI Client Integration

This document describes how an engine/client implementation inside this codebase should integrate with the current NGX scripted UI runtime.

It is aimed at native or web client implementers working in the engine layer, not script authors. Script authors should read `ngx_scripting.md`.

## Summary

The scripted UI system is split into two halves:

- The shared library owns script execution, reactive updates, UI-tree normalization, layout, and diffing.
- The client owns rendering, hit testing, and forwarding click actions back into the script runtime.

The client does not perform layout. It consumes flattened drawables that already contain resolved bounds.

## Current Ownership Model

The current runtime flow is:

1. A `CodeSpaceComponent` script exports `ui({ attributes, thisEntity, entityId })`.
2. `NgxCodeComponentRuntime` evaluates that `ui()` function reactively.
3. The normalized UI tree is mounted into `NgxUIRuntime`.
4. `NgxUIRuntime` runs layout and produces flattened drawables.
5. The client drains add/update/remove operations and mirrors them into its own render state.
6. When the user clicks a button, the client dispatches the associated `handlerId` back into `NgxScriptSystem`.

The important consequence is:

- Script state stays in the script runtime.
- Layout stays in the shared library.
- The client should be a renderer plus interaction bridge.

## Client Entry Points

The client integration points on `NgxScriptSystem` are part of the public API surface.

The relevant hooks are:

- `NgxScriptSystem::SetUIViewportSize(width, height)`
  - Call when the viewport size changes.
  - This causes screen-space UI to be re-laid out.
- `NgxScriptSystem::DrainPendingUIUpdates()`
  - Poll this after script/runtime work has been processed.
  - Returns a JSON array of `add`, `update`, and `remove` operations.
- `NgxScriptSystem::DispatchUIAction(entityId, handlerId)`
  - Call when the client determines that a clickable drawable was activated.
  - Returns `true` if the handler existed and ran.

There are also host bindings such as `csp.__uiMount(...)` and `csp.__uiUnmount(...)`, but those are internal to the script runtime and are not the client integration surface.

## Update Contract

`DrainPendingUIUpdates()` returns a JSON array with this shape:

```json
[
  {
    "op": "add",
    "entityId": "12345",
    "drawable": {
      "id": "root.0.1",
      "type": "button",
      "surface": "screen",
      "targetEntityId": "",
      "surfaceWidth": 1280,
      "surfaceHeight": 720,
      "worldOffset": { "x": 0, "y": 0, "z": 0 },
      "billboardMode": "",
      "x": 920,
      "y": 640,
      "width": 120,
      "height": 48,
      "backgroundColor": { "r": 0.145, "g": 0.388, "b": 0.922, "a": 1.0 },
      "textColor": { "r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0 },
      "cornerRadius": 0,
      "opacity": 1.0,
      "text": "Next",
      "fontSize": 16,
      "assetCollectionId": "",
      "imageAssetId": "",
      "handlerId": "root.0.1:click",
      "enabled": true
    }
  }
]
```

The client should treat these updates as authoritative.

Recommended behavior:

- `add`
  - Create a local render object and optional hit-test object.
- `update`
  - Update the existing local object in place.
- `remove`
  - Destroy the local object and remove any hit-test registration.

## Drawable Meanings

Important fields on each drawable:

- `id`
  - Stable drawable id within an entity's mounted UI.
- `type`
  - Current values are `rectangle`, `text`, `image`, and `button`.
- `surface`
  - `screen` or `world`.
- `targetEntityId`
  - Used for `world` surfaces. This is the entity the panel is anchored to.
- `surfaceWidth`, `surfaceHeight`
  - Logical layout surface size used when the drawable was laid out.
- `worldOffset`
  - Local offset from the world anchor entity for world UI.
- `billboardMode`
  - Optional billboard hint for world UI.
- `x`, `y`, `width`, `height`
  - Resolved logical bounds.
- `backgroundColor`, `textColor`, `cornerRadius`, `opacity`
  - Styling values already resolved by the library.
  - Color channels are normalized floats in the range `0..1`.
- `text`, `fontSize`
  - Text payload for text and button drawables.
  - Wrapped or multi-line text currently appears as one `text` drawable per rendered line.
- `assetCollectionId`, `imageAssetId`
  - Image identity for image drawables.
- `handlerId`
  - Present on interactive drawables such as buttons.
- `enabled`
  - Disabled drawables should not dispatch interaction.

## Screen-Space Rendering

For `surface == "screen"`:

- Interpret `x`, `y`, `width`, and `height` in viewport logical pixels.
- Render in your normal UI overlay pass.
- Use `surfaceWidth` and `surfaceHeight` as the layout reference size.
- Re-run layout by calling `SetUIViewportSize(...)` whenever the viewport changes.

Recommended screen-space approach:

- Maintain a per-entity UI registry keyed by `entityId`.
- Within each entity, maintain a drawable registry keyed by `drawable.id`.
- Render all visible screen drawables after 3D scene rendering or in your normal HUD pass.

## World-Space Rendering

For `surface == "world"`:

- Treat `x`, `y`, `width`, and `height` as panel-local logical coordinates.
- Resolve `targetEntityId` to a live world transform.
- Offset the panel origin by `worldOffset`.
- Apply billboard behavior if requested by `billboardMode`.
- Render the drawables onto a world-space panel or equivalent UI surface.

Recommended world-space approach:

1. Group world drawables by `entityId`.
2. Resolve the anchor transform from `targetEntityId`.
3. Create or update a world panel for that UI group.
4. Render the group's drawables into that panel using the same logical coordinates produced by the runtime.

The shared library is already doing layout. The client's job is only to place the final panel in 3D and render the flattened content.

## Interaction

The current interaction model is click-only.

Recommended flow:

1. Build hit-test entries only for drawables with a non-empty `handlerId` and `enabled == true`.
2. On pointer or controller activation, hit test against the appropriate surface.
3. If a hit succeeds, call `DispatchUIAction(entityId, handlerId)`.

Screen interaction:

- Hit test in viewport coordinates against screen drawables.

World interaction:

- Raycast or otherwise resolve pointer/controller focus against the world panel.
- Convert the hit point into panel-local coordinates.
- Hit test against the world drawables within that panel.

Current limitations:

- No hover callbacks.
- No keyboard focus model.
- No text input.
- No drag or scroll event dispatch.

## Text Measurement

Text layout currently falls back to a runtime heuristic unless the client supplies a text measurement callback through the NGX script system.

When a callback is provided:

- Clay asks the shared library to measure text during layout.
- The shared library forwards that request to the client callback.
- In wasm builds, the callback path should be treated as main-thread-only work. The runtime proxies the invocation back to the main runtime thread before calling the client callback.

That heuristic estimates:

- width as roughly `characterCount * fontSize * 0.6`
- height as roughly `fontSize * 1.2`

This is good enough for bring-up and simple HUD text, but it is not accurate enough for polished wrapped or tightly fitted UI. A real client-provided measurement hook can be reintroduced once the callback path is stable again.

Current styling notes:

- Script-side colors should currently be provided as hex strings such as `"#FFFFFF"` or `"#102713CC"`.
- The runtime accepts `textColor`, `color`, and `colour` for text color props.
- If no explicit color is provided, `text` drawables default to opaque white and `button` drawables default to opaque white text on a dark background.

## Recommended Client Architecture

The smallest maintainable client structure is:

- `NgxUIClientBridge`
  - Polls `DrainPendingUIUpdates()`
  - Applies diffs into local render state
  - Forwards clicks through `DispatchUIAction(...)`
- `ScreenUIRenderer`
  - Renders `surface == "screen"` drawables
- `WorldUIRenderer`
  - Renders `surface == "world"` drawables and owns world-panel transforms
- `UIHitTestRegistry`
  - Tracks interactive drawables by `entityId` and `drawable.id`

Recommended stored state per mounted UI:

- `entityId`
- `surface`
- local map of `drawable.id -> drawable`
- resolved render resources
- resolved hit-test data
- for world UI, resolved anchor reference and panel transform

## Lifecycle Notes

The client should expect UI to disappear when:

- the owning code component is removed
- the code component becomes inactive
- the JS context rebuilds
- the script returns `null` from `ui()`

In all of these cases, the library will emit `remove` operations. The client should not try to infer lifecycle independently.

## Suggested Main-Loop Integration

One practical order of operations per frame is:

1. Update viewport size if needed.
2. Tick script/runtime systems.
3. Drain pending UI updates.
4. Apply add/update/remove operations to local UI state.
5. Render screen and world UI.
6. Collect UI input and dispatch click actions back into `NgxScriptSystem`.

## Current Gaps To Be Aware Of

The current payload is enough to bring up rendering and click handling, but there are still a few gaps worth knowing about:

- Explicit draw order / z order is not yet exposed in the drawable payload.
- The image asset resolution path is still client-specific and should be documented alongside the renderer implementation.
- There is no richer event surface yet beyond click dispatch.

If overlapping production UI becomes common, adding explicit draw-order metadata would be a strong next step.
