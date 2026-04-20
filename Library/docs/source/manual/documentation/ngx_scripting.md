# NGX Scripting

This page documents the current NGX code-component scripting path used by `NgxScriptSystem`, `NgxCodeComponentRuntime`, and `NgxEntityScriptBinding`.

It is aimed at writing JavaScript modules for `CodeSpaceComponent` assets, not the older legacy `ScriptSpaceComponent` flow.

## What An NGX Script Module Looks Like

An NGX script module typically exports two things:

- `schema`: the editable attributes for the code component
- `script`: the function the runtime executes for the entity

Use this shape:

```js
import { useEffect } from "@csp/hooks";

export const schema = {
  speed: { type: "float", default: 1.0, min: 0.0, max: 10.0 },
  enabled: { type: "boolean", default: true },
};

export function script({ attributes, thisEntity, entityId }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity || !attributes.enabled.value) {
      return;
    }

    console.log(`NGX script running for entity ${entityId}: ${entity.name}`);
  });
}
```

## Current Runtime Contract

The current runtime calls your module like this:

```js
script({ attributes, thisEntity, entityId })
```

That means:

- `attributes` is an object of signals
- `thisEntity` is also a signal
- `entityId` is the current entity id

Prefer `export const schema` plus `export function script(...)`.

Your `script(...)` function may also return a teardown function. That teardown runs when the code component is replaced or removed.

Notes:

- `attributes`, `codeComponentSchema`, and `schema` are all accepted as schema exports today, but `schema` is the clearest choice.
- Older patterns such as `scriptFactory()` may still exist in sample projects, but they are not the clearest representation of the current runtime.

## Available Imports

The NGX runtime currently exposes these built-in modules:

- `@csp/hooks`
  - `useEffect`
- `@csp/code`
  - `useEffect`
  - `TheEntitySystem`
  - `keyboard`
  - `mouse`
  - `ThePlayerController`
  - `EAssetType`
  - `TheAssetSystem`
- `@csp/ui`
  - `screen`
  - `world`
  - `row`
  - `column`
  - `flowRow`
  - `floating`
  - `spacer`
  - `text`
  - `image`
  - `button`

## Scripted UI With `ui()`

An NGX module may optionally export a `ui(...)` function alongside `script(...)`.

The runtime calls it like this:

```js
ui({ attributes, thisEntity, entityId })
```

`ui(...)` returns a UI tree built with `@csp/ui`. The runtime evaluates that tree reactively, runs layout in the shared library, and publishes flattened drawables for the client renderer.

Use `screen(...)` for screen-space UI and `world(...)` for in-space UI anchored to an entity.

Example:

```js
import { screen, row, column, button, image } from "@csp/ui";

export function ui() {
  return screen(
    { width: 900, height: 420, alignX: "center", alignY: "center", padding: 20 },
    row(
      { width: "grow", height: "grow", gap: 20 },
      column(
        { width: 220, gap: 12 },
        button("Alpha", { key: "alpha" }),
        button("Beta", { key: "beta" }),
      ),
      image(
        { assetCollectionId: "museum-assets", imageAssetId: "hero" },
        { key: "heroImage", width: "grow", height: "grow" }
      )
    )
  );
}
```

Wrapping button list example:

```js
import { flowRow, button } from "@csp/ui";

const cameras = [
  "CAM_1_Lift",
  "CAM_2_Lift",
  "CAM_3_Dock",
  "CAM_4_Towe",
  "CAM_5_Land",
  "CAM_6_JIB",
  "CAM_7_Dron",
];

function cameraButton(label) {
  return button(label, {
    width: 215,
    height: 54,
    backgroundColor: "#111111",
    textColor: "#F2F2F2",
  });
}

export function ui() {
  return flowRow(
    {
      width: 910,
      columnGap: 16,
      rowGap: 14,
    },
    ...cameras.map(cameraButton)
  );
}
```

Current UI notes:

- `button` supports `onClick`
- `screen` supports viewport anchoring through root `alignX` and `alignY`
- `world` supports `targetEntityId`, `worldOffset`, and `billboardMode`
- `flowRow` lays out children left-to-right and starts a new row when the next child would overflow
- `floating` is the way to overlap or absolutely anchor UI relative to a parent or the root
- `width` and `height` support fixed numbers, the keywords `"grow"` and `"fit"`, plus constrained objects such as `{ mode: "grow", max: 920 }` or `{ mode: "fit", min: 120, max: 320 }`
- `aspectRatio` may be set on layout or image nodes to preserve a width-to-height ratio while Clay sizes the element
- colors are currently passed as hex strings such as `"#FFFFFF"` or `"#102713CC"`
- text color props may be written as `textColor`, `color`, or `colour`
- text alignment may be set with `textAlign: "left" | "center" | "right"`
- if no explicit color is provided, `text(...)` defaults to opaque white text and `button(...)` defaults to opaque white text on a dark background

`floating(...)` is preferred over a generic "stack" abstraction because it expresses overlap and anchoring explicitly.

Example text styling:

```js
text("Coins: 3", {
  color: "#00FFFF",
  fontSize: 16,
  textAlign: "left",
})
```

