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
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/Script/EntityScriptInterface.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Multiplayer/EntityQueryUtils.h"
#include "Multiplayer/NgxScript/NgxAssetScriptBinding.h"
#include "Multiplayer/NgxScript/NgxEntityScriptBinding.h"
#include "Multiplayer/NgxScript/NgxUIRuntime.h"
#include "Multiplayer/NgxScript/signals.h"

#include "quickjs.h"
#include "quickjspp.hpp"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cmath>
#include <fmt/format.h>
#include <optional>
#include <vector>

namespace
{
// QuickJS bounds its own recursion against a saved stack-top baseline. Must be
// smaller than V8's native stack budget so QuickJS throws InternalError: stack
// overflow (with a JS-level trace) before V8 throws an opaque RangeError from
// inside JS_CallInternal. DevTools shrinks V8's budget significantly, so keep
// this well under ~1MB. 256KB ≈ ~256 QuickJS frames — shallow enough to trip
// first under DevTools, deep enough for typical script recursion.
constexpr size_t NGX_SCRIPT_WASM_MAX_STACK_BYTES = 256 * 1024;

constexpr const char* PREACT_SIGNALS_CORE_MODULE = "@preact/signals-core";
constexpr const char* CODECOMPONENT_JSON_RESULT_SLOT = "__cspCodeComponentJsonResult";
constexpr const char* CODECOMPONENT_BOOL_RESULT_SLOT = "__cspCodeComponentBoolResult";
constexpr const char* EMPTY_JSON_OBJECT_STRING = "{}";
constexpr const char* ASSET_BLOB_CHANGED_RECEIVER_ID = "CSPInternal::NgxScriptSystem";

#ifdef CSP_WASM
// Re-baselines QuickJS's stack-overflow check to the caller's current SP.
// JS_NewRuntime captured stack_top at system-construction time (deep inside
// Module.ready resolution); any later entry (rAF tick, SignalR callback →
// ccall → eval, direct bindings) runs from a shallower stack. Without a fresh
// baseline the check is always "not overflowing" and V8 surfaces the overflow
// instead. Call this at every external entry into QuickJS, then re-apply the
// size so the limit (= top − size) is computed against the current frame.
void RebaseQuickJsStackCheck(qjs::Runtime& Runtime)
{
    JS_UpdateStackTop(Runtime.rt);
    JS_SetMaxStackSize(Runtime.rt, NGX_SCRIPT_WASM_MAX_STACK_BYTES);
}
#endif

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

std::optional<uint64_t> TryParseEntityId(const std::string& EntityIdText)
{
    if (EntityIdText.empty())
    {
        return std::nullopt;
    }

    try
    {
        return std::stoull(EntityIdText);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

csp::multiplayer::SpaceEntity* FindEntityById(const std::vector<csp::multiplayer::SpaceEntity*>& Entities, uint64_t EntityId)
{
    for (auto* Entity : Entities)
    {
        if ((Entity != nullptr) && (Entity->GetId() == EntityId))
        {
            return Entity;
        }
    }

    return nullptr;
}

struct EntityUpdateLockGuard
{
    explicit EntityUpdateLockGuard(csp::common::IRealtimeEngine* InEngine)
        : Engine(InEngine)
        , bLocked(false)
    {
        if (Engine != nullptr)
        {
            Engine->LockEntityUpdate();
            bLocked = true;
        }
    }

    ~EntityUpdateLockGuard()
    {
        if (bLocked && (Engine != nullptr))
        {
            Engine->UnlockEntityUpdate();
        }
    }

    csp::common::IRealtimeEngine* Engine;
    bool bLocked;
};

std::vector<csp::multiplayer::SpaceEntity*> CollectEntities(csp::common::IRealtimeEngine* RealtimeEngine)
{
    std::vector<csp::multiplayer::SpaceEntity*> Entities;
    if (RealtimeEngine == nullptr)
    {
        return Entities;
    }

    const size_t EntityCount = RealtimeEngine->GetNumEntities();
    Entities.reserve(EntityCount);
    for (size_t Index = 0; Index < EntityCount; ++Index)
    {
        auto* Entity = RealtimeEngine->GetEntityByIndex(Index);
        if (Entity != nullptr)
        {
            Entities.push_back(Entity);
        }
    }

    return Entities;
}

std::string BuildEntitySnapshotJson(const csp::multiplayer::SpaceEntity& Entity)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);

    Writer.StartObject();
    Writer.Key("id");
    Writer.Uint64(Entity.GetId());
    Writer.Key("name");
    Writer.String(Entity.GetName().c_str());
    Writer.Key("thirdPartyRef");
    Writer.String(Entity.GetThirdPartyRef().c_str());

    Writer.Key("position");
    Writer.StartArray();
    const auto Position = Entity.GetPosition();
    Writer.Double(static_cast<double>(Position.X));
    Writer.Double(static_cast<double>(Position.Y));
    Writer.Double(static_cast<double>(Position.Z));
    Writer.EndArray();

    Writer.Key("rotation");
    Writer.StartArray();
    const auto Rotation = Entity.GetRotation();
    Writer.Double(static_cast<double>(Rotation.X));
    Writer.Double(static_cast<double>(Rotation.Y));
    Writer.Double(static_cast<double>(Rotation.Z));
    Writer.Double(static_cast<double>(Rotation.W));
    Writer.EndArray();

    Writer.Key("scale");
    Writer.StartArray();
    const auto Scale = Entity.GetScale();
    Writer.Double(static_cast<double>(Scale.X));
    Writer.Double(static_cast<double>(Scale.Y));
    Writer.Double(static_cast<double>(Scale.Z));
    Writer.EndArray();

    Writer.EndObject();
    return Buffer.GetString();
}

std::string BuildComponentSnapshotJson(const csp::multiplayer::ComponentBase& Component)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);

    Writer.StartObject();
    Writer.Key("componentId");
    Writer.Uint(Component.GetId());
    Writer.Key("componentType");
    Writer.Int(static_cast<int32_t>(Component.GetComponentType()));
    Writer.Key("componentName");
    Writer.String(Component.GetComponentName().c_str());

    Writer.EndObject();

    return Buffer.GetString();
}

csp::multiplayer::ComponentBase* FindComponentByTypeAndIndex(
    csp::multiplayer::SpaceEntity& Entity, csp::multiplayer::ComponentType ComponentType, int32_t Index)
{
    if (Index < 0)
    {
        return nullptr;
    }

    const auto* Components = Entity.GetComponents();
    if (Components == nullptr)
    {
        return nullptr;
    }

    int32_t CurrentIndex = 0;
    for (const auto& ComponentPair : *Components)
    {
        auto* Component = ComponentPair.second;
        if ((Component != nullptr) && (Component->GetComponentType() == ComponentType))
        {
            if (CurrentIndex == Index)
            {
                return Component;
            }

            ++CurrentIndex;
        }
    }

    return nullptr;
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
namespace
{
std::vector<float> ToJSVector(const csp::common::Vector3& Value)
{
    return { Value.X, Value.Y, Value.Z };
}

std::vector<float> ToJSVector(const csp::common::Vector4& Value)
{
    return { Value.X, Value.Y, Value.Z, Value.W };
}
} // namespace

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
            System->PumpPendingJobs();
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
    , ContextGeneration(0)
    , ScriptModulesLoaded(false)
    , LastEvaluationDeferred(false)
    , PendingJobPumpActive(false)
    , LocalPlayerXrActive(false)
    , UIRuntime(std::make_unique<NgxUIRuntime>(InLogSystem))
    , bAssetDetailBlobChangedListenerRegistered(false)
    , GcTickCounter(0)
    , TickEventHandler(std::make_unique<NgxScriptTickEventHandler>(this))
{
    //ApplyNgxRuntimeLimits(*Runtime);
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
    const std::string EnterMessage
        = fmt::format("NgxScript Trace: OnEnterSpace(spaceId='{}', engineType={}).", InSpaceId.c_str(), RealtimeEngineTypeToString(InRealtimeEngine));
    LogSystem.LogMsg(csp::common::LogLevel::Log, EnterMessage.c_str());

    ActiveSpaceId = InSpaceId;
    ActiveRealtimeEngine = InRealtimeEngine;
    SessionGeneration.fetch_add(1);
    ScriptModulesLoaded.store(false);

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        LoadedModuleSources.clear();
        TrackedScriptAssetCollectionIds.clear();
    }

    RegisterAssetDetailBlobChangedListener();
    RebuildContext();
    LoadScriptModules();
}

void NgxScriptSystem::OnExitSpace()
{
    const std::string ExitMessage = fmt::format("NgxScript Trace: OnExitSpace(spaceId='{}').", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, ExitMessage.c_str());

    SessionGeneration.fetch_add(1);
    ScriptModulesLoaded.store(false);
    UnregisterAssetDetailBlobChangedListener();
    ActiveSpaceId = "";
    ActiveRealtimeEngine = nullptr;

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        LoadedModuleSources.clear();
        TrackedScriptAssetCollectionIds.clear();
    }

    {
        std::scoped_lock UILock(UIMutex);
        UIRuntime->Clear();
    }

    TeardownContext();
}

