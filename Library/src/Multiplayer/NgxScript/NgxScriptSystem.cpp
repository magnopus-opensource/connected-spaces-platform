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

#include "Multiplayer/NgxScript/NgxScriptSystem.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"

#include "quickjs.h"
#include "quickjspp.hpp"

#include <fmt/format.h>
#include <optional>
#include <vector>

namespace
{

const char* RealtimeEngineTypeToString(csp::common::IRealtimeEngine* RealtimeEngine)
{
    if (RealtimeEngine == nullptr)
    {
        return "Null";
    }

    switch (RealtimeEngine->GetRealtimeEngineType())
    {
    case csp::common::RealtimeEngineType::Online:
        return "Online";
    case csp::common::RealtimeEngineType::Offline:
        return "Offline";
    default:
        return "Unknown";
    }
}

std::string EscapeJSStringLiteral(const std::string& Input)
{
    std::string Escaped;
    Escaped.reserve(Input.size());

    for (const char Character : Input)
    {
        switch (Character)
        {
        case '\\':
            Escaped += "\\\\";
            break;
        case '\'':
            Escaped += "\\'";
            break;
        case '"':
            Escaped += "\\\"";
            break;
        case '\n':
            Escaped += "\\n";
            break;
        case '\r':
            Escaped += "\\r";
            break;
        case '\t':
            Escaped += "\\t";
            break;
        default:
            Escaped += Character;
            break;
        }
    }

    return Escaped;
}

std::string JoinArgs(const qjs::rest<std::string>& Args)
{
    std::string Output;

    for (size_t Index = 0; Index < Args.size(); ++Index)
    {
        Output += Args[Index];
        if ((Index + 1) < Args.size())
        {
            Output += " ";
        }
    }

    return Output;
}

} // namespace

namespace csp::systems::ngxscript
{

csp::common::String StripRandomPartFromScriptPath(const csp::common::String& ScriptPath)
{
    constexpr const char* RANDOM_SUFFIX_SEPARATOR = "___";
    const std::string ScriptPathStd = ScriptPath.c_str();
    const size_t FoundIndex = ScriptPathStd.find(RANDOM_SUFFIX_SEPARATOR);

    if (FoundIndex != std::string::npos)
    {
        return csp::common::String(ScriptPathStd.substr(0, FoundIndex).c_str());
    }

    return ScriptPath;
}

csp::common::String BuildCanonicalAssetPath(
    const csp::systems::Asset& Asset, const csp::common::Map<csp::common::String, csp::systems::AssetCollection>& AssetCollectionMap)
{
    csp::common::String Path = StripRandomPartFromScriptPath(Asset.Name);
    csp::common::String CurrentCollectionId = Asset.AssetCollectionId;

    while (!CurrentCollectionId.IsEmpty())
    {
        if (!AssetCollectionMap.HasKey(CurrentCollectionId))
        {
            break;
        }

        const auto& CurrentCollection = AssetCollectionMap[CurrentCollectionId];
        Path = StripRandomPartFromScriptPath(CurrentCollection.Name) + "/" + Path;
        CurrentCollectionId = CurrentCollection.ParentId;
    }

    return "/" + Path;
}

} // namespace csp::systems::ngxscript

namespace csp::systems
{

class NgxScriptSystem::NgxScriptTickEventHandler final : public csp::events::EventListener
{
public:
    explicit NgxScriptTickEventHandler(NgxScriptSystem* InSystem)
        : System(InSystem)
    {
    }

    void OnEvent(const csp::events::Event& InEvent) override
    {
        if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID)
        {
            System->DrainPendingJobs();
        }
    }

private:
    NgxScriptSystem* System;
};

NgxScriptSystem::NgxScriptSystem(csp::common::LogSystem& InLogSystem)
    : LogSystem(InLogSystem)
    , ActiveRealtimeEngine(nullptr)
    , ActiveSpaceId("")
    , Runtime(std::make_unique<qjs::Runtime>())
    , Context(nullptr)
    , SessionGeneration(0)
    , TickEventHandler(std::make_unique<NgxScriptTickEventHandler>(this))
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: System constructed and tick listener registered.");
}

NgxScriptSystem::~NgxScriptSystem()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: System shutting down.");
    OnExitSpace();
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
}

