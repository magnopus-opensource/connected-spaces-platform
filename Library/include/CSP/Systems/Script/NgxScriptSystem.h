/*
 * Copyright 2026 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace csp::multiplayer
{
class NgxAssetScriptBinding;
} // namespace csp::multiplayer

namespace qjs
{
class Runtime;
class Context;
} // namespace qjs

namespace csp::common
{
class IRealtimeEngine;
class LogSystem;
class NetworkEventData;
} // namespace csp::common

namespace csp::systems
{
class NgxUIRuntime;

class CSP_API NgxScriptSystem
{
public:
    explicit NgxScriptSystem(csp::common::LogSystem& InLogSystem);
    ~NgxScriptSystem();

    // Execute a loaded module path using the in-memory module map.
    bool ExecuteModule(const csp::common::String& ModulePath);

    // Invoke requestAnimationFrame callbacks using a display-timestamp tick.
    // This is intentionally separate from scriptRegistry tick and foundation tick cadence.
    bool TickAnimationFrame(double TimestampMs);

    // Sync schema from scriptRegistry for a Code component entity.
    // Returns JSON stringified attributes object. Falls back to "{}" on missing registry or failure.
    csp::common::String SyncCodeComponentSchema(const csp::common::String& EntityId);

    // Add or replace a Code component definition for an entity.
    // PayloadJson should contain { scriptAssetPath, attributes }.
    bool AddCodeComponent(const csp::common::String& EntityId, const csp::common::String& PayloadJson);

    // Sync all component attributes from scriptRegistry for a Code component entity.
    // Returns JSON stringified attributes object. Falls back to "{}" on missing registry or failure.
    csp::common::String SyncCodeComponentAttributes(const csp::common::String& EntityId, const csp::common::String& AttributesJson);

    // Update one attribute in scriptRegistry for a Code component entity.
    // ValueJson should be a JSON value (e.g. number, string, object).
    bool UpdateAttributeForEntity(const csp::common::String& EntityId, const csp::common::String& Key, const csp::common::String& ValueJson);

    // Remove a Code component from scriptRegistry for an entity.
    bool RemoveCodeComponent(const csp::common::String& EntityId);

    // Drain entity IDs whose schema was resolved after an async module load.
    // Returns a JSON array string e.g. ["123","456"]. Falls back to "[]" on failure.
    csp::common::String DrainPendingSchemaSyncs();

    // Fire a named event on an entity, dispatching to all JS listeners registered via entity.on().
    // EventName should be e.g. "click" or "trigger-enter".
    // PayloadJson should be a JSON object string e.g. "{}" or "{\"isLocalPlayer\":true}".
    void FireEntityEvent(const csp::common::String& EntityId, const csp::common::String& EventName, const csp::common::String& PayloadJson);

    // Dispatch a normalized keyboard event to active Local/Editor scripts.
    bool FireKeyboardEvent(const csp::common::String& EventType, const csp::common::String& Key, const csp::common::String& Code,
        bool Repeat, bool AltKey, bool CtrlKey, bool ShiftKey, bool MetaKey);

    // Dispatch a normalized mouse event to active Local/Editor scripts.
    bool FireMouseEvent(const csp::common::String& EventType, int32_t Button, int32_t Buttons, int32_t PointerId,
        const csp::common::String& PointerType, bool AltKey, bool CtrlKey, bool ShiftKey, bool MetaKey);

    // Cache the local player's active camera transform/orientation vectors for script queries.
    void SetLocalPlayerCameraState(const csp::common::Vector3& Position, const csp::common::Vector4& Rotation,
        const csp::common::Vector3& Forward, const csp::common::Vector3& ForwardFlat, const csp::common::Vector3& Right,
        const csp::common::Vector3& RightFlat, const csp::common::Vector3& Up);

    // Cache whether the local client is currently in an immersive WebXR session, for script queries
    // (e.g. AttachmentSpaceComponent anchor resolution or VR-specific UI layout choices).
    void SetLocalPlayerXrActive(bool bActive);

    // Update the logical viewport size used for screen-space UI layout.
    void SetUIViewportSize(float Width, float Height);

    // Drain pending browser-safe text measurement requests as a JSON array.
    // Each entry has shape: { "text": string, "fontSize": number, "fontWeight": string }.
    csp::common::String DrainPendingUITextMeasureRequests();

    // Submit measured text results as a JSON array.
    // Each entry has shape: { "text": string, "fontSize": number, "fontWeight": string, "width": number, "height": number }.
    bool SubmitUITextMeasureResults(const csp::common::String& ResultsJson);

    // Drain pending add/update/remove operations for mounted UI drawables as a JSON array.
    csp::common::String DrainPendingUIUpdates();

    // Unmount any UI whose entity id is not in the supplied active-id set.
    // Safety net for screen/world UIs left mounted when a code component
    // deactivates without its JS teardown reaching csp.__uiUnmount.
    CSP_NO_EXPORT void UnmountUIForInactiveEntities(const std::unordered_set<std::string>& ActiveEntityIds);

    // Dispatch a click action back into the scripted UI runtime.
    bool DispatchUIAction(const csp::common::String& EntityId, const csp::common::String& HandlerId, const csp::common::String& EventDataJson = "");

    // Toggle Clay's built-in debug overlay (bounding boxes, layout layers) for all active UI surfaces.
    void SetUIDebugModeEnabled(bool bEnabled);

    // Query the current Clay debug overlay state.
    bool IsUIDebugModeEnabled() const;

    CSP_START_IGNORE
    typedef std::function<void(const csp::common::String& Text, float FontSize, const csp::common::String& FontWeight,
        float& OutWidth, float& OutHeight)> UITextMeasureCallback;

    // Provide a client text measurement callback used by Clay during layout.
    // The callback receives text, font size, font weight, and out-parameters for width and height.
    // This path is intended for native clients. Web clients should prefer the async
    // request/result flow via DrainPendingUITextMeasureRequests and SubmitUITextMeasureResults.
    CSP_NO_EXPORT void SetUITextMeasureCallback(UITextMeasureCallback InCallback);

    // Internal runtime hooks.
    CSP_NO_EXPORT bool HasModuleSource(const csp::common::String& ModulePath) const;
    CSP_NO_EXPORT bool EvaluateSnippet(const csp::common::String& ScriptText, const csp::common::String& DebugName);
    CSP_NO_EXPORT void PumpPendingJobs();
    CSP_NO_EXPORT bool AreScriptModulesLoaded() const;
    CSP_NO_EXPORT uint64_t GetContextGeneration() const;
    CSP_NO_EXPORT bool WasLastEvaluationDeferred() const;
    CSP_NO_EXPORT bool FlushPendingCodeComponentUI();

    // Managed by SpaceSystem during space lifecycle.
    CSP_NO_EXPORT void OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine);
    CSP_NO_EXPORT void OnExitSpace();

    // Manual single-module refresh path.
    CSP_NO_EXPORT void ReloadScriptModule(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

    // Register persistent module source that can be resolved by module path.
    // These sources are independent of per-space asset module loading.
    CSP_NO_EXPORT void RegisterStaticModuleSource(const csp::common::String& ModulePath, const csp::common::String& ModuleSource);
    CSP_NO_EXPORT void UnregisterStaticModuleSource(const csp::common::String& ModulePath);

#ifdef CSP_TESTS
    CSP_NO_EXPORT bool HasActiveContextForTesting() const;
    CSP_NO_EXPORT bool EvaluateModuleScriptForTesting(const csp::common::String& ScriptText);
    CSP_NO_EXPORT int32_t GetGlobalIntForTesting(const csp::common::String& Key, int32_t DefaultValue = 0) const;
    CSP_NO_EXPORT void SetLoadedModuleSourceForTesting(const csp::common::String& ModulePath, const csp::common::String& ModuleSource);
    CSP_NO_EXPORT bool RunModuleForTesting(const csp::common::String& ModulePath);
    CSP_NO_EXPORT csp::common::String GetUIDrawablesJsonForTesting(const csp::common::String& EntityId) const;
#endif

private:
    using AssetCollectionMap = csp::common::Map<csp::common::String, csp::systems::AssetCollection>;
    using ModuleSourceMap = std::unordered_map<std::string, std::string>;

    class NgxScriptTickEventHandler;

    void RebuildContext();
    void TeardownContext();
    void InstallModuleLoader();
    void InstallHostBindings();
    void DrainPendingJobs();
    void ClearAllEntityEventListeners();

    void LoadScriptModules();
    void RegisterAssetDetailBlobChangedListener();
    void UnregisterAssetDetailBlobChangedListener();
    void OnAssetDetailBlobChanged(const csp::common::NetworkEventData& NetworkEventData);
    bool IsTrackedScriptAssetCollection(const csp::common::String& AssetCollectionId) const;

    void FetchAssetCollectionMapForSpace(uint64_t Generation, std::function<void(std::shared_ptr<AssetCollectionMap>)> Callback) const;

    bool IsGenerationCurrent(uint64_t Generation) const;
    bool EvaluateGlobalScript(const std::string& ScriptText, const char* DebugName);
    bool EvaluateModuleScript(const std::string& ScriptText, const char* DebugName);

    csp::common::Vector3 GetLocalPlayerCameraPosition() const;
    csp::common::Vector4 GetLocalPlayerCameraRotation() const;
    csp::common::Vector3 GetLocalPlayerCameraForward() const;
    csp::common::Vector3 GetLocalPlayerCameraForwardFlat() const;
    csp::common::Vector3 GetLocalPlayerCameraRight() const;
    csp::common::Vector3 GetLocalPlayerCameraRightFlat() const;
    csp::common::Vector3 GetLocalPlayerCameraUp() const;
    bool GetLocalPlayerXrActive() const;

    csp::common::LogSystem& LogSystem;
    csp::common::IRealtimeEngine* ActiveRealtimeEngine;
    csp::common::String ActiveSpaceId;

    std::unique_ptr<qjs::Runtime> Runtime;
    std::unique_ptr<qjs::Context> Context;

    mutable std::mutex ContextMutex;
    mutable std::mutex ModuleSourcesMutex;
    mutable std::mutex CameraStateMutex;
    mutable std::mutex UIMutex;
    ModuleSourceMap LoadedModuleSources;
    ModuleSourceMap StaticModuleSources;
    std::unordered_set<std::string> TrackedScriptAssetCollectionIds;

    std::atomic<uint64_t> SessionGeneration;
    std::atomic<uint64_t> ContextGeneration;
    std::atomic<bool> ScriptModulesLoaded;
    std::atomic<bool> LastEvaluationDeferred;
    std::atomic<bool> PendingJobPumpActive;
    csp::common::Vector3 LocalPlayerCameraPosition;
    csp::common::Vector4 LocalPlayerCameraRotation;
    csp::common::Vector3 LocalPlayerCameraForward;
    csp::common::Vector3 LocalPlayerCameraForwardFlat;
    csp::common::Vector3 LocalPlayerCameraRight;
    csp::common::Vector3 LocalPlayerCameraRightFlat;
    csp::common::Vector3 LocalPlayerCameraUp;
    bool LocalPlayerXrActive;
    std::unique_ptr<NgxUIRuntime> UIRuntime;
    std::unique_ptr<csp::multiplayer::NgxAssetScriptBinding> AssetBinding;
    bool bAssetDetailBlobChangedListenerRegistered;
    uint32_t GcTickCounter;
    std::unique_ptr<NgxScriptTickEventHandler> TickEventHandler;
    CSP_END_IGNORE
};

} // namespace csp::systems