void NgxScriptSystem::RegisterAssetDetailBlobChangedListener()
{
    if (bAssetDetailBlobChangedListenerRegistered)
    {
        return;
    }

    auto* MultiplayerConnection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();
    if (!MultiplayerConnection)
    {
        LogSystem.LogMsg(
            csp::common::LogLevel::Warning, "NgxScript: Failed to register AssetDetailBlobChanged listener (MultiplayerConnection unavailable).");
        return;
    }

    auto* EventBus = &MultiplayerConnection->GetEventBus();
    const csp::common::String EventName
        = csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged);

    EventBus->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ASSET_BLOB_CHANGED_RECEIVER_ID, EventName),
        [this](const csp::common::NetworkEventData& NetworkEventData) { this->OnAssetDetailBlobChanged(NetworkEventData); });

    bAssetDetailBlobChangedListenerRegistered = true;
}

void NgxScriptSystem::UnregisterAssetDetailBlobChangedListener()
{
    if (!bAssetDetailBlobChangedListenerRegistered)
    {
        return;
    }

    if (auto* MultiplayerConnection = csp::systems::SystemsManager::Get().GetMultiplayerConnection())
    {
        auto* EventBus = &MultiplayerConnection->GetEventBus();
        const csp::common::String EventName
            = csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AssetDetailBlobChanged);
        EventBus->StopListenNetworkEvent(csp::multiplayer::NetworkEventRegistration(ASSET_BLOB_CHANGED_RECEIVER_ID, EventName));
    }

    bAssetDetailBlobChangedListenerRegistered = false;
}

bool NgxScriptSystem::IsTrackedScriptAssetCollection(const csp::common::String& AssetCollectionId) const
{
    if (AssetCollectionId.IsEmpty())
    {
        return false;
    }

    std::scoped_lock ModuleLock(ModuleSourcesMutex);
    return TrackedScriptAssetCollectionIds.find(AssetCollectionId.c_str()) != TrackedScriptAssetCollectionIds.end();
}

void NgxScriptSystem::OnAssetDetailBlobChanged(const csp::common::NetworkEventData& NetworkEventData)
{
    if (ActiveSpaceId.IsEmpty())
    {
        return;
    }

    const auto& AssetBlobEvent = static_cast<const csp::common::AssetDetailBlobChangedNetworkEventData&>(NetworkEventData);
    if (AssetBlobEvent.AssetType != csp::systems::EAssetType::SCRIPT_LIBRARY)
    {
        return;
    }

    if (!IsTrackedScriptAssetCollection(AssetBlobEvent.AssetCollectionId))
    {
        return;
    }

    if ((AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Created)
        || (AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Updated))
    {
        ReloadScriptModule(AssetBlobEvent.AssetCollectionId, AssetBlobEvent.AssetId);
        return;
    }

    if (AssetBlobEvent.ChangeType == csp::common::EAssetChangeType::Deleted)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: Script library asset deleted; rebuilding context and reloading module map.");
        ScriptModulesLoaded.store(false);
        RebuildContext();
        LoadScriptModules();
    }
}

void NgxScriptSystem::RegisterStaticModuleSource(const csp::common::String& ModulePath, const csp::common::String& ModuleSource)
{
    if (ModulePath.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: Cannot register static module source with empty module path.");
        return;
    }

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        StaticModuleSources[ModulePath.c_str()] = ModuleSource.c_str();
    }

    const std::string RegisterMessage = fmt::format("NgxScript Trace: Registered static module source '{}'.", ModulePath.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, RegisterMessage.c_str());
}

void NgxScriptSystem::UnregisterStaticModuleSource(const csp::common::String& ModulePath)
{
    if (ModulePath.IsEmpty())
    {
        return;
    }

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        StaticModuleSources.erase(ModulePath.c_str());
    }

    const std::string UnregisterMessage = fmt::format("NgxScript Trace: Unregistered static module source '{}'.", ModulePath.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, UnregisterMessage.c_str());
}

void NgxScriptSystem::ReloadScriptModule(const csp::common::String& AssetCollectionId, const csp::common::String& AssetId)
{
    const std::string ReloadRequestMessage
        = fmt::format("NgxScript Trace: ReloadScriptModule(assetCollectionId='{}', assetId='{}').", AssetCollectionId.c_str(), AssetId.c_str());
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
                        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to find script asset to reload by id.");
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
                                LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to download script module for reload.");
                                return;
                            }

                            const char* ModuleSourceData = static_cast<const char*>(DownloadResult.GetData());
                            const size_t ModuleSourceLength = DownloadResult.GetDataLength();
                            const std::string ModuleSource
                                = (ModuleSourceData != nullptr) ? std::string(ModuleSourceData, ModuleSourceLength) : std::string();

                            const csp::common::String CanonicalPath = ngxscript::BuildCanonicalAssetPath(ScriptAsset, *AssetCollections);
                            const std::string ReloadSuccessMessage = fmt::format("NgxScript Trace: Reloaded module '{}'.", CanonicalPath.c_str());
                            LogSystem.LogMsg(csp::common::LogLevel::Log, ReloadSuccessMessage.c_str());

                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources[CanonicalPath.c_str()] = ModuleSource;
                            }

                            ScriptModulesLoaded.store(false);
                            // Rebuild context to clear module cache then refresh module source set for the active space.
                            RebuildContext();
                            LoadScriptModules();
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

csp::common::String NgxScriptSystem::GetUIDrawablesJsonForTesting(const csp::common::String& EntityId) const
{
    std::scoped_lock UILock(UIMutex);
    return UIRuntime->GetDrawablesJson(EntityId.c_str()).c_str();
}
#endif

void NgxScriptSystem::SetUIViewportSize(float Width, float Height)
{
    std::scoped_lock UILock(UIMutex);
    UIRuntime->SetViewportSize(Width, Height);
}

void NgxScriptSystem::SetUITextMeasureCallback(UITextMeasureCallback InCallback)
{
    std::scoped_lock UILock(UIMutex);
    if (!InCallback)
    {
        UIRuntime->SetTextMeasureCallback(nullptr);
        return;
    }

    UIRuntime->SetTextMeasureCallback(
        [InCallback = std::move(InCallback)](const csp::common::String& Text, float FontSize, const csp::common::String& FontWeight) {
        float Width = 0.0f;
        float Height = 0.0f;
        InCallback(Text, FontSize, FontWeight, Width, Height);
        return csp::common::Vector2(Width, Height);
    });
}

csp::common::String NgxScriptSystem::DrainPendingUITextMeasureRequests()
{
    std::scoped_lock UILock(UIMutex);
    return UIRuntime->DrainPendingTextMeasureRequestsJson().c_str();
}

bool NgxScriptSystem::SubmitUITextMeasureResults(const csp::common::String& ResultsJson)
{
    std::scoped_lock UILock(UIMutex);
    return UIRuntime->SubmitTextMeasureResultsJson(ResultsJson.c_str());
}

bool NgxScriptSystem::FlushPendingCodeComponentUI()
{
    static constexpr const char* SNIPPET = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.tick === 'function') {\n"
                                           "    globalThis.scriptRegistry.tick(0);\n"
                                           "}\n";
    return EvaluateSnippet(SNIPPET, "<ngx-codecomponent-flush-pending-ui>");
}

csp::common::String NgxScriptSystem::DrainPendingUIUpdates()
{
    std::scoped_lock UILock(UIMutex);
    return UIRuntime->DrainPendingUpdatesJson().c_str();
}

csp::common::String NgxScriptSystem::DrainPendingDebugDraws()
{
    std::vector<DebugDrawLineCommand> Drained;
    {
        std::scoped_lock Lock(DebugDrawMutex);
        Drained.swap(DebugDrawCommands);
    }

    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    Writer.StartArray();
    for (const auto& Cmd : Drained)
    {
        Writer.StartObject();
        Writer.Key("op");
        Writer.String("line");
        Writer.Key("start");
        Writer.StartArray();
        Writer.Double(Cmd.StartX);
        Writer.Double(Cmd.StartY);
        Writer.Double(Cmd.StartZ);
        Writer.EndArray();
        Writer.Key("end");
        Writer.StartArray();
        Writer.Double(Cmd.EndX);
        Writer.Double(Cmd.EndY);
        Writer.Double(Cmd.EndZ);
        Writer.EndArray();
        Writer.Key("color");
        Writer.StartArray();
        Writer.Double(Cmd.R);
        Writer.Double(Cmd.G);
        Writer.Double(Cmd.B);
        Writer.Double(Cmd.A);
        Writer.EndArray();
        Writer.Key("width");
        Writer.Double(Cmd.Width);
        Writer.EndObject();
    }
    Writer.EndArray();
    return Buffer.GetString();
}

void NgxScriptSystem::UnmountUIForInactiveEntities(const std::unordered_set<std::string>& ActiveEntityIds)
{
    std::scoped_lock UILock(UIMutex);
    if (!UIRuntime)
    {
        return;
    }

    const size_t StaleCount = UIRuntime->UnmountEntitiesNotIn(ActiveEntityIds);
    if (StaleCount > 0)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("NgxScriptSystem Trace: UnmountUIForInactiveEntities reclaimed {} orphan UI mount(s).", StaleCount).c_str());
    }
}

