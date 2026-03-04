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
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

    CSP_START_IGNORE
    // Internal runtime hooks (not exposed through wrappers).
    CSP_NO_EXPORT bool HasModuleSource(const csp::common::String& ModulePath) const;
    CSP_NO_EXPORT bool EvaluateSnippet(const csp::common::String& ScriptText, const csp::common::String& DebugName);
    CSP_NO_EXPORT void PumpPendingJobs();
    CSP_NO_EXPORT bool AreScriptModulesLoaded() const;
    CSP_NO_EXPORT uint64_t GetContextGeneration() const;

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

    void LoadScriptModules();
    void RegisterAssetDetailBlobChangedListener();
    void UnregisterAssetDetailBlobChangedListener();
    void OnAssetDetailBlobChanged(const csp::common::NetworkEventData& NetworkEventData);
    bool IsTrackedScriptAssetCollection(const csp::common::String& AssetCollectionId) const;

    void FetchAssetCollectionMapForSpace(uint64_t Generation, std::function<void(std::shared_ptr<AssetCollectionMap>)> Callback) const;

    bool IsGenerationCurrent(uint64_t Generation) const;
    bool EvaluateModuleScript(const std::string& ScriptText, const char* DebugName);

    csp::common::LogSystem& LogSystem;
    csp::common::IRealtimeEngine* ActiveRealtimeEngine;
    csp::common::String ActiveSpaceId;

    std::unique_ptr<qjs::Runtime> Runtime;
    std::unique_ptr<qjs::Context> Context;

    mutable std::mutex ContextMutex;
    mutable std::mutex ModuleSourcesMutex;
    ModuleSourceMap LoadedModuleSources;
    ModuleSourceMap StaticModuleSources;
    std::unordered_set<std::string> TrackedScriptAssetCollectionIds;

    std::atomic<uint64_t> SessionGeneration;
    std::atomic<uint64_t> ContextGeneration;
    std::atomic<bool> ScriptModulesLoaded;
    bool bAssetDetailBlobChangedListenerRegistered;
    std::unique_ptr<NgxScriptTickEventHandler> TickEventHandler;
    CSP_END_IGNORE
};

} // namespace csp::systems