Responsive image example:

```js
image(
  { assetCollectionId: "museum-assets", imageAssetId: "hero" },
  {
    width: { mode: "grow", max: 920 },
    height: "grow",
    aspectRatio: 920 / 620
  }
)
```

Example:

```js
import { screen, image, floating, button } from "@csp/ui";

export function ui() {
  return screen(
    { width: 640, height: 360, alignX: "center", alignY: "center" },
    image(
      { assetCollectionId: "museum-assets", imageAssetId: "hero" },
      { width: "grow", height: "grow" }
    ),
    floating(
      {
        attachTo: "parent",
        attachX: "right",
        attachY: "bottom",
        offset: { x: -16, y: -16 },
        zIndex: 10
      },
      button("Next")
    )
  );
}
```

## Signals And `useEffect`

`useEffect` is backed by reactive signals. Anything you read with `.value` inside an effect becomes a dependency.

Example:

```js
import { useEffect } from "@csp/hooks";

export const schema = {
  spinSpeed: { type: "float", default: 0.75 },
};

export function script({ attributes, thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    const spinSpeed = attributes.spinSpeed.value;

    if (!entity) {
      return;
    }

    console.log(`Entity ${entity.name} spin speed is ${spinSpeed}`);
  });
}
```

If an effect returns a function, that function is treated as cleanup. Cleanup runs before the effect is replaced and again when the code component is removed.

## `thisEntity`

`thisEntity` is a signal whose `.value` is the live bound entity or `null`.

Use it like this:

```js
const entity = thisEntity.value;
if (!entity) {
  return;
}

entity.position = [0, 1, 0];
```

The entity object exposes the familiar legacy entity API, including:

- transform properties such as `position`, `rotation`, and `scale`
- component getters such as `getStaticModelComponents()`
- component creation helpers such as `addStaticModelComponent()`
- event helpers such as `on(...)`, `off(...)`, and `fire(...)`

`entity.on(...)` listens to NGX entity events delivered through the NGX event bridge. Click, trigger, and collision events are wired today.

## Schema Types

The current NGX schema reader supports these normalized attribute types:

- `boolean`
- `integer`
- `float`
- `string`
- `entity`
- `modelAsset`

Example schema:

```js
export const schema = {
  enabled: { type: "boolean", default: true },
  count: { type: "integer", default: 3, min: 0, max: 10 },
  speed: { type: "float", default: 1.5, min: 0, max: 5, step: 0.1 },
  label: { type: "string", default: "Hello" },
  target: { type: "entity" },
  mesh: {
    type: "modelAsset",
    default: { assetCollectionId: "csp-default-assets", assetId: "cube" },
  },
};
```

### Entity Attributes

Entity attributes are resolved to entity references for you.

```js
export const schema = {
  target: { type: "entity" },
};

export function script({ attributes }) {
  useEffect(() => {
    const target = attributes.target.value;
    if (target) {
      console.log(`Target entity: ${target.name}`);
    }
  });
}
```

## Example: Animate The Host Entity

```js
import { useEffect } from "@csp/hooks";

export const schema = {
  speed: { type: "float", default: 1.0, min: 0.0, max: 10.0 },
  amplitude: { type: "float", default: 0.25, min: 0.0, max: 3.0 },
};

export function script({ attributes, thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity) {
      return;
    }

    let rafId = 0;
    const startPosition = [...entity.position];

    const tick = (timeMs = 0) => {
      const offset = Math.sin((timeMs / 1000) * attributes.speed.value) * attributes.amplitude.value;
      entity.position = [startPosition[0], startPosition[1] + offset, startPosition[2]];
      rafId = requestAnimationFrame(tick);
    };

    rafId = requestAnimationFrame(tick);
    return () => {
      cancelAnimationFrame(rafId);
      entity.position = startPosition;
    };
  });
}
```

## Example: Add And Clean Up A Runtime Component

This is the pattern to use when you want a script-created component to go away cleanly.

```js
import { useEffect } from "@csp/hooks";

export function script({ thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity) {
      return;
    }

    const mesh = entity.addStaticModelComponent();
    mesh.assetCollectionId = "csp-default-assets";
    mesh.modelAssetId = "cube";
    mesh.isVisible = true;
    mesh.position = [0, 1, 0];

    return () => {
      mesh.destroy();
    };
  });
}
```

Notes:

- `destroy()` is available on the script-side component base interface.
- This is the right pattern for `useEffect` cleanup.

## Example: Respond To Clicks

```js
import { useEffect } from "@csp/hooks";

export function script({ thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity) {
      return;
    }

    const onClick = (eventData) => {
      console.log(`Clicked ${entity.name}`, eventData);
    };

    entity.on("click", onClick);
    return () => entity.off("click", onClick);
  });
}
```

For component clicks, the event payload currently follows the legacy shape:

```js
{ id, cid }
```

Where:

- `id` is the clicked entity id
- `cid` is the clicked component id

## Example: Respond To Trigger Events