void NgxScriptSystem::SetUIDebugModeEnabled(bool bEnabled)
{
    std::scoped_lock UILock(UIMutex);
    if (UIRuntime)
    {
        UIRuntime->SetDebugModeEnabled(bEnabled);
        const std::string Message
            = fmt::format("NgxScript Trace: UI debug overlay {}.", bEnabled ? "enabled" : "disabled");
        LogSystem.LogMsg(csp::common::LogLevel::Log, Message.c_str());
    }
}

bool NgxScriptSystem::IsUIDebugModeEnabled() const
{
    std::scoped_lock UILock(UIMutex);
    return UIRuntime ? UIRuntime->IsDebugModeEnabled() : false;
}

bool NgxScriptSystem::DispatchUIAction(const csp::common::String& EntityId, const csp::common::String& HandlerId, const csp::common::String& EventDataJson)
{
    if (EntityId.IsEmpty() || HandlerId.IsEmpty())
    {
        return false;
    }

    // Look up the handler JSValue via the C++-owned handler table. We never
    // evaluate a JS snippet for each event — the old path chained parser ->
    // eval -> scriptRegistry.dispatchUIAction and blew the WASM stack on burst
    // clicks. Now we call the captured JS function directly with JS_Call.
    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }
    JSContext* Ctx = Context->ctx;

    JSValueConst HandlerFn = JS_UNDEFINED;
    {
        std::scoped_lock UILock(UIMutex);
        if (!UIRuntime)
        {
            return false;
        }
        HandlerFn = UIRuntime->GetHandler(std::string(EntityId.c_str()), std::string(HandlerId.c_str()));
    }

    if (!JS_IsFunction(Ctx, HandlerFn))
    {
        return false;
    }

    JSValue EventObj = JS_UNDEFINED;
    if (!EventDataJson.IsEmpty())
    {
        EventObj = JS_ParseJSON(Ctx, EventDataJson.c_str(), EventDataJson.Length(), "<ngx-dispatch-ui-event>");
        if (JS_IsException(EventObj))
        {
            JS_FreeValue(Ctx, EventObj);
            EventObj = JS_NULL;
        }
    }
    else
    {
        EventObj = JS_NULL;
    }

    // Hand off to the registry's batch-aware wrapper so signal effects still
    // flush in a single batch() pass. The wrapper sets thisEntity context too.
    JSValue GlobalObj = JS_GetGlobalObject(Ctx);
    JSValue Registry = JS_GetPropertyStr(Ctx, GlobalObj, "scriptRegistry");
    bool bResult = false;
    if (JS_IsObject(Registry))
    {
        JSValue Dispatcher = JS_GetPropertyStr(Ctx, Registry, "dispatchUIActionDirect");
        if (JS_IsFunction(Ctx, Dispatcher))
        {
            JSValue EntityIdStr = JS_NewString(Ctx, EntityId.c_str());
            JSValueConst Args[3];
            Args[0] = EntityIdStr;
            Args[1] = HandlerFn;
            Args[2] = EventObj;
            JSValue CallResult = JS_Call(Ctx, Dispatcher, Registry, 3, Args);
            if (!JS_IsException(CallResult))
            {
                bResult = JS_ToBool(Ctx, CallResult) > 0;
            }
            else
            {
                JSValue ExceptionValue = JS_GetException(Ctx);
                const char* ExceptionStr = JS_ToCString(Ctx, ExceptionValue);
                LogSystem.LogMsg(csp::common::LogLevel::Error,
                    std::string("NgxScript: dispatchUIActionDirect threw: ").append(ExceptionStr ? ExceptionStr : "<unknown>").c_str());
                if (ExceptionStr != nullptr)
                {
                    JS_FreeCString(Ctx, ExceptionStr);
                }
                JS_FreeValue(Ctx, ExceptionValue);
            }
            JS_FreeValue(Ctx, CallResult);
            JS_FreeValue(Ctx, EntityIdStr);
        }
        JS_FreeValue(Ctx, Dispatcher);
    }
    JS_FreeValue(Ctx, Registry);
    JS_FreeValue(Ctx, GlobalObj);
    JS_FreeValue(Ctx, EventObj);

    return bResult;
}

void NgxScriptSystem::ClearAllEntityEventListeners()
{
    if (ActiveRealtimeEngine == nullptr)
    {
        return;
    }
    for (size_t i = 0; i < ActiveRealtimeEngine->GetNumEntities(); ++i)
    {
        if (auto* Entity = ActiveRealtimeEngine->GetEntityByIndex(i))
        {
            Entity->GetScriptInterface()->ClearEventListeners();
        }
    }
}

void NgxScriptSystem::FireEntityEvent(const csp::common::String& EntityId, const csp::common::String& EventName, const csp::common::String& PayloadJson)
{
    if (EntityId.IsEmpty())
    {
        return;
    }
    // EntityId is a numeric string (e.g. "12345"). Embedding it unquoted makes it a
    // JS number literal. EventName is a C++ constant. PayloadJson is standard JSON
    // (double-quoted strings only) so it is safe to embed inside single quotes.
    const std::string Snippet = fmt::format(
        R"(if (typeof globalThis.__ngxFireEntityEvent === "function") {{ globalThis.__ngxFireEntityEvent({}, "{}", JSON.parse('{}')); }})",
        EntityId.c_str(), EventName.c_str(), PayloadJson.c_str());
    EvaluateSnippet(Snippet.c_str(), "<fire-entity-event>");
}

bool NgxScriptSystem::FireKeyboardEvent(const csp::common::String& EventType, const csp::common::String& Key, const csp::common::String& Code,
    bool Repeat, bool AltKey, bool CtrlKey, bool ShiftKey, bool MetaKey)
{
    if (EventType.IsEmpty())
    {
        return false;
    }

    const std::string Snippet = fmt::format(
        "if (typeof globalThis.__cspDispatchKeyboardEvent === 'function') {{ globalThis.__cspDispatchKeyboardEvent({{ type: '{}', key: '{}', code: "
        "'{}', repeat: {}, altKey: {}, ctrlKey: {}, shiftKey: {}, metaKey: {} }}); }}",
        EscapeJSStringLiteral(EventType.c_str()), EscapeJSStringLiteral(Key.c_str()), EscapeJSStringLiteral(Code.c_str()), Repeat ? "true" : "false",
        AltKey ? "true" : "false", CtrlKey ? "true" : "false", ShiftKey ? "true" : "false", MetaKey ? "true" : "false");
    return EvaluateSnippet(Snippet.c_str(), "<fire-keyboard-event>");
}

bool NgxScriptSystem::FireMouseEvent(const csp::common::String& EventType, int32_t Button, int32_t Buttons, int32_t PointerId,
    const csp::common::String& PointerType, bool AltKey, bool CtrlKey, bool ShiftKey, bool MetaKey)
{
    if (EventType.IsEmpty())
    {
        return false;
    }

    const std::string Snippet = fmt::format(
        "if (typeof globalThis.__cspDispatchMouseEvent === 'function') {{ globalThis.__cspDispatchMouseEvent({{ type: '{}', button: {}, buttons: {}, "
        "pointerId: {}, pointerType: '{}', altKey: {}, ctrlKey: {}, shiftKey: {}, metaKey: {} }}); }}",
        EscapeJSStringLiteral(EventType.c_str()), Button, Buttons, PointerId, EscapeJSStringLiteral(PointerType.c_str()),
        AltKey ? "true" : "false", CtrlKey ? "true" : "false", ShiftKey ? "true" : "false", MetaKey ? "true" : "false");
    return EvaluateSnippet(Snippet.c_str(), "<fire-mouse-event>");
}

void NgxScriptSystem::SetLocalPlayerCameraState(const csp::common::Vector3& Position, const csp::common::Vector4& Rotation,
    const csp::common::Vector3& Forward, const csp::common::Vector3& ForwardFlat, const csp::common::Vector3& Right,
    const csp::common::Vector3& RightFlat, const csp::common::Vector3& Up)
{
    std::scoped_lock CameraLock(CameraStateMutex);
    LocalPlayerCameraPosition = Position;
    LocalPlayerCameraRotation = Rotation;
    LocalPlayerCameraForward = Forward;
    LocalPlayerCameraForwardFlat = ForwardFlat;
    LocalPlayerCameraRight = Right;
    LocalPlayerCameraRightFlat = RightFlat;
    LocalPlayerCameraUp = Up;
}

csp::common::Vector3 NgxScriptSystem::GetLocalPlayerCameraPosition() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraPosition;
}

csp::common::Vector4 NgxScriptSystem::GetLocalPlayerCameraRotation() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraRotation;
}

csp::common::Vector3 NgxScriptSystem::GetLocalPlayerCameraForward() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraForward;
}

csp::common::Vector3 NgxScriptSystem::GetLocalPlayerCameraForwardFlat() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraForwardFlat;
}

