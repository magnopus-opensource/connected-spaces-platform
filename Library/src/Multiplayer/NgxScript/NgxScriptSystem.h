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

namespace qjs
{
class Runtime;
class Context;
} // namespace qjs

namespace csp::common
{
class IRealtimeEngine;
class LogSystem;
} // namespace csp::common

namespace csp::systems::ngxscript
{

CSP_NO_EXPORT csp::common::String StripRandomPartFromScriptPath(const csp::common::String& ScriptPath);

CSP_NO_EXPORT csp::common::String BuildCanonicalAssetPath(const csp::systems::Asset& Asset,
    const csp::common::Map<csp::common::String, csp::systems::AssetCollection>& AssetCollectionMap);

} // namespace csp::systems::ngxscript

namespace csp::systems
{

class NgxScriptSystem
{
public:
    explicit NgxScriptSystem(csp::common::LogSystem& InLogSystem);
    ~NgxScriptSystem();

    void OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine);
    void OnExitSpace();

    // Manual single-module refresh path.
    void ReloadScriptModule(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId);

#ifdef CSP_TESTS
    CSP_NO_EXPORT bool HasActiveContextForTesting() const;
    CSP_NO_EXPORT bool EvaluateModuleScriptForTesting(const csp::common::String& ScriptText);
    CSP_NO_EXPORT int32_t GetGlobalIntForTesting(const csp::common::String& Key, int32_t DefaultValue = 0) const;
    CSP_NO_EXPORT void SetLoadedModuleSourceForTesting(const csp::common::String& ModulePath, const csp::common::String& ModuleSource);
    CSP_NO_EXPORT bool RunEntryModuleForTesting(const csp::common::String& EntryModulePath);
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
    void RunBootstrapScript();

    void LoadScriptModules();

    void FetchAssetCollectionMapForSpace(
        uint64_t Generation, std::function<void(std::shared_ptr<AssetCollectionMap>)> Callback) const;

    bool IsGenerationCurrent(uint64_t Generation) const;
    bool EvaluateModuleScript(const std::string& ScriptText, const char* DebugName);
    bool ExecuteEntryModule(const std::string& EntryModulePath);

    csp::common::LogSystem& LogSystem;
    csp::common::IRealtimeEngine* ActiveRealtimeEngine;
    csp::common::String ActiveSpaceId;

    std::unique_ptr<qjs::Runtime> Runtime;
    std::unique_ptr<qjs::Context> Context;

    mutable std::mutex ContextMutex;
    mutable std::mutex ModuleSourcesMutex;
    ModuleSourceMap LoadedModuleSources;

    std::atomic<uint64_t> SessionGeneration;
    std::unique_ptr<NgxScriptTickEventHandler> TickEventHandler;
};

} // namespace csp::systems