void NgxScriptSystem::OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine)
{
    const std::string EnterMessage = fmt::format("NgxScript Trace: OnEnterSpace(spaceId='{}', engineType={}).", InSpaceId.c_str(),
        RealtimeEngineTypeToString(InRealtimeEngine));
    LogSystem.LogMsg(csp::common::LogLevel::Log, EnterMessage.c_str());

    ActiveSpaceId = InSpaceId;
    ActiveRealtimeEngine = InRealtimeEngine;
    SessionGeneration.fetch_add(1);

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        LoadedModuleSources.clear();
    }

    RebuildContext();
    LoadScriptModules();
}

void NgxScriptSystem::OnExitSpace()
{
    const std::string ExitMessage = fmt::format("NgxScript Trace: OnExitSpace(spaceId='{}').", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, ExitMessage.c_str());

    SessionGeneration.fetch_add(1);
    ActiveSpaceId = "";
    ActiveRealtimeEngine = nullptr;

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        LoadedModuleSources.clear();
    }

    TeardownContext();
}

void NgxScriptSystem::ReloadScriptModule(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
{
    const std::string ReloadRequestMessage = fmt::format(
        "NgxScript Trace: ReloadScriptModule(assetCollectionId='{}', assetId='{}').", AssetCollectionId.c_str(), AssetId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, ReloadRequestMessage.c_str());

    if (AssetCollectionId.IsEmpty() || AssetId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Cannot reload script with empty asset identifiers.");
        return;
    }

    if (ActiveSpaceId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Cannot reload script while no space is active.");
        return;
    }

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: CSPFoundation is not initialised. Skipping module reload.");
        return;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable for script reload.");
        return;
    }

    const uint64_t Generation = SessionGeneration.load();
    FetchAssetCollectionMapForSpace(Generation,
        [this, Generation, AssetCollectionId, AssetId, AssetSystem](std::shared_ptr<AssetCollectionMap> AssetCollections)
        {
            if (!IsGenerationCurrent(Generation))
            {
                return;
            }

            AssetSystem->GetAssetById(AssetCollectionId, AssetId,
                [this, Generation, AssetCollections, AssetSystem](const csp::systems::AssetResult& AssetResult)
                {
                    if (!IsGenerationCurrent(Generation))
                    {
                        return;
                    }

                    if (AssetResult.GetResultCode() != csp::systems::EResultCode::Success)
                    {
                        LogSystem.LogMsg(
                            csp::common::LogLevel::Error, "NgxScript: Failed to find script asset to reload by id.");
                        return;
                    }

                    const csp::systems::Asset ScriptAsset = AssetResult.GetAsset();
                    AssetSystem->DownloadAssetData(ScriptAsset,
                        [this, Generation, AssetCollections, ScriptAsset](const csp::systems::AssetDataResult& DownloadResult)
                        {
                            if (!IsGenerationCurrent(Generation))
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() == csp::systems::EResultCode::InProgress)
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() != csp::systems::EResultCode::Success)
                            {
                                LogSystem.LogMsg(
                                    csp::common::LogLevel::Error, "NgxScript: Failed to download script module for reload.");
                                return;
                            }

                            const char* ModuleSourceData = static_cast<const char*>(DownloadResult.GetData());
                            const size_t ModuleSourceLength = DownloadResult.GetDataLength();
                            const std::string ModuleSource = (ModuleSourceData != nullptr)
                                ? std::string(ModuleSourceData, ModuleSourceLength)
                                : std::string();

                            const csp::common::String CanonicalPath
                                = ngxscript::BuildCanonicalAssetPath(ScriptAsset, *AssetCollections);
                            const std::string ReloadSuccessMessage
                                = fmt::format("NgxScript Trace: Reloaded module '{}'.", CanonicalPath.c_str());
                            LogSystem.LogMsg(csp::common::LogLevel::Log, ReloadSuccessMessage.c_str());

                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources[CanonicalPath.c_str()] = ModuleSource;
                            }

                            // Rebuild context to clear module cache and execute updated source.
                            RebuildContext();
                        });
                });
        });
}

#ifdef CSP_TESTS
bool NgxScriptSystem::HasActiveContextForTesting() const
{
    std::scoped_lock ContextLock(ContextMutex);
    return Context != nullptr;
}

bool NgxScriptSystem::EvaluateModuleScriptForTesting(const csp::common::String& ScriptText)
{
    return EvaluateModuleScript(ScriptText.c_str(), "<ngx-test>");
}