csp::common::Vector3 NgxScriptSystem::GetLocalPlayerCameraRight() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraRight;
}

csp::common::Vector3 NgxScriptSystem::GetLocalPlayerCameraRightFlat() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraRightFlat;
}

csp::common::Vector3 NgxScriptSystem::GetLocalPlayerCameraUp() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerCameraUp;
}

void NgxScriptSystem::SetLocalPlayerXrActive(bool bActive)
{
    std::scoped_lock CameraLock(CameraStateMutex);
    LocalPlayerXrActive = bActive;
}

bool NgxScriptSystem::GetLocalPlayerXrActive() const
{
    std::scoped_lock CameraLock(CameraStateMutex);
    return LocalPlayerXrActive;
}

void NgxScriptSystem::RebuildContext()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: RebuildContext begin.");

    // Clear stale JS callbacks before destroying the context. qjs::Value destructors
    // call JS_FreeValue which must execute while the owning context is still alive.
    ClearAllEntityEventListeners();
    {
        std::scoped_lock UILock(UIMutex);
        UIRuntime->Clear();
    }

    {
        std::scoped_lock ContextLock(ContextMutex);
        Context.reset();

        if (!Runtime)
        {
            Runtime = std::make_unique<qjs::Runtime>();
           //ApplyNgxRuntimeLimits(*Runtime);
        }

        // Collect any cyclic garbage left over from the old context before
        // creating a new one, to reclaim memory held by signal/effect cycles.
        JS_RunGC(Runtime->rt);

        static constexpr size_t GC_HEAP_CAP = 4 * 1024 * 1024; // 4 MB
        JS_SetGCThreshold(Runtime->rt, GC_HEAP_CAP);

        Context = std::make_unique<qjs::Context>(*Runtime);
    }

    InstallModuleLoader();
    InstallHostBindings();
    ContextGeneration.fetch_add(1);
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: RebuildContext complete.");
}

void NgxScriptSystem::TeardownContext()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: TeardownContext.");
    ClearAllEntityEventListeners();
    std::scoped_lock ContextLock(ContextMutex);
    if (AssetBinding && Context)
    {
        AssetBinding->RejectAndCleanupPendingPromises(Context->ctx);
    }
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

            if (RequestedModule == PREACT_SIGNALS_CORE_MODULE)
            {
                const std::string ResolveMessage = fmt::format("NgxScript Trace: Resolved built-in module '{}'.", RequestedModule);
                LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                return qjs::Context::ModuleData { RequestedModule, csp::systems::SignalsScriptCode.c_str(), std::nullopt };
            }

            {
                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                auto StaticSourceIt = StaticModuleSources.find(RequestedModule);
                if (StaticSourceIt != StaticModuleSources.end())
                {
                    const std::string ResolveMessage = fmt::format("NgxScript Trace: Resolved static module '{}'.", RequestedModule);
                    LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                    return qjs::Context::ModuleData { RequestedModule, StaticSourceIt->second, std::nullopt };
                }

                auto SourceIt = LoadedModuleSources.find(RequestedModule);
                if (SourceIt != LoadedModuleSources.end())
                {
                    const std::string ResolveMessage
                        = fmt::format("NgxScript Trace: Resolved module '{}' from in-memory source map.", RequestedModule);
                    LogSystem.LogMsg(csp::common::LogLevel::Verbose, ResolveMessage.c_str());
                    return qjs::Context::ModuleData { RequestedModule, SourceIt->second, std::nullopt };
                }
            }

            const std::string MissingMessage = fmt::format("NgxScript: Missing module '{}' (module source may still be loading).", RequestedModule);
            LogSystem.LogMsg(csp::common::LogLevel::Warning, MissingMessage.c_str());

            const std::string ThrowingModule = "throw new Error('NgxScript module not found: " + EscapeJSStringLiteral(RequestedModule) + "');";
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
    CSPModule.function("__warn", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Warning, JoinArgs(Args).c_str()); });
    CSPModule.function("__error", [this](qjs::rest<std::string> Args) { LogSystem.LogMsg(csp::common::LogLevel::Error, JoinArgs(Args).c_str()); });
    CSPModule.function("__setUIDebugMode", [this](bool bEnabled) { if (UIRuntime) { UIRuntime->SetDebugModeEnabled(bEnabled); } });
    CSPModule.function("__getLocalPlayerCameraPosition", [this]() { return ToJSVector(GetLocalPlayerCameraPosition()); });
    CSPModule.function("__getLocalPlayerCameraRotation", [this]() { return ToJSVector(GetLocalPlayerCameraRotation()); });
    CSPModule.function("__getLocalPlayerCameraForward", [this]() { return ToJSVector(GetLocalPlayerCameraForward()); });
    CSPModule.function("__getLocalPlayerCameraForwardFlat", [this]() { return ToJSVector(GetLocalPlayerCameraForwardFlat()); });
    CSPModule.function("__getLocalPlayerCameraRight", [this]() { return ToJSVector(GetLocalPlayerCameraRight()); });
    CSPModule.function("__getLocalPlayerCameraRightFlat", [this]() { return ToJSVector(GetLocalPlayerCameraRightFlat()); });
    CSPModule.function("__getLocalPlayerCameraUp", [this]() { return ToJSVector(GetLocalPlayerCameraUp()); });
    CSPModule.function("__isXrActive", [this]() { return GetLocalPlayerXrActive(); });
    CSPModule.function("__resolveEntityQuery",
        [this](const std::string& QueryJson) -> std::string
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return "";
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            const auto ResolvedEntityId = ResolveEntityIdFromQueryJson(QueryJson, Entities);
            return ResolvedEntityId.has_value() ? std::to_string(*ResolvedEntityId) : std::string();
        });
    CSPModule.function("__getEntitySnapshot",
        [this](const std::string& EntityIdText) -> std::string
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return "";
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return "";
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return "";
            }

            return BuildEntitySnapshotJson(*Entity);
        });
    CSPModule.function("__getComponentSnapshot",
        [this](const std::string& EntityIdText, int32_t ComponentTypeValue, int32_t ComponentIndex) -> std::string
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return "";
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return "";
            }

            if (ComponentTypeValue <= 0)
            {
                return "";
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return "";
            }

            auto* Component = FindComponentByTypeAndIndex(*Entity, static_cast<csp::multiplayer::ComponentType>(ComponentTypeValue), ComponentIndex);
            if (Component == nullptr)
            {
                return "";
            }

            return BuildComponentSnapshotJson(*Component);
        });
    CSPModule.function("__setEntityPosition",
        [this](const std::string& EntityIdText, double X, double Y, double Z) -> bool
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return false;
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return false;
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return false;
            }

            return Entity->SetPosition(csp::common::Vector3 { static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z) });
        });
    CSPModule.function("__setEntityRotation",
        [this](const std::string& EntityIdText, double X, double Y, double Z, double W) -> bool
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return false;
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return false;
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return false;
            }

            return Entity->SetRotation(
                csp::common::Vector4 { static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z), static_cast<float>(W) });
        });
    CSPModule.function("__setEntityScale",
        [this](const std::string& EntityIdText, double X, double Y, double Z) -> bool
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return false;
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return false;
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return false;
            }

            return Entity->SetScale(csp::common::Vector3 { static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z) });
        });
    CSPModule.function("__setEntityName",
        [this](const std::string& EntityIdText, const std::string& Name) -> bool
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return false;
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return false;
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return false;
            }

            return Entity->SetName(Name.c_str());
        });
    CSPModule.function("__setEntityThirdPartyRef",
        [this](const std::string& EntityIdText, const std::string& ThirdPartyRef) -> bool
        {
            if (ActiveRealtimeEngine == nullptr)
            {
                return false;
            }

            const auto EntityId = TryParseEntityId(EntityIdText);
            if (!EntityId.has_value())
            {
                return false;
            }

            EntityUpdateLockGuard LockGuard(ActiveRealtimeEngine);
            const auto Entities = CollectEntities(ActiveRealtimeEngine);
            auto* Entity = FindEntityById(Entities, *EntityId);
            if (Entity == nullptr)
            {
                return false;
            }

            return Entity->SetThirdPartyRef(ThirdPartyRef.c_str());
        });
    CSPModule.function("__uiMount",
        [this](const std::string& EntityIdText, qjs::Value TreeValue) -> bool
        {
            std::scoped_lock UILock(UIMutex);
            if (!UIRuntime || TreeValue.ctx == nullptr)
            {
                return false;
            }
            return UIRuntime->Mount(EntityIdText, TreeValue.ctx, TreeValue.v);
        });
    CSPModule.function("__uiUnmount",
        [this](const std::string& EntityIdText) -> bool
        {
            std::scoped_lock UILock(UIMutex);
            return UIRuntime ? UIRuntime->Unmount(EntityIdText) : false;
        });
    CSPModule.function("__drawLine",
        [this](double StartX, double StartY, double StartZ, double EndX, double EndY, double EndZ, double R, double G, double B, double A,
            double Width)
        {
            std::scoped_lock Lock(DebugDrawMutex);
            DebugDrawCommands.push_back({ StartX, StartY, StartZ, EndX, EndY, EndZ, R, G, B, A, Width });
        });
    static constexpr const char* HOST_BINDINGS_SCRIPT = R"(
