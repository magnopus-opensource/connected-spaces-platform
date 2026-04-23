# Gotchas

Running list of non-obvious landmines and existing bugs in the foundation. Keep entries short and action-oriented: what the trap is, how it manifests, why, and what to do about it. Order by "most recently learned" at the top.

---

## `SpaceEntity::SelectedId` used to be replicated & persisted

**Symptom.** Code Components on entities that "nothing is selected" (per the editor UI) were running in Edit mode anyway. `IsEntityOrAncestorSelectedByLocalClient(entity)` returned `true` for entities the user had never explicitly selected this session. Selecting/deselecting the entity in the editor appeared to do nothing; sometimes after ~3 toggles the state finally cleared.

**Why.** `SelectedClientId` was a replicated property on `SpaceEntity`. Every call to `Select()` dirtied the property via the state patcher; the server persisted it as part of the entity. A past session's selection (or an editor UI flow that forgot to call `Deselect()`) could leave `SelectedId == <some client id>` latched on the server. When the entity loaded in a new session, `IsSelected()` returned `true` even though the local UI showed nothing selected.

**Fix applied.** `SelectedClientId` is no longer bound in `SpaceEntity::CreateReplicatedProperties()`. The two `StatePatcher->SetDirtyProperty(SelectedClientId, ...)` calls in `InternalSetSelectionStateOfEntity` are gone. Legacy persisted values are silently ignored in `SpaceEntityStatePatcher::NewFromObjectMessage`. Selection is now purely local per-client.

**Scope semantics.** With selection now local-only, `Local` scope runs in `Play`, and also in `Edit` when the entity (or an ancestor) is selected by *this* client — the historical "preview while editing" workflow is preserved. `Editor` scope runs whenever runtime mode is `Edit` (for scene-authoring helpers). `Server` runs only in `Server`. The ghost-activation risk is gone because `SelectedId` can no longer be latched server-side.

**Watch for.** Any other replicated property you suspect "should just be local" — check `CreateReplicatedProperties()` and grep for `SetDirtyProperty(SpaceEntityComponentKey::...)`. If nothing cross-client actually consumes the state, a local-only member is almost always the better design.

---

## `requestAnimationFrame` callbacks outlive their owning code component

**Symptom.** A code component with an animation loop (e.g. `script()` body calls `requestAnimationFrame(tick)` and the tick re-registers itself) kept firing side effects (entity position updates, UI mounts) after the component was removed — even after `removeCodeComponent` disposed all signal effects and called the user's `teardown`.

**Why.** The built-in `requestAnimationFrame` in `NgxScriptSystem`'s `HOST_BINDINGS_SCRIPT` stored callbacks in a single global `Map<id, callback>`. Nothing associated a callback with its owning entity, so teardown couldn't cancel them. Once a rAF chain was self-scheduling, it outlived the entity.

**Fix applied.** `requestAnimationFrame` now captures `globalThis.__cspCurrentEntityId` at registration time and stores `{callback, entityId}` entries. `__cspDispatchAnimationFrames` filters out entries whose entity is no longer in `scriptRegistry` via `scriptRegistry.isEntityRegistered(entityId)`. rAFs registered without an entity context (`entityId === null`) always run.

**Watch for.** Other global registries in the host bindings (`setTimeout` shims, event bus subscriptions, etc.) that don't scope by `__cspCurrentEntityId`. Same pattern applies: capture the entity on register, filter on dispatch.

---

## Direct `effect` imports from `@preact/signals-core` leak across code component teardown

**Status:** Known pitfall. Not fixed. Worth calling out.

**Symptom.** A script that imports `effect` (or `computed`) directly from `@preact/signals-core` keeps reacting to signal changes after the owning code component is removed.