int32_t NgxScriptSystem::GetGlobalIntForTesting(const csp::common::String& Key, int32_t DefaultValue) const
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return DefaultValue;
    }

    try
    {
        const qjs::Value Value = Context->global()[Key.c_str()];
        return Value.as<int32_t>();
    }
    catch (...)
    {
        return DefaultValue;
    }
}

void NgxScriptSystem::SetLoadedModuleSourceForTesting(const csp::common::String& ModulePath, const csp::common::String& ModuleSource)
{
    std::scoped_lock ModuleLock(ModuleSourcesMutex);
    LoadedModuleSources[ModulePath.c_str()] = ModuleSource.c_str();
}

bool NgxScriptSystem::RunModuleForTesting(const csp::common::String& ModulePath)
{
    // Mirror reload flow so test calls observe freshly compiled module state.
    RebuildContext();
    return ExecuteModule(ModulePath);
}
#endif

void NgxScriptSystem::RebuildContext()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: RebuildContext begin.");

    {
        std::scoped_lock ContextLock(ContextMutex);
        Context.reset();

        if (!Runtime)
        {
            Runtime = std::make_unique<qjs::Runtime>();
        }

        Context = std::make_unique<qjs::Context>(*Runtime);
    }

    InstallModuleLoader();
    InstallHostBindings();
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: RebuildContext complete.");
}

void NgxScriptSystem::TeardownContext()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: TeardownContext.");
    std::scoped_lock ContextLock(ContextMutex);
    Context.reset();
}

void NgxScriptSystem::InstallModuleLoader()
{
    std::scoped_lock ContextLock(ContextMutex);
    if (Context)
    {
        Context->moduleLoader = [this](std::string_view ModuleName) -> qjs::Context::ModuleData
        {
            const std::string RequestedModule(ModuleName);

            {
                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                auto SourceIt = LoadedModuleSources.find(RequestedModule);
                if (SourceIt != LoadedModuleSources.end())
                {
                    const std::string ResolveMessage
                        = fmt::format("NgxScript Trace: Resolved module '{}' from in-memory source map.", RequestedModule);
                    LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                    return qjs::Context::ModuleData { RequestedModule, SourceIt->second, std::nullopt };
                }
            }

            const std::string ErrorMessage = fmt::format("NgxScript: Missing module '{}'.", RequestedModule);
            LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMessage.c_str());

            const std::string ThrowingModule
                = "throw new Error('NgxScript module not found: " + EscapeJSStringLiteral(RequestedModule) + "');";
            return qjs::Context::ModuleData { RequestedModule, ThrowingModule, std::nullopt };
        };
        LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: Module loader installed.");
    }
}

void NgxScriptSystem::InstallHostBindings()
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return;
    }

    auto& CSPModule = Context->addModule("csp");
    CSPModule.function("__log", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Log, JoinArgs(Args).c_str()); });
    CSPModule.function(
        "__warn", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Warning, JoinArgs(Args).c_str()); });
    CSPModule.function(
        "__error", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Error, JoinArgs(Args).c_str()); });

    static constexpr const char* HOST_BINDINGS_SCRIPT = R"(
import * as csp from "csp";
globalThis.csp = csp;
globalThis.console = {
    log: (...args) => csp.__log(...args.map((value) => String(value))),
    warn: (...args) => csp.__warn(...args.map((value) => String(value))),
    error: (...args) => csp.__error(...args.map((value) => String(value))),
};
)";

    const qjs::Value EvalResult = Context->eval(HOST_BINDINGS_SCRIPT, "<ngx-host-bindings>", JS_EVAL_TYPE_MODULE);
    if (EvalResult.isException())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to install host bindings.");
        return;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: Host bindings installed.");
}

void NgxScriptSystem::DrainPendingJobs()
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Runtime || !Context)
    {
        return;
    }

    while (Runtime->isJobPending())
    {
        Runtime->executePendingJob();
    }
}