import * as csp from "csp";
globalThis.csp = csp;

// Format a single console argument. For Error objects we use the stack
// property (when available) so callers get a useful trace. Plain objects
// are serialised via a depth-limited, cycle-safe iterator so that deep
// reactive graphs (signals holding entity references, etc.) cannot blow
// the WASM stack. Unbounded JSON.stringify was the cause of repeated
// "Maximum call stack size exceeded" tick crashes.
const __CSP_FORMAT_MAX_DEPTH = 6;
function __cspFormatArg(value) {
    if (value instanceof Error) {
        return typeof value.stack === 'string' && value.stack.length > 0
            ? value.stack
            : String(value);
    }
    if (value !== null && typeof value === 'object') {
        try {
            const seen = new WeakSet();
            const sanitize = (node, depth) => {
                if (node === null) return null;
                const t = typeof node;
                if (t === 'string' || t === 'number' || t === 'boolean') return node;
                if (t === 'function') return '[Function]';
                if (t === 'undefined') return undefined;
                if (t === 'bigint' || t === 'symbol') return String(node);
                if (t !== 'object') return String(node);
                if (depth >= __CSP_FORMAT_MAX_DEPTH) return '[Object]';
                if (seen.has(node)) return '[Circular]';
                seen.add(node);
                if (Array.isArray(node)) {
                    const out = [];
                    const limit = Math.min(node.length, 64);
                    for (let i = 0; i < limit; i += 1) {
                        try { out.push(sanitize(node[i], depth + 1)); }
                        catch { out.push('[Error]'); }
                    }
                    if (node.length > limit) out.push(`[+${node.length - limit} more]`);
                    return out;
                }
                const out = {};
                let count = 0;
                for (const k of Object.keys(node)) {
                    if (count >= 32) { out['...'] = '[+more]'; break; }
                    try { out[k] = sanitize(node[k], depth + 1); }
                    catch { out[k] = '[Error]'; }
                    count += 1;
                }
                return out;
            };
            return JSON.stringify(sanitize(value, 0));
        } catch { return String(value); }
    }
    return String(value);
}

globalThis.console = {
    log:   (...args) => csp.__log(...args.map(__cspFormatArg)),
    warn:  (...args) => csp.__warn(...args.map(__cspFormatArg)),
    error: (...args) => csp.__error(...args.map(__cspFormatArg)),
};

const __cspAnimationFrameState = {
    nextId: 1,
    // id -> { callback, entityId } — entityId is the owning code component
    // at registration time (null for rAFs registered outside a component).
    callbacks: new Map(),
};

globalThis.__cspRafPendingCount = 0;

globalThis.requestAnimationFrame = (callback) => {
    if (typeof callback !== 'function') {
        throw new TypeError('requestAnimationFrame callback must be a function');
    }

    const id = __cspAnimationFrameState.nextId++;
    const entityId = globalThis.__cspCurrentEntityId ?? null;
    __cspAnimationFrameState.callbacks.set(id, { callback, entityId });
    globalThis.__cspRafPendingCount = __cspAnimationFrameState.callbacks.size;
    return id;
};

globalThis.cancelAnimationFrame = (handle) => {
    const id = Math.trunc(Number(handle));
    if (!Number.isFinite(id)) {
        return;
    }

    __cspAnimationFrameState.callbacks.delete(id);
    globalThis.__cspRafPendingCount = __cspAnimationFrameState.callbacks.size;
};

globalThis.__cspDispatchAnimationFrames = (timestampMs) => {
    if (__cspAnimationFrameState.callbacks.size === 0) {
        return;
    }

    const frameEntries = Array.from(__cspAnimationFrameState.callbacks.values());
    __cspAnimationFrameState.callbacks.clear();
    globalThis.__cspRafPendingCount = 0;

    const registry = globalThis.scriptRegistry;
    const isEntityRegistered = registry && typeof registry.isEntityRegistered === 'function'
        ? (id) => registry.isEntityRegistered(id)
        : null;

    // Track which entities we've already warned about this tick so one dead
    // animation loop doesn't flood the log with a message per frame.
    const warnedDeadEntities = globalThis.__cspRafWarnedDeadEntities
        ?? (globalThis.__cspRafWarnedDeadEntities = new Set());

    for (const entry of frameEntries) {
        // Skip callbacks owned by a code component that is no longer registered —
        // animation loops must not outlive their owning script. rAFs registered
        // without an entity context (entityId === null) always run.
        if (entry.entityId != null && isEntityRegistered && !isEntityRegistered(entry.entityId)) {
            if (!warnedDeadEntities.has(entry.entityId)) {
                warnedDeadEntities.add(entry.entityId);
                csp.__log(`NgxScript Trace: Skipping rAF callback for deregistered entity ${entry.entityId}.`);
            }
            continue;
        }

        // If the entity is alive again, clear the warn flag so a future death is logged again.
        if (entry.entityId != null) {
            warnedDeadEntities.delete(entry.entityId);
        }

        const previousEntityId = globalThis.__cspCurrentEntityId;
        if (entry.entityId != null) {
            globalThis.__cspCurrentEntityId = entry.entityId;
        }
        try {
            entry.callback(timestampMs);
        } catch (error) {
            csp.__error(error instanceof Error && error.stack ? error.stack : String(error));
        } finally {
            globalThis.__cspCurrentEntityId = previousEntityId;
        }
    }
};

const __cspCreateInputDevice = (pressedStore, valueNormalizer) => {
    const listeners = new Map();

    const getTypeListeners = (type, createIfMissing = false) => {
        if (!listeners.has(type) && createIfMissing) {
            listeners.set(type, new Map());
        }
        return listeners.get(type) ?? null;
    };

    return {
        on(type, callback) {
            if (typeof type !== 'string' || typeof callback !== 'function') {
                throw new TypeError('input.on(type, callback) expects a string event type and function callback');
            }

            const entityId = globalThis.__cspCurrentEntityId;
            if (!entityId) {
                throw new Error('input listeners must be registered while a code component is initializing');
            }

            const listenersByEntity = getTypeListeners(type, true);
            if (!listenersByEntity.has(entityId)) {
                listenersByEntity.set(entityId, new Set());
            }
            listenersByEntity.get(entityId).add(callback);
            return true;
        },

        off(type, callback) {
            const listenersByEntity = getTypeListeners(type, false);
            if (!listenersByEntity || typeof callback !== 'function') {
                return false;
            }

            let removed = false;
            for (const callbacks of listenersByEntity.values()) {
                if (callbacks.delete(callback)) {
                    removed = true;
                }
            }
            return removed;
        },

        isPressed(value) {
            return pressedStore.has(valueNormalizer(value));
        },

        dispatch(type, event) {
            const listenersByEntity = getTypeListeners(type, false);
            if (!listenersByEntity) {
                return;
            }

            const callbacks = [];
            for (const entityCallbacks of listenersByEntity.values()) {
                callbacks.push(...entityCallbacks);
            }

            for (const callback of callbacks) {
                try {
                    callback(event);
                } catch (error) {
                    csp.__error(error instanceof Error && error.stack ? error.stack : String(error));
                }
            }
        },

        removeListenersForEntity(entityId) {
            for (const listenersByEntity of listeners.values()) {
                listenersByEntity.delete(entityId);
            }
        }
    };
};

const __cspKeyboardPressed = new Set();
const __cspMousePressed = new Set();

const __cspKeyboard = __cspCreateInputDevice(__cspKeyboardPressed, (value) => String(value ?? ''));
const __cspMouse = __cspCreateInputDevice(__cspMousePressed, (value) => Number(value ?? -1));

globalThis.__cspInput = {
    keyboard: __cspKeyboard,
    mouse: __cspMouse,
    removeListenersForEntity(entityId) {
        __cspKeyboard.removeListenersForEntity(entityId);
        __cspMouse.removeListenersForEntity(entityId);
    }
};

globalThis.__cspDispatchKeyboardEvent = (event) => {
    if (!event || typeof event.type !== 'string') {
        return false;
    }

    const normalizedKey = String(event.key ?? '');
    if (event.type === 'keydown') {
        __cspKeyboardPressed.add(normalizedKey);
    } else if (event.type === 'keyup') {
        __cspKeyboardPressed.delete(normalizedKey);
    }

    __cspKeyboard.dispatch(event.type, Object.freeze({
        type: event.type,
        key: normalizedKey,
        code: String(event.code ?? ''),
        repeat: !!event.repeat,
        altKey: !!event.altKey,
        ctrlKey: !!event.ctrlKey,
        shiftKey: !!event.shiftKey,
        metaKey: !!event.metaKey,
    }));
    return true;
};