```js
import { useEffect } from "@csp/hooks";

export function script({ thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity) {
      return;
    }

    const onTriggerEnter = (eventData) => {
      console.log(`Trigger enter on ${entity.name}`, eventData);
    };

    entity.on("trigger-enter", onTriggerEnter);
    return () => entity.off("trigger-enter", onTriggerEnter);
  });
}
```

## Example: Keyboard And Mouse Input

```js
import { useEffect } from "@csp/hooks";
import { keyboard, mouse } from "@csp/code";

export function script() {
  useEffect(() => {
    const onKeyDown = (event) => console.log(`Key down: ${event.key}`);
    const onMouseDown = (event) => console.log(`Mouse button: ${event.button}`);

    keyboard.on("keydown", onKeyDown);
    mouse.on("mousedown", onMouseDown);

    return () => {
      keyboard.off("keydown", onKeyDown);
      mouse.off("mousedown", onMouseDown);
    };
  });
}
```

## Example: Player Controller Access

```js
import { useEffect } from "@csp/hooks";
import { ThePlayerController } from "@csp/code";

export function script() {
  useEffect(() => {
    ThePlayerController.teleportCharacter(0, 1, 0);
  });
}
```

`ThePlayerController` currently exposes helpers such as:

- `moveCharacter(x, y, z, jump, isFlying)`
- `teleportCharacter(x, y, z)`
- `setFirstPersonEnabled(enabled)`
- camera query helpers such as `getCameraPosition()` and `getCameraForward()`
- `isVrActive()` returns `true` when the local client is currently in a WebXR session (useful for choosing VR-specific layouts or deciding whether to attach a UI to a controller). Returns `false` outside VR.

## Visual Attachment To Anchors

An entity can be visually parented to a named anchor point (an XR controller, for example) without changing the CSP scene hierarchy. Add an `AttachmentSpaceComponent` and set `anchorPath`:

```js
import { useEffect } from "@csp/hooks";
import { ThePlayerController } from "@csp/code";

export function script({ thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity) {
      return;
    }

    const attachment = entity.addAttachmentComponent();
    attachment.anchorPath = ThePlayerController.isVrActive() ? "/xr/left-hand" : "";

    return () => attachment.destroy();
  });
}
```

When the path resolves, the entity's `SpaceTransform` is interpreted as **local** to the anchor — `(0, 0, 0)` sits at the anchor root. World-mounted NGX UIs hosted by the entity follow automatically via the renderer scene graph.

Supported paths (MVP):

- `/xr/left-hand` — the local client's left controller grip node
- `/xr/right-hand` — the local client's right controller grip node

Path resolution is local: each observer resolves `/xr/*` against their own controllers, so a UI attached on one client's hand is not visible on another client's hand. If the path does not resolve (XR not active, controller missing, unknown scheme, or empty string), the entity falls back to rendering at its `SpaceTransform` in world space — so non-VR users see the UI as an ordinary world panel.

## Accessing Other Entities

Use `TheEntitySystem` when you need to look up other entities.

```js
import { useEffect } from "@csp/hooks";
import { TheEntitySystem } from "@csp/code";

export const schema = {
  targetName: { type: "string", default: "Target" },
};

export function script({ attributes }) {
  useEffect(() => {
    const target = TheEntitySystem.getEntityByName(attributes.targetName.value);
    if (target) {
      console.log(`Found target ${target.id}`);
    }
  });
}
```

## Materials On Model Components

The NGX compatibility layer adds async material helpers for static and animated model components:

- `await model.getMaterial(path)`
- `await model.getMaterials()`

Example:

```js
import { useEffect } from "@csp/hooks";

export function script({ thisEntity }) {
  useEffect(() => {
    const entity = thisEntity.value;
    if (!entity) {
      return;
    }

    const model = entity.getStaticModelComponents()[0];
    if (!model) {
      return;
    }

    let cancelled = false;

    (async () => {
      const material = await model.getMaterial("/nodes/0");
      if (!cancelled && material) {
        material.emissiveStrength = 2.0;
      }
    })();

    return () => {
      cancelled = true;
    };
  });
}
```

## Practical Rules

- Read signals with `.value`.
- Guard `thisEntity.value` before using it.
- Prefer `schema` over older alias names.
- Prefer `export function script({ ... })` over older factory-style examples.
- Use `useEffect` for subscriptions, timers, animation loops, and cleanup.
- If you create runtime components, clean them up with `component.destroy()`.
- If you attach browser or input listeners, always remove them in cleanup.

## Current Runtime Notes

- The NGX code-component runtime is client-side.
- `Local` and `Editor` scopes are handled in the current client runtime path.
- `Server` scope is defined at the component level but is not executed by this runtime path today.

## Related Files

If you need to understand the implementation behind this API, the main entry points are:

- `Library/src/Multiplayer/NgxScript/NgxScriptSystem.cpp`
- `Library/src/Multiplayer/NgxScript/NgxCodeComponentRuntime.cpp`
- `Library/src/Multiplayer/NgxScript/NgxEntityScriptBinding.cpp`
- `Library/src/Multiplayer/Script/EntityScriptBinding.cpp`