void NgxScriptSystem::LoadScriptModules()
{
    const std::string LoadStartMessage = fmt::format("NgxScript Trace: LoadScriptModules(spaceId='{}').", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, LoadStartMessage.c_str());

    if (ActiveSpaceId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript Trace: LoadScriptModules aborted (no active space).");
        return;
    }

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript Trace: LoadScriptModules aborted (CSPFoundation not initialised).");
        return;
    }

    const uint64_t Generation = SessionGeneration.load();
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable, cannot load script modules.");
        return;
    }

    FetchAssetCollectionMapForSpace(Generation,
        [this, Generation, AssetSystem](std::shared_ptr<AssetCollectionMap> AssetCollections)
        {
            if (!IsGenerationCurrent(Generation))
            {
                return;
            }

            std::unique_ptr<const csp::common::Array<csp::common::String>> AssetCollectionIds(AssetCollections->Keys());
            if (!AssetCollectionIds || AssetCollectionIds->IsEmpty())
            {
                LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: No asset collections found while loading scripts.");
                return;
            }

            auto BeginDownloads = [this, Generation, AssetSystem, AssetCollections](const std::vector<csp::systems::Asset>& ScriptAssets)
            {
                const size_t TotalAssets = ScriptAssets.size();
                if (TotalAssets == 0)
                {
                    LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: No JavaScript module assets found for active space.");
                    return;
                }

                auto DownloadedSources = std::make_shared<ModuleSourceMap>();
                auto DownloadedCount = std::make_shared<size_t>(0);
                auto HasFailed = std::make_shared<bool>(false);

                for (const csp::systems::Asset& ScriptAsset : ScriptAssets)
                {
                    AssetSystem->DownloadAssetData(ScriptAsset,
                        [this, Generation, TotalAssets, ScriptAsset, AssetCollections, DownloadedSources, DownloadedCount, HasFailed](
                            const csp::systems::AssetDataResult& DownloadResult)
                        {
                            if (!IsGenerationCurrent(Generation))
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() == csp::systems::EResultCode::InProgress)
                            {
                                return;
                            }

                            if (*HasFailed)
                            {
                                return;
                            }

                            if (DownloadResult.GetResultCode() != csp::systems::EResultCode::Success)
                            {
                                *HasFailed = true;
                                LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to download a script module.");
                                return;
                            }

                            const char* ModuleSourceData = static_cast<const char*>(DownloadResult.GetData());
                            const size_t ModuleSourceLength = DownloadResult.GetDataLength();
                            const std::string ModuleSource = (ModuleSourceData != nullptr)
                                ? std::string(ModuleSourceData, ModuleSourceLength)
                                : std::string();

                            const csp::common::String CanonicalPath = ngxscript::BuildCanonicalAssetPath(ScriptAsset, *AssetCollections);
                            (*DownloadedSources)[CanonicalPath.c_str()] = ModuleSource;
                            const std::string DownloadMessage = fmt::format("NgxScript Trace: Loaded module '{}'.", CanonicalPath.c_str());
                            LogSystem.LogMsg(csp::common::LogLevel::Verbose, DownloadMessage.c_str());

                            (*DownloadedCount)++;
                            if (*DownloadedCount < TotalAssets)
                            {
                                return;
                            }

                            size_t LoadedModuleCount = 0;
                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources = *DownloadedSources;
                                LoadedModuleCount = LoadedModuleSources.size();
                            }

                            const std::string LoadCompleteMessage
                                = fmt::format("NgxScript Trace: Module load complete ({} modules).", LoadedModuleCount);
                            LogSystem.LogMsg(csp::common::LogLevel::Log, LoadCompleteMessage.c_str());
                        });
                }
            };

            AssetSystem->GetAssetsByCriteria(*AssetCollectionIds, nullptr, nullptr, csp::common::Array { csp::systems::EAssetType::SCRIPT_LIBRARY },
                [this, Generation, BeginDownloads](const csp::systems::AssetsResult& ScriptLibraryAssetsResult)
                {
                    if (!IsGenerationCurrent(Generation))
                    {
                        return;
                    }

                    if (ScriptLibraryAssetsResult.GetResultCode() != csp::systems::EResultCode::Success)
                    {
                        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Failed to load script asset metadata.");
                        return;
                    }

                    const auto ScriptLibraryAssetsArray = ScriptLibraryAssetsResult.GetAssets();
                    std::vector<csp::systems::Asset> ScriptLibraryAssets;
                    ScriptLibraryAssets.reserve(ScriptLibraryAssetsArray.Size());
                    for (size_t Index = 0; Index < ScriptLibraryAssetsArray.Size(); ++Index)
                    {
                        ScriptLibraryAssets.emplace_back(ScriptLibraryAssetsArray[Index]);
                    }

                    const std::string DiscoveredScriptLibrariesMessage = fmt::format(
                        "NgxScript Trace: Discovered {} script library asset(s) for active space.", ScriptLibraryAssets.size());
                    LogSystem.LogMsg(csp::common::LogLevel::Log, DiscoveredScriptLibrariesMessage.c_str());

                    if (!ScriptLibraryAssets.empty())
                    {
                        BeginDownloads(ScriptLibraryAssets);
                        return;
                    }

                    LogSystem.LogMsg(csp::common::LogLevel::Warning,
                        "NgxScript: No script library assets found for active space. Ensure scripts are uploaded as EAssetType::SCRIPT_LIBRARY.");
                });
        });
}