globalThis.__cspDispatchMouseEvent = (event) => {
    if (!event || typeof event.type !== 'string') {
        return false;
    }

    const normalizedButton = Number(event.button ?? 0);
    if (event.type === 'mousedown') {
        __cspMousePressed.add(normalizedButton);
    } else if (event.type === 'mouseup') {
        __cspMousePressed.delete(normalizedButton);
    }

    __cspMouse.dispatch(event.type, Object.freeze({
        type: event.type,
        button: normalizedButton,
        buttons: Number(event.buttons ?? 0),
        pointerId: Number(event.pointerId ?? 0),
        pointerType: String(event.pointerType ?? 'mouse'),
        altKey: !!event.altKey,
        ctrlKey: !!event.ctrlKey,
        shiftKey: !!event.shiftKey,
        metaKey: !!event.metaKey,
    }));
    return true;
};
)";

    const qjs::Value EvalResult = Context->eval(HOST_BINDINGS_SCRIPT, "<ngx-host-bindings>", JS_EVAL_TYPE_MODULE);
    if (EvalResult.isException())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Failed to install host bindings.");
        return;
    }

    csp::multiplayer::NgxEntityScriptBinding EntityBinding(ActiveRealtimeEngine, LogSystem, true);
    EntityBinding.BindToContext(*Context);

    if (!AssetBinding)
    {
        auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
        AssetBinding = std::make_unique<csp::multiplayer::NgxAssetScriptBinding>(AssetSystem, LogSystem);
    }
    AssetBinding->BindToContext(*Context, ActiveSpaceId);

    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxScript Trace: Host bindings installed.");
}

void NgxScriptSystem::DrainPendingJobs()
{
    static constexpr int32_t MAX_PENDING_JOBS_PER_PUMP = 64;

#ifdef CSP_WASM
    // Never block the browser thread waiting for another script operation to
    // release the context. Foundation ticks can safely skip one pump and retry
    // on the next interval.
    std::unique_lock<std::mutex> ContextLock(ContextMutex, std::defer_lock);
    if (!ContextLock.try_lock())
    {
        return;
    }
#else
    std::scoped_lock ContextLock(ContextMutex);
#endif
    if (!Runtime || !Context)
    {
        return;
    }

    int32_t ExecutedJobCount = 0;
    while (Runtime->isJobPending())
    {
        if (ExecutedJobCount >= MAX_PENDING_JOBS_PER_PUMP)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Warning,
                fmt::format(
                    "NgxScript: pending job budget exceeded ({} jobs in one pump); stopping drain to avoid a runaway loop.",
                    MAX_PENDING_JOBS_PER_PUMP)
                    .c_str());
            break;
        }

        Runtime->executePendingJob();
        ++ExecutedJobCount;
    }

    // QuickJS reference-counting cannot collect cyclic garbage (signals ↔ effects,
    // Proxy chains, etc.). JS_RunGC runs the full mark-and-sweep cycle collector.
    // We also reset the threshold afterwards to prevent the adaptive 1.5× growth
    // from allowing heap unbounded growth inside the fixed WASM address space.
    static constexpr uint32_t GC_INTERVAL_TICKS = 180; // ~3 s at 60 fps
    static constexpr size_t   GC_HEAP_CAP        = 4 * 1024 * 1024;
    if (++GcTickCounter >= GC_INTERVAL_TICKS)
    {
        GcTickCounter = 0;
        JS_RunGC(Runtime->rt);
        JS_SetGCThreshold(Runtime->rt, GC_HEAP_CAP);
    }
}

void NgxScriptSystem::LoadScriptModules()
{
    const std::string LoadStartMessage = fmt::format("NgxScript Trace: LoadScriptModules(spaceId='{}').", ActiveSpaceId.c_str());
    LogSystem.LogMsg(csp::common::LogLevel::Log, LoadStartMessage.c_str());

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        TrackedScriptAssetCollectionIds.clear();
    }

    if (ActiveSpaceId.IsEmpty())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript Trace: LoadScriptModules aborted (no active space).");
        ScriptModulesLoaded.store(true);
        return;
    }

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript Trace: LoadScriptModules aborted (CSPFoundation not initialised).");
        ScriptModulesLoaded.store(true);
        return;
    }

    const uint64_t Generation = SessionGeneration.load();
    auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!AssetSystem)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: AssetSystem unavailable, cannot load script modules.");
        ScriptModulesLoaded.store(true);
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
                ScriptModulesLoaded.store(true);
                return;
            }

            {
                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                TrackedScriptAssetCollectionIds.clear();
                for (size_t Index = 0; Index < AssetCollectionIds->Size(); ++Index)
                {
                    TrackedScriptAssetCollectionIds.insert((*AssetCollectionIds)[Index].c_str());
                }
            }

            auto BeginDownloads = [this, Generation, AssetSystem, AssetCollections](const std::vector<csp::systems::Asset>& ScriptAssets)
            {
                const size_t TotalAssets = ScriptAssets.size();
                if (TotalAssets == 0)
                {
                    LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: No JavaScript module assets found for active space.");
                    ScriptModulesLoaded.store(true);
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
                                ScriptModulesLoaded.store(true);
                                return;
                            }

                            const char* ModuleSourceData = static_cast<const char*>(DownloadResult.GetData());
                            const size_t ModuleSourceLength = DownloadResult.GetDataLength();
                            const std::string ModuleSource
                                = (ModuleSourceData != nullptr) ? std::string(ModuleSourceData, ModuleSourceLength) : std::string();

                            const csp::common::String CanonicalPath = ngxscript::BuildCanonicalAssetPath(ScriptAsset, *AssetCollections);
                            (*DownloadedSources)[CanonicalPath.c_str()] = ModuleSource;
                            {
                                std::scoped_lock ModuleLock(ModuleSourcesMutex);
                                LoadedModuleSources[CanonicalPath.c_str()] = ModuleSource;
                            }
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
                            ScriptModulesLoaded.store(true);
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
                        ScriptModulesLoaded.store(true);
                        return;
                    }

                    const auto ScriptLibraryAssetsArray = ScriptLibraryAssetsResult.GetAssets();
                    std::vector<csp::systems::Asset> ScriptLibraryAssets;
                    ScriptLibraryAssets.reserve(ScriptLibraryAssetsArray.Size());
                    for (size_t Index = 0; Index < ScriptLibraryAssetsArray.Size(); ++Index)
                    {
                        ScriptLibraryAssets.emplace_back(ScriptLibraryAssetsArray[Index]);
                    }

                    const std::string DiscoveredScriptLibrariesMessage
                        = fmt::format("NgxScript Trace: Discovered {} script library asset(s) for active space.", ScriptLibraryAssets.size());
                    LogSystem.LogMsg(csp::common::LogLevel::Log, DiscoveredScriptLibrariesMessage.c_str());

                    if (!ScriptLibraryAssets.empty())
                    {
                        BeginDownloads(ScriptLibraryAssets);
                        return;
                    }

                    LogSystem.LogMsg(csp::common::LogLevel::Warning,
                        "NgxScript: No script library assets found for active space. Ensure scripts are uploaded as EAssetType::SCRIPT_LIBRARY.");
                    ScriptModulesLoaded.store(true);
                });
        });
}

void NgxScriptSystem::FetchAssetCollectionMapForSpace(uint64_t Generation, std::function<void(std::shared_ptr<AssetCollectionMap>)> Callback) const
{
    const std::string QueryCollectionsMessage = fmt::format("NgxScript Trace: Querying asset collections for spaceId='{}'.", ActiveSpaceId.c_str());
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

    AssetSystem->FindAssetCollections(nullptr, nullptr, nullptr, nullptr, nullptr, csp::common::Array<csp::common::String> { ActiveSpaceId }, nullptr,
        nullptr,
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

            const std::string CollectionsMessage = fmt::format("NgxScript Trace: Resolved {} asset collection(s).", ResultMap->Size());
            LogSystem.LogMsg(csp::common::LogLevel::Log, CollectionsMessage.c_str());
            Callback(ResultMap);
        });
}

bool NgxScriptSystem::IsGenerationCurrent(uint64_t Generation) const { return SessionGeneration.load() == Generation; }

bool NgxScriptSystem::EvaluateGlobalScript(const std::string& ScriptText, const char* DebugName)
{
#ifdef CSP_WASM
    // Registry/bootstrap sync during foundation tick should never stall the browser
    // thread waiting for other script work to finish. If the context is busy, let
    // the caller retry on a later tick instead of hard-freezing startup.
    std::unique_lock<std::mutex> ContextLock(ContextMutex, std::defer_lock);
    if (!ContextLock.try_lock())
    {
        LastEvaluationDeferred.store(true);
        return false;
    }
#else
    std::scoped_lock ContextLock(ContextMutex);
#endif
    LastEvaluationDeferred.store(false);
    if (!Context)
    {
        return false;
    }

#ifdef CSP_WASM
    RebaseQuickJsStackCheck(*Runtime);
#endif

    const qjs::Value EvalResult = Context->eval(ScriptText, DebugName, JS_EVAL_TYPE_GLOBAL);
    if (!EvalResult.isException())
    {
        return true;
    }

    std::string ErrorMessage = "NgxScript: JavaScript execution failed.";
    try
    {
        qjs::Value ExceptionValue = Context->getException();
        ErrorMessage += " ";
        ErrorMessage += ExceptionValue.as<std::string>();

        // Append the stack trace when the exception is an Error object.
        if (ExceptionValue.isError())
        {
            try
            {
                const std::string StackString = ExceptionValue["stack"].as<std::string>();
                if (!StackString.empty())
                {
                    ErrorMessage += "\n";
                    ErrorMessage += StackString;
                }
            }
            catch (...)
            {
            }
        }
    }
    catch (...)
    {
    }

    LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMessage.c_str());
    return false;
}