**Why.** The registry tracks disposers in `entry.effects[]` only when `useEffect` from `@csp/hooks` is used. Direct `effect()` calls from `@preact/signals-core` return a disposer to the caller but are never registered with the registry — so `disposeAllEffects` can't clean them up, and the user's `teardown` function has to manually dispose them (most scripts don't).

**Mitigation for script authors.** Always import `useEffect` from `@csp/hooks` (or `@csp/code` / `@csp/ui`, which re-export it). Never import `effect` directly from `@preact/signals-core`.

**Possible future fix.** Wrap the `@preact/signals-core` module source so `effect`/`computed` auto-track into the current entity's `effects[]`. The scripts that need raw signals primitives (`signal`, `batch`) would still work.

---

## Full-entity serialization includes **all** registered properties, not just dirty ones

**Gotcha.** `SpaceEntityStatePatcher::CreateObjectMessage` (used on first-time send) iterates `RegisteredProperties` and calls `Get()` on every one of them. `CreateObjectPatch` (used for incremental updates) iterates `DirtyProperties`. So if a property is bound in `CreateReplicatedProperties()` but you never call `SetDirtyProperty`, it will *still* be serialized once (the initial message) with whatever the getter returns.

**Consequence.** Removing `SetDirtyProperty` calls alone is not enough to make a property local — the binding itself has to be dropped. (This is exactly what we did for `SelectedClientId`.)

**When adding a new property.** Decide up-front: replicated/persisted → bind it + dirty it on change. Local-only → don't bind it, use `SetPropertyDirect` for local change notifications.

---

## `thisEntity.on('select'/'deselect')` only fires for scripts that stay active across selection

**Gotcha.** Scripts subscribe to selection via `thisEntity.on('select', cb)` / `thisEntity.on('deselect', cb)`. The event fires from `NgxCodeComponentRuntime::SyncSnapshots` when `IsLocallySelected` transitions between two consecutive snapshots *while the entity is in both snapshots* — i.e. the script has to stay active across the transition.

**Consequence for `Local` scope in Edit.** A `Local` script activates *because* the entity got selected and deactivates *because* it got deselected. The entity isn't in both snapshots across the transition, so no `select`/`deselect` event fires. Scripts should treat `script()` execution itself as the "selected" moment and the returned teardown callback as the "deselected" moment — these run at exactly the right times.

**When the events actually fire.** `Editor`-scope scripts are always active in Edit mode regardless of selection, so selection toggles land inside both snapshots and the events fire. Same for any other case where the script stays active while the selection flag flips.

**Initial state.** Even for the scripts that do receive transitions, no event fires for the *initial* selected state at activation time — the listener inside `script()` was registered after the first snapshot. If you need initial state, query it directly (expose `thisEntity.value.isSelected` if missing) or deselect + reselect to cue the transition.

---

## V8's WASM stack frame budget is independent of Emscripten's `STACK_SIZE`

**Symptom.** Loading a scene with ~5 code components with Chrome DevTools open crashes with `RangeError: Maximum call stack size exceeded` inside `FlushPendingCodeComponentUI` → `scriptRegistry.tick(0)` → `JS_CallInternal` chain. With DevTools closed it runs fine.

**Why.** Two separate stack budgets apply to WASM:
1. **Linear-memory stack** — set by Emscripten's `-sSTACK_SIZE` (we use 32 MB). This is the memory region WASM uses for C++ function locals.
2. **V8's WASM frame budget** — a V8-enforced limit on the number of WASM call frames active at once, independent of the linear-memory stack. Much tighter than 32 MB would suggest.

DevTools adds per-call instrumentation overhead (stack capture, profiler hooks) that effectively shrinks the V8 budget. Code that fits fine with DevTools closed overflows with it open.

**QuickJS's `JS_SetMaxStackSize` does NOT help** — its check is based on linear-memory stack pointer comparison and can't detect V8's frame-count limit.

**Fix applied.** `scriptRegistry.tick(0)` now activates at most one code component module per foundation tick (see `NgxCodeComponentRuntime.cpp` `function tick()`). Each activation runs `script()` + the initial `ui()` effect + their signal/effect chain synchronously in the tick call stack, so processing N entities in one tick stacks N deep chains. Activations now queue across ticks — startup is visibly a few frames longer but the per-tick V8 stack usage stays bounded.

**Watch for.** Any other code path that fans out to multiple user-script executions in a single synchronous call. Similar patterns in `SyncSnapshots` / `FlushPendingCodeComponentUI` should batch or throttle the same way if they start running user code synchronously.

---

## Entity snapshot diff won't re-commit unless *every* op succeeds

**Gotcha.** `NgxCodeComponentRuntime::SyncSnapshots` only updates `LastEntitySnapshots = CurrentSnapshots` if every add/remove/attribute-sync call returned `true`. On WASM any registry snippet can return `false` if `ContextMutex::try_lock` fails. If that happens, the entire current snapshot is thrown away and retried next tick — even the ops that did succeed.

**Consequence.** Under contention, a single deferred op can cause the whole snapshot to re-run, which re-emits ops for entities that were already processed. The registry-side ops are idempotent so this is safe, but it's confusing when reading logs.

**If you add new ops here.** Make sure they're idempotent (handle "already present" / "already gone" gracefully) and cheap (they'll retry until they stick).

---

## Adding new entries

Keep sections short. Structure per entry:

- **One-line title** describing the gotcha in terms of the thing that bit someone (not the fix).
- **Symptom** — what a caller/dev sees. Include exact error messages, log lines, or behavior.
- **Why** — the underlying cause. Name the specific files/functions.
- **Fix applied / Status** — what was done, or "known, not fixed" with mitigations.
- **Watch for / When adding** — how to avoid hitting the same class of trap elsewhere.

Prefer concrete file names and function names over prose — a reader grep-searching for a symptom should land here.