void NgxScriptSystem::FetchAssetCollectionMapForSpace(
    uint64_t Generation, std::function<void(std::shared_ptr<AssetCollectionMap>)> Callback) const
{
    const std::string QueryCollectionsMessage
        = fmt::format("NgxScript Trace: Querying asset collections for spaceId='{}'.", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, QueryCollectionsMessage.c_str());

    if (ActiveSpaceId.IsEmpty())
    {
        return;
    }

    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable while resolving asset collections.");
        return;
    }

    AssetSystem->FindAssetCollections(
        nullptr, nullptr, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { ActiveSpaceId }, nullptr, nullptr,
        [this, Generation, Callback = std::move(Callback)](const csp::systems::AssetCollectionsResult& AssetCollectionsResult)
        {
            if (!IsGenerationCurrent(Generation))
            {
                return;
            }

            if (AssetCollectionsResult.GetResultCode() != csp::systems::EResultCode::Success)
            {
                LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Failed to query asset collections for space.");
                return;
            }

            auto ResultMap = std::make_shared<AssetCollectionMap>();
            const auto Collections = AssetCollectionsResult.GetAssetCollections();
            for (size_t Index = 0; Index < Collections.Size(); ++Index)
            {
                (*ResultMap)[Collections[Index].Id] = Collections[Index];
            }

            const std::string CollectionsMessage
                = fmt::format("NgxScript Trace: Resolved {} asset collection(s).", ResultMap->Size());
            LogSystem.LogMsg(csp::common::LogLevel::Log, CollectionsMessage.c_str());
            Callback(ResultMap);
        });
}

bool NgxScriptSystem::IsGenerationCurrent(uint64_t Generation) const { return SessionGeneration.load() == Generation; }

bool NgxScriptSystem::EvaluateModuleScript(const std::string& ScriptText, const char* DebugName)
{
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    const qjs::Value EvalResult = Context->eval(ScriptText, DebugName, JS_EVAL_TYPE_MODULE);
    if (!EvalResult.isException())
    {
        const std::string SuccessMessage = fmt::format("NgxScript Trace: JavaScript execution succeeded ({}).", DebugName);
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, SuccessMessage.c_str());
        return true;
    }

    std::string ErrorMessage = "NgxScript: JavaScript execution failed.";

    try
    {
        const qjs::Value ExceptionValue = Context->getException();
        ErrorMessage += " ";
        ErrorMessage += ExceptionValue.as<std::string>();
    }
    catch (...)
    {
    }

    LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMessage.c_str());
    return false;
}

bool NgxScriptSystem::ExecuteModule(const csp::common::String& ModulePath)
{
    const std::string ModulePathStd = ModulePath.c_str();
    const std::string RunModuleMessage = fmt::format("NgxScript Trace: Executing module '{}'.", ModulePathStd);
    LogSystem.LogMsg(csp::common::LogLevel::Log, RunModuleMessage.c_str());

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        if (LoadedModuleSources.find(ModulePathStd) == LoadedModuleSources.end())
        {
            const std::string WarningMessage = fmt::format(
                "NgxScript: Module '{}' not found in loaded module map.", ModulePathStd);
            LogSystem.LogMsg(csp::common::LogLevel::Warning, WarningMessage.c_str());
            return false;
        }
    }

    const std::string Script = "import '" + EscapeJSStringLiteral(ModulePathStd) + "';";

    const bool bSuccess = EvaluateModuleScript(Script, "<ngx-module>");

    const std::string CompletedMessage = fmt::format(
        "NgxScript Trace: Module '{}' {}.", ModulePathStd, bSuccess ? "completed" : "failed");
    LogSystem.LogMsg(csp::common::LogLevel::Log, CompletedMessage.c_str());
    return bSuccess;
}

} // namespace csp::systems