bool NgxScriptSystem::EvaluateModuleScript(const std::string& ScriptText, const char* DebugName)
{
#ifdef CSP_WASM
    std::unique_lock<std::mutex> ContextLock(ContextMutex, std::defer_lock);
    if (!ContextLock.try_lock())
    {
        LastEvaluationDeferred.store(true);
        return false;
    }
#else
    std::scoped_lock ContextLock(ContextMutex);
#endif
    LastEvaluationDeferred.store(false);
    if (!Context)
    {
        return false;
    }

#ifdef CSP_WASM
    RebaseQuickJsStackCheck(*Runtime);
#endif

    const qjs::Value EvalResult = Context->eval(ScriptText, DebugName, JS_EVAL_TYPE_MODULE);
    if (!EvalResult.isException())
    {
        const std::string SuccessMessage = fmt::format("NgxScript Trace: Module evaluation succeeded ({}).", DebugName);
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, SuccessMessage.c_str());
        return true;
    }

    std::string ErrorMessage = "NgxScript: Module evaluation failed.";

    try
    {
        qjs::Value ExceptionValue = Context->getException();
        ErrorMessage += " ";
        ErrorMessage += ExceptionValue.as<std::string>();

        if (ExceptionValue.isError())
        {
            try
            {
                const std::string StackString = ExceptionValue["stack"].as<std::string>();
                if (!StackString.empty())
                {
                    ErrorMessage += "\n";
                    ErrorMessage += StackString;
                }
            }
            catch (...)
            {
            }
        }
    }
    catch (...)
    {
    }

    LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMessage.c_str());
    return false;
}

bool NgxScriptSystem::HasModuleSource(const csp::common::String& ModulePath) const
{
    const std::string ModulePathStd = ModulePath.c_str();
    if (ModulePathStd.empty())
    {
        return false;
    }

    if (ModulePathStd == PREACT_SIGNALS_CORE_MODULE)
    {
        return true;
    }

    std::scoped_lock ModuleLock(ModuleSourcesMutex);
    const bool bFoundInLoaded = (LoadedModuleSources.find(ModulePathStd) != LoadedModuleSources.end());
    const bool bFoundInStatic = (StaticModuleSources.find(ModulePathStd) != StaticModuleSources.end());
    return bFoundInLoaded || bFoundInStatic;
}

bool NgxScriptSystem::EvaluateSnippet(const csp::common::String& ScriptText, const csp::common::String& DebugName)
{
    const char* DebugNameValue = DebugName.IsEmpty() ? "<ngx-snippet>" : DebugName.c_str();
    return EvaluateGlobalScript(ScriptText.c_str(), DebugNameValue);
}

void NgxScriptSystem::PumpPendingJobs()
{
    bool bExpectedInactive = false;
    if (!PendingJobPumpActive.compare_exchange_strong(bExpectedInactive, true))
    {
        return;
    }

#ifdef CSP_WASM
    if (Runtime)
    {
        RebaseQuickJsStackCheck(*Runtime);
    }
#endif

    DrainPendingJobs();
    PendingJobPumpActive.store(false);
}

bool NgxScriptSystem::AreScriptModulesLoaded() const { return ScriptModulesLoaded.load(); }

uint64_t NgxScriptSystem::GetContextGeneration() const { return ContextGeneration.load(); }

bool NgxScriptSystem::WasLastEvaluationDeferred() const { return LastEvaluationDeferred.load(); }

bool NgxScriptSystem::TickAnimationFrame(double TimestampMs)
{
    if (!std::isfinite(TimestampMs))
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxScript: TickAnimationFrame received non-finite timestamp.");
        return false;
    }

    bool bSuccess = true;
    {
#ifdef CSP_WASM
        // Never block the browser thread waiting for script context work to finish.
        // requestAnimationFrame can safely drop a frame and retry on the next tick.
        std::unique_lock<std::mutex> ContextLock(ContextMutex, std::defer_lock);
        if (!ContextLock.try_lock())
        {
            return false;
        }

        RebaseQuickJsStackCheck(*Runtime);
#else
        std::scoped_lock ContextLock(ContextMutex);
#endif
        if (!Context)
        {
            return false;
        }

        try
        {
            int32_t PendingCount = 0;
            try
            {
                PendingCount = Context->global()["__cspRafPendingCount"].as<int32_t>();
            }
            catch (...)
            {
                PendingCount = 0;
            }

            if (PendingCount > 0)
            {
                const qjs::Value DispatchValue = Context->global()["__cspDispatchAnimationFrames"];
                if (!JS_IsUndefined(DispatchValue.v) && !JS_IsNull(DispatchValue.v))
                {
                    std::function<void(double)> Dispatch = DispatchValue.as<std::function<void(double)>>();
                    Dispatch(static_cast<double>(TimestampMs));
                }
            }
        }
        catch (const qjs::exception&)
        {
            bSuccess = false;

            std::string ErrorMessage = "NgxScript: Animation frame dispatch failed.";
            try
            {
                qjs::Value ExceptionValue = Context->getException();
                ErrorMessage += " ";
                ErrorMessage += ExceptionValue.as<std::string>();

                if (ExceptionValue.isError())
                {
                    try
                    {
                        const std::string StackString = ExceptionValue["stack"].as<std::string>();
                        if (!StackString.empty())
                        {
                            ErrorMessage += "\n";
                            ErrorMessage += StackString;
                        }
                    }
                    catch (...)
                    {
                    }
                }
            }
            catch (...)
            {
            }

            LogSystem.LogMsg(csp::common::LogLevel::Error, ErrorMessage.c_str());
        }
        catch (...)
        {
            bSuccess = false;
            LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxScript: Animation frame dispatch failed with an unknown error.");
        }
    }

    PumpPendingJobs();
    return bSuccess;
}

csp::common::String NgxScriptSystem::SyncCodeComponentSchema(const csp::common::String& EntityId)
{
    if (EntityId.IsEmpty())
    {
        return EMPTY_JSON_OBJECT_STRING;
    }

    if (!ScriptModulesLoaded.load())
    {
        return EMPTY_JSON_OBJECT_STRING;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '{}';\n"
          "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.syncCodeComponentSchema === 'function') {\n"
          "    const __cspResult = globalThis.scriptRegistry.syncCodeComponentSchema('"
        + EntityIdEscaped
        + "');\n"
          "    const __cspSerializable = (__cspResult === undefined || __cspResult === null) ? {} : __cspResult;\n"
          "    try {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = JSON.stringify(__cspSerializable);\n"
          "    } catch (_error) {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '{}';\n"
          "    }\n"
          "}\n";

    auto ReadJsonResultString = [this]() -> std::string
    {
        std::scoped_lock ContextLock(ContextMutex);
        if (!Context)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }

        try
        {
            const qjs::Value JsonResult = Context->global()[CODECOMPONENT_JSON_RESULT_SLOT];
            return JsonResult.as<std::string>();
        }
        catch (...)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }
    };

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-schema-bridge>"))
    {
        PumpPendingJobs();
        return EMPTY_JSON_OBJECT_STRING;
    }

    PumpPendingJobs();
    std::string JsonResultString = ReadJsonResultString();

    // If module import completed during pending jobs, one immediate retry returns the resolved schema payload
    // rather than requiring a second user click.
    if (JsonResultString == EMPTY_JSON_OBJECT_STRING)
    {
        if (EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-schema-bridge-retry>"))
        {
            PumpPendingJobs();
            JsonResultString = ReadJsonResultString();
        }
        else
        {
            PumpPendingJobs();
        }
    }

    return csp::common::String(JsonResultString.c_str());
}

bool NgxScriptSystem::AddCodeComponent(const csp::common::String& EntityId, const csp::common::String& PayloadJson)
{
    const std::string AddRequestMessage
        = fmt::format("NgxScript Trace: AddCodeComponent(entityId='{}', payloadLength={}) called.", EntityId.c_str(), PayloadJson.Length());
    LogSystem.LogMsg(csp::common::LogLevel::Verbose, AddRequestMessage.c_str());

    if (EntityId.IsEmpty())
    {
        return false;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage
            = fmt::format("NgxScript Trace: AddCodeComponent(entityId='{}') deferred until script modules finish loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return true;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string PayloadJsonEscaped = EscapeJSStringLiteral(PayloadJson.c_str());
    const std::string Snippet = "{\n"
          "globalThis." + std::string(CODECOMPONENT_BOOL_RESULT_SLOT)
        + " = false;\n"
          "let __cspPayload = {};\n"
          "try {\n"
          "    __cspPayload = JSON.parse('"
        + PayloadJsonEscaped
        + "');\n"
          "} catch (_error) {\n"
          "    __cspPayload = {};\n"
          "}\n"
          "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.addCodeComponent === 'function') {\n"
          "    globalThis."
        + std::string(CODECOMPONENT_BOOL_RESULT_SLOT) + " = !!globalThis.scriptRegistry.addCodeComponent('" + EntityIdEscaped
        + "', __cspPayload);\n"
          "}\n"
          "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-add-bridge>"))
    {
        return false;
    }

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    try
    {
        const qjs::Value BoolResult = Context->global()[CODECOMPONENT_BOOL_RESULT_SLOT];
        const bool bResult = BoolResult.as<bool>();
        const std::string AddResultMessage = fmt::format("NgxScript Trace: AddCodeComponent(entityId='{}') -> {}.", EntityId.c_str(), bResult);
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, AddResultMessage.c_str());
        return bResult;
    }
    catch (...)
    {
        return false;
    }
}

csp::common::String NgxScriptSystem::SyncCodeComponentAttributes(const csp::common::String& EntityId, const csp::common::String& AttributesJson)
{
    if (EntityId.IsEmpty())
    {
        return EMPTY_JSON_OBJECT_STRING;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage
            = fmt::format("NgxScript Trace: SyncCodeComponentAttributes(entityId='{}') deferred while script modules are loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return EMPTY_JSON_OBJECT_STRING;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string AttributesJsonEscaped = EscapeJSStringLiteral(AttributesJson.c_str());
    const std::string Snippet = "{\n"
          "globalThis." + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '{}';\n"
          "let __cspAttributes = {};\n"
          "try {\n"
          "    __cspAttributes = JSON.parse('"
        + AttributesJsonEscaped
        + "');\n"
          "} catch (_error) {\n"
          "    __cspAttributes = {};\n"
          "}\n"
          "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.syncCodeComponentAttributes === 'function') {\n"
          "    const __cspResult = globalThis.scriptRegistry.syncCodeComponentAttributes('"
        + EntityIdEscaped
        + "', __cspAttributes);\n"
          "    const __cspSerializable = (__cspResult === undefined || __cspResult === null) ? {} : __cspResult;\n"
          "    try {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = JSON.stringify(__cspSerializable);\n"
          "    } catch (_error) {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '{}';\n"
          "    }\n"
          "}\n"
          "}\n";

    auto ReadJsonResultString = [this]() -> std::string
    {
        std::scoped_lock ContextLock(ContextMutex);
        if (!Context)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }

        try
        {
            const qjs::Value JsonResult = Context->global()[CODECOMPONENT_JSON_RESULT_SLOT];
            return JsonResult.as<std::string>();
        }
        catch (...)
        {
            return EMPTY_JSON_OBJECT_STRING;
        }
    };

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-attributes-bridge>"))
    {
        PumpPendingJobs();
        return EMPTY_JSON_OBJECT_STRING;
    }

    PumpPendingJobs();
    std::string JsonResultString = ReadJsonResultString();

    if (JsonResultString == EMPTY_JSON_OBJECT_STRING)
    {
        if (EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-sync-attributes-bridge-retry>"))
        {
            PumpPendingJobs();
            JsonResultString = ReadJsonResultString();
        }
        else
        {
            PumpPendingJobs();
        }
    }

    return csp::common::String(JsonResultString.c_str());
}

bool NgxScriptSystem::UpdateAttributeForEntity(
    const csp::common::String& EntityId, const csp::common::String& Key, const csp::common::String& ValueJson)
{
    if (EntityId.IsEmpty() || Key.IsEmpty())
    {
        return false;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage
            = fmt::format("NgxScript Trace: UpdateAttributeForEntity(entityId='{}', key='{}') deferred while script modules are loading.",
                EntityId.c_str(), Key.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return true;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string KeyEscaped = EscapeJSStringLiteral(Key.c_str());
    const std::string ValueJsonEscaped = EscapeJSStringLiteral(ValueJson.c_str());
    const std::string Snippet = "{\n"
          "globalThis." + std::string(CODECOMPONENT_BOOL_RESULT_SLOT)
        + " = false;\n"
          "let __cspValue = null;\n"
          "try {\n"
          "    __cspValue = JSON.parse('"
        + ValueJsonEscaped
        + "');\n"
          "} catch (_error) {\n"
          "    __cspValue = null;\n"
          "}\n"
          "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.updateAttributeForEntity === 'function') {\n"
          "    globalThis."
        + std::string(CODECOMPONENT_BOOL_RESULT_SLOT) + " = !!globalThis.scriptRegistry.updateAttributeForEntity('" + EntityIdEscaped + "', '"
        + KeyEscaped
        + "', __cspValue);\n"
          "}\n"
          "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-update-attribute-bridge>"))
    {
        return false;
    }

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    try
    {
        const qjs::Value BoolResult = Context->global()[CODECOMPONENT_BOOL_RESULT_SLOT];
        return BoolResult.as<bool>();
    }
    catch (...)
    {
        return false;
    }
}

bool NgxScriptSystem::RemoveCodeComponent(const csp::common::String& EntityId)
{
    if (EntityId.IsEmpty())
    {
        return false;
    }

    if (!ScriptModulesLoaded.load())
    {
        const std::string DeferredMessage
            = fmt::format("NgxScript Trace: RemoveCodeComponent(entityId='{}') deferred while script modules are loading.", EntityId.c_str());
        LogSystem.LogMsg(csp::common::LogLevel::Verbose, DeferredMessage.c_str());
        return true;
    }

    const std::string EntityIdEscaped = EscapeJSStringLiteral(EntityId.c_str());
    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_BOOL_RESULT_SLOT)
        + " = false;\n"
          "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.removeCodeComponent === 'function') {\n"
          "    globalThis."
        + std::string(CODECOMPONENT_BOOL_RESULT_SLOT) + " = !!globalThis.scriptRegistry.removeCodeComponent('" + EntityIdEscaped
        + "');\n"
          "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-remove-bridge>"))
    {
        return false;
    }

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return false;
    }

    try
    {
        const qjs::Value BoolResult = Context->global()[CODECOMPONENT_BOOL_RESULT_SLOT];
        return BoolResult.as<bool>();
    }
    catch (...)
    {
        return false;
    }
}

csp::common::String NgxScriptSystem::DrainPendingSchemaSyncs()
{
    if (!Context)
    {
        return "[]";
    }

    const std::string Snippet = "globalThis." + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = '[]';\n"
          "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.drainPendingSchemaSyncs === 'function') {\n"
          "    try {\n"
          "        globalThis."
        + std::string(CODECOMPONENT_JSON_RESULT_SLOT)
        + " = JSON.stringify(globalThis.scriptRegistry.drainPendingSchemaSyncs());\n"
          "    } catch (_e) {}\n"
          "}\n";

    if (!EvaluateSnippet(Snippet.c_str(), "<ngx-codecomponent-drain-schema-syncs>"))
    {
        PumpPendingJobs();
        return "[]";
    }

    PumpPendingJobs();

    std::scoped_lock ContextLock(ContextMutex);
    if (!Context)
    {
        return "[]";
    }
    try
    {
        const qjs::Value Result = Context->global()[CODECOMPONENT_JSON_RESULT_SLOT];
        return csp::common::String(Result.as<std::string>().c_str());
    }
    catch (...)
    {
        return "[]";
    }
}

bool NgxScriptSystem::ExecuteModule(const csp::common::String& ModulePath)
{
    const std::string ModulePathStd = ModulePath.c_str();
    const std::string RunModuleMessage = fmt::format("NgxScript Trace: Executing module '{}'.", ModulePathStd);
    LogSystem.LogMsg(csp::common::LogLevel::Log, RunModuleMessage.c_str());

    {
        std::scoped_lock ModuleLock(ModuleSourcesMutex);
        const bool bFoundInLoaded = (LoadedModuleSources.find(ModulePathStd) != LoadedModuleSources.end());
        const bool bFoundInStatic = (StaticModuleSources.find(ModulePathStd) != StaticModuleSources.end());
        const bool bFoundInBuiltIn = (ModulePathStd == PREACT_SIGNALS_CORE_MODULE);
        if (!bFoundInLoaded && !bFoundInStatic && !bFoundInBuiltIn)
        {
            const std::string WarningMessage = fmt::format("NgxScript: Module '{}' not found in loaded or static module maps.", ModulePathStd);
            LogSystem.LogMsg(csp::common::LogLevel::Warning, WarningMessage.c_str());
            return false;
        }
    }

    const std::string Script = "import '" + EscapeJSStringLiteral(ModulePathStd) + "';";

    const bool bSuccess = EvaluateModuleScript(Script, "<ngx-module>");

    const std::string CompletedMessage = fmt::format("NgxScript Trace: Module '{}' {}.", ModulePathStd, bSuccess ? "completed" : "failed");
    LogSystem.LogMsg(csp::common::LogLevel::Log, CompletedMessage.c_str());
    return bSuccess;
}

} // namespace csp::systems
